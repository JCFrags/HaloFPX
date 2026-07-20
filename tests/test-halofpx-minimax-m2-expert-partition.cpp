#include "halofpx-minimax-m2-expert-partition.h"

#include <array>
#include <cstdio>
#include <vector>

using namespace halofpx::minimax_m2;

static bool check(const bool condition, const char * message) {
    if (!condition) {
        std::fprintf(stderr, "P06a failure: %s\n", message);
        return false;
    }
    return true;
}

int main() {
    bool ok = true;
    ok &= check(hidden_size == 3072 && expert_count == 192 && selected_count == 8,
            "exact MiniMax-M2 geometry drifted");
    ok &= check(world_size == 2 && experts_per_rank == 96,
            "two-rank ownership geometry drifted");

    partition_plan plan{};
    const std::array<std::uint16_t, selected_count> all_rank0 = {0, 1, 2, 3, 4, 5, 6, 95};
    ok &= check(build_partition(all_rank0, 2, plan) == partition_status::ok,
            "rank-zero plan rejected");
    for (std::size_t slot = 0; slot < selected_count; ++slot) {
        ok &= check(plan.ranks[0].active[slot] == 1 && plan.ranks[1].active[slot] == 0,
                "rank-zero ownership mask is wrong");
        ok &= check(plan.ranks[0].local_ids[slot] == all_rank0[slot] &&
                    plan.ranks[1].local_ids[slot] == 0,
                "rank-zero local remap is wrong");
    }

    const std::array<std::uint16_t, selected_count> cross_rank = {0, 95, 96, 191, 47, 143, 20, 120};
    ok &= check(build_partition(cross_rank, 2, plan) == partition_status::ok,
            "cross-rank plan rejected");
    for (std::size_t slot = 0; slot < selected_count; ++slot) {
        const std::size_t owner = cross_rank[slot] / experts_per_rank;
        const std::uint16_t local = cross_rank[slot] % experts_per_rank;
        for (std::size_t rank = 0; rank < world_size; ++rank) {
            ok &= check(plan.ranks[rank].active[slot] == (rank == owner ? 1 : 0),
                    "cross-rank ownership mask is wrong");
            ok &= check(plan.ranks[rank].local_ids[slot] == (rank == owner ? local : 0),
                    "cross-rank local remap is wrong");
        }
    }

    auto invalid = cross_rank;
    invalid[7] = expert_count;
    ok &= check(build_partition(invalid, 2, plan) == partition_status::expert_out_of_range,
            "out-of-range expert was accepted");
    invalid = cross_rank;
    invalid[7] = invalid[0];
    ok &= check(build_partition(invalid, 2, plan) == partition_status::duplicate_expert,
            "duplicate expert was accepted");
    ok &= check(build_partition(cross_rank, 1, plan) == partition_status::wrong_world_size,
            "wrong world size was accepted");

    constexpr std::size_t n_tokens = 2;
    std::vector<float> rank0(hidden_size * n_tokens);
    std::vector<float> rank1(hidden_size * n_tokens);
    std::vector<float> combined(hidden_size * n_tokens);
    for (std::size_t i = 0; i < combined.size(); ++i) {
        rank0[i] = static_cast<float>(i % 64);
        rank1[i] = static_cast<float>((i * 3) % 64);
    }
    ok &= check(reduce_rank_partials(rank0.data(), rank1.data(), n_tokens, combined.data()) ==
                    partition_status::ok,
            "valid partial reduction rejected");
    for (std::size_t i = 0; i < combined.size(); ++i) {
        ok &= check(combined[i] == rank0[i] + rank1[i], "rank reduction mismatch");
    }
    ok &= check(reduce_rank_partials(nullptr, rank1.data(), n_tokens, combined.data()) ==
                    partition_status::invalid_reduction,
            "null rank partial was accepted");
    ok &= check(reduce_rank_partials(rank0.data(), rank1.data(), 0, combined.data()) ==
                    partition_status::invalid_reduction,
            "zero-token reduction was accepted");
    ok &= check(reduce_rank_partials(rank0.data(), rank1.data(), max_canary_tokens + 1, combined.data()) ==
                    partition_status::invalid_reduction,
            "oversized reduction was accepted");

    if (!ok) {
        return 1;
    }
    std::puts("HaloFPX MiniMax-M2 expert partition canary passed");
    return 0;
}
