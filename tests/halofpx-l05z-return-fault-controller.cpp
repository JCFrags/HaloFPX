





#if !defined(__linux__) || !defined(__x86_64__)
#error "The L05z predecessor-envelope returned-fault controller requires Linux x86-64"
#endif

// Reuse only the already reviewed L05x process, path, receipt, and fd-identity
// inspection primitives.  Its entry point is renamed; this controller neither
// calls the L05x controller nor weakens its exact directory-prefix checks.
int halofpx_l05z_imported_crash_controller_main(int, char **);
#define main halofpx_l05z_imported_crash_controller_main
#include "halofpx-l05x-ptrace-controller.cpp"
#undef main

#include <linux/fs.h>
#include <linux/btrfs.h>
#include "halofpx-context-store-protected-registry.h"
#include "halofpx-context-store-registry-lab-linux-initializer-internal.h"
#include "halofpx-l05z-return-hostile-manifest.inc"
#include "halofpx-l05z-return-role-authority-manifest.inc"
#include "halofpx-l05z-return-role-map-v1.inc"
#include "halofpx-l05z-return-response-manifest.inc"
#include <climits>
#include <fstream>
#include <sstream>
#include <sys/statfs.h>
#include <sys/uio.h>
#include <sys/user.h>

namespace {
namespace envelope_fault {

enum class boundary {
    step4_open,
    step4_close,
    transient_open,
    transient_fchmod,
    envelope_pwrite,
    envelope_pread,
    envelope_pread_eof,
    envelope_fsync,
    readonly_temp_open,
    envelope_rename,
    envelopes_fsync,
    staging_fsync,
    final_open,
    final_pread,
    final_pread_eof,
    marker_pread,
    marker_pread_eof,
    envelope_close,
    staging_close,
    writer_unlock,
    fixture_unlock,
    reserve_revalidation,
    final_validation,
    fstat_call,
    statx_call,
    readlink_call,
    getdents_call,
    facts_call,
    fstatfs_call,
    stdout_audit_response,
};

enum class mode {
    pre_error,
    late_error,
    eintr_once,
    short_pre,
    short_late,
    zero_pre,
    zero_late,
    response_loss_full,
    corrupt,
    truncate,
    append,
    unexpected_name,
    hardlink,
    symlink,
    inode_substitute,
    temp_inode_substitute,
    collision,
    reserve_loss,
};

struct arguments {
    const char * target = nullptr;
    const char * golden = nullptr;
    const char * parent = nullptr;
    const char * root = nullptr;
    const char * fixture = nullptr;
    const char * receipt = nullptr;
    boundary point = boundary::transient_open;
    mode injection = mode::pre_error;
    int returned_errno = EIO;
    unsigned occurrence = 1;
    unsigned short_count = 1;
    bool point_set = false;
    bool mode_set = false;
    bool errno_set = false;
    bool occurrence_set = false;
    bool short_set = false;
};

struct fault_state {
    bool pending_replacement = false;
    bool first_replaced = false;
    bool retry_seen = false;
    bool retry_succeeded = false;
    bool retry_window_open = false;
    bool mutation_applied = false;
    bool selected_signature_valid = false;
    std::uint64_t selected_nr = 0;
    std::array<std::uint64_t, 6> selected_args {};
    unsigned matches = 0;
};

inline constexpr std::size_t response_max_bytes = 65536;

struct response_span {
    std::uint64_t address = 0;
    std::uint64_t length = 0;
};

struct response_state {
    bool pending_replacement = false;
    bool suppressed = false;
    pid_t pending_pid = -1;
    std::uint64_t syscall_nr = 0;
    std::uint64_t requested = 0;
    unsigned fragments = 0;
    std::string transcript;
};

bool response_span_plan(const response_span * spans, std::size_t count,
                        std::size_t & aggregate) {
    aggregate = 0;
    if (spans == nullptr || count == 0 || count > IOV_MAX) return false;
    for (std::size_t index = 0; index < count; ++index) {
        const std::uint64_t length = spans[index].length;
        if ((length != 0 && spans[index].address == 0) ||
            (length != 0 && spans[index].address >
                std::numeric_limits<std::uint64_t>::max() - (length - 1)) ||
            length > response_max_bytes ||
            aggregate > response_max_bytes - static_cast<std::size_t>(length)) {
            return false;
        }
        aggregate += static_cast<std::size_t>(length);
    }
    return aggregate != 0;
}

bool response_iovec_table_shape(std::uint64_t address, std::uint64_t raw_count,
                                std::size_t & count, std::size_t & bytes) {
    count = 0;
    bytes = 0;
    if (address == 0 || raw_count == 0 || raw_count > IOV_MAX ||
        raw_count > std::numeric_limits<std::size_t>::max() /
            sizeof(struct iovec)) return false;
    count = static_cast<std::size_t>(raw_count);
    bytes = count * sizeof(struct iovec);
    return address <= std::numeric_limits<std::uint64_t>::max() - (bytes - 1);
}

bool response_selector(pid_t pid, pid_t live_child, pid_t launcher,
                       std::uint64_t fd, bool exact_pipe,
                       unsigned prior_fragments) {
    return pid == live_child && pid != launcher && live_child > 0 && fd == 1 &&
        exact_pipe && prior_fragments == 0;
}

bool response_exit_shape(const response_state & response, pid_t pid,
                         std::uint64_t syscall_nr, bool is_error,
                         long long result) {
    return response.pending_replacement && pid == response.pending_pid &&
        syscall_nr == response.syscall_nr && is_error && result == -ENOSYS &&
        response.requested != 0 &&
        response.requested <= static_cast<std::uint64_t>(SSIZE_MAX);
}

bool response_consumer_shape(std::size_t delivered, bool eof) {
    return delivered == 0 && eof;
}

bool response_decoder_self_check() {
    std::size_t aggregate = 0;
    std::size_t iovec_count = 0, iovec_bytes = 0;
    const response_span valid_write[] { { 0x1000, 17 } };
    const response_span valid_writev[] {
        { 0x1000, 7 }, { 0, 0 }, { 0x2000, 11 },
    };
    const response_span null_nonempty[] { { 0, 1 } };
    const response_span overrun[] {
        { 0x1000, response_max_bytes }, { 0x2000, 1 },
    };
    const response_span exact_max[] { { 1, response_max_bytes } };
    const response_span too_large[] { { 1, response_max_bytes + 1 } };
    const response_span address_wrap[] {
        { std::numeric_limits<std::uint64_t>::max(), 2 },
    };
    const response_span zero[] { { 0, 0 } };
    response_state exit {};
    exit.pending_replacement = true;
    exit.pending_pid = 41;
    exit.syscall_nr = SYS_write;
    exit.requested = 17;
    return response_span_plan(valid_write, 1, aggregate) && aggregate == 17 &&
        response_span_plan(valid_writev, 3, aggregate) && aggregate == 18 &&
        response_span_plan(exact_max, 1, aggregate) &&
        aggregate == response_max_bytes &&
        !response_span_plan(nullptr, 1, aggregate) &&
        !response_span_plan(valid_write, 0, aggregate) &&
        !response_span_plan(valid_write, static_cast<std::size_t>(IOV_MAX) + 1,
                            aggregate) &&
        !response_span_plan(null_nonempty, 1, aggregate) &&
        !response_span_plan(overrun, 2, aggregate) &&
        !response_span_plan(too_large, 1, aggregate) &&
        !response_span_plan(address_wrap, 1, aggregate) &&
        !response_span_plan(zero, 1, aggregate) &&
        response_iovec_table_shape(0x1000, 2, iovec_count, iovec_bytes) &&
        iovec_count == 2 && iovec_bytes == 2 * sizeof(struct iovec) &&
        !response_iovec_table_shape(0, 1, iovec_count, iovec_bytes) &&
        !response_iovec_table_shape(0x1000, 0, iovec_count, iovec_bytes) &&
        !response_iovec_table_shape(0x1000,
            static_cast<std::uint64_t>(IOV_MAX) + 1,
            iovec_count, iovec_bytes) &&
        !response_iovec_table_shape(
            std::numeric_limits<std::uint64_t>::max(), 2,
            iovec_count, iovec_bytes) &&
        !response_selector(40, 41, 39, 1, true, 0) &&
        !response_selector(41, 41, 41, 1, true, 0) &&
        !response_selector(41, 41, 39, 2, true, 0) &&
        !response_selector(41, 41, 39, 1, false, 0) &&
        !response_selector(41, 41, 39, 1, true, 1) &&
        response_selector(41, 41, 39, 1, true, 0) &&
        response_exit_shape(exit, 41, SYS_write, true, -ENOSYS) &&
        !response_exit_shape(exit, 40, SYS_write, true, -ENOSYS) &&
        !response_exit_shape(exit, 41, SYS_writev, true, -ENOSYS) &&
        !response_exit_shape(exit, 41, SYS_write, true, -EIO) &&
        !response_exit_shape(exit, 41, SYS_write, false, 0) &&
        response_consumer_shape(0, true) &&
        !response_consumer_shape(1, true) &&
        !response_consumer_shape(0, false);
}

bool decode_response_transcript(pid_t pid, const tracee_state & state,
                                std::string & output) {
    std::vector<response_span> spans;
    if (state.nr == SYS_write) {
        spans.push_back({ state.args[1], state.args[2] });
    } else if (state.nr == SYS_writev) {
        std::size_t count = 0, table_bytes = 0;
        if (!response_iovec_table_shape(
                state.args[1], state.args[2], count, table_bytes)) return false;
        std::vector<struct iovec> vectors(count);
        if (!read_tracee(pid, state.args[1], vectors.data(), table_bytes)) return false;
        spans.reserve(count);
        for (const auto & vector : vectors) {
            spans.push_back({ reinterpret_cast<std::uintptr_t>(vector.iov_base),
                              vector.iov_len });
        }
    } else {
        return false;
    }
    std::size_t aggregate = 0;
    if (!response_span_plan(spans.data(), spans.size(), aggregate)) return false;
    output.clear();
    output.reserve(aggregate);
    for (const auto & span : spans) {
        if (span.length == 0) continue;
        const std::size_t start = output.size();
        output.resize(start + static_cast<std::size_t>(span.length));
        if (!read_tracee(pid, span.address, output.data() + start,
                         static_cast<std::size_t>(span.length))) {
            std::fill(output.begin(), output.end(), '\0');
            output.clear();
            return false;
        }
    }
    return output.size() == aggregate;
}

// This is only an exact raw-signature tripwire. Canonical same-role retry
// authority remains unadmitted for the 247 aggregate-only roles until their
// object selectors and windows are trace-mapped in the role manifest.
bool exact_signature_retry_guard(const fault_state & fault,
                                 const tracee_state & state) {
    return fault.selected_signature_valid && fault.selected_nr == state.nr &&
        std::equal(fault.selected_args.begin(), fault.selected_args.end(),
                   std::begin(state.args));
}

enum class retry_window_result { outside, closed, selected, excessive };

retry_window_result observe_retry_window(fault_state & fault,
                                         bool exact_boundary,
                                         unsigned occurrence) {
    if (!fault.retry_window_open) return retry_window_result::outside;
    if (!exact_boundary) {
        fault.retry_window_open = false;
        return retry_window_result::closed;
    }
    ++fault.matches;
    return fault.matches > occurrence + 1
        ? retry_window_result::excessive
        : retry_window_result::selected;
}

bool retry_window_self_check() {
    fault_state extra {};
    extra.matches = 4;
    extra.retry_window_open = true;
    if (observe_retry_window(extra, true, 4) != retry_window_result::selected ||
        extra.matches != 5 || !extra.retry_window_open ||
        observe_retry_window(extra, true, 4) != retry_window_result::excessive ||
        extra.matches != 6) return false;
    fault_state phase_reuse {};
    phase_reuse.matches = 4;
    phase_reuse.retry_window_open = true;
    return observe_retry_window(phase_reuse, true, 4) ==
               retry_window_result::selected &&
           observe_retry_window(phase_reuse, false, 4) ==
               retry_window_result::closed &&
           observe_retry_window(phase_reuse, true, 4) ==
               retry_window_result::outside &&
           phase_reuse.matches == 5 && !phase_reuse.retry_window_open;
}

bool retry_alias_controller_path_self_check();

bool manifest_self_check() {
    constexpr unsigned errno_rows = 19;
    constexpr unsigned short_rows = 25;
    constexpr unsigned opens = 37 * errno_rows;
    constexpr unsigned closes = 52 * errno_rows;
    constexpr unsigned sync_mode_rename = (1 + 3 + 1) * errno_rows;
    constexpr unsigned writes = short_rows;
    constexpr unsigned data_reads = 8 * short_rows;
    constexpr unsigned eof_reads = 8 * errno_rows;
    constexpr unsigned syscall_seam_base = opens + closes +
        sync_mode_rename + writes + data_reads + eof_reads;
    static_assert(syscall_seam_base == 2163);
    constexpr unsigned cleanup_return_roles = 3;
    constexpr unsigned mountinfo_semantic_roles = 3 * 2;
    constexpr unsigned l05z_syscall_roles =
        86 + 36 + 30 + 32 + 40 + 23 + 2 +
        cleanup_return_roles + mountinfo_semantic_roles;
    constexpr unsigned deduplicated_nonretryable_profiles = 18;
    constexpr unsigned l05z_syscall_cases =
        l05z_syscall_roles * deduplicated_nonretryable_profiles;
    static_assert(l05z_syscall_roles == 258);
    static_assert(l05z_syscall_cases == 4644);

    constexpr unsigned canonical_nonbyte_compatibility_roles = 37 + 52 + 5;
    constexpr unsigned canonical_compatibility_cases =
        canonical_nonbyte_compatibility_roles * deduplicated_nonretryable_profiles +
        (1 + 8) * short_rows + 8 * errno_rows;
    static_assert(canonical_compatibility_cases == 2069);

    constexpr unsigned hostile_input_cases =
        halofpx_l05z_return_hostile_manifest_v1::canonical_case_count;
    // Structural row arithmetic intentionally does not claim a physical run
    // count: each mountinfo stream may contain multiple positive fragments.
    constexpr unsigned structural_row_subtotal =
        syscall_seam_base + l05z_syscall_cases + hostile_input_cases;
    static_assert(structural_row_subtotal == 8706);
    constexpr unsigned compatibility_execution_total = canonical_compatibility_cases +
        l05z_syscall_cases + hostile_input_cases;
    static_assert(compatibility_execution_total == 8612);
    constexpr unsigned semantic_duplicate_pairs = 17;
    constexpr unsigned semantic_unique_total =
        compatibility_execution_total - semantic_duplicate_pairs;
    static_assert(semantic_unique_total == 8595);
    return syscall_seam_base == 2163 && opens == 703 && closes == 988 &&
        sync_mode_rename == 95 && data_reads == 200 && eof_reads == 152 &&
        halofpx_l05z_return_hostile_manifest_v1::self_check() &&
        halofpx_l05z_return_role_authority_v1::self_check() &&
        halofpx_l05z_return_response_manifest_v1::self_check() &&
        retry_alias_controller_path_self_check() &&
        response_decoder_self_check() &&
        halofpx_l05z_return_hostile_manifest_v1::manifest().size() ==
            hostile_input_cases && structural_row_subtotal == 8706 &&
        compatibility_execution_total == 8612 &&
        semantic_unique_total == 8595;
}

halofpx::context_store_registered_id synthetic_registered_id(
        const std::string & value) {
    halofpx::context_store_registered_id output;
    if (value.size() > output.bytes.size()) return output;
    output.size = static_cast<std::uint8_t>(value.size());
    std::copy(value.begin(), value.end(), output.bytes.begin());
    return output;
}

struct synthetic_envelope_fixture {
    std::array<std::uint8_t, 32> secret {};
    halofpx::context_store_protected_registry_key_record key;
    std::array<std::uint8_t,
        halofpx::context_store_protected_registry_max_bytes> bytes {};
    std::size_t size = 0;
};

bool make_synthetic_envelope(synthetic_envelope_fixture & output) {
    output.secret.fill(0x44U);
    output.key.disposition = halofpx::context_store_key_disposition::active;
    output.key.key_id = synthetic_registered_id("registry-auth-v1");
    output.key.generation = 13;
    output.key.master_key = { output.secret.data(), output.secret.size() };
    halofpx::context_store_protected_registry_body body;
    body.registry_id = synthetic_registered_id("registry-v1");
    body.registry_epoch = 9;
    body.authority_base_scope_commitment.fill(0xaaU);
    body.policy_commitment.fill(0xbbU);
    body.last_consumed_sequence = 40;
    const auto encoded = halofpx::context_store_encode_protected_registry_v1(
        body, output.key, output.bytes.data(), output.bytes.size());
    output.size = encoded.encoded_size;
    return encoded.status ==
            halofpx::context_store_protected_registry_status::authenticated_unadmitted &&
        encoded.authenticated_carrier() != nullptr && output.size == 156;
}

bool retag_synthetic_envelope(synthetic_envelope_fixture & fixture) {
    constexpr char key_domain[] = "halofpx.registry-snapshot-key.v1";
    constexpr char auth_domain[] = "halofpx.registry-snapshot-auth.v1";
    if (fixture.size != 156 || fixture.key.key_id.size == 0 ||
        fixture.key.key_id.size > 23 || fixture.key.generation > 23) return false;
    std::vector<std::uint8_t> derivation(
        reinterpret_cast<const std::uint8_t *>(key_domain),
        reinterpret_cast<const std::uint8_t *>(key_domain) + sizeof(key_domain));
    derivation.push_back(static_cast<std::uint8_t>(
        0x60U | fixture.key.key_id.size));
    derivation.insert(derivation.end(), fixture.key.key_id.bytes.begin(),
        fixture.key.key_id.bytes.begin() + fixture.key.key_id.size);
    derivation.push_back(static_cast<std::uint8_t>(fixture.key.generation));
    halofpx::context_store_format_digest derived {}, tag {};
    if (!halofpx::context_store_hmac_sha256(
            fixture.secret.data(), fixture.secret.size(), derivation.data(),
            derivation.size(), derived)) return false;
    std::vector<std::uint8_t> authenticated(
        reinterpret_cast<const std::uint8_t *>(auth_domain),
        reinterpret_cast<const std::uint8_t *>(auth_domain) + sizeof(auth_domain));
    authenticated.insert(authenticated.end(), fixture.bytes.begin() + 2,
                         fixture.bytes.begin() + fixture.size - 35);
    const bool tagged = halofpx::context_store_hmac_sha256(
        derived.data(), derived.size(), authenticated.data(), authenticated.size(), tag);
    if (tagged) {
        std::copy(tag.begin(), tag.end(),
                  fixture.bytes.begin() + fixture.size - tag.size());
    }
    std::fill(derived.begin(), derived.end(), 0U);
    std::fill(tag.begin(), tag.end(), 0U);
    return tagged;
}

int hostile_case_self_test(const std::string & case_id) {
    namespace hostile = halofpx_l05z_return_hostile_manifest_v1;
    if (!manifest_self_check()) return 2;
    const hostile::case_record * selected = hostile::select_case(case_id);
    if (!selected) return 2;
    const auto exact_pre_latch = [selected](hostile::oracle expected,
                                             hostile::stage expected_stage) {
        return selected->semantic_stage == expected_stage &&
            selected->expected_oracle == expected && selected->expected_phase == 0 &&
            selected->expected_envelope_mask == 0 && !selected->expected_qualified &&
            selected->expected_child_status == hostile::process_status::not_run &&
            selected->expected_launcher_status == hostile::process_status::not_run &&
            selected->expected_inventory == hostile::inventory_class::pristine_unopened &&
            selected->expected_publication_attempts == 0 &&
            selected->requires_zero_storage_syscalls &&
            selected->requires_unset_latch &&
            selected->requires_all_l05z_facts_zero;
    };
    synthetic_envelope_fixture fixture;
    if (!make_synthetic_envelope(fixture)) return 1;
    using status = halofpx::context_store_protected_registry_status;
    const auto verify = [&fixture]() {
        return halofpx::context_store_verify_protected_registry_v1(
            fixture.bytes.data(), fixture.size, fixture.key);
    };
    if (case_id == "HCB-MAPCOUNT-OUTER-HIGH") {
        fixture.bytes[0] = 0xa3U;
        return exact_pre_latch(hostile::oracle::reject_bounded_parse_before_latch,
                               hostile::stage::sealed_input_pre_latch) &&
            verify().status == status::structural_rejection ? 0 : 1;
    }
    if (case_id == "HRAW-B0000") {
        fixture.bytes[0] ^= 0x80U;
        return exact_pre_latch(hostile::oracle::reject_sealed_input_before_latch,
                               hostile::stage::sealed_input_pre_latch) &&
            verify().status == status::structural_rejection ? 0 : 1;
    }
    if (case_id == "HTAG-B0000") {
        fixture.bytes[124] ^= 0x80U;
        return exact_pre_latch(hostile::oracle::reject_authentication_before_latch,
                               hostile::stage::sealed_input_pre_latch) &&
            verify().status == status::authentication_failed ? 0 : 1;
    }
    if (case_id == "HSEM-001") {
        fixture.bytes[6] = 2U;
        return exact_pre_latch(hostile::oracle::reject_semantic_before_latch,
                               hostile::stage::sealed_input_pre_latch) &&
            retag_synthetic_envelope(fixture) &&
            verify().status == status::structural_rejection ? 0 : 1;
    }
    if (case_id == "HDIG-B000") {
        if (!exact_pre_latch(hostile::oracle::reject_digest_pin_before_latch,
                             hostile::stage::sealed_input_pre_latch) ||
            selected->expected_gate != hostile::authority_gate::target_digest_equality_pin ||
            selected->expected_verifier_status !=
                hostile::verifier_status::authenticated_unadmitted ||
            selected->reason !=
                hostile::expected_reason::digest_equality_pin_mismatch) return 1;
        constexpr halofpx::context_store_format_digest locked_registry_digest {
            0x5b,0x6e,0xa5,0x88,0x7d,0x96,0xbf,0x20,
            0xc1,0x76,0xe2,0x34,0x6c,0x24,0x78,0x14,
            0x9a,0x8b,0x7b,0xcf,0x1d,0x84,0xdc,0x98,
            0x84,0x3c,0x28,0xf4,0xe0,0x0d,0x0b,0x9e,
        };
        halofpx::context_store_format_digest observed {};
        if (!halofpx::context_store_registry_lab_linux_initializer_predecessor_digest_v1(
                fixture.bytes.data(), fixture.size, observed) ||
            observed != locked_registry_digest) return 1;
        auto pinned = observed;
        pinned[0] ^= 0x80U;
        // This is the target-owned equality-pin oracle, not a protected-registry
        // verifier rejection: the exact bytes authenticate, then the supplied
        // registry-envelope digest pin is byte-distinct and must not match.
        std::uint8_t difference = 0;
        for (std::size_t index = 0; index < pinned.size(); ++index) {
            difference |= static_cast<std::uint8_t>(pinned[index] ^ observed[index]);
        }
        return difference != 0 ? 0 : 1;
    }
    if (case_id == "HPIN-KEYID") {
        fixture.key.key_id = synthetic_registered_id("other-key");
        return exact_pre_latch(hostile::oracle::reject_replay_pin_mismatch,
                               hostile::stage::replay_pin_policy) &&
            selected->expected_gate ==
                hostile::authority_gate::protected_registry_verifier &&
            selected->expected_verifier_status == hostile::verifier_status::unknown_key &&
            selected->reason == hostile::expected_reason::verifier_unknown_key &&
            verify().status == status::unknown_key ? 0 : 1;
    }
    return 3; // Enumerated but not admitted to the executable subset yet.
}



struct marker_state {

    bool transient_seen = false;
    bool published_seen = false;
    bool final_open_seen = false;
    bool marker_pinned = false;
    bool temp_substitute_open_seen = false;
    bool basename_known = false;
    std::array<char, 72> basename {};
    std::size_t encoded_size = 0;
    std::size_t retained_size = 0;
    std::array<std::uint8_t, 1024> intended_bytes {};
    std::array<std::uint8_t, 1025> retained_bytes {};
    std::array<bool, 1024> intended_known {};
    std::array<bool, 1025> retained_known {};
    struct stat identity {};
    std::uint64_t mount_id = 0;
};

bool valid_envelope_basename(const std::string & value) {
    if (value.size() != 71 || value.rfind("e-", 0) != 0 ||
        value.compare(66, 5, ".cbor") != 0) return false;
    return std::all_of(value.begin() + 2, value.begin() + 66, [](char byte) {
        return (byte >= '0' && byte <= '9') || (byte >= 'a' && byte <= 'f');
    });
}

bool expected_envelope_basename(const marker_state & marker,
                                std::array<char, 72> & output);

bool remember_envelope_basename(marker_state & marker,
                                const std::string & value) {
    if (!valid_envelope_basename(value)) return false;
    std::array<char, 72> expected {};
    if (!expected_envelope_basename(marker, expected) ||
        value != expected.data()) {
        (void) emit(std::string("\"event\":\"digest-name-mismatch\",\"observed\":\"") +
                    value + "\",\"expected\":\"" + expected.data() + "\"");
        return false;
    }
    if (marker.basename_known) {
        return std::memcmp(marker.basename.data(), value.data(), value.size()) == 0;
    }
    std::memcpy(marker.basename.data(), value.data(), value.size());
    marker.basename[value.size()] = '\0';
    marker.basename_known = true;
    return true;
}

std::uint32_t rotr32(std::uint32_t value, unsigned shift) {
    return (value >> shift) | (value << (32U - shift));
}

bool expected_envelope_basename(const marker_state & marker,
                                std::array<char, 72> & output) {
    if (marker.encoded_size == 0 || marker.encoded_size > 1024 ||
        !std::all_of(marker.intended_known.begin(),
                     marker.intended_known.begin() +
                         static_cast<std::ptrdiff_t>(marker.encoded_size),
                     [](bool known) { return known; })) return false;
    constexpr char domain[] =
        "halofpx.registry-lab-registry-envelope.v1";
    std::array<std::uint8_t, 1152> message {};
    const std::size_t domain_size = sizeof(domain); // includes the required NUL
    const std::size_t input_size = domain_size + marker.encoded_size;
    std::memcpy(message.data(), domain, domain_size);
    std::memcpy(message.data() + domain_size, marker.intended_bytes.data(),
                marker.encoded_size);
    message[input_size] = 0x80;
    std::size_t padded = input_size + 1;
    while ((padded % 64) != 56) ++padded;
    const std::uint64_t bits = static_cast<std::uint64_t>(input_size) * 8U;
    for (unsigned index = 0; index < 8; ++index) {
        message[padded + index] = static_cast<std::uint8_t>(
            bits >> (56U - 8U * index));
    }
    padded += 8;
    std::array<std::uint32_t, 8> hash {
        0x6a09e667U, 0xbb67ae85U, 0x3c6ef372U, 0xa54ff53aU,
        0x510e527fU, 0x9b05688cU, 0x1f83d9abU, 0x5be0cd19U,
    };
    constexpr std::array<std::uint32_t, 64> constants {
        0x428a2f98U,0x71374491U,0xb5c0fbcfU,0xe9b5dba5U,0x3956c25bU,0x59f111f1U,0x923f82a4U,0xab1c5ed5U,
        0xd807aa98U,0x12835b01U,0x243185beU,0x550c7dc3U,0x72be5d74U,0x80deb1feU,0x9bdc06a7U,0xc19bf174U,
        0xe49b69c1U,0xefbe4786U,0x0fc19dc6U,0x240ca1ccU,0x2de92c6fU,0x4a7484aaU,0x5cb0a9dcU,0x76f988daU,
        0x983e5152U,0xa831c66dU,0xb00327c8U,0xbf597fc7U,0xc6e00bf3U,0xd5a79147U,0x06ca6351U,0x14292967U,
        0x27b70a85U,0x2e1b2138U,0x4d2c6dfcU,0x53380d13U,0x650a7354U,0x766a0abbU,0x81c2c92eU,0x92722c85U,
        0xa2bfe8a1U,0xa81a664bU,0xc24b8b70U,0xc76c51a3U,0xd192e819U,0xd6990624U,0xf40e3585U,0x106aa070U,
        0x19a4c116U,0x1e376c08U,0x2748774cU,0x34b0bcb5U,0x391c0cb3U,0x4ed8aa4aU,0x5b9cca4fU,0x682e6ff3U,
        0x748f82eeU,0x78a5636fU,0x84c87814U,0x8cc70208U,0x90befffaU,0xa4506cebU,0xbef9a3f7U,0xc67178f2U,
    };
    for (std::size_t block = 0; block < padded; block += 64) {
        std::array<std::uint32_t, 64> words {};
        for (unsigned index = 0; index < 16; ++index) {
            const std::size_t offset = block + index * 4;
            words[index] = (static_cast<std::uint32_t>(message[offset]) << 24) |
                (static_cast<std::uint32_t>(message[offset + 1]) << 16) |
                (static_cast<std::uint32_t>(message[offset + 2]) << 8) |
                static_cast<std::uint32_t>(message[offset + 3]);
        }
        for (unsigned index = 16; index < 64; ++index) {
            const std::uint32_t s0 = rotr32(words[index - 15], 7) ^
                rotr32(words[index - 15], 18) ^ (words[index - 15] >> 3);
            const std::uint32_t s1 = rotr32(words[index - 2], 17) ^
                rotr32(words[index - 2], 19) ^ (words[index - 2] >> 10);
            words[index] = words[index - 16] + s0 + words[index - 7] + s1;
        }
        auto a=hash[0], b=hash[1], c=hash[2], d=hash[3];
        auto e=hash[4], f=hash[5], g=hash[6], h=hash[7];
        for (unsigned index = 0; index < 64; ++index) {
            const std::uint32_t s1 = rotr32(e,6)^rotr32(e,11)^rotr32(e,25);
            const std::uint32_t choice = (e & f) ^ (~e & g);
            const std::uint32_t temp1 = h+s1+choice+constants[index]+words[index];
            const std::uint32_t s0 = rotr32(a,2)^rotr32(a,13)^rotr32(a,22);
            const std::uint32_t majority = (a & b) ^ (a & c) ^ (b & c);
            const std::uint32_t temp2 = s0 + majority;
            h=g; g=f; f=e; e=d+temp1; d=c; c=b; b=a; a=temp1+temp2;
        }
        hash[0]+=a; hash[1]+=b; hash[2]+=c; hash[3]+=d;
        hash[4]+=e; hash[5]+=f; hash[6]+=g; hash[7]+=h;
    }
    constexpr char hex[] = "0123456789abcdef";
    output[0]='e'; output[1]='-';
    for (unsigned index=0; index<32; ++index) {
        const std::uint8_t byte = static_cast<std::uint8_t>(
            hash[index / 4] >> (24U - 8U * (index % 4)));
        output[2 + index * 2] = hex[byte >> 4];
        output[3 + index * 2] = hex[byte & 0x0f];
    }
    std::memcpy(output.data()+66, ".cbor", 5);
    output[71]='\0';
    std::fill(message.begin(), message.end(), 0);
    return true;
}

bool golden_json_string(const std::string & json, const char * key,
                        std::string & output) {
    const std::string prefix = std::string("\"") + key + "\": \"";
    const std::size_t begin = json.find(prefix);
    if (begin == std::string::npos || json.find(prefix, begin + 1) !=
            std::string::npos) return false;
    const std::size_t value_begin = begin + prefix.size();
    const std::size_t end = json.find('"', value_begin);
    if (end == std::string::npos) return false;
    output.assign(json, value_begin, end - value_begin);
    return !output.empty();
}

int hex_nibble(char value) {
    if (value >= '0' && value <= '9') return value - '0';
    if (value >= 'a' && value <= 'f') return value - 'a' + 10;
    return -1;
}

bool load_golden_envelope(const char * path, marker_state & output) {
    std::ifstream stream(path, std::ios::binary);
    if (!stream) return false;
    std::string json((std::istreambuf_iterator<char>(stream)), {});
    if (json.empty() || json.size() > 1024 * 1024) return false;
    std::string bytes_hex, digest_hex;
    if (!golden_json_string(json, "predecessor_registry_envelope_hex", bytes_hex) ||
        !golden_json_string(json, "predecessor_registry_envelope_digest_hex",
                            digest_hex) ||
        bytes_hex.size() < 2 || bytes_hex.size() > 2048 ||
        (bytes_hex.size() & 1U) != 0 || digest_hex.size() != 64) return false;
    output.encoded_size = bytes_hex.size() / 2;
    for (std::size_t index = 0; index < output.encoded_size; ++index) {
        const int high = hex_nibble(bytes_hex[index * 2]);
        const int low = hex_nibble(bytes_hex[index * 2 + 1]);
        if (high < 0 || low < 0) return false;
        output.intended_bytes[index] = static_cast<std::uint8_t>((high << 4) | low);
        output.intended_known[index] = true;
    }
    std::array<char, 72> expected {};
    if (!expected_envelope_basename(output, expected) ||
        std::memcmp(expected.data() + 2, digest_hex.data(), 64) != 0) return false;
    output.basename = expected;
    output.basename_known = true;
    return true;
}

[[noreturn]] void usage_marker(const char * program) {
    std::fprintf(stderr,
        "usage: %s --target PATH --golden PATH --parent PATH --root PATH "
        "--fixture PATH --receipt NEW-PATH --boundary "
        "open-step4|close-step4|open-temp|fchmod-temp|pwrite-envelope|pread-envelope|pread-envelope-eof|"
        "fsync-envelope|open-readonly-temp|rename-envelope|fsync-envelopes|"

        "fsync-staging|open-final|"

        "pread-final|pread-final-eof|pread-marker|pread-marker-eof|close-envelope|close-staging|"
        "unlock-writer|unlock-fixture|"

        "reserve-revalidation|final-validation|stdout-audit-response --mode "
        "pre|late|eintr-once|short-pre|short-late|zero-pre|zero-late|"
        "suppress-fake-full|"
        "corrupt|truncate|append|unexpected-name|hardlink|symlink|"
        "inode-substitute|temp-inode-substitute|collision|reserve-loss --errno "
        "EIO|ENOSPC|EDQUOT|EROFS|EEXIST|EXDEV|ENOSYS|EINVAL|EINTR "
        "[--occurrence N] [--short-count N]\n", program);

    std::exit(2);
}


bool parse_boundary_name(const char * value, boundary & output) {

    struct row { const char * name; boundary value; };
    constexpr row rows[] {
        { "open-step4", boundary::step4_open },
        { "close-step4", boundary::step4_close },
        { "open-temp", boundary::transient_open },
        { "fchmod-temp", boundary::transient_fchmod },
        { "pwrite-envelope", boundary::envelope_pwrite },
        { "pread-envelope", boundary::envelope_pread },
        { "pread-envelope-eof", boundary::envelope_pread_eof },
        { "fsync-envelope", boundary::envelope_fsync },
        { "open-readonly-temp", boundary::readonly_temp_open },
        { "rename-envelope", boundary::envelope_rename },
        { "fsync-envelopes", boundary::envelopes_fsync },
        { "fsync-staging", boundary::staging_fsync },
        { "open-final", boundary::final_open },
        { "pread-final", boundary::final_pread },
        { "pread-final-eof", boundary::final_pread_eof },
        { "pread-marker", boundary::marker_pread },
        { "pread-marker-eof", boundary::marker_pread_eof },
        { "close-envelope", boundary::envelope_close },
        { "close-staging", boundary::staging_close },
        { "unlock-writer", boundary::writer_unlock },
        { "unlock-fixture", boundary::fixture_unlock },
        { "reserve-revalidation", boundary::reserve_revalidation },
        { "final-validation", boundary::final_validation },
        { "fstat", boundary::fstat_call },
        { "statx", boundary::statx_call },
        { "readlink", boundary::readlink_call },
        { "getdents", boundary::getdents_call },
        { "facts", boundary::facts_call },
        { "fstatfs", boundary::fstatfs_call },
        { "stdout-audit-response", boundary::stdout_audit_response },
    };
    for (const auto & row : rows) {
        if (std::strcmp(value, row.name) == 0) {
            output = row.value;
            return true;
        }
    }
    return false;
}

bool parse_mode_name(const char * value, mode & output) {
    struct row { const char * name; mode value; };
    constexpr row rows[] {
        { "pre", mode::pre_error }, { "late", mode::late_error },
        { "eintr-once", mode::eintr_once },
        { "short-pre", mode::short_pre }, { "short-late", mode::short_late },
        { "zero-pre", mode::zero_pre }, { "zero-late", mode::zero_late },
        { "suppress-fake-full", mode::response_loss_full },
        { "corrupt", mode::corrupt }, { "truncate", mode::truncate },
        { "append", mode::append },
        { "unexpected-name", mode::unexpected_name },
        { "hardlink", mode::hardlink }, { "symlink", mode::symlink },
        { "inode-substitute", mode::inode_substitute },
        { "temp-inode-substitute", mode::temp_inode_substitute },
        { "collision", mode::collision },
        { "reserve-loss", mode::reserve_loss },
    };
    for (const auto & row : rows) {
        if (std::strcmp(value, row.name) == 0) {
            output = row.value;
            return true;
        }
    }
    return false;
}

bool parse_errno_name(const char * value, int & output) {
    struct row { const char * name; int value; };
    constexpr row rows[] {
        { "EIO", EIO }, { "ENOSPC", ENOSPC }, { "EDQUOT", EDQUOT },
        { "EROFS", EROFS }, { "EEXIST", EEXIST }, { "EXDEV", EXDEV },
        { "ENOSYS", ENOSYS }, { "EINVAL", EINVAL }, { "EINTR", EINTR },
    };
    for (const auto & row : rows) {
        if (std::strcmp(value, row.name) == 0) {
            output = row.value;
            return true;
        }
    }
    return false;
}

bool parse_unsigned(const char * value, unsigned maximum, unsigned & output) {
    if (value == nullptr || value[0] == '\0' || value[0] == '-') return false;
    errno = 0;
    char * end = nullptr;
    const unsigned long parsed = std::strtoul(value, &end, 10);
    if (errno != 0 || end == value || *end != '\0' || parsed == 0 ||
        parsed > maximum) return false;
    output = static_cast<unsigned>(parsed);
    return true;
}

bool mapped_role_boundary_execution_closed(boundary value) {
    switch (value) {
        case boundary::fstat_call:
        case boundary::statx_call:
        case boundary::readlink_call:
        case boundary::getdents_call:
        case boundary::facts_call:
        case boundary::fstatfs_call:
            return true;
        default:
            return false;
    }
}

bool mapped_role_boundary_execution_closed_self_check() {
    return mapped_role_boundary_execution_closed(boundary::fstat_call) &&
        mapped_role_boundary_execution_closed(boundary::statx_call) &&
        mapped_role_boundary_execution_closed(boundary::readlink_call) &&
        mapped_role_boundary_execution_closed(boundary::getdents_call) &&
        mapped_role_boundary_execution_closed(boundary::facts_call) &&
        mapped_role_boundary_execution_closed(boundary::fstatfs_call) &&
        !mapped_role_boundary_execution_closed(boundary::step4_open) &&
        !mapped_role_boundary_execution_closed(boundary::stdout_audit_response);
}

bool parse_arguments(int argc, char ** argv, arguments & output) {
    for (int index = 1; index < argc; index += 2) {
        if (index + 1 >= argc) return false;
        const char * key = argv[index];
        const char * value = argv[index + 1];
        if (std::strcmp(key, "--target") == 0) {
            if (!set_once(output.target, value)) return false;
        } else if (std::strcmp(key, "--golden") == 0) {
            if (!set_once(output.golden, value)) return false;
        } else if (std::strcmp(key, "--parent") == 0) {
            if (!set_once(output.parent, value)) return false;
        } else if (std::strcmp(key, "--root") == 0) {
            if (!set_once(output.root, value)) return false;
        } else if (std::strcmp(key, "--fixture") == 0) {
            if (!set_once(output.fixture, value)) return false;
        } else if (std::strcmp(key, "--receipt") == 0) {
            if (!set_once(output.receipt, value)) return false;
        } else if (std::strcmp(key, "--boundary") == 0) {
            if (output.point_set || !parse_boundary_name(value, output.point)) return false;
            output.point_set = true;
        } else if (std::strcmp(key, "--mode") == 0) {
            if (output.mode_set || !parse_mode_name(value, output.injection)) return false;
            output.mode_set = true;
        } else if (std::strcmp(key, "--errno") == 0) {
            if (output.errno_set || !parse_errno_name(value, output.returned_errno)) return false;
            output.errno_set = true;
        } else if (std::strcmp(key, "--occurrence") == 0) {
            if (output.occurrence_set || !parse_unsigned(value, 1000000, output.occurrence)) return false;
            output.occurrence_set = true;
        } else if (std::strcmp(key, "--short-count") == 0) {
            if (output.short_set || !parse_unsigned(value, 1023, output.short_count)) return false;
            output.short_set = true;
        } else {
            return false;
        }
    }
    if (!output.target || !output.golden || !output.parent || !output.root ||
        !output.fixture || !output.receipt || !output.point_set || !output.mode_set) {
        return false;
    }
    // The parallel v1 map records source evidence only.  None of its 247 roles
    // has an exact executable selector, so reject all six aggregate boundaries
    // before target validation or launch.  A later admission must use a new,
    // versioned exact-selector authority rather than weakening this gate.
    if (mapped_role_boundary_execution_closed(output.point)) return false;
    const bool error_mode = output.injection == mode::pre_error ||
        output.injection == mode::late_error || output.injection == mode::eintr_once;
    if (error_mode != output.errno_set) return false;
    if (output.injection == mode::eintr_once && output.returned_errno != EINTR) return false;
    const bool count_mode = output.injection == mode::short_pre ||
        output.injection == mode::short_late;
    if (count_mode != output.short_set) return false;
    const bool byte_count_boundary = output.point == boundary::envelope_pwrite ||
        output.point == boundary::envelope_pread ||
        output.point == boundary::marker_pread ||

        output.point == boundary::final_pread;
    if ((count_mode || output.injection == mode::zero_pre ||
         output.injection == mode::zero_late) && !byte_count_boundary) return false;
    const bool mutation_mode = output.injection >= mode::corrupt;
    const bool response_mode = output.injection == mode::response_loss_full;

    if (response_mode != (output.point == boundary::stdout_audit_response)) {
        return false;
    }

    if (response_mode) {
        if (output.occurrence != 1 || output.errno_set || output.short_set) return false;
    } else if (output.injection == mode::reserve_loss) {
        if (output.point != boundary::reserve_revalidation) return false;


    } else if (output.injection == mode::collision) {
        if (output.point != boundary::transient_open &&

            output.point != boundary::envelope_rename) return false;
    } else if (output.injection == mode::temp_inode_substitute) {
        if (output.point != boundary::readonly_temp_open) return false;

    } else if (mutation_mode && output.point != boundary::final_validation) {
        return false;
    }
    if (output.point == boundary::final_validation && !mutation_mode) return false;
    const unsigned occurrence_limit =
        output.point == boundary::step4_open ? 37U :
        output.point == boundary::step4_close ? 52U :
        (output.point == boundary::envelope_pread ||
         output.point == boundary::envelope_pread_eof ||
         output.point == boundary::marker_pread ||
         output.point == boundary::marker_pread_eof) ? 4U :
        output.point == boundary::envelope_close ? 4U :
        output.point == boundary::staging_close ? 3U :
        (output.point == boundary::final_open ||
         output.point == boundary::final_pread ||
         output.point == boundary::final_pread_eof ||
         output.point == boundary::final_validation) ? 2U :
        output.point == boundary::reserve_revalidation ? 3U :
        (output.point == boundary::fstat_call || output.point == boundary::statx_call ||
         output.point == boundary::readlink_call ||
         output.point == boundary::getdents_call ||
         output.point == boundary::facts_call ||
         output.point == boundary::fstatfs_call) ? 1000000U : 1U;
    if (output.occurrence > occurrence_limit) return false;
    return true;
}

bool eintr_is_retryable(boundary value) {
    switch (value) {
        case boundary::marker_pread:
        case boundary::marker_pread_eof:
        case boundary::envelope_pwrite:
        case boundary::envelope_pread:
        case boundary::envelope_pread_eof:
        case boundary::final_pread:
        case boundary::final_pread_eof:
            return true;
        case boundary::step4_open:
        case boundary::step4_close:
        case boundary::transient_open:
        case boundary::transient_fchmod:
        case boundary::envelope_fsync:
        case boundary::readonly_temp_open:
        case boundary::envelope_rename:
        case boundary::envelopes_fsync:
        case boundary::staging_fsync:
        case boundary::final_open:
        case boundary::envelope_close:
        case boundary::staging_close:
        case boundary::writer_unlock:
        case boundary::fixture_unlock:
        case boundary::reserve_revalidation:
        case boundary::final_validation:
        case boundary::fstat_call:
        case boundary::statx_call:
        case boundary::readlink_call:
        case boundary::getdents_call:
        case boundary::facts_call:
        case boundary::fstatfs_call:
        case boundary::stdout_audit_response:
            return false;
    }
    return false;
}

bool selected_entry_is_suppressed(const arguments & input) {
    return input.injection == mode::pre_error ||
        input.injection == mode::short_pre ||
        input.injection == mode::zero_pre ||
        input.injection == mode::eintr_once;
}

bool synthetic_exit_returns_errno(const arguments & input) {
    return input.injection == mode::pre_error ||
        input.injection == mode::eintr_once;
}

bool completed_exit_returns_errno(const arguments & input) {
    return input.injection == mode::late_error;
}

bool returned_eintr_must_retry(const arguments & input) {
    return input.returned_errno == EINTR && eintr_is_retryable(input.point) &&
        ((selected_entry_is_suppressed(input) &&
          synthetic_exit_returns_errno(input)) ||
         (!selected_entry_is_suppressed(input) &&
          completed_exit_returns_errno(input)));
}

bool retry_alias_controller_path_self_check() {
    constexpr boundary retry_boundaries[] {
        boundary::envelope_pwrite,
        boundary::envelope_pread, boundary::envelope_pread_eof,
        boundary::final_pread, boundary::final_pread_eof,
        boundary::marker_pread, boundary::marker_pread_eof,
    };
    for (const boundary point : retry_boundaries) {
        arguments pre {};
        pre.point = point;
        pre.injection = mode::pre_error;
        pre.returned_errno = EINTR;
        arguments once = pre;
        once.injection = mode::eintr_once;
        const long long pre_replacement = -static_cast<long long>(pre.returned_errno);
        const long long once_replacement =
            -static_cast<long long>(once.returned_errno);
        if (!returned_eintr_must_retry(pre) ||
            !returned_eintr_must_retry(once) ||
            selected_entry_is_suppressed(pre) !=
                selected_entry_is_suppressed(once) ||
            !selected_entry_is_suppressed(pre) ||
            synthetic_exit_returns_errno(pre) !=
                synthetic_exit_returns_errno(once) ||
            !synthetic_exit_returns_errno(pre) ||
            completed_exit_returns_errno(pre) ||
            completed_exit_returns_errno(once) ||
            pre_replacement != once_replacement ||
            pre_replacement != -EINTR) return false;
    }
    arguments late {};
    late.point = boundary::envelope_pwrite;
    late.injection = mode::late_error;
    late.returned_errno = EINTR;
    return returned_eintr_must_retry(late) &&
        !selected_entry_is_suppressed(late) &&
        !synthetic_exit_returns_errno(late) &&
        completed_exit_returns_errno(late);
}

const char * boundary_name(boundary value) {
    switch (value) {
        case boundary::step4_open: return "open-step4";
        case boundary::step4_close: return "close-step4";
        case boundary::transient_open: return "open-temp";
        case boundary::transient_fchmod: return "fchmod-temp";
        case boundary::envelope_pwrite: return "pwrite-envelope";
        case boundary::envelope_pread: return "pread-envelope";
        case boundary::envelope_pread_eof: return "pread-envelope-eof";
        case boundary::envelope_fsync: return "fsync-envelope";
        case boundary::readonly_temp_open: return "open-readonly-temp";
        case boundary::envelope_rename: return "rename-envelope";
        case boundary::envelopes_fsync: return "fsync-envelopes";
        case boundary::staging_fsync: return "fsync-staging";
        case boundary::final_open: return "open-final";
        case boundary::final_pread: return "pread-final";
        case boundary::final_pread_eof: return "pread-final-eof";
        case boundary::marker_pread: return "pread-marker";
        case boundary::marker_pread_eof: return "pread-marker-eof";
        case boundary::envelope_close: return "close-envelope";
        case boundary::staging_close: return "close-staging";
        case boundary::writer_unlock: return "unlock-writer";
        case boundary::fixture_unlock: return "unlock-fixture";
        case boundary::reserve_revalidation: return "reserve-revalidation";
        case boundary::final_validation: return "final-validation";
        case boundary::fstat_call: return "fstat";
        case boundary::statx_call: return "statx";
        case boundary::readlink_call: return "readlink";
        case boundary::getdents_call: return "getdents";
        case boundary::facts_call: return "facts";
        case boundary::fstatfs_call: return "fstatfs";
        case boundary::stdout_audit_response: return "stdout-audit-response";
    }
    return "unknown";
}

const char * mode_name(mode value) {
    switch (value) {
        case mode::pre_error: return "pre";
        case mode::late_error: return "late";
        case mode::eintr_once: return "eintr-once";
        case mode::short_pre: return "short-pre";
        case mode::short_late: return "short-late";
        case mode::zero_pre: return "zero-pre";
        case mode::zero_late: return "zero-late";
        case mode::response_loss_full: return "suppress-fake-full";
        case mode::corrupt: return "corrupt";
        case mode::truncate: return "truncate";
        case mode::append: return "append";
        case mode::unexpected_name: return "unexpected-name";
        case mode::hardlink: return "hardlink";
        case mode::symlink: return "symlink";
        case mode::inode_substitute: return "inode-substitute";
        case mode::temp_inode_substitute: return "temp-inode-substitute";
        case mode::collision: return "collision";
        case mode::reserve_loss: return "reserve-loss";
    }
    return "unknown";
}

bool replace_entry_with_enosys(pid_t pid) {
    struct user_regs_struct registers {};
    if (::ptrace(PTRACE_GETREGS, pid, nullptr, &registers) != 0) return false;
    registers.orig_rax = static_cast<decltype(registers.orig_rax)>(-1LL);
    return ::ptrace(PTRACE_SETREGS, pid, nullptr, &registers) == 0;
}

bool replace_return(pid_t pid, long long value) {
    struct user_regs_struct registers {};
    if (::ptrace(PTRACE_GETREGS, pid, nullptr, &registers) != 0) return false;
    registers.rax = static_cast<decltype(registers.rax)>(value);
    return ::ptrace(PTRACE_SETREGS, pid, nullptr, &registers) == 0;
}

bool write_tracee(pid_t pid, std::uint64_t address,
                  const void * input, std::size_t size) {
    const auto * bytes = static_cast<const unsigned char *>(input);
    for (std::size_t offset = 0; offset < size; offset += sizeof(long)) {
        const std::size_t copied = std::min(sizeof(long), size - offset);
        long word = 0;
        if (copied != sizeof(long)) {
            errno = 0;
            word = ::ptrace(PTRACE_PEEKDATA, pid,

                reinterpret_cast<void *>(address + offset), nullptr);
            if (word == -1 && errno != 0) return false;
        }
        std::memcpy(&word, bytes + offset, copied);
        if (::ptrace(PTRACE_POKEDATA, pid,
                     reinterpret_cast<void *>(address + offset), word) != 0) return false;
    }
    return true;

}

bool exact_envelope_child_argv(pid_t pid) {
    char path[64];

    const int size = std::snprintf(path, sizeof(path), "/proc/%ld/cmdline",
                                   static_cast<long>(pid));
    if (size <= 0 || static_cast<std::size_t>(size) >= sizeof(path)) return false;
    const int fd = ::open(path, O_RDONLY | O_CLOEXEC | O_NOFOLLOW);

    if (fd < 0) return false;
    std::array<char, 128> bytes {};
    ssize_t count;
    do { count = ::read(fd, bytes.data(), bytes.size()); } while (count < 0 && errno == EINTR);
    const bool closed = ::close(fd) == 0;
    constexpr char expected[] = "halofpx-l05z-live-child\0--live-envelope-child\0";
    return closed && count == static_cast<ssize_t>(sizeof(expected) - 1) &&
           std::memcmp(bytes.data(), expected, sizeof(expected) - 1) == 0;
}

bool exact_fd(pid_t pid, std::uint64_t raw_fd, const struct stat & expected,
              std::uint64_t expected_mount) {
    struct stat observed {};
    std::uint64_t mount = 0;

    return tracee_fd_identity(pid, static_cast<int>(raw_fd), observed, mount) &&
           same_object(expected, observed) && mount == expected_mount;
}


bool read_tracee_name(pid_t pid, std::uint64_t address, std::string & output) {
    output.clear();
    for (std::size_t offset = 0; offset < 4097; offset += sizeof(long)) {
        long word = 0;
        if (!read_tracee(pid, address + offset, &word, sizeof(word))) return false;
        for (std::size_t index = 0; index < sizeof(word); ++index) {
            const char byte = reinterpret_cast<const char *>(&word)[index];
            if (byte == '\0') return true;
            if (output.size() == 4096) return false;
            output.push_back(byte);
        }
    }
    return false;
}

bool read_how(pid_t pid, const tracee_state & state, struct open_how & how,
              std::string & name) {
    return state.nr == SYS_openat2 && state.args[3] == sizeof(how) &&
           read_tracee(pid, state.args[2], &how, sizeof(how)) &&
           read_tracee_name(pid, state.args[1], name);
}

bool matches_boundary(pid_t pid, const tracee_state & state,
                      const arguments & input, const struct stat & root,
                      std::uint64_t root_mount, const struct stat & parent,
                      std::uint64_t parent_mount, const struct stat & fixture,
                      std::uint64_t fixture_mount,
                      const pinned_directory & envelopes,
                      const pinned_directory & attempts,
                      const pinned_directory & staging,
                      const marker_state & marker, const marker_state & root_marker,
                      bool step4_gate, bool l05z_core_gate, bool writer_pinned,
                      const struct stat & writer, const struct stat & fixture_lock) {
    struct open_how how {};
    std::string name;
    const auto marker_fd = [&]() {
        return marker.marker_pinned && exact_fd(
            pid, state.args[0], marker.identity, marker.mount_id);
    };
    const auto admitted_fd = [&](std::uint64_t raw_fd, bool require_directory) {
        const bool exact_authority =
            exact_fd(pid, raw_fd, root, root_mount) ||
            exact_fd(pid, raw_fd, parent, parent_mount) ||
            exact_fd(pid, raw_fd, fixture, fixture_mount) ||
            (envelopes.pinned && exact_directory_fd(
                pid, static_cast<int>(raw_fd), envelopes)) ||
            (attempts.pinned && exact_directory_fd(
                pid, static_cast<int>(raw_fd), attempts)) ||
            (staging.pinned && exact_directory_fd(
                pid, static_cast<int>(raw_fd), staging)) ||
            (marker.marker_pinned && exact_fd(
                pid, raw_fd, marker.identity, marker.mount_id)) ||
            (root_marker.marker_pinned && exact_fd(
                pid, raw_fd, root_marker.identity, root_marker.mount_id)) ||
            (writer_pinned && exact_fd(pid, raw_fd, writer, root_mount)) ||
            exact_fd(pid, raw_fd, fixture_lock, fixture_mount);
        if (!exact_authority || !require_directory) return exact_authority;
        struct stat observed {};
        std::uint64_t mount = 0;
        return tracee_fd_identity(pid, static_cast<int>(raw_fd), observed, mount) &&
            S_ISDIR(observed.st_mode) && mount != 0;
    };
    switch (input.point) {
        case boundary::step4_open:
            if (!step4_gate) return false;
            if (state.nr == SYS_open) {
                if (!read_tracee_name(pid, state.args[0], name)) return false;
                return name == input.parent || name == input.root ||
                    name == input.fixture || name == "/proc/self/mountinfo";
            }
            if (state.nr != SYS_openat2 || !read_how(pid, state, how, name) ||
                how.resolve != static_cast<std::uint64_t>(
                    RESOLVE_BENEATH | RESOLVE_NO_MAGICLINKS |
                    RESOLVE_NO_SYMLINKS | RESOLVE_NO_XDEV) ||
                name.empty() || name.find('/') != std::string::npos ||
                name == "." || name == "..") return false;
            return name == "root.marker" || name == "writer.lock" ||
                name == "primitive.lock" || name == "envelopes" ||
                name == "attempts" || name == "staging" ||
                name == (std::strrchr(input.root, '/') + 1) ||
                name == (std::strrchr(input.fixture, '/') + 1) ||
                name == "initialize-envelope.tmp" ||
                (marker.basename_known && name == marker.basename.data()) ||
                valid_envelope_basename(name);
        case boundary::step4_close:
            return step4_gate && state.nr == SYS_close;
        case boundary::transient_open:
            return exact_directory_fd(pid, static_cast<int>(state.args[0]), staging) &&
                   read_how(pid, state, how, name) && name == "initialize-envelope.tmp" &&
                   how.flags == static_cast<std::uint64_t>(
                       O_CREAT | O_EXCL | O_RDWR | O_CLOEXEC | O_NOFOLLOW) &&
                   how.mode == 0600 && how.resolve ==
                       static_cast<std::uint64_t>(RESOLVE_BENEATH | RESOLVE_NO_MAGICLINKS |
                                                  RESOLVE_NO_SYMLINKS | RESOLVE_NO_XDEV);
        case boundary::transient_fchmod:
            return state.nr == SYS_fchmod && state.args[1] == 0600 && marker_fd();
        case boundary::envelope_pwrite:
            return state.nr == SYS_pwrite64 && state.args[3] <= 1024 &&
                   state.args[2] > 0 && state.args[2] <= 1024 - state.args[3] &&
                   marker_fd();
        case boundary::envelope_pread:
            return state.nr == SYS_pread64 && state.args[3] == 0 &&
                   state.args[2] == marker.encoded_size && marker.encoded_size != 0 &&
                   marker_fd();
        case boundary::envelope_pread_eof:
            return state.nr == SYS_pread64 && state.args[2] == 1 &&
                   state.args[3] == marker.encoded_size && marker.encoded_size != 0 &&
                   marker_fd();
        case boundary::envelope_fsync:
            return state.nr == SYS_fsync && marker_fd();
        case boundary::readonly_temp_open:
            return marker.transient_seen && !marker.published_seen &&
                   exact_directory_fd(pid, static_cast<int>(state.args[0]), staging) &&
                   read_how(pid, state, how, name) &&
                   name == "initialize-envelope.tmp" &&
                   (how.flags & static_cast<std::uint64_t>(O_ACCMODE)) == O_RDONLY &&
                   (how.flags & static_cast<std::uint64_t>(O_CREAT | O_TRUNC)) == 0;
        case boundary::envelope_rename: {
            std::string old_name, new_name;
            return state.nr == SYS_renameat2 && state.args[4] == RENAME_NOREPLACE &&
                   exact_directory_fd(pid, static_cast<int>(state.args[0]), staging) &&
                   exact_directory_fd(pid, static_cast<int>(state.args[2]), envelopes) &&
                   read_tracee_name(pid, state.args[1], old_name) &&
                   read_tracee_name(pid, state.args[3], new_name) &&
                   old_name == "initialize-envelope.tmp" &&
                   valid_envelope_basename(new_name) && marker.basename_known &&
                   new_name == marker.basename.data();
        }
        case boundary::envelopes_fsync:
            return marker.published_seen && state.nr == SYS_fsync &&
                   exact_directory_fd(pid, static_cast<int>(state.args[0]), envelopes);
        case boundary::staging_fsync:
            return marker.transient_seen && state.nr == SYS_fsync &&
                   exact_directory_fd(pid, static_cast<int>(state.args[0]), staging);
        case boundary::final_open:
            return marker.published_seen && marker.basename_known &&
                   exact_directory_fd(pid, static_cast<int>(state.args[0]), envelopes) &&
                   read_how(pid, state, how, name) && name == marker.basename.data() &&
                   (how.flags & static_cast<std::uint64_t>(O_ACCMODE)) == O_RDONLY &&
                   (how.flags & static_cast<std::uint64_t>(O_CREAT | O_TRUNC)) == 0;
        case boundary::final_pread:
            return marker.final_open_seen && state.nr == SYS_pread64 &&
                   state.args[3] == 0 && state.args[2] == marker.encoded_size && marker_fd();
        case boundary::final_pread_eof:
            return marker.final_open_seen && state.nr == SYS_pread64 &&
                   state.args[2] == 1 && state.args[3] == marker.encoded_size && marker_fd();
        case boundary::marker_pread:
            return step4_gate && root_marker.marker_pinned && state.nr == SYS_pread64 &&
                   state.args[3] == 0 && state.args[2] == root_marker.encoded_size &&
                   root_marker.encoded_size != 0 &&
                   exact_fd(pid, state.args[0], root_marker.identity,
                            root_marker.mount_id);

        case boundary::marker_pread_eof:
            return step4_gate && root_marker.marker_pinned && state.nr == SYS_pread64 &&
                   state.args[2] == 1 && state.args[3] == root_marker.encoded_size &&
                   root_marker.encoded_size != 0 &&
                   exact_fd(pid, state.args[0], root_marker.identity,
                            root_marker.mount_id);
        case boundary::envelope_close:
            return state.nr == SYS_close && marker_fd();
        case boundary::staging_close:

            return marker.published_seen && state.nr == SYS_close &&
                   exact_directory_fd(pid, static_cast<int>(state.args[0]), staging);
        case boundary::writer_unlock:
        case boundary::fixture_unlock: {
            if (state.nr != SYS_fcntl || state.args[1] != F_OFD_SETLK) return false;

            struct flock lock {};
            const bool exact_lock = input.point == boundary::writer_unlock
                ? writer_pinned && exact_fd(pid, state.args[0], writer, root_mount)
                : exact_fd(pid, state.args[0], fixture_lock, fixture_mount);
            return exact_lock && read_tracee(pid, state.args[2], &lock, sizeof(lock)) &&
                   lock.l_type == F_UNLCK && lock.l_whence == SEEK_SET &&
                   lock.l_start == 0 && lock.l_len == 0;
        }
        case boundary::reserve_revalidation:
            return step4_gate && state.nr == SYS_fstatfs &&
                   exact_fd(pid, state.args[0], root, root_mount);
        case boundary::final_validation:
            return marker.published_seen && marker.final_open_seen &&
                   state.nr == SYS_pread64 && state.args[3] == 0 && marker_fd();
        case boundary::fstat_call:
            return l05z_core_gate && state.nr == SYS_fstat &&
                   admitted_fd(state.args[0], false);
        case boundary::statx_call: {
            std::string empty;
            return l05z_core_gate && state.nr == SYS_statx &&
                   state.args[2] == (AT_EMPTY_PATH | AT_STATX_SYNC_AS_STAT) &&
                   read_tracee_name(pid, state.args[1], empty) && empty.empty() &&
                   admitted_fd(state.args[0], false);
        }
        case boundary::readlink_call: {
            std::string path;
            return l05z_core_gate && state.nr == SYS_readlink && state.args[2] == 4097 &&
                   read_tracee_name(pid, state.args[0], path) &&
                   path.rfind("/proc/self/fd/", 0) == 0;
        }
        case boundary::getdents_call:
            return l05z_core_gate && state.nr == SYS_getdents64 && state.args[2] == 4096 &&
                   admitted_fd(state.args[0], true);
        case boundary::facts_call:
            return l05z_core_gate && state.nr == SYS_ioctl &&
                   (state.args[1] == BTRFS_IOC_FS_INFO ||
                    state.args[1] == BTRFS_IOC_GET_SUBVOL_INFO) &&
                   admitted_fd(state.args[0], false);
        case boundary::fstatfs_call:
            return l05z_core_gate && state.nr == SYS_fstatfs &&
                   admitted_fd(state.args[0], false);
        case boundary::stdout_audit_response:
            return false;
    }
    return false;
}

bool pin_marker_fd(pid_t pid, int fd, marker_state & marker) {
    struct stat observed {};
    std::uint64_t mount = 0;
    if (!tracee_fd_identity(pid, fd, observed, mount) ||
        !S_ISREG(observed.st_mode) || observed.st_nlink != 1 ||
        observed.st_uid != ::geteuid() ||

        ((observed.st_mode & 07777) != 0000 &&
         (observed.st_mode & 07777) != 0600)) return false;
    if (marker.marker_pinned && !same_object(marker.identity, observed)) return false;
    marker.marker_pinned = true;
    marker.identity = observed;
    marker.mount_id = mount;
    if (observed.st_size > 0 && observed.st_size <= 1024) {
        marker.encoded_size = static_cast<std::size_t>(observed.st_size);
    }
    return true;
}

bool capture_pwrite_intent(pid_t pid, const tracee_state & state,
                           marker_state & marker) {
    if (state.nr != SYS_pwrite64 || !marker.marker_pinned ||
        !exact_fd(pid, state.args[0], marker.identity, marker.mount_id) ||
        state.args[3] > 1024 || state.args[2] == 0 ||
        state.args[2] > 1024 - state.args[3]) return true;
    const std::size_t offset = static_cast<std::size_t>(state.args[3]);
    const std::size_t count = static_cast<std::size_t>(state.args[2]);
    std::array<std::uint8_t, 1024> observed {};
    if (!read_tracee(pid, state.args[1], observed.data(), count)) return false;
    for (std::size_t index = 0; index < count; ++index) {
        if (marker.intended_known[offset + index] &&
            marker.intended_bytes[offset + index] != observed[index]) return false;
        marker.intended_bytes[offset + index] = observed[index];
    }
    std::fill_n(marker.intended_known.begin() + static_cast<std::ptrdiff_t>(offset),
                count, true);
    marker.encoded_size = std::max(marker.encoded_size, offset + count);
    return true;
}

bool capture_completed_pwrite(const tracee_state & state, long long result,
                              marker_state & marker) {
    if (state.nr != SYS_pwrite64 || result <= 0) return true;
    const std::size_t offset = static_cast<std::size_t>(state.args[3]);
    const std::size_t count = static_cast<std::size_t>(result);
    if (offset > 1024 || count > 1024 - offset ||
        !std::all_of(marker.intended_known.begin() +
                         static_cast<std::ptrdiff_t>(offset),
                     marker.intended_known.begin() +
                         static_cast<std::ptrdiff_t>(offset + count),
                     [](bool known) { return known; })) return false;

    std::copy_n(marker.intended_bytes.begin() +
                    static_cast<std::ptrdiff_t>(offset),
                count, marker.retained_bytes.begin() +
                           static_cast<std::ptrdiff_t>(offset));

    std::fill_n(marker.retained_known.begin() +
                    static_cast<std::ptrdiff_t>(offset),
                count, true);
    marker.retained_size = std::max(marker.retained_size, offset + count);
    return true;
}

bool exact_byte_coverage(const arguments & input, const marker_state & marker,
                         bool qualified) {
    if (marker.encoded_size == 0) {
        return marker.retained_size == 0 && !qualified;
    }
    if (marker.encoded_size > marker.intended_known.size() ||
        marker.retained_size > marker.retained_known.size() ||
        !std::all_of(marker.intended_known.begin(),
                     marker.intended_known.begin() +
                         static_cast<std::ptrdiff_t>(marker.encoded_size),
                     [](bool known) { return known; })) return false;
    if (input.injection == mode::short_pre &&
        input.point == boundary::envelope_pwrite) {
        if (marker.retained_size != marker.encoded_size ||
            input.short_count >= marker.encoded_size) return false;
        for (std::size_t index = 0; index < marker.encoded_size; ++index) {
            const bool prefix_hole = index < input.short_count;
            if (marker.retained_known[index] == prefix_hole ||
                (prefix_hole && marker.retained_bytes[index] != 0)) return false;
        }
        return !qualified;
    }
    if (marker.retained_size != 0 &&
        !std::all_of(marker.retained_known.begin(),
                     marker.retained_known.begin() +
                         static_cast<std::ptrdiff_t>(marker.retained_size),
                     [](bool known) { return known; })) return false;
    if (qualified) {
        return marker.retained_size == marker.encoded_size;
    }
    if (input.injection == mode::inode_substitute ||
        input.injection == mode::temp_inode_substitute) {
        return marker.retained_size == marker.encoded_size;
    }
    return true;
}

bool apply_retained_mutation(const arguments & input, int envelopes_fd, int staging_fd,
                             marker_state & marker) {
    if (!marker.published_seen || !marker.basename_known ||
        marker.encoded_size == 0) return false;
    if (input.injection == mode::unexpected_name) {
        struct open_how how {};
        how.flags = O_CREAT | O_EXCL | O_WRONLY | O_CLOEXEC | O_NOFOLLOW;
        how.mode = 0600;
        how.resolve = RESOLVE_BENEATH | RESOLVE_NO_MAGICLINKS |
                      RESOLVE_NO_SYMLINKS | RESOLVE_NO_XDEV;
        const int fd = static_cast<int>(::syscall(
            SYS_openat2, staging_fd, "unexpected.retained", &how, sizeof(how)));
        return fd >= 0 && ::fchmod(fd, 0600) == 0 && ::fsync(fd) == 0 &&
               ::close(fd) == 0;
    }
    if (input.injection == mode::hardlink) {
        return ::linkat(envelopes_fd, marker.basename.data(), staging_fd,
                        "predecessor-envelope.hardlink", 0) == 0;
    }
    if (input.injection == mode::symlink) {

        return ::symlinkat((std::string("../envelopes/") + marker.basename.data()).c_str(),
                           staging_fd, "predecessor-envelope.symlink") == 0;
    }
    if (input.injection == mode::inode_substitute) {
        if (::syscall(SYS_renameat2, envelopes_fd, marker.basename.data(),
                      envelopes_fd, "predecessor-envelope.original",
                      RENAME_NOREPLACE) != 0) return false;
        struct open_how how {};
        how.flags = O_CREAT | O_EXCL | O_WRONLY | O_CLOEXEC | O_NOFOLLOW;
        how.mode = 0600;

        how.resolve = RESOLVE_BENEATH | RESOLVE_NO_MAGICLINKS |
                      RESOLVE_NO_SYMLINKS | RESOLVE_NO_XDEV;
        const int replacement = static_cast<int>(::syscall(
            SYS_openat2, envelopes_fd, marker.basename.data(), &how, sizeof(how)));
        return replacement >= 0 && ::fchmod(replacement, 0600) == 0 &&
               ::fsync(replacement) == 0 && ::close(replacement) == 0;

    }
    int fd = -1;
    if (!open_named(envelopes_fd, marker.basename.data(), O_RDWR, fd)) return false;
    bool mutated = false;
    if (input.injection == mode::corrupt) {
        std::uint8_t byte = 0;
        mutated = ::pread(fd, &byte, 1, 0) == 1 &&
                  (++byte, ::pwrite(fd, &byte, 1, 0) == 1);
        if (mutated && marker.retained_size != 0 && marker.retained_known[0]) {
            ++marker.retained_bytes[0];
        }
    } else if (input.injection == mode::truncate) {
        mutated = marker.encoded_size > 1 &&
                  ::ftruncate(fd, static_cast<off_t>(marker.encoded_size - 1)) == 0;
        if (mutated) marker.retained_size = marker.encoded_size - 1;
    } else if (input.injection == mode::append) {
        const std::uint8_t trailing = 0xa5;
        mutated = ::pwrite(fd, &trailing, 1,
                           static_cast<off_t>(marker.encoded_size)) == 1;
        if (mutated) {
            marker.retained_bytes[marker.encoded_size] = trailing;
            marker.retained_known[marker.encoded_size] = true;
            marker.retained_size = marker.encoded_size + 1;
        }
    }

    const bool synchronized = mutated && ::fsync(fd) == 0;
    const bool closed = ::close(fd) == 0;
    return synchronized && closed;
}

bool apply_collision(const arguments & input, int envelopes_fd, int staging_fd,
                     const marker_state & marker) {
    const int parent = input.point == boundary::transient_open
        ? staging_fd : envelopes_fd;
    const char * name = input.point == boundary::transient_open
        ? "initialize-envelope.tmp" : marker.basename.data();
    if (input.point == boundary::envelope_rename && !marker.basename_known) return false;
    struct open_how how {};
    how.flags = O_CREAT | O_EXCL | O_WRONLY | O_CLOEXEC | O_NOFOLLOW;
    how.mode = 0600;
    how.resolve = RESOLVE_BENEATH | RESOLVE_NO_MAGICLINKS |
                  RESOLVE_NO_SYMLINKS | RESOLVE_NO_XDEV;
    const int fd = static_cast<int>(::syscall(
        SYS_openat2, parent, name, &how, sizeof(how)));
    return fd >= 0 && ::fchmod(fd, 0600) == 0 && ::fsync(fd) == 0 &&
           ::close(fd) == 0;
}

bool apply_temp_inode_substitution(int staging_fd) {
    if (::syscall(SYS_renameat2, staging_fd, "initialize-envelope.tmp", staging_fd,
                  "initialize-root.original", RENAME_NOREPLACE) != 0) return false;
    struct open_how how {};
    how.flags = O_CREAT | O_EXCL | O_WRONLY | O_CLOEXEC | O_NOFOLLOW;
    how.mode = 0600;
    how.resolve = RESOLVE_BENEATH | RESOLVE_NO_MAGICLINKS |
                  RESOLVE_NO_SYMLINKS | RESOLVE_NO_XDEV;
    const int replacement = static_cast<int>(::syscall(
        SYS_openat2, staging_fd, "initialize-envelope.tmp", &how, sizeof(how)));
    return replacement >= 0 && ::fchmod(replacement, 0600) == 0 &&
           ::fsync(replacement) == 0 && ::close(replacement) == 0;
}

bool exact_audit(const std::string & audit, bool expect_qualified,
                 const arguments & input, const marker_state & observed_marker) {
    const boundary point = input.point;
    if (audit.empty() || audit.back() != '\n' ||
        audit.find('\n') != audit.size() - 1) return false;
    std::unordered_map<std::string, std::string> fields;
    std::istringstream stream(audit.substr(0, audit.size() - 1));
    std::string token;

    while (stream >> token) {
        const std::size_t equals = token.find('=');
        if (equals == std::string::npos || equals == 0 ||
            (equals + 1 == token.size() && token != "envelope_name=") ||
            token.find('=', equals + 1) !=
                std::string::npos ||

            !fields.emplace(token.substr(0, equals), token.substr(equals + 1)).second) {
            return false;
        }
    }
    constexpr const char * required[] {
        "result", "sealed_result", "sealed_before_root", "latch", "qualified",
        "root_syscalls", "reserve", "root_id_nonzero", "store_uuid_nonzero",
        "writer_created", "writer_synced", "writer_ofd", "sole_entry",
        "writer_released", "fixture_released", "guard_released", "envelopes",
        "attempts", "staging", "final_dirs", "root_synced", "prefix_qualified",
        "path_policy_computed", "marker", "marker_sync", "marker_final",
        "marker_qualified", "root_id", "store_uuid", "path_policy",
        "marker_digest", "marker_dev", "marker_inode", "marker_mount",
        "marker_size", "marker_phase", "envelope", "envelope_digest",
        "envelope_name", "envelope_dev", "envelope_inode", "envelope_mount",
        "envelope_size", "envelope_phase",
    };
    if (fields.size() != std::size(required)) return false;
    for (const char * key : required) if (fields.find(key) == fields.end()) return false;
    const auto exact = [&](const char * key, const char * value) {
        return fields.at(key) == value;
    };
    // The production cleanup stack is layered. A close in finish_envelope
    // invalidates only the predecessor envelope, a close in finish_marker
    // invalidates only the marker, and only finish()/unlock failure invalidates
    // the lock anchor and accumulated prefix as well. The generic close
    // occurrences are the exact post-L05y production order.
    const bool envelope_cleanup_fault = !expect_qualified &&
        (point == boundary::envelope_close ||
         (point == boundary::staging_close && input.occurrence == 2) ||
         (point == boundary::step4_close && input.occurrence >= 40 &&
          input.occurrence <= 46));
    const bool marker_cleanup_fault = !expect_qualified &&
        ((point == boundary::staging_close && input.occurrence == 3) ||
         (point == boundary::step4_close && input.occurrence == 47));
    const bool outer_cleanup_fault = !expect_qualified &&
        (point == boundary::writer_unlock || point == boundary::fixture_unlock ||
         (point == boundary::step4_close && input.occurrence >= 48 &&
          input.occurrence <= 52));
    const bool envelope_unqualified =
        envelope_cleanup_fault || outer_cleanup_fault;
    if (!exact("result", "0") || !exact("sealed_result", "0") ||
        !exact("sealed_before_root", "1") || !exact("latch", "1") ||
        !exact("root_id_nonzero", "1") || !exact("store_uuid_nonzero", "1") ||
        !exact("writer_created", "1") || !exact("writer_synced", "1") ||
        !exact("writer_ofd", "1") || !exact("sole_entry", "1") ||
        !exact("guard_released", "1") || !exact("envelopes", "1/1/1") ||
        !exact("attempts", "1/1/1") || !exact("staging", "1/1/1") ||
        !exact("final_dirs", "1/1/1") || !exact("root_synced", "1") ||
        !exact("path_policy_computed", "1") ||
        !exact("qualified", outer_cleanup_fault ? "0" : "1") ||
        !exact("prefix_qualified", outer_cleanup_fault ? "0" : "1") ||
        !exact("writer_released",
               point == boundary::writer_unlock && !expect_qualified ? "0" : "1") ||
        !exact("fixture_released",
               point == boundary::fixture_unlock && !expect_qualified ? "0" : "1") ||
        !exact("marker_qualified",
               marker_cleanup_fault || outer_cleanup_fault ? "0" : "1"))
        return false;
    const auto parse_bits = [](const std::string & value, std::size_t count,
                               auto & output) {
        if (count > output.size()) return false;
        std::size_t begin = 0;
        for (std::size_t index = 0; index < count; ++index) {
            if (begin >= value.size() || (value[begin] != '0' && value[begin] != '1'))
                return false;
            output[index] = static_cast<unsigned>(value[begin] - '0');
            ++begin;
            if (index + 1 != count) {
                if (begin >= value.size() || value[begin] != '/') return false;
                ++begin;
            }

        }
        return begin == value.size();
    };
    std::array<unsigned, 10> marker_bits {}, sync_bits {}, final_bits {};
    if (!parse_bits(fields.at("marker"), 10, marker_bits) ||
        !parse_bits(fields.at("marker_sync"), 4, sync_bits) ||
        !parse_bits(fields.at("marker_final"), 2, final_bits)) return false;
    for (std::size_t index = 1; index < marker_bits.size(); ++index) {
        if (marker_bits[index] > marker_bits[index - 1]) return false;
    }
    if (marker_bits[0] != 1 || marker_bits[1] != 1 || marker_bits[2] != 1 ||

        (sync_bits[0] && !marker_bits[8]) ||
        (sync_bits[1] && !sync_bits[0]) ||
        (marker_bits[9] && !sync_bits[1]) ||
        (sync_bits[2] && !marker_bits[9]) ||
        (sync_bits[3] && !sync_bits[2]) ||
        (final_bits[0] && !sync_bits[3]) ||
        (final_bits[1] && !final_bits[0])) return false;

    const unsigned expected_phase = 13;
    const unsigned boundary_phase = 13;
    unsigned marker_size = 0, marker_phase = 0, root_syscalls = 0;
    std::uint64_t reserve = 0;
    const auto parse_number = [](const std::string & value, std::uint64_t maximum,
                                 std::uint64_t & output) {
        if (value.empty() || value[0] == '-') return false;
        errno = 0;
        char * end = nullptr;
        const unsigned long long parsed = std::strtoull(value.c_str(), &end, 10);
        if (errno != 0 || end == value.c_str() || *end != '\0' || parsed > maximum)
            return false;
        output = static_cast<std::uint64_t>(parsed);
        return true;
    };
    std::uint64_t parsed = 0;
    if (!parse_number(fields.at("marker_size"), 1024, parsed) || parsed == 0) return false;
    marker_size = static_cast<unsigned>(parsed);
    if (!parse_number(fields.at("marker_phase"), 13, parsed)) return false;
    marker_phase = static_cast<unsigned>(parsed);
    if (!parse_number(fields.at("root_syscalls"), UINT32_MAX, parsed) || parsed == 0)
        return false;
    root_syscalls = static_cast<unsigned>(parsed);
    if (!parse_number(fields.at("reserve"), UINT64_MAX, reserve) ||
        marker_phase != expected_phase || marker_phase != boundary_phase ||
        marker_size == 0 || root_syscalls == 0) return false;

    (void) reserve;
    const auto exact_hex = [](const std::string & value, std::size_t length) {
        return value.size() == length &&
            std::all_of(value.begin(), value.end(), [](char byte) {
                return (byte >= '0' && byte <= '9') || (byte >= 'a' && byte <= 'f');
            }) && value.find_first_not_of('0') != std::string::npos;
    };
    if (!exact_hex(fields.at("root_id"), 64) ||
        !exact_hex(fields.at("store_uuid"), 32) ||
        !exact_hex(fields.at("path_policy"), 64) ||
        !exact_hex(fields.at("marker_digest"), 64)) return false;
    std::uint64_t marker_device = 0, marker_inode = 0, marker_mount = 0;

    if (!parse_number(fields.at("marker_dev"), UINT64_MAX, marker_device) ||
        !parse_number(fields.at("marker_inode"), UINT64_MAX, marker_inode) ||
        !parse_number(fields.at("marker_mount"), UINT64_MAX, marker_mount)) return false;
    const bool final_identity = marker_device != 0 && marker_inode != 0 &&
        marker_mount != 0;
    if (final_identity != (final_bits[0] == 1 && final_bits[1] == 1)) return false;

    std::array<unsigned, 10> expected_marker {};
    expected_marker[0] = expected_marker[1] = expected_marker[2] = 1;
    if (boundary_phase >= 5) expected_marker[3] = 1;
    if (boundary_phase >= 6) expected_marker[4] = 1;
    if (boundary_phase >= 7) expected_marker[5] = 1;
    if (boundary_phase >= 8) expected_marker[6] = expected_marker[7] = 1;
    if (boundary_phase >= 9) expected_marker[8] = 1;
    if (boundary_phase >= 10) expected_marker[9] = 1;
    std::array<unsigned, 10> expected_sync {};
    if (boundary_phase >= 10 || point == boundary::envelope_rename) {
        expected_sync[0] = expected_sync[1] = 1;
    }
    if (boundary_phase >= 11) expected_sync[2] = 1;
    if (boundary_phase >= 12) expected_sync[3] = 1;
    std::array<unsigned, 10> expected_final {};
    if (boundary_phase >= 13) expected_final[0] = expected_final[1] = 1;
    if (marker_bits != expected_marker ||
        !std::equal(sync_bits.begin(), sync_bits.begin() + 4,
                    expected_sync.begin()) ||
        final_bits[0] != expected_final[0] ||
        final_bits[1] != expected_final[1]) return false;
    std::array<unsigned, 21> envelope_bits {};
    if (!parse_bits(fields.at("envelope"), envelope_bits.size(), envelope_bits))
        return false;
    std::uint64_t envelope_dev = 0, envelope_inode = 0, envelope_mount = 0,
                  envelope_size = 0, envelope_phase = 0;
    if (!parse_number(fields.at("envelope_dev"), UINT64_MAX, envelope_dev) ||
        !parse_number(fields.at("envelope_inode"), UINT64_MAX, envelope_inode) ||
        !parse_number(fields.at("envelope_mount"), UINT64_MAX, envelope_mount) ||
        !parse_number(fields.at("envelope_size"), 1024, envelope_size) ||
        !parse_number(fields.at("envelope_phase"), 13, envelope_phase) ||
        (envelope_phase >= 2 && !exact_hex(fields.at("envelope_digest"), 64)) ||
        (envelope_phase < 2 && fields.at("envelope_digest") != std::string(64, '0')) ||
        (envelope_phase >= 2 && !valid_envelope_basename(fields.at("envelope_name"))))
        return false;
    const bool envelope_identity = envelope_dev != 0 && envelope_inode != 0 &&
        envelope_mount != 0;
    if (envelope_identity != (envelope_bits[19] == 1) ||
        (envelope_phase >= 2 && envelope_size == 0)) return false;
    if (observed_marker.basename_known &&
        fields.at("envelope_name") != observed_marker.basename.data()) return false;
    if (observed_marker.basename_known &&
        fields.at("envelope_digest") !=
            std::string(observed_marker.basename.data() + 2, 64)) return false;
    const std::array<std::size_t, 14> completed_by_phase {
        0, 1, 2, 4, 6, 7, 8, 9, 11, 12, 15, 16, 17, 21,
    };
    if (envelope_phase > 13) return false;
    const std::size_t completed = envelope_phase == 13 && envelope_unqualified
        ? 20 : completed_by_phase[envelope_phase];
    if (!std::all_of(envelope_bits.begin(), envelope_bits.begin() +
                         static_cast<std::ptrdiff_t>(completed),
                     [](unsigned bit) { return bit == 1; })) return false;
    if (envelope_phase < 9 &&
        !std::all_of(envelope_bits.begin() +
                         static_cast<std::ptrdiff_t>(completed),
                     envelope_bits.end(), [](unsigned bit) { return bit == 0; }))
        return false;
    if (envelope_phase == 9 &&
        (!std::all_of(envelope_bits.begin(), envelope_bits.begin() + 12,
                      [](unsigned bit) { return bit == 1; }) ||
         envelope_bits[14] || envelope_bits[15] || envelope_bits[16] ||
         envelope_bits[17] || envelope_bits[18] || envelope_bits[19] ||
         envelope_bits[20] || (envelope_bits[13] && !envelope_bits[12])))
        return false;
    if (envelope_phase >= 10 && envelope_phase <= 11 &&
        !std::all_of(envelope_bits.begin() +
                         static_cast<std::ptrdiff_t>(completed),
                     envelope_bits.end(), [](unsigned bit) { return bit == 0; }))
        return false;
    if (envelope_phase == 12 &&
        (envelope_bits[20] || (envelope_bits[18] && !envelope_bits[17]) ||
         (envelope_bits[19] && !envelope_bits[18]))) return false;
    if (expect_qualified &&
        (!std::all_of(envelope_bits.begin(), envelope_bits.end(),
                      [](unsigned bit) { return bit == 1; }) ||
         envelope_phase != 13 || !envelope_identity ||
         envelope_size != observed_marker.encoded_size)) return false;
    if (envelope_unqualified && envelope_phase == 13 &&
        (!std::all_of(envelope_bits.begin(), envelope_bits.begin() + 20,
                      [](unsigned bit) { return bit == 1; }) ||
         envelope_bits[20] != 0 || envelope_phase != 13 ||
         !envelope_identity || envelope_size != observed_marker.encoded_size))
        return false;
    if (expect_qualified) {
        return std::all_of(marker_bits.begin(), marker_bits.end(),
                           [](unsigned bit) { return bit == 1; }) &&
               std::all_of(sync_bits.begin(), sync_bits.begin() + 4,
                           [](unsigned bit) { return bit == 1; }) &&
               final_bits[0] == 1 && final_bits[1] == 1 && marker_phase == 13;
    }
    return true;
}

bool list_names(int fd, std::vector<std::string> & names) {
    const int duplicate = ::openat(
        fd, ".", O_RDONLY | O_DIRECTORY | O_CLOEXEC | O_NOFOLLOW);
    if (duplicate < 0) return false;
    DIR * stream = ::fdopendir(duplicate);
    if (!stream) { (void) ::close(duplicate); return false; }
    names.clear();
    errno = 0;
    for (;;) {
        dirent * entry = ::readdir(stream);
        if (!entry) break;
        if (std::strcmp(entry->d_name, ".") != 0 &&
            std::strcmp(entry->d_name, "..") != 0) names.emplace_back(entry->d_name);
        errno = 0;
    }
    const bool ok = errno == 0;
    const bool closed = ::closedir(stream) == 0;
    std::sort(names.begin(), names.end());
    return ok && closed;
}

bool exact_file_bytes(int fd, const marker_state & marker,
                      std::size_t expected_size) {
    if (expected_size > marker.retained_bytes.size()) return false;
    std::array<std::uint8_t, 1025> observed {};
    std::size_t total = 0;
    while (total < expected_size) {

        ssize_t count;
        do {
            count = ::pread(fd, observed.data() + total, expected_size - total,
                            static_cast<off_t>(total));
        } while (count < 0 && errno == EINTR);
        if (count <= 0) return false;
        total += static_cast<std::size_t>(count);
    }
    std::uint8_t trailing = 0;
    ssize_t trailing_count;
    do {
        trailing_count = ::pread(fd, &trailing, 1,

                                 static_cast<off_t>(expected_size));
    } while (trailing_count < 0 && errno == EINTR);
    return trailing_count == 0 &&
           std::memcmp(observed.data(), marker.retained_bytes.data(),
                       expected_size) == 0;
}

bool snapshot_root_marker(int root_fd, marker_state & marker) {
    int fd = -1;
    if (!open_named(root_fd, "root.marker", O_RDONLY, fd)) return false;
    struct stat observed {};
    std::uint64_t mount = 0;
    const bool identity = ::fstat(fd, &observed) == 0 &&
        fd_mount_id(fd, mount) && S_ISREG(observed.st_mode) &&
        observed.st_nlink == 1 && observed.st_uid == ::geteuid() &&
        (observed.st_mode & 07777) == 0600 && observed.st_size > 0 &&
        observed.st_size <= 1024 &&
        (!marker.marker_pinned ||
         (same_object(marker.identity, observed) && marker.mount_id == mount));
    if (!identity) {
        if (fd >= 0) (void) ::close(fd);
        return false;
    }
    marker.marker_pinned = true;
    marker.identity = observed;
    marker.mount_id = mount;
    marker.encoded_size = static_cast<std::size_t>(observed.st_size);
    marker.retained_size = marker.encoded_size;
    std::size_t total = 0;
    while (total < marker.encoded_size) {
        ssize_t count;
        do {
            count = ::pread(fd, marker.retained_bytes.data() + total,
                            marker.encoded_size - total,
                            static_cast<off_t>(total));
        } while (count < 0 && errno == EINTR);
        if (count <= 0) {
            (void) ::close(fd);
            return false;
        }
        total += static_cast<std::size_t>(count);
    }
    std::fill_n(marker.retained_known.begin(), marker.encoded_size, true);
    std::uint8_t trailing = 0;
    ssize_t trailing_count;
    do {
        trailing_count = ::pread(fd, &trailing, 1,
                                 static_cast<off_t>(marker.encoded_size));
    } while (trailing_count < 0 && errno == EINTR);
    return trailing_count == 0 && ::close(fd) == 0;
}

bool exact_inventory(int root_fd, const marker_state & marker,
                     const marker_state & root_marker,
                     const arguments & input, const struct stat & writer,
                     bool writer_pinned,
                     const std::array<pinned_directory, 3> & directories,
                     bool & whole_root_discard) {
    int envelopes_fd = -1;
    int staging_fd = -1;
    if (!open_named(root_fd, "envelopes", O_RDONLY | O_DIRECTORY, envelopes_fd) ||
        !open_named(root_fd, "staging", O_RDONLY | O_DIRECTORY, staging_fd)) {
        return false;
    }
    if (envelopes_fd < 0 || staging_fd < 0) return false;
    std::vector<std::string> root_names, envelope_names, staging_names;
    const bool listed = list_names(root_fd, root_names) &&
        list_names(envelopes_fd, envelope_names) &&
        list_names(staging_fd, staging_names);
    std::vector<std::string> expected_root {
        "attempts", "envelopes", "root.marker", "staging", "writer.lock"
    };
    std::vector<std::string> expected_envelopes;
    std::vector<std::string> expected_staging;
    if (marker.published_seen && marker.basename_known)
        expected_envelopes.emplace_back(marker.basename.data());
    else if (marker.transient_seen) expected_staging.emplace_back("initialize-envelope.tmp");
    if (input.injection == mode::collision &&
        input.point == boundary::envelope_rename && marker.basename_known)
        expected_envelopes.emplace_back(marker.basename.data());
    if (input.injection == mode::unexpected_name)

        expected_staging.emplace_back("unexpected.retained");
    if (input.injection == mode::hardlink)
        expected_staging.emplace_back("predecessor-envelope.hardlink");
    if (input.injection == mode::symlink)
        expected_staging.emplace_back("predecessor-envelope.symlink");
    if (input.injection == mode::inode_substitute)
        expected_envelopes.emplace_back("predecessor-envelope.original");
    if (input.injection == mode::temp_inode_substitute)
        expected_staging.emplace_back("initialize-envelope.original");
    std::sort(expected_root.begin(), expected_root.end());
    std::sort(expected_envelopes.begin(), expected_envelopes.end());
    std::sort(expected_staging.begin(), expected_staging.end());
    int writer_fd = -1;
    struct stat observed_writer {};
    std::uint64_t writer_mount = 0, root_mount = 0;
    const bool writer_shape = writer_pinned &&
        fd_mount_id(root_fd, root_mount) &&
        open_named(root_fd, "writer.lock", O_RDONLY, writer_fd) &&
        ::fstat(writer_fd, &observed_writer) == 0 &&
        fd_mount_id(writer_fd, writer_mount) &&
        same_object(writer, observed_writer) && writer_mount == root_mount &&
        observed_writer.st_nlink == 1 &&
        observed_writer.st_size == 0 && observed_writer.st_uid == ::geteuid() &&
        (observed_writer.st_mode & 07777) == 0600;
    const bool writer_closed = writer_fd >= 0 && ::close(writer_fd) == 0;
    bool directory_shapes = true;
    std::array<bool, 3> directory_shape_bits {};
    for (int index = 0; index < 3; ++index) {
        int fd = -1;
        struct stat observed {};
        std::uint64_t mount = 0;
        const auto & expected = directories[static_cast<std::size_t>(index)];
        const bool opened = open_named(
            root_fd, directory_names[static_cast<std::size_t>(index)],
            O_RDONLY | O_DIRECTORY, fd);
        bool shape = expected.pinned && opened &&
            ::fstat(fd, &observed) == 0 && fd_mount_id(fd, mount) &&
            S_ISDIR(observed.st_mode) && same_object(expected.identity, observed) &&
            mount == expected.mount_id && mount == root_mount &&
            observed.st_uid == ::geteuid() &&

            (observed.st_mode & 07777) == 0700 &&
            (index != 1 || directory_empty(fd));
        const bool closed = fd >= 0 && ::close(fd) == 0;
        directory_shape_bits[static_cast<std::size_t>(index)] = shape && closed;
        directory_shapes = directory_shapes && shape && closed;
    }
    bool extra_shape = true;

    if (input.injection == mode::unexpected_name) {
        struct stat extra {};
        extra_shape = ::fstatat(staging_fd, "unexpected.retained", &extra,
                                AT_SYMLINK_NOFOLLOW) == 0 &&
            S_ISREG(extra.st_mode) && extra.st_nlink == 1 && extra.st_size == 0 &&
            extra.st_uid == ::geteuid() && (extra.st_mode & 07777) == 0600;
    } else if (input.injection == mode::hardlink) {
        struct stat extra {};
        extra_shape = ::fstatat(staging_fd, "predecessor-envelope.hardlink", &extra,
                                AT_SYMLINK_NOFOLLOW) == 0 &&
            S_ISREG(extra.st_mode) && same_object(marker.identity, extra) &&
            extra.st_nlink == 2;
    } else if (input.injection == mode::symlink) {
        struct stat extra {};
        std::array<char, 4097> target {};
        const std::string expected_target = std::string("../envelopes/") + marker.basename.data();
        extra_shape = ::fstatat(staging_fd, "predecessor-envelope.symlink", &extra,
                                AT_SYMLINK_NOFOLLOW) == 0 && S_ISLNK(extra.st_mode) &&
            ::readlinkat(staging_fd, "predecessor-envelope.symlink", target.data(),
                         target.size()) ==
                static_cast<ssize_t>(expected_target.size()) &&
            std::memcmp(target.data(), expected_target.data(),
                        expected_target.size()) == 0;
    }
    bool marker_shape = true;
    if (marker.transient_seen || marker.published_seen) {
        const char * name = marker.published_seen
            ? marker.basename.data() : "initialize-envelope.tmp";
        const int parent = marker.published_seen ? envelopes_fd : staging_fd;
        int fd = -1;
        struct stat observed {};
        std::uint64_t mount = 0;
        mode_t expected_mode = 0600;
        if (!marker.published_seen &&
            ((input.point == boundary::transient_open &&
              input.injection == mode::late_error) ||
             (input.point == boundary::transient_fchmod &&
              (input.injection == mode::pre_error ||
               input.injection == mode::eintr_once)))) expected_mode = 0000;
        std::size_t expected_size = marker.retained_size;
        if (input.injection == mode::inode_substitute ||
            input.injection == mode::temp_inode_substitute) expected_size = 0;
        const nlink_t expected_links = input.injection == mode::hardlink ? 2 : 1;
        marker_shape = open_named(parent, name, O_RDONLY, fd) &&
            ::fstat(fd, &observed) == 0 && fd_mount_id(fd, mount) &&
            S_ISREG(observed.st_mode) && observed.st_uid == ::geteuid() &&
            observed.st_nlink == expected_links &&
            (observed.st_mode & 07777) == expected_mode &&
            observed.st_size == static_cast<off_t>(expected_size) &&
            exact_file_bytes(fd, marker, expected_size) &&
            ((input.injection == mode::inode_substitute ||

              input.injection == mode::temp_inode_substitute ||
              (input.injection == mode::collision &&
               input.point == boundary::transient_open)) ||
             (same_object(marker.identity, observed) && mount == marker.mount_id));
        if (fd >= 0) marker_shape = (::close(fd) == 0) && marker_shape;
        if (input.injection == mode::inode_substitute) {
            int original_fd = -1;
            struct stat original {};
            std::uint64_t original_mount = 0;
            const bool original_ok = open_named(
                    envelopes_fd, "predecessor-envelope.original", O_RDONLY, original_fd) &&
                ::fstat(original_fd, &original) == 0 &&
                fd_mount_id(original_fd, original_mount) &&

                same_object(marker.identity, original) &&
                original_mount == marker.mount_id &&
                original.st_size == static_cast<off_t>(marker.encoded_size) &&
                exact_file_bytes(original_fd, marker, marker.retained_size) &&
                !same_object(original, observed);
            if (original_fd >= 0) marker_shape =
                (::close(original_fd) == 0) && original_ok && marker_shape;
            else marker_shape = false;
        }

        if (input.injection == mode::temp_inode_substitute) {
            int original_fd = -1;
            struct stat original {};
            std::uint64_t original_mount = 0;
            const bool original_ok = open_named(
                    staging_fd, "initialize-envelope.original", O_RDONLY, original_fd) &&
                ::fstat(original_fd, &original) == 0 &&
                fd_mount_id(original_fd, original_mount) &&
                same_object(marker.identity, original) &&
                original_mount == marker.mount_id &&
                original.st_size == static_cast<off_t>(marker.retained_size) &&
                exact_file_bytes(original_fd, marker, marker.retained_size) &&
                !same_object(original, observed);
            if (original_fd >= 0) marker_shape =
                (::close(original_fd) == 0) && original_ok && marker_shape;
            else marker_shape = false;
        }
        if (input.injection == mode::collision &&
            input.point == boundary::envelope_rename) {
            int collision_fd = -1;
            struct stat collision {};
            std::uint64_t collision_mount = 0;
            const bool collision_ok = open_named(
                    envelopes_fd, marker.basename.data(), O_RDONLY, collision_fd) &&
                ::fstat(collision_fd, &collision) == 0 &&
                fd_mount_id(collision_fd, collision_mount) &&
                S_ISREG(collision.st_mode) && collision.st_nlink == 1 &&
                collision.st_size == 0 && collision.st_uid == ::geteuid() &&

                (collision.st_mode & 07777) == 0600 &&
                collision_mount == marker.mount_id &&
                !same_object(marker.identity, collision);
            if (collision_fd >= 0) marker_shape =
                (::close(collision_fd) == 0) && collision_ok && marker_shape;
            else marker_shape = false;
        }
    }
    int root_marker_fd = -1;
    struct stat root_marker_observed {};
    std::uint64_t root_marker_mount = 0;
    const bool root_marker_shape = root_marker.marker_pinned &&
        open_named(root_fd, "root.marker", O_RDONLY, root_marker_fd) &&
        ::fstat(root_marker_fd, &root_marker_observed) == 0 &&
        fd_mount_id(root_marker_fd, root_marker_mount) &&
        S_ISREG(root_marker_observed.st_mode) &&
        root_marker_observed.st_nlink == 1 &&
        root_marker_observed.st_uid == ::geteuid() &&
        (root_marker_observed.st_mode & 07777) == 0600 &&
        same_object(root_marker.identity, root_marker_observed) &&
        root_marker.mount_id == root_marker_mount &&
        root_marker_observed.st_size ==
            static_cast<off_t>(root_marker.encoded_size) &&
        exact_file_bytes(root_marker_fd, root_marker,
                         root_marker.encoded_size);
    const bool root_marker_closed = root_marker_fd >= 0 &&
        ::close(root_marker_fd) == 0;
    const bool envelopes_closed = ::close(envelopes_fd) == 0;
    const bool closed = ::close(staging_fd) == 0;
    whole_root_discard = listed && writer_shape && writer_closed &&
        directory_shapes && extra_shape && root_names == expected_root &&
        envelope_names == expected_envelopes &&
        staging_names == expected_staging && marker_shape &&
        root_marker_shape && root_marker_closed && envelopes_closed;
    if (!whole_root_discard) {
        (void) emit(std::string("\"event\":\"inventory-detail\",\"listed\":") +
            (listed ? "true" : "false") + ",\"writer_shape\":" +
            (writer_shape ? "true" : "false") + ",\"writer_closed\":" +
            (writer_closed ? "true" : "false") + ",\"directory_shapes\":" +
            (directory_shapes ? "true" : "false") + ",\"extra_shape\":" +
            (extra_shape ? "true" : "false") + ",\"directory_shape_bits\":\"" +
            (directory_shape_bits[0] ? "1" : "0") + "/" +
            (directory_shape_bits[1] ? "1" : "0") + "/" +
            (directory_shape_bits[2] ? "1" : "0") + "\",\"extra_shape_repeat\":" +
            (extra_shape ? "true" : "false") + ",\"root_names\":" +
            (root_names == expected_root ? "true" : "false") +
            ",\"root_name_count\":" + std::to_string(root_names.size()) +
            ",\"expected_root_name_count\":" +
            std::to_string(expected_root.size()) +
            ",\"staging_names\":" +
            (staging_names == expected_staging ? "true" : "false") +
            ",\"marker_shape\":" + (marker_shape ? "true" : "false"));
    }
    return whole_root_discard && closed;
}

bool open_receipt(const arguments & input, int & parent_fd,
                  struct stat & parent_identity, std::uint64_t & parent_mount,
                  std::string & name) {
    std::string parent_path;
    if (!split_receipt_path(input.receipt, parent_path, name) ||
        !canonical_existing_path(parent_path.c_str()) ||
        same_or_descendant(parent_path.c_str(), input.parent) ||
        same_or_descendant(input.parent, parent_path.c_str())) return false;

    parent_fd = open_absolute_directory_no_symlink(parent_path.c_str());
    if (parent_fd < 0 || ::fstat(parent_fd, &parent_identity) != 0 ||
        !S_ISDIR(parent_identity.st_mode) || parent_identity.st_uid != ::geteuid() ||
        (parent_identity.st_mode & 07777) != 0700 ||
        !fd_mount_id(parent_fd, parent_mount)) return false;
    struct open_how how {};
    how.flags = O_WRONLY | O_CREAT | O_EXCL | O_CLOEXEC | O_NOFOLLOW;
    how.mode = 0600;

    how.resolve = RESOLVE_BENEATH | RESOLVE_NO_MAGICLINKS |
                  RESOLVE_NO_SYMLINKS | RESOLVE_NO_XDEV;
    receipt_fd = static_cast<int>(::syscall(
        SYS_openat2, parent_fd, name.c_str(), &how, sizeof(how)));
    struct stat receipt {};
    return receipt_fd >= 0 && ::fchmod(receipt_fd, 0600) == 0 &&
           ::fstat(receipt_fd, &receipt) == 0 && S_ISREG(receipt.st_mode) &&
           receipt.st_nlink == 1 && receipt.st_size == 0 &&
           receipt.st_uid == ::geteuid() && (receipt.st_mode & 07777) == 0600;
}

int run(const arguments & input) {
    if (!retry_window_self_check() || !manifest_self_check()) return 2;
    const bool response_mode_requested =
        input.injection == mode::response_loss_full;
    if (!canonical_existing_path(input.target) || !canonical_existing_path(input.golden) ||
        !canonical_existing_path(input.parent) || !canonical_existing_path(input.root) ||
        !canonical_existing_path(input.fixture) || !direct_child(input.root, input.parent) ||
        !direct_child(input.fixture, input.parent)) {
        std::fprintf(stderr, "non-canonical authority scope\n");
        return 2;
    }
    marker_state golden_envelope {};
    if (!load_golden_envelope(input.golden, golden_envelope)) return 2;
    receipt_ok = true;
    receipt_sequence = 0;
    int receipt_parent_fd = -1;
    struct stat receipt_parent_identity {};
    std::uint64_t receipt_parent_mount = 0;
    std::string receipt_name;
    if (!open_receipt(input, receipt_parent_fd, receipt_parent_identity,
                      receipt_parent_mount, receipt_name)) return 2;

    const int target_fd = ::open(input.target, O_RDONLY | O_CLOEXEC | O_NOFOLLOW);
    const int parent_fd = ::open(input.parent, O_RDONLY | O_DIRECTORY | O_CLOEXEC | O_NOFOLLOW);
    const int root_fd = ::open(input.root, O_RDONLY | O_DIRECTORY | O_CLOEXEC | O_NOFOLLOW);
    const int fixture_fd = ::open(input.fixture, O_RDONLY | O_DIRECTORY | O_CLOEXEC | O_NOFOLLOW);
    int envelopes_fd = -1;
    int staging_fd = -1;
    const int fixture_lock_fd = fixture_fd >= 0
        ? ::openat(fixture_fd, "primitive.lock", O_RDONLY | O_CLOEXEC | O_NOFOLLOW) : -1;
    struct stat target_identity {}, parent_identity {}, root_identity {}, fixture_identity {};
    struct stat fixture_lock_identity {};
    std::uint64_t parent_mount = 0, root_mount = 0, fixture_mount = 0;
    if (target_fd < 0 || ::fstat(target_fd, &target_identity) != 0 ||
        !S_ISREG(target_identity.st_mode) || (target_identity.st_mode & 0111) == 0 ||
        parent_fd < 0 || ::fstat(parent_fd, &parent_identity) != 0 ||

        !S_ISDIR(parent_identity.st_mode) || parent_identity.st_uid != ::geteuid() ||
        (parent_identity.st_mode & 07777) != 0700 ||
        !fd_mount_id(parent_fd, parent_mount) || root_fd < 0 ||
        ::fstat(root_fd, &root_identity) != 0 || !fd_mount_id(root_fd, root_mount) ||
        !S_ISDIR(root_identity.st_mode) || root_identity.st_uid != ::geteuid() ||
        (root_identity.st_mode & 07777) != 0700 ||
        fixture_fd < 0 || ::fstat(fixture_fd, &fixture_identity) != 0 ||
        !S_ISDIR(fixture_identity.st_mode) || fixture_identity.st_uid != ::geteuid() ||
        (fixture_identity.st_mode & 07777) != 0700 ||
        !fd_mount_id(fixture_fd, fixture_mount) || fixture_lock_fd < 0 ||
        ::fstat(fixture_lock_fd, &fixture_lock_identity) != 0 ||
        !S_ISREG(fixture_lock_identity.st_mode) || fixture_lock_identity.st_nlink != 1 ||
        fixture_lock_identity.st_size != 0 || fixture_lock_identity.st_uid != ::geteuid() ||
        (fixture_lock_identity.st_mode & 07777) != 0600) return 2;

    (void) ::close(fixture_lock_fd);
    pinned_directory envelopes {}, staging {};
    std::array<pinned_directory, 3> directories {};
    if (!emit(std::string("\"event\":\"start\",\"boundary\":\"") +
              boundary_name(input.point) + "\",\"mode\":\"" +
              mode_name(input.injection) + "\",\"occurrence\":" +
              std::to_string(input.occurrence))) return 2;

    int audit_pipe[2] { -1, -1 };
    if (::pipe2(audit_pipe, O_CLOEXEC) != 0) return 2;
    struct stat response_pipe_identity {}, response_pipe_write_identity {};
    std::uint64_t response_pipe_mount = 0, response_pipe_write_mount = 0;
    if (response_mode_requested &&
        (::fstat(audit_pipe[0], &response_pipe_identity) != 0 ||
         ::fstat(audit_pipe[1], &response_pipe_write_identity) != 0 ||
         !S_ISFIFO(response_pipe_identity.st_mode) ||
         !S_ISFIFO(response_pipe_write_identity.st_mode) ||
         !same_object(response_pipe_identity, response_pipe_write_identity) ||
         !fd_mount_id(audit_pipe[0], response_pipe_mount) ||
         !fd_mount_id(audit_pipe[1], response_pipe_write_mount) ||
         response_pipe_mount == 0 ||
         response_pipe_mount != response_pipe_write_mount)) {
        return 2;
    }

    const pid_t launcher = ::fork();
    if (launcher < 0) return 2;
    if (launcher == 0) {
        (void) ::close(audit_pipe[0]);
        if (::dup2(audit_pipe[1], STDOUT_FILENO) < 0) _exit(125);
        (void) ::close(audit_pipe[1]);
        if (::ptrace(PTRACE_TRACEME, 0, nullptr, nullptr) != 0) _exit(125);
        if (::raise(SIGSTOP) != 0) _exit(125);
        char executable[64];
        const int size = std::snprintf(executable, sizeof(executable),
                                       "/proc/self/fd/%d", target_fd);
        if (size <= 0 || static_cast<std::size_t>(size) >= sizeof(executable)) _exit(125);
        ::execl(executable, input.target, "--live-envelope-controller",
                input.golden, input.parent, input.root, input.fixture,
                static_cast<char *>(nullptr));
        _exit(127);
    }
    (void) ::close(audit_pipe[1]);
    (void) ::close(target_fd);

    int status = 0;
    if (::waitpid(launcher, &status, 0) != launcher || !WIFSTOPPED(status)) return 2;
    constexpr long ptrace_options = PTRACE_O_TRACESYSGOOD | PTRACE_O_TRACEFORK |
        PTRACE_O_TRACEVFORK | PTRACE_O_TRACECLONE | PTRACE_O_TRACEEXEC | PTRACE_O_EXITKILL;
    if (::ptrace(PTRACE_SETOPTIONS, launcher, nullptr,
                 reinterpret_cast<void *>(ptrace_options)) != 0 ||
        !resume_syscalls(launcher)) return 2;

    std::unordered_map<pid_t, tracee_state> tracees;

    std::unordered_map<pid_t, bool> selected;
    tracees.emplace(launcher, tracee_state {});
    selected.emplace(launcher, false);
    fault_state fault {};
    response_state response {};
    marker_state marker = golden_envelope, root_marker {};
    pid_t live_child = -1;
    pid_t response_expected_child = -1;
    int launcher_exit = -1, child_exit = -1;
    bool launcher_exec = false, child_exec = false, controller_error = false;
    unsigned controller_error_line = 0;
#define HALOFPX_CONTROLLER_ERROR() do { controller_error = true; controller_error_line = __LINE__; } while (false)
    bool bounded_cleanup = false, writer_pinned = false;
    bool inherited_marker_published = false;
    bool inherited_root_synced = false, inherited_staging_synced = false;
    bool inherited_final_marker_data = false, inherited_final_marker_eof = false;
    bool step4_gate = false, l05z_core_gate = false;
    unsigned staging_eof_after_inherited_marker = 0;
    bool cleanup_syscall_seen = false;
    unsigned inherited_marker_publication_attempts = 0;
    unsigned publication_attempts = 0;
    struct stat writer_identity {};
    const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(30);

    while (!tracees.empty() && std::chrono::steady_clock::now() < deadline) {
        const pid_t pid = ::waitpid(-1, &status, __WALL | WNOHANG);
        if (pid == 0) {
            const struct timespec pause { 0, 1000000 };
            (void) ::nanosleep(&pause, nullptr);
            continue;
        }
        if (pid < 0) {
            if (errno == EINTR) continue;
            if (errno == ECHILD) break;
            HALOFPX_CONTROLLER_ERROR();
            break;
        }
        if (WIFEXITED(status) || WIFSIGNALED(status)) {
            const int code = WIFEXITED(status) ? WEXITSTATUS(status) : 128 + WTERMSIG(status);
            if (pid == launcher) launcher_exit = code;
            if (pid == live_child) child_exit = code;
            tracees.erase(pid);
            selected.erase(pid);
            continue;
        }
        if (!WIFSTOPPED(status)) continue;
        const int signal = WSTOPSIG(status);
        const unsigned event = static_cast<unsigned>(status) >> 16;
        if (signal == SIGTRAP && event != 0) {
            if (event == PTRACE_EVENT_FORK || event == PTRACE_EVENT_VFORK ||
                event == PTRACE_EVENT_CLONE) {
                unsigned long child_value = 0;
                if (::ptrace(PTRACE_GETEVENTMSG, pid, nullptr, &child_value) != 0) {
                    HALOFPX_CONTROLLER_ERROR();
                    break;
                }

                tracees[static_cast<pid_t>(child_value)].initial_sigstop_expected = true;
                selected[static_cast<pid_t>(child_value)] = false;
                if (response_mode_requested) {
                    if (event != PTRACE_EVENT_FORK || pid != launcher ||
                        !launcher_exec || response_expected_child != -1 ||
                        child_value == 0 || child_value >
                            static_cast<unsigned long>(
                                std::numeric_limits<pid_t>::max())) {
                        HALOFPX_CONTROLLER_ERROR();
                        break;
                    }
                    response_expected_child = static_cast<pid_t>(child_value);
                }
            } else if (event == PTRACE_EVENT_EXEC && pid == launcher) {
                launcher_exec = !launcher_exec && same_executable(pid, target_identity);
                if (!launcher_exec) { HALOFPX_CONTROLLER_ERROR(); break; }
            } else if (event == PTRACE_EVENT_EXEC) {
                if (live_child != -1 ||
                    (response_mode_requested && pid != response_expected_child) ||
                    !same_executable(pid, target_identity) ||
                    !exact_envelope_child_argv(pid) ||
                    (response_mode_requested && !exact_fd(
                        pid, STDOUT_FILENO, response_pipe_identity,
                        response_pipe_mount))) {
                    HALOFPX_CONTROLLER_ERROR();
                    break;
                }
                live_child = pid;

                child_exec = true;
                tracees[pid].live_child = true;
            }
            if (!resume_syscalls(pid)) HALOFPX_CONTROLLER_ERROR();
            continue;
        }
        if (signal == (SIGTRAP | 0x80)) {
            struct __ptrace_syscall_info info {};
            const long size = ::ptrace(PTRACE_GET_SYSCALL_INFO, pid, sizeof(info), &info);
            if (size < 0 || info.arch != AUDIT_ARCH_X86_64) {
                HALOFPX_CONTROLLER_ERROR();
                break;
            }
            auto & state = tracees[pid];
            const bool retryable_eintr = returned_eintr_must_retry(input);
            const bool response_mode = response_mode_requested;
            const bool fault_expected_to_fail =
                !response_mode && !retryable_eintr &&
                input.injection != mode::short_late;
            if (info.op == PTRACE_SYSCALL_INFO_ENTRY) {
                state.have_entry = true;
                state.nr = info.entry.nr;
                std::memcpy(state.args, info.entry.args, sizeof(state.args));
                bool response_entry = false;
                if (response_mode &&
                    (state.nr == SYS_write || state.nr == SYS_writev) &&
                    state.args[0] == STDOUT_FILENO) {
                    const bool exact_pipe = exact_fd(
                        pid, state.args[0], response_pipe_identity,
                        response_pipe_mount);
                    if (!response_selector(pid, live_child, launcher,
                                           state.args[0], exact_pipe,
                                           response.fragments)) {
                        HALOFPX_CONTROLLER_ERROR();
                        break;
                    }
                    std::string fragment;
                    if (!decode_response_transcript(pid, state, fragment) ||
                        state.nr != SYS_write || fragment.empty() ||
                        response.pending_replacement || response.suppressed) {
                        std::fill(fragment.begin(), fragment.end(), '\0');
                        HALOFPX_CONTROLLER_ERROR();
                        break;
                    }
                    response.transcript.swap(fragment);
                    std::fill(fragment.begin(), fragment.end(), '\0');
                    response.requested = response.transcript.size();
                    response.syscall_nr = state.nr;
                    response.pending_pid = pid;
                    ++response.fragments;
                    if (!replace_entry_with_enosys(pid)) {
                        HALOFPX_CONTROLLER_ERROR();
                        break;
                    }
                    response.pending_replacement = true;
                    response_entry = true;
                }
                if (state.live_child && fault.first_replaced &&
                    fault_expected_to_fail &&
                    exact_signature_retry_guard(fault, state)) {
                    HALOFPX_CONTROLLER_ERROR();
                    break;
                }
                if (state.live_child && writer_pinned) {
                        const bool forbidden_cleanup = state.nr == SYS_unlinkat ||
                            state.nr == SYS_unlink || state.nr == SYS_rmdir ||
                            state.nr == SYS_truncate || state.nr == SYS_ftruncate ||
                            state.nr == SYS_rename ||
                            state.nr == SYS_renameat || state.nr == SYS_link ||
                            state.nr == SYS_linkat || state.nr == SYS_symlink ||
                            state.nr == SYS_symlinkat || state.nr == SYS_mount ||
                            state.nr == SYS_umount2;
                    if (forbidden_cleanup) {
                        cleanup_syscall_seen = true;
                        HALOFPX_CONTROLLER_ERROR();

                        break;
                    }
#if defined(SYS_move_mount)
                    if (state.nr == SYS_move_mount) {
                        cleanup_syscall_seen = true;
                        HALOFPX_CONTROLLER_ERROR();
                        break;
                    }
#endif
                    bool truncating_open = false;
                    if (state.nr == SYS_open) {
                        truncating_open = (state.args[1] & O_TRUNC) != 0;
                    } else if (state.nr == SYS_openat) {
                        truncating_open = (state.args[2] & O_TRUNC) != 0;
                    } else if (state.nr == SYS_openat2) {
                        struct open_how observed_how {};
                        truncating_open = state.args[3] == sizeof(observed_how) &&

                            read_tracee(pid, state.args[2], &observed_how,
                                        sizeof(observed_how)) &&
                            (observed_how.flags & O_TRUNC) != 0;
                    }
                    if (truncating_open) {
                        cleanup_syscall_seen = true;
                        HALOFPX_CONTROLLER_ERROR();
                        break;
                    }
                    if (fault.first_replaced && fault_expected_to_fail) {
                        const auto authority_raw_fd = [&](std::uint64_t raw_fd) {
                            return exact_fd(pid, raw_fd, root_identity, root_mount) ||
                            (staging.pinned && exact_directory_fd(
                                pid, static_cast<int>(raw_fd), staging)) ||
                            (envelopes.pinned && exact_directory_fd(
                                pid, static_cast<int>(raw_fd), envelopes)) ||
                            (marker.marker_pinned && exact_fd(
                                pid, raw_fd, marker.identity, marker.mount_id)) ||
                            (root_marker.marker_pinned && exact_fd(
                                pid, raw_fd, root_marker.identity,
                                root_marker.mount_id)) ||
                            (writer_pinned && exact_fd(
                                pid, raw_fd, writer_identity, root_mount)) ||
                            exact_fd(pid, raw_fd, fixture_lock_identity,
                                     fixture_mount);
                        };
                        const bool authority_fd = authority_raw_fd(state.args[0]);
                        const bool copy_destination_authority =
                            state.nr == SYS_copy_file_range &&
                            authority_raw_fd(state.args[2]);
                        const bool expected_pwrite_completion =
                            state.nr == SYS_pwrite64 &&
                            input.point == boundary::envelope_pwrite &&
                            (returned_eintr_must_retry(input) ||
                             input.injection == mode::short_late ||
                             input.injection == mode::short_pre) &&
                            marker.marker_pinned && exact_fd(
                                pid, state.args[0], marker.identity,
                                marker.mount_id);
                        bool writable_open = false;
                        if (state.nr == SYS_open) {
                            writable_open = (state.args[1] &
                                (O_CREAT | O_WRONLY | O_RDWR | O_TRUNC)) != 0;
                        } else if (state.nr == SYS_openat) {
                            writable_open = (state.args[2] &
                                (O_CREAT | O_WRONLY | O_RDWR | O_TRUNC)) != 0;
                        } else if (state.nr == SYS_openat2) {
                            struct open_how observed_how {};
                            writable_open = state.args[3] == sizeof(observed_how) &&
                                read_tracee(pid, state.args[2], &observed_how,
                                            sizeof(observed_how)) &&
                                (observed_how.flags &
                                 (O_CREAT | O_WRONLY | O_RDWR | O_TRUNC)) != 0;
                        }
                        const bool forbidden_post_fault_mutation =
                            (authority_fd &&
                             (state.nr == SYS_write || state.nr == SYS_pwrite64 ||
                              state.nr == SYS_writev || state.nr == SYS_pwritev ||
                              state.nr == SYS_pwritev2 || state.nr == SYS_fchmod ||
                              state.nr == SYS_fallocate)) ||
                            copy_destination_authority ||
                            state.nr == SYS_chmod || state.nr == SYS_fchmodat ||
                            state.nr == SYS_fchmodat2 || state.nr == SYS_creat ||
                            state.nr == SYS_link || state.nr == SYS_linkat ||
                            state.nr == SYS_symlink || state.nr == SYS_symlinkat ||
                            state.nr == SYS_renameat2 ||
                            writable_open;
                        if (forbidden_post_fault_mutation &&
                            !expected_pwrite_completion) {
                            cleanup_syscall_seen = true;
                            (void) emit(std::string("\"event\":\"post-fault-mutation\",\"nr\":") +
                                std::to_string(state.nr) +
                                ",\"authority_fd\":" +
                                (authority_fd ? "true" : "false") +
                                ",\"writable_open\":" +
                                (writable_open ? "true" : "false"));
                            HALOFPX_CONTROLLER_ERROR();
                            break;
                        }
                    }
                    if (state.nr == SYS_renameat2) {
                        std::string old_name, new_name;

                        const bool names_read =
                            read_tracee_name(pid, state.args[1], old_name) &&
                            read_tracee_name(pid, state.args[3], new_name);
                        const bool inherited_publication = names_read &&
                            state.args[4] == RENAME_NOREPLACE &&
                            staging.pinned &&
                            exact_directory_fd(pid, static_cast<int>(state.args[0]),
                                               staging) &&
                            exact_fd(pid, state.args[2], root_identity, root_mount) &&
                            old_name == "initialize-root.tmp" &&
                            new_name == "root.marker";
                        const bool publication = names_read &&
                            state.args[4] == RENAME_NOREPLACE &&
                            staging.pinned &&
                            exact_directory_fd(pid, static_cast<int>(state.args[0]),
                                               staging) &&
                            envelopes.pinned &&
                            exact_directory_fd(pid, static_cast<int>(state.args[2]),
                                               envelopes) &&
                            old_name == "initialize-envelope.tmp" &&
                            remember_envelope_basename(marker, new_name);
                        if (inherited_publication) {
                            ++inherited_marker_publication_attempts;
                            if (inherited_marker_publication_attempts != 1) {
                                cleanup_syscall_seen = true;
                                HALOFPX_CONTROLLER_ERROR();
                                break;
                            }
                        } else if (!publication) {
                            cleanup_syscall_seen = true;
                            (void) emit(std::string("\"event\":\"unexpected-rename\",\"old\":\"") +
                                old_name + "\",\"new\":\"" + new_name +
                                "\",\"flags\":" + std::to_string(state.args[4]) +
                                ",\"staging_pinned\":" + (staging.pinned ? "true" : "false") +
                                ",\"envelopes_pinned\":" + (envelopes.pinned ? "true" : "false"));

                            HALOFPX_CONTROLLER_ERROR();
                            break;
                        } else {
                            ++publication_attempts;
                            if (publication_attempts != 1) {
                                cleanup_syscall_seen = true;
                                HALOFPX_CONTROLLER_ERROR();
                                break;
                            }
                        }
                    }
                }
                if (state.live_child && !capture_pwrite_intent(pid, state, marker)) {
                    HALOFPX_CONTROLLER_ERROR();
                    break;
                }
                bool match = false;
                if (!response_entry && state.live_child &&
                    !fault.pending_replacement) {
                    const bool exact_boundary = matches_boundary(
                        pid, state, input, root_identity, root_mount,
                        parent_identity, parent_mount, fixture_identity, fixture_mount,
                        envelopes, directories[1], staging,
                        marker, root_marker, step4_gate, l05z_core_gate,
                        writer_pinned, writer_identity,
                        fixture_lock_identity);
                    if (!fault.first_replaced) {
                        if (exact_boundary) {
                            ++fault.matches;
                            match = fault.matches == input.occurrence;
                            if (match) {
                                fault.selected_signature_valid = true;
                                fault.selected_nr = state.nr;
                                std::copy(std::begin(state.args), std::end(state.args),
                                          fault.selected_args.begin());
                            }
                        }
                    } else if (retryable_eintr) {
                        const retry_window_result observed = observe_retry_window(
                            fault, exact_boundary, input.occurrence);
                        if (observed == retry_window_result::excessive) {
                            HALOFPX_CONTROLLER_ERROR();
                            break;
                        }
                        match = observed == retry_window_result::selected;
                    }
                }
                selected[pid] = match;
                if (!response_entry && match &&
                    (input.injection == mode::short_pre ||
                     input.injection == mode::short_late) &&
                    input.short_count >= state.args[2]) {
                    HALOFPX_CONTROLLER_ERROR();
                    break;
                }
                const bool skip = !response_entry && match &&
                    fault.matches == input.occurrence &&
                    selected_entry_is_suppressed(input);
                if (skip) {
                    if (!replace_entry_with_enosys(pid)) { HALOFPX_CONTROLLER_ERROR(); break; }
                    fault.pending_replacement = true;
                } else if (match && retryable_eintr && fault.first_replaced) {
                    fault.retry_seen = true;
                }
                if (!response_entry && match &&
                    input.point == boundary::final_validation &&
                    !fault.mutation_applied) {
                    if (input.injection == mode::reserve_loss) {
                        HALOFPX_CONTROLLER_ERROR();
                        break;
                    }
                    if (!apply_retained_mutation(input, envelopes_fd, staging_fd, marker)) {
                        HALOFPX_CONTROLLER_ERROR();
                        break;
                    }
                    fault.mutation_applied = true;
                }
                if (!response_entry && match && input.injection == mode::collision &&
                    !fault.mutation_applied) {

                    if (!apply_collision(input, envelopes_fd, staging_fd, marker)) {
                        HALOFPX_CONTROLLER_ERROR();
                        break;
                    }
                    if (input.point == boundary::transient_open) {
                        marker.transient_seen = true;
                    }
                    fault.mutation_applied = true;
                }
                if (!response_entry && match &&
                    input.injection == mode::temp_inode_substitute &&


                    !fault.mutation_applied) {
                    if (!apply_temp_inode_substitution(staging_fd)) {
                        HALOFPX_CONTROLLER_ERROR();
                        break;
                    }
                    fault.mutation_applied = true;
                }
            } else if (info.op == PTRACE_SYSCALL_INFO_EXIT) {
                if (state.live_child && state.have_entry) {
                    if (response_mode && response.pending_replacement) {
                        if (!response_exit_shape(
                                response, pid, state.nr, info.exit.is_error,
                                info.exit.rval) ||
                            !replace_return(pid, static_cast<long long>(
                                response.requested))) {
                            HALOFPX_CONTROLLER_ERROR();
                            break;
                        }
                        response.pending_replacement = false;
                        response.suppressed = true;
                    }
                    if (state.nr == SYS_renameat2 && !info.exit.is_error &&
                        info.exit.rval == 0) {
                        std::string inherited_old, inherited_new;
                        if (read_tracee_name(pid, state.args[1], inherited_old) &&
                            read_tracee_name(pid, state.args[3], inherited_new) &&
                            inherited_old == "initialize-root.tmp" &&
                            inherited_new == "root.marker") {
                            inherited_marker_published = true;
                        }
                    }
                    if (inherited_marker_published && state.nr == SYS_fsync &&
                        !info.exit.is_error && info.exit.rval == 0) {
                        if (exact_fd(pid, state.args[0], root_identity, root_mount))
                            inherited_root_synced = true;

                        if (staging.pinned && exact_directory_fd(
                                pid, static_cast<int>(state.args[0]), staging))
                            inherited_staging_synced = true;
                    }
                    if (inherited_root_synced && inherited_staging_synced &&
                        root_marker.marker_pinned && state.nr == SYS_pread64 &&
                        exact_fd(pid, state.args[0], root_marker.identity,
                                 root_marker.mount_id) && !info.exit.is_error) {
                        if (state.args[3] == 0 && state.args[2] == root_marker.encoded_size &&
                            info.exit.rval == static_cast<long long>(root_marker.encoded_size))
                            inherited_final_marker_data = true;
                        if (inherited_final_marker_data && state.args[2] == 1 &&

                            state.args[3] == root_marker.encoded_size &&
                            info.exit.rval == 0) inherited_final_marker_eof = true;
                    }
                    if (inherited_final_marker_eof && staging.pinned &&
                        state.nr == SYS_getdents64 && !info.exit.is_error &&
                        info.exit.rval == 0 && exact_directory_fd(
                            pid, static_cast<int>(state.args[0]), staging)) {
                        ++staging_eof_after_inherited_marker;
                        if (staging_eof_after_inherited_marker == 1) {
                            if (!snapshot_root_marker(root_fd, root_marker)) {
                                HALOFPX_CONTROLLER_ERROR();
                                break;
                            }
                            step4_gate = true;
                        } else if (staging_eof_after_inherited_marker == 2) {
                            l05z_core_gate = true;
                        } else if (!l05z_core_gate) {
                            HALOFPX_CONTROLLER_ERROR();
                            break;
                        }
                    }
                    if (l05z_core_gate && state.nr == SYS_fcntl &&
                        state.args[1] == F_OFD_SETLK &&
                        exact_fd(pid, state.args[0], fixture_lock_identity,
                                 fixture_mount)) {
                        struct flock lock {};
                        if (!read_tracee(pid, state.args[2], &lock, sizeof(lock)) ||
                            lock.l_type != F_UNLCK || lock.l_whence != SEEK_SET ||
                            lock.l_start != 0 || lock.l_len != 0) {
                            HALOFPX_CONTROLLER_ERROR();
                            break;
                        }
                        l05z_core_gate = false;
                    }
                    if (state.nr == SYS_openat2 &&
                        writer_openat2_entry(pid, state, root_identity, root_mount) &&
                        !info.exit.is_error && info.exit.rval >= 0) {
                        std::uint64_t writer_mount = 0;
                        if (!tracee_fd_identity(pid, static_cast<int>(info.exit.rval),
                                               writer_identity, writer_mount) ||
                            writer_mount != root_mount) { HALOFPX_CONTROLLER_ERROR(); break; }
                        writer_pinned = true;
                    }
                    if (state.nr == SYS_mkdirat && !info.exit.is_error &&
                        info.exit.rval == 0) {
                        std::string created_name;
                        if (!read_tracee_name(pid, state.args[1], created_name)) {
                            HALOFPX_CONTROLLER_ERROR();
                            break;
                        }
                        for (int directory = 0; directory < 3; ++directory) {
                            if (created_name == directory_names[
                                    static_cast<std::size_t>(directory)]) {
                                pinned_directory observed {};
                                if (!pin_created_directory(root_fd, directory, 0000,
                                                           observed)) {

                                    HALOFPX_CONTROLLER_ERROR();
                                    break;
                                }
                                directories[static_cast<std::size_t>(directory)] = observed;
                                if (directory == 0) {
                                    envelopes = observed;
                                }
                                if (directory == 2) {
                                    staging = observed;
                                }
                            }
                        }
                        if (controller_error) break;
                    }
                    if (state.nr == SYS_fchmodat2 && !info.exit.is_error &&
                        info.exit.rval == 0) {
                        std::string repaired_name;
                        if (!read_tracee_name(pid, state.args[1], repaired_name)) {
                            HALOFPX_CONTROLLER_ERROR();
                            break;
                        }
                        const bool repaired_envelopes = repaired_name == "envelopes" ||
                            (envelopes.pinned && exact_directory_fd(
                                pid, static_cast<int>(state.args[0]), envelopes));
                        const bool repaired_staging = repaired_name == "staging" ||
                            (staging.pinned && exact_directory_fd(
                                pid, static_cast<int>(state.args[0]), staging));
                        if (repaired_envelopes && envelopes_fd < 0 &&
                            !open_named(root_fd, "envelopes", O_RDONLY | O_DIRECTORY,
                                        envelopes_fd)) {
                            HALOFPX_CONTROLLER_ERROR();
                            break;
                        }
                        if (repaired_staging && staging_fd < 0 &&
                            !open_named(root_fd, "staging", O_RDONLY | O_DIRECTORY,
                                        staging_fd)) {
                            HALOFPX_CONTROLLER_ERROR();
                            break;
                        }
                    }
                    struct open_how how {};
                    std::string name;
                    if (read_how(pid, state, how, name) && name == "root.marker" &&
                        !info.exit.is_error && info.exit.rval >= 0 &&
                        !pin_marker_fd(pid, static_cast<int>(info.exit.rval),
                                       root_marker)) {
                        HALOFPX_CONTROLLER_ERROR();
                        break;
                    }
                    if (read_how(pid, state, how, name) &&
                        (name == "initialize-envelope.tmp" ||
                         (marker.basename_known && name == marker.basename.data())) &&
                        !info.exit.is_error && info.exit.rval >= 0) {
                        const bool substituted_temp =
                            input.injection == mode::temp_inode_substitute &&
                            input.point == boundary::readonly_temp_open &&
                            fault.mutation_applied && selected[pid] &&
                            name == "initialize-envelope.tmp";
                        if (substituted_temp) {
                            struct stat replacement {};
                            std::uint64_t replacement_mount = 0;
                            if (!tracee_fd_identity(
                                    pid, static_cast<int>(info.exit.rval),
                                    replacement, replacement_mount) ||
                                !S_ISREG(replacement.st_mode) ||
                                replacement.st_nlink != 1 || replacement.st_size != 0 ||
                                replacement.st_uid != ::geteuid() ||
                                (replacement.st_mode & 07777) != 0600 ||
                                replacement_mount != marker.mount_id ||
                                same_object(marker.identity, replacement)) {
                                HALOFPX_CONTROLLER_ERROR();
                                break;
                            }
                            marker.temp_substitute_open_seen = true;
                        } else {
                            if (!pin_marker_fd(
                                    pid, static_cast<int>(info.exit.rval), marker)) {
                                HALOFPX_CONTROLLER_ERROR();
                                break;
                            }
                            if (name == "initialize-envelope.tmp") marker.transient_seen = true;
                            else marker.final_open_seen = true;
                        }
                    }
                    if (state.nr == SYS_pwrite64 && marker.marker_pinned &&
                        exact_fd(pid, state.args[0], marker.identity, marker.mount_id) &&
                        !info.exit.is_error &&

                        !capture_completed_pwrite(state, info.exit.rval, marker)) {
                        HALOFPX_CONTROLLER_ERROR();
                        break;
                    }
                    if (state.nr == SYS_renameat2 && !info.exit.is_error && info.exit.rval == 0) {
                        std::string old_name, new_name;
                        if (read_tracee_name(pid, state.args[1], old_name) &&
                            read_tracee_name(pid, state.args[3], new_name) &&
                            old_name == "initialize-envelope.tmp" &&
                            remember_envelope_basename(marker, new_name)) {
                            marker.published_seen = true;
                        }
                    }
                    if (fault.pending_replacement) {
                        long long replacement = 0;
                        if (synthetic_exit_returns_errno(input))
                            replacement = -static_cast<long long>(input.returned_errno);
                        else if (input.injection == mode::short_pre)
                            replacement = input.short_count;
                        if (!info.exit.is_error || info.exit.rval != -ENOSYS ||
                            !replace_return(pid, replacement)) { HALOFPX_CONTROLLER_ERROR(); break; }
                        fault.pending_replacement = false;
                        fault.first_replaced = true;
                        if (retryable_eintr) fault.retry_window_open = true;
                    } else if (selected[pid] &&
                               (completed_exit_returns_errno(input) ||
                                input.injection == mode::short_late ||
                                input.injection == mode::zero_late) &&
                               fault.matches == input.occurrence) {
                        if (info.exit.is_error) { HALOFPX_CONTROLLER_ERROR(); break; }
                        long long replacement = 0;
                        if (completed_exit_returns_errno(input))
                            replacement = -static_cast<long long>(input.returned_errno);

                        else if (input.injection == mode::short_late)
                            replacement = input.short_count;
                        if (!replace_return(pid, replacement)) { HALOFPX_CONTROLLER_ERROR(); break; }
                        fault.first_replaced = true;
                        if (retryable_eintr) fault.retry_window_open = true;
                    } else if (selected[pid] && retryable_eintr && fault.retry_seen) {
                        fault.retry_succeeded = !info.exit.is_error;
                    } else if (selected[pid] && input.point == boundary::reserve_revalidation &&
                               input.injection == mode::reserve_loss && !info.exit.is_error) {
                        struct statfs value {};
                        if (!read_tracee(pid, state.args[1], &value, sizeof(value))) {
                            HALOFPX_CONTROLLER_ERROR();
                            break;

                        }
                        value.f_bavail = 0;
                        if (!write_tracee(pid, state.args[1], &value, sizeof(value))) {
                            HALOFPX_CONTROLLER_ERROR();
                            break;
                        }
                        fault.first_replaced = true;
                        fault.mutation_applied = true;

                    }
                }
                selected[pid] = false;
                state.have_entry = false;
            } else {
                HALOFPX_CONTROLLER_ERROR();
                break;
            }
            if (!resume_syscalls(pid)) HALOFPX_CONTROLLER_ERROR();
            continue;
        }

        auto item = tracees.find(pid);
        if (signal == SIGSTOP && item != tracees.end() &&
            item->second.initial_sigstop_expected) {
            item->second.initial_sigstop_expected = false;
            if (!resume_syscalls(pid)) HALOFPX_CONTROLLER_ERROR();
            continue;
        }
        if (signal == SIGSTOP || signal == SIGTRAP) {
            HALOFPX_CONTROLLER_ERROR();
            break;
        }

        if (!resume_syscalls(pid, signal)) HALOFPX_CONTROLLER_ERROR();
    }

    if (!tracees.empty()) {
        bounded_cleanup = true;
        for (const auto & item : tracees) (void) ::kill(item.first, SIGKILL);
        while (::waitpid(-1, &status, __WALL) > 0 || errno == EINTR) {}
    }
    std::string audit;
    std::array<char, 4096> audit_bytes {};
    bool audit_read = true;
    bool audit_eof = false;
    for (;;) {
        ssize_t audit_count;
        do { audit_count = ::read(audit_pipe[0], audit_bytes.data(), audit_bytes.size()); }
        while (audit_count < 0 && errno == EINTR);
        if (audit_count == 0) {
            audit_eof = true;
            break;
        }
        if (audit_count < 0 || audit.size() >
                65536U - static_cast<std::size_t>(audit_count)) {
            audit_read = false;
            break;
        }
        audit.append(audit_bytes.data(), static_cast<std::size_t>(audit_count));
    }
    const bool audit_closed = ::close(audit_pipe[0]) == 0;

    bool whole_root_discard = false;
    struct stat parent_now {}, root_now {}, fixture_now {};
    std::uint64_t parent_mount_now = 0, root_mount_now = 0, fixture_mount_now = 0;
    const auto exact_directory_authority = [](const struct stat & value) {
        return S_ISDIR(value.st_mode) && value.st_uid == ::geteuid() &&
               (value.st_mode & 07777) == 0700;
    };
    const auto reopened_matches = [](const char * path, const struct stat & expected,
                                     std::uint64_t expected_mount) {
        const int fd = open_absolute_directory_no_symlink(path);
        struct stat observed {};
        std::uint64_t mount = 0;
        const bool matched = fd >= 0 && ::fstat(fd, &observed) == 0 &&
            fd_mount_id(fd, mount) && same_object(expected, observed) &&
            mount == expected_mount && S_ISDIR(observed.st_mode) &&
            observed.st_uid == ::geteuid() && (observed.st_mode & 07777) == 0700;
        const bool closed = fd >= 0 && ::close(fd) == 0;
        return matched && closed;
    };
    const bool authorities_pinned =
        ::fstat(parent_fd, &parent_now) == 0 && exact_directory_authority(parent_now) &&
        same_object(parent_identity, parent_now) &&
        fd_mount_id(parent_fd, parent_mount_now) && parent_mount_now == parent_mount &&
        ::fstat(root_fd, &root_now) == 0 && exact_directory_authority(root_now) &&
        same_object(root_identity, root_now) &&
        fd_mount_id(root_fd, root_mount_now) && root_mount_now == root_mount &&
        ::fstat(fixture_fd, &fixture_now) == 0 &&
        exact_directory_authority(fixture_now) && same_object(fixture_identity, fixture_now) &&
        fd_mount_id(fixture_fd, fixture_mount_now) && fixture_mount_now == fixture_mount &&
        reopened_matches(input.parent, parent_identity, parent_mount) &&
        reopened_matches(input.root, root_identity, root_mount) &&
        reopened_matches(input.fixture, fixture_identity, fixture_mount);
    std::vector<std::string> parent_names, fixture_names;
    const bool authority_inventory = list_names(parent_fd, parent_names) &&
        list_names(fixture_fd, fixture_names) &&
        parent_names == std::vector<std::string>({ "fixture", "root" }) &&
        fixture_names == std::vector<std::string>({ "primitive.lock" });
    const bool inventory = exact_inventory(
        root_fd, marker, root_marker, input, writer_identity, writer_pinned,
        directories, whole_root_discard);
    const bool fixture_released = prove_ofd_released(
        fixture_fd, "primitive.lock", fixture_lock_identity);
    const bool writer_released = writer_pinned && prove_ofd_released(

        root_fd, "writer.lock", writer_identity);
    const bool mutation_mode = input.injection >= mode::corrupt;
    const bool response_mode = input.injection == mode::response_loss_full;
    const bool eintr_success = returned_eintr_must_retry(input);
    const bool short_late_success = input.injection == mode::short_late;
    const bool late_eintr_success = input.injection == mode::late_error &&
        input.returned_errno == EINTR && eintr_is_retryable(input.point);
    const bool injection = response_mode
        ? response.suppressed && !response.pending_replacement &&
            response.fragments == 1 && response.syscall_nr == SYS_write &&
            response.requested == response.transcript.size() &&
            response.requested != 0
        : mutation_mode
        ? fault.mutation_applied &&
            (input.injection != mode::temp_inode_substitute ||
             marker.temp_substitute_open_seen)
        : eintr_success
            ? fault.first_replaced && fault.retry_seen && fault.retry_succeeded &&
                fault.matches == input.occurrence + 1 &&
                !fault.retry_window_open
            : fault.first_replaced;
    const bool qualified = response_mode || eintr_success || short_late_success ||
        late_eintr_success;
    const bool byte_coverage = exact_byte_coverage(input, marker, qualified);
    const unsigned expected_publication_attempts =
        input.point == boundary::envelope_rename || marker.published_seen ? 1U : 0U;
    const bool audit_exact = exact_audit(
        response_mode ? response.transcript : audit, qualified, input, marker);
    const bool response_delivery_exact = !response_mode ||
        (audit_read && response_consumer_shape(audit.size(), audit_eof));
    const std::size_t response_transcript_size = response.transcript.size();
    const std::string response_transcript_sha256 = response_mode
        ? halofpx_l05z_return_hostile_manifest_v1::detail::hex(
              halofpx_l05z_return_hostile_manifest_v1::detail::sha256(
                  response.transcript))
        : std::string();
    const bool post_child_final_validation =
        input.point == boundary::final_validation && input.occurrence == 2 &&
        (input.injection == mode::unexpected_name ||
         input.injection == mode::hardlink || input.injection == mode::symlink ||
         input.injection == mode::inode_substitute);
    const bool process = audit_exact && (qualified
        ? child_exit == 0 && launcher_exit == 0
        : post_child_final_validation
            ? child_exit == 0 && launcher_exit == 2
            : child_exit != 0 && launcher_exit != 0);
    const bool pass = !controller_error && !bounded_cleanup && launcher_exec &&
        child_exec && audit_read && audit_closed && audit_eof &&
        response_delivery_exact && !cleanup_syscall_seen &&
        inherited_marker_publication_attempts == 1 &&
        publication_attempts == expected_publication_attempts &&
        injection && byte_coverage && process &&
        authorities_pinned && authority_inventory && inventory &&
        whole_root_discard && fixture_released && writer_released;
    emit(std::string("\"event\":\"post-fault\",\"pass\":") +
         (pass ? "true" : "false") + ",\"controller_error_line\":" +
         std::to_string(controller_error_line) + ",\"matches\":" +
         std::to_string(fault.matches) + ",\"encoded_size\":" +
         std::to_string(marker.encoded_size) + ",\"transient_retained\":" +
         (marker.transient_seen && !marker.published_seen ? "true" : "false") +

         ",\"published_retained\":" + (marker.published_seen ? "true" : "false") +
         ",\"inventory_exact\":" + (inventory ? "true" : "false") +
         ",\"authorities_pinned\":" + (authorities_pinned ? "true" : "false") +
         ",\"authority_inventory_exact\":" +
         (authority_inventory ? "true" : "false") +
         ",\"injection_exact\":" + (injection ? "true" : "false") +
         ",\"audit_exact\":" + (audit_exact ? "true" : "false") +
         ",\"child_status\":" + std::to_string(child_exit) +
         ",\"launcher_status\":" + std::to_string(launcher_exit) +
         ",\"publication_attempts\":" +
         std::to_string(publication_attempts) +
         ",\"inherited_marker_publication_attempts\":" +
         std::to_string(inherited_marker_publication_attempts) +
         ",\"byte_coverage_exact\":" + (byte_coverage ? "true" : "false") +

         ",\"fixture_lock_released\":" + (fixture_released ? "true" : "false") +
         ",\"writer_lock_released\":" + (writer_released ? "true" : "false"));
    if (response_mode) emit(std::string(
         "\"event\":\"response-loss\",\"selected\":true") +
         ",\"case_id\":\"" +
         halofpx_l05z_return_response_manifest_v1::case_id +
         "\",\"syscall_profile\":\"" +
         "SYS_write" +
         "\",\"fragments\":" + std::to_string(response.fragments) +
         ",\"attempted_bytes\":" +
         std::to_string(response_transcript_size) +
         ",\"delivered_bytes\":" + std::to_string(audit.size()) +
         ",\"consumer_eof\":" + (audit_eof ? "true" : "false") +
         ",\"transcript_sha256\":\"" + response_transcript_sha256 +
         "\",\"suppressed_before_kernel\":" +
         (response.suppressed ? "true" : "false") +
         ",\"fake_full_success\":" +
         (response.suppressed && response.requested == response_transcript_size
              ? "true" : "false"));
    emit(std::string("\"event\":\"summary\",\"pass\":") +
         (pass ? "true" : "false") +
         ",\"initialization_discard_required\":true" +
         ",\"whole_root_discard_required\":true" +
         ",\"individual_cleanup_performed\":" +
         (cleanup_syscall_seen ? "true" : "false") +
         ",\"marker_qualified\":" + (qualified ? "true" : "false"));

    std::fill(response.transcript.begin(), response.transcript.end(), '\0');
    response.transcript.clear();

    const bool receipt_synced = synchronize_receipt(
        receipt_parent_fd, receipt_parent_identity, receipt_parent_mount, receipt_name);
    const bool receipt_closed = ::close(receipt_fd) == 0;

    receipt_fd = -1;
    const bool receipt_parent_closed = ::close(receipt_parent_fd) == 0;
    const bool staging_closed = staging_fd >= 0 && ::close(staging_fd) == 0;
    const bool fixture_closed = ::close(fixture_fd) == 0;
    const bool root_closed = ::close(root_fd) == 0;
    const bool parent_closed = ::close(parent_fd) == 0;
    return pass && receipt_ok && receipt_synced && receipt_closed &&
        receipt_parent_closed && staging_closed && fixture_closed &&
        root_closed && parent_closed ? 0 : 1;
#undef HALOFPX_CONTROLLER_ERROR
}

} // namespace envelope_fault

} // namespace

int main(int argc, char ** argv) {
    if (argc == 2 &&
        std::strcmp(argv[1], "--mapped-role-authority-self-test") == 0) {
        const bool ok = halofpx_l05z_return_role_map_v1::self_check() &&
            envelope_fault::mapped_role_boundary_execution_closed_self_check();
        std::array<std::size_t, 3> states {};
        for (const auto & role : halofpx_l05z_return_role_map_v1::records()) {
            ++states[role.state == halofpx_l05z_return_role_map_v1::admission::pending ? 0 :
                role.state == halofpx_l05z_return_role_map_v1::admission::mapped_not_executable ? 1 : 2];
        }
        std::printf("version=v1 roles=%zu pending=%zu mapped_not_executable=%zu executable=%zu cli_closed=6 source_commit=%s source_tree=%s profile=%s manifest_hash=%s\n",
            halofpx_l05z_return_role_map_v1::records().size(), states[0],
            states[1], states[2],
            halofpx_l05z_return_role_map_v1::mapping_source_commit,
            halofpx_l05z_return_role_map_v1::mapping_source_tree,
            halofpx_l05z_return_role_map_v1::physical_profile,
            halofpx_l05z_return_role_map_v1::manifest_hash_hex().c_str());
        return ok ? 0 : 1;
    }
    if (argc == 2 &&
        std::strcmp(argv[1], "--response-decoder-self-test") == 0) {
        const bool ok = envelope_fault::response_decoder_self_check() &&
            halofpx_l05z_return_response_manifest_v1::self_check();
        std::printf("case_id=%s canonical_cases=%zu extended_total=%zu dedup_key=%s id_hash=%s manifest_hash=%s extended_id_hash=%s extended_manifest_hash=%s profile=%s\n",
            halofpx_l05z_return_response_manifest_v1::case_id,
            halofpx_l05z_return_response_manifest_v1::canonical_case_count,
            halofpx_l05z_return_response_manifest_v1::extended_semantic_total,
            halofpx_l05z_return_response_manifest_v1::dedup_key,
            halofpx_l05z_return_response_manifest_v1::id_set_hash_hex().c_str(),
            halofpx_l05z_return_response_manifest_v1::manifest_hash_hex().c_str(),
            halofpx_l05z_return_response_manifest_v1::extended_id_set_hash_hex().c_str(),
            halofpx_l05z_return_response_manifest_v1::extended_manifest_hash_hex().c_str(),
            halofpx_l05z_return_response_manifest_v1::admitted_physical_profile);
        return ok ? 0 : 1;
    }
    if (argc == 2 && std::strcmp(argv[1], "--role-authority-self-test") == 0) {
        const bool ok = envelope_fault::manifest_self_check();
        std::printf("roles=%zu canonical_cases=%zu physical_rows_frozen=%u id_hash=%s manifest_hash=%s case_id_hash=%s case_manifest_hash=%s",
            halofpx_l05z_return_role_authority_v1::roles().size(),
            halofpx_l05z_return_role_authority_v1::canonical_total_cases,
            halofpx_l05z_return_role_authority_v1::physical_execution_rows_frozen ? 1U : 0U,
            halofpx_l05z_return_role_authority_v1::sorted_role_id_hash_hex().c_str(),
            halofpx_l05z_return_role_authority_v1::manifest_hash_hex().c_str(),
            halofpx_l05z_return_role_authority_v1::canonical_case_id_hash_hex().c_str(),
            halofpx_l05z_return_role_authority_v1::canonical_case_manifest_hash_hex().c_str());
        for (std::uint32_t shard = 0; shard < 17; ++shard) {
            std::size_t count = 0;
            for (const auto & role : halofpx_l05z_return_role_authority_v1::roles()) {
                if (halofpx_l05z_return_role_authority_v1::shard_for(
                        role.role_id, 17) == shard) ++count;
            }
            std::printf(" shard%02u=%zu", shard, count);
        }
        for (std::uint32_t shard = 0; shard < 17; ++shard) {
            std::size_t count = 0;
            for (const auto & item :
                    halofpx_l05z_return_role_authority_v1::
                        compatibility_execution_case_roster()) {
                if (halofpx_l05z_return_role_authority_v1::case_shard_for(
                        item.case_id, 17) == shard) ++count;
            }
            std::printf(" case_shard%02u=%zu", shard, count);
        }
        std::printf("\n");
        return ok ? 0 : 1;
    }
    if (argc == 2 &&
        std::strcmp(argv[1], "--semantic-authority-self-test") == 0) {
        const bool ok = envelope_fault::manifest_self_check();
        std::printf("compatibility_execution_cases=%zu semantic_unique_cases=%zu duplicate_pairs=%zu semantic_id_hash=%s semantic_manifest_hash=%s duplicate_pair_hash=%s semantic_extended_total=%zu semantic_extended_id_hash=%s semantic_extended_manifest_hash=%s",
            halofpx_l05z_return_role_authority_v1::canonical_total_cases,
            halofpx_l05z_return_role_authority_v1::semantic_unique_cases().size(),
            halofpx_l05z_return_role_authority_v1::semantic_duplicate_pairs().size(),
            halofpx_l05z_return_role_authority_v1::semantic_unique_case_id_hash_hex().c_str(),
            halofpx_l05z_return_role_authority_v1::semantic_unique_manifest_hash_hex().c_str(),
            halofpx_l05z_return_role_authority_v1::semantic_duplicate_pair_hash_hex().c_str(),
            halofpx_l05z_return_response_manifest_v1::semantic_unique_extended_total,
            halofpx_l05z_return_response_manifest_v1::semantic_unique_extended_id_set_hash_hex().c_str(),
            halofpx_l05z_return_response_manifest_v1::semantic_unique_extended_manifest_hash_hex().c_str());
        for (std::uint32_t shard = 0; shard < 17; ++shard) {
            std::size_t count = 0;
            for (const auto & item :
                    halofpx_l05z_return_role_authority_v1::semantic_unique_cases()) {
                if (halofpx_l05z_return_role_authority_v1::case_shard_for(
                        item.case_id, 17) == shard) ++count;
            }
            std::printf(" semantic_shard%02u=%zu", shard, count);
        }
        std::printf("\n");
        return ok ? 0 : 1;
    }
    if (argc == 3 && std::strcmp(argv[1], "--case-id") == 0) {
        if (std::strcmp(argv[2],
                        halofpx_l05z_return_response_manifest_v1::case_id) == 0) {
            return halofpx_l05z_return_response_manifest_v1::self_check() &&
                envelope_fault::response_decoder_self_check() ? 0 : 1;
        }
        return envelope_fault::hostile_case_self_test(argv[2]);
    }
    envelope_fault::arguments input {};
    if (!envelope_fault::parse_arguments(argc, argv, input)) {
        envelope_fault::usage_marker(argv[0]);

    }
    return envelope_fault::run(input);
}
