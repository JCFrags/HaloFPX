#ifndef HALOFPX_L05Z_RETURN_ROLE_AUTHORITY_MANIFEST_INC
#define HALOFPX_L05Z_RETURN_ROLE_AUTHORITY_MANIFEST_INC

// Test-only structural authority for L05z returned-fault roles.  This manifest
// deliberately distinguishes canonical semantic cases from legacy execution
// rows and never promotes an aggregate-only role to an executable oracle.

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <set>
#include <sstream>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace halofpx_l05z_return_role_authority_v1 {

inline constexpr std::size_t legacy_compatibility_execution_rows = 2163;
inline constexpr std::size_t canonical_compatibility_cases = 2069;
inline constexpr std::size_t canonical_l05z_role_cases = 4644;
inline constexpr std::size_t canonical_hostile_cases = 1899;
inline constexpr std::size_t canonical_total_cases = 8612;
inline constexpr bool physical_execution_rows_frozen = false;
inline constexpr std::size_t canonical_cleanup_subset = 324;
inline constexpr std::size_t legacy_cleanup_execution_subset = 337;
inline constexpr char canonical_role_id_set_sha256[] =
    "b19609a8de23f0e4f13cf5c2c3f7bcbf6b0de7b9aed34463f27efac403e7aec0";
inline constexpr char canonical_manifest_sha256[] =
    "cdd2b371bac623fb86cb1f13ce82fc10b540255a623cdd301f6c5499fe152b32";
inline constexpr char canonical_case_id_set_sha256[] =
    "467a0a2ff38858a46f7e4fdfbb8e3a2fc6a753c26a059e5be84319465c926a8d";
inline constexpr char canonical_case_manifest_sha256[] =
    "81fa3971f34fdaab711c43918eb8abf0e571764454c86e7305ec6f4b18e3f031";
inline constexpr char compatible_hostile_id_set_sha256[] =
    "1028ac1be238cd418e48699c509f27a0dfcded792869c4014a9f5f6d9a8e8698";
inline constexpr std::array<std::size_t, 17> canonical_shard_counts {
    30, 27, 20, 30, 21, 22, 17, 21, 17, 16, 18, 25, 23, 26, 16, 22, 18,
};
inline constexpr std::array<std::size_t, 17> canonical_case_shard_counts {
    492, 481, 530, 513, 518, 535, 492, 489, 519, 483, 470, 481, 538, 506,
    525, 494, 546,
};

enum class admission : std::uint8_t {
    exact_source_window,
    unadmitted_pending_trace_role_map,
};

struct role_record {
    std::string role_id;
    std::string syscall;
    std::string object;
    unsigned occurrence;
    std::string oracle_window;
    admission state;
    bool nonretryable;
    bool cleanup;
    unsigned profile_count;
};

struct case_identity {
    std::string case_id;
    std::string dedup_key;
};

namespace detail {

inline void add(std::vector<role_record> & out, const char * prefix,
                const char * syscall, unsigned occurrence,
                std::string object, std::string window, admission state,
                bool nonretryable, bool cleanup = false,
                unsigned profile_count = 18) {
    std::ostringstream id;
    id << prefix;
    if (occurrence < 100) id << '0';
    if (occurrence < 10) id << '0';
    id << occurrence;
    out.push_back({ id.str(), syscall, std::move(object), occurrence,
                    std::move(window), state, nonretryable, cleanup,
                    profile_count });
}

inline std::vector<role_record> build() {
    std::vector<role_record> out;
    out.reserve(369);

    constexpr const char * open_objects[37] {
        "parent", "root", "/proc/self/mountinfo#1", "writer.lock",
        "envelopes", "attempts", "staging", "fixture", "primitive.lock",
        "root.marker", "envelopes", "initialize-envelope.tmp:create",
        "parent", "root", "/proc/self/mountinfo#2", "writer.lock",
        "envelopes", "attempts", "staging", "fixture", "primitive.lock",
        "root.marker", "staging", "initialize-envelope.tmp:readonly",
        "published-envelope:first", "root.marker:first", "parent", "root",
        "/proc/self/mountinfo#3", "writer.lock", "envelopes", "attempts",
        "staging", "fixture", "primitive.lock", "published-envelope:confirm",
        "root.marker:confirm",
    };
    for (unsigned i = 1; i <= 37; ++i) {
        const char * window = i <= 10 ? "W03" : i <= 12 ? "W04" :
            i <= 24 ? "W09A" : i == 25 ? "W12A" : i == 26 ? "W12B" :
            i <= 35 ? "W12C" : "W12D";
        add(out, "COMP-OPEN-", "open/openat2", i, open_objects[i - 1],
            window, admission::exact_source_window, true);
    }

    constexpr const char * close_objects[52] {
        "l05y-reopened-staging", "l05y-reopened-root",
        "l05y-reopened-fixture-lock", "l05y-reopened-fixture",
        "premutation-reopened-parent", "/proc/self/mountinfo#1",
        "premutation-reopened-writer", "premutation-reopened-envelopes",
        "premutation-reopened-attempts", "premutation-reopened-staging",
        "premutation-reopened-root", "premutation-reopened-fixture-lock",
        "premutation-reopened-fixture", "marker-after", "marker-write",
        "marker-readonly", "published-marker", "staging-validation",
        "staging-final", "prepublication-reopened-parent",
        "/proc/self/mountinfo#2", "prepublication-reopened-writer",
        "prepublication-reopened-envelopes", "prepublication-reopened-attempts",
        "prepublication-reopened-staging", "prepublication-reopened-root",
        "prepublication-reopened-fixture-lock", "prepublication-reopened-fixture",
        "barrier-marker", "final-reopened-parent", "/proc/self/mountinfo#3",
        "final-reopened-writer", "final-reopened-envelopes",
        "final-reopened-attempts", "final-reopened-staging",
        "final-reopened-root", "final-reopened-fixture-lock",
        "final-reopened-fixture", "final-marker-confirmation", "final-marker",
        "final-envelope-confirmation", "published-envelope", "envelope-readonly",
        "envelope-staging-validation", "envelope-write", "envelopes",
        "staging", "fixture", "root", "parent", "writer", "fixture-lock",
    };
    for (unsigned i = 1; i <= 52; ++i) {
        const bool tail = i >= 40;
        const char * window = i <= 4 ? "W00/M12" :
            i <= 15 ? "W03" : i <= 19 ? "W03C" :
            i <= 29 ? "W09A" : i <= 39 ? "W12C" :
            i <= 46 ? "W13E" : i == 47 ? "W13M" : "W13O";
        add(out, "COMP-CLOSE-", "close", i, close_objects[i - 1], window,
            admission::exact_source_window,
            true, tail);
    }

    add(out, "COMP-FCHMOD-", "fchmod", 1, "initialize-envelope.tmp", "W05",
        admission::exact_source_window, true);
    add(out, "COMP-FSYNC-", "fsync", 1, "initialize-envelope.tmp", "W08",
        admission::exact_source_window, true);
    add(out, "COMP-FSYNC-", "fsync", 2, "envelopes", "W10",
        admission::exact_source_window, true);
    add(out, "COMP-FSYNC-", "fsync", 3, "staging", "W11",
        admission::exact_source_window, true);
    add(out, "COMP-RENAME-", "renameat2", 1,
        "initialize-envelope.tmp->published-envelope", "W09B",
        admission::exact_source_window, true);
    add(out, "COMP-PWRITE-", "pwrite64", 1, "initialize-envelope.tmp", "W06",
        admission::exact_source_window, false, false, 25);

    constexpr const char * read_windows[4] { "W07", "W09A", "W12A", "W12C" };
    constexpr const char * marker_windows[4] { "W03", "W09A", "W12B", "W12C" };
    for (unsigned i = 1; i <= 4; ++i) {
        add(out, "COMP-PREAD-ENV-", "pread64", i, "predecessor-envelope:data",
            read_windows[i - 1], admission::exact_source_window, false, false, 25);
        add(out, "COMP-PREAD-MARKER-", "pread64", i, "root.marker:data",
            marker_windows[i - 1], admission::exact_source_window, false, false, 25);
        add(out, "COMP-EOF-ENV-", "pread64", i, "predecessor-envelope:eof",
            read_windows[i - 1], admission::exact_source_window, false, false, 19);
        add(out, "COMP-EOF-MARKER-", "pread64", i, "root.marker:eof",
            marker_windows[i - 1], admission::exact_source_window, false, false, 19);
    }

    struct family { const char * id; const char * syscall; unsigned count; };
    constexpr family pending[] {
        { "L05Z-FSTAT-", "fstat", 86 }, { "L05Z-STATX-", "statx", 36 },
        { "L05Z-READLINK-", "readlink", 30 },
        { "L05Z-GETDENTS-", "getdents64", 32 },
        { "L05Z-FACTS-", "BTRFS_IOC_FS_INFO", 40 },
        { "L05Z-FSTATFS-", "fstatfs", 23 },
    };
    for (const auto & group : pending) {
        for (unsigned i = 1; i <= group.count; ++i) {
            add(out, group.id, group.syscall, i, "UNMAPPED_EXACT_OBJECT",
                "UNADMITTED_PENDING_TRACE_ROLE_MAP",
                admission::unadmitted_pending_trace_role_map, true);
        }
    }

    add(out, "L05Z-UNLOCK-", "fcntl:F_OFD_SETLK:F_UNLCK", 1,
        "writer.lock", "W13O", admission::exact_source_window, true, true);
    add(out, "L05Z-UNLOCK-", "fcntl:F_OFD_SETLK:F_UNLCK", 2,
        "fixture-lock", "W13O", admission::exact_source_window, true, true);

    add(out, "L05Z-CLEANUP-MUNLOCK-", "munlock", 1,
        "authenticated-input-storage", "W13O", admission::exact_source_window, true, true);
    add(out, "L05Z-CLEANUP-MUNMAP-", "munmap", 1,
        "authenticated-input-storage", "W13O", admission::exact_source_window, true, true);
    add(out, "L05Z-CLEANUP-SIGMASK-", "rt_sigprocmask", 1,
        "authenticated-input-saved-mask", "W13O", admission::exact_source_window, true, true);

    constexpr const char * mount_windows[3] { "W03", "W09A", "W12C" };
    for (unsigned pass = 1; pass <= 3; ++pass) {
        add(out, "L05Z-MOUNTINFO-DATA-", "read", pass,
            "/proc/self/mountinfo:stream", mount_windows[pass - 1],
            admission::exact_source_window, true);
        add(out, "L05Z-MOUNTINFO-EOF-", "read", pass,
            "/proc/self/mountinfo:eof", mount_windows[pass - 1],
            admission::exact_source_window, true);
    }
    return out;
}

} // namespace detail

inline const std::vector<role_record> & roles() {
    static const std::vector<role_record> value = detail::build();
    return value;
}

inline std::uint32_t shard_for(std::string_view id, std::uint32_t shard_count) {
    if (shard_count == 0) return UINT32_MAX;
    std::string material("halofpx.l05z.return-role-shard.v1");
    material.push_back('\0');
    material.append(id.data(), id.size());
    const auto digest = halofpx_l05z_return_hostile_manifest_v1::detail::sha256(material);
    std::uint32_t remainder = 0;
    for (const auto byte : digest) {
        remainder = static_cast<std::uint32_t>(
            (static_cast<std::uint64_t>(remainder) * 256U + byte) % shard_count);
    }
    return remainder;
}

inline std::string sorted_role_id_hash_hex() {
    std::string material("halofpx.l05z.return-role-authority.v1");
    material.push_back('\0');
    std::vector<std::string> ids;
    ids.reserve(roles().size());
    for (const auto & role : roles()) ids.push_back(role.role_id);
    std::sort(ids.begin(), ids.end());
    for (const auto & id : ids) { material += id; material.push_back('\n'); }
    return halofpx_l05z_return_hostile_manifest_v1::detail::hex(
        halofpx_l05z_return_hostile_manifest_v1::detail::sha256(material));
}

inline void append_canonical_field(std::string & output, std::string_view value) {
    output += std::to_string(value.size());
    output.push_back(':');
    output.append(value.data(), value.size());
}

inline const std::vector<case_identity> & canonical_cases() {
    static const std::vector<case_identity> value = [] {
        constexpr const char * errno_names[9] {
            "EIO", "ENOSPC", "EDQUOT", "EROFS", "EEXIST", "EXDEV",
            "ENOSYS", "EINVAL", "EINTR",
        };
        std::vector<std::string> profiles;
        profiles.reserve(25);
        for (const char * completion : { "PRE", "LATE" }) {
            for (const char * error : errno_names) {
                profiles.push_back(std::string(completion) + '-' + error);
            }
        }
        for (const char * extra : {
                 "EINTR-ONCE", "SHORT-PRE-1", "SHORT-PRE-NM1",
                 "SHORT-LATE-1", "SHORT-LATE-NM1", "ZERO-PRE", "ZERO-LATE" }) {
            profiles.emplace_back(extra);
        }

        std::vector<case_identity> output;
        output.reserve(canonical_total_cases);
        for (const auto & role : roles()) {
            for (unsigned index = 0; index < role.profile_count; ++index) {
                const std::string & profile = profiles[index];
                std::string semantics;
                append_canonical_field(semantics, role.syscall);
                append_canonical_field(semantics, role.object);
                append_canonical_field(semantics, std::to_string(role.occurrence));
                append_canonical_field(semantics, role.oracle_window);
                append_canonical_field(semantics, profile);
                append_canonical_field(semantics,
                    role.state == admission::exact_source_window ? "exact" : "pending");
                append_canonical_field(semantics,
                    role.nonretryable ? "nonretry" : "retry");
                append_canonical_field(semantics,
                    role.cleanup ? "cleanup" : "ordinary");
                output.push_back({ role.role_id + '-' + profile,
                    halofpx_l05z_return_hostile_manifest_v1::detail::domain_hash(
                        "halofpx.l05z.return-case-dedup.v1", semantics) });
            }
        }
        for (const auto & hostile :
                halofpx_l05z_return_hostile_manifest_v1::manifest()) {
            output.push_back({ hostile.case_id, hostile.dedup_key });
        }
        std::sort(output.begin(), output.end(),
            [](const case_identity & left, const case_identity & right) {
                return left.case_id < right.case_id;
            });
        return output;
    }();
    return value;
}

inline std::uint32_t case_shard_for(std::string_view id,
                                    std::uint32_t shard_count) {
    if (shard_count == 0) return UINT32_MAX;
    std::string material("halofpx.l05z.return-case-shard.v1");
    material.push_back('\0');
    material.append(id.data(), id.size());
    const auto digest = halofpx_l05z_return_hostile_manifest_v1::detail::sha256(material);
    std::uint32_t remainder = 0;
    for (const auto byte : digest) {
        remainder = static_cast<std::uint32_t>(
            (static_cast<std::uint64_t>(remainder) * 256U + byte) % shard_count);
    }
    return remainder;
}

inline std::string canonical_case_id_hash_hex() {
    std::string material("halofpx.l05z.return-canonical-case-ids.v1");
    material.push_back('\0');
    for (const auto & item : canonical_cases()) {
        material += item.case_id;
        material.push_back('\n');
    }
    return halofpx_l05z_return_hostile_manifest_v1::detail::hex(
        halofpx_l05z_return_hostile_manifest_v1::detail::sha256(material));
}

inline std::string canonical_case_manifest_hash_hex() {
    std::string material("halofpx.l05z.return-canonical-case-manifest.v1");
    material.push_back('\0');
    for (const auto & item : canonical_cases()) {
        append_canonical_field(material, item.case_id);
        append_canonical_field(material, item.dedup_key);
        material.push_back('\n');
    }
    return halofpx_l05z_return_hostile_manifest_v1::detail::hex(
        halofpx_l05z_return_hostile_manifest_v1::detail::sha256(material));
}

inline std::string manifest_hash_hex() {
    std::string material("halofpx.l05z.return-role-authority-manifest.v1");
    material.push_back('\0');
    for (const auto & role : roles()) {
        append_canonical_field(material, role.role_id);
        append_canonical_field(material, role.syscall);
        append_canonical_field(material, role.object);
        append_canonical_field(material, std::to_string(role.occurrence));
        append_canonical_field(material, role.oracle_window);
        append_canonical_field(material,
            role.state == admission::exact_source_window ? "exact" : "pending");
        append_canonical_field(material, role.nonretryable ? "nonretry" : "retry");
        append_canonical_field(material, role.cleanup ? "cleanup" : "ordinary");
        append_canonical_field(material, std::to_string(role.profile_count));
        material.push_back('\n');
    }
    return halofpx_l05z_return_hostile_manifest_v1::detail::hex(
        halofpx_l05z_return_hostile_manifest_v1::detail::sha256(material));
}

inline bool self_check() {
    if (roles().size() != 369 || canonical_compatibility_cases != 2069 ||
        canonical_l05z_role_cases != 4644 || canonical_hostile_cases != 1899 ||
        canonical_total_cases != 8612 || physical_execution_rows_frozen ||
        canonical_cleanup_subset != 324 || legacy_cleanup_execution_subset != 337)
        return false;
    std::set<std::string> ids;
    std::array<std::size_t, 17> shards {};
    std::size_t pending = 0, retryable = 0, cleanup = 0;
    for (const auto & role : roles()) {
        if (role.role_id.empty() || role.syscall.empty() || role.object.empty() ||
            role.oracle_window.empty() || !ids.insert(role.role_id).second) return false;
        ++shards[shard_for(role.role_id, static_cast<std::uint32_t>(shards.size()))];
        if (role.state == admission::unadmitted_pending_trace_role_map) ++pending;
        if (!role.nonretryable) ++retryable;
        if (role.cleanup) ++cleanup;
    }
    std::set<std::string> case_ids, dedup_keys;
    std::array<std::size_t, 17> case_shards {};
    for (const auto & item : canonical_cases()) {
        if (item.case_id.empty() || item.dedup_key.size() != 64 ||
            !case_ids.insert(item.case_id).second ||
            !dedup_keys.insert(item.dedup_key).second) return false;
        ++case_shards[case_shard_for(
            item.case_id, static_cast<std::uint32_t>(case_shards.size()))];
    }
    return ids.size() == roles().size() && pending == 247 && retryable == 17 &&
        cleanup == 18 && shards == canonical_shard_counts &&
        canonical_cases().size() == canonical_total_cases &&
        case_ids.size() == canonical_total_cases &&
        case_shards == canonical_case_shard_counts &&
        sorted_role_id_hash_hex() == canonical_role_id_set_sha256 &&
        manifest_hash_hex() == canonical_manifest_sha256 &&
        canonical_case_id_hash_hex() == canonical_case_id_set_sha256 &&
        canonical_case_manifest_hash_hex() == canonical_case_manifest_sha256 &&
        halofpx_l05z_return_hostile_manifest_v1::sorted_id_set_hash_hex() ==
            compatible_hostile_id_set_sha256;
}

} // namespace halofpx_l05z_return_role_authority_v1

#endif
