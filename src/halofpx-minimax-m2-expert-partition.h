#pragma once

#include <array>
#include <cstddef>
#include <cstdint>

namespace halofpx::minimax_m2 {

constexpr std::size_t hidden_size        = 3072;
constexpr std::size_t expert_count       = 192;
constexpr std::size_t selected_count     = 8;
constexpr std::size_t world_size         = 2;
constexpr std::size_t experts_per_rank   = expert_count / world_size;
constexpr std::size_t max_canary_tokens  = 512;

enum class partition_status : std::uint8_t {
    ok = 0,
    wrong_world_size,
    expert_out_of_range,
    duplicate_expert,
    invalid_reduction,
};

struct rank_partition {
    std::array<std::uint16_t, selected_count> local_ids{};
    std::array<std::uint8_t, selected_count> active{};
};

struct partition_plan {
    std::array<std::uint16_t, selected_count> global_ids{};
    std::array<rank_partition, world_size> ranks{};
};

partition_status build_partition(
        const std::array<std::uint16_t, selected_count> & selected_global_ids,
        std::size_t requested_world_size,
        partition_plan & output) noexcept;

partition_status reduce_rank_partials(
        const float * rank0,
        const float * rank1,
        std::size_t n_tokens,
        float * output) noexcept;

} // namespace halofpx::minimax_m2
