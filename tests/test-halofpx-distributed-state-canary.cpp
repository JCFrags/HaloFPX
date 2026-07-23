#include "arg.h"
#include "common.h"
#include "ggml-rpc.h"
#include "llama.h"

#include <algorithm>
#include <array>
#include <charconv>
#include <chrono>
#include <clocale>
#include <cerrno>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <fcntl.h>
#include <fstream>
#include <string>
#include <sys/random.h>
#include <sys/stat.h>
#include <unistd.h>
#include <vector>

namespace fs = std::filesystem;

namespace {

struct canary_options {
    std::string mode;
    std::string result_label;
    fs::path root;
    std::array<uint8_t, 32> model {};
    std::array<uint8_t, 32> compatibility {};
    std::array<uint8_t, 32> plan {};
    std::array<uint8_t, 32> topology {};
    std::array<uint8_t, 32> placement {};
    std::array<uint8_t, 32> checkpoint {};
    std::array<uint8_t, 32> control_key {};
    std::array<uint8_t, 32> channel_binding {};
    size_t expected_prompt_tokens = 0;
    fs::path restore_gate_root;
};

#pragma pack(push, 1)
struct coordinator_receipt {
    uint8_t magic[8];
    uint32_t version;
    uint32_t reserved;
    uint64_t key_generation;
    uint64_t generation;
    uint64_t token_count;
    uint64_t token_boundary;
    uint32_t world_size;
    uint32_t logical_rank;
    uint8_t model_digest[32];
    uint8_t compatibility_root[32];
    uint8_t plan_digest[32];
    uint8_t topology_digest[32];
    uint8_t placement_digest[32];
    uint8_t checkpoint_digest[32];
    uint8_t token_prefix_digest[32];
    uint8_t component_manifest_digest[32];
    uint8_t channel_binding[32];
    uint8_t control_digest[32];
    uint8_t local_digest[32];
    uint8_t tokens_digest[32];
    uint8_t worker_object_digest[32];
    uint8_t tag[32];
};
#pragma pack(pop)

static_assert(sizeof(coordinator_receipt) == 504, "unexpected coordinator receipt size");

bool parse_hex(const std::string & text, std::array<uint8_t, 32> & output) {
    if (text.size() != 64) return false;
    auto nibble = [](char c) -> int {
        if (c >= '0' && c <= '9') return c - '0';
        if (c >= 'a' && c <= 'f') return c - 'a' + 10;
        return -1;
    };
    for (size_t i = 0; i < output.size(); ++i) {
        const int a = nibble(text[2*i]);
        const int b = nibble(text[2*i + 1]);
        if (a < 0 || b < 0) return false;
        output[i] = static_cast<uint8_t>((a << 4) | b);
    }
    return true;
}

std::string hex(const uint8_t value[32]) {
    static constexpr char digits[] = "0123456789abcdef";
    std::string result(64, '0');
    for (size_t i = 0; i < 32; ++i) {
        result[2*i] = digits[value[i] >> 4];
        result[2*i + 1] = digits[value[i] & 15];
    }
    return result;
}

bool take_option(std::vector<std::string> & args, const std::string & name, std::string & value) {
    for (size_t i = 1; i + 1 < args.size(); ++i) {
        if (args[i] == name) {
            value = args[i + 1];
            args.erase(args.begin() + i, args.begin() + i + 2);
            return true;
        }
    }
    return false;
}

bool load_control_file(const fs::path & path, canary_options & options) {
    if (!path.is_absolute()) return false;
    const int fd = open(path.c_str(), O_RDONLY | O_CLOEXEC | O_NOFOLLOW);
    if (fd < 0) return false;
    struct stat st {};
    if (fstat(fd, &st) != 0 || !S_ISREG(st.st_mode) || st.st_uid != geteuid() ||
        (st.st_mode & 0077) != 0 || st.st_size <= 0 || st.st_size > 256) {
        close(fd);
        return false;
    }
    std::array<char, 257> data {};
    size_t used = 0;
    while (used < data.size()) {
        const ssize_t n = read(fd, data.data() + used, data.size() - used);
        if (n < 0 && errno == EINTR) continue;
        if (n <= 0) break;
        used += static_cast<size_t>(n);
    }
    close(fd);
    if (used == data.size()) return false;
    const std::string text(data.data(), used);
    const size_t first = text.find('\n');
    const size_t second = first == std::string::npos ? first : text.find('\n', first + 1);
    if (first == std::string::npos || (second != std::string::npos && second + 1 != text.size())) return false;
    return parse_hex(text.substr(0, first), options.control_key) &&
        parse_hex(text.substr(first + 1, second == std::string::npos ? second : second - first - 1), options.channel_binding);
}

bool parse_canary_options(int argc, char ** argv, canary_options & options, std::vector<std::string> & args) {
    args.assign(argv, argv + argc);
    std::string root;
    std::string model;
    std::string compatibility;
    std::string plan;
    std::string topology;
    std::string placement;
    std::string checkpoint;
    std::string control_file;
    std::string expected_prompt_tokens;
    std::string rendezvous_root;
    std::string restore_gate_root;
    take_option(args, "--hfx-result-label", options.result_label);
    take_option(args, "--hfx-rendezvous-root", rendezvous_root);
    take_option(args, "--hfx-restore-gate-root", restore_gate_root);
    if (!restore_gate_root.empty()) {
        options.restore_gate_root = restore_gate_root;
        if (!options.restore_gate_root.is_absolute()) return false;
    }
    if (take_option(args, "--hfx-expected-prompt-tokens", expected_prompt_tokens)) {
        const auto * begin = expected_prompt_tokens.data();
        const auto * end = begin + expected_prompt_tokens.size();
        const auto parsed = std::from_chars(begin, end, options.expected_prompt_tokens);
        if (parsed.ec != std::errc() || parsed.ptr != end || options.expected_prompt_tokens == 0) return false;
    }
    const bool valid = take_option(args, "--hfx-mode", options.mode) &&
        (options.mode == "capture" || options.mode == "restore" || options.mode == "cold") &&
        take_option(args, "--hfx-artifact-root", root) && !(options.root = root).empty() && options.root.is_absolute() &&
        take_option(args, "--hfx-model-digest", model) && parse_hex(model, options.model) &&
        take_option(args, "--hfx-compatibility-root", compatibility) && parse_hex(compatibility, options.compatibility) &&
        take_option(args, "--hfx-plan-digest", plan) && parse_hex(plan, options.plan) &&
        take_option(args, "--hfx-topology-digest", topology) && parse_hex(topology, options.topology) &&
        take_option(args, "--hfx-placement-digest", placement) && parse_hex(placement, options.placement) &&
        take_option(args, "--hfx-checkpoint-digest", checkpoint) && parse_hex(checkpoint, options.checkpoint) &&
        take_option(args, "--hfx-control-file", control_file) && load_control_file(control_file, options);
    if (options.result_label.empty()) options.result_label = options.mode;
    return valid && !options.result_label.empty() &&
        std::all_of(options.result_label.begin(), options.result_label.end(), [](unsigned char c) {
            return (c >= 'a' && c <= 'z') || c == '-';
        });
}

template<class T>
bool write_vector(const fs::path & path, const std::vector<T> & value) {
    std::ofstream output(path, std::ios::binary | std::ios::trunc);
    const uint64_t count = value.size();
    output.write(reinterpret_cast<const char *>(&count), sizeof(count));
    if (!value.empty()) output.write(reinterpret_cast<const char *>(value.data()), value.size() * sizeof(T));
    output.flush();
    return output.good();
}

template<class T>
bool read_vector(const fs::path & path, std::vector<T> & value, uint64_t max_count) {
    std::ifstream input(path, std::ios::binary);
    uint64_t count = 0;
    input.read(reinterpret_cast<char *>(&count), sizeof(count));
    if (!input || count > max_count) return false;
    value.resize(count);
    if (count != 0) input.read(reinterpret_cast<char *>(value.data()), count * sizeof(T));
    return input.good() && input.peek() == std::ifstream::traits_type::eof();
}

bool write_receipt(const fs::path & path, const coordinator_receipt & value) {
    std::ofstream output(path, std::ios::binary | std::ios::trunc);
    output.write(reinterpret_cast<const char *>(&value), sizeof(value));
    output.flush();
    return output.good();
}

bool write_text(const fs::path & path, const std::string & value) {
    std::ofstream output(path, std::ios::binary | std::ios::trunc);
    output.write(value.data(), value.size());
    output.flush();
    return output.good();
}

bool read_receipt(const fs::path & path, coordinator_receipt & value) {
    std::ifstream input(path, std::ios::binary);
    input.read(reinterpret_cast<char *>(&value), sizeof(value));
    return input.good() && input.peek() == std::ifstream::traits_type::eof();
}

bool sha256(const void * data, size_t size, std::array<uint8_t, 32> & result) {
    result.fill(0);
    return ggml_backend_rpc_halofpx_state_sha256(data, size, result.data());
}

bool equal_32(const uint8_t a[32], const uint8_t b[32]) {
    uint8_t diff = 0;
    for (size_t i = 0; i < 32; ++i) diff |= a[i] ^ b[i];
    return diff == 0;
}

bool receipt_hmac(
        const std::array<uint8_t, 32> & key,
        const coordinator_receipt & receipt,
        std::array<uint8_t, 32> & result) {
    static constexpr char domain[] = "halofpx.coordinator-canary.v1";
    std::array<uint8_t, 64> ipad {};
    std::array<uint8_t, 64> opad {};
    for (size_t i = 0; i < ipad.size(); ++i) {
        const uint8_t b = i < key.size() ? key[i] : 0;
        ipad[i] = b ^ 0x36;
        opad[i] = b ^ 0x5c;
    }
    std::vector<uint8_t> inner(ipad.begin(), ipad.end());
    inner.insert(inner.end(), domain, domain + sizeof(domain));
    const auto * receipt_bytes = reinterpret_cast<const uint8_t *>(&receipt);
    inner.insert(inner.end(), receipt_bytes, receipt_bytes + sizeof(receipt));
    std::array<uint8_t, 32> mid {};
    if (!sha256(inner.data(), inner.size(), mid)) return false;
    std::vector<uint8_t> outer(opad.begin(), opad.end());
    outer.insert(outer.end(), mid.begin(), mid.end());
    return sha256(outer.data(), outer.size(), result);
}

struct decode_measurement {
    size_t n_batch = 0;
    size_t chunks = 0;
    size_t max_chunk = 0;
};

bool decode_tokens(
        llama_context * ctx,
        const std::vector<llama_token> & tokens,
        size_t count,
        decode_measurement * measurement = nullptr) {
    if (count == 0 || count > tokens.size()) return false;
    const size_t n_batch = llama_n_batch(ctx);
    if (n_batch == 0) return false;
    if (measurement) {
        measurement->n_batch = n_batch;
        measurement->chunks = 0;
        measurement->max_chunk = 0;
    }
    for (size_t offset = 0; offset < count; offset += n_batch) {
        const size_t chunk = std::min(n_batch, count - offset);
        if (measurement) {
            ++measurement->chunks;
            measurement->max_chunk = std::max(measurement->max_chunk, chunk);
        }
        llama_batch batch = llama_batch_get_one(
            const_cast<llama_token *>(tokens.data() + offset), static_cast<int32_t>(chunk));
        if (llama_decode(ctx, batch) != 0) return false;
    }
    return true;
}

double elapsed_ms(const std::chrono::steady_clock::time_point & start) {
    return std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - start).count();
}

std::vector<llama_token> suffix(
        llama_context * ctx,
        const std::vector<llama_token> & prefix,
        int n_predict) {
    std::vector<llama_token> result;
    llama_sampler * sampler = llama_sampler_init_greedy();
    llama_token replay = prefix.back();
    llama_batch batch = llama_batch_get_one(&replay, 1);
    if (llama_decode(ctx, batch) != 0) {
        llama_sampler_free(sampler);
        return {};
    }
    for (int i = 0; i < n_predict; ++i) {
        const llama_token token = llama_sampler_sample(sampler, ctx, -1);
        result.push_back(token);
        batch = llama_batch_get_one(const_cast<llama_token *>(&result.back()), 1);
        if (i + 1 < n_predict && llama_decode(ctx, batch) != 0) {
            result.clear();
            break;
        }
    }
    llama_sampler_free(sampler);
    return result;
}

ggml_backend_rpc_halofpx_state_identity make_identity(
        const canary_options & options,
        const std::vector<llama_token> & prefix,
        const std::array<uint8_t, 32> & attempt,
        const std::array<uint8_t, 32> & component_manifest) {
    ggml_backend_rpc_halofpx_state_identity value {};
    value.key_generation = 7;
    value.generation = 1;
    value.token_count = prefix.size() - 1;
    value.token_boundary = prefix.size() - 1;
    value.world_size = 2;
    value.logical_rank = 1;
    memcpy(value.model_digest, options.model.data(), 32);
    memcpy(value.compatibility_root, options.compatibility.data(), 32);
    memcpy(value.plan_digest, options.plan.data(), 32);
    memcpy(value.topology_digest, options.topology.data(), 32);
    memcpy(value.placement_digest, options.placement.data(), 32);
    memcpy(value.checkpoint_digest, options.checkpoint.data(), 32);
    ggml_backend_rpc_halofpx_state_sha256(prefix.data(), (prefix.size() - 1) * sizeof(prefix[0]), value.token_prefix_digest);
    memcpy(value.component_manifest_digest, component_manifest.data(), component_manifest.size());
    memcpy(value.attempt_nonce, attempt.data(), attempt.size());
    memcpy(value.channel_binding, options.channel_binding.data(), options.channel_binding.size());
    return value;
}

bool fresh_nonce(std::array<uint8_t, 32> & nonce) {
    size_t done = 0;
    while (done < nonce.size()) {
        const ssize_t n = getrandom(nonce.data() + done, nonce.size() - done, 0);
        if (n < 0 && errno == EINTR) continue;
        if (n <= 0) return false;
        done += static_cast<size_t>(n);
    }
    return true;
}

bool make_receipt(
        const ggml_backend_rpc_halofpx_state_identity & identity,
        const std::vector<uint8_t> & control,
        const std::vector<uint8_t> & local,
        const std::vector<llama_token> & tokens,
        const std::vector<uint8_t> & worker_object,
        const std::array<uint8_t, 32> & key,
        coordinator_receipt & receipt) {
    receipt = {};
    memcpy(receipt.magic, "HFXCOO1\0", 8);
    receipt.version = 1;
    receipt.key_generation = identity.key_generation;
    receipt.generation = identity.generation;
    receipt.token_count = identity.token_count;
    receipt.token_boundary = identity.token_boundary;
    receipt.world_size = identity.world_size;
    receipt.logical_rank = 0;
#define COPY_RECEIPT_ID(field) memcpy(receipt.field, identity.field, sizeof(receipt.field))
    COPY_RECEIPT_ID(model_digest);
    COPY_RECEIPT_ID(compatibility_root);
    COPY_RECEIPT_ID(plan_digest);
    COPY_RECEIPT_ID(topology_digest);
    COPY_RECEIPT_ID(placement_digest);
    COPY_RECEIPT_ID(checkpoint_digest);
    COPY_RECEIPT_ID(token_prefix_digest);
    COPY_RECEIPT_ID(component_manifest_digest);
    COPY_RECEIPT_ID(channel_binding);
#undef COPY_RECEIPT_ID
    std::array<uint8_t, 32> control_digest {};
    std::array<uint8_t, 32> local_digest {};
    std::array<uint8_t, 32> tokens_digest {};
    if (!sha256(control.data(), control.size(), control_digest) ||
        !sha256(local.data(), local.size(), local_digest) ||
        !sha256(tokens.data(), tokens.size() * sizeof(tokens[0]), tokens_digest)) return false;
    memcpy(receipt.control_digest, control_digest.data(), control_digest.size());
    memcpy(receipt.local_digest, local_digest.data(), local_digest.size());
    memcpy(receipt.tokens_digest, tokens_digest.data(), tokens_digest.size());
    if (worker_object.size() == 32) memcpy(receipt.worker_object_digest, worker_object.data(), 32);
    std::array<uint8_t, 32> tag {};
    if (!receipt_hmac(key, receipt, tag)) return false;
    memcpy(receipt.tag, tag.data(), tag.size());
    return true;
}

bool validate_receipt(
        const coordinator_receipt & supplied,
        const ggml_backend_rpc_halofpx_state_identity & identity,
        const std::vector<uint8_t> & control,
        const std::vector<uint8_t> & local,
        const std::vector<llama_token> & tokens,
        const std::vector<uint8_t> & worker_object,
        const std::array<uint8_t, 32> & key) {
    coordinator_receipt authenticated = supplied;
    std::array<uint8_t, 32> supplied_tag {};
    memcpy(supplied_tag.data(), authenticated.tag, supplied_tag.size());
    memset(authenticated.tag, 0, sizeof(authenticated.tag));
    std::array<uint8_t, 32> expected_tag {};
    if (!receipt_hmac(key, authenticated, expected_tag)) return false;
    if (!equal_32(supplied_tag.data(), expected_tag.data())) return false;
    coordinator_receipt expected {};
    if (!make_receipt(identity, control, local, tokens, worker_object, key, expected)) return false;
    return memcmp(&supplied, &expected, sizeof(supplied)) == 0;
}

} // namespace

static bool rendezvous(const fs::path & root, const std::string & ready, const std::string & proceed);

static int run_canary(int argc, char ** argv) {
    std::setlocale(LC_NUMERIC, "C");
    canary_options options;
    std::vector<std::string> args;
    if (!parse_canary_options(argc, argv, options, args)) return 2;
    std::vector<char *> cargs;
    for (auto & arg : args) cargs.push_back(arg.data());
    common_params params;
    params.prompt = "The quick brown fox";
    params.n_predict = 8;
    params.kv_unified = true;
    common_init();
    if (!common_params_parse(static_cast<int>(cargs.size()), cargs.data(), params, LLAMA_EXAMPLE_COMMON)) return 2;
    if (params.n_predict <= 0 || params.n_predict > 512) return 2;
    ggml_backend_load_all();
    static common_init_result_ptr resident_init;
    static bool resident_context_taken = false;
    if (!resident_init) resident_init = common_init_from_params(params);
    if (!resident_init || !resident_init->model() || !resident_init->context()) return 3;
    llama_context * ctx = nullptr;
    bool owns_ctx = false;
    if (!resident_context_taken) {
        ctx = resident_init->context();
        resident_context_taken = true;
    } else {
        ctx = llama_init_from_model(resident_init->model(), common_context_params_to_llama(params));
        owns_ctx = true;
    }
    if (!ctx) return 3;
    if (options.mode == "restore" && !options.restore_gate_root.empty() &&
        !rendezvous(options.restore_gate_root, "model-ready", "restore-authorized")) {
        if (owns_ctx) llama_free(ctx);
        return 15;
    }
    std::vector<llama_token> prefix;
    double prompt_ms = 0.0;
    double state_ms = 0.0;
    double generation_ms = 0.0;
    size_t coordinator_control_bytes = 0;
    size_t coordinator_local_bytes = 0;
    uint64_t worker_bytes = 0;
    uint32_t worker_components = 0;
    std::array<uint8_t, 32> control_diagnostic {};
    std::array<uint8_t, 32> local_diagnostic {};
    std::array<uint8_t, 32> manifest_diagnostic {};
    decode_measurement prompt_decode {};
    const fs::path checkpoint_root = options.root / hex(options.checkpoint.data());
    const fs::path control_path = checkpoint_root / "coordinator-control.bin";
    const fs::path local_path = checkpoint_root / "coordinator-local.bin";
    const fs::path tokens_path = checkpoint_root / "tokens.bin";
    const fs::path object_path = checkpoint_root / "worker-object-digest.bin";
    const fs::path receipt_path = checkpoint_root / "coordinator-receipt.bin";
    const fs::path suffix_path = checkpoint_root / (options.result_label + "-suffix.bin");
    const fs::path suffix_text_path = checkpoint_root / (options.result_label + "-suffix.txt");
    if (options.mode == "capture") {
        fs::create_directories(checkpoint_root);
        fs::permissions(checkpoint_root, fs::perms::owner_all, fs::perm_options::replace);
        prefix = common_tokenize(ctx, params.prompt, true);
        if (prefix.size() < 2 || (options.expected_prompt_tokens != 0 && prefix.size() != options.expected_prompt_tokens)) return 4;
        const auto prompt_start = std::chrono::steady_clock::now();
        if (!decode_tokens(ctx, prefix, prefix.size() - 1, &prompt_decode)) return 4;
        prompt_ms = elapsed_ms(prompt_start);
        const auto state_start = std::chrono::steady_clock::now();
        llama_state_seq_storage * storage = llama_state_seq_storage_init();
        const size_t control_size = llama_state_seq_get_size_ext(ctx, 0, LLAMA_STATE_SEQ_FLAGS_ON_DEVICE);
        std::vector<uint8_t> control(control_size);
        if (!storage || llama_state_seq_get_data_ext_storage(ctx, control.data(), control.size(), 0,
                LLAMA_STATE_SEQ_FLAGS_ON_DEVICE, storage) != control.size()) return 5;
        std::vector<uint8_t> local(llama_state_seq_storage_local_size(storage));
        if (local.empty() || llama_state_seq_storage_get_local(storage, local.data(), local.size()) != local.size()) return 6;
        const auto & key = options.control_key;
        std::array<uint8_t, 32> attempt {};
        std::array<uint8_t, 32> component_manifest {};
        if (!llama_state_seq_storage_halofpx_manifest_digest(storage, component_manifest.data())) return 7;
        if (!fresh_nonce(attempt)) return 7;
        const auto identity = make_identity(options, prefix, attempt, component_manifest);
        ggml_backend_rpc_halofpx_state_result captured {};
        if (!llama_state_seq_storage_halofpx_capture_remote(storage, &identity, key.data(), &captured)) return 7;
        std::vector<uint8_t> object(captured.object_digest, captured.object_digest + 32);
        coordinator_receipt receipt {};
        if (!make_receipt(identity, control, local, prefix, object, key, receipt)) return 8;
        memcpy(control_diagnostic.data(), receipt.control_digest, control_diagnostic.size());
        memcpy(local_diagnostic.data(), receipt.local_digest, local_diagnostic.size());
        memcpy(manifest_diagnostic.data(), receipt.component_manifest_digest, manifest_diagnostic.size());
        coordinator_control_bytes = control.size();
        coordinator_local_bytes = local.size();
        worker_bytes = captured.verified_bytes;
        worker_components = captured.verified_components;
        if (!write_vector(control_path, control) || !write_vector(local_path, local) ||
            !write_vector(tokens_path, prefix) || !write_vector(object_path, object) ||
            !write_receipt(receipt_path, receipt)) return 8;
        state_ms = elapsed_ms(state_start);
        const auto generation_start = std::chrono::steady_clock::now();
        const auto generated = suffix(ctx, prefix, params.n_predict);
        generation_ms = elapsed_ms(generation_start);
        const auto decoded = common_detokenize(ctx, generated, false);
        llama_state_seq_storage_free(storage);
        if (generated.size() != static_cast<size_t>(params.n_predict) ||
            !write_vector(suffix_path, generated) || !write_text(suffix_text_path, decoded)) return 8;
        std::printf("mode=capture label=%s coordinator_pid=%ld object=%s control_sha256=%s local_sha256=%s component_manifest_sha256=%s prompt_tokens=%zu saved_boundary=%zu n_batch=%zu prompt_chunks=%zu max_prompt_chunk=%zu prompt_ms=%.3f state_ms=%.3f generation_ms=%.3f coordinator_control_bytes=%zu coordinator_local_bytes=%zu worker_bytes=%llu worker_components=%u tokens=",
            options.result_label.c_str(), static_cast<long>(getpid()), hex(captured.object_digest).c_str(),
            hex(control_diagnostic.data()).c_str(), hex(local_diagnostic.data()).c_str(),
            hex(manifest_diagnostic.data()).c_str(), prefix.size(), prefix.size() - 1,
            prompt_decode.n_batch, prompt_decode.chunks, prompt_decode.max_chunk, prompt_ms, state_ms, generation_ms,
            coordinator_control_bytes, coordinator_local_bytes, static_cast<unsigned long long>(worker_bytes), worker_components);
        for (auto token : generated) std::printf("%d,", token);
        std::printf("\n");
        std::fflush(stdout);
        if (owns_ctx) llama_free(ctx);
        return 0;
    }
    std::vector<uint8_t> control;
    std::vector<uint8_t> local;
    std::vector<uint8_t> object;
    llama_context * run_ctx = ctx;
    llama_context * disposable_ctx = nullptr;
    std::string fallback_reason;
    if (options.mode == "cold") {
        prefix = common_tokenize(ctx, params.prompt, true);
        if (prefix.size() < 2 || (options.expected_prompt_tokens != 0 && prefix.size() != options.expected_prompt_tokens)) return 9;
        const auto prompt_start = std::chrono::steady_clock::now();
        if (!decode_tokens(ctx, prefix, prefix.size() - 1, &prompt_decode)) return 10;
        prompt_ms = elapsed_ms(prompt_start);
    } else {
        const auto state_start = std::chrono::steady_clock::now();
        coordinator_receipt receipt {};
        std::array<uint8_t, 32> attempt {};
        if (!fresh_nonce(attempt)) return 11;
        bool artifacts_valid = read_vector(control_path, control, UINT64_C(1) << 20) &&
            read_vector(local_path, local, UINT64_C(1) << 30) &&
            read_vector(tokens_path, prefix, 4096) && read_vector(object_path, object, 32) && object.size() == 32 &&
            read_receipt(receipt_path, receipt);
        std::array<uint8_t, 32> component_manifest {};
        ggml_backend_rpc_halofpx_state_identity identity {};
        if (artifacts_valid && prefix.size() >= 2 &&
            (options.expected_prompt_tokens == 0 || prefix.size() == options.expected_prompt_tokens)) {
            memcpy(component_manifest.data(), receipt.component_manifest_digest, component_manifest.size());
            identity = make_identity(options, prefix, attempt, component_manifest);
            artifacts_valid = validate_receipt(
                receipt, identity, control, local, prefix, object, options.control_key);
            if (artifacts_valid) {
                memcpy(control_diagnostic.data(), receipt.control_digest, control_diagnostic.size());
                memcpy(local_diagnostic.data(), receipt.local_digest, local_diagnostic.size());
                memcpy(manifest_diagnostic.data(), receipt.component_manifest_digest, manifest_diagnostic.size());
            }
        } else {
            artifacts_valid = false;
        }
        if (!artifacts_valid) {
            fallback_reason = "coordinator-artifact";
            prefix = common_tokenize(ctx, params.prompt, true);
            disposable_ctx = llama_init_from_model(resident_init->model(), common_context_params_to_llama(params));
            if (prefix.size() < 2 || (options.expected_prompt_tokens != 0 && prefix.size() != options.expected_prompt_tokens) || !disposable_ctx) {
                if (disposable_ctx) llama_free(disposable_ctx);
                return 11;
            }
            const auto prompt_start = std::chrono::steady_clock::now();
            if (!decode_tokens(disposable_ctx, prefix, prefix.size() - 1, &prompt_decode)) {
                llama_free(disposable_ctx);
                return 11;
            }
            prompt_ms = elapsed_ms(prompt_start);
            run_ctx = disposable_ctx;
        } else {
        disposable_ctx = llama_init_from_model(resident_init->model(), common_context_params_to_llama(params));
        if (!disposable_ctx) return 11;
        llama_state_seq_storage * storage = llama_state_seq_storage_init();
        bool ready_accepted = false;
        const auto & key = options.control_key;
        ggml_backend_rpc_halofpx_state_result ready {};
        if (!storage || llama_state_seq_prepare_data_ext_storage(disposable_ctx, control.data(), control.size(), 0,
                LLAMA_STATE_SEQ_FLAGS_ON_DEVICE, storage) != control.size()) {
            fallback_reason = "coordinator-prepare";
        } else if (llama_state_seq_storage_set_local(storage, local.data(), local.size()) != local.size()) {
            fallback_reason = "coordinator-local";
        } else if (!llama_state_seq_storage_halofpx_stage_remote(storage, &identity, object.data(), key.data(), &ready)) {
            fallback_reason = "worker-stage";
        } else {
            ready_accepted = true;
            worker_bytes = ready.verified_bytes;
            worker_components = ready.verified_components;
            ggml_backend_rpc_halofpx_state_result applied {};
            if (!llama_state_seq_storage_halofpx_commit_remote(storage, &identity, object.data(),
                    ready.worker_nonce, key.data(), &applied)) {
                fallback_reason = "worker-commit";
            } else if (llama_state_seq_set_data_ext_storage(disposable_ctx, control.data(), control.size(), 0,
                    LLAMA_STATE_SEQ_FLAGS_ON_DEVICE, storage) != control.size()) {
                fallback_reason = "coordinator-apply";
            }
        }
        if (!fallback_reason.empty() && ready_accepted) {
            ggml_backend_rpc_halofpx_state_result aborted {};
            llama_state_seq_storage_halofpx_abort_remote(&identity, ready.worker_nonce, key.data(), &aborted);
        }
        llama_state_seq_storage_free(storage);
        if (fallback_reason.empty()) {
            run_ctx = disposable_ctx;
        } else {
            llama_free(disposable_ctx);
            disposable_ctx = llama_init_from_model(resident_init->model(), common_context_params_to_llama(params));
            if (!disposable_ctx) {
                if (disposable_ctx) llama_free(disposable_ctx);
                return 12;
            }
            const auto prompt_start = std::chrono::steady_clock::now();
            if (!decode_tokens(disposable_ctx, prefix, prefix.size() - 1, &prompt_decode)) {
                llama_free(disposable_ctx);
                return 12;
            }
            prompt_ms = elapsed_ms(prompt_start);
            run_ctx = disposable_ctx;
        }
        }
        coordinator_control_bytes = control.size();
        coordinator_local_bytes = local.size();
        state_ms = elapsed_ms(state_start);
    }
    const auto generation_start = std::chrono::steady_clock::now();
    const auto generated = suffix(run_ctx, prefix, params.n_predict);
    generation_ms = elapsed_ms(generation_start);
    const auto decoded = common_detokenize(run_ctx, generated, false);
    if (disposable_ctx) llama_free(disposable_ctx);
    if (generated.size() != static_cast<size_t>(params.n_predict) ||
        !write_vector(suffix_path, generated) || !write_text(suffix_text_path, decoded)) return 13;
    std::printf("mode=%s label=%s coordinator_pid=%ld control_sha256=%s local_sha256=%s component_manifest_sha256=%s prompt_tokens=%zu saved_boundary=%zu n_batch=%zu prompt_chunks=%zu max_prompt_chunk=%zu prompt_ms=%.3f state_ms=%.3f generation_ms=%.3f coordinator_control_bytes=%zu coordinator_local_bytes=%zu worker_bytes=%llu worker_components=%u tokens=",
        options.mode.c_str(), options.result_label.c_str(), static_cast<long>(getpid()),
        hex(control_diagnostic.data()).c_str(), hex(local_diagnostic.data()).c_str(),
        hex(manifest_diagnostic.data()).c_str(), prefix.size(), prefix.size() - 1,
        static_cast<size_t>(llama_n_batch(run_ctx)),
        prompt_decode.chunks, prompt_decode.max_chunk, prompt_ms, state_ms, generation_ms,
        coordinator_control_bytes, coordinator_local_bytes, static_cast<unsigned long long>(worker_bytes), worker_components);
    for (auto token : generated) std::printf("%d,", token);
    if (!fallback_reason.empty()) std::printf(" fallback=cold reason=%s", fallback_reason.c_str());
    std::printf("\n");
    std::fflush(stdout);
    if (owns_ctx) llama_free(ctx);
    return 0;
}

static std::string option_value(const std::vector<std::string> & args, const std::string & name) {
    for (size_t i = 1; i + 1 < args.size(); ++i) {
        if (args[i] == name) return args[i + 1];
    }
    return {};
}

static int invoke_mode(
        const std::vector<std::string> & original,
        const std::string & mode,
        const std::string & label,
        const std::string & plan_override = {}) {
    std::vector<std::string> args = original;
    for (size_t i = 1; i + 1 < args.size();) {
        if (args[i] == "--hfx-sequence") {
            args.erase(args.begin() + i, args.begin() + i + 2);
            continue;
        }
        if (args[i] == "--hfx-mode") args[i + 1] = mode;
        if (args[i] == "--hfx-plan-digest" && !plan_override.empty()) args[i + 1] = plan_override;
        ++i;
    }
    args.push_back("--hfx-result-label");
    args.push_back(label);
    std::vector<char *> cargs;
    for (auto & arg : args) cargs.push_back(arg.data());
    return run_canary(static_cast<int>(cargs.size()), cargs.data());
}

static bool rendezvous(const fs::path & root, const std::string & ready, const std::string & proceed) {
    if (root.empty() || !root.is_absolute()) return false;
    fs::create_directories(root);
    fs::permissions(root, fs::perms::owner_all, fs::perm_options::replace);
    if (!write_text(root / ready, "ready\n")) return false;
    const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(120);
    while (std::chrono::steady_clock::now() < deadline) {
        if (fs::exists(root / proceed)) {
            std::error_code ec;
            fs::remove(root / ready, ec);
            fs::remove(root / proceed, ec);
            return !ec;
        }
        usleep(100000);
    }
    return false;
}

int main(int argc, char ** argv) {
    std::vector<std::string> args(argv, argv + argc);
    const std::string sequence = option_value(args, "--hfx-sequence");
    if (sequence.empty()) return run_canary(argc, argv);
    if (sequence == "residency1") {
        const int capture = invoke_mode(args, "capture", "capture");
        return capture == 0 ? invoke_mode(args, "cold", "cold") : capture;
    }
    if (sequence == "residency2") {
        const int restore = invoke_mode(args, "restore", "restore");
        if (restore != 0) return restore;
        const fs::path rendezvous_root = option_value(args, "--hfx-rendezvous-root");
        if (!rendezvous(rendezvous_root, "restore-ready", "worker-object-missing")) return 14;
        const int missing_result = invoke_mode(args, "restore", "missing");
        if (missing_result != 0) return missing_result;
        if (!rendezvous(rendezvous_root, "missing-done", "worker-object-restored")) return 14;
        return invoke_mode(args, "restore", "plan-mismatch", std::string(64, 'f'));
    }
    if (sequence == "residency3") return invoke_mode(args, "cold", "runtime-off");
    if (sequence == "capture-only") return invoke_mode(args, "capture", "capture");
    if (sequence == "restore-guarded") return invoke_mode(args, "restore", "restore");
    if (sequence == "diagnostic") {
        const int capture = invoke_mode(args, "capture", "capture");
        if (capture != 0) return capture;
        const fs::path rendezvous_root = option_value(args, "--hfx-rendezvous-root");
        if (!rendezvous(rendezvous_root, "capture-ready", "worker-restarted")) return 14;
        return invoke_mode(args, "restore", "restore");
    }
    return 2;
}
