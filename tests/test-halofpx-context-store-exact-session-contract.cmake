if (NOT DEFINED HALOFPX_SOURCE_DIR OR NOT IS_DIRECTORY "${HALOFPX_SOURCE_DIR}")
    message(FATAL_ERROR "HALOFPX_SOURCE_DIR must name the source tree")
endif()

file(READ "${HALOFPX_SOURCE_DIR}/tools/server/CMakeLists.txt" server_cmake)
string(FIND "${server_cmake}"
    "add_library(halofpx-context-store-exact-session STATIC EXCLUDE_FROM_ALL"
    excluded_target)
if (excluded_target EQUAL -1)
    message(FATAL_ERROR "L10b exact-session target must remain EXCLUDE_FROM_ALL")
endif()

string(FIND "${server_cmake}" "# server-context containing the core server logic" runtime_marker)
if (runtime_marker EQUAL -1)
    message(FATAL_ERROR "Cannot locate production server-context CMake section")
endif()
string(SUBSTRING "${server_cmake}" ${runtime_marker} -1 runtime_cmake)
string(FIND "${runtime_cmake}" "halofpx-context-store-exact-session" runtime_link)
if (HALOFPX_CONTEXT_STORE_EXACT_KEY_CANARY)
    if (runtime_link EQUAL -1)
        message(FATAL_ERROR "L10c opt-in lost its exact-session runtime link")
    endif()
else()
    if (NOT runtime_link EQUAL -1)
        # The name may occur inside the guarded L10c block. Require that it is
        # controlled by the exact-key gate rather than rejecting source text.
        string(FIND "${runtime_cmake}" "if (HALOFPX_CONTEXT_STORE_EXACT_KEY_CANARY)" gate)
        if (gate EQUAL -1 OR runtime_link LESS gate)
            message(FATAL_ERROR "exact-session resolver has an unguarded production link")
        endif()
    endif()
endif()

message(STATUS "HaloFPX exact-session runtime edge remains separately gated")
