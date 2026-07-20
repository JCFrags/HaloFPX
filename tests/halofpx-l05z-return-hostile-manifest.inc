#ifndef HALOFPX_L05Z_RETURN_HOSTILE_MANIFEST_INC
#define HALOFPX_L05Z_RETURN_HOSTILE_MANIFEST_INC

// Test-only canonical manifest for the L05z hostile input, object-name, and
// pin/replay qualification cells.  This component has no product linkage and
// deliberately excludes the frozen 2,087 returned-syscall compatibility base.

#include <algorithm>
#include <array>
#include <cstdint>
#include <cstdio>
#include <iomanip>
#include <initializer_list>
#include <limits>
#include <ostream>
#include <set>
#include <sstream>
#include <string>
#include <string_view>
#include <vector>

namespace halofpx_l05z_return_hostile_manifest_v1 {

inline constexpr std::size_t canonical_case_count = 1899;
inline constexpr std::size_t locked_fixture_size = 156;
inline constexpr char locked_fixture_registry_digest[] =
    "5b6ea5887d96bf20c176e2346c2478149a8b7bcf1d84dc98843c28f4e00d0b9e";
inline constexpr char reviewed_base_commit[] =
    "61f2f2d7bc4955e9bca821095ef69125837133b5";
inline constexpr char reviewed_base_tree[] =
    "0a35143f33a7b99a81c824fa8ffd8f743f7ae0dd";
inline constexpr char reviewed_initializer_anchor_sha256[] =
    "a94c08e554ca449e04f1159b1dbd4ff049f00365e3b3cf45feffab8fbb968f82";
inline constexpr char canonical_manifest_sha256[] =
    "5c7a84a0bcf7e2bad3ad98e988d88c898137bee97c1c66023e8bea8a1c15d213";

enum class family : std::uint8_t {
    closed_cbor,
    malformed_length,
    raw_pre_tag_bit,
    tag_bit,
    retagged_semantic,
    digest_pin,
    filename_object,
    collision,
    replay_pin,
    count,
};

enum class stage : std::uint8_t {
    sealed_input_pre_latch,
    final_layout_name_validation,
    temporary_create,
    final_publish,
    replay_pin_policy,
};

enum class injection : std::uint8_t {
    prefix_truncation,
    closed_cbor_grammar,
    declared_length,
    pre_tag_single_bit,
    tag_single_bit,
    retagged_semantic_value,
    digest_or_domain_pin,
    filename_or_object,
    identical_collision,
    unequal_collision,
    replay_or_pin,
};

enum class oracle : std::uint8_t {
    reject_bounded_parse_before_latch,
    reject_sealed_input_before_latch,
    reject_authentication_before_latch,
    reject_semantic_before_latch,
    reject_digest_pin_before_latch,
    reject_name_or_object_and_discard,
    reject_collision_and_discard,
    admit_fresh_primary,
    admit_exact_authenticated_replay,
    reject_same_process_latch,
    reject_replay_pin_mismatch,
    reject_stale_digest,
};

enum class authority_gate : std::uint8_t {
    protected_registry_verifier,
    target_digest_equality_pin,
    target_predecessor_facts_equality,
    target_filename_object_validation,
    target_no_replace_publication,
    target_replay_admission,
    target_process_latch,
};

enum class verifier_status : std::uint8_t {
    not_invoked,
    rejected_not_authenticated,
    structural_rejection,
    authentication_failed,
    authenticated_unadmitted,
    unknown_key,
    key_generation_mismatch,
};

enum class expected_reason : std::uint8_t {
    malformed_cbor,
    malformed_declared_length,
    hostile_pre_tag_mutation,
    authentication_tag_mismatch,
    retagged_semantic_mismatch,
    digest_equality_pin_mismatch,
    filename_or_object_mismatch,
    identical_name_collision,
    unequal_name_collision,
    fresh_primary_admitted,
    exact_authenticated_replay_admitted,
    same_process_latch_set,
    predecessor_facts_mismatch,
    stale_digest_pin_mismatch,
    verifier_unknown_key,
    verifier_key_generation_mismatch,
    registry_id_mismatch,
    registry_epoch_mismatch,
    authority_scope_mismatch,
    policy_commitment_mismatch,
    predecessor_high_water_mismatch,
    key_continuity_mismatch,
};

enum class process_status : std::uint8_t {
    not_run,
    success,
    failure,
};

enum class inventory_class : std::uint8_t {
    pristine_unopened,
    discard_initialization_root,
    exact_qualified_root,
    policy_only,
};

struct family_record {
    family kind;
    const char * name;
    std::size_t count;
    const char * stable_ranges;
    const char * authority_oracle;
};

inline constexpr std::array<family_record,
    static_cast<std::size_t>(family::count)> family_records {{
    { family::closed_cbor, "closed-cbor", 185,
      "HCB-T000..HCB-T155; HCB-TRAIL-00; HCB-OVER-1025; 27 exact HCB grammar IDs",
      "bounded rejection before latch; zero root/fixture syscalls; all L05z facts zero" },
    { family::malformed_length, "malformed-length", 19,
      "14 HLEN field lengths plus HLEN-NONSHORTEST-{RID,KID,AUTHORITY,POLICY,TAG}",
      "bounded rejection before latch; zero root/fixture syscalls; all L05z facts zero" },
    { family::raw_pre_tag_bit, "raw-pre-tag-bit", 992,
      "HRAW-B0000..HRAW-B0991",
      "sealed-input rejection before latch; zero root/fixture syscalls; all L05z facts zero" },
    { family::tag_bit, "tag-bit", 256,
      "HTAG-B0000..HTAG-B0255 (fixture offsets 124..155)",
      "authentication rejection before latch; zero root/fixture syscalls; all L05z facts zero" },
    { family::retagged_semantic, "retagged-semantic", 10,
      "HSEM-001..HSEM-010",
      "semantic/pin rejection before latch despite a recomputed tag" },
    { family::digest_pin, "digest-pin", 261,
      "HDIG-B000..HDIG-B255; HDIG-ZERO; HDIG-DOMAIN-{NONUL,DOUBLENUL,SNAPSHOT,RAW}",
      "verifier authenticates; target-owned digest equality pin rejects before latch" },
    { family::filename_object, "filename-object", 158,
      "HNAME-DIGEST-00..63; HNAME-UPPER-00..22; HNAME-NONHEX-00..63; seven exact form IDs",
      "phase 12; first 19 envelope facts set; reject adoption and discard root" },
    { family::collision, "collision", 4,
      "HCOLL-{TEMP,FINAL}-{IDENTICAL,UNEQUAL}",
      "temporary collision: phase 4/E6; final collision: phase 9/E14; discard root" },
    { family::replay_pin, "replay-pin", 14,
      "six HREPLAY IDs and HPIN-{KEYID,KEYGEN,REGISTRYID,EPOCH,AUTHORITY,POLICY,HIGHWATER,CONTINUITY}",
      "KEYID/KEYGEN use exact verifier statuses; other facts use target equality; fresh exact pin-matched replay admitted" },
}};

struct case_record {
    std::string case_id;
    family family_kind;
    stage semantic_stage;
    injection mutation;
    oracle expected_oracle;
    authority_gate expected_gate;
    verifier_status expected_verifier_status;
    expected_reason reason;
    std::uint8_t expected_phase;
    std::uint32_t expected_envelope_mask;
    bool expected_qualified;
    process_status expected_child_status;
    process_status expected_launcher_status;
    inventory_class expected_inventory;
    std::uint8_t expected_publication_attempts;
    bool requires_zero_storage_syscalls;
    bool requires_unset_latch;
    bool requires_all_l05z_facts_zero;
    std::string dedup_key;
};

inline constexpr std::uint32_t first_bits(unsigned count) {
    return count == 0 ? 0U : count >= 21 ? 0x1fffffU : (1U << count) - 1U;
}

inline const char * family_name(family value) {
    const auto index = static_cast<std::size_t>(value);
    return index < family_records.size() ? family_records[index].name : "invalid";
}

inline const char * stage_name(stage value) {
    switch (value) {
        case stage::sealed_input_pre_latch: return "sealed-input-pre-latch";
        case stage::final_layout_name_validation: return "final-layout-name-validation";
        case stage::temporary_create: return "temporary-create";
        case stage::final_publish: return "final-publish";
        case stage::replay_pin_policy: return "replay-pin-policy";
    }
    return "invalid";
}

inline const char * injection_name(injection value) {
    switch (value) {
        case injection::prefix_truncation: return "prefix-truncation";
        case injection::closed_cbor_grammar: return "closed-cbor-grammar";
        case injection::declared_length: return "declared-length";
        case injection::pre_tag_single_bit: return "pre-tag-single-bit";
        case injection::tag_single_bit: return "tag-single-bit";
        case injection::retagged_semantic_value: return "retagged-semantic-value";
        case injection::digest_or_domain_pin: return "digest-or-domain-pin";
        case injection::filename_or_object: return "filename-or-object";
        case injection::identical_collision: return "identical-collision";
        case injection::unequal_collision: return "unequal-collision";
        case injection::replay_or_pin: return "replay-or-pin";
    }
    return "invalid";
}

inline const char * oracle_name(oracle value) {
    switch (value) {
        case oracle::reject_bounded_parse_before_latch: return "reject-bounded-parse-before-latch";
        case oracle::reject_sealed_input_before_latch: return "reject-sealed-input-before-latch";
        case oracle::reject_authentication_before_latch: return "reject-authentication-before-latch";
        case oracle::reject_semantic_before_latch: return "reject-semantic-before-latch";
        case oracle::reject_digest_pin_before_latch: return "reject-digest-pin-before-latch";
        case oracle::reject_name_or_object_and_discard: return "reject-name-or-object-and-discard";
        case oracle::reject_collision_and_discard: return "reject-collision-and-discard";
        case oracle::admit_fresh_primary: return "admit-fresh-primary";
        case oracle::admit_exact_authenticated_replay: return "admit-exact-authenticated-replay";
        case oracle::reject_same_process_latch: return "reject-same-process-latch";
        case oracle::reject_replay_pin_mismatch: return "reject-replay-pin-mismatch";
        case oracle::reject_stale_digest: return "reject-stale-digest";
    }
    return "invalid";
}

inline const char * authority_gate_name(authority_gate value) {
    switch (value) {
        case authority_gate::protected_registry_verifier: return "protected-registry-verifier";
        case authority_gate::target_digest_equality_pin: return "target-digest-equality-pin";
        case authority_gate::target_predecessor_facts_equality: return "target-predecessor-facts-equality";
        case authority_gate::target_filename_object_validation: return "target-filename-object-validation";
        case authority_gate::target_no_replace_publication: return "target-no-replace-publication";
        case authority_gate::target_replay_admission: return "target-replay-admission";
        case authority_gate::target_process_latch: return "target-process-latch";
    }
    return "invalid";
}

inline const char * verifier_status_name(verifier_status value) {
    switch (value) {
        case verifier_status::not_invoked: return "not-invoked";
        case verifier_status::rejected_not_authenticated: return "rejected-not-authenticated";
        case verifier_status::structural_rejection: return "structural-rejection";
        case verifier_status::authentication_failed: return "authentication-failed";
        case verifier_status::authenticated_unadmitted: return "authenticated-unadmitted";
        case verifier_status::unknown_key: return "unknown-key";
        case verifier_status::key_generation_mismatch: return "key-generation-mismatch";
    }
    return "invalid";
}

inline const char * expected_reason_name(expected_reason value) {
    switch (value) {
        case expected_reason::malformed_cbor: return "malformed-cbor";
        case expected_reason::malformed_declared_length: return "malformed-declared-length";
        case expected_reason::hostile_pre_tag_mutation: return "hostile-pre-tag-mutation";
        case expected_reason::authentication_tag_mismatch: return "authentication-tag-mismatch";
        case expected_reason::retagged_semantic_mismatch: return "retagged-semantic-mismatch";
        case expected_reason::digest_equality_pin_mismatch: return "digest-equality-pin-mismatch";
        case expected_reason::filename_or_object_mismatch: return "filename-or-object-mismatch";
        case expected_reason::identical_name_collision: return "identical-name-collision";
        case expected_reason::unequal_name_collision: return "unequal-name-collision";
        case expected_reason::fresh_primary_admitted: return "fresh-primary-admitted";
        case expected_reason::exact_authenticated_replay_admitted: return "exact-authenticated-replay-admitted";
        case expected_reason::same_process_latch_set: return "same-process-latch-set";
        case expected_reason::predecessor_facts_mismatch: return "predecessor-facts-mismatch";
        case expected_reason::stale_digest_pin_mismatch: return "stale-digest-pin-mismatch";
        case expected_reason::verifier_unknown_key: return "verifier-unknown-key";
        case expected_reason::verifier_key_generation_mismatch: return "verifier-key-generation-mismatch";
        case expected_reason::registry_id_mismatch: return "registry-id-mismatch";
        case expected_reason::registry_epoch_mismatch: return "registry-epoch-mismatch";
        case expected_reason::authority_scope_mismatch: return "authority-scope-mismatch";
        case expected_reason::policy_commitment_mismatch: return "policy-commitment-mismatch";
        case expected_reason::predecessor_high_water_mismatch: return "predecessor-high-water-mismatch";
        case expected_reason::key_continuity_mismatch: return "key-continuity-mismatch";
    }
    return "invalid";
}

inline const char * process_status_name(process_status value) {
    switch (value) {
        case process_status::not_run: return "not-run";
        case process_status::success: return "success";
        case process_status::failure: return "failure";
    }
    return "invalid";
}

inline const char * inventory_name(inventory_class value) {
    switch (value) {
        case inventory_class::pristine_unopened: return "pristine-unopened";
        case inventory_class::discard_initialization_root: return "discard-initialization-root";
        case inventory_class::exact_qualified_root: return "exact-qualified-root";
        case inventory_class::policy_only: return "policy-only";
    }
    return "invalid";
}

namespace detail {

inline std::array<std::uint8_t, 32> sha256(std::string_view input) {
    constexpr std::array<std::uint32_t, 64> constants {{
        0x428a2f98U,0x71374491U,0xb5c0fbcfU,0xe9b5dba5U,0x3956c25bU,0x59f111f1U,0x923f82a4U,0xab1c5ed5U,
        0xd807aa98U,0x12835b01U,0x243185beU,0x550c7dc3U,0x72be5d74U,0x80deb1feU,0x9bdc06a7U,0xc19bf174U,
        0xe49b69c1U,0xefbe4786U,0x0fc19dc6U,0x240ca1ccU,0x2de92c6fU,0x4a7484aaU,0x5cb0a9dcU,0x76f988daU,
        0x983e5152U,0xa831c66dU,0xb00327c8U,0xbf597fc7U,0xc6e00bf3U,0xd5a79147U,0x06ca6351U,0x14292967U,
        0x27b70a85U,0x2e1b2138U,0x4d2c6dfcU,0x53380d13U,0x650a7354U,0x766a0abbU,0x81c2c92eU,0x92722c85U,
        0xa2bfe8a1U,0xa81a664bU,0xc24b8b70U,0xc76c51a3U,0xd192e819U,0xd6990624U,0xf40e3585U,0x106aa070U,
        0x19a4c116U,0x1e376c08U,0x2748774cU,0x34b0bcb5U,0x391c0cb3U,0x4ed8aa4aU,0x5b9cca4fU,0x682e6ff3U,
        0x748f82eeU,0x78a5636fU,0x84c87814U,0x8cc70208U,0x90befffaU,0xa4506cebU,0xbef9a3f7U,0xc67178f2U,
    }};
    const auto rotate = [](std::uint32_t value, unsigned shift) {
        return (value >> shift) | (value << (32U - shift));
    };
    std::vector<std::uint8_t> padded(input.begin(), input.end());
    padded.push_back(0x80U);
    while ((padded.size() % 64U) != 56U) padded.push_back(0U);
    const std::uint64_t bits = static_cast<std::uint64_t>(input.size()) * 8U;
    for (unsigned index = 0; index < 8; ++index) {
        padded.push_back(static_cast<std::uint8_t>(bits >> (56U - 8U * index)));
    }
    std::array<std::uint32_t, 8> state {{
        0x6a09e667U,0xbb67ae85U,0x3c6ef372U,0xa54ff53aU,
        0x510e527fU,0x9b05688cU,0x1f83d9abU,0x5be0cd19U,
    }};
    for (std::size_t block = 0; block < padded.size(); block += 64U) {
        std::array<std::uint32_t, 64> words {};
        for (unsigned index = 0; index < 16; ++index) {
            const std::size_t offset = block + 4U * index;
            words[index] = (static_cast<std::uint32_t>(padded[offset]) << 24U) |
                (static_cast<std::uint32_t>(padded[offset + 1]) << 16U) |
                (static_cast<std::uint32_t>(padded[offset + 2]) << 8U) |
                static_cast<std::uint32_t>(padded[offset + 3]);
        }
        for (unsigned index = 16; index < 64; ++index) {
            const auto x = words[index - 15];
            const auto y = words[index - 2];
            const std::uint32_t s0 = rotate(x, 7) ^ rotate(x, 18) ^ (x >> 3U);
            const std::uint32_t s1 = rotate(y, 17) ^ rotate(y, 19) ^ (y >> 10U);
            words[index] = words[index - 16] + s0 + words[index - 7] + s1;
        }
        auto a=state[0], b=state[1], c=state[2], d=state[3];
        auto e=state[4], f=state[5], g=state[6], h=state[7];
        for (unsigned index = 0; index < 64; ++index) {
            const std::uint32_t s1 = rotate(e,6)^rotate(e,11)^rotate(e,25);
            const std::uint32_t choice = (e & f) ^ (~e & g);
            const std::uint32_t temp1 = h+s1+choice+constants[index]+words[index];
            const std::uint32_t s0 = rotate(a,2)^rotate(a,13)^rotate(a,22);
            const std::uint32_t majority = (a & b) ^ (a & c) ^ (b & c);
            const std::uint32_t temp2 = s0 + majority;
            h=g; g=f; f=e; e=d+temp1; d=c; c=b; b=a; a=temp1+temp2;
        }
        state[0]+=a; state[1]+=b; state[2]+=c; state[3]+=d;
        state[4]+=e; state[5]+=f; state[6]+=g; state[7]+=h;
    }
    std::array<std::uint8_t, 32> output {};
    for (unsigned index = 0; index < output.size(); ++index) {
        output[index] = static_cast<std::uint8_t>(
            state[index / 4U] >> (24U - 8U * (index % 4U)));
    }
    std::fill(padded.begin(), padded.end(), 0U);
    return output;
}

inline std::string hex(const std::array<std::uint8_t, 32> & digest) {
    constexpr char digits[] = "0123456789abcdef";
    std::string output(64, '0');
    for (std::size_t index = 0; index < digest.size(); ++index) {
        output[2 * index] = digits[digest[index] >> 4U];
        output[2 * index + 1] = digits[digest[index] & 0x0fU];
    }
    return output;
}

inline std::string domain_hash(const char * domain, std::string_view value) {
    std::string material(domain);
    material.push_back('\0');
    material.append(value.data(), value.size());
    return hex(sha256(material));
}

inline case_record make_case(std::string id, family family_kind,
                             stage semantic_stage, injection mutation,
                             oracle expected_oracle, authority_gate expected_gate,
                             verifier_status expected_status,
                             expected_reason reason, std::uint8_t phase,
                             std::uint32_t mask, bool qualified,
                             process_status child, process_status launcher,
                             inventory_class inventory,
                             std::uint8_t publication_attempts,
                             bool zero_storage, bool unset_latch,
                             bool zero_facts) {
    std::string semantics = id;
    semantics.push_back('|');
    semantics += family_name(family_kind);
    semantics.push_back('|');
    semantics += stage_name(semantic_stage);
    semantics.push_back('|');
    semantics += injection_name(mutation);
    semantics.push_back('|');
    semantics += oracle_name(expected_oracle);
    semantics.push_back('|');
    semantics += authority_gate_name(expected_gate);
    semantics.push_back('|');
    semantics += verifier_status_name(expected_status);
    semantics.push_back('|');
    semantics += expected_reason_name(reason);
    semantics.push_back('|');
    semantics += std::to_string(phase);
    semantics.push_back('|');
    semantics += std::to_string(mask);
    return { std::move(id), family_kind, semantic_stage, mutation,
             expected_oracle, expected_gate, expected_status, reason,
             phase, mask, qualified, child, launcher,
             inventory, publication_attempts, zero_storage, unset_latch,
             zero_facts, domain_hash("halofpx.l05z.case-dedup.v1", semantics) };
}

inline std::vector<case_record> build_manifest() {
    std::vector<case_record> output;
    output.reserve(canonical_case_count);
    const auto pre_latch = [&output](std::string id, family family_kind,
                                    injection mutation, oracle expected) {
        const authority_gate gate = family_kind == family::digest_pin
            ? authority_gate::target_digest_equality_pin
            : authority_gate::protected_registry_verifier;
        verifier_status status = verifier_status::structural_rejection;
        expected_reason reason = expected_reason::malformed_cbor;
        switch (family_kind) {
            case family::malformed_length:
                reason = expected_reason::malformed_declared_length;
                break;
            case family::raw_pre_tag_bit:
                status = verifier_status::rejected_not_authenticated;
                reason = expected_reason::hostile_pre_tag_mutation;
                break;
            case family::tag_bit:
                status = verifier_status::authentication_failed;
                reason = expected_reason::authentication_tag_mismatch;
                break;
            case family::retagged_semantic:
                reason = expected_reason::retagged_semantic_mismatch;
                break;
            case family::digest_pin:
                status = verifier_status::authenticated_unadmitted;
                reason = expected_reason::digest_equality_pin_mismatch;
                break;
            default:
                break;
        }
        output.push_back(make_case(std::move(id), family_kind,
            stage::sealed_input_pre_latch, mutation, expected, gate, status,
            reason, 0, 0, false,
            process_status::not_run, process_status::not_run,
            inventory_class::pristine_unopened, 0, true, true, true));
    };
    const auto numbered = [](const char * prefix, unsigned first,
                             unsigned count, unsigned width) {
        std::vector<std::string> ids;
        ids.reserve(count);
        for (unsigned index = first; index < first + count; ++index) {
            char buffer[64] {};
            (void) std::snprintf(buffer, sizeof(buffer), "%s%0*u", prefix,
                                 static_cast<int>(width), index);
            ids.emplace_back(buffer);
        }
        return ids;
    };

    for (auto & id : numbered("HCB-T", 0, 156, 3)) {
        pre_latch(std::move(id), family::closed_cbor,
            injection::prefix_truncation,
            oracle::reject_bounded_parse_before_latch);
    }
    for (const char * id : { "HCB-TRAIL-00", "HCB-OVER-1025" }) {
        pre_latch(id, family::closed_cbor, injection::closed_cbor_grammar,
            oracle::reject_bounded_parse_before_latch);
    }
    for (const char * context : { "OUTER", "AUTH", "BODY" }) {
        for (const char * value : { "LOW", "HIGH" }) {
            pre_latch(std::string("HCB-MAPCOUNT-") + context + "-" + value,
                family::closed_cbor, injection::closed_cbor_grammar,
                oracle::reject_bounded_parse_before_latch);
        }
    }
    for (const char * operation : { "INDEF", "MAPWIDTH", "DUP", "UNKNOWN",
                                    "OMIT", "ORDER", "KEYWIDTH" }) {
        for (const char * context : { "OUTER", "AUTH", "BODY" }) {
            pre_latch(std::string("HCB-") + operation + "-" + context,
                family::closed_cbor, injection::closed_cbor_grammar,
                oracle::reject_bounded_parse_before_latch);
        }
    }

    const auto add_lengths = [&pre_latch](const char * field,
                                         std::initializer_list<unsigned> values) {
        for (const unsigned value : values) {
            pre_latch(std::string("HLEN-") + field + "-" + std::to_string(value),
                family::malformed_length, injection::declared_length,
                oracle::reject_bounded_parse_before_latch);
        }
    };
    add_lengths("RID", { 0, 10, 12, 129 });
    add_lengths("KID", { 0, 15, 17, 129 });
    add_lengths("AUTHORITY", { 31, 33 });
    add_lengths("POLICY", { 31, 33 });
    add_lengths("TAG", { 31, 33 });
    for (const char * field : { "RID", "KID", "AUTHORITY", "POLICY", "TAG" }) {
        pre_latch(std::string("HLEN-NONSHORTEST-") + field,
            family::malformed_length, injection::declared_length,
            oracle::reject_bounded_parse_before_latch);
    }
    for (auto & id : numbered("HRAW-B", 0, 992, 4)) {
        pre_latch(std::move(id), family::raw_pre_tag_bit,
            injection::pre_tag_single_bit,
            oracle::reject_sealed_input_before_latch);
    }
    for (auto & id : numbered("HTAG-B", 0, 256, 4)) {
        pre_latch(std::move(id), family::tag_bit, injection::tag_single_bit,
            oracle::reject_authentication_before_latch);
    }
    for (auto & id : numbered("HSEM-", 1, 10, 3)) {
        pre_latch(std::move(id), family::retagged_semantic,
            injection::retagged_semantic_value,
            oracle::reject_semantic_before_latch);
    }
    for (auto & id : numbered("HDIG-B", 0, 256, 3)) {
        pre_latch(std::move(id), family::digest_pin,
            injection::digest_or_domain_pin,
            oracle::reject_digest_pin_before_latch);
    }
    for (const char * suffix : { "ZERO", "DOMAIN-NONUL", "DOMAIN-DOUBLENUL",
                                "DOMAIN-SNAPSHOT", "DOMAIN-RAW" }) {
        pre_latch(std::string("HDIG-") + suffix, family::digest_pin,
            injection::digest_or_domain_pin,
            oracle::reject_digest_pin_before_latch);
    }

    const auto name_case = [&output](std::string id) {
        output.push_back(make_case(std::move(id), family::filename_object,
            stage::final_layout_name_validation, injection::filename_or_object,
            oracle::reject_name_or_object_and_discard,
            authority_gate::target_filename_object_validation,
            verifier_status::authenticated_unadmitted,
            expected_reason::filename_or_object_mismatch,
            12, first_bits(19), false,
            process_status::failure, process_status::failure,
            inventory_class::discard_initialization_root, 1,
            false, false, false));
    };
    for (auto & id : numbered("HNAME-DIGEST-", 0, 64, 2)) name_case(std::move(id));
    for (auto & id : numbered("HNAME-UPPER-", 0, 23, 2)) name_case(std::move(id));
    for (auto & id : numbered("HNAME-NONHEX-", 0, 64, 2)) name_case(std::move(id));
    for (const char * suffix : { "WIDTH-63", "WIDTH-65", "PREFIX-UPPER",
                                "PREFIX-UNDERSCORE", "SUFFIX-UPPER",
                                "SUFFIX-SHORT", "SUFFIX-LONG" }) {
        name_case(std::string("HNAME-") + suffix);
    }

    const auto collision_case = [&output](const char * id, stage collision_stage,
                                          injection collision_injection,
                                          std::uint8_t phase, unsigned facts) {
        output.push_back(make_case(id, family::collision, collision_stage,
            collision_injection, oracle::reject_collision_and_discard,
            authority_gate::target_no_replace_publication,
            verifier_status::authenticated_unadmitted,
            collision_injection == injection::identical_collision
                ? expected_reason::identical_name_collision
                : expected_reason::unequal_name_collision,
            phase, first_bits(facts), false, process_status::failure,
            process_status::failure, inventory_class::discard_initialization_root,
            collision_stage == stage::final_publish ? 1 : 0,
            false, false, false));
    };
    collision_case("HCOLL-TEMP-IDENTICAL", stage::temporary_create,
                   injection::identical_collision, 4, 6);
    collision_case("HCOLL-TEMP-UNEQUAL", stage::temporary_create,
                   injection::unequal_collision, 4, 6);
    collision_case("HCOLL-FINAL-IDENTICAL", stage::final_publish,
                   injection::identical_collision, 9, 14);
    collision_case("HCOLL-FINAL-UNEQUAL", stage::final_publish,
                   injection::unequal_collision, 9, 14);

    const auto policy_case = [&output](std::string id, oracle expected,
                                      bool qualified, inventory_class inventory,
                                      bool rejected_before_latch = true) {
        const bool admitted = expected == oracle::admit_fresh_primary ||
            expected == oracle::admit_exact_authenticated_replay;
        authority_gate gate = authority_gate::target_predecessor_facts_equality;
        verifier_status status = verifier_status::authenticated_unadmitted;
        expected_reason reason = expected_reason::predecessor_facts_mismatch;
        if (expected == oracle::admit_fresh_primary) {
            gate = authority_gate::target_replay_admission;
            reason = expected_reason::fresh_primary_admitted;
        } else if (expected == oracle::admit_exact_authenticated_replay) {
            gate = authority_gate::target_replay_admission;
            reason = expected_reason::exact_authenticated_replay_admitted;
        } else if (expected == oracle::reject_same_process_latch) {
            gate = authority_gate::target_process_latch;
            status = verifier_status::not_invoked;
            reason = expected_reason::same_process_latch_set;
        } else if (expected == oracle::reject_stale_digest) {
            gate = authority_gate::target_digest_equality_pin;
            reason = expected_reason::stale_digest_pin_mismatch;
        } else if (id == "HPIN-KEYID") {
            gate = authority_gate::protected_registry_verifier;
            status = verifier_status::unknown_key;
            reason = expected_reason::verifier_unknown_key;
        } else if (id == "HPIN-KEYGEN") {
            gate = authority_gate::protected_registry_verifier;
            status = verifier_status::key_generation_mismatch;
            reason = expected_reason::verifier_key_generation_mismatch;
        } else if (id == "HPIN-REGISTRYID") {
            reason = expected_reason::registry_id_mismatch;
        } else if (id == "HPIN-EPOCH") {
            reason = expected_reason::registry_epoch_mismatch;
        } else if (id == "HPIN-AUTHORITY") {
            reason = expected_reason::authority_scope_mismatch;
        } else if (id == "HPIN-POLICY") {
            reason = expected_reason::policy_commitment_mismatch;
        } else if (id == "HPIN-HIGHWATER") {
            reason = expected_reason::predecessor_high_water_mismatch;
        } else if (id == "HPIN-CONTINUITY") {
            reason = expected_reason::key_continuity_mismatch;
        }
        output.push_back(make_case(std::move(id), family::replay_pin,
            stage::replay_pin_policy, injection::replay_or_pin, expected,
            gate, status, reason,
            admitted ? 13 : 0, admitted ? first_bits(21) : 0,
            qualified, admitted ? process_status::success : process_status::not_run,
            admitted ? process_status::success : process_status::not_run,
            inventory, admitted ? 1 : 0,
            !admitted && rejected_before_latch,
            !admitted && rejected_before_latch,
            !admitted && rejected_before_latch));
    };
    policy_case("HREPLAY-FRESH-PRIMARY", oracle::admit_fresh_primary, true,
                inventory_class::exact_qualified_root);
    policy_case("HREPLAY-FRESH-EXACT", oracle::admit_exact_authenticated_replay,
                true, inventory_class::exact_qualified_root);
    policy_case("HREPLAY-SAME-PROCESS", oracle::reject_same_process_latch,
                false, inventory_class::policy_only, false);
    policy_case("HREPLAY-OLD-PIN-MISMATCH", oracle::reject_replay_pin_mismatch,
                false, inventory_class::pristine_unopened);
    policy_case("HREPLAY-OLD-PIN-MATCH", oracle::admit_exact_authenticated_replay,
                true, inventory_class::exact_qualified_root);
    policy_case("HREPLAY-OLD-DIGEST-STALE", oracle::reject_stale_digest,
                false, inventory_class::pristine_unopened);
    for (const char * suffix : { "KEYID", "KEYGEN", "REGISTRYID", "EPOCH",
                                "AUTHORITY", "POLICY", "HIGHWATER", "CONTINUITY" }) {
        policy_case(std::string("HPIN-") + suffix,
            oracle::reject_replay_pin_mismatch, false,
            inventory_class::pristine_unopened);
    }

    std::sort(output.begin(), output.end(), [](const case_record & left,
                                                const case_record & right) {
        return left.case_id < right.case_id;
    });
    return output;
}

} // namespace detail

inline const std::vector<case_record> & manifest() {
    static const std::vector<case_record> value = detail::build_manifest();
    return value;
}

inline const case_record * select_case(std::string_view case_id) {
    const auto & values = manifest();
    const auto found = std::lower_bound(values.begin(), values.end(), case_id,
        [](const case_record & item, std::string_view id) {
            return item.case_id < id;
        });
    return found != values.end() && found->case_id == case_id ? &*found : nullptr;
}

inline std::uint32_t shard_for(std::string_view case_id,
                               std::uint32_t shard_count) {
    if (shard_count == 0) return std::numeric_limits<std::uint32_t>::max();
    std::string material("halofpx.l05z.case-shard.v1");
    material.push_back('\0');
    material.append(case_id.data(), case_id.size());
    const auto digest = detail::sha256(material);
    std::uint32_t remainder = 0;
    for (const auto byte : digest) {
        remainder = static_cast<std::uint32_t>(
            (static_cast<std::uint64_t>(remainder) * 256U + byte) % shard_count);
    }
    return remainder;
}

inline std::vector<const case_record *> cases_for_shard(
        std::uint32_t shard_index, std::uint32_t shard_count) {
    std::vector<const case_record *> output;
    if (shard_count == 0 || shard_index >= shard_count) return output;
    for (const auto & item : manifest()) {
        if (shard_for(item.case_id, shard_count) == shard_index) {
            output.push_back(&item);
        }
    }
    return output;
}

inline void emit_manifest(std::ostream & output) {
    output << "halofpx-l05z-hostile-manifest-v1\n"
           << "base_commit\t" << reviewed_base_commit << '\n'
           << "base_tree\t" << reviewed_base_tree << '\n'
           << "reviewed_anchor_sha256\t" << reviewed_initializer_anchor_sha256 << '\n'
           << "fixture_size\t" << locked_fixture_size << '\n'
           << "fixture_registry_digest\t" << locked_fixture_registry_digest << '\n'
           << "case_id\tfamily\tstage\tinjection\toracle\tauthority_gate"
              "\tverifier_status\treason\tphase\tmask_hex\tqualified"
              "\tchild\tlauncher\tinventory\tpublication_attempts\tzero_storage_syscalls"
              "\tunset_latch\tzero_l05z_facts\tdedup_key\n";
    for (const auto & item : manifest()) {
        std::ostringstream mask;
        mask << std::hex << std::setw(6) << std::setfill('0')
             << item.expected_envelope_mask;
        output << item.case_id << '\t' << family_name(item.family_kind) << '\t'
               << stage_name(item.semantic_stage) << '\t'
               << injection_name(item.mutation) << '\t'
               << oracle_name(item.expected_oracle) << '\t'
               << authority_gate_name(item.expected_gate) << '\t'
               << verifier_status_name(item.expected_verifier_status) << '\t'
               << expected_reason_name(item.reason) << '\t'
               << static_cast<unsigned>(item.expected_phase) << '\t'
               << mask.str() << '\t' << (item.expected_qualified ? 1 : 0) << '\t'
               << process_status_name(item.expected_child_status) << '\t'
               << process_status_name(item.expected_launcher_status) << '\t'
               << inventory_name(item.expected_inventory) << '\t'
               << static_cast<unsigned>(item.expected_publication_attempts) << '\t'
               << (item.requires_zero_storage_syscalls ? 1 : 0) << '\t'
               << (item.requires_unset_latch ? 1 : 0) << '\t'
               << (item.requires_all_l05z_facts_zero ? 1 : 0) << '\t'
               << item.dedup_key << '\n';
    }
}

inline std::string manifest_text() {
    std::ostringstream output;
    emit_manifest(output);
    return output.str();
}

inline std::array<std::uint8_t, 32> manifest_hash() {
    return detail::sha256(manifest_text());
}

inline std::string manifest_hash_hex() {
    return detail::hex(manifest_hash());
}

inline std::string sorted_id_set_hash_hex() {
    std::string serialized("halofpx.l05z.hostile-input-manifest.v1");
    serialized.push_back('\0');
    for (const auto & item : manifest()) {
        serialized += item.case_id;
        serialized.push_back('\n');
    }
    return detail::hex(detail::sha256(serialized));
}

inline bool shard_union_self_check(std::uint32_t shard_count) {
    if (shard_count == 0) return false;
    std::set<std::string> union_ids;
    std::size_t observed = 0;
    for (std::uint32_t shard = 0; shard < shard_count; ++shard) {
        const auto selected = cases_for_shard(shard, shard_count);
        for (const auto * item : selected) {
            if (item == nullptr || shard_for(item->case_id, shard_count) != shard ||
                !union_ids.insert(item->case_id).second) return false;
            ++observed;
        }
    }
    return observed == manifest().size() && union_ids.size() == manifest().size();
}

inline bool self_check() {
    const auto & values = manifest();
    if (values.size() != canonical_case_count || values.empty() ||
        !std::is_sorted(values.begin(), values.end(),
            [](const case_record & left, const case_record & right) {
                return left.case_id < right.case_id;
            })) return false;

    std::array<std::size_t, static_cast<std::size_t>(family::count)> counts {};
    std::set<std::string> ids;
    std::set<std::string> dedup_keys;
    for (const auto & item : values) {
        const auto family_index = static_cast<std::size_t>(item.family_kind);
        if (family_index >= counts.size() || item.case_id.empty() ||
            item.dedup_key.size() != 64 || !ids.insert(item.case_id).second ||
            !dedup_keys.insert(item.dedup_key).second ||
            select_case(item.case_id) != &item ||
            (item.expected_envelope_mask & ~first_bits(21)) != 0) return false;
        ++counts[family_index];
        switch (item.family_kind) {
            case family::closed_cbor:
                if (item.expected_gate != authority_gate::protected_registry_verifier ||
                    item.expected_verifier_status != verifier_status::structural_rejection ||
                    item.reason != expected_reason::malformed_cbor) return false;
                break;
            case family::malformed_length:
                if (item.expected_gate != authority_gate::protected_registry_verifier ||
                    item.expected_verifier_status != verifier_status::structural_rejection ||
                    item.reason != expected_reason::malformed_declared_length) return false;
                break;
            case family::raw_pre_tag_bit:
                if (item.expected_gate != authority_gate::protected_registry_verifier ||
                    item.expected_verifier_status != verifier_status::rejected_not_authenticated ||
                    item.reason != expected_reason::hostile_pre_tag_mutation) return false;
                break;
            case family::tag_bit:
                if (item.expected_gate != authority_gate::protected_registry_verifier ||
                    item.expected_verifier_status != verifier_status::authentication_failed ||
                    item.reason != expected_reason::authentication_tag_mismatch) return false;
                break;
            case family::retagged_semantic:
                if (item.expected_gate != authority_gate::protected_registry_verifier ||
                    item.expected_verifier_status != verifier_status::structural_rejection ||
                    item.reason != expected_reason::retagged_semantic_mismatch) return false;
                break;
            case family::digest_pin:
                if (item.expected_gate != authority_gate::target_digest_equality_pin ||
                    item.expected_verifier_status != verifier_status::authenticated_unadmitted ||
                    item.reason != expected_reason::digest_equality_pin_mismatch) return false;
                break;
            case family::filename_object:
                if (item.expected_gate != authority_gate::target_filename_object_validation ||
                    item.expected_verifier_status != verifier_status::authenticated_unadmitted ||
                    item.reason != expected_reason::filename_or_object_mismatch) return false;
                break;
            case family::collision:
                if (item.expected_gate != authority_gate::target_no_replace_publication ||
                    item.expected_verifier_status != verifier_status::authenticated_unadmitted ||
                    (item.mutation == injection::identical_collision
                        ? item.reason != expected_reason::identical_name_collision
                        : item.reason != expected_reason::unequal_name_collision)) return false;
                break;
            case family::replay_pin:
            case family::count:
                break;
        }
    }
    for (std::size_t index = 0; index < family_records.size(); ++index) {
        if (counts[index] != family_records[index].count) return false;
    }
    if (select_case("") != nullptr || select_case("HCB-T156") != nullptr ||
        sorted_id_set_hash_hex() !=
            "1028ac1be238cd418e48699c509f27a0dfcded792869c4014a9f5f6d9a8e8698" ||
        manifest_hash_hex() != canonical_manifest_sha256) {
        return false;
    }
    const auto exact_policy = [](const char * id, authority_gate gate,
                                 verifier_status status,
                                 expected_reason reason) {
        const auto * item = select_case(id);
        return item && item->family_kind == family::replay_pin &&
            item->expected_gate == gate &&
            item->expected_verifier_status == status && item->reason == reason;
    };
    if (!exact_policy("HPIN-KEYID", authority_gate::protected_registry_verifier,
                      verifier_status::unknown_key,
                      expected_reason::verifier_unknown_key) ||
        !exact_policy("HPIN-KEYGEN", authority_gate::protected_registry_verifier,
                      verifier_status::key_generation_mismatch,
                      expected_reason::verifier_key_generation_mismatch) ||
        !exact_policy("HPIN-REGISTRYID", authority_gate::target_predecessor_facts_equality,
                      verifier_status::authenticated_unadmitted,
                      expected_reason::registry_id_mismatch) ||
        !exact_policy("HPIN-EPOCH", authority_gate::target_predecessor_facts_equality,
                      verifier_status::authenticated_unadmitted,
                      expected_reason::registry_epoch_mismatch) ||
        !exact_policy("HPIN-AUTHORITY", authority_gate::target_predecessor_facts_equality,
                      verifier_status::authenticated_unadmitted,
                      expected_reason::authority_scope_mismatch) ||
        !exact_policy("HPIN-POLICY", authority_gate::target_predecessor_facts_equality,
                      verifier_status::authenticated_unadmitted,
                      expected_reason::policy_commitment_mismatch) ||
        !exact_policy("HPIN-HIGHWATER", authority_gate::target_predecessor_facts_equality,
                      verifier_status::authenticated_unadmitted,
                      expected_reason::predecessor_high_water_mismatch) ||
        !exact_policy("HPIN-CONTINUITY", authority_gate::target_predecessor_facts_equality,
                      verifier_status::authenticated_unadmitted,
                      expected_reason::key_continuity_mismatch) ||
        !exact_policy("HREPLAY-FRESH-PRIMARY", authority_gate::target_replay_admission,
                      verifier_status::authenticated_unadmitted,
                      expected_reason::fresh_primary_admitted) ||
        !exact_policy("HREPLAY-FRESH-EXACT", authority_gate::target_replay_admission,
                      verifier_status::authenticated_unadmitted,
                      expected_reason::exact_authenticated_replay_admitted) ||
        !exact_policy("HREPLAY-OLD-PIN-MATCH", authority_gate::target_replay_admission,
                      verifier_status::authenticated_unadmitted,
                      expected_reason::exact_authenticated_replay_admitted) ||
        !exact_policy("HREPLAY-SAME-PROCESS", authority_gate::target_process_latch,
                      verifier_status::not_invoked,
                      expected_reason::same_process_latch_set) ||
        !exact_policy("HREPLAY-OLD-PIN-MISMATCH",
                      authority_gate::target_predecessor_facts_equality,
                      verifier_status::authenticated_unadmitted,
                      expected_reason::predecessor_facts_mismatch) ||
        !exact_policy("HREPLAY-OLD-DIGEST-STALE",
                      authority_gate::target_digest_equality_pin,
                      verifier_status::authenticated_unadmitted,
                      expected_reason::stale_digest_pin_mismatch)) return false;
    for (std::uint32_t shard_count = 1; shard_count <= 64; ++shard_count) {
        if (!shard_union_self_check(shard_count)) return false;
    }
    return true;
}

} // namespace halofpx_l05z_return_hostile_manifest_v1

#endif // HALOFPX_L05Z_RETURN_HOSTILE_MANIFEST_INC
