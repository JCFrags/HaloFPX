file(READ "${SOURCE_ROOT}/tools/server/CMakeLists.txt" SERVER_CMAKE)
file(READ "${SOURCE_ROOT}/tools/server/halofpx-context-store-anchor.cpp" ANCHOR_SOURCE)
file(READ "${SOURCE_ROOT}/tools/server/halofpx-context-store-anchor.h" ANCHOR_HEADER)

if(NOT SERVER_CMAKE MATCHES "halofpx-context-store-anchor STATIC EXCLUDE_FROM_ALL")
    message(FATAL_ERROR "anchor codec must remain EXCLUDE_FROM_ALL")
endif()
if(SERVER_CMAKE MATCHES "server-context[^\n]*halofpx-context-store-anchor" OR
   SERVER_CMAKE MATCHES "target_link_libraries\\(\\$\\{TARGET\\}[^\\)]*halofpx-context-store-anchor")
    message(FATAL_ERROR "anchor codec must not enter the server runtime")
endif()
set(ANCHOR_ALL "${ANCHOR_SOURCE}\n${ANCHOR_HEADER}")
foreach(FORBIDDEN_PATTERN
        "#[ \t]*include[ \t]*[<\"]filesystem"
        "#[ \t]*include[ \t]*[<\"]fstream"
        "#[ \t]*include[ \t]*\"halofpx-context-store.h\""
        "std::filesystem"
        "fopen[ \t]*\\("
        "CreateFile[A-Z]*[ \t]*\\("
        "openat[0-9]*[ \t]*\\("
        "rename[at0-9_]*[ \t]*\\("
        "context_store_candidate"
        "context_store_provider")
    if(ANCHOR_ALL MATCHES "${FORBIDDEN_PATTERN}")
        message(FATAL_ERROR "forbidden runtime or filesystem dependency in offline anchor codec: ${FORBIDDEN_PATTERN}")
    endif()
endforeach()
