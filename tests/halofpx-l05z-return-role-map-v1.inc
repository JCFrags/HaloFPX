#ifndef HALOFPX_L05Z_RETURN_ROLE_MAP_V1_INC
#define HALOFPX_L05Z_RETURN_ROLE_MAP_V1_INC

// Target-native, test-only source map for the 247 L05z returned-fault roles
// that remain closed to execution.  This authority is parallel to the frozen
// v1 compatibility roster: it does not alter a legacy role, case ID, or hash.
// Mapping is evidence, not executable admission; the controller now rejects
// these aggregate boundaries until a separate exact-selector version exists.

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <set>
#include <string>
#include <string_view>
#include <vector>

namespace halofpx_l05z_return_role_map_v1 {

enum class admission : std::uint8_t {
    pending,
    mapped_not_executable,
    executable,
};

struct role_map_record {
    const char * role_id;
    const char * syscall_subtype;
    const char * semantic_object;
    const char * descriptor_role;
    const char * window_phase;
    const char * source_route_helper;
    unsigned clean_path_ordinal;
    const char * stream_part_class;
    bool nonretryable;
    const char * expected_residual_oracle;
    const char * current_physical_profile;
    admission state;
};

inline constexpr std::size_t mapped_role_count = 247;
inline constexpr std::array<std::size_t, 6> family_counts {
    86, 36, 30, 32, 40, 23,
};
inline constexpr char source_base_commit[] =
    "61f2f2d7bc4955e9bca821095ef69125837133b5";
inline constexpr char source_base_tree[] =
    "0a35143f33a7b99a81c824fa8ffd8f743f7ae0dd";
inline constexpr char authority_parent_commit[] =
    "65ef0eba8027ce1738fdc97fab649d4ca5b4e0d3";
inline constexpr char authority_parent_tree[] =
    "fdc34babba04e1766b852d6d1dcddfbb6314f0a6";
// The trace maps were derived against this immutable pre-map source snapshot.
inline constexpr char mapping_source_commit[] =
    "eba2e36764e1624d6d20bb00657dd7f2fe64a1be";
inline constexpr char mapping_source_tree[] =
    "32a87f74cadb1853abe190b25910d3cdde0b4d70";
inline constexpr char initializer_anchor_sha256[] =
    "a94c08e554ca449e04f1159b1dbd4ff049f00365e3b3cf45feffab8fbb968f82";
inline constexpr char mapping_snapshot_controller_source_sha256[] =
    "d7a915012d15857a8acf3146ff00d5cc4f31d33dec931c282b4761ece58b7e3e";
inline constexpr char mapping_snapshot_role_authority_source_sha256[] =
    "95cf09b5e3ca03574e02bba6c5d01db516ad798049018947d6d139d7c0e261fe";
inline constexpr char frozen_role_authority_canonical_manifest_sha256[] =
    "cdd2b371bac623fb86cb1f13ce82fc10b540255a623cdd301f6c5499fe152b32";
inline constexpr std::array<const char *, 3> retained_discovery_trace_sha256 {
    "b5eb5027be47b0aea4e8353068537fe675c9f073aa39c9d0f2d16ff22303aa5c",
    "6693b059dfefd108f3e364b49bf46d7806417cdccb019d9397251d6791064742",
    "501c684fe925432c86cf17813d1fbd04660def5e0b36b173ecb65a6c4c847fc8",
};
// Ordered fstat, statx, readlink, getdents64, Btrfs-facts, fstatfs fragment
// source commitments. All values identify the exact reviewed text expanded
// below.
inline constexpr std::array<const char *, 6> role_map_fragment_sha256 {
    "0805097ec2494f87142a8f01192cd1ba500216a9e57e421e14ef9e3a68273fac",
    "26bffe5496d3bbda7e6eccb80de373beeda3832b417672c5f45bc35be45b3250",
    "299f4d3128e785e52c5f6ba562b84cbba257b486d8dfee996dc4fa51fb69b958",
    "8784bc99dc512e29f4a70049c2f2897c0bd71194683115e65ac0e80c5e95a0fc",
    "5a63dfbf62c6b0f39ce7f2eab683b4434a5fd6664cf6dbc358fda36d21a91dc8",
    "233cad281518ee300acb72e7336861087794c5790562c72fcacbb0d97d720a98",
};
inline constexpr char physical_profile[] =
    "nimo-2|Linux-7.1.3|glibc-2.43|x86_64|Btrfs|one-getdents-fragment-plus-EOF";
// Domain-separated semantic commitment for the stable reviewed fragments.
// A mismatch keeps the diagnostic closed and nonzero; it cannot open product
// execution.
inline constexpr char canonical_manifest_sha256[] =
    "190abec2a90a78ea1b3aebf41adb75d824e9b533f4cfd9ff8ac2f3152994df8b";

inline bool starts_with(std::string_view value, std::string_view prefix) {
    return value.size() >= prefix.size() &&
        value.substr(0, prefix.size()) == prefix;
}

inline const char * physical_profile_for(std::string_view id) {
    if (starts_with(id, "L05Z-FSTAT-"))
        return "Linux/glibc SYS_fstat;rval=0;semantic-output-validation-required";
    if (starts_with(id, "L05Z-STATX-"))
        return "SYS_statx empty-path/basic-stats/mnt-id;rval=0;runtime-size-witness";
    if (starts_with(id, "L05Z-READLINK-"))
        return "SYS_readlink count=4097;exact-target-bytes-and-length";
    if (starts_with(id, "L05Z-GETDENTS-"))
        return "SYS_getdents64 count=4096;one-positive-fragment-plus-EOF;extra-fragment-mismatch";
    if (starts_with(id, "L05Z-FACTS-"))
        return "Btrfs-ioctl;rval=0;exact-subtype-and-runtime-UUID-witness";
    if (starts_with(id, "L05Z-FSTATFS-"))
        return "SYS_fstatfs;rval=0;D/C native,R glibc-fstatvfs-lowering";
    return "unknown-closed";
}

inline const std::vector<role_map_record> & records() {
    static const std::vector<role_map_record> value = [] {
        std::vector<role_map_record> output;
        output.reserve(mapped_role_count);
#define HALOFPX_L05Z_ROLE_MAP_ROW(ID, SYSCALL, OBJECT, DESCRIPTOR, WINDOW, ROUTE, ORDINAL, STREAM, ORACLE) \
        output.push_back({ ID, SYSCALL, OBJECT, DESCRIPTOR, WINDOW, ROUTE, ORDINAL, STREAM, true, ORACLE, physical_profile_for(ID), admission::mapped_not_executable });
#include "halofpx-l05z-return-role-map-fstat.inc"
#include "halofpx-l05z-return-role-map-statx.inc"
#include "halofpx-l05z-return-role-map-readlink.inc"
#include "halofpx-l05z-return-role-map-getdents.inc"
#include "halofpx-l05z-return-role-map-facts.inc"
#include "halofpx-l05z-return-role-map-fstatfs.inc"
#undef HALOFPX_L05Z_ROLE_MAP_ROW
        return output;
    }();
    return value;
}

inline void append_field(std::string & output, std::string_view value) {
    output += std::to_string(value.size());
    output.push_back(':');
    output.append(value.data(), value.size());
}

inline std::string manifest_hash_hex() {
    std::string material("halofpx.l05z.return-role-map.v1");
    material.push_back('\0');
    append_field(material, source_base_commit);
    append_field(material, source_base_tree);
    append_field(material, authority_parent_commit);
    append_field(material, authority_parent_tree);
    append_field(material, mapping_source_commit);
    append_field(material, mapping_source_tree);
    append_field(material, initializer_anchor_sha256);
    append_field(material, mapping_snapshot_controller_source_sha256);
    append_field(material, mapping_snapshot_role_authority_source_sha256);
    append_field(material, frozen_role_authority_canonical_manifest_sha256);
    for (const char * trace : retained_discovery_trace_sha256)
        append_field(material, trace);
    for (const char * fragment : role_map_fragment_sha256)
        append_field(material, fragment);
    append_field(material, physical_profile);
    material.push_back('\n');
    for (const auto & role : records()) {
        append_field(material, role.role_id);
        append_field(material, role.syscall_subtype);
        append_field(material, role.semantic_object);
        append_field(material, role.descriptor_role);
        append_field(material, role.window_phase);
        append_field(material, role.source_route_helper);
        append_field(material, std::to_string(role.clean_path_ordinal));
        append_field(material, role.stream_part_class);
        append_field(material, role.nonretryable ? "nonretry" : "retry");
        append_field(material, role.expected_residual_oracle);
        append_field(material, role.current_physical_profile);
        append_field(material, role.state == admission::pending ? "pending" :
            role.state == admission::mapped_not_executable ?
                "mapped_not_executable" : "executable");
        material.push_back('\n');
    }
    return halofpx_l05z_return_hostile_manifest_v1::detail::hex(
        halofpx_l05z_return_hostile_manifest_v1::detail::sha256(material));
}

inline bool self_check() {
    if (records().size() != mapped_role_count) return false;
    constexpr std::array<const char *, 6> prefixes {
        "L05Z-FSTAT-", "L05Z-STATX-", "L05Z-READLINK-",
        "L05Z-GETDENTS-", "L05Z-FACTS-", "L05Z-FSTATFS-",
    };
    std::array<std::size_t, 6> counts {};
    std::array<unsigned, 6> next_ordinal { 1, 1, 1, 1, 1, 1 };
    std::set<std::string> ids;
    std::size_t pending = 0, mapped = 0, executable = 0;
    std::size_t facts_fs_info = 0, facts_subvol = 0;
    std::size_t getdents_pos = 0, getdents_eof = 0;
    std::size_t fstatfs_directory = 0, fstatfs_created = 0,
        fstatfs_reserve = 0;
    for (const auto & role : records()) {
        if (!role.role_id || !*role.role_id || !role.syscall_subtype ||
            !*role.syscall_subtype || !role.semantic_object ||
            !*role.semantic_object || !role.descriptor_role ||
            !*role.descriptor_role || !role.window_phase ||
            !*role.window_phase || !role.source_route_helper ||
            !*role.source_route_helper || !role.stream_part_class ||
            !*role.stream_part_class || !role.expected_residual_oracle ||
            !*role.expected_residual_oracle || !role.current_physical_profile ||
            !*role.current_physical_profile || !role.nonretryable ||
            !ids.insert(role.role_id).second) return false;
        std::size_t family = prefixes.size();
        for (std::size_t i = 0; i < prefixes.size(); ++i) {
            if (starts_with(role.role_id, prefixes[i])) { family = i; break; }
        }
        if (family == prefixes.size() ||
            role.clean_path_ordinal != next_ordinal[family]++) return false;
        ++counts[family];
        if (role.state == admission::pending) ++pending;
        else if (role.state == admission::mapped_not_executable) ++mapped;
        else ++executable;
    }
    if (counts != family_counts || pending != 0 || mapped != mapped_role_count ||
        executable != 0) return false;

    for (const auto & role : records()) {
        const std::string_view id(role.role_id), subtype(role.syscall_subtype);
        if (starts_with(id, "L05Z-FACTS-")) {
            const bool odd = (role.clean_path_ordinal % 2U) == 1U;
            if ((odd && subtype != "BTRFS_IOC_FS_INFO") ||
                (!odd && subtype != "BTRFS_IOC_GET_SUBVOL_INFO")) return false;
            odd ? ++facts_fs_info : ++facts_subvol;
        }
        if (starts_with(id, "L05Z-GETDENTS-")) {
            const bool odd = (role.clean_path_ordinal % 2U) == 1U;
            if ((odd && std::string_view(role.stream_part_class) != "POS") ||
                (!odd && std::string_view(role.stream_part_class) != "EOF"))
                return false;
            odd ? ++getdents_pos : ++getdents_eof;
        }
        if (starts_with(id, "L05Z-STATX-") &&
            subtype != "SYS_statx-empty-path-basic-stats-mnt-id") {
            return false;
        }
        if (starts_with(id, "L05Z-FSTATFS-")) {
            if (subtype == "SYS_fstatfs-inspect-directory") ++fstatfs_directory;
            else if (subtype == "SYS_fstatfs-inspect-created-directory")
                ++fstatfs_created;
            else if (subtype == "SYS_fstatfs-fstatvfs-reserve")
                ++fstatfs_reserve;
            else return false;
        }
        if (id == "L05Z-FSTAT-083" || id == "L05Z-FSTAT-084" ||
            id == "L05Z-FSTAT-085" || id == "L05Z-FSTAT-086" ||
            id == "L05Z-STATX-035" || id == "L05Z-STATX-036") {
            if (!starts_with(role.window_phase, "W12D")) return false;
        }
        if (id == "L05Z-FSTATFS-003" || id == "L05Z-FSTATFS-011" ||
            id == "L05Z-FSTATFS-019") {
            if (subtype != "SYS_fstatfs-fstatvfs-reserve") return false;
        }
    }
    if (facts_fs_info != 20 || facts_subvol != 20 || getdents_pos != 16 ||
        getdents_eof != 16 || fstatfs_directory != 9 ||
        fstatfs_created != 11 || fstatfs_reserve != 3) return false;
    const auto statx_mode_witness = std::find_if(records().begin(), records().end(),
        [](const role_map_record & role) {
            return std::string_view(role.role_id) == "L05Z-STATX-011";
        });
    if (statx_mode_witness == records().end() ||
        std::string_view(statx_mode_witness->expected_residual_oracle).find(
            "temp-retained-mode-subset-of-0600-size0") ==
                std::string_view::npos) return false;
    std::set<std::string> frozen_pending_ids;
    for (const auto & role : halofpx_l05z_return_role_authority_v1::roles()) {
        if (role.state == halofpx_l05z_return_role_authority_v1::admission::
                unadmitted_pending_trace_role_map) {
            frozen_pending_ids.insert(role.role_id);
        }
    }
    return frozen_pending_ids.size() == mapped_role_count &&
        frozen_pending_ids == ids &&
        manifest_hash_hex() == canonical_manifest_sha256;
}

} // namespace halofpx_l05z_return_role_map_v1

#endif
