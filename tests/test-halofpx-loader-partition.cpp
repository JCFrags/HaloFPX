#ifdef NDEBUG
#undef NDEBUG
#endif

#include "llama-model-loader.h"

#include "ggml-alloc.h"
#include "ggml-backend.h"
#include "ggml-backend-impl.h"
#include "ggml-cpp.h"
#include "gguf.h"

#include <array>
#include <cassert>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

void llama_model_minimax_m2_reject_removed_peer_half_load_from_env();

extern "C" {
int ggml_loader_txn_create_tensor_pair(
        ggml_context * ctx0,
        ggml_context * ctx1,
        const void * owner,
        const uint64_t * generation_authority,
        uint64_t generation,
        enum ggml_type type,
        int64_t ne0,
        int64_t ne1,
        int64_t ne20,
        int64_t ne21,
        int fail_after_creation,
        ggml_tensor ** tensor0,
        ggml_tensor ** tensor1);
bool ggml_loader_txn_rollback_tensor_pair(
        ggml_context * ctx0,
        ggml_tensor * tensor0,
        ggml_context * ctx1,
        ggml_tensor * tensor1,
        const void * owner,
        const uint64_t * generation_authority,
        uint64_t generation);
bool ggml_loader_txn_commit_tensor_pair(
        ggml_context * ctx0,
        ggml_tensor * tensor0,
        ggml_context * ctx1,
        ggml_tensor * tensor1,
        const void * owner,
        const uint64_t * generation_authority,
        uint64_t generation);
}

struct llama_model_loader_test_access {
    static void fail_after_mutation(llama_model_loader & loader, int boundary) {
        assert(boundary >= 1 && boundary <= 6);
        assert(loader.test_failure_after_mutation == 0);
        loader.test_failure_after_mutation = boundary;
    }
};

namespace {

constexpr const char * tensor_name = "blk.0.ffn_gate_exps.weight";
const std::array<float, 16> source_values = {
    0, 1, 2, 3, 4, 5, 6, 7,
    8, 9, 10, 11, 12, 13, 14, 15,
};

struct allocation_stats {
    int allocations = 0;
    int frees = 0;
    std::vector<std::pair<ggml_backend_buffer_type_t, size_t>> requests;

    void reset() {
        allocations = 0;
        frees = 0;
        requests.clear();
    }
};

allocation_stats & stats() {
    static allocation_stats value;
    return value;
}

void mock_buffer_free(ggml_backend_buffer_t buffer) {
    ++stats().frees;
    std::free(buffer->context);
}
void * mock_buffer_base(ggml_backend_buffer_t buffer) {
    return buffer->context;
}
void mock_tensor_memset(ggml_backend_buffer_t, ggml_tensor * tensor, uint8_t value, size_t offset, size_t size) {
    std::memset(static_cast<uint8_t *>(tensor->data) + offset, value, size);
}
void mock_tensor_set(ggml_backend_buffer_t, ggml_tensor * tensor, const void * data, size_t offset, size_t size) {
    std::memcpy(static_cast<uint8_t *>(tensor->data) + offset, data, size);
}
void mock_tensor_get(ggml_backend_buffer_t, const ggml_tensor * tensor, void * data, size_t offset, size_t size) {
    std::memcpy(data, static_cast<const uint8_t *>(tensor->data) + offset, size);
}
void mock_buffer_clear(ggml_backend_buffer_t buffer, uint8_t value) {
    std::memset(buffer->context, value, buffer->size);
}

struct mock_dev_context {
    const char * name;
    enum ggml_backend_dev_type type;
    ggml_backend_buffer_type_t default_buft = nullptr;
    ggml_backend_buffer_type_t host_buft = nullptr;
};

const char * mock_buft_name(ggml_backend_buffer_type_t buft) {
    return static_cast<const char *>(buft->context);
}
ggml_backend_buffer_t mock_alloc_buffer(ggml_backend_buffer_type_t buft, size_t size) {
    static const ggml_backend_buffer_i iface = {
        mock_buffer_free, mock_buffer_base, nullptr,
        mock_tensor_memset, mock_tensor_set, mock_tensor_get,
        nullptr, nullptr, nullptr, mock_buffer_clear, nullptr,
    };
    void * data = std::malloc(size == 0 ? 1 : size);
    if (data == nullptr) {
        return nullptr;
    }
    ++stats().allocations;
    stats().requests.emplace_back(buft, size);
    return ggml_backend_buffer_init(buft, iface, data, size);
}
size_t mock_alignment(ggml_backend_buffer_type_t) {
    return GGML_MEM_ALIGN;
}
mock_dev_context & mock_context(ggml_backend_dev_t dev) {
    return *static_cast<mock_dev_context *>(dev->context);
}
const char * mock_dev_name(ggml_backend_dev_t dev) {
    return mock_context(dev).name;
}
const char * mock_dev_description(ggml_backend_dev_t dev) {
    return mock_dev_name(dev);
}
void mock_dev_memory(ggml_backend_dev_t, size_t * free, size_t * total) {
    *free = *total = 0;
}
enum ggml_backend_dev_type mock_dev_type(ggml_backend_dev_t dev) {
    return mock_context(dev).type;
}
void mock_dev_props(ggml_backend_dev_t dev, ggml_backend_dev_props * props) {
    *props = {};
    props->name = mock_dev_name(dev);
    props->description = mock_dev_description(dev);
    props->type = mock_dev_type(dev);
}
ggml_backend_buffer_type_t mock_dev_buft(ggml_backend_dev_t dev) {
    return mock_context(dev).default_buft;
}
ggml_backend_buffer_type_t mock_dev_host_buft(ggml_backend_dev_t dev) {
    return mock_context(dev).host_buft;
}
bool mock_supports_op(ggml_backend_dev_t, const ggml_tensor *) {
    return true;
}
bool mock_supports_buft(ggml_backend_dev_t dev, ggml_backend_buffer_type_t buft) {
    return ggml_backend_buft_get_device(buft) == dev;
}

struct mock_targets {
    mock_dev_context ctx0 { "L111Rank0", GGML_BACKEND_DEVICE_TYPE_GPU };
    mock_dev_context ctx1 { "L111Rank1", GGML_BACKEND_DEVICE_TYPE_GPU };
    mock_dev_context host_ctx { "L111Host", GGML_BACKEND_DEVICE_TYPE_CPU };
    ggml_backend_device dev0;
    ggml_backend_device dev1;
    ggml_backend_device host_dev;
    ggml_backend_buffer_type buft0;
    ggml_backend_buffer_type buft1;
    ggml_backend_buffer_type buft1_alt;
    ggml_backend_buffer_type host_buft;

    mock_targets() {
        static const ggml_backend_buffer_type_i buft_iface = {
            mock_buft_name, mock_alloc_buffer, mock_alignment, nullptr, nullptr, nullptr,
        };
        static const ggml_backend_device_i dev_iface = {
            mock_dev_name, mock_dev_description, mock_dev_memory, mock_dev_type, mock_dev_props,
            nullptr, mock_dev_buft, mock_dev_host_buft, nullptr, mock_supports_op, mock_supports_buft, nullptr,
            nullptr, nullptr, nullptr,
        };
        dev0 = { dev_iface, nullptr, &ctx0 };
        dev1 = { dev_iface, nullptr, &ctx1 };
        host_dev = { dev_iface, nullptr, &host_ctx };
        buft0 = { buft_iface, &dev0, const_cast<char *>("L111Rank0Buffer") };
        buft1 = { buft_iface, &dev1, const_cast<char *>("L111Rank1Buffer") };
        buft1_alt = { buft_iface, &dev1, const_cast<char *>("L111Rank1AlternateBuffer") };
        host_buft = { buft_iface, &host_dev, const_cast<char *>("L111HostBuffer") };
        ctx0.default_buft = &buft0;
        ctx1.default_buft = &buft1;
        host_ctx.default_buft = &host_buft;
        host_ctx.host_buft = &host_buft;
    }
};

mock_targets & targets() {
    static mock_targets value;
    return value;
}

void set_legacy_failure_environment(const char * value) {
#ifdef _WIN32
    _putenv_s("HALOFPX_L111_FAIL_AFTER_MUTATION", value == nullptr ? "" : value);
#else
    if (value == nullptr || value[0] == '\0') {
        unsetenv("HALOFPX_L111_FAIL_AFTER_MUTATION");
    } else {
        setenv("HALOFPX_L111_FAIL_AFTER_MUTATION", value, 1);
    }
#endif
}

void set_peer_half_raw(const char * value) {
#ifdef _WIN32
    _putenv_s("HALOFPX_MINIMAX_M2_EXPERT_PEER_HALF_LOAD", value == nullptr ? "" : value);
#else
    if (value == nullptr || value[0] == '\0') {
        unsetenv("HALOFPX_MINIMAX_M2_EXPERT_PEER_HALF_LOAD");
    } else {
        setenv("HALOFPX_MINIMAX_M2_EXPERT_PEER_HALF_LOAD", value, 1);
    }
#endif
}

std::filesystem::path write_fixture(bool rank4) {
    const auto path = std::filesystem::temp_directory_path() /
            (rank4 ? "halofpx-l111-rank4.gguf" : "halofpx-l111-tiny.gguf");
    gguf_context_ptr gguf(gguf_init_empty());
    gguf_set_val_str(gguf.get(), "general.architecture", "llama");

    ggml_init_params params = {
        /*.mem_size   =*/ 2 * ggml_tensor_overhead(),
        /*.mem_buffer =*/ nullptr,
        /*.no_alloc   =*/ false,
    };
    ggml_context_ptr ctx(ggml_init(params));
    assert(ctx);
    ggml_tensor * tensor = rank4
            ? ggml_new_tensor_4d(ctx.get(), GGML_TYPE_F32, 2, 2, 2, 2)
            : ggml_new_tensor_3d(ctx.get(), GGML_TYPE_F32, 2, 2, 4);
    ggml_set_name(tensor, tensor_name);
    std::memcpy(tensor->data, source_values.data(), sizeof(source_values));
    gguf_add_tensor(gguf.get(), tensor);
    gguf_write_to_file(gguf.get(), path.string().c_str(), false);
    return path;
}

struct fixture_loader {
    std::vector<std::string> splits;
    llama_model_loader loader;
    ggml_backend_dev_t dev0 = &targets().dev0;
    ggml_backend_dev_t dev1 = &targets().dev1;
    ggml_backend_buffer_type_t buft0 = &targets().buft0;
    ggml_backend_buffer_type_t buft1 = &targets().buft1;
    ggml_context * ctx0 = nullptr;
    ggml_context * ctx1 = nullptr;

    explicit fixture_loader(
            const std::filesystem::path & path,
            bool use_mmap = false,
            const llama_model_tensor_buft_override * overrides = nullptr,
            size_t context_bytes = 4 * ggml_tensor_overhead())
        : loader(nullptr, nullptr, nullptr, path.string(), splits, nullptr,
                use_mmap, false, false, false, nullptr, overrides) {
        assert(dev0 && dev1 && dev0 != dev1);
        assert(buft0 && buft1 && buft0 != buft1);
        ggml_init_params params = {
            /*.mem_size   =*/ context_bytes,
            /*.mem_buffer =*/ nullptr,
            /*.no_alloc   =*/ true,
        };
        ctx0 = ggml_init(params);
        ctx1 = ggml_init(params);
        assert(ctx0 && ctx1);
        assert(loader.ctx_map.emplace(buft0, ggml_context_ptr(ctx0)).second);
        assert(loader.ctx_map.emplace(buft1, ggml_context_ptr(ctx1)).second);
    }

    std::array<llama_model_loader::axis2_partition, 2> good() const {
        return {{
            { 0, 0, 2, dev0, buft0 },
            { 1, 2, 4, dev1, buft1 },
        }};
    }
};

std::vector<ggml_tensor *> tensor_chain(ggml_context * ctx) {
    std::vector<ggml_tensor *> result;
    for (ggml_tensor * tensor = ggml_get_first_tensor(ctx);
            tensor != nullptr;
            tensor = ggml_get_next_tensor(ctx, tensor)) {
        result.push_back(tensor);
    }
    return result;
}

struct loader_snapshot {
    size_t used0;
    size_t used1;
    std::vector<ggml_tensor *> chain0;
    std::vector<ggml_tensor *> chain1;
    std::vector<std::pair<const ggml_tensor *, size_t>> source_offsets;
    std::vector<const ggml_tensor *> lookup_exclusions;
    int n_created;
    size_t size_data;
    size_t size_done;
    uint64_t generation;
    bool creation_sealed;
};

loader_snapshot snapshot(const fixture_loader & f) {
    return {
        ggml_used_mem(f.ctx0),
        ggml_used_mem(f.ctx1),
        tensor_chain(f.ctx0),
        tensor_chain(f.ctx1),
        f.loader.tensor_source_offsets,
        f.loader.tensors_excluded_from_lookup,
        f.loader.n_created,
        f.loader.size_data,
        f.loader.size_done,
        f.loader.partition_generation,
        f.loader.tensor_creation_sealed,
    };
}

void assert_snapshot(const fixture_loader & f, const loader_snapshot & before) {
    assert(ggml_used_mem(f.ctx0) == before.used0);
    assert(ggml_used_mem(f.ctx1) == before.used1);
    assert(tensor_chain(f.ctx0) == before.chain0);
    assert(tensor_chain(f.ctx1) == before.chain1);
    assert(f.loader.tensor_source_offsets == before.source_offsets);
    assert(f.loader.tensors_excluded_from_lookup == before.lookup_exclusions);
    assert(f.loader.n_created == before.n_created);
    assert(f.loader.size_data == before.size_data);
    assert(f.loader.size_done == before.size_done);
    assert(f.loader.partition_generation == before.generation);
    assert(f.loader.tensor_creation_sealed == before.creation_sealed);
}

void expect_refusal(
        fixture_loader & f,
        const std::array<llama_model_loader::axis2_partition, 2> & request,
        const std::string & source = tensor_name) {
    const loader_snapshot before = snapshot(f);
    bool refused = false;
    try {
        (void) f.loader.create_axis2_partition_pair(source, request);
    } catch (const std::runtime_error &) {
        refused = true;
    }
    assert(refused);
    assert_snapshot(f, before);
}

struct progress_trace {
    std::vector<float> values;
};

bool progress(float value, void * userdata) {
    static_cast<progress_trace *>(userdata)->values.push_back(value);
    return true;
}

void assert_progress(const progress_trace & trace) {
    assert(trace.values.size() == 3);
    assert(std::fabs(trace.values[0] - 0.0f) < 0.001f);
    assert(std::fabs(trace.values[1] - 0.5f) < 0.001f);
    assert(std::fabs(trace.values[2] - 1.0f) < 0.001f);
}

void test_success(const std::filesystem::path & path, bool use_mmap) {
    fixture_loader f(path, use_mmap);
    auto pair = f.loader.create_axis2_partition_pair(tensor_name, f.good());
    assert(pair.rank0 && pair.rank1);
    assert(pair.rank0->ne[0] == 2 && pair.rank0->ne[1] == 2 && pair.rank0->ne[2] == 2 && pair.rank0->ne[3] == 1);
    assert(pair.rank1->ne[0] == 2 && pair.rank1->ne[1] == 2 && pair.rank1->ne[2] == 2 && pair.rank1->ne[3] == 1);
    assert(pair.rank0->type == GGML_TYPE_F32 && pair.rank1->type == GGML_TYPE_F32);
    assert(f.loader.tensor_source_offset(pair.rank0) == std::optional<size_t>(0));
    assert(f.loader.tensor_source_offset(pair.rank1) == std::optional<size_t>(8 * sizeof(float)));
    assert(f.loader.tensor_is_public(pair.rank0));
    assert(!f.loader.tensor_is_public(pair.rank1));
    assert(f.loader.n_created == 1);
    assert(f.loader.partition_generation == 1);
    f.loader.done_getting_tensors();
    assert(f.loader.tensor_creation_sealed);

    size_t public_count = 0;
    for (ggml_context * ctx : { f.ctx0, f.ctx1 }) {
        for (ggml_tensor * tensor = ggml_get_first_tensor(ctx);
                tensor != nullptr;
                tensor = ggml_get_next_tensor(ctx, tensor)) {
            public_count += f.loader.tensor_is_public(tensor) ? 1 : 0;
        }
    }
    assert(public_count == 1);
    assert(tensor_chain(f.ctx0).size() == 1);
    assert(tensor_chain(f.ctx1).size() == 1);

    const size_t request0 = ggml_backend_alloc_ctx_tensors_from_buft_size(f.ctx0, f.buft0);
    const size_t request1 = ggml_backend_alloc_ctx_tensors_from_buft_size(f.ctx1, f.buft1);
    assert(request0 == 8 * sizeof(float));
    assert(request1 == 8 * sizeof(float));
    assert(request0 + request1 == sizeof(source_values));

    stats().reset();
    {
        ggml_backend_buffer_ptr buffer0(ggml_backend_alloc_ctx_tensors_from_buft(f.ctx0, f.buft0));
        ggml_backend_buffer_ptr buffer1(ggml_backend_alloc_ctx_tensors_from_buft(f.ctx1, f.buft1));
        assert(buffer0 && buffer1);
        assert(stats().allocations == 2);
        assert(stats().requests.size() == 2);
        assert(ggml_backend_buffer_get_size(buffer0.get()) == request0);
        assert(ggml_backend_buffer_get_size(buffer1.get()) == request1);

        f.loader.init_mappings(false);
        assert(f.loader.size_data == sizeof(source_values));
        if (use_mmap) {
            const auto & weight = f.loader.require_weight(tensor_name);
            size_t first0 = 0;
            size_t last0 = 0;
            size_t first1 = 0;
            size_t last1 = 0;
            void * addr0 = nullptr;
            void * addr1 = nullptr;
            f.loader.get_mapping_range(&first0, &last0, &addr0, weight.idx, f.ctx0);
            f.loader.get_mapping_range(&first1, &last1, &addr1, weight.idx, f.ctx1);
            assert(addr0 != nullptr && addr0 == addr1);
            assert(first0 == weight.offs);
            assert(last0 == weight.offs + 8 * sizeof(float));
            assert(first1 == weight.offs + 8 * sizeof(float));
            assert(last1 == weight.offs + sizeof(source_values));
        }
        llama_buf_map bufs;
        progress_trace trace;
        assert(f.loader.load_all_data(f.ctx0, bufs, nullptr, progress, &trace));
        assert(f.loader.load_all_data(f.ctx1, bufs, nullptr, progress, &trace));
        assert_progress(trace);
        assert(f.loader.size_done == sizeof(source_values));

        std::array<float, 8> rank0 {};
        std::array<float, 8> rank1 {};
        ggml_backend_tensor_get(pair.rank0, rank0.data(), 0, sizeof(rank0));
        ggml_backend_tensor_get(pair.rank1, rank1.data(), 0, sizeof(rank1));
        assert(std::memcmp(rank0.data(), source_values.data(), sizeof(rank0)) == 0);
        assert(std::memcmp(rank1.data(), source_values.data() + 8, sizeof(rank1)) == 0);

    }
    assert(stats().frees == 2);
}

void test_full_duplicate_parity(const std::filesystem::path & path) {
    fixture_loader f(path);
    llama_hparams hparams = {};
    hparams.n_layer = 1;
    hparams.n_expert_used = 1;
    const buft_list_t list0 = {{ f.dev0, f.buft0 }};
    const buft_list_t list1 = {{ f.dev1, f.buft1 }};
    const auto tn = LLM_TN(LLM_ARCH_LLAMA)(LLM_TENSOR_FFN_GATE_EXPS, "weight", 0);

    ggml_tensor * primary = f.loader.create_tensor(
            hparams, &list0, &list0, &list0, &list0, tn, { 2, 2, 4 }, 0);
    const loader_snapshot before_same_context = snapshot(f);
    bool same_context_refused = false;
    try {
        (void) f.loader.create_tensor(
                hparams, &list0, &list0, &list0, &list0, tn, { 2, 2, 4 },
                llama_model_loader::TENSOR_DUPLICATED | llama_model_loader::TENSOR_IMPLEMENTATION_ONLY);
    } catch (const std::runtime_error &) {
        same_context_refused = true;
    }
    assert(same_context_refused);
    assert_snapshot(f, before_same_context);
    assert(f.loader.tensor_is_public(primary));

    ggml_tensor * implementation_duplicate = f.loader.create_tensor(
            hparams, &list1, &list1, &list1, &list1, tn, { 2, 2, 4 },
            llama_model_loader::TENSOR_DUPLICATED | llama_model_loader::TENSOR_IMPLEMENTATION_ONLY);
    assert(primary && implementation_duplicate && primary != implementation_duplicate);
    assert(f.loader.tensor_is_public(primary));
    assert(!f.loader.tensor_is_public(implementation_duplicate));
    assert(f.loader.require_tensor_source_offset(primary) == 0);
    assert(f.loader.require_tensor_source_offset(implementation_duplicate) == 0);
    assert(f.loader.n_created == 1);
    assert(f.loader.size_data == sizeof(source_values));
    f.loader.done_getting_tensors();

    stats().reset();
    {
        ggml_backend_buffer_ptr buffer0(ggml_backend_alloc_ctx_tensors_from_buft(f.ctx0, f.buft0));
        ggml_backend_buffer_ptr buffer1(ggml_backend_alloc_ctx_tensors_from_buft(f.ctx1, f.buft1));
        assert(buffer0 && buffer1);
        assert(ggml_backend_buffer_get_size(buffer0.get()) == sizeof(source_values));
        assert(ggml_backend_buffer_get_size(buffer1.get()) == sizeof(source_values));
        f.loader.init_mappings(false);
        assert(f.loader.size_data == 2 * sizeof(source_values));

        llama_buf_map bufs;
        progress_trace trace;
        assert(f.loader.load_all_data(f.ctx0, bufs, nullptr, progress, &trace));
        assert(f.loader.load_all_data(f.ctx1, bufs, nullptr, progress, &trace));
        assert_progress(trace);
        assert(f.loader.size_done == 2 * sizeof(source_values));

        std::array<float, 16> primary_bytes {};
        std::array<float, 16> duplicate_bytes {};
        ggml_backend_tensor_get(primary, primary_bytes.data(), 0, sizeof(primary_bytes));
        ggml_backend_tensor_get(implementation_duplicate, duplicate_bytes.data(), 0, sizeof(duplicate_bytes));
        assert(primary_bytes == source_values);
        assert(duplicate_bytes == source_values);
    }
    assert(stats().frees == 2);
}

void test_failure_boundaries(const std::filesystem::path & path) {
    for (int boundary = 1; boundary <= 6; ++boundary) {
        fixture_loader f(path);
        llama_model_loader_test_access::fail_after_mutation(f.loader, boundary);
        expect_refusal(f, f.good());
        const auto pair = f.loader.create_axis2_partition_pair(tensor_name, f.good());
        assert(pair.rank0 != nullptr && pair.rank1 != nullptr);
        assert(f.loader.partition_generation == 1);
    }
    {
        fixture_loader f(path);
        ggml_tensor * sentinel0 = ggml_new_tensor_1d(f.ctx0, GGML_TYPE_F32, 1);
        ggml_tensor * sentinel1 = ggml_new_tensor_1d(f.ctx1, GGML_TYPE_F32, 1);
        ggml_set_name(sentinel0, "l111.sentinel.rank0");
        ggml_set_name(sentinel1, "l111.sentinel.rank1");
        f.loader.size_data = 23;
        llama_model_loader_test_access::fail_after_mutation(f.loader, 4);
        expect_refusal(f, f.good());
    }
}

void test_legacy_failure_environment_is_ignored(const std::filesystem::path & path) {
    set_legacy_failure_environment("1");
    fixture_loader f(path);
    const auto pair = f.loader.create_axis2_partition_pair(tensor_name, f.good());
    assert(pair.rank0 != nullptr && pair.rank1 != nullptr);
    assert(f.loader.partition_generation == 1);
    set_legacy_failure_environment(nullptr);
}

void test_transaction_binding(const std::filesystem::path & path) {
    fixture_loader owner(path);
    fixture_loader foreign(path);
    uint64_t generation_authority = 0;
    ggml_tensor * tensor0 = nullptr;
    ggml_tensor * tensor1 = nullptr;
    assert(ggml_loader_txn_create_tensor_pair(
            owner.ctx0, owner.ctx1, &owner.loader, &generation_authority, 1,
            GGML_TYPE_F32, 1, 1, 1, 1, 0, &tensor0, &tensor1) == 0);
    assert(tensor0 && tensor1);
    assert(!ggml_loader_txn_rollback_tensor_pair(
            owner.ctx0, tensor0, owner.ctx1, tensor1,
            &foreign.loader, &generation_authority, 1));
    assert(ggml_get_first_tensor(owner.ctx0) == tensor0);
    assert(ggml_get_first_tensor(owner.ctx1) == tensor1);
    assert(!ggml_loader_txn_rollback_tensor_pair(
            owner.ctx0, tensor0, owner.ctx1, tensor1,
            &owner.loader, &generation_authority, 2));
    assert(ggml_get_first_tensor(owner.ctx0) == tensor0);
    assert(ggml_get_first_tensor(owner.ctx1) == tensor1);
    generation_authority = 1;
    assert(!ggml_loader_txn_rollback_tensor_pair(
            owner.ctx0, tensor0, owner.ctx1, tensor1,
            &owner.loader, &generation_authority, 1));
    assert(ggml_get_first_tensor(owner.ctx0) == tensor0);
    assert(ggml_get_first_tensor(owner.ctx1) == tensor1);
    generation_authority = 0;
    assert(ggml_loader_txn_rollback_tensor_pair(
            owner.ctx0, tensor0, owner.ctx1, tensor1,
            &owner.loader, &generation_authority, 1));
    assert(ggml_get_first_tensor(owner.ctx0) == nullptr);
    assert(ggml_get_first_tensor(owner.ctx1) == nullptr);

    ggml_tensor * committed0 = nullptr;
    ggml_tensor * committed1 = nullptr;
    assert(ggml_loader_txn_create_tensor_pair(
            owner.ctx0, owner.ctx1, &owner.loader, &generation_authority, 1,
            GGML_TYPE_F32, 1, 1, 1, 1, 0, &committed0, &committed1) == 0);
    assert(ggml_loader_txn_commit_tensor_pair(
            owner.ctx0, committed0, owner.ctx1, committed1,
            &owner.loader, &generation_authority, 1));
    generation_authority = 1;

    ggml_tensor * current0 = nullptr;
    ggml_tensor * current1 = nullptr;
    assert(ggml_loader_txn_create_tensor_pair(
            owner.ctx0, owner.ctx1, &owner.loader, &generation_authority, 2,
            GGML_TYPE_F32, 1, 1, 1, 1, 0, &current0, &current1) == 0);
    assert(ggml_loader_txn_commit_tensor_pair(
            owner.ctx0, current0, owner.ctx1, current1,
            &owner.loader, &generation_authority, 2));
    generation_authority = 2;
    assert(!ggml_loader_txn_rollback_tensor_pair(
            owner.ctx0, committed0, owner.ctx1, committed1,
            &owner.loader, &generation_authority, 1));
    assert(tensor_chain(owner.ctx0) == std::vector<ggml_tensor *>({ committed0, current0 }));
    assert(tensor_chain(owner.ctx1) == std::vector<ggml_tensor *>({ committed1, current1 }));
}

void test_creation_lifecycle_seal(const std::filesystem::path & path) {
    {
        fixture_loader f(path);
        f.loader.done_getting_tensors(true);
        assert(f.loader.tensor_creation_sealed);
        expect_refusal(f, f.good());
    }
    {
        fixture_loader f(path);
        f.loader.init_mappings(false);
        assert(f.loader.tensor_creation_sealed);
        expect_refusal(f, f.good());
    }
}

void test_minimax_removed_peer_half_gate() {
    set_peer_half_raw(nullptr);
    llama_model_minimax_m2_reject_removed_peer_half_load_from_env();

    set_peer_half_raw("1");
    bool typed_refusal = false;
    try {
        llama_model_minimax_m2_reject_removed_peer_half_load_from_env();
    } catch (const std::invalid_argument & error) {
        typed_refusal =
                std::string(error.what()).find("no longer supported") != std::string::npos;
    }
    assert(typed_refusal);
    set_peer_half_raw(nullptr);
}

void test_unknown_source_offset(const std::filesystem::path & path) {
    fixture_loader f(path);
    ggml_tensor * unknown = ggml_new_tensor_1d(f.ctx0, GGML_TYPE_F32, 1);
    ggml_set_name(unknown, "unknown.loader.tensor");
    assert(!f.loader.tensor_source_offset(unknown).has_value());
    bool refused = false;
    try {
        (void) f.loader.require_tensor_source_offset(unknown);
    } catch (const std::runtime_error &) {
        refused = true;
    }
    assert(refused);
}

void test_negatives(const std::filesystem::path & path, const std::filesystem::path & rank4_path) {
    {
        fixture_loader f(path);
        auto r = f.good(); r[1] = { 1, 0, 0, nullptr, nullptr };
        expect_refusal(f, r);
    }
    {
        fixture_loader f(path);
        auto r = f.good(); r[1].rank = 0;
        expect_refusal(f, r);
    }
    {
        fixture_loader f(path);
        auto r = f.good(); r[1].rank = 2;
        expect_refusal(f, r);
    }
    {
        fixture_loader f(path);
        auto r = f.good(); r[1].begin = 3;
        expect_refusal(f, r);
    }
    {
        fixture_loader f(path);
        auto r = f.good(); r[1].begin = 1;
        expect_refusal(f, r);
    }
    {
        fixture_loader f(path);
        auto r = f.good(); r[0].begin = 2; r[0].end = 1;
        expect_refusal(f, r);
    }
    {
        fixture_loader f(path);
        auto r = f.good(); r[1].end = 5;
        expect_refusal(f, r);
    }
    {
        fixture_loader f(path);
        auto r = f.good(); r[1].begin = r[1].end;
        expect_refusal(f, r);
    }
    {
        fixture_loader f(path);
        auto r = f.good(); r[1].dev = r[0].dev; r[1].buft = r[0].buft;
        expect_refusal(f, r);
    }
    {
        fixture_loader f(path);
        auto r = f.good(); r[1].dev = &targets().host_dev; r[1].buft = &targets().host_buft;
        expect_refusal(f, r);
    }
    {
        fixture_loader f(path);
        auto r = f.good(); r[1].buft = r[0].buft;
        expect_refusal(f, r);
    }
    {
        fixture_loader f(path);
        assert(f.loader.ctx_map.emplace(&targets().buft1_alt, ggml_context_ptr(ggml_init({
            /*.mem_size   =*/ 4 * ggml_tensor_overhead(),
            /*.mem_buffer =*/ nullptr,
            /*.no_alloc   =*/ true,
        }))).second);
        auto r = f.good(); r[1].buft = &targets().buft1_alt;
        expect_refusal(f, r);
    }
    {
        fixture_loader f(path);
        ggml_tensor * ambiguous = ggml_new_tensor_3d(f.ctx0, GGML_TYPE_F32, 2, 2, 2);
        ggml_set_name(ambiguous, tensor_name);
        expect_refusal(f, f.good());
    }
    {
        fixture_loader f(path);
        ggml_tensor * hidden_full = ggml_new_tensor_3d(f.ctx1, GGML_TYPE_F32, 2, 2, 4);
        ggml_set_name(hidden_full, tensor_name);
        f.loader.tensors_excluded_from_lookup.push_back(hidden_full);
        expect_refusal(f, f.good());
    }
    {
        fixture_loader f(rank4_path);
        expect_refusal(f, f.good());
    }
    {
        llama_model_tensor_buft_override terminator = {};
        fixture_loader f(path, false, &terminator);
        expect_refusal(f, f.good());
    }
    {
        fixture_loader f(path, false, nullptr, GGML_MEM_ALIGN);
        expect_refusal(f, f.good());
    }
    {
        fixture_loader f(path);
        expect_refusal(f, f.good(), "missing.tensor");
    }
}

} // namespace

int main() {
    const auto path = write_fixture(false);
    const auto rank4_path = write_fixture(true);
    set_legacy_failure_environment(nullptr);
    test_failure_boundaries(path);
    test_legacy_failure_environment_is_ignored(path);
    test_transaction_binding(path);
    test_creation_lifecycle_seal(path);
    test_minimax_removed_peer_half_gate();
    test_unknown_source_offset(path);
    test_negatives(path, rank4_path);
    test_success(path, false);
    test_success(path, true);
    test_full_duplicate_parity(path);
    set_legacy_failure_environment(nullptr);
    set_peer_half_raw(nullptr);
    std::filesystem::remove(path);
    std::filesystem::remove(rank4_path);
    return 0;
}
