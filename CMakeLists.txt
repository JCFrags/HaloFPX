cmake_minimum_required(VERSION 3.14...3.28) # for add_link_options and implicit target directories.
project("llama.cpp" C CXX)

option(HALOFPX_RELOCATABLE_STAGED_RUNTIME
    "Use origin-relative build lookup for closed staged HaloFPX runtimes" OFF)
if (HALOFPX_RELOCATABLE_STAGED_RUNTIME)
    set(CMAKE_BUILD_WITH_INSTALL_RPATH ON)
    set(CMAKE_INSTALL_RPATH "$ORIGIN;/opt/rocm/lib")
endif()

set(HALOFPX_PROVENANCE_SOURCE_ROOT "" CACHE STRING "Canonical HaloFPX source root")
set(HALOFPX_PROVENANCE_BUILD_ID "" CACHE STRING "Immutable HaloFPX diagnostic build identity")
if (HALOFPX_PROVENANCE_SOURCE_ROOT OR HALOFPX_PROVENANCE_BUILD_ID)
    string(LENGTH "${HALOFPX_PROVENANCE_SOURCE_ROOT}" HALOFPX_PROVENANCE_SOURCE_ROOT_LENGTH)
    string(LENGTH "${HALOFPX_PROVENANCE_BUILD_ID}" HALOFPX_PROVENANCE_BUILD_ID_LENGTH)
    if (NOT HALOFPX_PROVENANCE_SOURCE_ROOT_LENGTH EQUAL 64 OR
        NOT HALOFPX_PROVENANCE_BUILD_ID_LENGTH EQUAL 64 OR
        NOT HALOFPX_PROVENANCE_SOURCE_ROOT MATCHES "^[0-9a-f]+$" OR
        NOT HALOFPX_PROVENANCE_BUILD_ID MATCHES "^[0-9a-f]+$")
        message(FATAL_ERROR "HaloFPX provenance values must be exact lowercase SHA-256")
    endif()
    add_compile_definitions(
        HALOFPX_PROVENANCE_SOURCE_ROOT="${HALOFPX_PROVENANCE_SOURCE_ROOT}"
        HALOFPX_PROVENANCE_BUILD_ID="${HALOFPX_PROVENANCE_BUILD_ID}"
        HALOFPX_PROVENANCE_SCHEMA="halofpx.l57.binary-provenance.v1")
endif()
include(CheckIncludeFileCXX)

#set(CMAKE_WARN_DEPRECATED YES)
set(CMAKE_WARN_UNUSED_CLI YES)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if (NOT XCODE AND NOT MSVC AND NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
    set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS "Debug" "Release" "MinSizeRel" "RelWithDebInfo")
endif()

message("CMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}")

# Add path to modules
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake/")

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

if (CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
    set(LLAMA_STANDALONE ON)

    include(git-vars)

    # configure project version
    # TODO
else()
    set(LLAMA_STANDALONE OFF)
endif()

option(LLAMA_USE_SYSTEM_GGML "Use system libggml" OFF)

option(LLAMA_WASM_MEM64 "llama: use 64-bit memory in WASM builds" ON)

if (EMSCRIPTEN)
    set(BUILD_SHARED_LIBS_DEFAULT OFF)

    # Use 64-bit memory to support backend_get_memory queries
    # TODO: analyze performance impact, see https://spidermonkey.dev/blog/2025/01/15/is-memory64-actually-worth-using
    if (LLAMA_WASM_MEM64)
      add_compile_options("-sMEMORY64=1")
      add_link_options("-sMEMORY64=1")
    endif()
    add_link_options("-sALLOW_MEMORY_GROWTH=1")

    option(LLAMA_WASM_SINGLE_FILE "llama: embed WASM inside the generated llama.js" OFF)
    option(LLAMA_BUILD_HTML "llama: build HTML file" ON)
    if (LLAMA_BUILD_HTML)
        set(CMAKE_EXECUTABLE_SUFFIX ".html")
    endif()
else()
    if (MINGW)
        set(BUILD_SHARED_LIBS_DEFAULT OFF)
    else()
        set(BUILD_SHARED_LIBS_DEFAULT ON)
    endif()
endif()

option(BUILD_SHARED_LIBS "build shared libraries" ${BUILD_SHARED_LIBS_DEFAULT})

if (WIN32)
    add_compile_definitions(_CRT_SECURE_NO_WARNINGS)
endif()

if (MSVC)
    add_compile_options("$<$<COMPILE_LANGUAGE:C>:/utf-8>")
    add_compile_options("$<$<COMPILE_LANGUAGE:CXX>:/utf-8>")
    add_compile_options("$<$<COMPILE_LANGUAGE:C>:/bigobj>")
    add_compile_options("$<$<COMPILE_LANGUAGE:CXX>:/bigobj>")
endif()

if (LLAMA_STANDALONE)
    # enable parallel builds for msbuild
    list(APPEND CMAKE_VS_GLOBALS UseMultiToolTask=true)
    list(APPEND CMAKE_VS_GLOBALS EnforceProcessCountAcrossBuilds=true)
endif()

if (CMAKE_SYSTEM_NAME STREQUAL "iOS")
    set(LLAMA_TOOLS_INSTALL_DEFAULT OFF)
else()
    set(LLAMA_TOOLS_INSTALL_DEFAULT ${LLAMA_STANDALONE})
endif()

#
# option list
#

# debug
option(LLAMA_ALL_WARNINGS           "llama: enable all compiler warnings"                   ON)
option(LLAMA_ALL_WARNINGS_3RD_PARTY "llama: enable all compiler warnings in 3rd party libs" OFF)

# build
option(LLAMA_FATAL_WARNINGS "llama: enable -Werror flag" OFF)

# sanitizers
option(LLAMA_SANITIZE_THREAD    "llama: enable thread sanitizer"    OFF)
option(LLAMA_SANITIZE_ADDRESS   "llama: enable address sanitizer"   OFF)
option(LLAMA_SANITIZE_UNDEFINED "llama: enable undefined sanitizer" OFF)

# utils
option(LLAMA_BUILD_COMMON "llama: build common utils library" ${LLAMA_STANDALONE})

# extra artifacts
option(LLAMA_BUILD_TESTS            "llama: build tests"                                                                            ${LLAMA_STANDALONE})
option(LLAMA_BUILD_TOOLS            "llama: build tools"                                                                            ${LLAMA_STANDALONE})
option(LLAMA_BUILD_EXAMPLES         "llama: build examples"                                                                         ${LLAMA_STANDALONE})
option(LLAMA_BUILD_SERVER           "llama: build server example"                                                                   ${LLAMA_STANDALONE})
option(LLAMA_BUILD_WEBUI            "llama: build the embedded Web UI for server"                                                   ON)
option(LLAMA_USE_PREBUILT_WEBUI     "llama: use prebuilt WebUI from HF Bucket when available (requires LLAMA_BUILD_WEBUI=ON)"       ON)
option(LLAMA_TOOLS_INSTALL          "llama: install tools"                                                                          ${LLAMA_TOOLS_INSTALL_DEFAULT})
option(LLAMA_TESTS_INSTALL          "llama: install tests"                                                                          ON)

# Default-off HaloFPX experiment. When enabled, completed host-output
# synchronizations may be reused by internal output-result readers until the
# next graph submission invalidates the output epoch.
option(HALOFPX_SAMPLING_SYNC_COALESCE_CANARY
       "Coalesce redundant completed output synchronizations during sampling"
       OFF)

# L10e is a standalone, non-product Linux selector seam. Declare and enforce
# its build graph before tests are processed: tests precede tools/server in the
# root subdirectory order, while the implementation target lives there.
option(HALOFPX_CONTEXT_STORE_LONGEST_PREFIX_CANARY
       "Compile the non-product exact longest-prefix catalog selector canary"
       OFF)
if (HALOFPX_CONTEXT_STORE_LONGEST_PREFIX_CANARY AND
    NOT HALOFPX_CONTEXT_STORE_EXACT_KEY_CATALOG_CANARY)
    message(FATAL_ERROR
        "HALOFPX_CONTEXT_STORE_LONGEST_PREFIX_CANARY requires the exact-key catalog canary gate")
endif()
if (HALOFPX_CONTEXT_STORE_LONGEST_PREFIX_CANARY AND
    (NOT LLAMA_BUILD_COMMON OR NOT LLAMA_BUILD_TOOLS OR NOT LLAMA_BUILD_SERVER))
    message(FATAL_ERROR
        "HALOFPX_CONTEXT_STORE_LONGEST_PREFIX_CANARY requires LLAMA_BUILD_COMMON, LLAMA_BUILD_TOOLS, and LLAMA_BUILD_SERVER")
endif()
if (HALOFPX_CONTEXT_STORE_LONGEST_PREFIX_CANARY AND
    NOT CMAKE_SYSTEM_NAME STREQUAL "Linux")
    message(FATAL_ERROR
        "HALOFPX_CONTEXT_STORE_LONGEST_PREFIX_CANARY is supported only on Linux")
endif()

# Product routing is a separate literal-default-OFF gate.  It composes the
# authenticated selector but remains cold until the server owns a typed live
# world1 loader authority.
option(HALOFPX_CONTEXT_STORE_WORLD1_PREFIX_PRODUCT
       "Compile the fail-closed world1 authenticated prefix product path"
       OFF)
if (HALOFPX_CONTEXT_STORE_WORLD1_PREFIX_PRODUCT AND
    NOT HALOFPX_CONTEXT_STORE_LONGEST_PREFIX_CANARY)
    message(FATAL_ERROR
        "HALOFPX_CONTEXT_STORE_WORLD1_PREFIX_PRODUCT requires the longest-prefix selector gate")
endif()
if (HALOFPX_CONTEXT_STORE_WORLD1_PREFIX_PRODUCT AND
    (NOT LLAMA_BUILD_COMMON OR NOT LLAMA_BUILD_TOOLS OR NOT LLAMA_BUILD_SERVER))
    message(FATAL_ERROR
        "HALOFPX_CONTEXT_STORE_WORLD1_PREFIX_PRODUCT requires LLAMA_BUILD_COMMON, LLAMA_BUILD_TOOLS, and LLAMA_BUILD_SERVER")
endif()
if (HALOFPX_CONTEXT_STORE_WORLD1_PREFIX_PRODUCT AND
    NOT CMAKE_SYSTEM_NAME STREQUAL "Linux")
    message(FATAL_ERROR
        "HALOFPX_CONTEXT_STORE_WORLD1_PREFIX_PRODUCT is supported only on Linux")
endif()

# P06a is a test-only expert-axis contract seam. It does not enable MiniMax
# tensor mode, alter model placement, or link into the inference runtime.
option(HALOFPX_MINIMAX_M2_EXPERT_PARTITION_CANARY
       "Build the isolated MiniMax-M2 two-rank expert-partition canary"
       OFF)
option(HALOFPX_MINIMAX_M2_Q6_PRIVATE_HIP_CANARY
       "Build the private HIP MiniMax-M2 Q6 owned-expert canary"
       OFF)
if (HALOFPX_MINIMAX_M2_Q6_PRIVATE_HIP_CANARY AND NOT GGML_HIP)
    message(FATAL_ERROR "HALOFPX_MINIMAX_M2_Q6_PRIVATE_HIP_CANARY requires GGML_HIP")
endif()

# HaloFPX L05s is a standalone Linux qualification seam. Both controls are
# deliberately off unless an isolated qualification build selects them.
option(HALOFPX_REGISTRY_LAB_LINUX_PREINIT
       "Build the Linux-only nonmutating registry-lab pre-initialization primitives"
       OFF)
option(HALOFPX_REGISTRY_LAB_LINUX_INITIALIZER
       "Build the Linux-only discard-required registry-lab initializer seam"
       OFF)
option(HALOFPX_REGISTRY_LAB_LINUX_MUTATION
       "Build Linux registry-lab mutation support (not admitted)"
       OFF)

include(cmake/halofpx-registry-lab-linux-gates.cmake)

# 3rd party libs
option(LLAMA_OPENSSL    "llama: use openssl to support HTTPS" ON)
option(LLAMA_LLGUIDANCE "llama-common: include LLGuidance library for structured output in common utils" OFF)


# Required for relocatable CMake package
include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/build-info.cmake)
include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/common.cmake)

if (NOT DEFINED LLAMA_BUILD_NUMBER)
    set(LLAMA_BUILD_NUMBER        ${BUILD_NUMBER})
endif()
if (NOT DEFINED LLAMA_BUILD_COMMIT)
    set(LLAMA_BUILD_COMMIT        ${BUILD_COMMIT})
endif()
set(LLAMA_INSTALL_VERSION 0.0.${LLAMA_BUILD_NUMBER})

# override ggml options
set(GGML_ALL_WARNINGS   ${LLAMA_ALL_WARNINGS})
set(GGML_FATAL_WARNINGS ${LLAMA_FATAL_WARNINGS})

# change the default for these ggml options
if (NOT DEFINED GGML_LLAMAFILE)
    set(GGML_LLAMAFILE_DEFAULT ON)
endif()

if (NOT DEFINED GGML_CUDA_GRAPHS)
    set(GGML_CUDA_GRAPHS_DEFAULT ON)
endif()

# transition helpers
function (llama_option_depr TYPE OLD)
    if (${OLD})
        set(NEW "${ARGV2}")
        if(NEW)
            message(${TYPE} "${OLD} is deprecated, use ${NEW} instead")
            set(${NEW} ON PARENT_SCOPE)
        else()
            message(${TYPE} "${OLD} is deprecated and will be ignored")
        endif()
    endif()
endfunction()

llama_option_depr(FATAL_ERROR LLAMA_CUBLAS              GGML_CUDA)
llama_option_depr(WARNING     LLAMA_CUDA                GGML_CUDA)
llama_option_depr(WARNING     LLAMA_METAL               GGML_METAL)
llama_option_depr(WARNING     LLAMA_METAL_EMBED_LIBRARY GGML_METAL_EMBED_LIBRARY)
llama_option_depr(WARNING     LLAMA_NATIVE              GGML_NATIVE)
llama_option_depr(WARNING     LLAMA_RPC                 GGML_RPC)
llama_option_depr(WARNING     LLAMA_SYCL                GGML_SYCL)
llama_option_depr(WARNING     LLAMA_SYCL_F16            GGML_SYCL_F16)
llama_option_depr(WARNING     LLAMA_CANN                GGML_CANN)
llama_option_depr(WARNING     LLAMA_CURL)

include("cmake/license.cmake")
license_add_file("llama.cpp" "LICENSE")

#
# 3rd-party
#

if (LLAMA_USE_SYSTEM_GGML)
    message(STATUS "Using system-provided libggml, skipping ggml build")
    find_package(ggml REQUIRED)
    add_library(ggml ALIAS ggml::ggml)
endif()

if (NOT TARGET ggml AND NOT LLAMA_USE_SYSTEM_GGML)
    set(GGML_BUILD_NUMBER ${LLAMA_BUILD_NUMBER})
    set(GGML_BUILD_COMMIT ${LLAMA_BUILD_COMMIT})
    add_subdirectory(ggml)
    # ... otherwise assume ggml is added by a parent CMakeLists.txt
endif()

#
# build the library
#

add_subdirectory(src)

#
# utils, programs, examples and tests
#

if (LLAMA_BUILD_COMMON)
    add_subdirectory(common)
    add_subdirectory(vendor/cpp-httplib)
endif()

if (LLAMA_BUILD_COMMON AND LLAMA_BUILD_TESTS AND NOT CMAKE_JS_VERSION)
    include(CTest)
    add_subdirectory(tests)
endif()

if (LLAMA_BUILD_COMMON AND LLAMA_BUILD_EXAMPLES)
    add_subdirectory(examples)
    add_subdirectory(pocs)
endif()

if (LLAMA_BUILD_COMMON AND LLAMA_BUILD_TOOLS)
    add_subdirectory(tools)
endif()

# Automatically add all files from the 'licenses' directory
file(GLOB EXTRA_LICENSES "${CMAKE_SOURCE_DIR}/licenses/LICENSE-*")

foreach(FILE_PATH ${EXTRA_LICENSES})
    get_filename_component(FILE_NAME "${FILE_PATH}" NAME)
    string(REGEX REPLACE "^LICENSE-" "" NAME "${FILE_NAME}")
    license_add_file("${NAME}" "${FILE_PATH}")
endforeach()

if (LLAMA_BUILD_COMMON)
    license_generate(llama-common)
endif()

#
# install
#

include(GNUInstallDirs)
include(CMakePackageConfigHelpers)

set(LLAMA_INCLUDE_INSTALL_DIR ${CMAKE_INSTALL_INCLUDEDIR} CACHE PATH "Location of header  files")
set(LLAMA_LIB_INSTALL_DIR     ${CMAKE_INSTALL_LIBDIR}     CACHE PATH "Location of library files")
set(LLAMA_BIN_INSTALL_DIR     ${CMAKE_INSTALL_BINDIR}     CACHE PATH "Location of binary  files")

set(LLAMA_PUBLIC_HEADERS
    ${CMAKE_CURRENT_SOURCE_DIR}/include/llama.h
    ${CMAKE_CURRENT_SOURCE_DIR}/include/llama-cpp.h)

set_target_properties(llama
    PROPERTIES
        PUBLIC_HEADER "${LLAMA_PUBLIC_HEADERS}")

install(TARGETS llama LIBRARY PUBLIC_HEADER)

if (LLAMA_BUILD_COMMON)
    install(TARGETS llama-common LIBRARY)
endif()

configure_package_config_file(
        ${CMAKE_CURRENT_SOURCE_DIR}/cmake/llama-config.cmake.in
        ${CMAKE_CURRENT_BINARY_DIR}/llama-config.cmake
    INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/llama
    PATH_VARS LLAMA_INCLUDE_INSTALL_DIR
              LLAMA_LIB_INSTALL_DIR
              LLAMA_BIN_INSTALL_DIR )

write_basic_package_version_file(
        ${CMAKE_CURRENT_BINARY_DIR}/llama-version.cmake
    VERSION ${LLAMA_INSTALL_VERSION}
    COMPATIBILITY SameMajorVersion)

install(FILES ${CMAKE_CURRENT_BINARY_DIR}/llama-config.cmake
              ${CMAKE_CURRENT_BINARY_DIR}/llama-version.cmake
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/llama)

install(
    FILES convert_hf_to_gguf.py
    PERMISSIONS
        OWNER_READ
        OWNER_WRITE
        OWNER_EXECUTE
        GROUP_READ
        GROUP_EXECUTE
        WORLD_READ
        WORLD_EXECUTE
    DESTINATION ${CMAKE_INSTALL_BINDIR})

configure_file(cmake/llama.pc.in
        "${CMAKE_CURRENT_BINARY_DIR}/llama.pc"
        @ONLY)

install(FILES "${CMAKE_CURRENT_BINARY_DIR}/llama.pc"
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/pkgconfig)
