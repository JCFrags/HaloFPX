#include "halofpx-minimax-m2-expert-partition.h"

namespace halofpx::minimax_m2 {

partition_status build_partition(
        const std::array<std::uint16_t, selected_count> & selected_global_ids,
        const std::size_t requested_world_size,
        partition_plan & output) noexcept {
    output = {};
    if (requested_world_size != world_size) {
        return partition_status::wrong_world_size;
    }

    for (std::size_t slot = 0; slot < selected_count; ++slot) {
        const std::uint16_t global_id = selected_global_ids[slot];
        if (global_id >= expert_count) {
            output = {};
            return partition_status::expert_out_of_range;
        }
        for (std::size_t prior = 0; prior < slot; ++prior) {
            if (selected_global_ids[prior] == global_id) {
                output = {};
                return partition_status::duplicate_expert;
            }
        }

        output.global_ids[slot] = global_id;
        const std::size_t owner = global_id / experts_per_rank;
        const auto local_id = static_cast<std::uint16_t>(global_id % experts_per_rank);
        for (std::size_t rank = 0; rank < world_size; ++rank) {
            const bool active = rank == owner;
            output.ranks[rank].local_ids[slot] = active ? local_id : 0;
            output.ranks[rank].active[slot] = active ? 1 : 0;
        }
    }
    return partition_status::ok;
}

partition_status reduce_rank_partials(
        const float * rank0,
        const float * rank1,
        const std::size_t n_tokens,
        float * output) noexcept {
    if (rank0 == nullptr || rank1 == nullptr || output == nullptr ||
            n_tokens == 0 || n_tokens > max_canary_tokens) {
        return partition_status::invalid_reduction;
    }

    const std::size_t n_values = hidden_size * n_tokens;
    for (std::size_t i = 0; i < n_values; ++i) {
        output[i] = rank0[i] + rank1[i];
    }
    return partition_status::ok;
}

} // namespace halofpx::minimax_m2
