// Exact L05z Btrfs-facts role map at HaloFPX eba2e36764e1624d6d20bb00657dd7f2fe64a1be.
//
// The clean stream contains twenty directory inspections.  Each inspection
// emits BTRFS_IOC_FS_INFO followed by BTRFS_IOC_GET_SUBVOL_INFO, so the forty
// rows below alternate subtype rather than describing forty FS_INFO calls.
// Physical fd numbers are intentionally excluded: descriptor_role binds the
// exact pinned object and source window, while clean_ordinal binds its place in
// the mapped combined facts stream.
//
// A late-success oracle must read the tracee's arg2 before replacing the
// successful return.  For BTRFS_IOC_FS_INFO it compares only fsid with the
// controller-snapshotted filesystem UUID for the pinned object.  For
// BTRFS_IOC_GET_SUBVOL_INFO it compares only uuid with that object's pinned
// subvolume UUID.  Padding and fields unused by production are not authority.
// A pre-suppressed call provides no output-field authority.
//
// Retained Linux observation:
//   nimo-2:/var/tmp/halofpx-qualification/
//     l05z-role-map-discovery-eba2e36-20260719-nimo2/trace.730429
//   exact role segment: lines 662..888
//   sha256: 6693b059dfefd108f3e364b49bf46d7806417cdccb019d9397251d6791064742
// This trace is discovery evidence, not a promotion receipt; observed fd
// numbers are a physical profile and never semantic identity.
// These roles are mapped but not executable: execution admission remains
// closed until the controller consumes this map with exact subtype, object,
// window, output-field, and residual-oracle enforcement and passes its gates.

#ifndef HALOFPX_L05Z_ROLE_MAP_ROW
#error "Define HALOFPX_L05Z_ROLE_MAP_ROW before including this fragment"
#endif

HALOFPX_L05Z_ROLE_MAP_ROW("L05Z-FACTS-001", "BTRFS_IOC_FS_INFO",        "parent",         "premutation-reopened-parent",        "W03",  "2604>1365>1296>294",  1,  "single", "phase=3;envelope_bits=111100000000000000000;temp=absent;published=absent")
HALOFPX_L05Z_ROLE_MAP_ROW("L05Z-FACTS-002", "BTRFS_IOC_GET_SUBVOL_INFO", "parent",         "premutation-reopened-parent",        "W03",  "2604>1365>1296>304",  2,  "single", "phase=3;envelope_bits=111100000000000000000;temp=absent;published=absent")
HALOFPX_L05Z_ROLE_MAP_ROW("L05Z-FACTS-003", "BTRFS_IOC_FS_INFO",        "candidate-root", "premutation-reopened-root",          "W03",  "2604>1368>1319>294",  3,  "single", "phase=3;envelope_bits=111100000000000000000;temp=absent;published=absent")
HALOFPX_L05Z_ROLE_MAP_ROW("L05Z-FACTS-004", "BTRFS_IOC_GET_SUBVOL_INFO", "candidate-root", "premutation-reopened-root",          "W03",  "2604>1368>1319>304",  4,  "single", "phase=3;envelope_bits=111100000000000000000;temp=absent;published=absent")
HALOFPX_L05Z_ROLE_MAP_ROW("L05Z-FACTS-005", "BTRFS_IOC_FS_INFO",        "envelopes",      "premutation-reopened-envelopes",     "W03",  "2604>1401>1340>610",  5,  "single", "phase=3;envelope_bits=111100000000000000000;temp=absent;published=absent")
HALOFPX_L05Z_ROLE_MAP_ROW("L05Z-FACTS-006", "BTRFS_IOC_GET_SUBVOL_INFO", "envelopes",      "premutation-reopened-envelopes",     "W03",  "2604>1401>1340>617",  6,  "single", "phase=3;envelope_bits=111100000000000000000;temp=absent;published=absent")
HALOFPX_L05Z_ROLE_MAP_ROW("L05Z-FACTS-007", "BTRFS_IOC_FS_INFO",        "attempts",       "premutation-reopened-attempts",      "W03",  "2604>1407>1340>610",  7,  "single", "phase=3;envelope_bits=111100000000000000000;temp=absent;published=absent")
HALOFPX_L05Z_ROLE_MAP_ROW("L05Z-FACTS-008", "BTRFS_IOC_GET_SUBVOL_INFO", "attempts",       "premutation-reopened-attempts",      "W03",  "2604>1407>1340>617",  8,  "single", "phase=3;envelope_bits=111100000000000000000;temp=absent;published=absent")
HALOFPX_L05Z_ROLE_MAP_ROW("L05Z-FACTS-009", "BTRFS_IOC_FS_INFO",        "staging",        "premutation-reopened-staging",       "W03",  "2604>1413>1340>610",  9,  "single", "phase=3;envelope_bits=111100000000000000000;temp=absent;published=absent")
HALOFPX_L05Z_ROLE_MAP_ROW("L05Z-FACTS-010", "BTRFS_IOC_GET_SUBVOL_INFO", "staging",        "premutation-reopened-staging",       "W03",  "2604>1413>1340>617", 10,  "single", "phase=3;envelope_bits=111100000000000000000;temp=absent;published=absent")
HALOFPX_L05Z_ROLE_MAP_ROW("L05Z-FACTS-011", "BTRFS_IOC_FS_INFO",        "fixture",        "premutation-reopened-fixture",       "W03",  "2604>1424>1319>294", 11,  "single", "phase=3;envelope_bits=111100000000000000000;temp=absent;published=absent")
HALOFPX_L05Z_ROLE_MAP_ROW("L05Z-FACTS-012", "BTRFS_IOC_GET_SUBVOL_INFO", "fixture",        "premutation-reopened-fixture",       "W03",  "2604>1424>1319>304", 12,  "single", "phase=3;envelope_bits=111100000000000000000;temp=absent;published=absent")

HALOFPX_L05Z_ROLE_MAP_ROW("L05Z-FACTS-013", "BTRFS_IOC_FS_INFO",        "envelopes",      "held-envelopes-fd",                  "W04",  "2683>2689>610",       13,  "single", "phase=4;envelope_bits=111111000000000000000;temp=absent;published=absent")
HALOFPX_L05Z_ROLE_MAP_ROW("L05Z-FACTS-014", "BTRFS_IOC_GET_SUBVOL_INFO", "envelopes",      "held-envelopes-fd",                  "W04",  "2683>2689>617",       14,  "single", "phase=4;envelope_bits=111111000000000000000;temp=absent;published=absent")

HALOFPX_L05Z_ROLE_MAP_ROW("L05Z-FACTS-015", "BTRFS_IOC_FS_INFO",        "parent",         "prepublication-reopened-parent",     "W09A", "2778>1365>1296>294", 15,  "single", "phase=9;envelope_bits=111111111111000000000;temp=complete-synchronized;published=absent")
HALOFPX_L05Z_ROLE_MAP_ROW("L05Z-FACTS-016", "BTRFS_IOC_GET_SUBVOL_INFO", "parent",         "prepublication-reopened-parent",     "W09A", "2778>1365>1296>304", 16,  "single", "phase=9;envelope_bits=111111111111000000000;temp=complete-synchronized;published=absent")
HALOFPX_L05Z_ROLE_MAP_ROW("L05Z-FACTS-017", "BTRFS_IOC_FS_INFO",        "candidate-root", "prepublication-reopened-root",       "W09A", "2778>1368>1319>294", 17,  "single", "phase=9;envelope_bits=111111111111000000000;temp=complete-synchronized;published=absent")
HALOFPX_L05Z_ROLE_MAP_ROW("L05Z-FACTS-018", "BTRFS_IOC_GET_SUBVOL_INFO", "candidate-root", "prepublication-reopened-root",       "W09A", "2778>1368>1319>304", 18,  "single", "phase=9;envelope_bits=111111111111000000000;temp=complete-synchronized;published=absent")
HALOFPX_L05Z_ROLE_MAP_ROW("L05Z-FACTS-019", "BTRFS_IOC_FS_INFO",        "envelopes",      "prepublication-reopened-envelopes",  "W09A", "2778>1401>1340>610", 19,  "single", "phase=9;envelope_bits=111111111111000000000;temp=complete-synchronized;published=absent")
HALOFPX_L05Z_ROLE_MAP_ROW("L05Z-FACTS-020", "BTRFS_IOC_GET_SUBVOL_INFO", "envelopes",      "prepublication-reopened-envelopes",  "W09A", "2778>1401>1340>617", 20,  "single", "phase=9;envelope_bits=111111111111000000000;temp=complete-synchronized;published=absent")
HALOFPX_L05Z_ROLE_MAP_ROW("L05Z-FACTS-021", "BTRFS_IOC_FS_INFO",        "attempts",       "prepublication-reopened-attempts",   "W09A", "2778>1407>1340>610", 21,  "single", "phase=9;envelope_bits=111111111111000000000;temp=complete-synchronized;published=absent")
HALOFPX_L05Z_ROLE_MAP_ROW("L05Z-FACTS-022", "BTRFS_IOC_GET_SUBVOL_INFO", "attempts",       "prepublication-reopened-attempts",   "W09A", "2778>1407>1340>617", 22,  "single", "phase=9;envelope_bits=111111111111000000000;temp=complete-synchronized;published=absent")
HALOFPX_L05Z_ROLE_MAP_ROW("L05Z-FACTS-023", "BTRFS_IOC_FS_INFO",        "staging",        "prepublication-reopened-staging",    "W09A", "2778>1413>1340>610", 23,  "single", "phase=9;envelope_bits=111111111111000000000;temp=complete-synchronized;published=absent")
HALOFPX_L05Z_ROLE_MAP_ROW("L05Z-FACTS-024", "BTRFS_IOC_GET_SUBVOL_INFO", "staging",        "prepublication-reopened-staging",    "W09A", "2778>1413>1340>617", 24,  "single", "phase=9;envelope_bits=111111111111000000000;temp=complete-synchronized;published=absent")
HALOFPX_L05Z_ROLE_MAP_ROW("L05Z-FACTS-025", "BTRFS_IOC_FS_INFO",        "fixture",        "prepublication-reopened-fixture",    "W09A", "2778>1424>1319>294", 25,  "single", "phase=9;envelope_bits=111111111111000000000;temp=complete-synchronized;published=absent")
HALOFPX_L05Z_ROLE_MAP_ROW("L05Z-FACTS-026", "BTRFS_IOC_GET_SUBVOL_INFO", "fixture",        "prepublication-reopened-fixture",    "W09A", "2778>1424>1319>304", 26,  "single", "phase=9;envelope_bits=111111111111000000000;temp=complete-synchronized;published=absent")
HALOFPX_L05Z_ROLE_MAP_ROW("L05Z-FACTS-027", "BTRFS_IOC_FS_INFO",        "staging",        "envelope-staging-validation-fd",     "W09A", "2817>2828>610",       27,  "single", "phase=9;envelope_bits=111111111111000000000;temp=complete-synchronized;published=absent")
HALOFPX_L05Z_ROLE_MAP_ROW("L05Z-FACTS-028", "BTRFS_IOC_GET_SUBVOL_INFO", "staging",        "envelope-staging-validation-fd",     "W09A", "2817>2828>617",       28,  "single", "phase=9;envelope_bits=111111111111000000000;temp=complete-synchronized;published=absent")

HALOFPX_L05Z_ROLE_MAP_ROW("L05Z-FACTS-029", "BTRFS_IOC_FS_INFO",        "parent",         "final-reopened-parent",              "W12C", "2959>1472>1296>294", 29,  "single", "phase=12;envelope_bits=111111111111111111100;temp=absent-after-rename;published=synchronized;final-layout=unmatched")
HALOFPX_L05Z_ROLE_MAP_ROW("L05Z-FACTS-030", "BTRFS_IOC_GET_SUBVOL_INFO", "parent",         "final-reopened-parent",              "W12C", "2959>1472>1296>304", 30,  "single", "phase=12;envelope_bits=111111111111111111100;temp=absent-after-rename;published=synchronized;final-layout=unmatched")
HALOFPX_L05Z_ROLE_MAP_ROW("L05Z-FACTS-031", "BTRFS_IOC_FS_INFO",        "candidate-root", "final-reopened-root",                "W12C", "2959>1475>1319>294", 31,  "single", "phase=12;envelope_bits=111111111111111111100;temp=absent-after-rename;published=synchronized;final-layout=unmatched")
HALOFPX_L05Z_ROLE_MAP_ROW("L05Z-FACTS-032", "BTRFS_IOC_GET_SUBVOL_INFO", "candidate-root", "final-reopened-root",                "W12C", "2959>1475>1319>304", 32,  "single", "phase=12;envelope_bits=111111111111111111100;temp=absent-after-rename;published=synchronized;final-layout=unmatched")
HALOFPX_L05Z_ROLE_MAP_ROW("L05Z-FACTS-033", "BTRFS_IOC_FS_INFO",        "envelopes",      "final-reopened-envelopes",           "W12C", "2959>1533>1520>610", 33,  "single", "phase=12;envelope_bits=111111111111111111100;temp=absent-after-rename;published=synchronized;final-layout=unmatched")
HALOFPX_L05Z_ROLE_MAP_ROW("L05Z-FACTS-034", "BTRFS_IOC_GET_SUBVOL_INFO", "envelopes",      "final-reopened-envelopes",           "W12C", "2959>1533>1520>617", 34,  "single", "phase=12;envelope_bits=111111111111111111100;temp=absent-after-rename;published=synchronized;final-layout=unmatched")
HALOFPX_L05Z_ROLE_MAP_ROW("L05Z-FACTS-035", "BTRFS_IOC_FS_INFO",        "attempts",       "final-reopened-attempts",            "W12C", "2959>1534>1520>610", 35,  "single", "phase=12;envelope_bits=111111111111111111100;temp=absent-after-rename;published=synchronized;final-layout=unmatched")
HALOFPX_L05Z_ROLE_MAP_ROW("L05Z-FACTS-036", "BTRFS_IOC_GET_SUBVOL_INFO", "attempts",       "final-reopened-attempts",            "W12C", "2959>1534>1520>617", 36,  "single", "phase=12;envelope_bits=111111111111111111100;temp=absent-after-rename;published=synchronized;final-layout=unmatched")
HALOFPX_L05Z_ROLE_MAP_ROW("L05Z-FACTS-037", "BTRFS_IOC_FS_INFO",        "staging",        "final-reopened-staging",             "W12C", "2959>1535>1520>610", 37,  "single", "phase=12;envelope_bits=111111111111111111100;temp=absent-after-rename;published=synchronized;final-layout=unmatched")
HALOFPX_L05Z_ROLE_MAP_ROW("L05Z-FACTS-038", "BTRFS_IOC_GET_SUBVOL_INFO", "staging",        "final-reopened-staging",             "W12C", "2959>1535>1520>617", 38,  "single", "phase=12;envelope_bits=111111111111111111100;temp=absent-after-rename;published=synchronized;final-layout=unmatched")
HALOFPX_L05Z_ROLE_MAP_ROW("L05Z-FACTS-039", "BTRFS_IOC_FS_INFO",        "fixture",        "final-reopened-fixture",             "W12C", "2959>1542>1319>294", 39,  "single", "phase=12;envelope_bits=111111111111111111100;temp=absent-after-rename;published=synchronized;final-layout=unmatched")
HALOFPX_L05Z_ROLE_MAP_ROW("L05Z-FACTS-040", "BTRFS_IOC_GET_SUBVOL_INFO", "fixture",        "final-reopened-fixture",             "W12C", "2959>1542>1319>304", 40,  "single", "phase=12;envelope_bits=111111111111111111100;temp=absent-after-rename;published=synchronized;final-layout=unmatched")
