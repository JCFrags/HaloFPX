#pragma once

#include <array>
#include <charconv>
#include <cstdint>
#include <ios>
#include <limits>
#include <ostream>
#include <string_view>
#include <system_error>

// Context-lifetime sampling/output counters carried by the server metrics
// task. These values are cumulative snapshots, not request attribution.
struct server_sampling_sync_metrics {
    uint64_t output_epochs = 0;
    uint64_t completed_barriers = 0;
    uint64_t reused_barriers = 0;
    uint64_t graph_submissions = 0;
    uint64_t output_transfers = 0;
};

inline void server_write_prometheus_uint64_counter(
        std::ostream & output,
        std::string_view name,
        std::string_view help,
        uint64_t value) {
    output << "# HELP llamacpp:" << name << " " << help << '\n'
           << "# TYPE llamacpp:" << name << " counter\n"
           << "llamacpp:" << name << " ";

    // Avoid a JSON/double intermediate: Prometheus accepts exact base-10
    // integers, including values above the IEEE-754 exact-integer boundary.
    std::array<char, std::numeric_limits<uint64_t>::digits10 + 3> encoded {};
    const auto result = std::to_chars(encoded.data(), encoded.data() + encoded.size(), value);
    if (result.ec != std::errc {}) {
        output.setstate(std::ios_base::failbit);
        return;
    }

    output.write(encoded.data(), result.ptr - encoded.data());
    output.put('\n');
}

inline void server_write_sampling_sync_prometheus(
        std::ostream & output,
        const server_sampling_sync_metrics & metrics) {
    server_write_prometheus_uint64_counter(
            output,
            "halofpx_sampling_sync_output_epochs_total",
            "Number of output synchronization epochs started by this context.",
            metrics.output_epochs);
    server_write_prometheus_uint64_counter(
            output,
            "halofpx_sampling_sync_completed_barriers_total",
            "Number of output-result scheduler barriers completed by this context.",
            metrics.completed_barriers);
    server_write_prometheus_uint64_counter(
            output,
            "halofpx_sampling_sync_reused_barriers_total",
            "Number of completed output-result barriers reused by this context.",
            metrics.reused_barriers);
    server_write_prometheus_uint64_counter(
            output,
            "halofpx_sampling_sync_graph_submissions_total",
            "Number of output graph submissions made by this context.",
            metrics.graph_submissions);
    server_write_prometheus_uint64_counter(
            output,
            "halofpx_sampling_sync_output_transfers_total",
            "Number of host-output transfers enqueued by this context.",
            metrics.output_transfers);
}
