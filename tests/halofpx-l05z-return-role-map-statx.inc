// Exact mapped-but-not-executable L05z statx research map. Execution remains
// closed: these rows document recovered semantic roles and do not admit a role
// to the executable returned-fault roster. A future change must independently
// implement and test per-role entry selectors, the successful-exit residual
// oracle described below, exact occurrence closure, and negative cross-role
// rejection before changing any role's admission state.
//
// The clean trace authority is
// nimo-2 trace.log SHA-256
// 501c684fe925432c86cf17813d1fbd04660def5e0b36b173ecb65a6c4c847fc8,
// with exactly 36 mapped statx observations at trace lines 685..927.
//
// Every row requires:
//   SYS_statx(exact_descriptor, "",
//             AT_EMPTY_PATH | AT_STATX_SYNC_AS_STAT,
//             STATX_BASIC_STATS | STATX_MNT_ID, nonnull_output)
// where the descriptor is bound to the row's exact device/inode/mount identity.
// For a late returned fault, the successful result must be zero and the tracee
// output must include STATX_MNT_ID, match the role mount, and agree in file
// type, permission bits, and size with a same-descriptor entry snapshot before
// its return is replaced. Btrfs directory sizes and the retained fixture's
// encoded sizes are observations, not portable literals; use the entry snapshot
// and runtime root-marker/predecessor-envelope encoded sizes.

HALOFPX_L05Z_ROLE_MAP_ROW("L05Z-STATX-001", "SYS_statx-empty-path-basic-stats-mnt-id", "parent", "premutation-reopened-parent", "W03", "inspect_directory", 1, "single", "root-marker-unchanged;envelopes-empty;staging-empty")
HALOFPX_L05Z_ROLE_MAP_ROW("L05Z-STATX-002", "SYS_statx-empty-path-basic-stats-mnt-id", "candidate-root", "premutation-reopened-root", "W03", "inspect_directory", 2, "single", "root-marker-unchanged;envelopes-empty;staging-empty")
HALOFPX_L05Z_ROLE_MAP_ROW("L05Z-STATX-003", "SYS_statx-empty-path-basic-stats-mnt-id", "writer.lock", "premutation-reopened-writer", "W03", "inspect_writer_lock", 3, "single", "root-marker-unchanged;envelopes-empty;staging-empty")
HALOFPX_L05Z_ROLE_MAP_ROW("L05Z-STATX-004", "SYS_statx-empty-path-basic-stats-mnt-id", "envelopes", "premutation-reopened-envelopes", "W03", "inspect_created_directory", 4, "single", "root-marker-unchanged;envelopes-empty;staging-empty")
HALOFPX_L05Z_ROLE_MAP_ROW("L05Z-STATX-005", "SYS_statx-empty-path-basic-stats-mnt-id", "attempts", "premutation-reopened-attempts", "W03", "inspect_created_directory", 5, "single", "root-marker-unchanged;envelopes-empty;staging-empty")
HALOFPX_L05Z_ROLE_MAP_ROW("L05Z-STATX-006", "SYS_statx-empty-path-basic-stats-mnt-id", "staging", "premutation-reopened-staging", "W03", "inspect_created_directory", 6, "single", "root-marker-unchanged;envelopes-empty;staging-empty")
HALOFPX_L05Z_ROLE_MAP_ROW("L05Z-STATX-007", "SYS_statx-empty-path-basic-stats-mnt-id", "fixture", "premutation-reopened-fixture", "W03", "inspect_directory", 7, "single", "root-marker-unchanged;envelopes-empty;staging-empty")
HALOFPX_L05Z_ROLE_MAP_ROW("L05Z-STATX-008", "SYS_statx-empty-path-basic-stats-mnt-id", "primitive.lock", "premutation-reopened-fixture-lock", "W03", "inspect_fixture_lock", 8, "single", "root-marker-unchanged;envelopes-empty;staging-empty")
HALOFPX_L05Z_ROLE_MAP_ROW("L05Z-STATX-009", "SYS_statx-empty-path-basic-stats-mnt-id", "initializing-root-marker", "marker-after", "W03", "inspect_marker_file", 9, "single", "root-marker-unchanged;envelopes-empty;staging-empty")
HALOFPX_L05Z_ROLE_MAP_ROW("L05Z-STATX-010", "SYS_statx-empty-path-basic-stats-mnt-id", "envelopes", "envelopes", "W04", "inspect_created_directory", 10, "single", "root-marker-unchanged;envelopes-empty;staging-empty")
HALOFPX_L05Z_ROLE_MAP_ROW("L05Z-STATX-011", "SYS_statx-empty-path-basic-stats-mnt-id", "predecessor-envelope-transient", "temp-created-pre-fchmod", "W04", "inspect_marker_file", 11, "single", "root-marker-unchanged;envelopes-empty;temp-retained-mode-subset-of-0600-size0")
HALOFPX_L05Z_ROLE_MAP_ROW("L05Z-STATX-012", "SYS_statx-empty-path-basic-stats-mnt-id", "predecessor-envelope-transient", "envelope-write", "W05", "inspect_marker_file", 12, "single", "root-marker-unchanged;envelopes-empty;temp-retained-mode0600-size0")
HALOFPX_L05Z_ROLE_MAP_ROW("L05Z-STATX-013", "SYS_statx-empty-path-basic-stats-mnt-id", "predecessor-envelope-transient", "envelope-write", "W06", "inspect_marker_file", 13, "single", "root-marker-unchanged;envelopes-empty;temp-retained-mode0600-runtime-size")
HALOFPX_L05Z_ROLE_MAP_ROW("L05Z-STATX-014", "SYS_statx-empty-path-basic-stats-mnt-id", "parent", "prepublication-reopened-parent", "W09A", "inspect_directory", 14, "single", "root-marker-unchanged;envelopes-empty;temp-retained-mode0600-runtime-size")
HALOFPX_L05Z_ROLE_MAP_ROW("L05Z-STATX-015", "SYS_statx-empty-path-basic-stats-mnt-id", "candidate-root", "prepublication-reopened-root", "W09A", "inspect_directory", 15, "single", "root-marker-unchanged;envelopes-empty;temp-retained-mode0600-runtime-size")
HALOFPX_L05Z_ROLE_MAP_ROW("L05Z-STATX-016", "SYS_statx-empty-path-basic-stats-mnt-id", "writer.lock", "prepublication-reopened-writer", "W09A", "inspect_writer_lock", 16, "single", "root-marker-unchanged;envelopes-empty;temp-retained-mode0600-runtime-size")
HALOFPX_L05Z_ROLE_MAP_ROW("L05Z-STATX-017", "SYS_statx-empty-path-basic-stats-mnt-id", "envelopes", "prepublication-reopened-envelopes", "W09A", "inspect_created_directory", 17, "single", "root-marker-unchanged;envelopes-empty;temp-retained-mode0600-runtime-size")
HALOFPX_L05Z_ROLE_MAP_ROW("L05Z-STATX-018", "SYS_statx-empty-path-basic-stats-mnt-id", "attempts", "prepublication-reopened-attempts", "W09A", "inspect_created_directory", 18, "single", "root-marker-unchanged;envelopes-empty;temp-retained-mode0600-runtime-size")
HALOFPX_L05Z_ROLE_MAP_ROW("L05Z-STATX-019", "SYS_statx-empty-path-basic-stats-mnt-id", "staging", "prepublication-reopened-staging", "W09A", "inspect_created_directory", 19, "single", "root-marker-unchanged;envelopes-empty;temp-retained-mode0600-runtime-size")
HALOFPX_L05Z_ROLE_MAP_ROW("L05Z-STATX-020", "SYS_statx-empty-path-basic-stats-mnt-id", "fixture", "prepublication-reopened-fixture", "W09A", "inspect_directory", 20, "single", "root-marker-unchanged;envelopes-empty;temp-retained-mode0600-runtime-size")
HALOFPX_L05Z_ROLE_MAP_ROW("L05Z-STATX-021", "SYS_statx-empty-path-basic-stats-mnt-id", "primitive.lock", "prepublication-reopened-fixture-lock", "W09A", "inspect_fixture_lock", 21, "single", "root-marker-unchanged;envelopes-empty;temp-retained-mode0600-runtime-size")
HALOFPX_L05Z_ROLE_MAP_ROW("L05Z-STATX-022", "SYS_statx-empty-path-basic-stats-mnt-id", "initializing-root-marker", "barrier-marker", "W09A", "inspect_marker_file", 22, "single", "root-marker-unchanged;envelopes-empty;temp-retained-mode0600-runtime-size")
HALOFPX_L05Z_ROLE_MAP_ROW("L05Z-STATX-023", "SYS_statx-empty-path-basic-stats-mnt-id", "staging", "envelope-staging-validation", "W09A", "inspect_created_directory", 23, "single", "root-marker-unchanged;envelopes-empty;temp-retained-mode0600-runtime-size")
HALOFPX_L05Z_ROLE_MAP_ROW("L05Z-STATX-024", "SYS_statx-empty-path-basic-stats-mnt-id", "predecessor-envelope-transient", "envelope-readonly", "W09A", "inspect_marker_file", 24, "single", "root-marker-unchanged;envelopes-empty;temp-retained-mode0600-runtime-size")
HALOFPX_L05Z_ROLE_MAP_ROW("L05Z-STATX-025", "SYS_statx-empty-path-basic-stats-mnt-id", "predecessor-envelope-published", "published-envelope", "W12A", "inspect_marker_file", 25, "single", "root-marker-unchanged;published-envelope-retained-mode0600-runtime-size;staging-empty")
HALOFPX_L05Z_ROLE_MAP_ROW("L05Z-STATX-026", "SYS_statx-empty-path-basic-stats-mnt-id", "initializing-root-marker", "final-marker", "W12B", "inspect_marker_file", 26, "single", "root-marker-unchanged;published-envelope-retained-mode0600-runtime-size;staging-empty")
HALOFPX_L05Z_ROLE_MAP_ROW("L05Z-STATX-027", "SYS_statx-empty-path-basic-stats-mnt-id", "parent", "final-reopened-parent", "W12C", "inspect_directory", 27, "single", "root-marker-unchanged;published-envelope-retained-mode0600-runtime-size;staging-empty")
HALOFPX_L05Z_ROLE_MAP_ROW("L05Z-STATX-028", "SYS_statx-empty-path-basic-stats-mnt-id", "candidate-root", "final-reopened-root", "W12C", "inspect_directory", 28, "single", "root-marker-unchanged;published-envelope-retained-mode0600-runtime-size;staging-empty")
HALOFPX_L05Z_ROLE_MAP_ROW("L05Z-STATX-029", "SYS_statx-empty-path-basic-stats-mnt-id", "writer.lock", "final-reopened-writer", "W12C", "inspect_writer_lock", 29, "single", "root-marker-unchanged;published-envelope-retained-mode0600-runtime-size;staging-empty")
HALOFPX_L05Z_ROLE_MAP_ROW("L05Z-STATX-030", "SYS_statx-empty-path-basic-stats-mnt-id", "envelopes", "final-reopened-envelopes", "W12C", "inspect_created_directory", 30, "single", "root-marker-unchanged;published-envelope-retained-mode0600-runtime-size;staging-empty")
HALOFPX_L05Z_ROLE_MAP_ROW("L05Z-STATX-031", "SYS_statx-empty-path-basic-stats-mnt-id", "attempts", "final-reopened-attempts", "W12C", "inspect_created_directory", 31, "single", "root-marker-unchanged;published-envelope-retained-mode0600-runtime-size;staging-empty")
HALOFPX_L05Z_ROLE_MAP_ROW("L05Z-STATX-032", "SYS_statx-empty-path-basic-stats-mnt-id", "staging", "final-reopened-staging", "W12C", "inspect_created_directory", 32, "single", "root-marker-unchanged;published-envelope-retained-mode0600-runtime-size;staging-empty")
HALOFPX_L05Z_ROLE_MAP_ROW("L05Z-STATX-033", "SYS_statx-empty-path-basic-stats-mnt-id", "fixture", "final-reopened-fixture", "W12C", "inspect_directory", 33, "single", "root-marker-unchanged;published-envelope-retained-mode0600-runtime-size;staging-empty")
HALOFPX_L05Z_ROLE_MAP_ROW("L05Z-STATX-034", "SYS_statx-empty-path-basic-stats-mnt-id", "primitive.lock", "final-reopened-fixture-lock", "W12C", "inspect_fixture_lock", 34, "single", "root-marker-unchanged;published-envelope-retained-mode0600-runtime-size;staging-empty")
HALOFPX_L05Z_ROLE_MAP_ROW("L05Z-STATX-035", "SYS_statx-empty-path-basic-stats-mnt-id", "predecessor-envelope-published", "final-envelope-confirmation", "W12D", "inspect_marker_file", 35, "single", "root-marker-unchanged;published-envelope-retained-mode0600-runtime-size;staging-empty")
HALOFPX_L05Z_ROLE_MAP_ROW("L05Z-STATX-036", "SYS_statx-empty-path-basic-stats-mnt-id", "initializing-root-marker", "final-marker-confirmation", "W12D", "inspect_marker_file", 36, "single", "root-marker-unchanged;published-envelope-retained-mode0600-runtime-size;staging-empty")
