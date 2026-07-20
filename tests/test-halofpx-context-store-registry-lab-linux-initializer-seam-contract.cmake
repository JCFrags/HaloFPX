set(ROOT "${SOURCE_ROOT}")
set(H "${ROOT}/tools/server/halofpx-context-store-registry-lab-linux-initializer-internal.h")
set(C "${ROOT}/tools/server/halofpx-context-store-registry-lab-linux-initializer.cpp")
foreach(F IN ITEMS "${H}" "${C}")
  if(NOT EXISTS "${F}")
    message(FATAL_ERROR "missing L05t initializer seam source: ${F}")
  endif()
  file(READ "${F}" TEXT)
  foreach(FORBIDDEN IN ITEMS
      "open(" "openat" "fopen(" "read(" "write(" "pwrite(" "rename"
      "fsync(" "fdatasync(" "mkdir" "unlink" "std::filesystem" "getenv("
      "system(" "fork(" "exec(" "ptrace" "llama-ai" "CachyLLama")
    string(FIND "${TEXT}" "${FORBIDDEN}" HIT)
    if(NOT HIT EQUAL -1)
      message(FATAL_ERROR "L05t initializer seam contains forbidden mutation token ${FORBIDDEN}: ${F}")
    endif()
  endforeach()
endforeach()

set(INPUTS "${ROOT}/tools/server/halofpx-context-store-registry-lab-linux-initializer-inputs.cpp")
if(NOT EXISTS "${INPUTS}")
  message(FATAL_ERROR "missing M63-01b sealed-input source: ${INPUTS}")
endif()
file(READ "${INPUTS}" INPUT_TEXT)
foreach(REQUIRED IN ITEMS
    "inspect_sealed_inputs_once"
    "/memfd:halofpx-registry-lab-credential (deleted)"
    "/memfd:halofpx-registry-lab-predecessor (deleted)"
    "F_SEAL_SEAL | F_SEAL_SHRINK | F_SEAL_GROW | F_SEAL_WRITE"
    "FD_CLOEXEC"
    "::open(\"/proc/self/task\""
    "::open(\"/proc/self/fd\""
    "::sigprocmask(SIG_SETMASK"
    "::syscall(SYS_unshare, CLONE_FILES)"
    "::mmap(nullptr, session.mapping_size"
    "::new (session.mapping) secure_inputs {}"
    "::mlock(session.mapping, session.mapping_size)"
    "::munlock(session.mapping, session.mapping_size)"
    "::munmap(session.mapping, session.mapping_size)"
    "context_store_registry_lab_linux_initializer_predecessor_digest_v1"
    "context_store_verify_protected_registry_facts_v1"
    "wire_credential_storage"
    "session.wire_credential = ::new ("
    "wire_credential->~context_store_registry_lab_credential()"
    "all_zero(session.secure->credential.data(),"
    "std::array<std::uint8_t, 1024> initializing_marker"
    "std::array<std::uint8_t, 1024> initializing_marker_readback"
    "std::array<std::uint8_t, 1024> predecessor_readback"
    "secure->~secure_inputs()"
    "predecessor_authenticated_pins_matched_no_root_access"
    "predecessor_authenticated_under_supplied_credential"
    "launcher_receipt_matched"
    "root_or_fixture_syscall_count")
  string(FIND "${INPUT_TEXT}" "${REQUIRED}" HIT)
  if(HIT EQUAL -1)
    message(FATAL_ERROR "missing M63-01b sealed-input contract token: ${REQUIRED}")
  endif()
endforeach()
string(FIND "${INPUT_TEXT}" "::mlock(session.mapping, session.mapping_size)" INPUT_MLOCK)
string(FIND "${INPUT_TEXT}" "output.transport_final_revalidation_matched = true" INPUT_FINAL_PIN)
string(FIND "${INPUT_TEXT}" "session.wire_credential = ::new (" INPUT_WIRE_CONSTRUCT)
string(FIND "${INPUT_TEXT}" "wipe(session.secure->credential.data(), session.secure->credential.size())" INPUT_RAW_WIPE)
string(FIND "${INPUT_TEXT}" "session.authenticated = true" INPUT_AUTHENTICATED)
if(INPUT_MLOCK EQUAL -1 OR INPUT_FINAL_PIN EQUAL -1 OR INPUT_WIRE_CONSTRUCT EQUAL -1 OR
   INPUT_RAW_WIPE EQUAL -1 OR INPUT_AUTHENTICATED EQUAL -1 OR
   NOT INPUT_MLOCK LESS INPUT_FINAL_PIN OR
   NOT INPUT_FINAL_PIN LESS INPUT_WIRE_CONSTRUCT OR
   NOT INPUT_WIRE_CONSTRUCT LESS INPUT_RAW_WIPE OR
   NOT INPUT_RAW_WIPE LESS INPUT_AUTHENTICATED)
  message(FATAL_ERROR "wire credential lifetime must begin only in locked storage after final transport/pin validation, then raw input must be wiped before authentication is exposed")
endif()
string(REGEX MATCHALL "::open\\(" INPUT_OPEN_CALLS "${INPUT_TEXT}")
list(LENGTH INPUT_OPEN_CALLS INPUT_OPEN_COUNT)
if(NOT INPUT_OPEN_COUNT EQUAL 2)
  message(FATAL_ERROR "sealed-input source may open only /proc/self/task and /proc/self/fd")
endif()
foreach(FORBIDDEN IN ITEMS
    "qualify_once(" "context_store_registry_lab_parse_credential_v1("
    "context_store_verify_protected_registry_v1(" "authority_binding(")
  string(FIND "${INPUT_TEXT}" "${FORBIDDEN}" HIT)
  if(NOT HIT EQUAL -1)
    message(FATAL_ERROR "M63-01b sealed-input slice contains forbidden root/auth/mutation token: ${FORBIDDEN}")
  endif()
endforeach()

foreach(REQUIRED IN ITEMS
    "struct authenticated_input_session"
    "authenticated_input_session(const authenticated_input_session &) = delete"
    "authenticate_sealed_inputs_for_session(input, session, output)"
    "cleanup_authenticated_input_storage"
    "restore_authenticated_input_signal_mask")
  string(FIND "${INPUT_TEXT}" "${REQUIRED}" HIT)
  if(HIT EQUAL -1)
    message(FATAL_ERROR "missing shared file-private authenticated-session contract: ${REQUIRED}")
  endif()
endforeach()

set(ANCHOR "${ROOT}/tools/server/halofpx-context-store-registry-lab-linux-initializer-anchor.inc")
if(NOT EXISTS "${ANCHOR}")
  message(FATAL_ERROR "missing L05w writer-lock anchor implementation")
endif()
file(READ "${ANCHOR}" ANCHOR_TEXT)
string(FIND "${ANCHOR_TEXT}" "output.initializing_marker_phase_ordinal = 13" L05Y_BARRIER)
string(FIND "${ANCHOR_TEXT}" "output.initializing_marker_write_descriptor_closed = true" MARKER_WRITABLE_CLOSED)
string(FIND "${ANCHOR_TEXT}" "envelope_fd = open_contained(" ENVELOPE_CREATE)
string(FIND "${ANCHOR_TEXT}" "envelope_readonly_fd = open_contained(" ENVELOPE_FINAL_OPEN)
string(FIND "${ANCHOR_TEXT}" "output.predecessor_envelope_pre_publication_revalidation_matched = true" ENVELOPE_FINAL_FACT)
string(FIND "${ANCHOR_TEXT}" "::syscall(SYS_renameat2, staging_fd, \"initialize-envelope.tmp\"" ENVELOPE_RENAME)
string(FIND "${ANCHOR_TEXT}" "if (::fsync(envelopes_fd) != 0" ENVELOPES_FSYNC)
if(NOT ENVELOPE_RENAME EQUAL -1)
  string(SUBSTRING "${ANCHOR_TEXT}" ${ENVELOPE_RENAME} -1 ENVELOPE_PUBLICATION_TAIL)
  string(FIND "${ENVELOPE_PUBLICATION_TAIL}" "if (::fsync(staging_fd) != 0" STAGING_FSYNC_RELATIVE)
  if(STAGING_FSYNC_RELATIVE EQUAL -1)
    set(STAGING_FSYNC -1)
  else()
    math(EXPR STAGING_FSYNC "${ENVELOPE_RENAME} + ${STAGING_FSYNC_RELATIVE}")
  endif()
else()
  set(STAGING_FSYNC -1)
endif()
if(L05Y_BARRIER EQUAL -1 OR MARKER_WRITABLE_CLOSED EQUAL -1 OR
   ENVELOPE_CREATE EQUAL -1 OR ENVELOPE_FINAL_OPEN EQUAL -1 OR
   ENVELOPE_FINAL_FACT EQUAL -1 OR ENVELOPE_RENAME EQUAL -1 OR
   ENVELOPES_FSYNC EQUAL -1 OR STAGING_FSYNC EQUAL -1 OR
   NOT L05Y_BARRIER LESS MARKER_WRITABLE_CLOSED OR
   NOT MARKER_WRITABLE_CLOSED LESS ENVELOPE_CREATE OR
   NOT ENVELOPE_CREATE LESS ENVELOPE_FINAL_OPEN OR
   NOT ENVELOPE_FINAL_OPEN LESS ENVELOPE_FINAL_FACT OR
   NOT ENVELOPE_FINAL_FACT LESS ENVELOPE_RENAME OR
   NOT ENVELOPE_RENAME LESS ENVELOPES_FSYNC OR
   NOT ENVELOPES_FSYNC LESS STAGING_FSYNC)
  message(FATAL_ERROR "L05z writable-marker closure and publication order is not exact")
endif()
string(REGEX MATCHALL "write_exact_at_zero\\([ \t\r\n]*envelope_fd" L05Z_ENVELOPE_WRITES "${ANCHOR_TEXT}")
string(REGEX MATCHALL "read_exact_at_zero\\([ \t\r\n]*envelope_fd" L05Z_INITIAL_READS "${ANCHOR_TEXT}")
string(REGEX MATCHALL "if \\(::fsync\\(envelope_fd\\) != 0" L05Z_FILE_SYNCS "${ANCHOR_TEXT}")
string(REGEX MATCHALL "SYS_renameat2, staging_fd, \"initialize-envelope.tmp\"" L05Z_RENAMES "${ANCHOR_TEXT}")
list(LENGTH L05Z_ENVELOPE_WRITES L05Z_ENVELOPE_WRITE_COUNT)
list(LENGTH L05Z_INITIAL_READS L05Z_INITIAL_READ_COUNT)
list(LENGTH L05Z_FILE_SYNCS L05Z_FILE_SYNC_COUNT)
list(LENGTH L05Z_RENAMES L05Z_RENAME_COUNT)
if(NOT L05Z_ENVELOPE_WRITE_COUNT EQUAL 1 OR
   NOT L05Z_INITIAL_READ_COUNT EQUAL 1 OR
   NOT L05Z_FILE_SYNC_COUNT EQUAL 1 OR
   NOT L05Z_RENAME_COUNT EQUAL 1)
  message(FATAL_ERROR "L05z envelope publication syscall occurrence counts changed")
endif()
string(SUBSTRING "${ANCHOR_TEXT}" ${ENVELOPE_CREATE} -1 L05Z_ENVELOPE_TAIL)
string(FIND "${L05Z_ENVELOPE_TAIL}" "result = write_exact_at_zero(" L05Z_WRITE)
string(FIND "${L05Z_ENVELOPE_TAIL}" "result = read_exact_at_zero(" L05Z_READ)
string(FIND "${L05Z_ENVELOPE_TAIL}" "output.predecessor_envelope_readback_exact = true" L05Z_EOF_EXACT)
string(FIND "${L05Z_ENVELOPE_TAIL}" "output.predecessor_envelope_readback_authenticated = true" L05Z_AUTH)
string(FIND "${L05Z_ENVELOPE_TAIL}" "if (::fsync(envelope_fd) != 0" L05Z_FILE_FSYNC)
if(L05Z_WRITE EQUAL -1 OR L05Z_READ EQUAL -1 OR L05Z_EOF_EXACT EQUAL -1 OR
   L05Z_AUTH EQUAL -1 OR L05Z_FILE_FSYNC EQUAL -1 OR
   NOT L05Z_WRITE LESS L05Z_READ OR NOT L05Z_READ LESS L05Z_EOF_EXACT OR
   NOT L05Z_EOF_EXACT LESS L05Z_AUTH OR NOT L05Z_AUTH LESS L05Z_FILE_FSYNC)
  message(FATAL_ERROR "L05z exact write/read/EOF/authentication/file-fsync order changed")
endif()
string(REGEX MATCHALL "trailing_count = ::pread\\(" EXACT_EOF_READS "${ANCHOR_TEXT}")
list(LENGTH EXACT_EOF_READS EXACT_EOF_READ_COUNT)
if(NOT EXACT_EOF_READ_COUNT EQUAL 1)
  message(FATAL_ERROR "bounded exact-I/O helper must own exactly one EOF pread")
endif()
foreach(REQUIRED IN ITEMS
    "initialize_writer_lock_anchor_once"
    "initialize_directory_prefix_once"
    "initialize_initializing_marker_once"
    "initialize_predecessor_envelope_once"
    "authenticate_sealed_inputs_for_session"
    "O_CREAT | O_EXCL | O_RDWR | O_CLOEXEC | O_NOFOLLOW"
    "RESOLVE_BENEATH | RESOLVE_NO_SYMLINKS"
    "RESOLVE_NO_MAGICLINKS | RESOLVE_NO_XDEV"
    "::fchmod(writer_fd, 0600)"
    "::fsync(writer_fd)"
    "SYS_getrandom"
    "F_OFD_SETLK"
    "F_UNLCK"
    "preexisting_root_discard_required"
    "clear_pre_latch_positive_audit"
    "if (!output.discard_required_latched)"
    "const bool retry_cleanup_unlock_eintr ="
    "!output.initializing_marker_write_descriptor_closed"
    "unlock_and_close(writer_fd, output.writer_lock_ofd_acquired,\n                                retry_cleanup_unlock_eintr"
    "unlock_and_close(fixture_lock_fd, output.fixture_lock_acquired,\n                                retry_cleanup_unlock_eintr"
    "discard_required_latched = true"
    "lock_anchor_qualified = true"
    "initialization_extent::writer_lock_anchor"
    "initialization_extent::directory_prefix"
    "SYS_mkdirat"
    "while (mkdir_result != 0 && errno == EINTR)"
    "O_PATH | O_DIRECTORY | O_CLOEXEC | O_NOFOLLOW"
    "SYS_fchmodat2, path_fd, \"\", 0700, AT_EMPTY_PATH"
    "while (chmod_result != 0 && errno == EINTR)"
    "& ~0700U) != 0"
    "compare_open_descriptions(path_fd, directory_fd, output)"
    "inspect_accumulated_prefix_layout"
    "revalidate_accumulated_prefix"
    "\"envelopes\""
    "\"attempts\""
    "\"staging\""
    "inspect_empty_directory(directory_fd, output)"
    "::fsync(directory_fd)"
    "::fsync(root_fd)"
    "envelopes_directory_final_revalidation_matched = true"
    "attempts_directory_final_revalidation_matched = true"
    "staging_directory_final_revalidation_matched = true"
    "directory_prefix_qualified = true"
    "initialization_extent::initializing_marker"
    "initialization_extent::predecessor_envelope"
    "context_store_registry_lab_path_policy_v1"
    "context_store_registry_lab_admit_root_v1"
    "context_store_registry_lab_encode_root_v1"
    "context_store_registry_lab_verify_v1"
    "context_store_registry_lab_root_state_v1::initializing"
    "\"initialize-root.tmp\""
    "\"root.marker\""
    "::pwrite"
    "SYS_renameat2"
    "RENAME_NOREPLACE"
    "initializing_marker_qualified = true"
    "staging_empty_after_marker_publication = true"
    "initializing_marker_pre_publication_revalidation_matched = true"
    "initializing_marker_readonly_reopen_matched = true")
  string(FIND "${ANCHOR_TEXT}" "${REQUIRED}" HIT)
  if(HIT EQUAL -1)
    message(FATAL_ERROR "missing L05w/L05x discard-only initializer contract token: ${REQUIRED}")
  endif()
endforeach()
foreach(REQUIRED IN ITEMS
    "context_store_verify_protected_registry_facts_v1"
    "initialize-envelope.tmp"
    "predecessor_envelope_digest_name_computed = true"
    "initializing_marker_write_descriptor_closed = true"
    "predecessor_envelope_pre_mutation_revalidation_matched = true"
    "predecessor_envelope_readback_authenticated = true"
    "predecessor_envelope_pre_publication_revalidation_matched = true"
    "envelopes_synchronized_after_predecessor = true"
    "staging_synchronized_after_predecessor = true"
    "predecessor_envelope_final_layout_matched = true"
    "predecessor_envelope_qualified = true")
  string(FIND "${ANCHOR_TEXT}" "${REQUIRED}" HIT)
  if(HIT EQUAL -1)
    message(FATAL_ERROR "missing L05z predecessor-envelope contract token: ${REQUIRED}")
  endif()
endforeach()
set(ANCHOR_TEST "${ROOT}/tests/test-halofpx-context-store-registry-lab-linux-initializer-anchor.cpp")
file(READ "${ANCHOR_TEST}" ANCHOR_TEST_TEXT)
foreach(REQUIRED IN ITEMS
    "std::array<char, 1024> envelope_suffix {}"
    "envelope=%u/%u/%u/%u/%u/%u/%u/%u/%u/%u/%u/%u/%u/%u/%u/%u/%u/%u/%u/%u/%u"
    "envelope_digest=%s envelope_name=%s envelope_dev=%llu"
    "envelope_inode=%llu envelope_mount=%llu envelope_size=%u envelope_phase=%u")
  string(FIND "${ANCHOR_TEST_TEXT}" "${REQUIRED}" HIT)
  if(HIT EQUAL -1)
    message(FATAL_ERROR "missing bounded L05z live-audit oracle token: ${REQUIRED}")
  endif()
endforeach()
string(FIND "${ANCHOR_TEXT}" "// Select and fully authenticate the exact staging inode immediately before" FINAL_SELECTION)
if(FINAL_SELECTION EQUAL -1)
  message(FATAL_ERROR "missing immediate pre-rename marker selection boundary")
endif()
string(SUBSTRING "${ANCHOR_TEXT}" ${FINAL_SELECTION} -1 FINAL_SELECTION_TEXT)
string(FIND "${FINAL_SELECTION_TEXT}" "marker_readonly_fd = open_contained(" FINAL_OPEN)
string(FIND "${FINAL_SELECTION_TEXT}" "compare_open_descriptions(marker_fd, marker_readonly_fd" FINAL_IDENTITY)
string(FIND "${FINAL_SELECTION_TEXT}" "read_exact_at_zero(marker_readonly_fd" FINAL_READ)
string(FIND "${FINAL_SELECTION_TEXT}" "const auto verified_readonly" FINAL_AUTH)
string(FIND "${FINAL_SELECTION_TEXT}" "initializing_marker_pre_publication_revalidation_matched = true" FINAL_FACT)
string(FIND "${FINAL_SELECTION_TEXT}" "SYS_renameat2" FINAL_RENAME)
if(FINAL_OPEN EQUAL -1 OR FINAL_IDENTITY EQUAL -1 OR FINAL_READ EQUAL -1 OR
   FINAL_AUTH EQUAL -1 OR FINAL_FACT EQUAL -1 OR FINAL_RENAME EQUAL -1 OR
   NOT FINAL_OPEN LESS FINAL_IDENTITY OR NOT FINAL_IDENTITY LESS FINAL_READ OR
   NOT FINAL_READ LESS FINAL_AUTH OR NOT FINAL_AUTH LESS FINAL_FACT OR
   NOT FINAL_FACT LESS FINAL_RENAME)
  message(FATAL_ERROR "marker identity, exact bytes/EOF, and authentication must immediately precede no-replace publication")
endif()
foreach(FORBIDDEN IN ITEMS
    "qualify_once(" "linux-preinit"
    "initialize-head.tmp" "initialize-marker.tmp" "HEAD"
    "::mkdir(" "::mkdirat(" "::fchmod(directory_fd" "::fchmodat2("
    "::rename(" "::write(" "SYS_write" "SYS_pwrite"
    "writev" "SYS_writev" "unlink" "fdatasync" "fork(" "exec("
    "llama-ai" "CachyLLama")
  string(FIND "${ANCHOR_TEXT}" "${FORBIDDEN}" HIT)
  if(NOT HIT EQUAL -1)
    message(FATAL_ERROR "L05w/L05x slice contains forbidden broader-mutation/link token: ${FORBIDDEN}")
  endif()
endforeach()

file(READ "${ROOT}/CMakeLists.txt" ROOT_CMAKE)
foreach(REQUIRED IN ITEMS
    "option(HALOFPX_REGISTRY_LAB_LINUX_INITIALIZER"
    "include(cmake/halofpx-registry-lab-linux-gates.cmake)")
  string(FIND "${ROOT_CMAKE}" "${REQUIRED}" HIT)
  if(HIT EQUAL -1)
    message(FATAL_ERROR "missing L05t root build gate: ${REQUIRED}")
  endif()
endforeach()

file(READ "${ROOT}/cmake/halofpx-registry-lab-linux-gates.cmake" GATE_CMAKE)
foreach(REQUIRED IN ITEMS
    "HALOFPX_REGISTRY_LAB_LINUX_INITIALIZER requires HALOFPX_REGISTRY_LAB_LINUX_PREINIT"
    "HALOFPX_REGISTRY_LAB_LINUX_INITIALIZER is available only on Linux"
    "HALOFPX_REGISTRY_LAB_LINUX_MUTATION is not admitted by ADR-0025")
  string(FIND "${GATE_CMAKE}" "${REQUIRED}" HIT)
  if(HIT EQUAL -1)
    message(FATAL_ERROR "missing L05t executable build gate: ${REQUIRED}")
  endif()
endforeach()

file(READ "${ROOT}/tools/server/CMakeLists.txt" SERVER_CMAKE)
foreach(REQUIRED IN ITEMS
    "add_library(halofpx-context-store-registry-lab-linux-initializer STATIC EXCLUDE_FROM_ALL"
    "target_link_libraries(halofpx-context-store-registry-lab-linux-initializer PRIVATE\n        halofpx-context-store-registry-lab-wire)"
    "add_executable(halofpx-l05x-ptrace-controller EXCLUDE_FROM_ALL"
    "add_executable(halofpx-l05x-return-fault-controller EXCLUDE_FROM_ALL"
    "add_executable(halofpx-l05y-ptrace-controller EXCLUDE_FROM_ALL"
    "add_executable(halofpx-l05y-return-fault-controller EXCLUDE_FROM_ALL"
    "if (CMAKE_SYSTEM_PROCESSOR MATCHES \"^(x86_64|amd64|AMD64)$\")"
    "add_executable(halofpx-l05z-ptrace-controller EXCLUDE_FROM_ALL"
    "add_executable(halofpx-l05z-return-fault-controller EXCLUDE_FROM_ALL")
  string(FIND "${SERVER_CMAKE}" "${REQUIRED}" HIT)
  if(HIT EQUAL -1)
    message(FATAL_ERROR "missing L05t excluded target boundary: ${REQUIRED}")
  endif()
endforeach()

string(REGEX MATCHALL
    "target_link_libraries\\(halofpx-context-store-registry-lab-linux-initializer"
    INITIALIZER_LINK_CALLS "${SERVER_CMAKE}")
list(LENGTH INITIALIZER_LINK_CALLS INITIALIZER_LINK_CALL_COUNT)
if(NOT INITIALIZER_LINK_CALL_COUNT EQUAL 1)
  message(FATAL_ERROR "initializer must own exactly one target_link_libraries call")
endif()
foreach(FORBIDDEN IN ITEMS
    "target_link_libraries(halofpx-context-store-registry-lab-linux-initializer PUBLIC"
    "target_link_libraries(halofpx-context-store-registry-lab-linux-initializer INTERFACE"
    "install(TARGETS halofpx-context-store-registry-lab-linux-initializer"
    "install(FILES halofpx-context-store-registry-lab-linux-initializer"
    "llama_build(halofpx-context-store-registry-lab-linux-initializer"
    "set(HALOFPX_REGISTRY_LAB_LINUX_INITIALIZER_TARGET")
  string(FIND "${SERVER_CMAKE}" "${FORBIDDEN}" HIT)
  if(NOT HIT EQUAL -1)
    message(FATAL_ERROR "initializer has a forbidden build/export edge: ${FORBIDDEN}")
  endif()
endforeach()

set(L05Z_PTRACE "${ROOT}/tests/halofpx-l05z-ptrace-controller.cpp")
set(L05Z_RETURN "${ROOT}/tests/halofpx-l05z-return-fault-controller.cpp")
foreach(CONTROLLER IN ITEMS "${L05Z_PTRACE}" "${L05Z_RETURN}")
  if(NOT EXISTS "${CONTROLLER}")
    message(FATAL_ERROR "missing excluded L05z qualification controller: ${CONTROLLER}")
  endif()
endforeach()
file(READ "${L05Z_PTRACE}" L05Z_PTRACE_TEXT)
foreach(REQUIRED IN ITEMS
    "openat2-initialize-envelope"
    "fchmod-initialize-envelope"
    "pwrite-initialize-envelope"
    "fsync-initialize-envelope"
    "renameat2-initialize-envelope"
    "fsync-envelopes-envelope"
    "fsync-staging-envelope"
    "derive_golden_predecessor"
    "golden_authority_unchanged"
    "envelopes_fsync_succeeded"
    "--live-envelope-controller")
  string(FIND "${L05Z_PTRACE_TEXT}" "${REQUIRED}" HIT)
  if(HIT EQUAL -1)
    message(FATAL_ERROR "missing L05z ptrace-controller authority token: ${REQUIRED}")
  endif()
endforeach()
file(READ "${L05Z_RETURN}" L05Z_RETURN_TEXT)
foreach(REQUIRED IN ITEMS
    "#include \"halofpx-l05z-return-hostile-manifest.inc\""
    "manifest_self_check"
    "static_assert(syscall_seam_base == 2087)"
    "step4_gate"
    "inherited_marker_publication_attempts"
    "cleanup_syscall_seen"
    "envelope_digest"
    "envelope_phase"
    "--live-envelope-controller")
  string(FIND "${L05Z_RETURN_TEXT}" "${REQUIRED}" HIT)
  if(HIT EQUAL -1)
    message(FATAL_ERROR "missing L05z returned-fault controller token: ${REQUIRED}")
  endif()
endforeach()
set(L05Z_RETURN_HOSTILE_MANIFEST
  "${ROOT}/tests/halofpx-l05z-return-hostile-manifest.inc")
if(NOT EXISTS "${L05Z_RETURN_HOSTILE_MANIFEST}")
  message(FATAL_ERROR "missing L05z canonical hostile manifest include")
endif()
file(READ "${L05Z_RETURN_HOSTILE_MANIFEST}" L05Z_RETURN_HOSTILE_MANIFEST_TEXT)
foreach(REQUIRED IN ITEMS
    "canonical_case_count = 1899"
    "sorted_id_set_hash_hex()"
    "1028ac1be238cd418e48699c509f27a0dfcded792869c4014a9f5f6d9a8e8698"
    "canonical_manifest_sha256"
    "halofpx.l05z.case-shard.v1"
    "shard_union_self_check"
    "select_case(std::string_view case_id)")
  string(FIND "${L05Z_RETURN_HOSTILE_MANIFEST_TEXT}" "${REQUIRED}" HIT)
  if(HIT EQUAL -1)
    message(FATAL_ERROR "missing L05z canonical hostile-manifest token: ${REQUIRED}")
  endif()
endforeach()
string(FIND "${SERVER_CMAKE}"
  "target_link_libraries(halofpx-l05z-return-fault-controller PRIVATE\n                halofpx-context-store-registry-lab-linux-initializer)"
  L05Z_RETURN_TEST_LINK)
if(L05Z_RETURN_TEST_LINK EQUAL -1)
  message(FATAL_ERROR "missing L05z returned-fault controller's private test-only initializer link")
endif()

string(FIND "${SERVER_CMAKE}" "set(TARGET server-context)" PRODUCT_MARKER)
if(PRODUCT_MARKER EQUAL -1)
  message(FATAL_ERROR "server product marker is missing")
endif()
string(SUBSTRING "${SERVER_CMAKE}" ${PRODUCT_MARKER} -1 PRODUCT_TAIL)
foreach(FORBIDDEN_PRODUCT_EDGE IN ITEMS
    "halofpx-context-store-registry-lab-linux-initializer"
    "halofpx-l05z-ptrace-controller"
    "halofpx-l05z-return-fault-controller")
  string(FIND "${PRODUCT_TAIL}" "${FORBIDDEN_PRODUCT_EDGE}" PRODUCT_LEAK)
  if(NOT PRODUCT_LEAK EQUAL -1)
    message(FATAL_ERROR "L05z initializer/controller seam leaked into product/server linkage: ${FORBIDDEN_PRODUCT_EDGE}")
  endif()
endforeach()

message(STATUS "PASS: L05t seam through L05z discard-only predecessor-envelope/default-off contract")
