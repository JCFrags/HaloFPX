#include "halofpx-context-store-v1-server-canary.h"

#if !defined(__linux__)
#error "The HaloFPX full-v1 server canary is Linux-only"
#endif

#include <linux/openat2.h>
#include <linux/stat.h>
#include <sys/stat.h>
#include <sys/statfs.h>
#include <sys/syscall.h>
#include <unistd.h>

#include <algorithm>
#include <array>
#include <cerrno>
#include <cstring>
#include <fcntl.h>
#include <limits>
#include <new>
#include <string>
#include <utility>
#include <vector>

namespace halofpx {
namespace {

constexpr uint32_t directory_mode = 0700;
constexpr uint32_t file_mode = 0600;
constexpr char manifest_key_domain[] = "halofpx.full-v1-canary.manifest-key.v1";
constexpr char anchor_key_domain[] = "halofpx.full-v1-canary.anchor-key.v1";
constexpr char attempt_key_domain[] = "halofpx.full-v1-canary.attempt-key.v1";
constexpr char manifest_key_id[] = "halofpx-full-v1-canary-manifest-v1";
constexpr char anchor_key_id[] = "halofpx-protected-anchor-v1";

void wipe(void * data, size_t size) noexcept {
    volatile uint8_t * cursor = static_cast<volatile uint8_t *>(data);
    while (size-- != 0) *cursor++ = 0;
}

class fd_owner {
public:
    explicit fd_owner(int fd = -1) noexcept : fd_(fd) {}
    ~fd_owner() { if (fd_ >= 0) ::close(fd_); }
    fd_owner(const fd_owner &) = delete;
    fd_owner & operator=(const fd_owner &) = delete;
    fd_owner(fd_owner && other) noexcept : fd_(std::exchange(other.fd_, -1)) {}
    int get() const noexcept { return fd_; }
    int release() noexcept { return std::exchange(fd_, -1); }
private:
    int fd_;
};

bool nonzero(const uint8_t * data, size_t size) noexcept {
    uint8_t combined = 0;
    for (size_t i = 0; i < size; ++i) combined |= data[i];
    return combined != 0;
}

template <size_t N>
bool nonzero(const std::array<uint8_t, N> & value) noexcept {
    return nonzero(value.data(), value.size());
}

context_store_registered_id registered_id(const char * value) noexcept {
    context_store_registered_id result;
    const size_t size = std::strlen(value);
    if (size == 0 || size > result.bytes.size()) return result;
    result.size = static_cast<uint8_t>(size);
    std::copy_n(value, size, result.bytes.begin());
    return result;
}

bool derive_key(const context_store_key_view & operator_key,
                const std::array<uint8_t, 16> & store_uuid,
                const char * domain, size_t domain_size,
                context_store_format_digest & output) noexcept {
    std::array<uint8_t, 128> input {};
    if (domain_size > input.size() - store_uuid.size()) return false;
    std::copy_n(reinterpret_cast<const uint8_t *>(domain), domain_size, input.begin());
    std::copy(store_uuid.begin(), store_uuid.end(), input.begin() + domain_size);
    const bool result = context_store_hmac_sha256(
        operator_key.data, operator_key.size, input.data(),
        domain_size + store_uuid.size(), output);
    wipe(input.data(), input.size());
    return result;
}

bool inspect_root(int fd, context_store_linux_root_identity_v1 & identity) noexcept {
    identity = {};
    struct stat value {};
    struct statx extended {};
    struct statfs filesystem {};
    if (::fstat(fd, &value) != 0 || !S_ISDIR(value.st_mode) ||
        static_cast<uint32_t>(value.st_mode & 07777) != directory_mode ||
        ::syscall(SYS_statx, fd, "", AT_EMPTY_PATH | AT_STATX_SYNC_AS_STAT,
                  STATX_BASIC_STATS | STATX_MNT_ID, &extended) != 0 ||
        (extended.stx_mask & STATX_MNT_ID) == 0 || ::fstatfs(fd, &filesystem) != 0) {
        return false;
    }
    identity.device = static_cast<uint64_t>(value.st_dev);
    identity.inode = static_cast<uint64_t>(value.st_ino);
    identity.mount_id = extended.stx_mnt_id;
    identity.owner_uid = static_cast<uint64_t>(value.st_uid);
    identity.mode = static_cast<uint32_t>(value.st_mode & 07777);
    identity.filesystem_type = static_cast<uint64_t>(filesystem.f_type);
    return identity.device != 0 && identity.inode != 0 && identity.mount_id != 0 &&
        identity.filesystem_type != 0;
}

int open_contained(int parent, const char * name, uint64_t flags) noexcept {
    struct open_how how {};
    how.flags = flags | O_CLOEXEC | O_NOFOLLOW;
    how.resolve = RESOLVE_BENEATH | RESOLVE_NO_SYMLINKS |
                  RESOLVE_NO_MAGICLINKS | RESOLVE_NO_XDEV;
    const long result = ::syscall(SYS_openat2, parent, name, &how, sizeof(how));
    return result >= 0 && result <= std::numeric_limits<int>::max()
        ? static_cast<int>(result) : -1;
}

bool exact_directory(int fd, const context_store_linux_root_identity_v1 & root) noexcept {
    context_store_linux_root_identity_v1 actual;
    return inspect_root(fd, actual) && actual.device == root.device &&
        actual.mount_id == root.mount_id && actual.owner_uid == root.owner_uid &&
        actual.mode == directory_mode && actual.filesystem_type == root.filesystem_type;
}

bool exact_regular(int fd, const context_store_linux_root_identity_v1 & root,
                   uint64_t maximum, uint64_t & size) noexcept {
    size = 0;
    struct stat value {};
    struct statx extended {};
    if (::fstat(fd, &value) != 0 || !S_ISREG(value.st_mode) || value.st_nlink != 1 ||
        value.st_size <= 0 || static_cast<uint64_t>(value.st_size) > maximum ||
        static_cast<uint64_t>(value.st_dev) != root.device ||
        static_cast<uint64_t>(value.st_uid) != root.owner_uid ||
        static_cast<uint32_t>(value.st_mode & 07777) != file_mode ||
        ::syscall(SYS_statx, fd, "", AT_EMPTY_PATH | AT_STATX_SYNC_AS_STAT,
                  STATX_BASIC_STATS | STATX_MNT_ID, &extended) != 0 ||
        (extended.stx_mask & STATX_MNT_ID) == 0 || extended.stx_mnt_id != root.mount_id) {
        return false;
    }
    size = static_cast<uint64_t>(value.st_size);
    return true;
}

bool read_exact(int fd, uint8_t * data, size_t size) noexcept {
    size_t offset = 0;
    while (offset < size) {
        const ssize_t count = ::pread(fd, data + offset, size - offset,
                                      static_cast<off_t>(offset));
        if (count < 0 && errno == EINTR) continue;
        if (count <= 0) return false;
        offset += static_cast<size_t>(count);
    }
    uint8_t trailing = 0;
    ssize_t count;
    do count = ::pread(fd, &trailing, 1, static_cast<off_t>(size));
    while (count < 0 && errno == EINTR);
    return count == 0;
}

std::string digest_name(const context_store_format_digest & digest) {
    static constexpr char hex[] = "0123456789abcdef";
    std::string result = "m-";
    result.reserve(2 + digest.size() * 2 + 5);
    for (uint8_t value : digest) {
        result.push_back(hex[value >> 4]);
        result.push_back(hex[value & 15]);
    }
    result += ".cbor";
    return result;
}

context_store_v1_server_canary_status lookup_status(context_store_lookup_status status) noexcept {
    switch (status) {
        case context_store_lookup_status::hit:
            return context_store_v1_server_canary_status::hit;
        case context_store_lookup_status::miss_not_found:
        case context_store_lookup_status::miss_incomplete:
            return context_store_v1_server_canary_status::miss_not_found;
        case context_store_lookup_status::miss_corrupt:
        case context_store_lookup_status::miss_storage:
            return context_store_v1_server_canary_status::miss_corrupt;
        case context_store_lookup_status::miss_incompatible:
        case context_store_lookup_status::miss_replay:
        case context_store_lookup_status::miss_unauthorized:
            return context_store_v1_server_canary_status::miss_incompatible;
        default:
            return context_store_v1_server_canary_status::miss_unsupported;
    }
}

context_store_v1_server_canary_status authority_status(
        context_store_v1_linux_generation_one_status status) noexcept {
    switch (status) {
        case context_store_v1_linux_generation_one_status::ready:
            return context_store_v1_server_canary_status::ready;
        case context_store_v1_linux_generation_one_status::published:
            return context_store_v1_server_canary_status::published;
        case context_store_v1_linux_generation_one_status::recovered_success:
        case context_store_v1_linux_generation_one_status::recovered_aborted:
            return context_store_v1_server_canary_status::ready;
        case context_store_v1_linux_generation_one_status::interrupted:
            return context_store_v1_server_canary_status::source_rejected;
        case context_store_v1_linux_generation_one_status::busy:
            return context_store_v1_server_canary_status::busy;
        case context_store_v1_linux_generation_one_status::invalid:
        case context_store_v1_linux_generation_one_status::unsupported:
        case context_store_v1_linux_generation_one_status::source_mismatch:
        case context_store_v1_linux_generation_one_status::conflict:
            return context_store_v1_server_canary_status::source_rejected;
        case context_store_v1_linux_generation_one_status::storage:
            return context_store_v1_server_canary_status::storage;
        case context_store_v1_linux_generation_one_status::synchronization:
        case context_store_v1_linux_generation_one_status::quota_exhausted:
        case context_store_v1_linux_generation_one_status::reserve_exhausted:
        case context_store_v1_linux_generation_one_status::layout_rejected:
        case context_store_v1_linux_generation_one_status::accounting_overflow:
            return context_store_v1_server_canary_status::storage;
        case context_store_v1_linux_generation_one_status::quarantined:
            return context_store_v1_server_canary_status::quarantined;
    }
    return context_store_v1_server_canary_status::source_rejected;
}

context_store_v1_server_canary_lifecycle_state lifecycle_state(
        context_store_v1_linux_generation_one_lifecycle_state state) noexcept {
    using source = context_store_v1_linux_generation_one_lifecycle_state;
    using target = context_store_v1_server_canary_lifecycle_state;
    switch (state) {
        case source::unavailable: return target::unavailable;
        case source::ready: return target::ready;
        case source::published: return target::published;
        case source::recovered_success: return target::recovered_success;
        case source::recovered_aborted: return target::recovered_aborted;
        case source::interrupted: return target::interrupted;
        case source::busy: return target::busy;
        case source::invalid: return target::invalid;
        case source::unsupported: return target::unsupported;
        case source::source_mismatch: return target::source_mismatch;
        case source::conflict: return target::conflict;
        case source::storage: return target::storage;
        case source::synchronization: return target::synchronization;
        case source::quota_exhausted: return target::quota_exhausted;
        case source::reserve_exhausted: return target::reserve_exhausted;
        case source::layout_rejected: return target::layout_rejected;
        case source::accounting_overflow: return target::accounting_overflow;
        case source::quarantined: return target::quarantined;
    }
    return target::unavailable;
}

context_store_v1_server_canary_close_reason close_reason(
        context_store_v1_linux_generation_one_close_reason reason) noexcept {
    using source = context_store_v1_linux_generation_one_close_reason;
    using target = context_store_v1_server_canary_close_reason;
    switch (reason) {
        case source::none: return target::none;
        case source::published: return target::published;
        case source::recovered_success: return target::recovered_success;
        case source::recovered_aborted: return target::recovered_aborted;
        case source::quota_exhausted: return target::quota_exhausted;
        case source::reserve_exhausted: return target::reserve_exhausted;
        case source::layout_rejected: return target::layout_rejected;
        case source::accounting_overflow: return target::accounting_overflow;
        case source::storage: return target::storage;
        case source::synchronization: return target::synchronization;
        case source::quarantined: return target::quarantined;
    }
    return target::none;
}

context_store_v1_server_canary_eviction_state eviction_state(
        context_store_v1_linux_generation_one_eviction_classification state) noexcept {
    using source = context_store_v1_linux_generation_one_eviction_classification;
    using target = context_store_v1_server_canary_eviction_state;
    switch (state) {
        case source::no_safe_online_eviction: return target::no_safe_online_eviction;
        case source::selected_generation_pinned: return target::selected_generation_pinned;
        case source::reconciliation_required: return target::reconciliation_required;
        case source::uncertain_material_retained: return target::uncertain_material_retained;
    }
    return target::no_safe_online_eviction;
}

} // namespace

class context_store_v1_server_canary::implementation {
public:
    implementation(int data_fd, int anchor_fd,
                   const context_store_linux_root_identity_v1 & data_identity_value,
                   const context_store_linux_root_identity_v1 & anchor_identity_value,
                   const context_store_v1_server_canary_config & config,
                   const context_store_format_digest & manifest,
                   const context_store_format_digest & anchor,
                   const context_store_format_digest & attempt)
        : data_root(data_fd), anchor_root(anchor_fd), data_identity(data_identity_value),
          anchor_identity(anchor_identity_value), store_uuid(config.store_uuid),
          compatibility(config.compatibility), producer_identity(config.producer_identity),
          global_plan_digest(config.global_plan_digest),
          rank_ownership_digest(config.rank_ownership_digest),
          rank_placement_digest(config.rank_placement_digest),
          topology_epoch(config.topology_epoch), limits(config.limits),
          budget { config.quota_bytes, config.reserve_bytes,
                   static_cast<uint64_t>(config.max_entries) },
          manifest_key(manifest), anchor_key_material(anchor), attempt_key(attempt) {
        observation_value.lifecycle_state =
            context_store_v1_server_canary_lifecycle_state::ready;
        observation_value.quota_bytes = budget.quota_bytes;
        observation_value.reserve_bytes = budget.reserve_bytes;
        observation_value.writes_closed = false;
    }

    ~implementation() {
        wipe(manifest_key.data(), manifest_key.size());
        wipe(anchor_key_material.data(), anchor_key_material.size());
        wipe(attempt_key.data(), attempt_key.size());
    }

    context_store_manifest_verification_policy policy(
            const context_store_identity & identity,
            const context_store_format_digest & selected) noexcept {
        context_store_manifest_verification_policy result;
        result.key.disposition = context_store_key_disposition::active;
        result.key.key_id = registered_id(manifest_key_id);
        result.key.generation = 1;
        result.key.master_key = { manifest_key.data(), manifest_key.size() };
        result.anchor.store_uuid = store_uuid;
        result.anchor.checkpoint_lineage_id = identity.checkpoint_lineage_id;
        result.anchor.namespace_id = identity.scope_namespace;
        result.anchor.policy_epoch = identity.policy_epoch;
        result.anchor.key_generation = 1;
        result.anchor.generation = 1;
        result.anchor.selected_manifest_digest = selected;
        result.compatibility = compatibility;
        return result;
    }

    context_store_protected_canary_anchor_body anchor_body(
            const context_store_identity & identity,
            const context_store_format_digest & selected) noexcept {
        context_store_protected_canary_anchor_body result;
        result.store_uuid = store_uuid;
        result.namespace_id = identity.scope_namespace;
        result.policy_epoch = identity.policy_epoch;
        result.checkpoint_lineage_id = identity.checkpoint_lineage_id;
        result.manifest_key_generation = 1;
        result.authority_epoch = 1;
        result.generation = 1;
        result.selected_manifest_digest = selected;
        return result;
    }

    context_store_v1_transformer_manifest_parameters parameters(
            const context_store_transformer_snapshot_v1 & snapshot) noexcept {
        context_store_v1_transformer_manifest_parameters result;
        result.store_uuid = store_uuid;
        result.compatibility_components = compatibility.components;
        result.producer_identity = producer_identity;
        result.global_plan_digest = global_plan_digest;
        result.rank_ownership_digest = rank_ownership_digest;
        result.rank_placement_digest = rank_placement_digest;
        result.generation = 1;
        result.topology_epoch = topology_epoch;
        result.logical_position = snapshot.tokens.size();
        result.output_boundary = snapshot.tokens.size();
        result.durability_mode = 0;
        result.signing_key.disposition = context_store_key_disposition::active;
        result.signing_key.key_id = registered_id(manifest_key_id);
        result.signing_key.generation = 1;
        result.signing_key.master_key = { manifest_key.data(), manifest_key.size() };
        return result;
    }

    void apply_observation(
            const context_store_v1_linux_generation_one_observation & source) noexcept {
        observation_value.lifecycle_state = lifecycle_state(source.lifecycle_state);
        observation_value.last_close_reason = close_reason(source.last_close_reason);
        observation_value.eviction_state = eviction_state(source.eviction_classification);
        observation_value.logical_bytes = source.logical_bytes;
        observation_value.allocated_bytes = source.allocated_bytes;
        observation_value.available_bytes = source.available_bytes;
        observation_value.projected_peak_logical_bytes =
            source.projected_logical_peak_bytes;
        observation_value.quota_bytes = source.quota_bytes;
        observation_value.reserve_bytes = source.reserve_bytes;
        // Generation one never admits online deletion, even if a future
        // provider accidentally reports a nonzero candidate value.
        observation_value.safe_online_eviction_bytes = 0;
        observation_value.accounting_valid = source.accounting_valid;
        observation_value.writes_closed = source.writes_closed;
    }

    void apply_status(context_store_v1_linux_generation_one_status status) noexcept {
        using source = context_store_v1_linux_generation_one_status;
        using lifecycle = context_store_v1_server_canary_lifecycle_state;
        using reason = context_store_v1_server_canary_close_reason;
        switch (status) {
            case source::ready: observation_value.lifecycle_state = lifecycle::ready; break;
            case source::published:
                observation_value.lifecycle_state = lifecycle::published;
                observation_value.last_close_reason = reason::published;
                observation_value.writes_closed = true;
                break;
            case source::recovered_success:
                observation_value.lifecycle_state = lifecycle::recovered_success;
                observation_value.last_close_reason = reason::recovered_success;
                observation_value.writes_closed = true;
                break;
            case source::recovered_aborted:
                observation_value.lifecycle_state = lifecycle::recovered_aborted;
                observation_value.last_close_reason = reason::recovered_aborted;
                observation_value.writes_closed = true;
                break;
            case source::interrupted:
                observation_value.lifecycle_state = lifecycle::interrupted;
                break;
            case source::busy: observation_value.lifecycle_state = lifecycle::busy; break;
            case source::invalid: observation_value.lifecycle_state = lifecycle::invalid; break;
            case source::unsupported:
                observation_value.lifecycle_state = lifecycle::unsupported;
                break;
            case source::source_mismatch:
                observation_value.lifecycle_state = lifecycle::source_mismatch;
                break;
            case source::conflict:
                observation_value.lifecycle_state = lifecycle::conflict;
                break;
            case source::storage:
                observation_value.lifecycle_state = lifecycle::storage;
                observation_value.last_close_reason = reason::storage;
                observation_value.writes_closed = true;
                break;
            case source::synchronization:
                observation_value.lifecycle_state = lifecycle::synchronization;
                observation_value.last_close_reason = reason::synchronization;
                observation_value.writes_closed = true;
                break;
            case source::quota_exhausted:
                observation_value.lifecycle_state = lifecycle::quota_exhausted;
                observation_value.last_close_reason = reason::quota_exhausted;
                observation_value.writes_closed = true;
                break;
            case source::reserve_exhausted:
                observation_value.lifecycle_state = lifecycle::reserve_exhausted;
                observation_value.last_close_reason = reason::reserve_exhausted;
                observation_value.writes_closed = true;
                break;
            case source::layout_rejected:
                observation_value.lifecycle_state = lifecycle::layout_rejected;
                observation_value.last_close_reason = reason::layout_rejected;
                observation_value.writes_closed = true;
                break;
            case source::accounting_overflow:
                observation_value.lifecycle_state = lifecycle::accounting_overflow;
                observation_value.last_close_reason = reason::accounting_overflow;
                observation_value.writes_closed = true;
                break;
            case source::quarantined:
                observation_value.lifecycle_state = lifecycle::quarantined;
                observation_value.last_close_reason = reason::quarantined;
                observation_value.writes_closed = true;
                break;
        }
    }

    context_store_v1_linux_generation_one_open_result make_authority(
            const context_store_identity & identity,
            const context_store_format_digest & selected,
            const context_store_authenticated_manifest_metadata & metadata,
            const context_store_object_reference * objects,
            size_t object_count) noexcept {
        context_store_v1_linux_generation_one_config config;
        config.data_root = { data_root.get(), data_identity };
        config.anchor_root = { anchor_root.get(), anchor_identity };
        config.verification_policy = policy(identity, selected);
        config.admission.manifest = metadata;
        config.admission.objects = objects;
        config.admission.object_count = object_count;
        config.object_limits = { limits.max_frame_bytes, limits.snapshot.max_state_bytes };
        config.max_total_frame_bytes = limits.max_frame_bytes > UINT64_MAX / 2
            ? UINT64_MAX : limits.max_frame_bytes * 2;
        config.budget = budget;
        config.anchor_body = anchor_body(identity, selected);
        config.anchor_key.key_id = registered_id(anchor_key_id);
        config.anchor_key.generation = 1;
        config.anchor_key.master_key = {
            anchor_key_material.data(), anchor_key_material.size() };
        config.attempt_key.master_key = { attempt_key.data(), attempt_key.size() };
        return make_context_store_v1_linux_generation_one(config);
    }

    struct loaded_admission {
        context_store_v1_server_canary_status status =
            context_store_v1_server_canary_status::miss_corrupt;
        context_store_authenticated_manifest_metadata metadata;
        std::array<context_store_object_reference, context_store_manifest_max_objects> objects {};
        size_t object_count = 0;
    };

    loaded_admission load_admission(const context_store_identity & identity,
                                    const context_store_format_digest & selected) noexcept {
        loaded_admission result;
        try {
            fd_owner manifests(open_contained(data_root.get(), "manifests",
                                               O_RDONLY | O_DIRECTORY));
            if (manifests.get() < 0) {
                result.status = errno == ENOENT
                    ? context_store_v1_server_canary_status::miss_not_found
                    : context_store_v1_server_canary_status::miss_corrupt;
                return result;
            }
            if (!exact_directory(manifests.get(), data_identity)) return result;
            const std::string name = digest_name(selected);
            fd_owner manifest(open_contained(manifests.get(), name.c_str(), O_RDONLY));
            if (manifest.get() < 0) {
                result.status = errno == ENOENT
                    ? context_store_v1_server_canary_status::miss_not_found
                    : context_store_v1_server_canary_status::miss_corrupt;
                return result;
            }
            uint64_t size = 0;
            if (!exact_regular(manifest.get(), data_identity,
                               context_store_manifest_max_bytes, size)) return result;
            std::vector<uint8_t> bytes(static_cast<size_t>(size));
            if (!read_exact(manifest.get(), bytes.data(), bytes.size())) return result;
            const auto verified = context_store_verify_manifest_v1(
                bytes.data(), bytes.size(), policy(identity, selected));
            if (verified.status != context_store_manifest_verify_status::authenticated_unadmitted ||
                verified.manifest_digest != selected ||
                verified.authenticated_object_count() == 0 ||
                verified.authenticated_object_count() > result.objects.size() ||
                verified.authenticated_manifest_metadata() == nullptr) {
                result.status = verified.status ==
                        context_store_manifest_verify_status::compatibility_mismatch
                    ? context_store_v1_server_canary_status::miss_incompatible
                    : context_store_v1_server_canary_status::miss_corrupt;
                return result;
            }
            result.metadata = *verified.authenticated_manifest_metadata();
            result.object_count = verified.authenticated_object_count();
            for (size_t i = 0; i < result.object_count; ++i) {
                const auto * object = verified.authenticated_object_reference(i);
                if (!object) return loaded_admission {};
                result.objects[i] = *object;
            }
            result.status = context_store_v1_server_canary_status::ready;
            return result;
        } catch (const std::bad_alloc &) {
            result.status = context_store_v1_server_canary_status::storage;
        } catch (...) {
            result.status = context_store_v1_server_canary_status::miss_corrupt;
        }
        return result;
    }

    context_store_v1_server_canary_status load_selected_anchor(
            const context_store_identity & identity,
            context_store_format_digest & selected) noexcept {
        selected.fill(0);
        fd_owner anchor(open_contained(anchor_root.get(), "anchor.v1", O_RDONLY));
        if (anchor.get() < 0) return errno == ENOENT
            ? context_store_v1_server_canary_status::miss_not_found
            : context_store_v1_server_canary_status::miss_corrupt;
        uint64_t size = 0;
        if (!exact_regular(anchor.get(), anchor_identity,
                           context_store_protected_canary_anchor_max_bytes, size))
            return context_store_v1_server_canary_status::miss_corrupt;
        std::array<uint8_t, context_store_protected_canary_anchor_max_bytes> bytes {};
        if (!read_exact(anchor.get(), bytes.data(), static_cast<size_t>(size)))
            return context_store_v1_server_canary_status::miss_corrupt;
        context_store_format_digest unknown_selected {};
        auto expected = anchor_body(identity, unknown_selected);
        context_store_protected_canary_anchor_key key;
        key.key_id = registered_id(anchor_key_id);
        key.generation = 1;
        key.master_key = { anchor_key_material.data(), anchor_key_material.size() };
        const auto decoded = context_store_protected_canary_anchor_decode_v1(
            bytes.data(), static_cast<size_t>(size), expected, key);
        const auto * body = decoded.authenticated_body();
        if (!body) return context_store_v1_server_canary_status::miss_corrupt;
        selected = body->selected_manifest_digest;
        return context_store_v1_server_canary_status::ready;
    }

    fd_owner data_root;
    fd_owner anchor_root;
    context_store_linux_root_identity_v1 data_identity;
    context_store_linux_root_identity_v1 anchor_identity;
    std::array<uint8_t, 16> store_uuid {};
    context_store_compatibility_expectation compatibility;
    context_store_format_digest producer_identity {};
    context_store_format_digest global_plan_digest {};
    context_store_format_digest rank_ownership_digest {};
    context_store_format_digest rank_placement_digest {};
    uint64_t topology_epoch = 0;
    context_store_v1_transformer_codec_limits limits;
    context_store_v1_linux_generation_one_budget budget;
    context_store_v1_server_canary_observation observation_value;
    context_store_format_digest manifest_key {};
    context_store_format_digest anchor_key_material {};
    context_store_format_digest attempt_key {};
    std::unique_ptr<context_store_v1_linux_generation_one> authority;
};

context_store_v1_server_canary::context_store_v1_server_canary(
        std::unique_ptr<implementation> implementation) noexcept
    : implementation_(std::move(implementation)) {}

context_store_v1_server_canary::~context_store_v1_server_canary() = default;

bool context_store_v1_server_canary::available() const noexcept {
    return implementation_ != nullptr;
}

context_store_v1_server_canary_observation
context_store_v1_server_canary::observation() const noexcept {
    return implementation_ ? implementation_->observation_value
                           : context_store_v1_server_canary_observation {};
}

context_store_v1_server_canary_publish_result context_store_v1_server_canary::publish(
        const context_store_transformer_snapshot_v1 & snapshot) noexcept {
    context_store_v1_server_canary_publish_result result;
    if (!implementation_ || implementation_->observation_value.writes_closed) return result;
    try {
        auto encoded = context_store_encode_transformer_snapshot_v1(
            snapshot, implementation_->parameters(snapshot), implementation_->limits);
        if (encoded.status != context_store_v1_transformer_codec_status::encoded) return result;
        const auto policy = implementation_->policy(
            snapshot.compatibility_identity, encoded.encoded.manifest_digest);
        std::array<context_store_v1_frame_view,
            context_store_v1_transformer_frame_count> frames {};
        for (size_t i = 0; i < frames.size(); ++i) {
            frames[i] = { encoded.encoded.frames[i].data(), encoded.encoded.frames[i].size() };
        }
        context_store_v1_read_only_source source;
        source.manifest_data = encoded.encoded.manifest.data();
        source.manifest_size = encoded.encoded.manifest.size();
        source.verification_policy = policy;
        source.admission.manifest = encoded.encoded.admission_metadata;
        source.admission.objects = encoded.encoded.admission_objects.data();
        source.admission.object_count = encoded.encoded.admission_objects.size();
        source.frames = frames.data();
        source.frame_count = frames.size();
        source.object_limits = {
            implementation_->limits.max_frame_bytes,
            implementation_->limits.snapshot.max_state_bytes };
        source.max_total_frame_bytes = implementation_->limits.max_frame_bytes > UINT64_MAX / 2
            ? UINT64_MAX : implementation_->limits.max_frame_bytes * 2;
        auto opened = implementation_->make_authority(
            snapshot.compatibility_identity, encoded.encoded.manifest_digest,
            encoded.encoded.admission_metadata, encoded.encoded.admission_objects.data(),
            encoded.encoded.admission_objects.size());
        if (!opened.authority) {
            implementation_->apply_status(opened.status);
            result.status = authority_status(opened.status);
            return result;
        }
        const auto published = opened.authority->publish(source);
        implementation_->apply_observation(opened.authority->observation());
        implementation_->apply_status(published);
        if (implementation_->observation_value.writes_closed) {
            // Retain the locked authority so any selected valid generation
            // remains readable after publication or a fail-closed outcome.
            implementation_->authority = std::move(opened.authority);
        } else {
            implementation_->authority.reset();
        }
        if (published != context_store_v1_linux_generation_one_status::published) {
            result.status = authority_status(published);
            return result;
        }
        result.selected_manifest = encoded.encoded.manifest_digest;
        result.status = context_store_v1_server_canary_status::published;
        return result;
    } catch (...) {
        implementation_->apply_status(context_store_v1_linux_generation_one_status::storage);
        result.status = context_store_v1_server_canary_status::storage;
        return result;
    }
}

context_store_v1_server_canary_restore_result context_store_v1_server_canary::restore(
        const context_store_format_digest & selected_manifest,
        const llama_token * expected_tokens,
        size_t expected_token_count,
        const context_store_identity & identity,
        const context_store_transformer_profile_v1 & profile) noexcept {
    context_store_v1_server_canary_restore_result result;
    if (!implementation_ || !nonzero(selected_manifest) || !expected_tokens ||
        expected_token_count == 0 ||
        expected_token_count > implementation_->limits.snapshot.max_tokens) return result;
    auto loaded = implementation_->load_admission(identity, selected_manifest);
    if (loaded.status != context_store_v1_server_canary_status::ready) {
        result.status = loaded.status;
        return result;
    }
    implementation_->authority.reset();
    auto opened = implementation_->make_authority(
        identity, selected_manifest, loaded.metadata,
        loaded.objects.data(), loaded.object_count);
    if (!opened.authority) {
        implementation_->apply_status(opened.status);
        result.status = authority_status(opened.status);
        return result;
    }
    implementation_->authority = std::move(opened.authority);
    implementation_->apply_observation(implementation_->authority->observation());
    if (implementation_->authority->quarantined()) {
        result.status = context_store_v1_server_canary_status::miss_corrupt;
        return result;
    }
    context_store_lookup_request request;
    request.identity = identity;
    auto lookup = implementation_->authority->lookup(request);
    if (!lookup.is_hit()) {
        result.status = lookup_status(lookup.status());
        return result;
    }
    const auto * candidate = dynamic_cast<const context_store_v1_read_only_candidate *>(
        lookup.candidate());
    if (!candidate) return result;
    context_store_v1_transformer_decode_request decode;
    decode.candidate = candidate;
    decode.expected_tokens = expected_tokens;
    decode.expected_token_count = expected_token_count;
    decode.compatibility_identity = identity;
    decode.profile = profile;
    decode.producer_identity = implementation_->producer_identity;
    decode.rank_ownership_digest = implementation_->rank_ownership_digest;
    decode.topology_epoch = implementation_->topology_epoch;
    decode.logical_position = expected_token_count;
    decode.output_boundary = expected_token_count;
    decode.limits = implementation_->limits.snapshot;
    auto decoded = context_store_decode_transformer_snapshot_v1(decode);
    if (decoded.status != context_store_v1_transformer_codec_status::decoded) {
        result.status = decoded.status ==
                context_store_v1_transformer_codec_status::token_mismatch
            ? context_store_v1_server_canary_status::miss_incompatible
            : context_store_v1_server_canary_status::miss_corrupt;
        return result;
    }
    result.snapshot = std::move(decoded.snapshot);
    result.status = context_store_v1_server_canary_status::hit;
    return result;
}

context_store_v1_server_canary_restore_result
context_store_v1_server_canary::restore_selected(
        const llama_token * expected_tokens,
        size_t expected_token_count,
        const context_store_identity & identity,
        const context_store_transformer_profile_v1 & profile) noexcept {
    context_store_v1_server_canary_restore_result result;
    if (!implementation_) return result;
    context_store_format_digest selected {};
    result.status = implementation_->load_selected_anchor(identity, selected);
    if (result.status != context_store_v1_server_canary_status::ready) return result;
    return restore(selected, expected_tokens, expected_token_count, identity, profile);
}

context_store_v1_server_canary_open_result make_context_store_v1_server_canary(
        const context_store_v1_server_canary_config & config) noexcept {
    context_store_v1_server_canary_open_result result;
    context_store_format_digest manifest {};
    context_store_format_digest anchor {};
    context_store_format_digest attempt {};
    try {
        if (!config.data_root_path || !config.anchor_root_path ||
            config.operator_key.size != context_store_v1_server_canary_operator_key_bytes ||
            !config.operator_key.data || !nonzero(config.store_uuid) ||
            !nonzero(config.compatibility.root) || !nonzero(config.producer_identity) ||
            !nonzero(config.global_plan_digest) || !nonzero(config.rank_ownership_digest) ||
            !nonzero(config.rank_placement_digest) || config.topology_epoch == 0 ||
            config.quota_bytes == 0 || config.max_entries != 1 ||
            config.limits.snapshot.max_state_bytes == 0 ||
            config.limits.snapshot.max_tokens == 0 || config.limits.max_frame_bytes == 0 ||
            config.limits.max_manifest_bytes == 0 ||
            config.limits.max_manifest_bytes > context_store_manifest_max_bytes) return result;
        for (const auto & component : config.compatibility.components) {
            if (!nonzero(component)) return result;
        }
        fd_owner data(::open(config.data_root_path,
            O_RDONLY | O_DIRECTORY | O_CLOEXEC | O_NOFOLLOW));
        fd_owner anchor_root(::open(config.anchor_root_path,
            O_RDONLY | O_DIRECTORY | O_CLOEXEC | O_NOFOLLOW));
        context_store_linux_root_identity_v1 data_identity;
        context_store_linux_root_identity_v1 anchor_identity;
        if (data.get() < 0 || anchor_root.get() < 0 ||
            !inspect_root(data.get(), data_identity) ||
            !inspect_root(anchor_root.get(), anchor_identity) ||
            (data_identity.device == anchor_identity.device &&
             data_identity.inode == anchor_identity.inode)) return result;
        if (!derive_key(config.operator_key, config.store_uuid,
                        manifest_key_domain, sizeof(manifest_key_domain), manifest) ||
            !derive_key(config.operator_key, config.store_uuid,
                        anchor_key_domain, sizeof(anchor_key_domain), anchor) ||
            !derive_key(config.operator_key, config.store_uuid,
                        attempt_key_domain, sizeof(attempt_key_domain), attempt)) return result;
        auto implementation = std::make_unique<context_store_v1_server_canary::implementation>(
            data.release(), anchor_root.release(), data_identity, anchor_identity,
            config, manifest, anchor, attempt);
        result.canary.reset(new context_store_v1_server_canary(std::move(implementation)));
        result.status = context_store_v1_server_canary_status::ready;
    } catch (const std::bad_alloc &) {
        result.status = context_store_v1_server_canary_status::storage;
    } catch (...) {
        result.status = context_store_v1_server_canary_status::storage;
    }
    wipe(manifest.data(), manifest.size());
    wipe(anchor.data(), anchor.size());
    wipe(attempt.data(), attempt.size());
    return result;
}

const char * context_store_v1_server_canary_status_name(
        context_store_v1_server_canary_status status) noexcept {
    switch (status) {
        case context_store_v1_server_canary_status::ready: return "ready";
        case context_store_v1_server_canary_status::published: return "published";
        case context_store_v1_server_canary_status::hit: return "hit";
        case context_store_v1_server_canary_status::miss_not_found: return "miss-not-found";
        case context_store_v1_server_canary_status::miss_corrupt: return "miss-corrupt";
        case context_store_v1_server_canary_status::miss_incompatible: return "miss-incompatible";
        case context_store_v1_server_canary_status::miss_unsupported: return "miss-unsupported";
        case context_store_v1_server_canary_status::source_rejected: return "source-rejected";
        case context_store_v1_server_canary_status::busy: return "busy";
        case context_store_v1_server_canary_status::storage: return "storage";
        case context_store_v1_server_canary_status::quarantined: return "quarantined";
    }
    return "invalid";
}

const char * context_store_v1_server_canary_lifecycle_state_name(
        context_store_v1_server_canary_lifecycle_state state) noexcept {
    using value = context_store_v1_server_canary_lifecycle_state;
    switch (state) {
        case value::unavailable: return "unavailable";
        case value::ready: return "ready";
        case value::published: return "published";
        case value::recovered_success: return "recovered-success";
        case value::recovered_aborted: return "recovered-aborted";
        case value::interrupted: return "interrupted";
        case value::busy: return "busy";
        case value::invalid: return "invalid";
        case value::unsupported: return "unsupported";
        case value::source_mismatch: return "source-mismatch";
        case value::conflict: return "conflict";
        case value::storage: return "storage";
        case value::synchronization: return "synchronization";
        case value::quota_exhausted: return "quota-exhausted";
        case value::reserve_exhausted: return "reserve-exhausted";
        case value::layout_rejected: return "layout-rejected";
        case value::accounting_overflow: return "accounting-overflow";
        case value::quarantined: return "quarantined";
    }
    return "unavailable";
}

const char * context_store_v1_server_canary_close_reason_name(
        context_store_v1_server_canary_close_reason reason) noexcept {
    using value = context_store_v1_server_canary_close_reason;
    switch (reason) {
        case value::none: return "none";
        case value::published: return "published";
        case value::recovered_success: return "recovered-success";
        case value::recovered_aborted: return "recovered-aborted";
        case value::quota_exhausted: return "quota-exhausted";
        case value::reserve_exhausted: return "reserve-exhausted";
        case value::layout_rejected: return "layout-rejected";
        case value::accounting_overflow: return "accounting-overflow";
        case value::storage: return "storage";
        case value::synchronization: return "synchronization";
        case value::quarantined: return "quarantined";
    }
    return "none";
}

const char * context_store_v1_server_canary_eviction_state_name(
        context_store_v1_server_canary_eviction_state state) noexcept {
    using value = context_store_v1_server_canary_eviction_state;
    switch (state) {
        case value::no_safe_online_eviction: return "no-safe-online-eviction";
        case value::selected_generation_pinned: return "selected-generation-pinned";
        case value::reconciliation_required: return "reconciliation-required";
        case value::uncertain_material_retained: return "uncertain-material-retained";
    }
    return "no-safe-online-eviction";
}

} // namespace halofpx
