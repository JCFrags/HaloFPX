#include "ggml.h"
#include "ggml-alloc.h"
#include "ggml-backend.h"
#include "ggml-rpc.h"

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
#include <vector>

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
            f.sched, &sched_handle, f.graph, &prepared)) {
        std::fprintf(stderr, "fixture: scheduler prepare failed sequence=%llu graph=%llu\n",
            static_cast<unsigned long long>(sequence),
            static_cast<unsigned long long>(graph_uid));
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
    if (!ggml_backend_rpc_halofpx_mutable_register(
            &session, f.input, GGML_RPC_HALOFPX_MUTABLE_TOKEN, 0) ||
        !ggml_backend_rpc_halofpx_mutable_exclude(
            &session, f.bias, GGML_RPC_HALOFPX_EXCLUDE_IMMUTABLE_MODEL_WEIGHT, 0)) {
        std::fprintf(stderr, "fixture: mutable begin/register failed sequence=%llu\n",
            static_cast<unsigned long long>(sequence));
        ggml_backend_rpc_halofpx_mutable_abort(&session);
        ggml_backend_rpc_halofpx_execution_disarm(
            f.rpc, nonce.data(), sequence);
        return false;
    }
    connection_epoch = session.connection_epoch;
    allocation_epoch = session.allocation_epoch;
    ggml_backend_tensor_set(f.input, input.data(), 0, sizeof(input));
    ggml_backend_rpc_halofpx_mutable_result mutable_result {};
    if (!ggml_backend_rpc_halofpx_mutable_prepare(
            &session, f.graph, &mutable_result) ||
        !ggml_backend_sched_authority_consume_prepared_admission(
            f.sched, &sched_handle, &admission) ||
        ggml_backend_sched_graph_compute(f.sched, f.graph) != GGML_STATUS_SUCCESS ||
        !ggml_backend_rpc_halofpx_mutable_commit(
            &session, f.graph, &mutable_result)) {
        std::fprintf(stderr, "fixture: prepare/compute/commit failed sequence=%llu\n",
            static_cast<unsigned long long>(sequence));
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
