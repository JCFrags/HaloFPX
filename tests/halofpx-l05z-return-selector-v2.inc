#ifndef HALOFPX_L05Z_RETURN_SELECTOR_V2_INC
#define HALOFPX_L05Z_RETURN_SELECTOR_V2_INC

// Test-only exact-selector authority for the mapped L05z return roles.  This
// version binds canonical case identities to role-map-v1 records but admits no
// selector to execution.  It is deliberately data-only: there is no run or
// syscall-matching entry point in this authority.

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <set>
#include <string>
#include <string_view>
#include <vector>

namespace halofpx_l05z_return_selector_v2 {

enum class admission : std::uint8_t {
    closed = 0,
    executable = 1,
};

enum class resolution : std::uint8_t {
    resolved_closed,
    resolved_executable,
    unknown_case_id,
    malformed_case_id,
    contradictory_identity,
};

struct profile_record {
    const char * profile_id;
    const char * completion;
    const char * returned_errno;
};

struct selector_record {
    std::string case_id;
    const halofpx_l05z_return_role_map_v1::role_map_record * role;
    const profile_record * profile;
    const halofpx_l05z_return_role_authority_v1::case_identity *
        compatibility_case;
    admission state;
};

struct selector_claim {
    std::string_view case_id;
    std::string_view role_id;
    std::string_view syscall_subtype;
    std::string_view semantic_object;
    std::string_view descriptor_role;
    std::string_view window_phase;
    std::string_view source_route_helper;
    unsigned clean_path_ordinal;
    std::string_view stream_part_class;
    bool nonretryable;
    std::string_view expected_residual_oracle;
    std::string_view current_physical_profile;
    std::string_view profile_id;
    std::string_view completion;
    std::string_view returned_errno;
    std::string_view compatibility_dedup_key;
    admission state;
};

struct resolve_result {
    resolution status;
    const selector_record * selector;
};

inline constexpr std::size_t mapped_role_count = 247;
inline constexpr std::size_t nonretryable_profile_count = 18;
inline constexpr std::size_t canonical_case_count =
    mapped_role_count * nonretryable_profile_count;
inline constexpr std::array<std::size_t, 6> family_role_counts {
    86, 36, 30, 32, 40, 23,
};
inline constexpr std::array<std::size_t, 6> family_case_counts {
    1548, 648, 540, 576, 720, 414,
};
inline constexpr admission selector_admission = admission::closed;
inline constexpr bool live_matching_connected = false;
inline constexpr bool run_entry_point_present = false;

// This authority was introduced immediately above this exact clean source
// snapshot.  The map commitment pins both the reviewed source bytes and its
// independently domain-separated semantic manifest.
inline constexpr char source_parent_commit[] =
    "51f073f592d24b0b3244a33b57b8bc14d95b0356";
inline constexpr char source_parent_tree[] =
    "4575b7c4709cf6fe6ec9eb55d3a09f6ab869a72d";
inline constexpr char role_map_v1_source_sha256[] =
    "383f59126f725047c9aca913bf3d3e2d86540146fc5672e570ce15b70a1a2447";
inline constexpr char role_map_v1_manifest_sha256[] =
    "190abec2a90a78ea1b3aebf41adb75d824e9b533f4cfd9ff8ac2f3152994df8b";
inline constexpr char canonical_manifest_sha256[] =
    "5735bd55d0a4885a796aee6c82d8e2a6cdbf88cfa8a4e148c56bcfe201f245f1";

inline constexpr std::array<profile_record, nonretryable_profile_count>
profiles {{
    { "PRE-EIO",     "PRE",  "EIO" },
    { "PRE-ENOSPC",  "PRE",  "ENOSPC" },
    { "PRE-EDQUOT",  "PRE",  "EDQUOT" },
    { "PRE-EROFS",   "PRE",  "EROFS" },
    { "PRE-EEXIST",  "PRE",  "EEXIST" },
    { "PRE-EXDEV",   "PRE",  "EXDEV" },
    { "PRE-ENOSYS",  "PRE",  "ENOSYS" },
    { "PRE-EINVAL",  "PRE",  "EINVAL" },
    { "PRE-EINTR",   "PRE",  "EINTR" },
    { "LATE-EIO",    "LATE", "EIO" },
    { "LATE-ENOSPC", "LATE", "ENOSPC" },
    { "LATE-EDQUOT", "LATE", "EDQUOT" },
    { "LATE-EROFS",  "LATE", "EROFS" },
    { "LATE-EEXIST", "LATE", "EEXIST" },
    { "LATE-EXDEV",  "LATE", "EXDEV" },
    { "LATE-ENOSYS", "LATE", "ENOSYS" },
    { "LATE-EINVAL", "LATE", "EINVAL" },
    { "LATE-EINTR",  "LATE", "EINTR" },
}};

inline bool starts_with(std::string_view value, std::string_view prefix) {
    return value.size() >= prefix.size() &&
        value.substr(0, prefix.size()) == prefix;
}

inline std::size_t family_for(std::string_view role_id) {
    constexpr std::array<const char *, 6> prefixes {
        "L05Z-FSTAT-", "L05Z-STATX-", "L05Z-READLINK-",
        "L05Z-GETDENTS-", "L05Z-FACTS-", "L05Z-FSTATFS-",
    };
    for (std::size_t index = 0; index < prefixes.size(); ++index) {
        if (starts_with(role_id, prefixes[index])) return index;
    }
    return prefixes.size();
}

inline const std::vector<selector_record> & records() {
    static const std::vector<selector_record> value = [] {
        std::vector<selector_record> output;
        output.reserve(canonical_case_count);
        for (const auto & role : halofpx_l05z_return_role_map_v1::records()) {
            for (const auto & profile : profiles) {
                const std::string case_id =
                    std::string(role.role_id) + '-' + profile.profile_id;
                const auto & compatibility =
                    halofpx_l05z_return_role_authority_v1::
                        compatibility_execution_case_roster();
                const auto found = std::lower_bound(
                    compatibility.begin(), compatibility.end(), case_id,
                    [](const auto & record, const std::string & requested) {
                        return record.case_id < requested;
                    });
                output.push_back({
                    case_id, &role, &profile,
                    found != compatibility.end() && found->case_id == case_id
                        ? &*found : nullptr,
                    admission::closed,
                });
            }
        }
        std::sort(output.begin(), output.end(),
            [](const selector_record & left, const selector_record & right) {
                return left.case_id < right.case_id;
            });
        return output;
    }();
    return value;
}

inline const selector_record * exact_case(std::string_view case_id) {
    const auto found = std::lower_bound(records().begin(), records().end(), case_id,
        [](const selector_record & record, std::string_view requested) {
            return std::string_view(record.case_id) < requested;
        });
    return found != records().end() &&
        std::string_view(found->case_id) == case_id ? &*found : nullptr;
}

inline bool has_mapped_role_prefix(std::string_view case_id) {
    for (const auto & role : halofpx_l05z_return_role_map_v1::records()) {
        const std::string prefix = std::string(role.role_id) + '-';
        if (starts_with(case_id, prefix)) return true;
    }
    return false;
}

inline resolve_result resolve_case_id(std::string_view case_id) {
    if (const auto * selected = exact_case(case_id)) {
        return { selected->state == admission::closed ?
            resolution::resolved_closed : resolution::resolved_executable,
            selected };
    }
    if (has_mapped_role_prefix(case_id)) {
        return { resolution::malformed_case_id, nullptr };
    }
    return { resolution::unknown_case_id, nullptr };
}

inline resolve_result resolve_exact_claim(const selector_claim & claim) {
    const resolve_result resolved = resolve_case_id(claim.case_id);
    if (resolved.status != resolution::resolved_closed &&
        resolved.status != resolution::resolved_executable) return resolved;
    const auto & selected = *resolved.selector;
    const auto & role = *selected.role;
    if (claim.role_id != role.role_id ||
        claim.syscall_subtype != role.syscall_subtype ||
        claim.semantic_object != role.semantic_object ||
        claim.descriptor_role != role.descriptor_role ||
        claim.window_phase != role.window_phase ||
        claim.source_route_helper != role.source_route_helper ||
        claim.clean_path_ordinal != role.clean_path_ordinal ||
        claim.stream_part_class != role.stream_part_class ||
        claim.nonretryable != role.nonretryable ||
        claim.expected_residual_oracle != role.expected_residual_oracle ||
        claim.current_physical_profile != role.current_physical_profile ||
        claim.profile_id != selected.profile->profile_id ||
        claim.completion != selected.profile->completion ||
        claim.returned_errno != selected.profile->returned_errno ||
        !selected.compatibility_case ||
        claim.compatibility_dedup_key !=
            selected.compatibility_case->dedup_key ||
        claim.state != selected.state) {
        return { resolution::contradictory_identity, nullptr };
    }
    return resolved;
}

inline void append_field(std::string & output, std::string_view value) {
    output += std::to_string(value.size());
    output.push_back(':');
    output.append(value.data(), value.size());
}

inline std::string manifest_hash_hex() {
    std::string material("halofpx.l05z.return-selector.v2");
    material.push_back('\0');
    append_field(material, source_parent_commit);
    append_field(material, source_parent_tree);
    append_field(material, role_map_v1_source_sha256);
    append_field(material, role_map_v1_manifest_sha256);
    append_field(material, selector_admission == admission::closed ?
        "closed" : "executable");
    append_field(material, live_matching_connected ? "connected" : "disconnected");
    append_field(material, run_entry_point_present ? "present" : "absent");
    for (const auto count : family_role_counts)
        append_field(material, std::to_string(count));
    for (const auto & selected : records()) {
        append_field(material, selected.case_id);
        append_field(material, selected.role->role_id);
        append_field(material, selected.role->syscall_subtype);
        append_field(material, selected.role->semantic_object);
        append_field(material, selected.role->descriptor_role);
        append_field(material, selected.role->window_phase);
        append_field(material,
            std::to_string(selected.role->clean_path_ordinal));
        append_field(material, selected.role->stream_part_class);
        append_field(material, selected.role->expected_residual_oracle);
        append_field(material, selected.profile->profile_id);
        append_field(material, selected.profile->completion);
        append_field(material, selected.profile->returned_errno);
        append_field(material, selected.compatibility_case
            ? selected.compatibility_case->dedup_key : "missing");
        append_field(material, selected.state == admission::closed ?
            "closed" : "executable");
        material.push_back('\n');
    }
    return halofpx_l05z_return_hostile_manifest_v1::detail::hex(
        halofpx_l05z_return_hostile_manifest_v1::detail::sha256(material));
}

inline selector_claim exact_claim_for(const selector_record & selected) {
    return {
        selected.case_id, selected.role->role_id,
        selected.role->syscall_subtype, selected.role->semantic_object,
        selected.role->descriptor_role, selected.role->window_phase,
        selected.role->source_route_helper,
        selected.role->clean_path_ordinal, selected.role->stream_part_class,
        selected.role->nonretryable, selected.role->expected_residual_oracle,
        selected.role->current_physical_profile, selected.profile->profile_id,
        selected.profile->completion, selected.profile->returned_errno,
        selected.compatibility_case
            ? std::string_view(selected.compatibility_case->dedup_key)
            : std::string_view(),
        selected.state,
    };
}

inline bool self_check() {
    static_assert(selector_admission == admission::closed,
        "selector-v2 execution requires an explicit reviewed source change");
    static_assert(!live_matching_connected && !run_entry_point_present,
        "selector-v2 must remain disconnected from execution");
    if (!halofpx_l05z_return_role_map_v1::self_check() ||
        !halofpx_l05z_return_role_authority_v1::self_check() ||
        std::string_view(halofpx_l05z_return_role_map_v1::canonical_manifest_sha256) !=
            role_map_v1_manifest_sha256 ||
        halofpx_l05z_return_role_map_v1::manifest_hash_hex() !=
            role_map_v1_manifest_sha256 ||
        halofpx_l05z_return_role_map_v1::records().size() != mapped_role_count ||
        profiles.size() != nonretryable_profile_count ||
        records().size() != canonical_case_count) return false;

    std::set<std::string> profile_ids;
    for (const auto & profile : profiles) {
        if (!profile.profile_id || !*profile.profile_id ||
            !profile.completion || !*profile.completion ||
            !profile.returned_errno || !*profile.returned_errno ||
            !profile_ids.insert(profile.profile_id).second ||
            std::string(profile.completion) + '-' + profile.returned_errno !=
                profile.profile_id) return false;
    }

    std::array<std::size_t, 6> roles_by_family {};
    for (const auto & role : halofpx_l05z_return_role_map_v1::records()) {
        const std::size_t family = family_for(role.role_id);
        if (family >= roles_by_family.size() || !role.nonretryable ||
            role.state != halofpx_l05z_return_role_map_v1::admission::
                mapped_not_executable) return false;
        ++roles_by_family[family];
    }
    if (roles_by_family != family_role_counts) return false;

    std::array<std::size_t, 6> cases_by_family {};
    std::set<std::string> case_ids;
    std::size_t executable = 0;
    for (const auto & selected : records()) {
        const std::size_t family = family_for(selected.role->role_id);
        if (family >= cases_by_family.size() ||
            !case_ids.insert(selected.case_id).second ||
            !selected.compatibility_case ||
            selected.compatibility_case->case_id != selected.case_id ||
            selected.compatibility_case->dedup_key.size() != 64 ||
            selected.case_id != std::string(selected.role->role_id) + '-' +
                selected.profile->profile_id) return false;
        ++cases_by_family[family];
        if (selected.state == admission::executable) ++executable;

        const resolve_result parsed = resolve_case_id(selected.case_id);
        if (parsed.status != resolution::resolved_closed ||
            parsed.selector != &selected) return false;
        selector_claim claim = exact_claim_for(selected);
        const resolve_result exact = resolve_exact_claim(claim);
        if (exact.status != resolution::resolved_closed ||
            exact.selector != &selected) return false;
        ++claim.clean_path_ordinal;
        if (resolve_exact_claim(claim).status !=
            resolution::contradictory_identity) return false;
    }
    if (cases_by_family != family_case_counts || executable != 0) return false;

    const auto & witness = records().front();
    selector_claim contradiction = exact_claim_for(witness);
    contradiction.role_id = "L05Z-FSTAT-999";
    if (resolve_exact_claim(contradiction).status !=
            resolution::contradictory_identity ||
        resolve_case_id(std::string(witness.role->role_id) + "-EINTR-ONCE").status !=
            resolution::malformed_case_id ||
        resolve_case_id("L05Z-FSTAT-999-PRE-EIO").status !=
            resolution::unknown_case_id ||
        resolve_case_id("not-a-selector-case").status !=
            resolution::unknown_case_id) return false;

    contradiction = exact_claim_for(witness);
    contradiction.source_route_helper = "wrong-route";
    if (resolve_exact_claim(contradiction).status !=
            resolution::contradictory_identity) return false;
    contradiction = exact_claim_for(witness);
    contradiction.stream_part_class = "wrong-stream";
    if (resolve_exact_claim(contradiction).status !=
            resolution::contradictory_identity) return false;
    contradiction = exact_claim_for(witness);
    contradiction.expected_residual_oracle = "wrong-residual";
    if (resolve_exact_claim(contradiction).status !=
            resolution::contradictory_identity) return false;
    contradiction = exact_claim_for(witness);
    contradiction.current_physical_profile = "wrong-profile";
    if (resolve_exact_claim(contradiction).status !=
            resolution::contradictory_identity) return false;
    contradiction = exact_claim_for(witness);
    contradiction.completion = "wrong-completion";
    if (resolve_exact_claim(contradiction).status !=
            resolution::contradictory_identity) return false;
    contradiction = exact_claim_for(witness);
    contradiction.returned_errno = "wrong-errno";
    if (resolve_exact_claim(contradiction).status !=
            resolution::contradictory_identity) return false;
    contradiction = exact_claim_for(witness);
    contradiction.compatibility_dedup_key =
        "0000000000000000000000000000000000000000000000000000000000000000";
    if (resolve_exact_claim(contradiction).status !=
            resolution::contradictory_identity) return false;
    contradiction = exact_claim_for(witness);
    contradiction.nonretryable = false;
    if (resolve_exact_claim(contradiction).status !=
            resolution::contradictory_identity) return false;
    contradiction = exact_claim_for(witness);
    contradiction.state = admission::executable;
    if (resolve_exact_claim(contradiction).status !=
            resolution::contradictory_identity) return false;

    return manifest_hash_hex() == canonical_manifest_sha256;
}

} // namespace halofpx_l05z_return_selector_v2

#endif
