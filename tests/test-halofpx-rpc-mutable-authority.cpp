#include "ggml.h"
#include "ggml-alloc.h"
#include "ggml-backend.h"
#include "ggml-rpc.h"
extern "C" {
#include "../examples/gguf-hash/deps/sha256/sha256.h"
}

#include <array>
#include <atomic>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <string>
#include <thread>
#include <type_traits>
#include <vector>

template<typename T>
static void census_le(std::vector<uint8_t> & out, T value) {
    using U = typename std::make_unsigned<T>::type;
    const U v = static_cast<U>(value);
    for (size_t i = 0; i < sizeof(T); ++i) {
        out.push_back(static_cast<uint8_t>(v >> (8*i)));
    }
}

extern "C" uint32_t ggml_backend_rpc_halofpx_mutable_auth_self_test(void);

struct fixture {
    ggml_backend_t rpc = nullptr;
    ggml_backend_t cpu = nullptr;
    ggml_backend_sched_t sched = nullptr;
    ggml_context * ctx = nullptr;
    ggml_tensor * input = nullptr;
    ggml_tensor * bias = nullptr;
    ggml_tensor * output = nullptr;
    ggml_cgraph * graph = nullptr;
    ggml_backend_buffer_t buffer = nullptr;
    std::vector<uint8_t> metadata;
};

static void nonce_for(uint64_t sequence, uint8_t nonce[32]) {
    for (size_t i = 0; i < 32; ++i) {
        nonce[i] = static_cast<uint8_t>(0x31 + sequence + i);
    }
}

static bool graph_key(std::array<uint8_t, 32> & key) {
    const char * path = std::getenv("HALOFPX_RPC_GRAPH_AUTH_KEY_FILE");
    if (path == nullptr) return false;
    std::ifstream input(path, std::ios::binary);
    std::string line;
    if (!input || !std::getline(input, line) || line.size() != 64) return false;
    auto digit = [](char value) {
        if (value >= '0' && value <= '9') return value - '0';
        if (value >= 'a' && value <= 'f') return value - 'a' + 10;
        if (value >= 'A' && value <= 'F') return value - 'A' + 10;
        return -1;
    };
    for (size_t i = 0; i < key.size(); ++i) {
        const int high = digit(line[2*i]);
        const int low = digit(line[2*i + 1]);
        if (high < 0 || low < 0) return false;
        key[i] = static_cast<uint8_t>((high << 4) | low);
    }
    return true;
}

static bool make_fixture(const char * endpoint, fixture & f) {
    f.rpc = ggml_backend_rpc_init(endpoint, 0);
    f.cpu = ggml_backend_init_by_type(GGML_BACKEND_DEVICE_TYPE_CPU, nullptr);
    if (!f.rpc || !f.cpu) return false;
    f.metadata.resize(ggml_tensor_overhead() * 8 + 2 * ggml_graph_overhead());
    ggml_init_params params { f.metadata.size(), f.metadata.data(), true };
    f.ctx = ggml_init(params);
    if (!f.ctx) return false;
    f.input = ggml_new_tensor_1d(f.ctx, GGML_TYPE_F32, 32);
    f.bias = ggml_new_tensor_1d(f.ctx, GGML_TYPE_F32, 32);
    f.output = ggml_sqr(f.ctx, ggml_add(f.ctx, f.input, f.bias));
    ggml_set_input(f.input);
    ggml_set_output(f.output);
    f.graph = ggml_new_graph(f.ctx);
    ggml_build_forward_expand(f.graph, f.output);
    ggml_backend_t backends[] = { f.rpc, f.cpu };
    ggml_backend_buffer_type_t bufts[] = {
        ggml_backend_get_default_buffer_type(f.rpc),
        ggml_backend_get_default_buffer_type(f.cpu),
    };
    f.sched = ggml_backend_sched_new(backends, bufts, 2, 64, false, false);
    if (!f.sched) return false;
    for (ggml_tensor * tensor : { f.input, f.bias, f.output }) {
        ggml_backend_sched_set_tensor_backend(f.sched, tensor, f.rpc);
    }
    return true;
}

static void free_fixture(fixture & f) {
    if (f.sched) ggml_backend_sched_free(f.sched);
    if (f.buffer) ggml_backend_buffer_free(f.buffer);
    if (f.ctx) ggml_free(f.ctx);
    if (f.rpc) ggml_backend_free(f.rpc);
    if (f.cpu) ggml_backend_free(f.cpu);
    f = {};
}

static bool false_rpc_destination_refusal(fixture & f) {
    std::vector<uint8_t> metadata(
        ggml_tensor_overhead() * 4 + ggml_graph_overhead());
    ggml_init_params params { metadata.size(), metadata.data(), true };
    ggml_context * ctx = ggml_init(params);
    if (ctx == nullptr) return false;
    ggml_tensor * input = ggml_new_tensor_1d(ctx, GGML_TYPE_F32, 16);
    ggml_set_input(input);
    ggml_tensor * output = ggml_sqr(ctx, input);
    ggml_set_output(output);
    ggml_backend_t backends[] = { f.rpc, f.cpu };
    ggml_backend_buffer_type_t bufts[] = {
        ggml_backend_get_default_buffer_type(f.rpc),
        ggml_backend_get_default_buffer_type(f.cpu),
    };
    ggml_backend_sched_t sched =
        ggml_backend_sched_new(backends, bufts, 2, 16, false, false);
    if (sched == nullptr) {
        ggml_free(ctx);
        return false;
    }
    ggml_backend_sched_set_tensor_backend(sched, input, f.cpu);
    ggml_backend_sched_set_tensor_backend(sched, output, f.cpu);
    ggml_backend_sched_authority_config config {};
    config.major = 1;
    config.minor = 0;
    config.encoded_size = sizeof(config);
    config.max_events = 64;
    config.execution_sequence = 73;
    config.attempt_nonce[0] = 0x73;
    config.key[0] = 0xa5;
    std::array<uint8_t, 16384> events {};
    config.event_buffer = events.data();
    config.event_buffer_size = events.size();
    ggml_backend_sched_authority_handle handle {};
    bool ok =
        ggml_backend_sched_authority_arm(sched, &config, &handle) &&
        // This deliberately false scheduler claim keeps backend 1 in the RPC
        // census; it must fail storage validation rather than be filtered.
        ggml_backend_sched_authority_mark_rpc_backend(sched, &handle, 1) &&
        ggml_backend_sched_authority_register_root(
            sched, &handle, input, GGML_BACKEND_SCHED_AUTH_MUTABLE,
            GGML_RPC_HALOFPX_MUTABLE_TOKEN, 0);
    std::memset(config.key, 0, sizeof(config.key));
    ggml_cgraph * graph = ggml_new_graph(ctx);
    ggml_build_forward_expand(graph, output);
    ggml_backend_sched_authority_prepared prepared {};
    ggml_backend_sched_authority_projection_failure failure {};
    const bool allocated = ok && ggml_backend_sched_alloc_graph(sched, graph);
    const bool prepared_ok = allocated &&
        ggml_backend_sched_authority_prepare(
            sched, &handle, graph, &prepared);
    const bool refused = prepared_ok &&
        !ggml_backend_sched_authority_resolve_census_typed(
            sched, &handle,
            [](void * user_data, uint32_t, ggml_tensor * tensor,
               ggml_backend_sched_authority_storage_resolution * out) {
                auto * fixture_value = static_cast<fixture *>(user_data);
                if (fixture_value == nullptr || out == nullptr) return false;
                ggml_backend_rpc_halofpx_storage_identity identity {};
                const auto result =
                    ggml_backend_rpc_halofpx_resolve_storage_identity(
                        fixture_value->rpc, tensor, &identity);
                if (result != GGML_RPC_HALOFPX_MUTABLE_ADMIT_SUCCESS) {
                    out->failure_reason =
                        result ==
                            GGML_RPC_HALOFPX_MUTABLE_ADMIT_STORAGE_NOT_RPC ?
                        GGML_BACKEND_SCHED_PROJECTION_NON_RPC_STORAGE :
                        GGML_BACKEND_SCHED_PROJECTION_OVERFLOW_INVALID;
                    return false;
                }
                return true;
            },
            &f, &failure);
    const size_t remaining =
        ggml_backend_sched_authority_census_count(sched, &handle, 1);
    ok = ok && allocated && prepared_ok && refused &&
        failure.reason == GGML_BACKEND_SCHED_PROJECTION_NON_RPC_STORAGE &&
        failure.backend_ordinal == 1 &&
        failure.candidate_index == 0 &&
        remaining == 0;
    if (!ok) {
        std::fprintf(
            stderr,
            "false RPC destination mismatch alloc=%u prepare=%u refused=%u "
            "reason=%u backend=%u candidate=%u remaining=%zu\n",
            allocated ? 1u : 0u, prepared_ok ? 1u : 0u,
            refused ? 1u : 0u, failure.reason, failure.backend_ordinal,
            failure.candidate_index, remaining);
    }
    ggml_backend_sched_authority_abort_execution(sched, &handle);
    ggml_backend_sched_free(sched);
    ggml_free(ctx);
    return ok;
}

static bool storage_identity_fixture(
        const char * first_endpoint, const char * second_endpoint) {
    fixture first {};
    fixture second {};
    if (!make_fixture(first_endpoint, first) ||
        !make_fixture(second_endpoint, second) ||
        !ggml_backend_sched_alloc_graph(first.sched, first.graph) ||
        !ggml_backend_sched_alloc_graph(second.sched, second.graph)) {
        free_fixture(first);
        free_fixture(second);
        return false;
    }
    ggml_backend_rpc_halofpx_storage_identity direct {};
    ggml_backend_rpc_halofpx_storage_identity nested {};
    ggml_backend_rpc_halofpx_storage_identity shifted {};
    ggml_tensor first_view = *first.input;
    ggml_tensor nested_view = *first.input;
    first_view.view_src = first.input;
    first_view.buffer = first.input->buffer;
    first_view.data = first.input->data;
    nested_view.view_src = &first_view;
    nested_view.buffer = first.input->buffer;
    nested_view.data = first.input->data;
    ggml_tensor shifted_view = *first.input;
    shifted_view.view_src = first.input;
    shifted_view.view_offs = sizeof(float);
    shifted_view.data =
        static_cast<uint8_t *>(first.input->data) + sizeof(float);
    shifted_view.ne[0] -= 1;
    ggml_tensor cycle_a {};
    ggml_tensor cycle_b {};
    cycle_a.view_src = &cycle_b;
    cycle_b.view_src = &cycle_a;
    ggml_backend_buffer_t cpu_buffer =
        ggml_backend_alloc_buffer(first.cpu, 64);
    ggml_tensor cpu_tensor {};
    cpu_tensor.buffer = cpu_buffer;
    cpu_tensor.data =
        cpu_buffer ? ggml_backend_buffer_get_base(cpu_buffer) : nullptr;
    ggml_backend_rpc_halofpx_storage_identity ignored {};
    const bool ok =
        ggml_backend_rpc_halofpx_resolve_storage_identity(
            first.rpc, first.input, &direct) ==
            GGML_RPC_HALOFPX_MUTABLE_ADMIT_SUCCESS &&
        ggml_backend_rpc_halofpx_resolve_storage_identity(
            first.rpc, &nested_view, &nested) ==
            GGML_RPC_HALOFPX_MUTABLE_ADMIT_SUCCESS &&
        ggml_backend_rpc_halofpx_resolve_storage_identity(
            first.rpc, &shifted_view, &shifted) ==
            GGML_RPC_HALOFPX_MUTABLE_ADMIT_SUCCESS &&
        memcmp(direct.storage_identity, nested.storage_identity, 32) == 0 &&
        memcmp(
            direct.runtime_semantic_identity,
            nested.runtime_semantic_identity, 32) == 0 &&
        memcmp(direct.storage_identity, shifted.storage_identity, 32) == 0 &&
        memcmp(
            direct.runtime_semantic_identity,
            shifted.runtime_semantic_identity, 32) != 0 &&
        ggml_backend_rpc_halofpx_resolve_storage_identity(
            first.rpc, &cycle_a, &ignored) ==
            GGML_RPC_HALOFPX_MUTABLE_ADMIT_VIEW_CYCLE &&
        ggml_backend_rpc_halofpx_resolve_storage_identity(
            first.rpc, &cpu_tensor, &ignored) ==
            GGML_RPC_HALOFPX_MUTABLE_ADMIT_STORAGE_NOT_RPC &&
        ggml_backend_rpc_halofpx_resolve_storage_identity(
            first.rpc, second.input, &ignored) ==
            GGML_RPC_HALOFPX_MUTABLE_ADMIT_WRONG_SOCKET &&
        false_rpc_destination_refusal(first);
    if (cpu_buffer) ggml_backend_buffer_free(cpu_buffer);
    free_fixture(first);
    free_fixture(second);
    return ok;
}

static bool execute_once(
        fixture & f,
        uint64_t sequence,
        bool new_graph,
        std::array<float, 32> & result,
        uint64_t & graph_uid,
        uint64_t & connection_epoch,
        uint64_t & allocation_epoch,
        std::atomic<uint32_t> * overlap = nullptr,
        ggml_tensor * foreign_tensor = nullptr,
        int admission_refusal_case = 0) {
    if (new_graph) {
        f.graph = ggml_new_graph(f.ctx);
        ggml_build_forward_expand(f.graph, f.output);
    }
    std::array<uint8_t, 32> nonce {};
    nonce_for(sequence, nonce.data());
    std::array<uint8_t, 65536> events {};
    ggml_backend_sched_authority_config config {};
    config.major = 1;
    config.minor = 0;
    config.encoded_size = sizeof(config);
    config.max_events = 256;
    config.event_buffer = events.data();
    config.event_buffer_size = events.size();
    config.execution_sequence = sequence;
    memcpy(config.attempt_nonce, nonce.data(), 32);
    std::array<uint8_t, 32> scheduler_key {};
    if (!graph_key(scheduler_key)) {
        std::fprintf(stderr, "fixture: key load failed\n");
        return false;
    }
    memcpy(config.key, scheduler_key.data(), scheduler_key.size());
    ggml_backend_sched_authority_handle sched_handle {};
    if (!ggml_backend_sched_authority_arm(f.sched, &config, &sched_handle)) {
        std::fprintf(stderr, "fixture: scheduler arm failed sequence=%llu\n",
            static_cast<unsigned long long>(sequence));
        return false;
    }
    memset(config.key, 0, sizeof(config.key));
    if (!ggml_backend_sched_authority_register_root(
            f.sched, &sched_handle, f.input, GGML_BACKEND_SCHED_AUTH_MUTABLE,
            GGML_RPC_HALOFPX_MUTABLE_TOKEN, 0) ||
        !ggml_backend_sched_authority_register_root(
            f.sched, &sched_handle, f.bias, GGML_BACKEND_SCHED_AUTH_IMMUTABLE_WEIGHT,
            1, 0) ||
        !ggml_backend_sched_authority_mark_rpc_backend(f.sched, &sched_handle, 0) ||
        !ggml_backend_sched_alloc_graph(f.sched, f.graph)) {
        std::fprintf(stderr, "fixture: scheduler arm/register/alloc failed sequence=%llu\n",
            static_cast<unsigned long long>(sequence));
        ggml_backend_sched_authority_abort_execution(f.sched, &sched_handle);
        return false;
    }
    graph_uid = ggml_backend_rpc_halofpx_mutable_graph_uid(f.graph);
    ggml_backend_sched_authority_prepared prepared {};
    struct ggml_backend_sched_authority_prepared_admission admission {};
    if (graph_uid == 0 ||
        !ggml_backend_sched_authority_prepare(
            f.sched, &sched_handle, f.graph, &prepared) ||
        !ggml_backend_sched_authority_resolve_census(
            f.sched, &sched_handle,
            [](void * user_data, uint32_t backend_ordinal,
               ggml_tensor * tensor,
               ggml_backend_sched_authority_storage_resolution * out) {
                auto * fixture_value = static_cast<fixture *>(user_data);
                if (fixture_value == nullptr || out == nullptr ||
                    backend_ordinal != 0) return false;
                ggml_backend_rpc_halofpx_storage_identity identity {};
                if (ggml_backend_rpc_halofpx_resolve_storage_identity(
                        fixture_value->rpc, tensor, &identity) !=
                        GGML_RPC_HALOFPX_MUTABLE_ADMIT_SUCCESS) return false;
                out->device = identity.device;
                out->connection_epoch = identity.connection_epoch;
                memcpy(out->endpoint_identity, identity.endpoint_identity, 32);
                memcpy(out->storage_identity, identity.storage_identity, 32);
                memcpy(
                    out->runtime_semantic_identity,
                    identity.runtime_semantic_identity, 32);
                return true;
            },
            &f)) {
        std::fprintf(stderr, "fixture: scheduler prepare failed sequence=%llu graph=%llu\n",
            static_cast<unsigned long long>(sequence),
            static_cast<unsigned long long>(graph_uid));
        ggml_backend_sched_authority_abort_execution(f.sched, &sched_handle);
        return false;
    }
    struct ggml_backend_sched_authority_projection_failure unexpected_failure {};
    if (ggml_backend_sched_authority_get_projection_failure(
            f.sched, &sched_handle, &unexpected_failure)) {
        std::fprintf(stderr,
            "fixture: successful census exported projection failure reason=%u\n",
            unexpected_failure.reason);
        ggml_backend_sched_authority_abort_execution(f.sched, &sched_handle);
        return false;
    }
    const size_t split_count =
        ggml_backend_sched_authority_split_count(f.sched, &sched_handle);
    if (split_count != 1) {
        std::fprintf(stderr, "fixture: split_count=%zu\n", split_count);
        return false;
    }
    ggml_backend_sched_authority_split split {};
    if (!ggml_backend_sched_authority_split_at(
            f.sched, &sched_handle, 0, &split) ||
        split.parent_graph_uid != graph_uid ||
        split.execution_sequence != sequence ||
        split.backend_ordinal != 0) {
        std::fprintf(stderr, "fixture: split mismatch parent=%llu graph=%llu exec=%llu want=%llu backend=%u\n",
            static_cast<unsigned long long>(split.parent_graph_uid),
            static_cast<unsigned long long>(graph_uid),
            static_cast<unsigned long long>(split.execution_sequence),
            static_cast<unsigned long long>(sequence), split.backend_ordinal);
        return false;
    }
    ggml_backend_rpc_halofpx_split_identity split_identity {
        split.split_graph_uid, split.split_ordinal, split.backend_ordinal
    };
    const bool rpc_armed =
        ggml_backend_rpc_halofpx_execution_arm(f.rpc, nonce.data(), sequence);
    const bool rpc_bound = rpc_armed &&
        ggml_backend_rpc_halofpx_execution_bind_splits(
            f.rpc, nonce.data(), sequence, graph_uid, split.mapping_root, 0,
            &split_identity, 1);
    if (!rpc_armed || !rpc_bound) {
        std::fprintf(stderr, "fixture: rpc arm/bind failed sequence=%llu arm=%d bind=%d\n",
            static_cast<unsigned long long>(sequence), rpc_armed ? 1 : 0, rpc_bound ? 1 : 0);
        return false;
    }
    ggml_backend_rpc_halofpx_mutable_preflight preflight {};
    ggml_backend_sched_authority_admission_expectation expectation {};
    expectation.major = 1;
    expectation.encoded_size = sizeof(expectation);
    expectation.allowed_operation =
        GGML_BACKEND_SCHED_OPERATION_AUTHENTICATED_EXECUTE;
    expectation.backend_ordinal = 0;
    if (!ggml_backend_rpc_halofpx_mutable_negotiate_preflight(f.rpc, 1, &preflight)) {
        std::fprintf(stderr, "fixture: preflight failed sequence=%llu\n",
            static_cast<unsigned long long>(sequence));
        return false;
    }
    expectation.key_generation = preflight.key_generation;
    expectation.client_connection_epoch = preflight.client_connection_epoch;
    expectation.server_connection_epoch = preflight.server_connection_epoch;
    expectation.allocation_topology_epoch = preflight.allocation_topology_epoch;
    expectation.issued_unix_ns =
        std::chrono::duration_cast<std::chrono::nanoseconds>(
            std::chrono::system_clock::now().time_since_epoch()).count();
    expectation.expires_unix_ns =
        expectation.issued_unix_ns + UINT64_C(30000000000);
    if (admission_refusal_case == 1) {
        expectation.issued_unix_ns -= UINT64_C(31000000000);
        expectation.expires_unix_ns -= UINT64_C(31000000000);
    }
    struct ggml_backend_sched_authority_prepared_admission expected_admission {};
    struct ggml_backend_sched_authority_prepared_admission mismatched_admission {};
    if (!ggml_backend_sched_authority_expected_prepared_admission(
            f.sched, &sched_handle, &expectation, &expected_admission) ||
        !ggml_backend_sched_authority_prepared_admission(
            f.sched, &sched_handle, &expectation, &admission) ||
        (admission_refusal_case != 1 &&
         !ggml_backend_sched_authority_verify_prepared_admission(
            &admission, scheduler_key.data(), &expected_admission))) {
        std::fprintf(stderr, "fixture: admission failed sequence=%llu epochs=%llu/%llu/%llu\n",
            static_cast<unsigned long long>(sequence),
            static_cast<unsigned long long>(preflight.client_connection_epoch),
            static_cast<unsigned long long>(preflight.server_connection_epoch),
            static_cast<unsigned long long>(preflight.allocation_topology_epoch));
        return false;
    }
    if (admission_refusal_case != 0) {
        const bool refused = admission_refusal_case == 1 ?
            (!ggml_backend_sched_authority_verify_prepared_admission(
                 &admission, scheduler_key.data(), &expected_admission) &&
             !ggml_backend_sched_authority_consume_prepared_admission(
                 f.sched, &sched_handle, &admission)) :
            (ggml_backend_sched_authority_abort_prepared_admission(
                 f.sched, &sched_handle) &&
             !ggml_backend_sched_authority_consume_prepared_admission(
                 f.sched, &sched_handle, &admission));
        ggml_backend_sched_authority_abort_execution(f.sched, &sched_handle);
        ggml_backend_rpc_halofpx_execution_disarm(f.rpc, nonce.data(), sequence);
        ggml_backend_sched_reset(f.sched);
        return refused;
    }
    mismatched_admission = expected_admission;
    mismatched_admission.logical_expected_mutable_count ^= 1;
    if (ggml_backend_sched_authority_verify_prepared_admission(
            &admission, scheduler_key.data(), &mismatched_admission) ||
        ggml_backend_sched_authority_verify_prepared_admission(
            &admission, scheduler_key.data(), &admission)) {
        std::fprintf(stderr, "fixture: independent admission comparison accepted invalid state\n");
        return false;
    }
    const size_t canonical_count =
        ggml_backend_sched_authority_census_count(
            f.sched, &sched_handle, expectation.backend_ordinal);
    std::vector<ggml_backend_sched_authority_census_entry> canonical_census(
        canonical_count);
    uint32_t canonical_register = 0;
    uint32_t canonical_exclude = 0;
    for (size_t i = 0; i < canonical_count; ++i) {
        if (!ggml_backend_sched_authority_census_at(
                f.sched, &sched_handle, expectation.backend_ordinal, i,
                &canonical_census[i])) {
            return false;
        }
        canonical_register += canonical_census[i].disposition ==
            GGML_BACKEND_SCHED_CENSUS_REGISTER;
        canonical_exclude += canonical_census[i].disposition ==
            GGML_BACKEND_SCHED_CENSUS_EXCLUDE;
    }
    std::vector<std::pair<uint32_t, uint32_t>> canonical_mutable;
    std::vector<std::pair<uint32_t, uint32_t>> canonical_excluded;
    for (const auto & entry : canonical_census) {
        auto & target =
            entry.disposition == GGML_BACKEND_SCHED_CENSUS_REGISTER ?
                canonical_mutable : canonical_excluded;
        target.emplace_back(
            entry.disposition == GGML_BACKEND_SCHED_CENSUS_REGISTER ?
                entry.role : UINT32_C(0x80000000) + entry.role,
            entry.role_ordinal);
    }
    std::sort(canonical_mutable.begin(), canonical_mutable.end());
    std::sort(canonical_excluded.begin(), canonical_excluded.end());
    std::vector<uint8_t> canonical_plan;
    census_le<uint32_t>(canonical_plan, canonical_mutable.size());
    census_le<uint32_t>(canonical_plan, canonical_excluded.size());
    for (const auto & value : canonical_mutable) {
        census_le<uint32_t>(canonical_plan, value.first);
        census_le<uint32_t>(canonical_plan, value.second);
    }
    for (const auto & value : canonical_excluded) {
        census_le<uint32_t>(canonical_plan, value.first);
        census_le<uint32_t>(canonical_plan, value.second);
    }
    for (const auto & entry : canonical_census) {
        canonical_plan.insert(
            canonical_plan.end(), entry.logical_tensor_identity,
            entry.logical_tensor_identity + 32);
        canonical_plan.insert(
            canonical_plan.end(), entry.storage_tensor_identity,
            entry.storage_tensor_identity + 32);
        canonical_plan.insert(
            canonical_plan.end(), entry.runtime_semantic_identity,
            entry.runtime_semantic_identity + 32);
        census_le<uint32_t>(canonical_plan, entry.disposition);
        census_le<uint32_t>(canonical_plan, entry.role);
        census_le<uint32_t>(canonical_plan, entry.role_ordinal);
        census_le<uint32_t>(
            canonical_plan, entry.destination_backend_ordinal);
        canonical_plan.insert(
            canonical_plan.end(), entry.rpc_endpoint_identity,
            entry.rpc_endpoint_identity + 32);
        census_le<uint32_t>(canonical_plan, entry.rpc_device);
        census_le<uint64_t>(
            canonical_plan, entry.rpc_connection_epoch);
    }
    std::array<uint8_t, 32> canonical_root {};
    sha256_t canonical_sha;
    sha256_init(&canonical_sha);
    sha256_update(
        &canonical_sha, canonical_plan.data(), canonical_plan.size());
    sha256_final(&canonical_sha, canonical_root.data());
    if (canonical_count == 0 ||
        canonical_register != admission.logical_expected_mutable_count ||
        canonical_exclude != admission.logical_expected_exclusion_count ||
        std::memcmp(
            canonical_root.data(), admission.logical_expected_census_root,
            canonical_root.size()) != 0) {
        std::fprintf(stderr, "fixture: exported census/count mismatch\n");
        return false;
    }

    ggml_backend_rpc_halofpx_mutable_attempt attempt {};
    attempt.version = 3;
    attempt.max_mutations = 16;
    attempt.max_census_entries = 16;
    attempt.graph_uid = graph_uid;
    attempt.execution_sequence = sequence;
    memcpy(attempt.attempt_nonce, nonce.data(), 32);
    std::array<float, 32> input {};
    std::array<float, 32> bias {};
    for (size_t i = 0; i < input.size(); ++i) {
        input[i] = static_cast<float>(i) / 32.0f;
        bias[i] = 0.25f;
    }
    ggml_backend_tensor_set(f.bias, bias.data(), 0, sizeof(bias));
    ggml_backend_rpc_halofpx_mutable_session session {};
    if (!ggml_backend_rpc_halofpx_mutable_begin(
            f.rpc, &admission, &expected_admission, &attempt, &session)) {
        std::fprintf(stderr, "fixture: mutable begin failed sequence=%llu\n",
            static_cast<unsigned long long>(sequence));
        ggml_backend_rpc_halofpx_execution_disarm(
            f.rpc, nonce.data(), sequence);
        return false;
    }
    if (overlap != nullptr) {
        overlap->fetch_add(1, std::memory_order_acq_rel);
        const auto deadline = std::chrono::steady_clock::now() +
            std::chrono::seconds(10);
        while (overlap->load(std::memory_order_acquire) < 2) {
            if (std::chrono::steady_clock::now() > deadline) {
                ggml_backend_rpc_halofpx_mutable_abort(&session);
                ggml_backend_rpc_halofpx_execution_disarm(
                    f.rpc, nonce.data(), sequence);
                return false;
            }
            std::this_thread::yield();
        }
    }
    GGML_UNUSED(foreign_tensor);
    for (const auto & entry : canonical_census) {
        const auto admit_result =
            entry.disposition == GGML_BACKEND_SCHED_CENSUS_REGISTER ?
            ggml_backend_rpc_halofpx_mutable_register_typed(
                &session, entry.runtime_tensor,
                static_cast<ggml_backend_rpc_halofpx_mutable_role>(entry.role),
                entry.role_ordinal, &entry) :
            entry.disposition == GGML_BACKEND_SCHED_CENSUS_EXCLUDE ?
            ggml_backend_rpc_halofpx_mutable_exclude_typed(
                &session, entry.runtime_tensor,
                static_cast<ggml_backend_rpc_halofpx_exclusion>(entry.role),
                entry.role_ordinal, &entry) :
            GGML_RPC_HALOFPX_MUTABLE_ADMIT_INVALID_ARGUMENT;
        const bool admitted =
            admit_result == GGML_RPC_HALOFPX_MUTABLE_ADMIT_SUCCESS ||
            admit_result == GGML_RPC_HALOFPX_MUTABLE_ADMIT_EXACT_DUPLICATE;
        if (!admitted) {
            std::fprintf(stderr,
                "fixture: canonical census iteration failed sequence=%llu index=%zu\n",
                static_cast<unsigned long long>(sequence),
                static_cast<size_t>(&entry - canonical_census.data()));
            ggml_backend_rpc_halofpx_mutable_abort(&session);
            ggml_backend_rpc_halofpx_execution_disarm(
                f.rpc, nonce.data(), sequence);
            return false;
        }
    }
    connection_epoch = session.connection_epoch;
    allocation_epoch = session.allocation_epoch;
    ggml_backend_tensor_set(f.input, input.data(), 0, sizeof(input));
    ggml_backend_rpc_halofpx_mutable_result mutable_result {};
    const bool mutable_prepared =
        ggml_backend_rpc_halofpx_mutable_prepare(
            &session, f.graph, &mutable_result);
    const bool admission_consumed = mutable_prepared &&
        ggml_backend_sched_authority_consume_prepared_admission(
            f.sched, &sched_handle, &admission);
    const ggml_status compute_status = admission_consumed ?
        ggml_backend_sched_graph_compute(f.sched, f.graph) :
        GGML_STATUS_FAILED;
    const bool mutable_committed =
        compute_status == GGML_STATUS_SUCCESS &&
        ggml_backend_rpc_halofpx_mutable_commit(
            &session, f.graph, &mutable_result);
    if (!mutable_prepared || !admission_consumed ||
        compute_status != GGML_STATUS_SUCCESS || !mutable_committed) {
        std::fprintf(stderr,
            "fixture: prepare/compute/commit failed sequence=%llu "
            "prepare=%u consume=%u compute=%d commit=%u\n",
            static_cast<unsigned long long>(sequence),
            mutable_prepared ? 1u : 0u, admission_consumed ? 1u : 0u,
            static_cast<int>(compute_status), mutable_committed ? 1u : 0u);
        ggml_backend_rpc_halofpx_mutable_abort(&session);
        ggml_backend_rpc_halofpx_execution_disarm(
            f.rpc, nonce.data(), sequence);
        return false;
    }
    ggml_backend_tensor_get(f.output, result.data(), 0, sizeof(result));
    struct ggml_backend_sched_authority_result sched_result {};
    const bool finalized = ggml_backend_sched_authority_finalize_execution(
        f.sched, &sched_handle, &sched_result);
    const auto stale_session = session;
    const bool closed = ggml_backend_rpc_halofpx_mutable_abort(&session) &&
        ggml_backend_rpc_halofpx_execution_disarm(f.rpc, nonce.data(), sequence);
    const bool post_abort_refused =
        !ggml_backend_rpc_halofpx_mutable_register(
            &stale_session, f.input, GGML_RPC_HALOFPX_MUTABLE_TOKEN, 0);
    if (!finalized || !closed || !post_abort_refused || mutable_result.status != 1 ||
        mutable_result.census_count != 2 || mutable_result.mutation_count != 1 ||
        ggml_backend_sched_authority_consume_prepared_admission(
            f.sched, &sched_handle, &admission)) {
        return false;
    }
    for (size_t i = 0; i < result.size(); ++i) {
        const float want = (input[i] + bias[i]) * (input[i] + bias[i]);
        if (!std::isfinite(result[i]) || std::abs(result[i] - want) > 1e-6f) return false;
    }
    ggml_backend_sched_reset(f.sched);
    return true;
}

int main(int argc, char ** argv) {
    if (argc == 4 && std::strcmp(argv[1], "--storage-identity") == 0) {
        ggml_backend_load_all();
        const bool ok =
            ggml_backend_sched_authority_self_test() == 0x7fffffU &&
            storage_identity_fixture(argv[2], argv[3]);
        std::printf(
            "resolved_storage_identity=%d false_rpc_destination_refused=%d\n",
            ok ? 1 : 0, ok ? 1 : 0);
        return ok ? 0 : 1;
    }
    if (argc == 2 && std::strcmp(argv[1], "--publication-self-test") == 0) {
        const bool published =
            ggml_backend_rpc_halofpx_preexecute_publication_self_test();
        std::printf("immutable_publication=%d\n", published ? 1 : 0);
        return published ? 0 : 1;
    }
    if (argc == 3 && std::strcmp(argv[1], "--feature-off") == 0) {
        ggml_backend_load_all();
        ggml_backend_t rpc = ggml_backend_rpc_init(argv[2], 0);
        ggml_backend_rpc_halofpx_mutable_attempt attempt {};
        ggml_backend_rpc_halofpx_mutable_session session {};
        attempt.version = 3;
        attempt.graph_uid = 1;
        attempt.execution_sequence = 1;
        attempt.max_mutations = 1;
        attempt.max_census_entries = 1;
        attempt.attempt_nonce[0] = 1;
        const bool inert = rpc != nullptr &&
            !ggml_backend_rpc_halofpx_mutable_begin(
                rpc, nullptr, nullptr, &attempt, &session) &&
            !ggml_backend_rpc_halofpx_mutable_abort(&session);
        ggml_backend_free(rpc);
        std::printf("feature_off_inert=%d\n", inert ? 1 : 0);
        return inert ? 0 : 1;
    }
    if (argc == 3 && std::strcmp(argv[1], "--single-success") == 0) {
        ggml_backend_load_all();
        fixture state {};
        if (!make_fixture(argv[2], state)) return 2;
        std::array<float, 32> output {};
        uint64_t uid = 0, connection = 0, allocation = 0;
        const bool succeeded = execute_once(
            state, 1, false, output, uid, connection, allocation);
        free_fixture(state);
        std::printf(
            "single_success=%d uid=%llu connection=%llu allocation=%llu\n",
            succeeded ? 1 : 0, static_cast<unsigned long long>(uid),
            static_cast<unsigned long long>(connection),
            static_cast<unsigned long long>(allocation));
        return succeeded ? 0 : 1;
    }
    if (argc == 3 && std::strcmp(argv[1], "--expect-transport-failure") == 0) {
        ggml_backend_load_all();
        fixture failed {};
        if (!make_fixture(argv[2], failed)) return 2;
        std::array<float, 32> output {};
        uint64_t uid = 0, connection = 0, allocation = 0;
        const bool refused = !execute_once(
            failed, 1, false, output, uid, connection, allocation);
        free_fixture(failed);
        std::printf("transport_failure_refused=%d\n", refused ? 1 : 0);
        return refused ? 0 : 1;
    }
    if (argc == 3 && std::strcmp(argv[1], "--admission-refusals") == 0) {
        ggml_backend_load_all();
        fixture state {};
        if (!make_fixture(argv[2], state)) return 2;
        std::array<float, 32> output {};
        uint64_t uid = 0, connection = 0, allocation = 0;
        const bool expired = execute_once(
            state, 10, false, output, uid, connection, allocation,
            nullptr, nullptr, 1);
        const bool aborted = execute_once(
            state, 11, false, output, uid, connection, allocation,
            nullptr, nullptr, 2);
        free_fixture(state);
        std::printf("expired_refused=%d aborted_refused=%d\n",
                    expired ? 1 : 0, aborted ? 1 : 0);
        return expired && aborted ? 0 : 1;
    }
    if (argc != 3) {
        std::fprintf(stderr, "usage: %s HOST:PORT SECOND_HOST:PORT\n", argv[0]);
        return 2;
    }
    const uint32_t mutable_self_test = ggml_backend_rpc_halofpx_mutable_auth_self_test();
    if (mutable_self_test != 0x3ffffU) {
        std::fprintf(stderr, "mutable authority self-test failed: 0x%x\n", mutable_self_test);
        return 1;
    }
    ggml_backend_load_all();
    fixture first {};
    fixture second {};
    if (!make_fixture(argv[1], first) || !make_fixture(argv[2], second)) return 2;
    std::array<float, 32> first_output {};
    std::array<float, 32> recompute_output {};
    std::array<float, 32> second_output {};
    uint64_t uid1 = 0, uid2 = 0, uid3 = 0;
    uint64_t conn1 = 0, conn2 = 0, conn3 = 0;
    uint64_t alloc1 = 0, alloc2 = 0, alloc3 = 0;
    std::atomic<uint32_t> overlap { 0 };
    bool first_ok = false;
    bool concurrent_ok = false;
    std::thread primary([&] {
        first_ok = execute_once(
            first, 1, false, first_output, uid1, conn1, alloc1, &overlap,
            second.input);
    });
    std::thread other([&] {
        concurrent_ok = execute_once(
            second, 1, true, second_output, uid3, conn3, alloc3, &overlap,
            first.input);
    });
    primary.join();
    other.join();
    ggml_backend_buffer_t rollover = ggml_backend_alloc_buffer(first.rpc, 64);
    const bool rollover_ok = rollover != nullptr;
    ggml_backend_buffer_free(rollover);
    const bool recompute_ok = first_ok && execute_once(
        first, 2, false, recompute_output, uid2, conn2, alloc2);
    const bool exact =
        std::memcmp(first_output.data(), recompute_output.data(), sizeof(first_output)) == 0 &&
        std::memcmp(first_output.data(), second_output.data(), sizeof(first_output)) == 0;
    const bool authority =
        uid1 != 0 && uid2 != 0 && uid3 != 0 &&
        uid1 != uid2 && uid1 != uid3 && uid2 != uid3 &&
        conn1 != 0 && conn1 == conn2 && conn3 != 0 &&
        alloc1 != 0 && alloc2 > alloc1 && alloc3 != 0 && rollover_ok;
    std::printf(
        "real_composed=%d recompute=%d concurrent=%d exact=%d "
        "uids=%llu/%llu/%llu connection_epochs=%llu/%llu/%llu "
        "allocation_epochs=%llu/%llu/%llu\n",
        first_ok ? 1 : 0, recompute_ok ? 1 : 0, concurrent_ok ? 1 : 0,
        exact ? 1 : 0,
        static_cast<unsigned long long>(uid1),
        static_cast<unsigned long long>(uid2),
        static_cast<unsigned long long>(uid3),
        static_cast<unsigned long long>(conn1),
        static_cast<unsigned long long>(conn2),
        static_cast<unsigned long long>(conn3),
        static_cast<unsigned long long>(alloc1),
        static_cast<unsigned long long>(alloc2),
        static_cast<unsigned long long>(alloc3));
    free_fixture(first);
    free_fixture(second);
    return first_ok && recompute_ok && concurrent_ok && exact && authority ? 0 : 1;
}
