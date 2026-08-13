#include "llama-output-sync.h"

#include <cstdint>
#include <cstdio>

namespace {

struct counters {
    uint64_t graph_submissions = 0;
    uint64_t output_transfers = 0;
    uint64_t scheduler_barriers = 0;
};

bool check(bool condition, const char * expression, int line) {
    if (!condition) {
        std::fprintf(stderr, "check failed at line %d: %s\n", line, expression);
    }
    return condition;
}

#define CHECK(expr) do { if (!check((expr), #expr, __LINE__)) return 1; } while (false)

void submit_generation(llama_output_sync_state & state, counters & counts, uint64_t transfers) {
    state.begin_epoch();
    {
        llama_output_sync_mutation graph_submission(state);
        ++counts.graph_submissions;
    }
    {
        llama_output_sync_mutation output_publication(state);
        counts.output_transfers += transfers;
    }
}

bool read_output(llama_output_sync_state & state, counters & counts, bool coalesce) {
    return state.synchronize_if_needed(coalesce, [&]() {
        ++counts.scheduler_barriers;
    });
}

} // namespace

int main() {
    // Exact ordinary sampler contract: one forced public synchronization plus
    // five internal output reads. Feature-off retains all six historical
    // barriers; canary-on reuses the forced boundary for all five reads.
    llama_output_sync_state feature_off;
    counters off;
    submit_generation(feature_off, off, 4);
    feature_off.synchronize_force([&]() {
        ++off.scheduler_barriers;
    });
    for (int i = 0; i < 5; ++i) {
        CHECK(read_output(feature_off, off, false));
    }
    CHECK(off.graph_submissions == 1);
    CHECK(off.output_transfers == 4);
    CHECK(off.scheduler_barriers == 6);
    CHECK(feature_off.completed_barrier_count() == 6);
    CHECK(feature_off.reused_barrier_count() == 0);

    // Canary-on coalesces only already-completed barriers. The exact same
    // simulated compute and transfer work is still submitted.
    llama_output_sync_state feature_on;
    counters on;
    submit_generation(feature_on, on, 4);
    feature_on.synchronize_force([&]() {
        ++on.scheduler_barriers;
    });
    for (int i = 0; i < 5; ++i) {
        CHECK(!read_output(feature_on, on, true));
    }
    CHECK(on.graph_submissions == off.graph_submissions);
    CHECK(on.output_transfers == off.output_transfers);
    CHECK(on.scheduler_barriers == 1);
    CHECK(feature_on.completed_barrier_count() == 1);
    CHECK(feature_on.reused_barrier_count() == 5);

    // The public force path remains unconditional even when the epoch is ready.
    feature_on.synchronize_force([&]() {
        ++on.scheduler_barriers;
    });
    CHECK(on.scheduler_barriers == 2);
    CHECK(feature_on.completed_barrier_count() == 2);

    // A later generation invalidates the latch and requires exactly one new
    // completed barrier before internal readers may reuse it.
    submit_generation(feature_on, on, 3);
    CHECK(!feature_on.is_ready());
    CHECK(read_output(feature_on, on, true));
    CHECK(!read_output(feature_on, on, true));
    CHECK(on.graph_submissions == 2);
    CHECK(on.output_transfers == 7);
    CHECK(on.scheduler_barriers == 3);

    // A forced synchronization inside active publication executes, but cannot
    // prime readiness for work still being enqueued. Nested mutation guards
    // likewise keep readiness invalid until the outermost phase ends.
    llama_output_sync_state nested;
    counters nested_counts;
    nested.begin_epoch();
    {
        llama_output_sync_mutation outer(nested);
        {
            llama_output_sync_mutation inner(nested);
            nested.synchronize_force([&]() {
                ++nested_counts.scheduler_barriers;
            });
            CHECK(!nested.is_ready());
        }
        CHECK(!nested.is_ready());
    }
    CHECK(!nested.is_ready());
    CHECK(read_output(nested, nested_counts, true));
    CHECK(nested_counts.scheduler_barriers == 2);

    // Exception/failure unwinding of a graph-publication phase must also leave
    // the latch invalid. The test models a failed submit without backend work.
    llama_output_sync_state failed_submit;
    failed_submit.begin_epoch();
    try {
        llama_output_sync_mutation graph_submission(failed_submit);
        throw 7;
    } catch (int) {
        CHECK(!failed_submit.is_ready());
    }
    counters failed_counts;
    CHECK(read_output(failed_submit, failed_counts, true));
    CHECK(failed_counts.scheduler_barriers == 1);

    // Regression for synchronous scheduler callback reentry: a barrier that
    // completes during graph dispatch must not prime readiness for output
    // transfers that are enqueued afterward.
    llama_output_sync_state reentrant;
    counters reentrant_counts;
    reentrant.begin_epoch();
    {
        llama_output_sync_mutation graph_submission(reentrant);
        ++reentrant_counts.graph_submissions;
        CHECK(read_output(reentrant, reentrant_counts, true));
        CHECK(!reentrant.is_ready());
    }
    {
        llama_output_sync_mutation output_publication(reentrant);
        ++reentrant_counts.output_transfers;
        CHECK(!reentrant.is_ready());
    }
    CHECK(!reentrant.is_ready());
    CHECK(read_output(reentrant, reentrant_counts, true));
    CHECK(!read_output(reentrant, reentrant_counts, true));
    CHECK(reentrant_counts.scheduler_barriers == 2);
    CHECK(reentrant.reused_barrier_count() == 1);

    // A lifecycle mutation observed during a barrier also prevents readiness
    // publication, even if no mutation remains active when the callback exits.
    llama_output_sync_state changed_during_barrier;
    counters changed_counts;
    changed_during_barrier.begin_epoch();
    CHECK(read_output(changed_during_barrier, changed_counts, true));
    CHECK(changed_during_barrier.is_ready());
    changed_during_barrier.invalidate();
    CHECK(!changed_during_barrier.is_ready());
    changed_during_barrier.synchronize_force([&]() {
        ++changed_counts.scheduler_barriers;
        changed_during_barrier.invalidate();
    });
    CHECK(!changed_during_barrier.is_ready());
    CHECK(read_output(changed_during_barrier, changed_counts, true));

    // Draft and target contexts retain independent latches.
    llama_output_sync_state target;
    llama_output_sync_state draft;
    counters target_counts;
    counters draft_counts;
    submit_generation(target, target_counts, 1);
    submit_generation(draft, draft_counts, 1);
    CHECK(read_output(target, target_counts, true));
    CHECK(!read_output(target, target_counts, true));
    CHECK(read_output(draft, draft_counts, true));
    CHECK(target_counts.scheduler_barriers == 1);
    CHECK(draft_counts.scheduler_barriers == 1);

    return 0;
}
