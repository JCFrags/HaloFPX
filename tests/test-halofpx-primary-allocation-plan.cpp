#include "arg.h"
#include "common.h"
#include "ggml-backend.h"
#include "llama-ext.h"
extern "C" {
#include "sha256.h"
}

#include <algorithm>
#include <array>
#include <cerrno>
#include <cinttypes>
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <limits>
#include <memory>
#include <set>
#include <string>
#include <vector>

namespace {

constexpr const char * EXPECTED_FILE = "saricles-MiniMax-M2.7-REAP-172B-A10B-Q6_0_ROCMFPX_AGENT.gguf";
constexpr const char * EXPECTED_REPOSITORY = "rcmorano/saricles-MiniMax-M2.7-REAP-172B-A10B-ROCMFPX";
constexpr const char * EXPECTED_REVISION = "dba517197f2854f3d362529e13abddcdcad6c10b";
constexpr const char * EXPECTED_SHA256 = "96506ada918e60ca9a9cfde8a5437790e4453401a6a3e236e3f55e7bac3aaea6";
constexpr uint64_t EXPECTED_SIZE = UINT64_C(159873097824);
constexpr uint64_t RUNTIME_RESERVE = UINT64_C(16) * 1024 * 1024 * 1024;
constexpr uint64_t FRAGMENTATION_BPS = 1000; // 10%, policy assumption

struct device_budget {
    std::string name;
    std::string backend;
    uint64_t free_now = 0;
    uint64_t total = 0;
    uint64_t weights = 0;
    uint64_t context = 0;
    uint64_t compute = 0;
    uint64_t fragmentation = 0;
    uint64_t required = 0;
    uint64_t max_request = 0;
    uint64_t margin = 0;
};

bool validate_artifact_identity(const std::string & file, uint64_t size, const std::string & sha, std::string & reason) {
    if (file != EXPECTED_FILE) { reason = "artifact-name"; return false; }
    if (size != EXPECTED_SIZE) { reason = "artifact-size"; return false; }
    if (sha != EXPECTED_SHA256) { reason = "artifact-sha256"; return false; }
    return true;
}

bool validate_loader_accounting(
        const std::array<std::string, 2> & names,
        const std::array<std::string, 2> & backends,
        const llama_model_allocation_plan & plan,
        std::string & reason) {
    if (names != std::array<std::string, 2> { "RPC0", "ROCm0" }) {
        reason = "device-order"; return false;
    }
    if (backends != std::array<std::string, 2> { "RPC", "ROCm" }) {
        reason = "device-backend"; return false;
    }
    if (!plan.no_alloc) { reason = "not-no-alloc"; return false; }
    if (plan.use_mmap) { reason = "mmap-enabled"; return false; }
    if (!plan.arithmetic_ok) { reason = "plan-arithmetic"; return false; }
    if (plan.unknown_created_tensors != 0) { reason = "unknown-created-tensor"; return false; }
    if (plan.unaccounted_source_tensors != 0) { reason = "unaccounted-source-tensor"; return false; }
    if (!plan.complete) { reason = "incomplete-plan"; return false; }
    return true;
}

bool add_checked(uint64_t & dst, uint64_t value) {
    if (dst > UINT64_MAX - value) return false;
    dst += value;
    return true;
}

bool mul_div_ceil_checked(uint64_t value, uint64_t mul, uint64_t div, uint64_t & result) {
    if (div == 0 || (value != 0 && mul > UINT64_MAX / value)) return false;
    const uint64_t product = value * mul;
    if (product > UINT64_MAX - (div - 1)) return false;
    result = (product + div - 1) / div;
    return true;
}

std::string json_escape(const std::string & value) {
    std::string out;
    for (unsigned char c : value) {
        switch (c) {
            case '\\': out += "\\\\"; break;
            case '"': out += "\\\""; break;
            case '\n': out += "\\n"; break;
            case '\r': out += "\\r"; break;
            case '\t': out += "\\t"; break;
            default:
                if (c < 0x20) {
                    char tmp[7];
                    std::snprintf(tmp, sizeof(tmp), "\\u%04x", c);
                    out += tmp;
                } else {
                    out += static_cast<char>(c);
                }
        }
    }
    return out;
}

bool sha256_file(const std::string & path, std::string & result, std::string & reason) {
    std::ifstream in(path, std::ios::binary);
    if (!in) { reason = "artifact-open"; return false; }
    sha256_t state;
    sha256_init(&state);
    std::vector<unsigned char> buffer(8 * 1024 * 1024);
    while (in) {
        in.read(reinterpret_cast<char *>(buffer.data()), buffer.size());
        const std::streamsize got = in.gcount();
        if (got > 0) sha256_update(&state, buffer.data(), static_cast<size_t>(got));
    }
    if (!in.eof()) { reason = "artifact-read"; return false; }
    unsigned char digest[SHA256_DIGEST_SIZE];
    sha256_final(&state, digest);
    char hex[SHA256_DIGEST_SIZE * 2 + 1] = {};
    for (size_t i = 0; i < SHA256_DIGEST_SIZE; ++i) std::snprintf(hex + 2*i, 3, "%02x", digest[i]);
    result = hex;
    return true;
}

bool validate_budgets(std::vector<device_budget> & devices, std::string & reason) {
    for (auto & d : devices) {
        uint64_t planned = d.weights;
        if (!add_checked(planned, d.context) || !add_checked(planned, d.compute)) {
            reason = "planned-total-overflow"; return false;
        }
        if (!mul_div_ceil_checked(planned, FRAGMENTATION_BPS, 10000, d.fragmentation)) {
            reason = "fragmentation-overflow"; return false;
        }
        d.required = planned;
        if (!add_checked(d.required, d.fragmentation) || !add_checked(d.required, RUNTIME_RESERVE)) {
            reason = "required-total-overflow"; return false;
        }
        if (d.total == 0 || d.free_now > d.total) { reason = "device-capacity"; return false; }
        if (d.required > d.total) { reason = "required-margin"; return false; }
        d.margin = d.total - d.required;
        uint64_t single_with_margin = d.max_request;
        if (!add_checked(single_with_margin, d.context) || !add_checked(single_with_margin, d.compute) ||
                !add_checked(single_with_margin, d.fragmentation) || !add_checked(single_with_margin, RUNTIME_RESERVE)) {
            reason = "single-request-overflow"; return false;
        }
        if (single_with_margin > d.total) { reason = "single-request-margin"; return false; }
    }
    return true;
}

int self_test() {
    bool ok = true;
    auto expect = [&](std::vector<device_budget> input, bool wanted, const char * label) {
        std::string reason;
        const bool got = validate_budgets(input, reason);
        if (got != wanted) {
            std::fprintf(stderr, "self-test %s expected %s got %s (%s)\n", label,
                    wanted ? "pass" : "refusal", got ? "pass" : "refusal", reason.c_str());
            ok = false;
        }
    };
    expect({ {"RPC0", "RPC", 1, UINT64_C(128) << 30, UINT64_C(70) << 30,
              UINT64_C(2) << 30, UINT64_C(4) << 30, 0, 0, UINT64_C(40) << 30, 0} }, true, "correct");
    expect({ {"RPC0", "RPC", 1, UINT64_C(64) << 30, UINT64_C(60) << 30,
              UINT64_C(2) << 30, UINT64_C(4) << 30, 0, 0, UINT64_C(40) << 30, 0} }, false, "margin");
    expect({ {"RPC0", "RPC", 1, UINT64_MAX, UINT64_MAX,
              1, 0, 0, 0, 1, 0} }, false, "overflow");
    std::string reason;
    if (!validate_artifact_identity(EXPECTED_FILE, EXPECTED_SIZE, EXPECTED_SHA256, reason)) ok = false;
    reason.clear();
    if (validate_artifact_identity(EXPECTED_FILE, EXPECTED_SIZE, std::string(64, '0'), reason)) ok = false;
    llama_model_allocation_plan complete;
    complete.complete = true;
    complete.no_alloc = true;
    complete.arithmetic_ok = true;
    if (!validate_loader_accounting({ "RPC0", "ROCm0" }, { "RPC", "ROCm" }, complete, reason)) ok = false;
    reason.clear();
    if (validate_loader_accounting({ "ROCm0", "RPC0" }, { "ROCm", "RPC" }, complete, reason)) ok = false;
    auto unaccounted = complete;
    unaccounted.unaccounted_source_tensors = 1;
    reason.clear();
    if (validate_loader_accounting({ "RPC0", "ROCm0" }, { "RPC", "ROCm" }, unaccounted, reason)) ok = false;
    if (ok) std::puts("halofpx-primary-allocation-self-test: PASS (correct identity wrong-hash wrong-order unaccounted margin overflow)");
    return ok ? 0 : 1;
}

} // namespace

int main(int argc, char ** argv) {
    if (argc == 2 && std::strcmp(argv[1], "--self-test") == 0) return self_test();

    std::string endpoint;
    bool device_present = false;
    bool no_mmap_present = false;
    std::vector<std::string> filtered { argv[0] };
    for (int i = 1; i < argc; ++i) {
        const std::string arg = argv[i];
        if (arg == "--hfx-expected-rpc-endpoint") {
            if (++i >= argc) { std::fprintf(stderr, "allocation refusal: endpoint-argument\n"); return 2; }
            endpoint = argv[i];
            continue;
        }
        if (arg == "--device" || arg == "-dev" || arg.rfind("--device=", 0) == 0) device_present = true;
        if (arg == "--no-mmap") no_mmap_present = true;
        filtered.emplace_back(arg);
    }
    if (endpoint.empty() || endpoint.size() > 128) { std::fprintf(stderr, "allocation refusal: endpoint\n"); return 2; }

    std::vector<char *> cargs;
    for (auto & arg : filtered) cargs.push_back(arg.data());
    common_params params;
    common_init();
    if (!common_params_parse(static_cast<int>(cargs.size()), cargs.data(), params, LLAMA_EXAMPLE_COMMON)) return 2;

    std::string reason;
    std::error_code ec;
    const uint64_t file_size = std::filesystem::file_size(params.model.path, ec);
    if (ec) { std::fprintf(stderr, "allocation refusal: artifact-stat\n"); return 3; }
    std::string artifact_sha;
    if (!sha256_file(params.model.path, artifact_sha, reason) ||
            !validate_artifact_identity(std::filesystem::path(params.model.path).filename().string(), file_size, artifact_sha, reason)) {
        std::fprintf(stderr, "allocation refusal: %s\n", reason.c_str()); return 3;
    }

    if (!device_present || !no_mmap_present || params.use_mmap || params.split_mode != LLAMA_SPLIT_MODE_LAYER ||
            params.tensor_split[0] != 1.0f || params.tensor_split[1] != 1.0f || params.n_gpu_layers != 999 ||
            params.n_ctx != 4096 || params.n_batch != 512 || params.n_ubatch != 512 ||
            params.cache_type_k != GGML_TYPE_Q8_0 || params.cache_type_v != GGML_TYPE_Q8_0 ||
            !std::all_of(params.tensor_split + 2, params.tensor_split + llama_max_devices(), [](float v) { return v == 0.0f; })) {
        std::fprintf(stderr, "allocation refusal: frozen-loader-or-context-arguments\n"); return 3;
    }
    if (params.devices.size() != 3 || !params.devices[0] || !params.devices[1] || params.devices[2]) {
        std::fprintf(stderr, "allocation refusal: selected-device-count\n"); return 3;
    }

    std::vector<device_budget> budgets(2);
    const std::array<const char *, 2> expected_names { "RPC0", "ROCm0" };
    const std::array<const char *, 2> expected_backends { "RPC", "ROCm" };
    for (size_t i = 0; i < 2; ++i) {
        auto * dev = params.devices[i];
        auto * reg = ggml_backend_dev_backend_reg(dev);
        budgets[i].name = ggml_backend_dev_name(dev);
        budgets[i].backend = reg ? ggml_backend_reg_name(reg) : "";
        if (budgets[i].name != expected_names[i] || budgets[i].backend != expected_backends[i]) {
            std::fprintf(stderr, "allocation refusal: device-order-or-backend\n"); return 3;
        }
        if (i == 0 && endpoint != ggml_backend_dev_description(dev)) {
            std::fprintf(stderr, "allocation refusal: rpc-endpoint-identity\n"); return 3;
        }
        size_t free_bytes = 0, total_bytes = 0;
        ggml_backend_dev_memory(dev, &free_bytes, &total_bytes);
        budgets[i].free_now = free_bytes;
        budgets[i].total = total_bytes;
    }

    llama_backend_init();
    llama_numa_init(params.numa);
    auto mparams = common_model_params_to_llama(params);
    mparams.no_alloc = true;
    mparams.use_mmap = false;
    mparams.use_mlock = false;
    std::unique_ptr<llama_model, decltype(&llama_model_free)> model(
            llama_model_load_from_file(params.model.path.c_str(), mparams), llama_model_free);
    if (!model) { std::fprintf(stderr, "allocation refusal: metadata-loader\n"); return 4; }

    const llama_model_allocation_plan plan = llama_model_get_allocation_plan(model.get());
    if (!validate_loader_accounting({ budgets[0].name, budgets[1].name },
            { budgets[0].backend, budgets[1].backend }, plan, reason)) {
        std::fprintf(stderr,
                "allocation refusal: %s no_alloc=%d mmap=%d arithmetic=%d complete=%d groups=%zu source=%" PRIu64
                " created=%" PRIu64 " unknown=%" PRIu64 " unaccounted=%" PRIu64 "\n",
                reason.c_str(), plan.no_alloc, plan.use_mmap, plan.arithmetic_ok, plan.complete, plan.groups.size(),
                plan.source_tensor_count, plan.created_tensor_count, plan.unknown_created_tensors,
                plan.unaccounted_source_tensors);
        return 4;
    }
    std::set<std::string> group_ids;
    for (const auto & group : plan.groups) {
        const std::string id = group.device + "\n" + group.backend + "\n" + group.buffer_type;
        if (!group_ids.insert(id).second || group.request_bytes == 0 || group.tensors.empty()) {
            std::fprintf(stderr, "allocation refusal: ambiguous-or-empty-group\n"); return 4;
        }
        auto it = std::find_if(budgets.begin(), budgets.end(), [&](const device_budget & d) {
            return d.name == group.device && d.backend == group.backend;
        });
        if (it == budgets.end() || !add_checked(it->weights, group.request_bytes)) {
            std::fprintf(stderr, "allocation refusal: group-device-or-overflow\n"); return 4;
        }
        it->max_request = std::max(it->max_request, group.request_bytes);
    }

    auto cparams = common_context_params_to_llama(params);
    std::unique_ptr<llama_context, decltype(&llama_free)> ctx(llama_init_from_model(model.get(), cparams), llama_free);
    if (!ctx) { std::fprintf(stderr, "allocation refusal: context-simulation\n"); return 4; }
    const llama_memory_breakdown breakdown = llama_get_memory_breakdown(ctx.get());
    for (const auto & [buft, mb] : breakdown) {
        auto * dev = ggml_backend_buft_get_device(buft);
        if (!dev) continue;
        auto it = std::find_if(budgets.begin(), budgets.end(), [&](const device_budget & d) {
            return d.name == ggml_backend_dev_name(dev);
        });
        if (it == budgets.end()) continue;
        if (!add_checked(it->context, mb.context) || !add_checked(it->compute, mb.compute)) {
            std::fprintf(stderr, "allocation refusal: context-compute-overflow\n"); return 4;
        }
    }
    if (!validate_budgets(budgets, reason)) {
        std::fprintf(stderr, "allocation refusal: %s\n", reason.c_str()); return 5;
    }

    std::printf("{\"admitted\":true,\"scope\":\"no_alloc_metadata_only\",\"artifact\":{");
    std::printf("\"repository\":\"%s\",\"revision\":\"%s\",\"file\":\"%s\",\"size\":%" PRIu64 ",\"sha256\":\"%s\"},",
            EXPECTED_REPOSITORY, EXPECTED_REVISION, EXPECTED_FILE, file_size, artifact_sha.c_str());
    std::printf("\"loader\":{\"device_order\":[\"RPC0\",\"ROCm0\"],\"backends\":[\"RPC\",\"ROCm\"],"
                "\"rpc_endpoint\":\"%s\",\"split_mode\":\"layer\",\"tensor_split\":\"1,1\","
                "\"n_gpu_layers\":999,\"no_alloc\":true,\"mmap\":false,\"zero_byte_backend_sentinels\":true},",
            json_escape(endpoint).c_str());
    std::printf("\"accounting\":{\"complete\":true,\"source_tensor_count\":%" PRIu64 ","
                "\"source_tensor_bytes\":%" PRIu64 ",\"created_tensor_count\":%" PRIu64 ","
                "\"unknown_created_tensors\":0,\"unaccounted_source_tensors\":0},",
            plan.source_tensor_count, plan.source_tensor_bytes, plan.created_tensor_count);
    std::printf("\"capacity_policy\":{\"basis\":\"reported_total_not_production_loaded_free\","
                "\"runtime_reserve_bytes_per_device\":%" PRIu64 ",\"fragmentation_basis_points\":%" PRIu64 ","
                "\"context_classification\":\"no_alloc_runtime_estimate\","
                "\"compute_classification\":\"no_alloc_graph_estimate\","
                "\"reserve_classification\":\"policy_assumption\","
                "\"fragmentation_classification\":\"policy_assumption\"},",
            RUNTIME_RESERVE, FRAGMENTATION_BPS);
    std::printf("\"devices\":[");
    for (size_t i = 0; i < budgets.size(); ++i) {
        const auto & d = budgets[i];
        if (i) std::printf(",");
        std::printf("{\"name\":\"%s\",\"backend\":\"%s\",\"free_now\":%" PRIu64 ",\"total\":%" PRIu64
                    ",\"exact_weight_group_total\":%" PRIu64 ",\"max_exact_weight_request\":%" PRIu64
                    ",\"context_estimate\":%" PRIu64 ",\"compute_estimate\":%" PRIu64
                    ",\"fragmentation_assumption\":%" PRIu64 ",\"runtime_reserve_assumption\":%" PRIu64
                    ",\"required\":%" PRIu64 ",\"remaining_margin\":%" PRIu64 "}",
                d.name.c_str(), d.backend.c_str(), d.free_now, d.total, d.weights, d.max_request,
                d.context, d.compute, d.fragmentation, RUNTIME_RESERVE, d.required, d.margin);
    }
    std::printf("],\"groups\":[");
    for (size_t gi = 0; gi < plan.groups.size(); ++gi) {
        const auto & group = plan.groups[gi];
        if (gi) std::printf(",");
        std::set<int32_t> layers;
        for (const auto & tensor : group.tensors) if (tensor.layer >= 0) layers.insert(tensor.layer);
        std::printf("{\"index\":%zu,\"buffer_type\":\"%s\",\"device\":\"%s\",\"backend\":\"%s\","
                    "\"request_bytes\":%" PRIu64 ",\"layers\":[", gi, json_escape(group.buffer_type).c_str(),
                json_escape(group.device).c_str(), json_escape(group.backend).c_str(), group.request_bytes);
        bool first = true;
        for (int32_t layer : layers) { if (!first) std::printf(","); std::printf("%d", layer); first = false; }
        std::printf("],\"tensors\":[");
        for (size_t ti = 0; ti < group.tensors.size(); ++ti) {
            const auto & tensor = group.tensors[ti];
            if (ti) std::printf(",");
            std::printf("{\"name\":\"%s\",\"type\":\"%s\",\"logical_bytes\":%" PRIu64
                        ",\"source_bytes\":%" PRIu64 ",\"source_offset\":%" PRIu64
                        ",\"layer\":%d,\"view\":%s,\"source_known\":%s}",
                    json_escape(tensor.name).c_str(), json_escape(tensor.type).c_str(), tensor.logical_bytes,
                    tensor.source_bytes, tensor.source_offset, tensor.layer,
                    tensor.view ? "true" : "false", tensor.source_known ? "true" : "false");
        }
        std::printf("]}");
    }
    std::printf("]}\n");
    return 0;
}
