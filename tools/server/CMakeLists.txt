include_directories(${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_BINARY_DIR})

# Offline-only L04 manifest parser. This target is deliberately not linked into
# server-context or llama-server.
add_library(halofpx-context-store-format STATIC EXCLUDE_FROM_ALL
    halofpx-context-store-format.cpp
    halofpx-context-store-format.h
)
target_compile_features(halofpx-context-store-format PRIVATE cxx_std_17)

# Shared offline-only selected-base public-domain SHA-256 primitive.
add_library(halofpx-context-store-sha256 STATIC EXCLUDE_FROM_ALL
    ${CMAKE_SOURCE_DIR}/examples/gguf-hash/deps/sha256/sha256.c
)
target_include_directories(halofpx-context-store-sha256 PUBLIC
    ${CMAKE_SOURCE_DIR}/examples/gguf-hash/deps)

# Offline-only L04 authentication/compatibility verifier. HMAC and policy are
# target-owned; the SHA primitive remains a separate exact source-locked target.
add_library(halofpx-context-store-auth STATIC EXCLUDE_FROM_ALL
    halofpx-context-store-auth.cpp
    halofpx-context-store-auth.h
)
target_link_libraries(halofpx-context-store-auth
    PUBLIC halofpx-context-store-format
    PRIVATE halofpx-context-store-sha256)
target_compile_features(halofpx-context-store-auth PRIVATE cxx_std_17)

# Offline-only L05e authenticated protected-anchor codec. It performs no I/O
# and is not linked into server-context, the publication coordinator, or server.
add_library(halofpx-context-store-anchor STATIC EXCLUDE_FROM_ALL
    halofpx-context-store-anchor.cpp
    halofpx-context-store-anchor.h
)
target_link_libraries(halofpx-context-store-anchor PUBLIC halofpx-context-store-auth)
target_compile_features(halofpx-context-store-anchor PRIVATE cxx_std_17)

# Offline-only L05g memory-resident bootstrap authority planner. It owns its
# bounded key snapshot, performs no I/O, and is not linked into the server.
add_library(halofpx-context-store-bootstrap-token STATIC EXCLUDE_FROM_ALL
    halofpx-context-store-bootstrap-token.cpp
    halofpx-context-store-bootstrap-token.h
)
target_link_libraries(halofpx-context-store-bootstrap-token PUBLIC halofpx-context-store-auth)
target_compile_features(halofpx-context-store-bootstrap-token PRIVATE cxx_std_17)

add_library(halofpx-context-store-protected-registry STATIC EXCLUDE_FROM_ALL
    halofpx-context-store-protected-registry.cpp
    halofpx-context-store-protected-registry.h
)
target_link_libraries(halofpx-context-store-protected-registry PUBLIC halofpx-context-store-auth)
target_compile_features(halofpx-context-store-protected-registry PRIVATE cxx_std_17)

add_library(halofpx-context-store-authority STATIC EXCLUDE_FROM_ALL
    halofpx-context-store-authority.cpp
    halofpx-context-store-authority.h
)
target_link_libraries(halofpx-context-store-authority PUBLIC
    halofpx-context-store-anchor halofpx-context-store-bootstrap-token
    halofpx-context-store-protected-registry)
target_compile_features(halofpx-context-store-authority PRIVATE cxx_std_17)

add_library(halofpx-context-store-registry-successor STATIC EXCLUDE_FROM_ALL
    halofpx-context-store-protected-registry-successor.cpp
    halofpx-context-store-protected-registry-successor.h
)
target_link_libraries(halofpx-context-store-registry-successor PUBLIC
    halofpx-context-store-protected-registry)
target_compile_features(halofpx-context-store-registry-successor PRIVATE cxx_std_17)

add_library(halofpx-context-store-bootstrap-consumption STATIC EXCLUDE_FROM_ALL
    halofpx-context-store-bootstrap-consumption.cpp
    halofpx-context-store-bootstrap-consumption.h
)
target_link_libraries(halofpx-context-store-bootstrap-consumption PUBLIC
    halofpx-context-store-authority halofpx-context-store-registry-successor)
target_compile_features(halofpx-context-store-bootstrap-consumption PRIVATE cxx_std_17)

# Offline-only L04 immutable object-frame verifier.
add_library(halofpx-context-store-object STATIC EXCLUDE_FROM_ALL
    halofpx-context-store-object.cpp
    halofpx-context-store-object.h
)
target_link_libraries(halofpx-context-store-object PUBLIC halofpx-context-store-auth)
target_compile_features(halofpx-context-store-object PRIVATE cxx_std_17)

# Permanently synthetic L05m bootstrap-material preparation seam. This target
# owns no filesystem or anchor-storage implementation and remains excluded.
add_library(halofpx-context-store-bootstrap-material STATIC EXCLUDE_FROM_ALL
    halofpx-context-store-bootstrap-material.cpp
    halofpx-context-store-bootstrap-material.h
)
target_link_libraries(halofpx-context-store-bootstrap-material PUBLIC
    halofpx-context-store-bootstrap-consumption halofpx-context-store-object)
target_compile_features(halofpx-context-store-bootstrap-material PRIVATE cxx_std_17)

# Permanently synthetic L05n protected-anchor bootstrap create/reconciliation
# model. It owns no concrete storage and remains excluded from every product.
add_library(halofpx-context-store-bootstrap-anchor STATIC EXCLUDE_FROM_ALL
    halofpx-context-store-bootstrap-anchor.cpp
    halofpx-context-store-bootstrap-anchor.h
)
target_link_libraries(halofpx-context-store-bootstrap-anchor PUBLIC
    halofpx-context-store-bootstrap-material)
target_compile_features(halofpx-context-store-bootstrap-anchor PRIVATE cxx_std_17)

# L05o target-native deterministic-CBOR and semantic validation slice. It has
# no filesystem implementation and is excluded from every server/product target.
add_library(halofpx-context-store-registry-lab-wire STATIC EXCLUDE_FROM_ALL
    halofpx-context-store-registry-lab-wire.cpp
    halofpx-context-store-registry-lab-wire.h
)
target_link_libraries(halofpx-context-store-registry-lab-wire PUBLIC
    halofpx-context-store-registry-successor)
target_compile_features(halofpx-context-store-registry-lab-wire PRIVATE cxx_std_17)
set(HALOFPX_REGISTRY_LAB_WIRE_TARGET halofpx-context-store-registry-lab-wire)

# Internal-only L05p portable fake-engine prefix through operations 1-4. It has
# no public header, decoder, mutation path, filesystem adapter, or product edge.
add_library(halofpx-context-store-registry-lab-read-only STATIC EXCLUDE_FROM_ALL
    halofpx-context-store-registry-lab-read-only.cpp
    halofpx-context-store-registry-lab-read-only-internal.h
)
target_link_libraries(halofpx-context-store-registry-lab-read-only PRIVATE
    halofpx-context-store-registry-lab-wire)
target_compile_features(halofpx-context-store-registry-lab-read-only PRIVATE cxx_std_17)
set(HALOFPX_REGISTRY_LAB_READ_ONLY_TARGET
    halofpx-context-store-registry-lab-read-only)

# Offline-only L05a publication coordinator and injected fault seam. No
# concrete filesystem backend exists and this target is not linked to server.
add_library(halofpx-context-store-publication STATIC EXCLUDE_FROM_ALL
    halofpx-context-store-publication.cpp
    halofpx-context-store-publication.h
    halofpx-context-store-publication-simulator.cpp
    halofpx-context-store-publication-simulator.h
)
target_link_libraries(halofpx-context-store-publication PUBLIC halofpx-context-store-anchor)
target_compile_features(halofpx-context-store-publication PRIVATE cxx_std_17)

# Build-only L05o static-qualification inputs. These targets are deliberately
# excluded from ALL and have no product/server edge.
if (BUILD_TESTING AND MSVC)
    add_custom_target(halofpx-registry-lab-static-audit-inputs
        DEPENDS
            ${HALOFPX_REGISTRY_LAB_WIRE_TARGET}
            halofpx-context-store-registry-successor
            halofpx-context-store-protected-registry
            halofpx-context-store-auth
            halofpx-context-store-format
            halofpx-context-store-sha256)

    add_custom_target(halofpx-registry-lab-read-only-static-audit-inputs
        DEPENDS
            ${HALOFPX_REGISTRY_LAB_READ_ONLY_TARGET}
            ${HALOFPX_REGISTRY_LAB_WIRE_TARGET}
            halofpx-context-store-registry-successor
            halofpx-context-store-protected-registry
            halofpx-context-store-auth
            halofpx-context-store-format
            halofpx-context-store-sha256)

    add_executable(halofpx-registry-lab-negative-construct EXCLUDE_FROM_ALL
        ${CMAKE_SOURCE_DIR}/tests/halofpx-registry-lab-negative-construct.cpp)
    target_include_directories(halofpx-registry-lab-negative-construct PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR})
    target_compile_features(halofpx-registry-lab-negative-construct PRIVATE cxx_std_17)

    add_executable(halofpx-registry-lab-negative-link EXCLUDE_FROM_ALL
        ${CMAKE_SOURCE_DIR}/tests/halofpx-registry-lab-negative-link.cpp)
    target_link_libraries(halofpx-registry-lab-negative-link PRIVATE
        ${HALOFPX_REGISTRY_LAB_WIRE_TARGET})
    target_compile_features(halofpx-registry-lab-negative-link PRIVATE cxx_std_17)

    add_executable(halofpx-registry-lab-positive-probe EXCLUDE_FROM_ALL
        ${CMAKE_SOURCE_DIR}/tests/halofpx-registry-lab-positive-probe.cpp)
    target_compile_features(halofpx-registry-lab-positive-probe PRIVATE cxx_std_17)

    get_filename_component(HALOFPX_MSVC_BIN_DIR "${CMAKE_LINKER}" DIRECTORY)
    find_program(HALOFPX_DUMPBIN_EXECUTABLE dumpbin.exe
        HINTS "${HALOFPX_MSVC_BIN_DIR}" NO_DEFAULT_PATH REQUIRED)
    find_program(HALOFPX_LIB_EXECUTABLE lib.exe
        HINTS "${HALOFPX_MSVC_BIN_DIR}" NO_DEFAULT_PATH REQUIRED)

    set(HALOFPX_REGISTRY_LAB_GRAPH_MANIFEST
        "${CMAKE_BINARY_DIR}/halofpx-registry-lab-link-graph.txt")
    file(GENERATE
        OUTPUT "${CMAKE_BINARY_DIR}/halofpx-registry-lab-archives-$<CONFIG>.cmake"
        CONTENT
"set(HALOFPX_REGISTRY_LAB_ARCHIVES
  \"$<TARGET_FILE:${HALOFPX_REGISTRY_LAB_WIRE_TARGET}>\"
  \"$<TARGET_FILE:halofpx-context-store-registry-successor>\"
  \"$<TARGET_FILE:halofpx-context-store-protected-registry>\"
  \"$<TARGET_FILE:halofpx-context-store-auth>\"
  \"$<TARGET_FILE:halofpx-context-store-format>\"
  \"$<TARGET_FILE:halofpx-context-store-sha256>\")
")
    file(GENERATE
        OUTPUT "${CMAKE_BINARY_DIR}/halofpx-registry-lab-read-only-archives-$<CONFIG>.cmake"
        CONTENT
"set(HALOFPX_REGISTRY_LAB_READ_ONLY_ARCHIVES
  \"$<TARGET_FILE:${HALOFPX_REGISTRY_LAB_READ_ONLY_TARGET}>\"
  \"$<TARGET_FILE:${HALOFPX_REGISTRY_LAB_WIRE_TARGET}>\"
  \"$<TARGET_FILE:halofpx-context-store-registry-successor>\"
  \"$<TARGET_FILE:halofpx-context-store-protected-registry>\"
  \"$<TARGET_FILE:halofpx-context-store-auth>\"
  \"$<TARGET_FILE:halofpx-context-store-format>\"
  \"$<TARGET_FILE:halofpx-context-store-sha256>\")
")

    add_test(NAME test-halofpx-context-store-registry-lab-static-audit
        COMMAND ${CMAKE_COMMAND}
            -DBUILD_ROOT=${CMAKE_BINARY_DIR}
            -DCONFIG=$<CONFIG>
            -DGRAPH_MANIFEST=${HALOFPX_REGISTRY_LAB_GRAPH_MANIFEST}
            -DARCHIVE_MANIFEST=${CMAKE_BINARY_DIR}/halofpx-registry-lab-archives-$<CONFIG>.cmake
            -DREAD_ONLY_ARCHIVE_MANIFEST=${CMAKE_BINARY_DIR}/halofpx-registry-lab-read-only-archives-$<CONFIG>.cmake
            -DDUMPBIN=${HALOFPX_DUMPBIN_EXECUTABLE}
            -DLIB_TOOL=${HALOFPX_LIB_EXECUTABLE}
            -P ${CMAKE_SOURCE_DIR}/tests/halofpx-registry-lab-static-audit.cmake)
    set_tests_properties(test-halofpx-context-store-registry-lab-static-audit PROPERTIES
        LABELS "main;halofpx" RUN_SERIAL TRUE)

    add_test(NAME test-halofpx-context-store-registry-lab-negative-probes
        COMMAND ${CMAKE_COMMAND}
            -DBUILD_ROOT=${CMAKE_BINARY_DIR}
            -DCONFIG=$<CONFIG>
            -P ${CMAKE_SOURCE_DIR}/tests/halofpx-registry-lab-negative-probes.cmake)
    set_tests_properties(test-halofpx-context-store-registry-lab-negative-probes PROPERTIES
        LABELS "main;halofpx" RUN_SERIAL TRUE)
endif()

# The Linux object/archive audit is independently runnable on the disposable
# qualification nodes. It makes no claim about Windows COFF or concrete I/O.
if (BUILD_TESTING AND CMAKE_SYSTEM_NAME STREQUAL "Linux")
    add_custom_target(halofpx-registry-lab-linux-audit-inputs
        DEPENDS
            ${HALOFPX_REGISTRY_LAB_WIRE_TARGET}
            halofpx-context-store-registry-successor
            halofpx-context-store-protected-registry
            halofpx-context-store-auth
            halofpx-context-store-format
            halofpx-context-store-sha256)
    add_custom_target(halofpx-registry-lab-read-only-linux-audit-inputs
        DEPENDS
            ${HALOFPX_REGISTRY_LAB_READ_ONLY_TARGET}
            ${HALOFPX_REGISTRY_LAB_WIRE_TARGET}
            halofpx-context-store-registry-successor
            halofpx-context-store-protected-registry
            halofpx-context-store-auth
            halofpx-context-store-format
            halofpx-context-store-sha256)
    find_program(HALOFPX_NM_EXECUTABLE NAMES llvm-nm nm)
    find_program(HALOFPX_AR_EXECUTABLE NAMES llvm-ar ar)
    find_program(HALOFPX_READELF_EXECUTABLE NAMES llvm-readelf readelf)
    foreach(HALOFPX_LINUX_TOOL IN ITEMS
            HALOFPX_NM_EXECUTABLE HALOFPX_AR_EXECUTABLE HALOFPX_READELF_EXECUTABLE)
        if(NOT ${HALOFPX_LINUX_TOOL})
            message(FATAL_ERROR "Linux registry-lab audit requires ${HALOFPX_LINUX_TOOL}")
        endif()
    endforeach()
    file(GENERATE
        OUTPUT "${CMAKE_BINARY_DIR}/halofpx-registry-lab-linux-archives-$<CONFIG>.cmake"
        CONTENT
"set(HALOFPX_REGISTRY_LAB_ARCHIVES
  \"$<TARGET_FILE:${HALOFPX_REGISTRY_LAB_WIRE_TARGET}>\"
  \"$<TARGET_FILE:halofpx-context-store-registry-successor>\"
  \"$<TARGET_FILE:halofpx-context-store-protected-registry>\"
  \"$<TARGET_FILE:halofpx-context-store-auth>\"
  \"$<TARGET_FILE:halofpx-context-store-format>\"
  \"$<TARGET_FILE:halofpx-context-store-sha256>\")
")
    file(GENERATE
        OUTPUT "${CMAKE_BINARY_DIR}/halofpx-registry-lab-read-only-linux-archives-$<CONFIG>.cmake"
        CONTENT
"set(HALOFPX_REGISTRY_LAB_READ_ONLY_ARCHIVES
  \"$<TARGET_FILE:${HALOFPX_REGISTRY_LAB_READ_ONLY_TARGET}>\"
  \"$<TARGET_FILE:${HALOFPX_REGISTRY_LAB_WIRE_TARGET}>\"
  \"$<TARGET_FILE:halofpx-context-store-registry-successor>\"
  \"$<TARGET_FILE:halofpx-context-store-protected-registry>\"
  \"$<TARGET_FILE:halofpx-context-store-auth>\"
  \"$<TARGET_FILE:halofpx-context-store-format>\"
  \"$<TARGET_FILE:halofpx-context-store-sha256>\")
")
    add_test(NAME test-halofpx-context-store-registry-lab-linux-archive-audit
        COMMAND ${CMAKE_COMMAND}
            -DBUILD_ROOT=${CMAKE_BINARY_DIR}
            -DCONFIG=$<CONFIG>
            -DARCHIVE_MANIFEST=${CMAKE_BINARY_DIR}/halofpx-registry-lab-linux-archives-$<CONFIG>.cmake
            -DREAD_ONLY_ARCHIVE_MANIFEST=${CMAKE_BINARY_DIR}/halofpx-registry-lab-read-only-linux-archives-$<CONFIG>.cmake
            -DNM_TOOL=${HALOFPX_NM_EXECUTABLE}
            -DAR_TOOL=${HALOFPX_AR_EXECUTABLE}
            -DREADELF_TOOL=${HALOFPX_READELF_EXECUTABLE}
            -P ${CMAKE_SOURCE_DIR}/tests/halofpx-registry-lab-linux-archive-audit.cmake)
    set_tests_properties(test-halofpx-context-store-registry-lab-linux-archive-audit PROPERTIES
        LABELS "main;halofpx;linux" RUN_SERIAL TRUE)
endif()

# server-context containing the core server logic, used by llama-server and CLI

set(TARGET server-context)

add_library(${TARGET} STATIC
    ${CMAKE_SOURCE_DIR}/examples/diffusion/diffusion.cpp
    halofpx-context-store.cpp
    halofpx-context-store.h
    server-chat.cpp
    server-chat.h
    server-task.cpp
    server-task.h
    server-queue.cpp
    server-queue.h
    server-common.cpp
    server-common.h
    server-context.cpp
    server-context.h
    server-tools.cpp
    server-tools.h
)

if (BUILD_SHARED_LIBS)
    set_target_properties(${TARGET} PROPERTIES POSITION_INDEPENDENT_CODE ON)
endif()

target_include_directories(${TARGET} PRIVATE ../mtmd)
target_include_directories(${TARGET} PRIVATE ${CMAKE_SOURCE_DIR})
target_include_directories(${TARGET} PRIVATE ${CMAKE_SOURCE_DIR}/examples/diffusion)
target_link_libraries(${TARGET} PUBLIC llama-common mtmd ${CMAKE_THREAD_LIBS_INIT})


# llama-server executable

set(TARGET llama-server)

set(TARGET_SRCS
    server.cpp
    server-http.cpp
    server-http.h
    server-models.cpp
    server-models.h
)

# Option to specify custom HF bucket for webui (defaults to llama-ui)
# Usage: cmake -B build -DLLAMA_WEBUI_HF_BUCKET=llama-ui
set(LLAMA_WEBUI_HF_BUCKET "llama-ui" CACHE STRING "Hugging Face bucket name for prebuilt webui assets")

if (LLAMA_BUILD_WEBUI)
    set(PUBLIC_ASSETS
        index.html
        bundle.js
        bundle.css
        loading.html
    )

    # Determine source of webui assets (priority: local > HF Bucket)
    set(WEBUI_SOURCE "")
    set(WEBUI_SOURCE_DIR "")

    # Priority 1: Check for local webui build output
    set(LOCAL_WEBUI_DIR "${CMAKE_CURRENT_SOURCE_DIR}/public")

    # Verify all required assets exist before declaring local source valid
    set(ALL_ASSETS_PRESENT TRUE)
    foreach(asset ${PUBLIC_ASSETS})
        if(NOT EXISTS "${LOCAL_WEBUI_DIR}/${asset}")
            set(ALL_ASSETS_PRESENT FALSE)
            break()
        endif()
    endforeach()

    if(ALL_ASSETS_PRESENT)
        set(WEBUI_SOURCE "local")
        set(WEBUI_SOURCE_DIR "${LOCAL_WEBUI_DIR}")
        message(STATUS "WebUI: using local build from ${WEBUI_SOURCE_DIR}")
    endif()

    # Priority 2: Build-time asset provisioning (npm build → HF Bucket fallback)
    if(NOT WEBUI_SOURCE_DIR)
        # Environment variable takes precedence (e.g., from CI workflows)
        if(DEFINED ENV{HF_WEBUI_VERSION} AND NOT "$ENV{HF_WEBUI_VERSION}" STREQUAL "")
            set(HF_WEBUI_VERSION "$ENV{HF_WEBUI_VERSION}")
            # Validate against allowed characters to prevent CMake list separator
            # or path-traversal issues in stamp filenames and download URLs
            if(NOT HF_WEBUI_VERSION MATCHES "^[A-Za-z0-9._-]+$")
                message(FATAL_ERROR "WebUI: invalid HF_WEBUI_VERSION='${HF_WEBUI_VERSION}' - must match ^[A-Za-z0-9._-]+$")
            endif()
            message(STATUS "WebUI: using HF_WEBUI_VERSION from environment=${HF_WEBUI_VERSION}")
        elseif(DEFINED LLAMA_BUILD_NUMBER)
            set(HF_WEBUI_VERSION "b${LLAMA_BUILD_NUMBER}")
            message(STATUS "WebUI: using LLAMA_BUILD_NUMBER=${HF_WEBUI_VERSION}")
        else()
            set(HF_WEBUI_VERSION "")
            message(STATUS "WebUI: version not specified (will resolve from git before download)")
        endif()

        # Stamp file embeds the version tag so a changed build number triggers
        # a fresh provision run on the next `cmake --build` without reconfiguring.
        if("${HF_WEBUI_VERSION}" STREQUAL "")
            set(WEBUI_VERSION_TAG "provisioned")
        else()
            set(WEBUI_VERSION_TAG "${HF_WEBUI_VERSION}")
        endif()
        set(WEBUI_STAMP "${CMAKE_CURRENT_BINARY_DIR}/.webui-${WEBUI_VERSION_TAG}.stamp")

        # Join assets with + separator (safe across all platforms, unlike ; and |)
        string(REPLACE ";" "+" PUBLIC_ASSETS_JOINED "${PUBLIC_ASSETS}")

        add_custom_command(
            OUTPUT ${WEBUI_STAMP}
            COMMAND ${CMAKE_COMMAND}
                "-DSOURCE_DIR=${PROJECT_SOURCE_DIR}"
                "-DPUBLIC_DIR=${CMAKE_CURRENT_SOURCE_DIR}/public"
                "-DHF_BUCKET=${LLAMA_WEBUI_HF_BUCKET}"
                "-DHF_VERSION=${HF_WEBUI_VERSION}"
                "-DHF_ENABLED=${LLAMA_USE_PREBUILT_WEBUI}"
                "-DASSETS=${PUBLIC_ASSETS_JOINED}"
                "-DSTAMP_FILE=${WEBUI_STAMP}"
                "-DNPM_DIR=${CMAKE_CURRENT_SOURCE_DIR}/webui"
                -P ${PROJECT_SOURCE_DIR}/scripts/webui-download.cmake
            COMMENT "Building/provisioning WebUI assets (npm build -> HF Bucket fallback)"
        )

        set(WEBUI_SOURCE "provisioned")
        set(WEBUI_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/public")
    endif()

    # Process assets from the determined source
    if(WEBUI_SOURCE_DIR)
        foreach(asset ${PUBLIC_ASSETS})
            set(input "${WEBUI_SOURCE_DIR}/${asset}")
            set(output "${CMAKE_CURRENT_BINARY_DIR}/${asset}.hpp")
            list(APPEND TARGET_SRCS ${output})

            if(WEBUI_SOURCE STREQUAL "local")
                # Local build: files exist at configure time
                if(NOT EXISTS "${input}")
                    message(FATAL_ERROR "WebUI asset not found: ${input}")
                endif()
                set(dependency "${input}")
            else()
                # HF Bucket: files are downloaded at build time
                set(dependency "${WEBUI_STAMP}")
            endif()

            add_custom_command(
                DEPENDS ${dependency}
                OUTPUT "${output}"
                COMMAND "${CMAKE_COMMAND}" "-DINPUT=${input}" "-DOUTPUT=${output}" -P "${PROJECT_SOURCE_DIR}/scripts/xxd.cmake"
            )
            set_source_files_properties(${output} PROPERTIES GENERATED TRUE)
        endforeach()

        add_definitions(-DLLAMA_BUILD_WEBUI)
        add_definitions(-DLLAMA_WEBUI_DEFAULT_ENABLED=1)
        message(STATUS "WebUI: embedded with source: ${WEBUI_SOURCE}")
    else()
        # WebUI source not found - issue warning but don't fail the build
        # The server will still build but without webui embedded
        message(WARNING "WebUI: no source available. Neither local build (tools/server/public/) nor HF Bucket download succeeded.")
        message(WARNING "WebUI: building server without embedded WebUI. Set LLAMA_BUILD_WEBUI=OFF to suppress this warning.")
        add_definitions(-DLLAMA_WEBUI_DEFAULT_ENABLED=0)
    endif()
else()
    # WebUI is disabled at build time
    add_definitions(-DLLAMA_WEBUI_DEFAULT_ENABLED=0)
endif()

add_executable(${TARGET} ${TARGET_SRCS})
install(TARGETS ${TARGET} RUNTIME)

target_include_directories(${TARGET} PRIVATE ../mtmd)
target_include_directories(${TARGET} PRIVATE ${CMAKE_SOURCE_DIR})
target_link_libraries(${TARGET} PRIVATE server-context PUBLIC llama-common cpp-httplib ${CMAKE_THREAD_LIBS_INIT})

target_compile_features(${TARGET} PRIVATE cxx_std_17)
