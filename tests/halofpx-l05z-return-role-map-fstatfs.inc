// Exact L05z SYS_fstatfs role map at source commit
// eba2e36764e1624d6d20bb00657dd7f2fe64a1be.
//
// Status: mapped but not executable; execution admission remains closed.  The
// rows below are exact source-and-trace role identities only and must not be
// expanded into returned-fault execution cases until the admission blocker is
// resolved and the strengthened late-success oracle is implemented and
// independently reviewed.
//
// Late-success oracle for every row: the native syscall must return exactly
// zero; the complete struct statfs result must be validated against the pinned
// descriptor and filesystem expectations before the controller substitutes the
// requested negative errno.  The tracee must not retry (including EINTR), must
// perform no post-fault authority mutation or individual cleanup, and must
// terminate with nonzero child and launcher status while retaining the exact
// window-specific residual tree, releasing both locks, and requiring whole-root
// discard.
//
// Admission blocker: the current reserve-revalidation selector is only
// step4_gate + SYS_fstatfs + exact root fd.  It therefore overlaps all six root
// rows 002, 003, 010, 011, 018, and 019.  Its occurrence ceiling of three makes
// 002, 003, and 010 executable, although the actual fstatvfs-reserve roles are
// 003, 011, and 019.  Do not admit this map until reserve selection is bound to
// those three exact roles (or an equivalently exact fail-closed phase state).

#ifndef HALOFPX_L05Z_ROLE_MAP_ROW
#error "Define HALOFPX_L05Z_ROLE_MAP_ROW before including this role map"
#endif

HALOFPX_L05Z_ROLE_MAP_ROW(
    "L05Z-FSTATFS-001", "SYS_fstatfs-inspect-directory", "parent",
    "premutation-reopened-parent", "W03",
    "initialize_once:2604>revalidate_accumulated_prefix:1365>revalidate_parent:1296>inspect_directory:286",
    1, "fixed-statfs-result",
    "no-envelope-temp;no-published-envelope;publication-attempts=0;whole-root-discard")
HALOFPX_L05Z_ROLE_MAP_ROW(
    "L05Z-FSTATFS-002", "SYS_fstatfs-inspect-directory", "candidate-root",
    "premutation-reopened-root", "W03",
    "initialize_once:2604>revalidate_accumulated_prefix:1368>reopen_directory:1319>inspect_directory:286",
    2, "fixed-statfs-result",
    "no-envelope-temp;no-published-envelope;publication-attempts=0;whole-root-discard")
HALOFPX_L05Z_ROLE_MAP_ROW(
    "L05Z-FSTATFS-003", "SYS_fstatfs-fstatvfs-reserve", "candidate-root",
    "premutation-reopened-root", "W03",
    "initialize_once:2604>revalidate_accumulated_prefix:1372>reserve_and_read_only:1117>glibc-fstatvfs",
    3, "fixed-statfs-result",
    "no-envelope-temp;no-published-envelope;publication-attempts=0;whole-root-discard")
HALOFPX_L05Z_ROLE_MAP_ROW(
    "L05Z-FSTATFS-004", "SYS_fstatfs-inspect-created-directory", "envelopes",
    "premutation-reopened-envelopes", "W03",
    "initialize_once:2604>revalidate_accumulated_prefix:1401>reopen_created_directory:1340>inspect_created_directory:601",
    4, "fixed-statfs-result",
    "no-envelope-temp;no-published-envelope;publication-attempts=0;whole-root-discard")
HALOFPX_L05Z_ROLE_MAP_ROW(
    "L05Z-FSTATFS-005", "SYS_fstatfs-inspect-created-directory", "attempts",
    "premutation-reopened-attempts", "W03",
    "initialize_once:2604>revalidate_accumulated_prefix:1407>reopen_created_directory:1340>inspect_created_directory:601",
    5, "fixed-statfs-result",
    "no-envelope-temp;no-published-envelope;publication-attempts=0;whole-root-discard")
HALOFPX_L05Z_ROLE_MAP_ROW(
    "L05Z-FSTATFS-006", "SYS_fstatfs-inspect-created-directory", "staging",
    "premutation-reopened-staging", "W03",
    "initialize_once:2604>revalidate_accumulated_prefix:1413>reopen_created_directory:1340>inspect_created_directory:601",
    6, "fixed-statfs-result",
    "no-envelope-temp;no-published-envelope;publication-attempts=0;whole-root-discard")
HALOFPX_L05Z_ROLE_MAP_ROW(
    "L05Z-FSTATFS-007", "SYS_fstatfs-inspect-directory", "fixture",
    "premutation-reopened-fixture", "W03",
    "initialize_once:2604>revalidate_accumulated_prefix:1424>reopen_directory:1319>inspect_directory:286",
    7, "fixed-statfs-result",
    "no-envelope-temp;no-published-envelope;publication-attempts=0;whole-root-discard")

HALOFPX_L05Z_ROLE_MAP_ROW(
    "L05Z-FSTATFS-008", "SYS_fstatfs-inspect-created-directory", "envelopes",
    "held-envelopes", "W04",
    "initialize_once:2689>inspect_created_directory:601",
    8, "fixed-statfs-result",
    "no-envelope-temp;no-published-envelope;publication-attempts=0;whole-root-discard")

HALOFPX_L05Z_ROLE_MAP_ROW(
    "L05Z-FSTATFS-009", "SYS_fstatfs-inspect-directory", "parent",
    "prepublication-reopened-parent", "W09A",
    "initialize_once:2778>revalidate_accumulated_prefix:1365>revalidate_parent:1296>inspect_directory:286",
    9, "fixed-statfs-result",
    "synchronized-envelope-temp-retained;no-published-envelope;publication-attempts=0;whole-root-discard")
HALOFPX_L05Z_ROLE_MAP_ROW(
    "L05Z-FSTATFS-010", "SYS_fstatfs-inspect-directory", "candidate-root",
    "prepublication-reopened-root", "W09A",
    "initialize_once:2778>revalidate_accumulated_prefix:1368>reopen_directory:1319>inspect_directory:286",
    10, "fixed-statfs-result",
    "synchronized-envelope-temp-retained;no-published-envelope;publication-attempts=0;whole-root-discard")
HALOFPX_L05Z_ROLE_MAP_ROW(
    "L05Z-FSTATFS-011", "SYS_fstatfs-fstatvfs-reserve", "candidate-root",
    "prepublication-reopened-root", "W09A",
    "initialize_once:2778>revalidate_accumulated_prefix:1372>reserve_and_read_only:1117>glibc-fstatvfs",
    11, "fixed-statfs-result",
    "synchronized-envelope-temp-retained;no-published-envelope;publication-attempts=0;whole-root-discard")
HALOFPX_L05Z_ROLE_MAP_ROW(
    "L05Z-FSTATFS-012", "SYS_fstatfs-inspect-created-directory", "envelopes",
    "prepublication-reopened-envelopes", "W09A",
    "initialize_once:2778>revalidate_accumulated_prefix:1401>reopen_created_directory:1340>inspect_created_directory:601",
    12, "fixed-statfs-result",
    "synchronized-envelope-temp-retained;no-published-envelope;publication-attempts=0;whole-root-discard")
HALOFPX_L05Z_ROLE_MAP_ROW(
    "L05Z-FSTATFS-013", "SYS_fstatfs-inspect-created-directory", "attempts",
    "prepublication-reopened-attempts", "W09A",
    "initialize_once:2778>revalidate_accumulated_prefix:1407>reopen_created_directory:1340>inspect_created_directory:601",
    13, "fixed-statfs-result",
    "synchronized-envelope-temp-retained;no-published-envelope;publication-attempts=0;whole-root-discard")
HALOFPX_L05Z_ROLE_MAP_ROW(
    "L05Z-FSTATFS-014", "SYS_fstatfs-inspect-created-directory", "staging",
    "prepublication-reopened-staging", "W09A",
    "initialize_once:2778>revalidate_accumulated_prefix:1413>reopen_created_directory:1340>inspect_created_directory:601",
    14, "fixed-statfs-result",
    "synchronized-envelope-temp-retained;no-published-envelope;publication-attempts=0;whole-root-discard")
HALOFPX_L05Z_ROLE_MAP_ROW(
    "L05Z-FSTATFS-015", "SYS_fstatfs-inspect-directory", "fixture",
    "prepublication-reopened-fixture", "W09A",
    "initialize_once:2778>revalidate_accumulated_prefix:1424>reopen_directory:1319>inspect_directory:286",
    15, "fixed-statfs-result",
    "synchronized-envelope-temp-retained;no-published-envelope;publication-attempts=0;whole-root-discard")
HALOFPX_L05Z_ROLE_MAP_ROW(
    "L05Z-FSTATFS-016", "SYS_fstatfs-inspect-created-directory", "staging",
    "envelope-staging-validation", "W09A",
    "initialize_once:2828>inspect_created_directory:601",
    16, "fixed-statfs-result",
    "synchronized-envelope-temp-retained;no-published-envelope;publication-attempts=0;whole-root-discard")

HALOFPX_L05Z_ROLE_MAP_ROW(
    "L05Z-FSTATFS-017", "SYS_fstatfs-inspect-directory", "parent",
    "final-reopened-parent", "W12C",
    "initialize_once:2959>revalidate_predecessor_final_layout:1472>revalidate_parent:1296>inspect_directory:286",
    17, "fixed-statfs-result",
    "published-envelope-retained;publication-attempts=1;whole-root-discard")
HALOFPX_L05Z_ROLE_MAP_ROW(
    "L05Z-FSTATFS-018", "SYS_fstatfs-inspect-directory", "candidate-root",
    "final-reopened-root", "W12C",
    "initialize_once:2959>revalidate_predecessor_final_layout:1475>reopen_directory:1319>inspect_directory:286",
    18, "fixed-statfs-result",
    "published-envelope-retained;publication-attempts=1;whole-root-discard")
HALOFPX_L05Z_ROLE_MAP_ROW(
    "L05Z-FSTATFS-019", "SYS_fstatfs-fstatvfs-reserve", "candidate-root",
    "final-reopened-root", "W12C",
    "initialize_once:2959>revalidate_predecessor_final_layout:1479>reserve_and_read_only:1117>glibc-fstatvfs",
    19, "fixed-statfs-result",
    "published-envelope-retained;publication-attempts=1;whole-root-discard")
HALOFPX_L05Z_ROLE_MAP_ROW(
    "L05Z-FSTATFS-020", "SYS_fstatfs-inspect-created-directory", "envelopes",
    "final-reopened-envelopes", "W12C",
    "initialize_once:2959>revalidate_predecessor_final_layout:1533>validate_child:1520>inspect_created_directory:601",
    20, "fixed-statfs-result",
    "published-envelope-retained;publication-attempts=1;whole-root-discard")
HALOFPX_L05Z_ROLE_MAP_ROW(
    "L05Z-FSTATFS-021", "SYS_fstatfs-inspect-created-directory", "attempts",
    "final-reopened-attempts", "W12C",
    "initialize_once:2959>revalidate_predecessor_final_layout:1534>validate_child:1520>inspect_created_directory:601",
    21, "fixed-statfs-result",
    "published-envelope-retained;publication-attempts=1;whole-root-discard")
HALOFPX_L05Z_ROLE_MAP_ROW(
    "L05Z-FSTATFS-022", "SYS_fstatfs-inspect-created-directory", "staging",
    "final-reopened-staging", "W12C",
    "initialize_once:2959>revalidate_predecessor_final_layout:1535>validate_child:1520>inspect_created_directory:601",
    22, "fixed-statfs-result",
    "published-envelope-retained;publication-attempts=1;whole-root-discard")
HALOFPX_L05Z_ROLE_MAP_ROW(
    "L05Z-FSTATFS-023", "SYS_fstatfs-inspect-directory", "fixture",
    "final-reopened-fixture", "W12C",
    "initialize_once:2959>revalidate_predecessor_final_layout:1542>reopen_directory:1319>inspect_directory:286",
    23, "fixed-statfs-result",
    "published-envelope-retained;publication-attempts=1;whole-root-discard")
