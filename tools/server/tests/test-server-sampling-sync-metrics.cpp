#include "llama-output-sync.h"
#include "server-metrics-prometheus.h"

#include <cstdint>
#include <cstdio>
#include <limits>
#include <sstream>
#include <string>

namespace {

bool check(bool condition, const char * expression, int line) {
    if (!condition) {
        std::fprintf(stderr, "check failed at line %d: %s\n", line, expression);
    }
    return condition;
}

#define CHECK(expr) do { if (!check((expr), #expr, __LINE__)) return 1; } while (false)

size_t count_occurrences(const std::string & text, const std::string & needle) {
    size_t count = 0;
    size_t offset = 0;
    while ((offset = text.find(needle, offset)) != std::string::npos) {
        ++count;
        offset += needle.size();
    }
    return count;
}

server_sampling_sync_metrics snapshot(
        const llama_output_sync_state & state,
        uint64_t graph_submissions,
        uint64_t output_transfers) {
    return {
        /* .output_epochs      = */ state.generation(),
        /* .completed_barriers = */ state.completed_barrier_count(),
        /* .reused_barriers    = */ state.reused_barrier_count(),
        /* .graph_submissions  = */ graph_submissions,
        /* .output_transfers   = */ output_transfers,
    };
}

void begin_output_epoch(
        llama_output_sync_state & state,
        uint64_t & graph_submissions,
        uint64_t & output_transfers) {
    state.begin_epoch();
    {
        llama_output_sync_mutation graph(state);
        ++graph_submissions;
    }
    {
        llama_output_sync_mutation output(state);
        output_transfers += 4;
    }
}

} // namespace

int main() {
    // Feature OFF preserves one completed barrier per public/internal read and
    // reports zero reuse. The metrics snapshot itself does not synchronize.
    llama_output_sync_state feature_off;
    uint64_t off_graphs = 0;
    uint64_t off_transfers = 0;
    uint64_t off_callbacks = 0;
    begin_output_epoch(feature_off, off_graphs, off_transfers);
    feature_off.synchronize_force([&]() { ++off_callbacks; });
    for (int i = 0; i < 5; ++i) {
        CHECK(feature_off.synchronize_if_needed(false, [&]() { ++off_callbacks; }));
    }
    const auto off = snapshot(feature_off, off_graphs, off_transfers);
    CHECK(off.output_epochs == 1);
    CHECK(off.completed_barriers == 6);
    CHECK(off.reused_barriers == 0);
    CHECK(off.graph_submissions == 1);
    CHECK(off.output_transfers == 4);
    CHECK(off_callbacks == 6);

    std::ostringstream off_output;
    server_write_sampling_sync_prometheus(off_output, off);
    CHECK(off_callbacks == 6);
    CHECK(off_output.str().find("llamacpp:halofpx_sampling_sync_completed_barriers_total 6\n") != std::string::npos);
    CHECK(off_output.str().find("llamacpp:halofpx_sampling_sync_reused_barriers_total 0\n") != std::string::npos);

    // Feature ON executes the same graph and transfer work, completes one
    // barrier, and reuses it for the five internal reads.
    llama_output_sync_state feature_on;
    uint64_t on_graphs = 0;
    uint64_t on_transfers = 0;
    uint64_t on_callbacks = 0;
    begin_output_epoch(feature_on, on_graphs, on_transfers);
    feature_on.synchronize_force([&]() { ++on_callbacks; });
    for (int i = 0; i < 5; ++i) {
        CHECK(!feature_on.synchronize_if_needed(true, [&]() { ++on_callbacks; }));
    }
    const auto on = snapshot(feature_on, on_graphs, on_transfers);
    CHECK(on.output_epochs == off.output_epochs);
    CHECK(on.completed_barriers == 1);
    CHECK(on.reused_barriers == 5);
    CHECK(on.graph_submissions == off.graph_submissions);
    CHECK(on.output_transfers == off.output_transfers);
    CHECK(on_callbacks == 1);

    std::ostringstream on_output;
    server_write_sampling_sync_prometheus(on_output, on);
    CHECK(on_callbacks == 1);
    CHECK(on_output.str().find("llamacpp:halofpx_sampling_sync_completed_barriers_total 1\n") != std::string::npos);
    CHECK(on_output.str().find("llamacpp:halofpx_sampling_sync_reused_barriers_total 5\n") != std::string::npos);

    // Decimal serialization must remain exact above 2^53 and at uint64 max;
    // no double or JSON numeric conversion is permitted on this path.
    const server_sampling_sync_metrics large {
        /* .output_epochs      = */ UINT64_C(9007199254740993),
        /* .completed_barriers = */ std::numeric_limits<uint64_t>::max(),
        /* .reused_barriers    = */ UINT64_C(9007199254740995),
        /* .graph_submissions  = */ UINT64_C(9007199254740997),
        /* .output_transfers   = */ UINT64_C(9007199254740999),
    };
    std::ostringstream large_output;
    server_write_sampling_sync_prometheus(large_output, large);
    const std::string encoded = large_output.str();
    CHECK(large_output.good());
    CHECK(encoded.find("llamacpp:halofpx_sampling_sync_output_epochs_total 9007199254740993\n") != std::string::npos);
    CHECK(encoded.find("llamacpp:halofpx_sampling_sync_completed_barriers_total 18446744073709551615\n") != std::string::npos);
    CHECK(encoded.find("llamacpp:halofpx_sampling_sync_reused_barriers_total 9007199254740995\n") != std::string::npos);
    CHECK(encoded.find("llamacpp:halofpx_sampling_sync_graph_submissions_total 9007199254740997\n") != std::string::npos);
    CHECK(encoded.find("llamacpp:halofpx_sampling_sync_output_transfers_total 9007199254740999\n") != std::string::npos);
    CHECK(encoded.find("halofpx_sampling_sync_generations_total") == std::string::npos);
    CHECK(encoded.find("e+") == std::string::npos);
    CHECK(count_occurrences(encoded, "# TYPE llamacpp:halofpx_sampling_sync_") == 5);
    CHECK(count_occurrences(encoded, " counter\n") == 5);

    return 0;
}
