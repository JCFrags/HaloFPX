// L05z readlink returned-fault role map.
//
// MAPPED BUT NOT EXECUTABLE: every row remains closed pending implementation,
// adversarial review, and qualification of a target-owned exact selector and
// late-success oracle.  This fragment records source/trace mapping only and
// does not admit any returned-fault execution.
//
// A future selector must require the live child and the admitted L05z core
// gate, SYS_readlink, count 4097, and the canonical source-generated spelling
// "/proc/self/fd/<n>".  It must parse <n> as the current tracee descriptor and
// bind it to the row's semantic descriptor role with pinned device, inode,
// mount, type, and path authority.  Physical descriptor numbers observed in a
// trace are deliberately excluded from hashed authority; descriptor reuse must
// never substitute for the semantic binding.
//
// A future success and late-success oracle must account for readlink output not
// being NUL terminated.  It must require the returned length to equal the
// request-derived expected target length and compare exactly that many target
// bytes.  The expected target is
// the admitted canonical parent/root/fixture path, a target-owned suffix under
// the canonical root, or the authenticated digest-derived envelope basename.
// The bytes and request-relative length are deterministic; the absolute numeric
// length is intentionally not frozen because qualification-root lengths vary.
// A late returned fault is admitted only after this exact successful byte and
// length oracle passes.  residual_oracle names the required partial audit/tree
// state after the injected returned fault.

HALOFPX_L05Z_ROLE_MAP_ROW(
    "L05Z-READLINK-001", "SYS_readlink-proc-self-fd-4097", "parent",
    "reopened-parent;alias-parent-fd", "W03",
    "fd_canonical_path/revalidate_parent/revalidate_accumulated_prefix:pre-mutation",
    1, "target-bytes", "O3")
HALOFPX_L05Z_ROLE_MAP_ROW(
    "L05Z-READLINK-002", "SYS_readlink-proc-self-fd-4097", "candidate-root",
    "reopened-root;alias-root-fd", "W03",
    "fd_canonical_path/reopen_directory/revalidate_accumulated_prefix:pre-mutation",
    2, "target-bytes", "O3")
HALOFPX_L05Z_ROLE_MAP_ROW(
    "L05Z-READLINK-003", "SYS_readlink-proc-self-fd-4097", "envelopes",
    "reopened-envelopes;alias-envelopes-identity", "W03",
    "inspect_created_directory/reopen_created_directory/revalidate_accumulated_prefix:pre-mutation",
    3, "target-bytes", "O3")
HALOFPX_L05Z_ROLE_MAP_ROW(
    "L05Z-READLINK-004", "SYS_readlink-proc-self-fd-4097", "attempts",
    "reopened-attempts;alias-attempts-identity", "W03",
    "inspect_created_directory/reopen_created_directory/revalidate_accumulated_prefix:pre-mutation",
    4, "target-bytes", "O3")
HALOFPX_L05Z_ROLE_MAP_ROW(
    "L05Z-READLINK-005", "SYS_readlink-proc-self-fd-4097", "staging",
    "reopened-staging;alias-staging-fd", "W03",
    "inspect_created_directory/reopen_created_directory/revalidate_accumulated_prefix:pre-mutation",
    5, "target-bytes", "O3")
HALOFPX_L05Z_ROLE_MAP_ROW(
    "L05Z-READLINK-006", "SYS_readlink-proc-self-fd-4097", "fixture",
    "reopened-fixture;alias-fixture-fd", "W03",
    "fd_canonical_path/reopen_directory/revalidate_accumulated_prefix:pre-mutation",
    6, "target-bytes", "O3")
HALOFPX_L05Z_ROLE_MAP_ROW(
    "L05Z-READLINK-007", "SYS_readlink-proc-self-fd-4097", "initializing-root-marker",
    "marker-after-fd;alias-marker-identity", "W03",
    "inspect_marker_file/initialize_predecessor_envelope_once:marker-before-envelope",
    7, "target-bytes", "O3")
HALOFPX_L05Z_ROLE_MAP_ROW(
    "L05Z-READLINK-008", "SYS_readlink-proc-self-fd-4097", "envelopes",
    "envelopes-fd;alias-envelopes-identity", "W04",
    "inspect_created_directory/initialize_predecessor_envelope_once:envelopes-empty",
    8, "target-bytes", "O4")
HALOFPX_L05Z_ROLE_MAP_ROW(
    "L05Z-READLINK-009", "SYS_readlink-proc-self-fd-4097", "predecessor-envelope-transient",
    "envelope-fd;alias-envelope-identity", "W04",
    "inspect_marker_file/initialize_predecessor_envelope_once:temp-created",
    9, "target-bytes", "O5a")
HALOFPX_L05Z_ROLE_MAP_ROW(
    "L05Z-READLINK-010", "SYS_readlink-proc-self-fd-4097", "predecessor-envelope-transient",
    "envelope-fd;alias-readlink-009", "W05",
    "inspect_marker_file/initialize_predecessor_envelope_once:temp-mode-revalidated",
    10, "target-bytes", "O5b")
HALOFPX_L05Z_ROLE_MAP_ROW(
    "L05Z-READLINK-011", "SYS_readlink-proc-self-fd-4097", "predecessor-envelope-transient",
    "envelope-fd;alias-readlink-009-010", "W06",
    "inspect_marker_file/initialize_predecessor_envelope_once:temp-write-revalidated",
    11, "target-bytes", "O6")
HALOFPX_L05Z_ROLE_MAP_ROW(
    "L05Z-READLINK-012", "SYS_readlink-proc-self-fd-4097", "parent",
    "reopened-parent;alias-parent-fd", "W09A",
    "fd_canonical_path/revalidate_parent/revalidate_accumulated_prefix:pre-publication",
    12, "target-bytes", "O9")
HALOFPX_L05Z_ROLE_MAP_ROW(
    "L05Z-READLINK-013", "SYS_readlink-proc-self-fd-4097", "candidate-root",
    "reopened-root;alias-root-fd", "W09A",
    "fd_canonical_path/reopen_directory/revalidate_accumulated_prefix:pre-publication",
    13, "target-bytes", "O9")
HALOFPX_L05Z_ROLE_MAP_ROW(
    "L05Z-READLINK-014", "SYS_readlink-proc-self-fd-4097", "envelopes",
    "reopened-envelopes;alias-envelopes-fd", "W09A",
    "inspect_created_directory/reopen_created_directory/revalidate_accumulated_prefix:pre-publication",
    14, "target-bytes", "O9")
HALOFPX_L05Z_ROLE_MAP_ROW(
    "L05Z-READLINK-015", "SYS_readlink-proc-self-fd-4097", "attempts",
    "reopened-attempts;alias-attempts-identity", "W09A",
    "inspect_created_directory/reopen_created_directory/revalidate_accumulated_prefix:pre-publication",
    15, "target-bytes", "O9")
HALOFPX_L05Z_ROLE_MAP_ROW(
    "L05Z-READLINK-016", "SYS_readlink-proc-self-fd-4097", "staging",
    "reopened-staging;alias-staging-fd", "W09A",
    "inspect_created_directory/reopen_created_directory/revalidate_accumulated_prefix:pre-publication",
    16, "target-bytes", "O9")
HALOFPX_L05Z_ROLE_MAP_ROW(
    "L05Z-READLINK-017", "SYS_readlink-proc-self-fd-4097", "fixture",
    "reopened-fixture;alias-fixture-fd", "W09A",
    "fd_canonical_path/reopen_directory/revalidate_accumulated_prefix:pre-publication",
    17, "target-bytes", "O9")
HALOFPX_L05Z_ROLE_MAP_ROW(
    "L05Z-READLINK-018", "SYS_readlink-proc-self-fd-4097", "initializing-root-marker",
    "barrier-marker-fd;alias-marker-identity", "W09A",
    "inspect_marker_file/initialize_predecessor_envelope_once:barrier-marker",
    18, "target-bytes", "O9")
HALOFPX_L05Z_ROLE_MAP_ROW(
    "L05Z-READLINK-019", "SYS_readlink-proc-self-fd-4097", "staging",
    "envelope-staging-validation-fd;alias-staging-fd", "W09A",
    "inspect_created_directory/initialize_predecessor_envelope_once:staging-single-temp",
    19, "target-bytes", "O9")
HALOFPX_L05Z_ROLE_MAP_ROW(
    "L05Z-READLINK-020", "SYS_readlink-proc-self-fd-4097", "predecessor-envelope-transient",
    "envelope-readonly-fd;alias-envelope-fd", "W09A",
    "inspect_marker_file/initialize_predecessor_envelope_once:readonly-temp-before-rename",
    20, "target-bytes", "O9")
HALOFPX_L05Z_ROLE_MAP_ROW(
    "L05Z-READLINK-021", "SYS_readlink-proc-self-fd-4097", "predecessor-envelope-published",
    "published-envelope-fd;alias-envelope-fd-and-readonly-fd", "W12A",
    "inspect_marker_file/initialize_predecessor_envelope_once:published-envelope",
    21, "target-bytes", "O12a")
HALOFPX_L05Z_ROLE_MAP_ROW(
    "L05Z-READLINK-022", "SYS_readlink-proc-self-fd-4097", "initializing-root-marker",
    "final-marker-fd;alias-marker-identity", "W12B",
    "inspect_marker_file/initialize_predecessor_envelope_once:final-marker",
    22, "target-bytes", "O12b")
HALOFPX_L05Z_ROLE_MAP_ROW(
    "L05Z-READLINK-023", "SYS_readlink-proc-self-fd-4097", "parent",
    "reopened-parent;alias-parent-fd", "W12C",
    "fd_canonical_path/revalidate_parent/revalidate_predecessor_final_layout",
    23, "target-bytes", "O12c")
HALOFPX_L05Z_ROLE_MAP_ROW(
    "L05Z-READLINK-024", "SYS_readlink-proc-self-fd-4097", "candidate-root",
    "reopened-root;alias-root-fd", "W12C",
    "fd_canonical_path/reopen_directory/revalidate_predecessor_final_layout",
    24, "target-bytes", "O12c")
HALOFPX_L05Z_ROLE_MAP_ROW(
    "L05Z-READLINK-025", "SYS_readlink-proc-self-fd-4097", "envelopes",
    "reopened-envelopes;alias-envelopes-fd", "W12C",
    "inspect_created_directory/revalidate_predecessor_final_layout:envelopes",
    25, "target-bytes", "O12c")
HALOFPX_L05Z_ROLE_MAP_ROW(
    "L05Z-READLINK-026", "SYS_readlink-proc-self-fd-4097", "attempts",
    "reopened-attempts;alias-attempts-identity", "W12C",
    "inspect_created_directory/revalidate_predecessor_final_layout:attempts",
    26, "target-bytes", "O12c")
HALOFPX_L05Z_ROLE_MAP_ROW(
    "L05Z-READLINK-027", "SYS_readlink-proc-self-fd-4097", "staging",
    "reopened-staging;alias-staging-fd", "W12C",
    "inspect_created_directory/revalidate_predecessor_final_layout:staging",
    27, "target-bytes", "O12c")
HALOFPX_L05Z_ROLE_MAP_ROW(
    "L05Z-READLINK-028", "SYS_readlink-proc-self-fd-4097", "fixture",
    "reopened-fixture;alias-fixture-fd", "W12C",
    "fd_canonical_path/reopen_directory/revalidate_predecessor_final_layout",
    28, "target-bytes", "O12c")
HALOFPX_L05Z_ROLE_MAP_ROW(
    "L05Z-READLINK-029", "SYS_readlink-proc-self-fd-4097", "predecessor-envelope-published",
    "final-envelope-confirmation-fd;alias-published-envelope-fd", "W12D",
    "inspect_marker_file/initialize_predecessor_envelope_once:final-envelope-confirmation",
    29, "target-bytes", "O12c")
HALOFPX_L05Z_ROLE_MAP_ROW(
    "L05Z-READLINK-030", "SYS_readlink-proc-self-fd-4097", "initializing-root-marker",
    "final-marker-confirmation-fd;alias-final-marker-fd", "W12D",
    "inspect_marker_file/initialize_predecessor_envelope_once:final-marker-confirmation",
    30, "target-bytes", "O12c")
