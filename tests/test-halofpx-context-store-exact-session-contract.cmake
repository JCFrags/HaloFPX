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
if (NOT runtime_link EQUAL -1)
    message(FATAL_ERROR "L10b exact-session resolver entered the production CMake graph")
endif()

foreach(runtime_source
        "${HALOFPX_SOURCE_DIR}/tools/server/server-context.cpp"
        "${HALOFPX_SOURCE_DIR}/tools/server/server.cpp"
        "${HALOFPX_SOURCE_DIR}/common/arg.cpp")
    file(READ "${runtime_source}" runtime_text)
    string(FIND "${runtime_text}" "context_store_resolve_exact_session_v1" runtime_hook)
    if (NOT runtime_hook EQUAL -1)
        message(FATAL_ERROR "L10b exact-session resolver entered runtime source: ${runtime_source}")
    endif()
endforeach()

message(STATUS "HaloFPX L10b exact-session resolver remains product-excluded")
