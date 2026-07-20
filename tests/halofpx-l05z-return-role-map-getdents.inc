// Mapped-but-not-executable L05z getdents64 role map; execution remains closed.
// Current nimo-2 Btrfs profile evidence observed each of the 16 logical
// directory streams as exactly one positive data fragment followed by terminal
// EOF.  That physical shape is neither source-exact nor executable admission.
// An extra positive fragment, a missing EOF, or stream/call reordering is a
// profile mismatch and is not authority.
// The schema value POS denotes the single mapped data-fragment-1 observation;
// EOF denotes the paired terminal-zero observation.
// Directory records are structurally bounded and names are compared as an
// unordered exact set; d_off and inode values are deliberately not identities.

#ifndef HALOFPX_L05Z_ROLE_MAP_ROW
#error "Define HALOFPX_L05Z_ROLE_MAP_ROW before including this role map"
#endif

HALOFPX_L05Z_ROLE_MAP_ROW("L05Z-GETDENTS-001", "SYS_getdents64-4096", "candidate-root", "reopened-root", "W03", "scan-01:pre-mutation-prefix/root-layout", 1, "POS", "l05y-barrier:no-envelope")
HALOFPX_L05Z_ROLE_MAP_ROW("L05Z-GETDENTS-002", "SYS_getdents64-4096", "candidate-root", "reopened-root", "W03", "scan-01:pre-mutation-prefix/root-layout", 2, "EOF", "l05y-barrier:no-envelope")
HALOFPX_L05Z_ROLE_MAP_ROW("L05Z-GETDENTS-003", "SYS_getdents64-4096", "envelopes", "reopened-envelopes", "W03", "scan-02:pre-mutation-prefix/envelopes-empty", 3, "POS", "l05y-barrier:no-envelope")
HALOFPX_L05Z_ROLE_MAP_ROW("L05Z-GETDENTS-004", "SYS_getdents64-4096", "envelopes", "reopened-envelopes", "W03", "scan-02:pre-mutation-prefix/envelopes-empty", 4, "EOF", "l05y-barrier:no-envelope")
HALOFPX_L05Z_ROLE_MAP_ROW("L05Z-GETDENTS-005", "SYS_getdents64-4096", "attempts", "reopened-attempts", "W03", "scan-03:pre-mutation-prefix/attempts-empty", 5, "POS", "l05y-barrier:no-envelope")
HALOFPX_L05Z_ROLE_MAP_ROW("L05Z-GETDENTS-006", "SYS_getdents64-4096", "attempts", "reopened-attempts", "W03", "scan-03:pre-mutation-prefix/attempts-empty", 6, "EOF", "l05y-barrier:no-envelope")
HALOFPX_L05Z_ROLE_MAP_ROW("L05Z-GETDENTS-007", "SYS_getdents64-4096", "staging", "reopened-staging", "W03", "scan-04:pre-mutation-prefix/staging-empty", 7, "POS", "l05y-barrier:no-envelope")
HALOFPX_L05Z_ROLE_MAP_ROW("L05Z-GETDENTS-008", "SYS_getdents64-4096", "staging", "reopened-staging", "W03", "scan-04:pre-mutation-prefix/staging-empty", 8, "EOF", "l05y-barrier:no-envelope")
HALOFPX_L05Z_ROLE_MAP_ROW("L05Z-GETDENTS-009", "SYS_getdents64-4096", "fixture", "reopened-fixture", "W03", "scan-05:pre-mutation-prefix/fixture-layout", 9, "POS", "l05y-barrier:no-envelope")
HALOFPX_L05Z_ROLE_MAP_ROW("L05Z-GETDENTS-010", "SYS_getdents64-4096", "fixture", "reopened-fixture", "W03", "scan-05:pre-mutation-prefix/fixture-layout", 10, "EOF", "l05y-barrier:no-envelope")

HALOFPX_L05Z_ROLE_MAP_ROW("L05Z-GETDENTS-011", "SYS_getdents64-4096", "envelopes", "held-envelopes", "W04", "scan-06:initial-envelopes-open/empty-layout", 11, "POS", "l05y-barrier:no-envelope")
HALOFPX_L05Z_ROLE_MAP_ROW("L05Z-GETDENTS-012", "SYS_getdents64-4096", "envelopes", "held-envelopes", "W04", "scan-06:initial-envelopes-open/empty-layout", 12, "EOF", "l05y-barrier:no-envelope")

HALOFPX_L05Z_ROLE_MAP_ROW("L05Z-GETDENTS-013", "SYS_getdents64-4096", "candidate-root", "reopened-root", "W09A", "scan-07:pre-publication-prefix/root-layout", 13, "POS", "l05z-temp:synchronized-unpublished")
HALOFPX_L05Z_ROLE_MAP_ROW("L05Z-GETDENTS-014", "SYS_getdents64-4096", "candidate-root", "reopened-root", "W09A", "scan-07:pre-publication-prefix/root-layout", 14, "EOF", "l05z-temp:synchronized-unpublished")
HALOFPX_L05Z_ROLE_MAP_ROW("L05Z-GETDENTS-015", "SYS_getdents64-4096", "envelopes", "reopened-envelopes", "W09A", "scan-08:pre-publication-prefix/envelopes-empty", 15, "POS", "l05z-temp:synchronized-unpublished")
HALOFPX_L05Z_ROLE_MAP_ROW("L05Z-GETDENTS-016", "SYS_getdents64-4096", "envelopes", "reopened-envelopes", "W09A", "scan-08:pre-publication-prefix/envelopes-empty", 16, "EOF", "l05z-temp:synchronized-unpublished")
HALOFPX_L05Z_ROLE_MAP_ROW("L05Z-GETDENTS-017", "SYS_getdents64-4096", "attempts", "reopened-attempts", "W09A", "scan-09:pre-publication-prefix/attempts-empty", 17, "POS", "l05z-temp:synchronized-unpublished")
HALOFPX_L05Z_ROLE_MAP_ROW("L05Z-GETDENTS-018", "SYS_getdents64-4096", "attempts", "reopened-attempts", "W09A", "scan-09:pre-publication-prefix/attempts-empty", 18, "EOF", "l05z-temp:synchronized-unpublished")
HALOFPX_L05Z_ROLE_MAP_ROW("L05Z-GETDENTS-019", "SYS_getdents64-4096", "fixture", "reopened-fixture", "W09A", "scan-10:pre-publication-prefix/fixture-layout", 19, "POS", "l05z-temp:synchronized-unpublished")
HALOFPX_L05Z_ROLE_MAP_ROW("L05Z-GETDENTS-020", "SYS_getdents64-4096", "fixture", "reopened-fixture", "W09A", "scan-10:pre-publication-prefix/fixture-layout", 20, "EOF", "l05z-temp:synchronized-unpublished")
HALOFPX_L05Z_ROLE_MAP_ROW("L05Z-GETDENTS-021", "SYS_getdents64-4096", "staging", "staging-validation-alias", "W09A", "scan-11:pre-publication-staging/single-envelope-temp", 21, "POS", "l05z-temp:synchronized-unpublished")
HALOFPX_L05Z_ROLE_MAP_ROW("L05Z-GETDENTS-022", "SYS_getdents64-4096", "staging", "staging-validation-alias", "W09A", "scan-11:pre-publication-staging/single-envelope-temp", 22, "EOF", "l05z-temp:synchronized-unpublished")

HALOFPX_L05Z_ROLE_MAP_ROW("L05Z-GETDENTS-023", "SYS_getdents64-4096", "candidate-root", "final-reopened-root", "W12C", "scan-12:predecessor-final-layout/root-layout", 23, "POS", "l05z-envelope:published-and-directory-synchronized")
HALOFPX_L05Z_ROLE_MAP_ROW("L05Z-GETDENTS-024", "SYS_getdents64-4096", "candidate-root", "final-reopened-root", "W12C", "scan-12:predecessor-final-layout/root-layout", 24, "EOF", "l05z-envelope:published-and-directory-synchronized")
HALOFPX_L05Z_ROLE_MAP_ROW("L05Z-GETDENTS-025", "SYS_getdents64-4096", "envelopes", "final-reopened-envelopes", "W12C", "scan-13:predecessor-final-layout/single-digest-envelope", 25, "POS", "l05z-envelope:published-and-directory-synchronized")
HALOFPX_L05Z_ROLE_MAP_ROW("L05Z-GETDENTS-026", "SYS_getdents64-4096", "envelopes", "final-reopened-envelopes", "W12C", "scan-13:predecessor-final-layout/single-digest-envelope", 26, "EOF", "l05z-envelope:published-and-directory-synchronized")
HALOFPX_L05Z_ROLE_MAP_ROW("L05Z-GETDENTS-027", "SYS_getdents64-4096", "attempts", "final-reopened-attempts", "W12C", "scan-14:predecessor-final-layout/attempts-empty", 27, "POS", "l05z-envelope:published-and-directory-synchronized")
HALOFPX_L05Z_ROLE_MAP_ROW("L05Z-GETDENTS-028", "SYS_getdents64-4096", "attempts", "final-reopened-attempts", "W12C", "scan-14:predecessor-final-layout/attempts-empty", 28, "EOF", "l05z-envelope:published-and-directory-synchronized")
HALOFPX_L05Z_ROLE_MAP_ROW("L05Z-GETDENTS-029", "SYS_getdents64-4096", "staging", "final-reopened-staging", "W12C", "scan-15:predecessor-final-layout/staging-empty", 29, "POS", "l05z-envelope:published-and-directory-synchronized")
HALOFPX_L05Z_ROLE_MAP_ROW("L05Z-GETDENTS-030", "SYS_getdents64-4096", "staging", "final-reopened-staging", "W12C", "scan-15:predecessor-final-layout/staging-empty", 30, "EOF", "l05z-envelope:published-and-directory-synchronized")
HALOFPX_L05Z_ROLE_MAP_ROW("L05Z-GETDENTS-031", "SYS_getdents64-4096", "fixture", "final-reopened-fixture", "W12C", "scan-16:predecessor-final-layout/fixture-layout", 31, "POS", "l05z-envelope:published-and-directory-synchronized")
HALOFPX_L05Z_ROLE_MAP_ROW("L05Z-GETDENTS-032", "SYS_getdents64-4096", "fixture", "final-reopened-fixture", "W12C", "scan-16:predecessor-final-layout/fixture-layout", 32, "EOF", "l05z-envelope:published-and-directory-synchronized")
