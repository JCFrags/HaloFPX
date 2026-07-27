#include "ggml.h"
#include "ggml-backend.h"
#include "ggml-alloc.h"
#include "ggml-rpc.h"

#include <array>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <vector>

extern "C" uint32_t ggml_backend_rpc_halofpx_mutable_auth_self_test(void);

static void fill_attempt(
        ggml_backend_rpc_halofpx_mutable_attempt & attempt,
        uint64_t graph_uid,
        uint64_t sequence,
        const uint8_t scheduler_nonce[32]) {
    attempt = {};
    attempt.version = 1;
    attempt.max_mutations = 16;
    attempt.max_census_entries = 16;
    attempt.graph_uid = graph_uid;
    attempt.execution_sequence = sequence;
    attempt.scheduler_execution_sequence = sequence;
    for (size_t i = 0; i < 32; ++i) {
        attempt.attempt_nonce[i] = static_cast<uint8_t>(0x20 + sequence + i);
        attempt.scheduler_attempt_nonce[i] = scheduler_nonce[i];
        attempt.scheduler_transcript_root[i] = static_cast<uint8_t>(0xa0 + i);
    }
}

static bool split_identity_refusals(ggml_backend_t rpc) {
    std::array<uint8_t, 32> nonce {};
    std::array<uint8_t, 32> root {};
    for (size_t i = 0; i < nonce.size(); ++i) {
        nonce[i] = static_cast<uint8_t>(0x31 + i);
        root[i] = static_cast<uint8_t>(0xa1 + i);
    }
    const ggml_backend_rpc_halofpx_split_identity exact[] = {
        { 1001, 0, 0 },
        { 1002, 2, 0 },
    };
    const ggml_backend_rpc_halofpx_split_identity duplicate[] = {
        { 1001, 0, 0 },
        { 1001, 2, 0 },
    };
    const ggml_backend_rpc_halofpx_split_identity reordered[] = {
        { 1001, 2, 0 },
        { 1002, 0, 0 },
    };
    const ggml_backend_rpc_halofpx_split_identity wrong_backend[] = {
        { 1001, 0, 1 },
    };
    std::array<uint8_t, 32> zero {};
    ggml_backend_rpc_halofpx_graph_result result {};
    ggml_backend_rpc_halofpx_graph_result_reason reason =
        GGML_RPC_HALOFPX_GRAPH_RESULT_OK;
    bool ok = ggml_backend_rpc_halofpx_execution_arm(rpc, nonce.data(), 900) &&
        !ggml_backend_rpc_halofpx_execution_bind_splits(
            rpc, nonce.data(), 900, 0, root.data(), 0, exact, 2) &&
        !ggml_backend_rpc_halofpx_execution_bind_splits(
            rpc, nonce.data(), 900, 999, zero.data(), 0, exact, 2) &&
        !ggml_backend_rpc_halofpx_execution_bind_splits(
            rpc, nonce.data(), 900, 999, root.data(), 0, duplicate, 2) &&
        !ggml_backend_rpc_halofpx_execution_bind_splits(
            rpc, nonce.data(), 900, 999, root.data(), 0, reordered, 2) &&
        !ggml_backend_rpc_halofpx_execution_bind_splits(
            rpc, nonce.data(), 900, 999, root.data(), 0, wrong_backend, 1) &&
        ggml_backend_rpc_halofpx_execution_bind_splits(
            rpc, nonce.data(), 900, 999, root.data(), 0, exact, 2) &&
        !ggml_backend_rpc_halofpx_execution_bind_splits(
            rpc, nonce.data(), 900, 999, root.data(), 0, exact, 2) &&
        !ggml_backend_rpc_halofpx_graph_result_for_split(
            rpc, 998, root.data(), 1001, 0, 0, 900, &result, &reason) &&
        reason == GGML_RPC_HALOFPX_GRAPH_RESULT_STATUS_NOT_EXECUTED &&
        !ggml_backend_rpc_halofpx_graph_result_for_split(
            rpc, 999, root.data(), 1001, 0, 1, 900, &result, &reason) &&
        reason == GGML_RPC_HALOFPX_GRAPH_RESULT_STATUS_NOT_EXECUTED &&
        !ggml_backend_rpc_halofpx_graph_result_for_split(
            rpc, 999, root.data(), 1001, 0, 0, 899, &result, &reason) &&
        reason == GGML_RPC_HALOFPX_GRAPH_RESULT_STATUS_NOT_EXECUTED &&
        !ggml_backend_rpc_halofpx_graph_result_for_split(
            rpc, 999, root.data(), 1001, 0, 0, 900, &result, &reason) &&
        reason == GGML_RPC_HALOFPX_GRAPH_RESULT_GRAPH_UID_ZERO &&
        !ggml_backend_rpc_halofpx_execution_disarm(rpc, nonce.data(), 899);
    // A mismatched disarm closes the authority fail-closed; it cannot be reused.
    ok = ok &&
        !ggml_backend_rpc_halofpx_execution_bind_splits(
            rpc, nonce.data(), 900, 999, root.data(), 0, exact, 2) &&
        ggml_backend_rpc_halofpx_execution_arm(rpc, nonce.data(), 901) &&
        ggml_backend_rpc_halofpx_execution_disarm(rpc, nonce.data(), 901);
    return ok;
}

int main(int argc, char ** argv) {
    if (argc == 3 && std::strcmp(argv[1], "--split-refusals") == 0) {
        ggml_backend_load_all();
        ggml_backend_t backend = ggml_backend_rpc_init(argv[2], 0);
        const bool ok = backend != nullptr && split_identity_refusals(backend);
        ggml_backend_free(backend);
        std::printf("split_identity_refusals=%d\n", ok ? 1 : 0);
        return ok ? 0 : 1;
    }
    if (argc == 3 && std::strcmp(argv[1], "--feature-off") == 0) {
        ggml_backend_load_all();
        ggml_backend_t backend = ggml_backend_rpc_init(argv[2], 0);
        size_t free = 0;
        size_t total = 0;
        ggml_backend_rpc_get_device_memory(argv[2], 0, &free, &total);
        ggml_backend_rpc_halofpx_mutable_attempt attempt {};
        ggml_backend_rpc_halofpx_mutable_session session {};
        attempt.version = 1;
        attempt.graph_uid = 1;
        attempt.execution_sequence = 1;
        attempt.max_mutations = 1;
        attempt.max_census_entries = 1;
        attempt.attempt_nonce[0] = 1;
        const bool inert = backend && free > 0 && total >= free &&
            !ggml_backend_rpc_halofpx_mutable_begin(backend, nullptr, &attempt, &session) &&
            !ggml_backend_rpc_halofpx_mutable_abort(&session);
        ggml_backend_free(backend);
        std::printf("feature_off_inert=%d free=%zu total=%zu\n", inert, free, total);
        return inert ? 0 : 1;
    }
    if (argc != 3) {
        std::fprintf(stderr, "usage: %s HOST:PORT SECOND_HOST:PORT\n", argv[0]);
        return 2;
    }
    if (ggml_backend_rpc_halofpx_mutable_auth_self_test() != 0x3ffffU) {
        std::fprintf(stderr, "mutable authority refusal self-test failed\n");
        return 1;
    }
    ggml_backend_load_all();
    ggml_backend_t rpc = ggml_backend_rpc_init(argv[1], 0);
    ggml_backend_t rpc2 = ggml_backend_rpc_init(argv[2], 0);
    ggml_backend_t cpu = ggml_backend_init_by_type(GGML_BACKEND_DEVICE_TYPE_CPU, nullptr);
    if (!rpc || !rpc2 || !cpu) return 2;
    if (!split_identity_refusals(rpc2)) {
        std::fprintf(stderr, "split identity refusal contract failed\n");
        return 1;
    }
    std::vector<uint8_t> metadata(ggml_tensor_overhead() * 10 + ggml_graph_overhead());
    ggml_init_params params { metadata.size(), metadata.data(), true };
    ggml_context * ctx = ggml_init(params);
    if (!ctx) return 2;
    ggml_tensor * token = ggml_new_tensor_1d(ctx, GGML_TYPE_F32, 32);
    ggml_tensor * storage = ggml_new_tensor_2d(ctx, GGML_TYPE_F32, 40, 2);
    ggml_tensor * strided = ggml_view_2d(ctx, storage, 32, 1, storage->nb[1], 4 * sizeof(float));
    ggml_tensor * nested = ggml_view_1d(ctx, strided, 32, 0);
    ggml_set_input(token);
    // Deliberately unflagged: structural registration, not INPUT, is authority.
    ggml_tensor * sum = ggml_add(ctx, token, nested);
    ggml_tensor * out = ggml_sqr(ctx, sum);
    ggml_set_output(out);
    ggml_backend_t backends[] = { rpc, cpu };
    ggml_backend_buffer_type_t bufts[] = {
        ggml_backend_get_default_buffer_type(rpc),
        ggml_backend_get_default_buffer_type(cpu),
    };
    ggml_backend_sched_t sched = ggml_backend_sched_new(backends, bufts, 2, 64, false, false);
    if (!sched) return 2;
    for (ggml_tensor * tensor : { token, storage, strided, nested, sum, out }) {
        ggml_backend_sched_set_tensor_backend(sched, tensor, rpc);
    }
    std::array<uint8_t, 65536> scheduler_events {};
    std::array<uint8_t, 32> scheduler_nonce {};
    ggml_backend_sched_authority_config scheduler_config {};
    scheduler_config.major = 1;
    scheduler_config.minor = 0;
    scheduler_config.encoded_size = sizeof(scheduler_config);
    scheduler_config.max_events = 256;
    scheduler_config.event_buffer_size = scheduler_events.size();
    scheduler_config.execution_sequence = 1;
    for (size_t i = 0; i < 32; ++i) {
        scheduler_nonce[i] = static_cast<uint8_t>(0x60 + i);
        scheduler_config.attempt_nonce[i] = scheduler_nonce[i];
        scheduler_config.key[i] = static_cast<uint8_t>(0xc0 + i);
    }
    scheduler_config.event_buffer = scheduler_events.data();
    if (!ggml_backend_sched_authority_enable(sched, &scheduler_config)) return 2;
    std::memset(scheduler_config.key, 0, sizeof(scheduler_config.key));
    ggml_cgraph * graph = ggml_new_graph(ctx);
    ggml_build_forward_expand(graph, out);
    if (!ggml_backend_sched_alloc_graph(sched, graph)) return 2;
    const uint64_t graph_uid = ggml_backend_rpc_halofpx_mutable_graph_uid(graph);
    if (graph_uid == 0) return 2;
    ggml_backend_rpc_halofpx_mutable_attempt attempt {};
    ggml_backend_rpc_halofpx_mutable_session session {};
    fill_attempt(attempt, graph_uid, 1, scheduler_nonce.data());
    struct ggml_backend_sched_authority_admission admission_probe {};
    if (!ggml_backend_sched_authority_admission(sched, &admission_probe)) {
        std::fprintf(stderr, "scheduler admission unavailable\n"); return 1;
    }
    if (!ggml_backend_rpc_halofpx_mutable_begin(rpc, sched, &attempt, &session) ||
        !ggml_backend_rpc_halofpx_mutable_register(
            &session, token, GGML_RPC_HALOFPX_MUTABLE_TOKEN, 0) ||
        !ggml_backend_rpc_halofpx_mutable_register(
            &session, storage, GGML_RPC_HALOFPX_MUTABLE_ARCHITECTURE_INPUT, 0) ||
        !ggml_backend_rpc_halofpx_mutable_register(
            &session, nested, GGML_RPC_HALOFPX_MUTABLE_ARCHITECTURE_INPUT, 1)) {
        std::fprintf(stderr, "begin1 failed\n"); return 1;
    }
    std::vector<uint8_t> other_metadata(ggml_tensor_overhead() * 2);
    ggml_init_params other_params { other_metadata.size(), other_metadata.data(), true };
    ggml_context * other_ctx = ggml_init(other_params);
    ggml_tensor * other = other_ctx ? ggml_new_tensor_1d(other_ctx, GGML_TYPE_F32, 8) : nullptr;
    ggml_backend_buffer_t other_buffer = other_ctx ? ggml_backend_alloc_ctx_tensors(other_ctx, rpc2) : nullptr;
    ggml_backend_rpc_halofpx_mutable_attempt other_attempt {};
    fill_attempt(other_attempt, graph_uid + 1000, 101, scheduler_nonce.data());
    other_attempt.scheduler_execution_sequence = 1;
    ggml_backend_rpc_halofpx_mutable_session other_session {};
    if (!other_buffer ||
        !ggml_backend_rpc_halofpx_mutable_begin(rpc2, sched, &other_attempt, &other_session) ||
        !ggml_backend_rpc_halofpx_mutable_register(
            &other_session, other, GGML_RPC_HALOFPX_MUTABLE_TOKEN, 77) ||
        ggml_backend_rpc_halofpx_mutable_register(
            &session, other, GGML_RPC_HALOFPX_MUTABLE_TOKEN, 77) ||
        ggml_backend_rpc_halofpx_mutable_register(
            &other_session, token, GGML_RPC_HALOFPX_MUTABLE_TOKEN, 78)) {
        std::fprintf(stderr, "concurrent session isolation failed\n"); return 1;
    }
    auto stale_session = other_session;
    if (!ggml_backend_rpc_halofpx_mutable_abort(&other_session) ||
        ggml_backend_rpc_halofpx_mutable_register(
            &stale_session, other, GGML_RPC_HALOFPX_MUTABLE_TOKEN, 77) ||
        ggml_backend_rpc_halofpx_mutable_abort(&stale_session)) {
        std::fprintf(stderr, "stale session refusal failed\n"); return 1;
    }
    for (uint32_t test_case = GGML_RPC_HALOFPX_MUTABLE_TEST_MALFORMED;
         test_case <= GGML_RPC_HALOFPX_MUTABLE_TEST_WRONG_VIEW; ++test_case) {
        fill_attempt(other_attempt, graph_uid + 1000 + test_case, 101 + test_case, scheduler_nonce.data());
        other_attempt.scheduler_execution_sequence = 1;
        uint32_t handler_status = UINT32_MAX;
        if (!ggml_backend_rpc_halofpx_mutable_begin(rpc2, sched, &other_attempt, &other_session) ||
            !ggml_backend_rpc_halofpx_mutable_register(
                &other_session, other, GGML_RPC_HALOFPX_MUTABLE_TOKEN, 77) ||
            !ggml_backend_rpc_halofpx_mutable_test_inject(
                &other_session, other,
                static_cast<ggml_backend_rpc_halofpx_mutable_test_case>(test_case),
                &handler_status) ||
            handler_status != 0 ||
            !ggml_backend_rpc_halofpx_mutable_abort(&other_session)) {
            std::fprintf(stderr, "real handler refusal case %u failed status=%u\n", test_case, handler_status);
            return 1;
        }
        std::printf("handler_case_%u_status=%u authenticated=1\n", test_case, handler_status);
    }
    fill_attempt(other_attempt, graph_uid + 2000, 200, scheduler_nonce.data());
    other_attempt.scheduler_execution_sequence = 1;
    if (!ggml_backend_rpc_halofpx_mutable_begin(rpc2, sched, &other_attempt, &other_session) ||
        !ggml_backend_rpc_halofpx_mutable_register(
            &other_session, other, GGML_RPC_HALOFPX_MUTABLE_TOKEN, 88)) return 1;
    auto freed_buffer_session = other_session;
    ggml_backend_buffer_free(other_buffer);
    if (ggml_backend_rpc_halofpx_mutable_abort(&freed_buffer_session)) {
        std::fprintf(stderr, "buffer teardown left session authority\n"); return 1;
    }
    ggml_free(other_ctx);
    std::array<float, 32> av {};
    std::vector<float> bv(3 * 1024 * 1024);
    for (size_t i = 0; i < av.size(); ++i) av[i] = static_cast<float>(i) / 16.0f;
    for (size_t i = 0; i < bv.size(); ++i) bv[i] = static_cast<float>((i % 19) + 3) / 64.0f;
    ggml_backend_tensor_set(token, av.data(), 0, sizeof(av));
    ggml_backend_tensor_set(storage, bv.data(), 0, ggml_nbytes(storage));
    ggml_backend_rpc_halofpx_mutable_result first_authority {};
    if (!ggml_backend_rpc_halofpx_mutable_prepare(&session, graph, &first_authority)) {
        std::fprintf(stderr, "prepare1 failed\n"); return 1;
    }
    if (ggml_backend_sched_graph_compute(sched, graph) != GGML_STATUS_SUCCESS) {
        std::fprintf(stderr, "compute1 failed\n"); return 1;
    }
    if (!ggml_backend_rpc_halofpx_mutable_commit(&session, graph, &first_authority)) {
        std::fprintf(stderr, "commit1 failed\n"); return 1;
    }
    std::array<float, 32> first {};
    ggml_backend_tensor_get(out, first.data(), 0, sizeof(first));
    struct ggml_backend_sched_authority_result scheduler_result {};
    if (!ggml_backend_sched_authority_result(sched, &scheduler_result) ||
        scheduler_result.execution_sequence != 1 ||
        std::memcmp(scheduler_result.attempt_nonce, scheduler_nonce.data(), 32) != 0) {
        std::fprintf(stderr, "scheduler result1 failed\n"); return 1;
    }
    if (!ggml_backend_rpc_halofpx_mutable_abort(&session)) return 1;

    // A direct RPC graph exercises real compute+recompute while remaining
    // bound to the scheduler attempt authenticated above.
    std::vector<uint8_t> direct_metadata(ggml_tensor_overhead() * 8 + 2 * ggml_graph_overhead());
    ggml_init_params direct_params { direct_metadata.size(), direct_metadata.data(), true };
    ggml_context * direct_ctx = ggml_init(direct_params);
    if (!direct_ctx) return 2;
    ggml_tensor * da = ggml_new_tensor_1d(direct_ctx, GGML_TYPE_F32, 32);
    ggml_tensor * db_storage = ggml_new_tensor_1d(direct_ctx, GGML_TYPE_F32, bv.size());
    ggml_tensor * db = ggml_view_1d(direct_ctx, db_storage, 32, 4 * sizeof(float));
    ggml_tensor * bias = ggml_new_tensor_1d(direct_ctx, GGML_TYPE_F32, 32);
    ggml_tensor * direct_out = ggml_sqr(direct_ctx, ggml_add(direct_ctx, ggml_add(direct_ctx, da, db), bias));
    ggml_set_input(da);
    ggml_set_output(direct_out);
    ggml_cgraph * direct_graph = ggml_new_graph(direct_ctx);
    ggml_build_forward_expand(direct_graph, direct_out);
    ggml_backend_buffer_t direct_buffer = ggml_backend_alloc_ctx_tensors(direct_ctx, rpc);
    if (!direct_buffer) return 2;
    std::array<float, 32> zeros {};
    ggml_backend_tensor_set(bias, zeros.data(), 0, sizeof(zeros));
    const uint64_t direct_uid = ggml_backend_rpc_halofpx_mutable_graph_uid(direct_graph);
    if (direct_uid == 0) return 1;
    fill_attempt(attempt, direct_uid, 2, scheduler_nonce.data());
    attempt.scheduler_execution_sequence = 1;
    if (!ggml_backend_rpc_halofpx_mutable_begin(rpc, sched, &attempt, &session) ||
        !ggml_backend_rpc_halofpx_mutable_register(&session, da, GGML_RPC_HALOFPX_MUTABLE_TOKEN, 10) ||
        !ggml_backend_rpc_halofpx_mutable_register(&session, db_storage, GGML_RPC_HALOFPX_MUTABLE_ARCHITECTURE_INPUT, 10) ||
        !ggml_backend_rpc_halofpx_mutable_register(&session, db, GGML_RPC_HALOFPX_MUTABLE_ARCHITECTURE_INPUT, 11) ||
        !ggml_backend_rpc_halofpx_mutable_exclude(&session, bias, GGML_RPC_HALOFPX_EXCLUDE_IMMUTABLE_MODEL_WEIGHT, 0)) {
        std::fprintf(stderr, "begin2 failed\n"); return 1;
    }
    ggml_backend_tensor_set(da, av.data(), 0, sizeof(av));
    ggml_backend_tensor_set(db_storage, bv.data(), 0, bv.size() * sizeof(float));
    ggml_backend_rpc_halofpx_mutable_result second_authority {};
    if (!ggml_backend_rpc_halofpx_mutable_prepare(&session, direct_graph, &second_authority)) {
        std::fprintf(stderr, "prepare2 failed\n"); return 1;
    }
    if (ggml_backend_graph_compute(rpc, direct_graph) != GGML_STATUS_SUCCESS) {
        std::fprintf(stderr, "compute2 failed\n"); return 1;
    }
    if (!ggml_backend_rpc_halofpx_mutable_commit(&session, direct_graph, &second_authority)) {
        std::fprintf(stderr, "commit2 failed\n"); return 1;
    }
    std::array<float, 32> direct_first {};
    ggml_backend_tensor_get(direct_out, direct_first.data(), 0, sizeof(direct_first));
    if (!ggml_backend_rpc_halofpx_mutable_abort(&session)) return 1;
    fill_attempt(attempt, direct_uid, 3, scheduler_nonce.data());
    attempt.scheduler_execution_sequence = 1;
    if (!ggml_backend_rpc_halofpx_mutable_begin(rpc, sched, &attempt, &session) ||
        !ggml_backend_rpc_halofpx_mutable_register(&session, da, GGML_RPC_HALOFPX_MUTABLE_TOKEN, 10) ||
        !ggml_backend_rpc_halofpx_mutable_register(&session, db_storage, GGML_RPC_HALOFPX_MUTABLE_ARCHITECTURE_INPUT, 10) ||
        !ggml_backend_rpc_halofpx_mutable_register(&session, db, GGML_RPC_HALOFPX_MUTABLE_ARCHITECTURE_INPUT, 11) ||
        !ggml_backend_rpc_halofpx_mutable_exclude(&session, bias, GGML_RPC_HALOFPX_EXCLUDE_IMMUTABLE_MODEL_WEIGHT, 0)) {
        std::fprintf(stderr, "begin3 failed\n"); return 1;
    }
    ggml_backend_tensor_set(da, av.data(), 0, sizeof(av));
    ggml_backend_tensor_set(db_storage, bv.data(), 0, bv.size() * sizeof(float));
    ggml_backend_rpc_halofpx_mutable_result third_authority {};
    if (!ggml_backend_rpc_halofpx_mutable_prepare(&session, direct_graph, &third_authority)) {
        std::fprintf(stderr, "prepare3 failed\n"); return 1;
    }
    if (ggml_backend_graph_compute(rpc, direct_graph) != GGML_STATUS_SUCCESS) {
        std::fprintf(stderr, "recompute failed\n"); return 1;
    }
    if (!ggml_backend_rpc_halofpx_mutable_commit(&session, direct_graph, &third_authority)) {
        std::fprintf(stderr, "commit3 failed\n"); return 1;
    }
    std::array<float, 32> second {};
    ggml_backend_tensor_get(direct_out, second.data(), 0, sizeof(second));
    if (!ggml_backend_rpc_halofpx_mutable_abort(&session)) return 1;
    const bool exact = std::memcmp(direct_first.data(), second.data(), sizeof(second)) == 0;
    const bool identical_authority =
        std::memcmp(second_authority.semantic_root, third_authority.semantic_root, 32) == 0 &&
        std::memcmp(second_authority.census_root, third_authority.census_root, 32) == 0;

    ggml_cgraph * changed_graph = ggml_new_graph(direct_ctx);
    ggml_build_forward_expand(changed_graph, direct_out);
    const uint64_t changed_uid = ggml_backend_rpc_halofpx_mutable_graph_uid(changed_graph);
    fill_attempt(attempt, changed_uid, 4, scheduler_nonce.data());
    attempt.scheduler_execution_sequence = 1;
    if (!ggml_backend_rpc_halofpx_mutable_begin(rpc, sched, &attempt, &session) ||
        !ggml_backend_rpc_halofpx_mutable_register(&session, da, GGML_RPC_HALOFPX_MUTABLE_TOKEN, 10) ||
        !ggml_backend_rpc_halofpx_mutable_register(&session, db_storage, GGML_RPC_HALOFPX_MUTABLE_ARCHITECTURE_INPUT, 10) ||
        !ggml_backend_rpc_halofpx_mutable_register(&session, db, GGML_RPC_HALOFPX_MUTABLE_ARCHITECTURE_INPUT, 11) ||
        !ggml_backend_rpc_halofpx_mutable_exclude(&session, bias, GGML_RPC_HALOFPX_EXCLUDE_IMMUTABLE_MODEL_WEIGHT, 0)) return 1;
    auto changed = av;
    changed[0] += 1.0f;
    ggml_backend_tensor_set(da, changed.data(), 0, sizeof(changed));
    ggml_backend_tensor_set(db_storage, bv.data(), 0, bv.size() * sizeof(float));
    ggml_backend_rpc_halofpx_mutable_result changed_authority {};
    if (!ggml_backend_rpc_halofpx_mutable_prepare(&session, changed_graph, &changed_authority) ||
        ggml_backend_graph_compute(rpc, changed_graph) != GGML_STATUS_SUCCESS ||
        !ggml_backend_rpc_halofpx_mutable_commit(&session, changed_graph, &changed_authority)) return 1;
    std::array<float, 32> changed_output {};
    ggml_backend_tensor_get(direct_out, changed_output.data(), 0, sizeof(changed_output));
    const bool mutation_sensitive =
        std::memcmp(third_authority.semantic_root, changed_authority.semantic_root, 32) != 0 &&
        std::memcmp(second.data(), changed_output.data(), sizeof(second)) != 0;
    if (!ggml_backend_rpc_halofpx_mutable_abort(&session)) return 1;

    std::array<float, 32> sentinel {};
    sentinel.fill(-777.0f);
    ggml_backend_tensor_set(direct_out, sentinel.data(), 0, sizeof(sentinel));
    fill_attempt(attempt, direct_uid, 5, scheduler_nonce.data());
    attempt.scheduler_execution_sequence = 1;
    if (!ggml_backend_rpc_halofpx_mutable_begin(rpc, sched, &attempt, &session) ||
        !ggml_backend_rpc_halofpx_mutable_register(&session, da, GGML_RPC_HALOFPX_MUTABLE_TOKEN, 10) ||
        !ggml_backend_rpc_halofpx_mutable_register(&session, db_storage, GGML_RPC_HALOFPX_MUTABLE_ARCHITECTURE_INPUT, 10) ||
        !ggml_backend_rpc_halofpx_mutable_register(&session, db, GGML_RPC_HALOFPX_MUTABLE_ARCHITECTURE_INPUT, 11) ||
        !ggml_backend_rpc_halofpx_mutable_exclude(&session, bias, GGML_RPC_HALOFPX_EXCLUDE_IMMUTABLE_MODEL_WEIGHT, 0)) return 1;
    ggml_backend_tensor_set(da, av.data(), 0, sizeof(av));
    ggml_backend_tensor_set(db_storage, bv.data(), 0, bv.size() * sizeof(float));
    const bool omitted_leaf_refused =
        ggml_backend_rpc_halofpx_mutable_test_commit_omit_unmutated_leaf(&session) &&
        ggml_backend_graph_compute(rpc, direct_graph) != GGML_STATUS_SUCCESS;
    std::array<float, 32> after_refusal {};
    ggml_backend_tensor_get(direct_out, after_refusal.data(), 0, sizeof(after_refusal));
    if (!omitted_leaf_refused ||
        std::memcmp(sentinel.data(), after_refusal.data(), sizeof(sentinel)) != 0 ||
        !ggml_backend_rpc_halofpx_mutable_abort(&session)) {
        std::fprintf(stderr, "omitted reconstructed leaf handler refusal failed\n"); return 1;
    }
    std::vector<uint8_t> unknown_metadata(ggml_tensor_overhead() * 4 + ggml_graph_overhead());
    ggml_init_params unknown_params { unknown_metadata.size(), unknown_metadata.data(), true };
    ggml_context * unknown_ctx = ggml_init(unknown_params);
    ggml_tensor * unknown = ggml_new_tensor_1d(unknown_ctx, GGML_TYPE_F32, 8);
    ggml_tensor * unknown_out = ggml_sqr(unknown_ctx, unknown);
    ggml_cgraph * unknown_graph = ggml_new_graph(unknown_ctx);
    ggml_build_forward_expand(unknown_graph, unknown_out);
    ggml_backend_buffer_t unknown_buffer = ggml_backend_alloc_ctx_tensors(unknown_ctx, rpc);
    const uint64_t unknown_uid = ggml_backend_rpc_halofpx_mutable_graph_uid(unknown_graph);
    fill_attempt(attempt, unknown_uid, 6, scheduler_nonce.data());
    attempt.scheduler_execution_sequence = 1;
    ggml_backend_rpc_halofpx_mutable_result unknown_result {};
    const bool unknown_refused = unknown_buffer && unknown_uid != 0 &&
        ggml_backend_rpc_halofpx_mutable_begin(rpc, sched, &attempt, &session) &&
        !ggml_backend_rpc_halofpx_mutable_prepare(&session, unknown_graph, &unknown_result);
    const bool roots = first_authority.mutation_count == 2 &&
        second_authority.mutation_count == 2 &&
        third_authority.mutation_count == 2 &&
        first_authority.census_count == 2 &&
        second_authority.census_count == 3 &&
        third_authority.census_count == 3;
    bool expected = true;
    for (size_t i = 0; i < first.size(); ++i) {
        const float want = (av[i] + bv[4 + i]) * (av[i] + bv[4 + i]);
        expected = expected && std::isfinite(direct_first[i]) && std::abs(direct_first[i] - want) <= 1e-6f;
    }
    std::printf("self_tests=18 handler_refusals=6 compute_recompute_exact=%d expected=%d authority=%d identical_authority=%d mutation_sensitive=%d unknown_refused=%d omitted_leaf_refused=%d mutations=%u/%u census=%u/%u hash_miss=%u/%u hash_hit=%u/%u\n",
                exact, expected, roots, identical_authority, mutation_sensitive, unknown_refused, omitted_leaf_refused,
                first_authority.mutation_count, second_authority.mutation_count,
                first_authority.census_count, second_authority.census_count,
                second_authority.set_hash_miss_count, third_authority.set_hash_miss_count,
                second_authority.set_hash_hit_count, third_authority.set_hash_hit_count);
    if (!ggml_backend_rpc_halofpx_mutable_abort(&session)) {
        std::fprintf(stderr, "failed-session teardown failed\n"); return 1;
    }
    ggml_backend_buffer_free(unknown_buffer);
    ggml_free(unknown_ctx);
    ggml_backend_buffer_free(direct_buffer);
    ggml_free(direct_ctx);
    ggml_backend_sched_free(sched);
    ggml_free(ctx);
    ggml_backend_free(rpc);
    ggml_backend_free(rpc2);
    ggml_backend_free(cpu);
    return exact && expected && roots && identical_authority && mutation_sensitive &&
        unknown_refused && omitted_leaf_refused ? 0 : 1;
}
