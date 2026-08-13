#pragma once

#include <cassert>
#include <cstdint>
#include <utility>

// Tracks whether host-visible outputs for the current output epoch have passed
// a full context synchronization. This does not make llama_context thread-safe;
// callers retain the existing serialized same-context requirement.
class llama_output_sync_state {
public:
    void begin_epoch() noexcept {
        invalidate();
        ++epoch;
        if (epoch == 0) {
            ++epoch;
        }
    }

    void invalidate() noexcept {
        ready = false;
        ++mutation_serial;
        if (mutation_serial == 0) {
            ++mutation_serial;
        }
    }

    void begin_mutation() noexcept {
        invalidate();
        ++mutation_depth;
    }

    void end_mutation() noexcept {
        assert(mutation_depth > 0);
        --mutation_depth;

        // A synchronization reentered from graph dispatch or an asynchronous
        // output-copy enqueue must never survive the rest of that publication
        // phase. This also covers partial submission failures.
        invalidate();
    }

    template <typename F>
    bool synchronize_if_needed(bool coalesce, F && synchronize) {
        if (coalesce && ready && mutation_depth == 0) {
            ++barriers_reused;
            return false;
        }

        const uint64_t observed_serial = mutation_serial;
        std::forward<F>(synchronize)();
        ++barriers_executed;
        ready = mutation_depth == 0 && mutation_serial == observed_serial;
        return true;
    }

    template <typename F>
    void synchronize_force(F && synchronize) {
        const uint64_t observed_serial = mutation_serial;
        std::forward<F>(synchronize)();
        ++barriers_executed;
        ready = mutation_depth == 0 && mutation_serial == observed_serial;
    }

    bool is_ready() const noexcept {
        return ready;
    }

    uint64_t generation() const noexcept {
        return epoch;
    }

    bool is_mutating() const noexcept {
        return mutation_depth != 0;
    }

    uint64_t completed_barrier_count() const noexcept {
        return barriers_executed;
    }

    uint64_t reused_barrier_count() const noexcept {
        return barriers_reused;
    }

private:
    bool ready = false;
    uint64_t epoch = 0;
    uint64_t mutation_serial = 0;
    uint32_t mutation_depth = 0;
    uint64_t barriers_executed = 0;
    uint64_t barriers_reused = 0;
};

// Keeps readiness invalid for the whole graph-dispatch or host-output
// publication phase, including synchronous backend callback reentry.
class llama_output_sync_mutation {
public:
    explicit llama_output_sync_mutation(llama_output_sync_state & state) noexcept
        : state(state) {
        state.begin_mutation();
    }

    ~llama_output_sync_mutation() {
        state.end_mutation();
    }

    llama_output_sync_mutation(const llama_output_sync_mutation &) = delete;
    llama_output_sync_mutation & operator=(const llama_output_sync_mutation &) = delete;

private:
    llama_output_sync_state & state;
};
