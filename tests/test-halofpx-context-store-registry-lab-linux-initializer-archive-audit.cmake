if(NOT DEFINED ARCHIVE_MANIFEST OR NOT DEFINED NM_TOOL OR NOT DEFINED AR_TOOL)
  message(FATAL_ERROR "ARCHIVE_MANIFEST, NM_TOOL, and AR_TOOL are required")
endif()
include("${ARCHIVE_MANIFEST}")
if(NOT EXISTS "${HALOFPX_L05T_ARCHIVE}")
  message(FATAL_ERROR "L05t archive does not exist: ${HALOFPX_L05T_ARCHIVE}")
endif()

execute_process(COMMAND "${AR_TOOL}" t "${HALOFPX_L05T_ARCHIVE}"
  RESULT_VARIABLE AR_RESULT OUTPUT_VARIABLE MEMBERS ERROR_VARIABLE AR_ERROR)
if(NOT AR_RESULT EQUAL 0)
  message(FATAL_ERROR "archive listing failed: ${AR_ERROR}")
endif()
string(REGEX MATCHALL "[^\r\n]+" MEMBER_LINES "${MEMBERS}")
list(LENGTH MEMBER_LINES MEMBER_COUNT)
if(NOT MEMBER_COUNT EQUAL 2)
  message(FATAL_ERROR "initializer archive must contain exactly the seam and sealed-input objects; got ${MEMBER_COUNT}")
endif()

execute_process(COMMAND "${NM_TOOL}" -g --defined-only --format=posix -C "${HALOFPX_L05T_ARCHIVE}"
  RESULT_VARIABLE DEFINED_RESULT OUTPUT_VARIABLE DEFINED ERROR_VARIABLE DEFINED_ERROR)
if(NOT DEFINED_RESULT EQUAL 0)
  message(FATAL_ERROR "defined-symbol audit failed: ${DEFINED_ERROR}")
endif()
string(REGEX MATCHALL "[^\r\n]+" DEFINED_LINES "${DEFINED}")
set(DEFINED_TEXT_SYMBOL_LINES "")
foreach(LINE IN LISTS DEFINED_LINES)
  if(LINE MATCHES " T [0-9A-Fa-f]+ [0-9A-Fa-f]+$")
    list(APPEND DEFINED_TEXT_SYMBOL_LINES "${LINE}")
  elseif(LINE MATCHES "DW.ref.__gxx_personality_v0 V ")
    # Compiler exception-unwind metadata; not a callable authority edge.
  elseif(SANITIZED_BUILD AND LINE MATCHES "std::.* W ")
    # Debug/sanitizer builds may emit weak std::array/copy template helpers.
  elseif(SANITIZED_BUILD AND LINE MATCHES "context_store_protected_registry_facts_result::authenticated_facts\\(\\) const W ")
    # Inline status-gated facts accessor; it carries no bytes or authority.
  elseif(SANITIZED_BUILD AND LINE MATCHES "operator (new\\(unsigned long, void\\*\\)|delete\\(void\\*, void\\*\\)) W ")
    # Inline placement-new lifetime helpers; no allocating operator is admitted.
  elseif(LINE MATCHES "^[^:]+:$" OR LINE STREQUAL "")
    # Archive member heading or blank line.
  else()
    message(FATAL_ERROR "initializer archive defines an unadmitted global symbol: ${LINE}")
  endif()
endforeach()
list(LENGTH DEFINED_TEXT_SYMBOL_LINES DEFINED_SYMBOL_COUNT)
if(NOT DEFINED_SYMBOL_COUNT EQUAL 3)
  message(FATAL_ERROR "initializer archive must define exactly three callable symbols: ${DEFINED}")
endif()
foreach(REQUIRED IN ITEMS
    "halofpx::context_store_registry_lab_linux_initializer_predecessor_digest_v1"
    "halofpx::registry_lab::linux_initializer::inspect_sealed_inputs_once"
    "halofpx::registry_lab::linux_initializer::initialize_writer_lock_anchor_once")
  string(FIND "${DEFINED}" "${REQUIRED}" HIT)
  if(HIT EQUAL -1)
    message(FATAL_ERROR "initializer archive is missing admitted definition: ${REQUIRED}")
  endif()
endforeach()
foreach(FORBIDDEN IN ITEMS " T open" " T read" " T write" " T rename" " T fsync"
    "server_context" "llama_server" "publication_coordinator")
  string(FIND "${DEFINED}" "${FORBIDDEN}" HIT)
  if(NOT HIT EQUAL -1)
    message(FATAL_ERROR "forbidden L05t defined symbol: ${FORBIDDEN}")
  endif()
endforeach()

execute_process(COMMAND "${NM_TOOL}" -u -C "${HALOFPX_L05T_ARCHIVE}"
  RESULT_VARIABLE UNDEFINED_RESULT OUTPUT_VARIABLE UNDEFINED ERROR_VARIABLE UNDEFINED_ERROR)
if(NOT UNDEFINED_RESULT EQUAL 0)
  message(FATAL_ERROR "undefined-symbol audit failed: ${UNDEFINED_ERROR}")
endif()
string(REGEX MATCHALL "[^\r\n]+" UNDEFINED_LINES "${UNDEFINED}")
set(UNDEFINED_SYMBOL_LINES "")
foreach(LINE IN LISTS UNDEFINED_LINES)
  if(LINE MATCHES "[ \t]U[ \t]")
    list(APPEND UNDEFINED_SYMBOL_LINES "${LINE}")
  endif()
endforeach()
set(HALOFPX_IMPORT_COUNT 0)
foreach(LINE IN LISTS UNDEFINED_SYMBOL_LINES)
  if(LINE MATCHES "halofpx::")
    math(EXPR HALOFPX_IMPORT_COUNT "${HALOFPX_IMPORT_COUNT} + 1")
    if(NOT LINE MATCHES
        "context_store_registry_lab_(registry_envelope_digest_v1|linux_initializer_predecessor_digest_v1)" AND
       NOT LINE MATCHES "context_store_verify_protected_registry_facts_v1")
      message(FATAL_ERROR "L05t archive imports an unadmitted HaloFPX symbol: ${LINE}")
    endif()
  elseif(SANITIZED_BUILD AND LINE MATCHES "[ \t]U[ \t]+__(asan|ubsan)_")
    # Compiler sanitizer runtime only; it carries no project or I/O authority.
  elseif(SANITIZED_BUILD AND LINE MATCHES "[ \t]U[ \t]+std::__glibcxx_assert_fail")
    # libstdc++ debug assertion runtime introduced by the sanitizer build type.
  elseif(LINE MATCHES "[ \t]U[ \t]+(clock_gettime|clock_nanosleep|close|__errno_location|fcntl|fchmod|fsync|fstat|fstatfs|fstatvfs|geteuid|ioctl|__gxx_personality_v0|(__isoc23_)?strtol|memcmp|memchr|memcpy|memmove|memset|mlock|mmap|munlock|munmap|open|pread|read|readlink|sigfillset|sigprocmask|snprintf|__stack_chk_fail|strcmp|strlen|strnlen|syscall|sysconf)$")
    # Exact POSIX/libc/compiler surface admitted by sealed input plus L05w.
  else()
    message(FATAL_ERROR "L05t archive imports an unadmitted symbol: ${LINE}")
  endif()
endforeach()
if(NOT HALOFPX_IMPORT_COUNT EQUAL 3)
  message(FATAL_ERROR "initializer archive must import only the two-stage digest lineage and facts verifier: ${UNDEFINED}")
endif()
foreach(FORBIDDEN IN ITEMS " openat" " write" "pwrite" " rename" "fdatasync"
    "mkdir" "unlink" "system" "fork" "exec" "socket" "connect")
  string(FIND "${UNDEFINED}" "${FORBIDDEN}" HIT)
  if(NOT HIT EQUAL -1)
    message(FATAL_ERROR "forbidden L05t imported symbol: ${FORBIDDEN}")
  endif()
endforeach()

message(STATUS "PASS: L05t seam and L05u/L05v sealed authenticated-input archive/import isolation")
