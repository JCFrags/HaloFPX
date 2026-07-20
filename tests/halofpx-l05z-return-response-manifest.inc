#ifndef HALOFPX_L05Z_RETURN_RESPONSE_MANIFEST_INC
#define HALOFPX_L05Z_RETURN_RESPONSE_MANIFEST_INC

// Test-only authority for the single admitted L05z response-loss semantic
// case.  The observed libc syscall profile is physical metadata and is not
// part of the case identity or the frozen 8,612-case returned-fault set.

#include <array>
#include <string>
#include <string_view>

namespace halofpx_l05z_return_response_manifest_v1 {

inline constexpr std::size_t canonical_case_count = 1;
inline constexpr std::size_t compatible_authority_cases = 8612;
inline constexpr std::size_t compatible_extended_execution_total = 8613;
inline constexpr std::size_t semantic_unique_authority_cases = 8595;
inline constexpr std::size_t semantic_unique_extended_total = 8596;
inline constexpr std::array<std::size_t, 17>
    semantic_unique_extended_case_shard_counts {
        492, 481, 530, 511, 517, 534, 491, 490, 518, 482, 470, 480, 535,
        504, 522, 493, 546,
    };
// Historical name retained for source compatibility.  This is an execution
// roster count, not a unique-semantic count.
inline constexpr std::size_t extended_semantic_total = 8613;
inline constexpr std::array<std::size_t, 17> extended_case_shard_counts {
    492, 481, 530, 513, 518, 535, 492, 490, 519, 483, 470, 481, 538, 506,
    525, 494, 546,
};
inline constexpr char reviewed_base_commit[] =
    "61f2f2d7bc4955e9bca821095ef69125837133b5";
inline constexpr char reviewed_base_tree[] =
    "0a35143f33a7b99a81c824fa8ffd8f743f7ae0dd";
inline constexpr char case_id[] = "L05Z-RSP-LOSS-FULL-001";
inline constexpr char semantic_tuple[] =
    "extent=l05z|channel=stdout-audit|transaction=1|suppress-before-kernel|"
    "fake-full-success|delivered=0|child=0|launcher=0|discard=whole-root";
inline constexpr char dedup_key[] =
    "3ee5b6e0d4d4042dbf37b9512582fbb3db20ba355cccdeb35b8656b81dbdc706";
inline constexpr char canonical_id_set_sha256[] =
    "e156ba883397da2fa6d8e56236a1be3dd7832ec36bcf99e4049a89005a7ff7a8";
inline constexpr char canonical_manifest_sha256[] =
    "5e9b6c8f4dfe31826c66bd1749df106abea4e90ecdacd164bada3506f086194f";
inline constexpr char extended_case_id_set_sha256[] =
    "78dbb9b07f7023abf79f3dc745e77be857b32b46455fe937fda84df2084fe2e4";
inline constexpr char extended_case_manifest_sha256[] =
    "0af374c673a292a9c7f4a391a95bb3279d9d2ddd84ce210a54ef4db072b2c9e6";
inline constexpr char semantic_unique_extended_case_id_set_sha256[] =
    "3988a34d1db9c9778c4daab8d5260e0d1f3da5ac4e668abea011ba279c646561";
inline constexpr char semantic_unique_extended_case_manifest_sha256[] =
    "832f94b26165ec1ca3e5ec183aa8b241612bc950a8377c8ac62d7a9e72761056";
inline constexpr char admitted_physical_profile[] =
    "linux-x86_64-glibc;syscall=write;fragments=1;"
    "writev_decoder=bounded-not-live-admitted;max_bytes=65536";

inline std::string manifest_text() {
    return std::string("halofpx-l05z-response-loss-manifest-v1\n") +
        "base_commit\t" + reviewed_base_commit + "\n" +
        "base_tree\t" + reviewed_base_tree + "\n" +
        "case_id\tdedup_key\tsemantic\n" + case_id + "\t" + dedup_key +
        "\t" + semantic_tuple + "\nprofile\t" + admitted_physical_profile + "\n";
}

inline std::string id_set_hash_hex() {
    std::string material("halofpx.l05z.response-loss-id-set.v1");
    material.push_back('\0');
    material += case_id;
    material.push_back('\n');
    return halofpx_l05z_return_hostile_manifest_v1::detail::hex(
        halofpx_l05z_return_hostile_manifest_v1::detail::sha256(material));
}

inline std::string manifest_hash_hex() {
    return halofpx_l05z_return_hostile_manifest_v1::detail::hex(
        halofpx_l05z_return_hostile_manifest_v1::detail::sha256(manifest_text()));
}

inline std::string extended_id_set_hash_hex() {
    std::string material("halofpx.l05z.extended-case-id-set.v1");
    material.push_back('\0');
    material += halofpx_l05z_return_role_authority_v1::
        canonical_case_id_set_sha256;
    material.push_back('\n');
    material += canonical_id_set_sha256;
    material.push_back('\n');
    return halofpx_l05z_return_hostile_manifest_v1::detail::hex(
        halofpx_l05z_return_hostile_manifest_v1::detail::sha256(material));
}

inline std::string extended_manifest_hash_hex() {
    std::string material("halofpx.l05z.extended-case-manifest.v1");
    material.push_back('\0');
    material += halofpx_l05z_return_role_authority_v1::
        canonical_case_manifest_sha256;
    material.push_back('\n');
    material += canonical_manifest_sha256;
    material.push_back('\n');
    return halofpx_l05z_return_hostile_manifest_v1::detail::hex(
        halofpx_l05z_return_hostile_manifest_v1::detail::sha256(material));
}

inline std::string semantic_unique_extended_id_set_hash_hex() {
    std::string material(
        "halofpx.l05z.return-semantic-unique-extended-case-id-set.v1");
    material.push_back('\0');
    material += halofpx_l05z_return_role_authority_v1::
        semantic_unique_case_id_set_sha256;
    material.push_back('\n');
    material += canonical_id_set_sha256;
    material.push_back('\n');
    return halofpx_l05z_return_hostile_manifest_v1::detail::hex(
        halofpx_l05z_return_hostile_manifest_v1::detail::sha256(material));
}

inline std::string semantic_unique_extended_manifest_hash_hex() {
    std::string material(
        "halofpx.l05z.return-semantic-unique-extended-case-manifest.v1");
    material.push_back('\0');
    material += halofpx_l05z_return_role_authority_v1::
        semantic_unique_case_manifest_sha256;
    material.push_back('\n');
    material += canonical_manifest_sha256;
    material.push_back('\n');
    return halofpx_l05z_return_hostile_manifest_v1::detail::hex(
        halofpx_l05z_return_hostile_manifest_v1::detail::sha256(material));
}

inline bool self_check() {
    std::string dedup_material("halofpx.l05z.response-loss-case-dedup.v1");
    dedup_material.push_back('\0');
    dedup_material += semantic_tuple;
    const std::string derived_dedup =
        halofpx_l05z_return_hostile_manifest_v1::detail::hex(
            halofpx_l05z_return_hostile_manifest_v1::detail::sha256(
                dedup_material));
    bool distinct = true;
    for (const auto & item :
            halofpx_l05z_return_role_authority_v1::
                compatibility_execution_case_roster()) {
        if (item.case_id == case_id || item.dedup_key == dedup_key) {
            distinct = false;
        }
    }
    std::array<std::size_t, 17> derived_semantic_extended_shards =
        halofpx_l05z_return_role_authority_v1::
            semantic_unique_case_shard_counts;
    std::array<std::size_t, 17> derived_legacy_extended_shards =
        halofpx_l05z_return_role_authority_v1::canonical_case_shard_counts;
    const std::size_t response_shard =
        halofpx_l05z_return_role_authority_v1::case_shard_for(case_id, 17);
    if (response_shard >= derived_semantic_extended_shards.size() ||
        response_shard >= derived_legacy_extended_shards.size()) return false;
    ++derived_semantic_extended_shards[response_shard];
    ++derived_legacy_extended_shards[response_shard];
    std::size_t semantic_base_sum = 0, semantic_extended_sum = 0;
    std::size_t legacy_base_sum = 0, legacy_extended_sum = 0;
    for (std::size_t index = 0;
         index < derived_semantic_extended_shards.size(); ++index) {
        semantic_base_sum += halofpx_l05z_return_role_authority_v1::
            semantic_unique_case_shard_counts[index];
        semantic_extended_sum += derived_semantic_extended_shards[index];
        legacy_base_sum += halofpx_l05z_return_role_authority_v1::
            canonical_case_shard_counts[index];
        legacy_extended_sum += derived_legacy_extended_shards[index];
    }
    return canonical_case_count == 1 && distinct &&
        halofpx_l05z_return_role_authority_v1::canonical_total_cases ==
            compatible_authority_cases &&
        compatible_authority_cases + canonical_case_count ==
            compatible_extended_execution_total &&
        legacy_base_sum == compatible_authority_cases &&
        legacy_extended_sum == compatible_extended_execution_total &&
        derived_legacy_extended_shards == extended_case_shard_counts &&
        halofpx_l05z_return_role_authority_v1::semantic_unique_total_cases ==
            semantic_unique_authority_cases &&
        semantic_unique_authority_cases + canonical_case_count ==
            semantic_unique_extended_total &&
        semantic_base_sum == semantic_unique_authority_cases &&
        semantic_extended_sum == semantic_unique_extended_total &&
        derived_semantic_extended_shards ==
            semantic_unique_extended_case_shard_counts &&
        response_shard == 7 &&
        std::string_view(case_id) == "L05Z-RSP-LOSS-FULL-001" &&
        derived_dedup == dedup_key &&
        id_set_hash_hex() == canonical_id_set_sha256 &&
        manifest_hash_hex() == canonical_manifest_sha256 &&
        extended_id_set_hash_hex() == extended_case_id_set_sha256 &&
        extended_manifest_hash_hex() == extended_case_manifest_sha256 &&
        semantic_unique_extended_id_set_hash_hex() ==
            semantic_unique_extended_case_id_set_sha256 &&
        semantic_unique_extended_manifest_hash_hex() ==
            semantic_unique_extended_case_manifest_sha256;
}

} // namespace halofpx_l05z_return_response_manifest_v1

#endif
