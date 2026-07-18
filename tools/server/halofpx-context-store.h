#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <memory>

namespace halofpx {

using context_store_digest = std::array<uint8_t, 32>;

struct context_store_identity {
    context_store_digest compatibility_root {};
    context_store_digest scope_namespace {};
    context_store_digest checkpoint_lineage_id {};
    uint64_t policy_epoch = 0;
};

struct context_store_lookup_request {
    context_store_identity identity;
};

class context_store_candidate {
public:
    virtual ~context_store_candidate();

    // A provider may return a candidate only after authenticating and fully
    // validating its immutable generation. Callers must still apply the
    // explicit match policy and an admitted codec before live mutation.
    // The returned reference remains valid for this candidate's lifetime.
    virtual const context_store_identity & identity() const noexcept = 0;
};

class context_store_generation {
public:
    virtual ~context_store_generation();

    // The returned reference remains valid for this generation's lifetime.
    virtual const context_store_identity & identity() const noexcept = 0;
};

enum class context_store_lookup_status : uint8_t {
    hit,
    miss_disabled,
    miss_not_found,
    miss_unauthorized,
    miss_incompatible,
    miss_unsupported,
    miss_corrupt,
    miss_incomplete,
    miss_replay,
    miss_storage,
};

class context_store_lookup_result {
public:
    context_store_lookup_result(context_store_lookup_result &&) noexcept;
    context_store_lookup_result & operator=(context_store_lookup_result &&) noexcept;
    ~context_store_lookup_result();

    context_store_lookup_result(const context_store_lookup_result &) = delete;
    context_store_lookup_result & operator=(const context_store_lookup_result &) = delete;

    static context_store_lookup_result miss(context_store_lookup_status status) noexcept;
    static context_store_lookup_result hit(std::unique_ptr<const context_store_candidate> candidate) noexcept;

    context_store_lookup_status status() const noexcept;
    const context_store_candidate * candidate() const noexcept;
    bool is_hit() const noexcept;

private:
    context_store_lookup_result(
        context_store_lookup_status status,
        std::unique_ptr<const context_store_candidate> candidate) noexcept;

    context_store_lookup_status status_;
    std::unique_ptr<const context_store_candidate> candidate_;
};

struct context_store_publish_request {
    // Borrowed for the synchronous publish() call only. A provider must not
    // retain this pointer, start work that outlives the call, or mutate it.
    const context_store_generation * generation = nullptr;
};

enum class context_store_publish_status : uint8_t {
    published,
    disabled,
    rejected_unauthorized,
    rejected_incomplete,
    rejected_unsupported,
    rejected_replay,
    storage_error,
};

struct context_store_capabilities {
    bool persistent_reads = false;
    bool persistent_writes = false;
    bool enumeration = false;
    bool anonymous_scope = false;
    bool shared_scope = false;
    size_t admitted_state_profiles = 0;
    size_t admitted_codecs = 0;
};

class context_store_provider {
public:
    virtual ~context_store_provider();

    virtual const char * name() const noexcept = 0;
    virtual context_store_capabilities capabilities() const noexcept = 0;
    // Requests are borrowed for each synchronous call and must not be retained.
    // A lookup result exclusively owns any returned immutable candidate.
    // Implementations catch internal failures and map them to miss_storage or
    // storage_error; exceptions must never cross these noexcept boundaries.
    virtual context_store_lookup_result lookup(const context_store_lookup_request & request) const noexcept = 0;
    virtual context_store_publish_status publish(const context_store_publish_request & request) const noexcept = 0;
};

// The only L03a provider. It owns no resources and performs no I/O, logging,
// metrics, enumeration, parsing, capture, restore, or live-context mutation.
// Factory allocation failure may throw; no provider operation may throw.
std::unique_ptr<context_store_provider> make_disabled_context_store_provider();

} // namespace halofpx
