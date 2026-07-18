#pragma once

#include "halofpx-context-store-publication.h"

#include <cstddef>
#include <cstdint>
#include <vector>

namespace halofpx {

constexpr size_t context_store_publication_simulator_no_index = static_cast<size_t>(-1);

enum class context_store_publication_simulator_operation : uint8_t {
    read_anchor,
    stage_object,
    write_object,
    verify_object,
    sync_object_file,
    publish_object,
    sync_object_directory,
    stage_manifest,
    write_manifest,
    verify_manifest,
    sync_manifest_file,
    publish_manifest,
    sync_manifest_directory,
    replace_anchor,
    sync_anchor,
};

enum class context_store_publication_simulator_phase : uint8_t {
    before,
    after,
};

enum class context_store_publication_simulator_collision : uint8_t {
    absent,
    equal,
    unequal,
};

struct context_store_publication_simulator_failpoint {
    bool enabled = false;
    context_store_publication_simulator_operation operation =
        context_store_publication_simulator_operation::read_anchor;
    size_t index = context_store_publication_simulator_no_index;
    context_store_publication_simulator_phase phase =
        context_store_publication_simulator_phase::before;
    context_store_publication_step_result result =
        context_store_publication_step_result::io_error;
};

struct context_store_publication_simulator_trace_entry {
    context_store_publication_simulator_operation operation;
    size_t index;
    context_store_publication_simulator_phase phase;
    context_store_publication_step_result result;
    bool injected;
};

struct context_store_publication_simulator_crash_policy {
    bool retain_unsynced_namespace = false;
    bool retain_unsynced_anchor = false;
};

enum class context_store_publication_simulator_recovery : uint8_t {
    old_generation,
    new_generation,
    miss,
};

struct context_store_publication_simulator_entry_state {
    bool temp_live = false;
    bool temp_written = false;
    bool temp_verified = false;
    bool temp_durable = false;
    bool published_live = false;
    bool published_durable = false;
    bool published_by_attempt = false;
    context_store_publication_simulator_collision destination =
        context_store_publication_simulator_collision::absent;
};

// Deterministic offline state machine. It performs no filesystem operation and
// makes no OS durability claim. Separate live/durable namespaces make crash
// outcomes explicit and reproducible.
class context_store_publication_simulator final :
    public context_store_publication_backend {
public:
    context_store_publication_simulator(
        const context_store_publication_anchor & predecessor,
        const context_store_publication_anchor & next,
        size_t object_count);

    void set_failpoint(const context_store_publication_simulator_failpoint & failpoint);
    void clear_failpoint() noexcept;
    void set_object_destination(size_t index, context_store_publication_simulator_collision collision);
    void set_manifest_destination(context_store_publication_simulator_collision collision) noexcept;
    void set_verified_manifest_digest(const context_store_digest & digest) noexcept;
    void invalidate_predecessor_chain() noexcept;

    void crash(const context_store_publication_simulator_crash_policy & policy) noexcept;
    context_store_publication_simulator_recovery recover() const noexcept;

    const std::vector<context_store_publication_simulator_trace_entry> & trace() const noexcept;
    const std::vector<context_store_publication_simulator_entry_state> & objects() const noexcept;
    const context_store_publication_simulator_entry_state & manifest() const noexcept;
    const context_store_publication_anchor & live_anchor() const noexcept;
    const context_store_publication_anchor & durable_anchor() const noexcept;
    size_t retained_garbage_count() const noexcept;
    bool trace_overflowed() const noexcept;

    context_store_publication_step_result read_anchor(context_store_publication_anchor & anchor) override;
    context_store_publication_step_result stage_object(size_t index) override;
    context_store_publication_step_result write_object(size_t index) override;
    context_store_publication_step_result verify_object(size_t index) override;
    context_store_publication_step_result sync_object_file(size_t index) override;
    context_store_publication_step_result publish_object_no_replace(size_t index) override;
    context_store_publication_step_result sync_object_directory(size_t index) override;
    context_store_publication_step_result stage_manifest() override;
    context_store_publication_step_result write_manifest() override;
    context_store_publication_step_result verify_manifest(context_store_digest & digest) override;
    context_store_publication_step_result sync_manifest_file() override;
    context_store_publication_step_result publish_manifest_no_replace() override;
    context_store_publication_step_result sync_manifest_directory() override;
    context_store_publication_step_result replace_anchor_atomically(
        const context_store_publication_id & attempt_id,
        const context_store_publication_anchor & expected_predecessor,
        const context_store_publication_anchor & next) override;
    context_store_publication_step_result sync_anchor() override;

private:
    bool before(
        context_store_publication_simulator_operation operation,
        size_t index,
        context_store_publication_step_result & result);
    context_store_publication_step_result after(
        context_store_publication_simulator_operation operation,
        size_t index,
        context_store_publication_step_result result);
    bool valid_object(size_t index) const noexcept;
    static void discard_temp(
        context_store_publication_simulator_entry_state & state,
        size_t & garbage_count) noexcept;

    context_store_publication_anchor predecessor_;
    context_store_publication_anchor next_;
    context_store_publication_anchor live_anchor_;
    context_store_publication_anchor durable_anchor_;
    context_store_digest verified_manifest_digest_ {};
    std::vector<context_store_publication_simulator_entry_state> objects_;
    context_store_publication_simulator_entry_state manifest_;
    context_store_publication_simulator_failpoint failpoint_;
    std::vector<context_store_publication_simulator_trace_entry> trace_;
    size_t retained_garbage_count_ = 0;
    size_t trace_limit_ = 0;
    bool valid_ = false;
    bool anchor_unsynced_ = false;
    bool trace_overflowed_ = false;
    bool predecessor_chain_valid_ = true;
};

const char * context_store_publication_simulator_operation_name(
    context_store_publication_simulator_operation operation) noexcept;
const char * context_store_publication_simulator_recovery_name(
    context_store_publication_simulator_recovery recovery) noexcept;

} // namespace halofpx
