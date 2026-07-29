llama_add_compile_flags()

function(llama_build source)
    set(TEST_SOURCES ${source} ${ARGN})

    if (DEFINED LLAMA_TEST_NAME)
        set(TEST_TARGET ${LLAMA_TEST_NAME})
    else()
        get_filename_component(TEST_TARGET ${source} NAME_WE)
    endif()

    add_executable(${TEST_TARGET} ${TEST_SOURCES})
    target_link_libraries(${TEST_TARGET} PRIVATE llama llama-common)
    if (LLAMA_TESTS_INSTALL)
        install(TARGETS ${TEST_TARGET} RUNTIME)
    endif()
endfunction()

function(llama_test target)
    include(CMakeParseArguments)
    set(options)
    set(oneValueArgs NAME LABEL WORKING_DIRECTORY)
    set(multiValueArgs ARGS)
    cmake_parse_arguments(LLAMA_TEST "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

    if (NOT DEFINED LLAMA_TEST_LABEL)
        set(LLAMA_TEST_LABEL "main")
    endif()
    if (NOT DEFINED LLAMA_TEST_WORKING_DIRECTORY)
        set(LLAMA_TEST_WORKING_DIRECTORY .)
    endif()
    if (DEFINED LLAMA_TEST_NAME)
        set(TEST_NAME ${LLAMA_TEST_NAME})
    else()
        set(TEST_NAME ${target})
    endif()

    set(TEST_TARGET ${target})

    add_test(
        NAME ${TEST_NAME}
        WORKING_DIRECTORY ${LLAMA_TEST_WORKING_DIRECTORY}
        COMMAND $<TARGET_FILE:${TEST_TARGET}>
        ${LLAMA_TEST_ARGS})

    set_property(TEST ${TEST_NAME} PROPERTY LABELS ${LLAMA_TEST_LABEL})
endfunction()

function(llama_test_cmd target)
    include(CMakeParseArguments)
    set(options)
    set(oneValueArgs NAME LABEL WORKING_DIRECTORY)
    set(multiValueArgs ARGS)
    cmake_parse_arguments(LLAMA_TEST "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

    if (NOT DEFINED LLAMA_TEST_LABEL)
        set(LLAMA_TEST_LABEL "main")
    endif()
    if (NOT DEFINED LLAMA_TEST_WORKING_DIRECTORY)
        set(LLAMA_TEST_WORKING_DIRECTORY .)
    endif()
    if (DEFINED LLAMA_TEST_NAME)
        set(TEST_NAME ${LLAMA_TEST_NAME})
    else()
        set(TEST_NAME ${target})
    endif()

    add_test(
        NAME ${TEST_NAME}
        WORKING_DIRECTORY ${LLAMA_TEST_WORKING_DIRECTORY}
        COMMAND ${target}
        ${LLAMA_TEST_ARGS})

    set_property(TEST ${TEST_NAME} PROPERTY LABELS ${LLAMA_TEST_LABEL})
endfunction()

# Builds and runs a test source file.
# Optional args:
# - NAME: name of the executable & test target (defaults to the source file name without extension)
# - LABEL: label for the test (defaults to main)
# - ARGS: arguments to pass to the test executable
# - WORKING_DIRECTORY
function(llama_build_and_test source)
    include(CMakeParseArguments)
    set(options)
    set(oneValueArgs NAME LABEL WORKING_DIRECTORY)
    set(multiValueArgs ARGS)
    cmake_parse_arguments(LLAMA_TEST "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

    set(TEST_SOURCES ${source} ${LLAMA_TEST_UNPARSED_ARGUMENTS} get-model.cpp)

    if (NOT DEFINED LLAMA_TEST_LABEL)
        set(LLAMA_TEST_LABEL "main")
    endif()
    if (NOT DEFINED LLAMA_TEST_WORKING_DIRECTORY)
        set(LLAMA_TEST_WORKING_DIRECTORY .)
    endif()
    if (DEFINED LLAMA_TEST_NAME)
        set(TEST_TARGET ${LLAMA_TEST_NAME})
    else()
        get_filename_component(TEST_TARGET ${source} NAME_WE)
    endif()

    add_executable(${TEST_TARGET} ${TEST_SOURCES})
    if (LLAMA_TESTS_INSTALL)
        install(TARGETS ${TEST_TARGET} RUNTIME)
    endif()
    target_link_libraries(${TEST_TARGET} PRIVATE llama-common)

    add_test(
        NAME ${TEST_TARGET}
        WORKING_DIRECTORY ${LLAMA_TEST_WORKING_DIRECTORY}
        COMMAND $<TARGET_FILE:${TEST_TARGET}>
        ${LLAMA_TEST_ARGS})

    set_property(TEST ${TEST_TARGET} PROPERTY LABELS ${LLAMA_TEST_LABEL})
endfunction()

if (LLAMA_BUILD_SERVER)
    add_test(
        NAME test-halofpx-feature-off-contract
        COMMAND ${CMAKE_COMMAND}
            -DHALOFPX_SERVER=$<TARGET_FILE:llama-server>
            -DHALOFPX_SOURCE_DIR=${PROJECT_SOURCE_DIR}
            -DHALOFPX_CONTEXT_STORE_CANARY=${HALOFPX_CONTEXT_STORE_CANARY}
            -DHALOFPX_CONTEXT_STORE_PROTECTED_CANARY=${HALOFPX_CONTEXT_STORE_PROTECTED_CANARY}
            -DHALOFPX_CONTEXT_STORE_COMPONENT_AUTHORITY=${HALOFPX_CONTEXT_STORE_COMPONENT_AUTHORITY}
            -DHALOFPX_CONTEXT_STORE_FULL_V1_CANARY=${HALOFPX_CONTEXT_STORE_FULL_V1_CANARY}
            -DHALOFPX_CONTEXT_STORE_EXACT_KEY_CANARY=${HALOFPX_CONTEXT_STORE_EXACT_KEY_CANARY}
            -DHALOFPX_CONTEXT_STORE_EXACT_KEY_CATALOG_CANARY=${HALOFPX_CONTEXT_STORE_EXACT_KEY_CATALOG_CANARY}
            -P ${CMAKE_CURRENT_SOURCE_DIR}/test-halofpx-feature-off-contract.cmake)
    set_property(TEST test-halofpx-feature-off-contract PROPERTY LABELS "main;halofpx")
endif()

add_test(
    NAME test-halofpx-l02-contracts
    COMMAND ${CMAKE_COMMAND}
        -DHALOFPX_SOURCE_DIR=${PROJECT_SOURCE_DIR}
        -P ${CMAKE_CURRENT_SOURCE_DIR}/test-halofpx-l02-contracts.cmake)
set_property(TEST test-halofpx-l02-contracts PROPERTY LABELS "main;halofpx")

llama_build(test-halofpx-vulkan-q8-predequant.cpp)
target_include_directories(test-halofpx-vulkan-q8-predequant PRIVATE ${CMAKE_SOURCE_DIR}/ggml/src/ggml-vulkan)
llama_test(test-halofpx-vulkan-q8-predequant)
set_property(TEST test-halofpx-vulkan-q8-predequant PROPERTY LABELS "main;halofpx")

llama_build(test-halofpx-context-store.cpp
    ${CMAKE_SOURCE_DIR}/tools/server/halofpx-context-store.cpp)
target_include_directories(test-halofpx-context-store PRIVATE ${CMAKE_SOURCE_DIR}/tools/server)
llama_test(test-halofpx-context-store)
set_property(TEST test-halofpx-context-store PROPERTY LABELS "main;halofpx")

add_test(
    NAME test-halofpx-context-store-contract
    COMMAND ${CMAKE_COMMAND}
        -DHALOFPX_SOURCE_DIR=${PROJECT_SOURCE_DIR}
        -P ${CMAKE_CURRENT_SOURCE_DIR}/test-halofpx-context-store-contract.cmake)
set_property(TEST test-halofpx-context-store-contract PROPERTY LABELS "main;halofpx")

llama_build(test-halofpx-context-store-format.cpp
    ${CMAKE_SOURCE_DIR}/tools/server/halofpx-context-store-format.cpp)
target_include_directories(test-halofpx-context-store-format PRIVATE ${CMAKE_SOURCE_DIR}/tools/server)
llama_test(test-halofpx-context-store-format)
set_property(TEST test-halofpx-context-store-format PROPERTY LABELS "main;halofpx")

add_test(
    NAME test-halofpx-context-store-format-contract
    COMMAND ${CMAKE_COMMAND}
        -DHALOFPX_SOURCE_DIR=${PROJECT_SOURCE_DIR}
        -P ${CMAKE_CURRENT_SOURCE_DIR}/test-halofpx-context-store-format-contract.cmake)
set_property(TEST test-halofpx-context-store-format-contract PROPERTY LABELS "main;halofpx")

llama_build(test-halofpx-context-store-auth.cpp)
target_include_directories(test-halofpx-context-store-auth PRIVATE ${CMAKE_SOURCE_DIR}/tools/server)
target_link_libraries(test-halofpx-context-store-auth PRIVATE halofpx-context-store-auth)
llama_test(test-halofpx-context-store-auth)
set_property(TEST test-halofpx-context-store-auth PROPERTY LABELS "main;halofpx")

llama_build(test-halofpx-context-store-compatibility-v1.cpp)
target_include_directories(test-halofpx-context-store-compatibility-v1 PRIVATE
    ${CMAKE_SOURCE_DIR}/tools/server)
target_link_libraries(test-halofpx-context-store-compatibility-v1 PRIVATE
    halofpx-context-store-compatibility-v1)
llama_test(test-halofpx-context-store-compatibility-v1)
set_property(TEST test-halofpx-context-store-compatibility-v1 PROPERTY LABELS "main;halofpx")

llama_build(test-halofpx-context-store-scope.cpp)
target_include_directories(test-halofpx-context-store-scope PRIVATE ${CMAKE_SOURCE_DIR}/tools/server)
target_link_libraries(test-halofpx-context-store-scope PRIVATE halofpx-context-store-scope)
llama_test(test-halofpx-context-store-scope)
set_property(TEST test-halofpx-context-store-scope PROPERTY LABELS "main;halofpx")

llama_build(test-halofpx-context-store-exact-session.cpp)
target_include_directories(test-halofpx-context-store-exact-session PRIVATE
    ${CMAKE_SOURCE_DIR}/tools/server)
target_link_libraries(test-halofpx-context-store-exact-session PRIVATE
    halofpx-context-store-exact-session
    halofpx-context-store-scope)
llama_test(test-halofpx-context-store-exact-session)
set_property(TEST test-halofpx-context-store-exact-session PROPERTY LABELS "main;halofpx")

add_test(
    NAME test-halofpx-context-store-exact-session-contract
    COMMAND ${CMAKE_COMMAND}
        -DHALOFPX_SOURCE_DIR=${PROJECT_SOURCE_DIR}
        -DHALOFPX_CONTEXT_STORE_EXACT_KEY_CANARY=${HALOFPX_CONTEXT_STORE_EXACT_KEY_CANARY}
        -P ${CMAKE_CURRENT_SOURCE_DIR}/test-halofpx-context-store-exact-session-contract.cmake)
set_property(TEST test-halofpx-context-store-exact-session-contract PROPERTY LABELS "main;halofpx")

add_test(
    NAME test-halofpx-context-store-exact-key-runtime-contract
    COMMAND ${CMAKE_COMMAND}
        -DHALOFPX_SOURCE_DIR=${PROJECT_SOURCE_DIR}
        -DHALOFPX_CONTEXT_STORE_EXACT_KEY_CANARY=${HALOFPX_CONTEXT_STORE_EXACT_KEY_CANARY}
        -P ${CMAKE_CURRENT_SOURCE_DIR}/test-halofpx-context-store-exact-key-runtime-contract.cmake)
set_property(TEST test-halofpx-context-store-exact-key-runtime-contract PROPERTY LABELS "main;halofpx")

llama_build(test-halofpx-context-store-state-transformer-v1.cpp)
target_include_directories(test-halofpx-context-store-state-transformer-v1 PRIVATE ${CMAKE_SOURCE_DIR}/tools/server)
target_link_libraries(test-halofpx-context-store-state-transformer-v1 PRIVATE
    halofpx-context-store-state-transformer-v1)
llama_test(test-halofpx-context-store-state-transformer-v1)
set_property(TEST test-halofpx-context-store-state-transformer-v1 PROPERTY LABELS "main;halofpx")

if (UNIX AND NOT APPLE)
    llama_build(test-halofpx-context-store-v1-catalog.cpp)
    target_include_directories(test-halofpx-context-store-v1-catalog PRIVATE
        ${CMAKE_SOURCE_DIR}/tools/server)
    target_link_libraries(test-halofpx-context-store-v1-catalog PRIVATE
        halofpx-context-store-v1-catalog)
    llama_test(test-halofpx-context-store-v1-catalog)
    set_property(TEST test-halofpx-context-store-v1-catalog PROPERTY
        LABELS "main;halofpx;linux")

    llama_build(test-halofpx-context-store-linux-direct.cpp)
    target_include_directories(test-halofpx-context-store-linux-direct PRIVATE ${CMAKE_SOURCE_DIR}/tools/server)
    target_link_libraries(test-halofpx-context-store-linux-direct PRIVATE
        halofpx-context-store-linux-direct)
    llama_test(test-halofpx-context-store-linux-direct)
    set_property(TEST test-halofpx-context-store-linux-direct PROPERTY LABELS "main;halofpx")

    llama_build(test-halofpx-context-store-linux-protected.cpp)
    target_include_directories(test-halofpx-context-store-linux-protected PRIVATE ${CMAKE_SOURCE_DIR}/tools/server)
    target_link_libraries(test-halofpx-context-store-linux-protected PRIVATE
        halofpx-context-store-linux-protected)
    llama_test(test-halofpx-context-store-linux-protected)
    set_property(TEST test-halofpx-context-store-linux-protected PROPERTY LABELS "main;halofpx")
endif()

llama_build(test-halofpx-context-store-protected-canary-anchor.cpp)
target_include_directories(test-halofpx-context-store-protected-canary-anchor PRIVATE ${CMAKE_SOURCE_DIR}/tools/server)
target_link_libraries(test-halofpx-context-store-protected-canary-anchor PRIVATE
    halofpx-context-store-protected-canary-anchor)
llama_test(test-halofpx-context-store-protected-canary-anchor)
set_property(TEST test-halofpx-context-store-protected-canary-anchor PROPERTY LABELS "main;halofpx")

add_test(
    NAME test-halofpx-context-store-auth-contract
    COMMAND ${CMAKE_COMMAND}
        -DHALOFPX_SOURCE_DIR=${PROJECT_SOURCE_DIR}
        -P ${CMAKE_CURRENT_SOURCE_DIR}/test-halofpx-context-store-auth-contract.cmake)
set_property(TEST test-halofpx-context-store-auth-contract PROPERTY LABELS "main;halofpx")

llama_build(test-halofpx-context-store-anchor.cpp)
target_include_directories(test-halofpx-context-store-anchor PRIVATE ${CMAKE_SOURCE_DIR}/tools/server)
target_link_libraries(test-halofpx-context-store-anchor PRIVATE halofpx-context-store-anchor)
llama_test(test-halofpx-context-store-anchor)
set_property(TEST test-halofpx-context-store-anchor PROPERTY LABELS "main;halofpx")

add_test(
    NAME test-halofpx-context-store-anchor-contract
    COMMAND ${CMAKE_COMMAND}
        -DSOURCE_ROOT=${CMAKE_SOURCE_DIR}
        -P ${CMAKE_CURRENT_SOURCE_DIR}/test-halofpx-context-store-anchor-contract.cmake)
set_property(TEST test-halofpx-context-store-anchor-contract PROPERTY LABELS "main;halofpx")

find_program(HALOFPX_PYTHON_EXECUTABLE NAMES python3 python REQUIRED)
add_test(
    NAME test-halofpx-context-store-exact-session-golden
    COMMAND ${HALOFPX_PYTHON_EXECUTABLE}
        ${CMAKE_CURRENT_SOURCE_DIR}/halofpx-exact-session-golden.py
        --check 90240152fe0449ba92a1746dcdf804d7cca55f0034d7a96d846cc59d87f1a25c)
set_property(TEST test-halofpx-context-store-exact-session-golden PROPERTY LABELS "main;halofpx")

add_test(
    NAME test-halofpx-context-store-anchor-golden
    COMMAND ${HALOFPX_PYTHON_EXECUTABLE}
        ${CMAKE_CURRENT_SOURCE_DIR}/halofpx-anchor-golden.py
        --check ${CMAKE_SOURCE_DIR}/docs/halofpx/evidence/l05e-anchor-golden-vector.json)
set_property(TEST test-halofpx-context-store-anchor-golden PROPERTY LABELS "main;halofpx")

llama_build(test-halofpx-context-store-authority.cpp)
target_include_directories(test-halofpx-context-store-authority PRIVATE ${CMAKE_SOURCE_DIR}/tools/server)
target_link_libraries(test-halofpx-context-store-authority PRIVATE halofpx-context-store-bootstrap-anchor)
if (MSVC)
    target_link_options(test-halofpx-context-store-authority PRIVATE /STACK:8388608)
endif()
llama_test(test-halofpx-context-store-authority)
set_property(TEST test-halofpx-context-store-authority PROPERTY LABELS "main;halofpx")

add_test(
    NAME test-halofpx-context-store-authority-contract
    COMMAND ${CMAKE_COMMAND}
        -DSOURCE_ROOT=${CMAKE_SOURCE_DIR}
        -P ${CMAKE_CURRENT_SOURCE_DIR}/test-halofpx-context-store-authority-contract.cmake)
set_property(TEST test-halofpx-context-store-authority-contract PROPERTY LABELS "main;halofpx")

llama_build(test-halofpx-context-store-bootstrap-token.cpp)
target_include_directories(test-halofpx-context-store-bootstrap-token PRIVATE ${CMAKE_SOURCE_DIR}/tools/server)
target_link_libraries(test-halofpx-context-store-bootstrap-token PRIVATE halofpx-context-store-bootstrap-token)
llama_test(test-halofpx-context-store-bootstrap-token)
set_property(TEST test-halofpx-context-store-bootstrap-token PROPERTY LABELS "main;halofpx")

add_test(NAME test-halofpx-context-store-bootstrap-token-contract
    COMMAND ${CMAKE_COMMAND} -DSOURCE_ROOT=${CMAKE_SOURCE_DIR}
        -P ${CMAKE_CURRENT_SOURCE_DIR}/test-halofpx-context-store-bootstrap-token-contract.cmake)
set_property(TEST test-halofpx-context-store-bootstrap-token-contract PROPERTY LABELS "main;halofpx")

llama_build(test-halofpx-context-store-protected-registry.cpp)
target_include_directories(test-halofpx-context-store-protected-registry PRIVATE ${CMAKE_SOURCE_DIR}/tools/server)
target_link_libraries(test-halofpx-context-store-protected-registry PRIVATE halofpx-context-store-protected-registry)
llama_test(test-halofpx-context-store-protected-registry)
set_property(TEST test-halofpx-context-store-protected-registry PROPERTY LABELS "main;halofpx")

add_test(NAME test-halofpx-context-store-protected-registry-contract
    COMMAND ${CMAKE_COMMAND} -DSOURCE_ROOT=${PROJECT_SOURCE_DIR}
        -P ${CMAKE_CURRENT_SOURCE_DIR}/test-halofpx-context-store-protected-registry-contract.cmake)
set_property(TEST test-halofpx-context-store-protected-registry-contract PROPERTY LABELS "main;halofpx")

add_test(NAME test-halofpx-context-store-protected-registry-golden
    COMMAND ${HALOFPX_PYTHON_EXECUTABLE}
        ${CMAKE_CURRENT_SOURCE_DIR}/halofpx-protected-registry-golden.py
        --check ${CMAKE_SOURCE_DIR}/docs/halofpx/evidence/l05j-protected-registry-golden-vector.json)
set_property(TEST test-halofpx-context-store-protected-registry-golden PROPERTY LABELS "main;halofpx")

llama_build(test-halofpx-context-store-registry-successor.cpp)
target_include_directories(test-halofpx-context-store-registry-successor PRIVATE ${CMAKE_SOURCE_DIR}/tools/server)
target_link_libraries(test-halofpx-context-store-registry-successor PRIVATE halofpx-context-store-bootstrap-consumption)
llama_test(test-halofpx-context-store-registry-successor)
set_property(TEST test-halofpx-context-store-registry-successor PROPERTY LABELS "main;halofpx")

add_test(NAME test-halofpx-context-store-registry-successor-golden
    COMMAND ${HALOFPX_PYTHON_EXECUTABLE}
        ${CMAKE_CURRENT_SOURCE_DIR}/halofpx-registry-successor-golden.py
        --check ${CMAKE_SOURCE_DIR}/docs/halofpx/evidence/l05k-registry-successor-golden-vector.json)
set_property(TEST test-halofpx-context-store-registry-successor-golden PROPERTY LABELS "main;halofpx")

add_test(NAME test-halofpx-context-store-bootstrap-consumption-contract
    COMMAND ${CMAKE_COMMAND} -DSOURCE_ROOT=${CMAKE_SOURCE_DIR}
        -P ${CMAKE_CURRENT_SOURCE_DIR}/test-halofpx-context-store-bootstrap-consumption-contract.cmake)
set_property(TEST test-halofpx-context-store-bootstrap-consumption-contract PROPERTY LABELS "main;halofpx")

add_test(NAME test-halofpx-context-store-bootstrap-reconciliation-contract
    COMMAND ${CMAKE_COMMAND} -DSOURCE_ROOT=${CMAKE_SOURCE_DIR}
        -P ${CMAKE_CURRENT_SOURCE_DIR}/test-halofpx-context-store-bootstrap-reconciliation-contract.cmake)
set_property(TEST test-halofpx-context-store-bootstrap-reconciliation-contract PROPERTY LABELS "main;halofpx")

add_test(NAME test-halofpx-context-store-bootstrap-reconciliation-golden
    COMMAND ${HALOFPX_PYTHON_EXECUTABLE}
        ${CMAKE_CURRENT_SOURCE_DIR}/halofpx-bootstrap-reconciliation-golden.py --check
        ${CMAKE_SOURCE_DIR}/docs/halofpx/evidence/l05l-bootstrap-reconciliation-golden-vector.json)
set_property(TEST test-halofpx-context-store-bootstrap-reconciliation-golden PROPERTY LABELS "main;halofpx")

add_test(NAME test-halofpx-context-store-bootstrap-material-contract
    COMMAND ${CMAKE_COMMAND} -DSOURCE_ROOT=${CMAKE_SOURCE_DIR}
        -P ${CMAKE_CURRENT_SOURCE_DIR}/test-halofpx-context-store-bootstrap-material-contract.cmake)
set_property(TEST test-halofpx-context-store-bootstrap-material-contract PROPERTY LABELS "main;halofpx")

add_test(NAME test-halofpx-context-store-bootstrap-material-golden
    COMMAND ${HALOFPX_PYTHON_EXECUTABLE}
        ${CMAKE_CURRENT_SOURCE_DIR}/halofpx-bootstrap-material-golden.py --check
        ${CMAKE_SOURCE_DIR}/docs/halofpx/evidence/l05m-bootstrap-material-golden-vector.json)
set_property(TEST test-halofpx-context-store-bootstrap-material-golden PROPERTY LABELS "main;halofpx")

add_test(NAME test-halofpx-context-store-bootstrap-anchor-contract
    COMMAND ${CMAKE_COMMAND} -DSOURCE_ROOT=${CMAKE_SOURCE_DIR}
        -P ${CMAKE_CURRENT_SOURCE_DIR}/test-halofpx-context-store-bootstrap-anchor-contract.cmake)
set_property(TEST test-halofpx-context-store-bootstrap-anchor-contract PROPERTY LABELS "main;halofpx")

add_test(NAME test-halofpx-context-store-bootstrap-anchor-golden
    COMMAND ${HALOFPX_PYTHON_EXECUTABLE}
        ${CMAKE_CURRENT_SOURCE_DIR}/halofpx-bootstrap-anchor-golden.py --check
        ${CMAKE_SOURCE_DIR}/docs/halofpx/evidence/l05n-bootstrap-anchor-golden-vector.json)
set_property(TEST test-halofpx-context-store-bootstrap-anchor-golden PROPERTY LABELS "main;halofpx")

llama_build(test-halofpx-context-store-registry-lab-wire.cpp)
target_include_directories(test-halofpx-context-store-registry-lab-wire PRIVATE ${CMAKE_SOURCE_DIR}/tools/server)
target_link_libraries(test-halofpx-context-store-registry-lab-wire PRIVATE halofpx-context-store-registry-lab-wire)
llama_test(test-halofpx-context-store-registry-lab-wire ARGS
    ${CMAKE_SOURCE_DIR}/docs/halofpx/evidence/l05o-registry-lab-golden-vector.json)
set_property(TEST test-halofpx-context-store-registry-lab-wire PROPERTY LABELS "main;halofpx")

add_test(NAME test-halofpx-context-store-registry-lab-wire-contract
    COMMAND ${CMAKE_COMMAND} -DSOURCE_ROOT=${CMAKE_SOURCE_DIR}
        -P ${CMAKE_CURRENT_SOURCE_DIR}/test-halofpx-context-store-registry-lab-wire-contract.cmake)
set_property(TEST test-halofpx-context-store-registry-lab-wire-contract PROPERTY LABELS "main;halofpx")

add_test(NAME test-halofpx-context-store-registry-lab-golden
    COMMAND ${HALOFPX_PYTHON_EXECUTABLE}
        ${CMAKE_CURRENT_SOURCE_DIR}/halofpx-registry-lab-golden.py --check
        ${CMAKE_SOURCE_DIR}/docs/halofpx/evidence/l05o-registry-lab-golden-vector.json)
set_property(TEST test-halofpx-context-store-registry-lab-golden PROPERTY LABELS "main;halofpx")

llama_build(test-halofpx-context-store-registry-lab-read-only.cpp)
target_include_directories(test-halofpx-context-store-registry-lab-read-only PRIVATE
    ${CMAKE_SOURCE_DIR}/tools/server)
target_link_libraries(test-halofpx-context-store-registry-lab-read-only PRIVATE
    halofpx-context-store-registry-lab-read-only)
llama_test(test-halofpx-context-store-registry-lab-read-only ARGS
    ${CMAKE_SOURCE_DIR}/docs/halofpx/evidence/l05o-registry-lab-golden-vector.json)
set_property(TEST test-halofpx-context-store-registry-lab-read-only PROPERTY
    LABELS "main;halofpx")

add_test(NAME test-halofpx-context-store-registry-lab-read-only-contract
    COMMAND ${CMAKE_COMMAND} -DSOURCE_ROOT=${CMAKE_SOURCE_DIR}
        -P ${CMAKE_CURRENT_SOURCE_DIR}/test-halofpx-context-store-registry-lab-read-only-contract.cmake)
set_property(TEST test-halofpx-context-store-registry-lab-read-only-contract PROPERTY
    LABELS "main;halofpx")

add_test(
    NAME test-halofpx-context-store-bootstrap-token-golden
    COMMAND ${HALOFPX_PYTHON_EXECUTABLE}
        ${CMAKE_CURRENT_SOURCE_DIR}/halofpx-bootstrap-token-golden.py
        --check ${CMAKE_SOURCE_DIR}/docs/halofpx/evidence/l05i-bootstrap-token-golden-vector.json)
set_property(TEST test-halofpx-context-store-bootstrap-token-golden PROPERTY LABELS "main;halofpx")

llama_build(test-halofpx-context-store-object.cpp)
target_include_directories(test-halofpx-context-store-object PRIVATE ${CMAKE_SOURCE_DIR}/tools/server)
target_link_libraries(test-halofpx-context-store-object PRIVATE halofpx-context-store-object)
llama_test(test-halofpx-context-store-object)
set_property(TEST test-halofpx-context-store-object PROPERTY LABELS "main;halofpx")

add_test(
    NAME test-halofpx-context-store-object-contract
    COMMAND ${CMAKE_COMMAND}
        -DHALOFPX_SOURCE_DIR=${PROJECT_SOURCE_DIR}
        -P ${CMAKE_CURRENT_SOURCE_DIR}/test-halofpx-context-store-object-contract.cmake)
set_property(TEST test-halofpx-context-store-object-contract PROPERTY LABELS "main;halofpx")

llama_build(test-halofpx-context-store-v1-read-only.cpp
    ${CMAKE_SOURCE_DIR}/tools/server/halofpx-context-store.cpp)
target_include_directories(test-halofpx-context-store-v1-read-only PRIVATE ${CMAKE_SOURCE_DIR}/tools/server)
target_link_libraries(test-halofpx-context-store-v1-read-only PRIVATE halofpx-context-store-v1-read-only)
llama_test(test-halofpx-context-store-v1-read-only)
set_property(TEST test-halofpx-context-store-v1-read-only PROPERTY LABELS "main;halofpx")

add_test(
    NAME test-halofpx-context-store-v1-read-only-contract
    COMMAND ${CMAKE_COMMAND}
        -DSOURCE_ROOT=${PROJECT_SOURCE_DIR}
        -P ${CMAKE_CURRENT_SOURCE_DIR}/test-halofpx-context-store-v1-read-only-contract.cmake)
set_property(TEST test-halofpx-context-store-v1-read-only-contract PROPERTY LABELS "main;halofpx")

llama_build(test-halofpx-context-store-v1-transformer-codec.cpp
    ${CMAKE_SOURCE_DIR}/tools/server/halofpx-context-store.cpp)
target_include_directories(test-halofpx-context-store-v1-transformer-codec PRIVATE
    ${CMAKE_SOURCE_DIR}/tools/server)
target_link_libraries(test-halofpx-context-store-v1-transformer-codec PRIVATE
    halofpx-context-store-v1-transformer-codec
    halofpx-context-store-v1-read-only)
llama_test(test-halofpx-context-store-v1-transformer-codec)
set_property(TEST test-halofpx-context-store-v1-transformer-codec PROPERTY LABELS "main;halofpx")

add_test(
    NAME test-halofpx-context-store-v1-transformer-codec-contract
    COMMAND ${CMAKE_COMMAND}
        -DSOURCE_ROOT=${PROJECT_SOURCE_DIR}
        -P ${CMAKE_CURRENT_SOURCE_DIR}/test-halofpx-context-store-v1-transformer-codec-contract.cmake)
set_property(TEST test-halofpx-context-store-v1-transformer-codec-contract PROPERTY
    LABELS "main;halofpx")

llama_build(test-halofpx-context-store-v1-attempt-wire.cpp)
target_include_directories(test-halofpx-context-store-v1-attempt-wire PRIVATE
    ${CMAKE_SOURCE_DIR}/tools/server)
target_link_libraries(test-halofpx-context-store-v1-attempt-wire PRIVATE
    halofpx-context-store-v1-attempt-wire)
llama_test(test-halofpx-context-store-v1-attempt-wire)
set_property(TEST test-halofpx-context-store-v1-attempt-wire PROPERTY LABELS "main;halofpx")

add_test(
    NAME test-halofpx-context-store-v1-attempt-wire-contract
    COMMAND ${CMAKE_COMMAND}
        -DSOURCE_ROOT=${PROJECT_SOURCE_DIR}
        -P ${CMAKE_CURRENT_SOURCE_DIR}/test-halofpx-context-store-v1-attempt-wire-contract.cmake)
set_property(TEST test-halofpx-context-store-v1-attempt-wire-contract PROPERTY LABELS "main;halofpx")

add_executable(test-halofpx-state-byte-geometry test-halofpx-state-byte-geometry.cpp)
target_link_libraries(test-halofpx-state-byte-geometry PRIVATE ggml)

if (GGML_RPC_HALOFPX_LOCAL_STATE)
    add_executable(test-halofpx-rpc-local-state test-halofpx-rpc-local-state.cpp)
    target_include_directories(test-halofpx-rpc-local-state PRIVATE ${CMAKE_SOURCE_DIR}/ggml/include)
    target_link_libraries(test-halofpx-rpc-local-state PRIVATE ggml-rpc ggml)
    llama_build(test-halofpx-distributed-state-canary.cpp)
    target_link_libraries(test-halofpx-distributed-state-canary PRIVATE ggml-rpc)
    llama_build(test-halofpx-graph-input-authority.cpp)
    target_include_directories(test-halofpx-graph-input-authority PRIVATE
        ${CMAKE_SOURCE_DIR}/src)
    target_link_libraries(test-halofpx-graph-input-authority PRIVATE ggml-rpc)
endif()

if (CMAKE_SYSTEM_NAME STREQUAL "Linux")
    llama_build(test-halofpx-context-store-v1-linux-read-only.cpp
        ${CMAKE_SOURCE_DIR}/tools/server/halofpx-context-store.cpp)
    target_include_directories(test-halofpx-context-store-v1-linux-read-only PRIVATE
        ${CMAKE_SOURCE_DIR}/tools/server)
    target_link_libraries(test-halofpx-context-store-v1-linux-read-only PRIVATE
        halofpx-context-store-v1-linux-read-only)
    llama_test(test-halofpx-context-store-v1-linux-read-only)
    set_property(TEST test-halofpx-context-store-v1-linux-read-only PROPERTY
        LABELS "main;halofpx")
endif()

add_test(
    NAME test-halofpx-context-store-v1-linux-read-only-contract
    COMMAND ${CMAKE_COMMAND}
        -DSOURCE_ROOT=${PROJECT_SOURCE_DIR}
        -P ${CMAKE_CURRENT_SOURCE_DIR}/test-halofpx-context-store-v1-linux-read-only-contract.cmake)
set_property(TEST test-halofpx-context-store-v1-linux-read-only-contract PROPERTY
    LABELS "main;halofpx")

if (CMAKE_SYSTEM_NAME STREQUAL "Linux")
    llama_build(test-halofpx-context-store-v1-linux-publish.cpp
        ${CMAKE_SOURCE_DIR}/tools/server/halofpx-context-store.cpp)
    target_include_directories(test-halofpx-context-store-v1-linux-publish PRIVATE
        ${CMAKE_SOURCE_DIR}/tools/server)
    target_link_libraries(test-halofpx-context-store-v1-linux-publish PRIVATE
        halofpx-context-store-v1-linux-publish)
    llama_test(test-halofpx-context-store-v1-linux-publish)
    set_property(TEST test-halofpx-context-store-v1-linux-publish PROPERTY
        LABELS "main;halofpx")

    llama_build(test-halofpx-context-store-v1-linux-generation-one.cpp
        ${CMAKE_SOURCE_DIR}/tools/server/halofpx-context-store.cpp)
    target_include_directories(test-halofpx-context-store-v1-linux-generation-one PRIVATE
        ${CMAKE_SOURCE_DIR}/tools/server)
    target_link_libraries(test-halofpx-context-store-v1-linux-generation-one PRIVATE
        halofpx-context-store-v1-linux-generation-one)
    llama_test(test-halofpx-context-store-v1-linux-generation-one)
    set_property(TEST test-halofpx-context-store-v1-linux-generation-one PROPERTY
        LABELS "main;halofpx")

    llama_build(test-halofpx-context-store-v1-server-canary-selection.cpp)
    target_include_directories(test-halofpx-context-store-v1-server-canary-selection PRIVATE
        ${CMAKE_SOURCE_DIR}/tools/server)
    target_link_libraries(test-halofpx-context-store-v1-server-canary-selection PRIVATE
        halofpx-context-store-v1-server-canary)
    llama_test(test-halofpx-context-store-v1-server-canary-selection)
    set_property(TEST test-halofpx-context-store-v1-server-canary-selection PROPERTY
        LABELS "main;halofpx")
endif()

add_test(
    NAME test-halofpx-context-store-v1-linux-publish-contract
    COMMAND ${CMAKE_COMMAND}
        -DSOURCE_ROOT=${PROJECT_SOURCE_DIR}
        -P ${CMAKE_CURRENT_SOURCE_DIR}/test-halofpx-context-store-v1-linux-publish-contract.cmake)
set_property(TEST test-halofpx-context-store-v1-linux-publish-contract PROPERTY
    LABELS "main;halofpx")

add_test(
    NAME test-halofpx-context-store-v1-linux-generation-one-contract
    COMMAND ${CMAKE_COMMAND}
        -DSOURCE_ROOT=${PROJECT_SOURCE_DIR}
        -P ${CMAKE_CURRENT_SOURCE_DIR}/test-halofpx-context-store-v1-linux-generation-one-contract.cmake)
set_property(TEST test-halofpx-context-store-v1-linux-generation-one-contract PROPERTY
    LABELS "main;halofpx")

llama_build(test-halofpx-context-store-publication.cpp)
target_include_directories(test-halofpx-context-store-publication PRIVATE ${CMAKE_SOURCE_DIR}/tools/server)
target_link_libraries(test-halofpx-context-store-publication PRIVATE halofpx-context-store-publication)
llama_test(test-halofpx-context-store-publication)
set_property(TEST test-halofpx-context-store-publication PROPERTY LABELS "main;halofpx")

add_test(
    NAME test-halofpx-context-store-publication-contract
    COMMAND ${CMAKE_COMMAND}
        -DHALOFPX_SOURCE_DIR=${PROJECT_SOURCE_DIR}
        -P ${CMAKE_CURRENT_SOURCE_DIR}/test-halofpx-context-store-publication-contract.cmake)
set_property(TEST test-halofpx-context-store-publication-contract PROPERTY LABELS "main;halofpx")

llama_build(test-halofpx-context-store-publication-simulator.cpp)
target_include_directories(test-halofpx-context-store-publication-simulator PRIVATE ${CMAKE_SOURCE_DIR}/tools/server)
target_link_libraries(test-halofpx-context-store-publication-simulator PRIVATE halofpx-context-store-publication)
llama_test(test-halofpx-context-store-publication-simulator)
set_property(TEST test-halofpx-context-store-publication-simulator PROPERTY LABELS "main;halofpx")

# build test-tokenizer-0 target once and add many tests
llama_build(test-tokenizer-0.cpp)

llama_test(test-tokenizer-0 NAME test-tokenizer-0-bert-bge          ARGS ${PROJECT_SOURCE_DIR}/models/ggml-vocab-bert-bge.gguf)
llama_test(test-tokenizer-0 NAME test-tokenizer-0-command-r         ARGS ${PROJECT_SOURCE_DIR}/models/ggml-vocab-command-r.gguf)
llama_test(test-tokenizer-0 NAME test-tokenizer-0-deepseek-coder    ARGS ${PROJECT_SOURCE_DIR}/models/ggml-vocab-deepseek-coder.gguf)
llama_test(test-tokenizer-0 NAME test-tokenizer-0-deepseek-llm      ARGS ${PROJECT_SOURCE_DIR}/models/ggml-vocab-deepseek-llm.gguf)
llama_test(test-tokenizer-0 NAME test-tokenizer-0-falcon            ARGS ${PROJECT_SOURCE_DIR}/models/ggml-vocab-falcon.gguf)
llama_test(test-tokenizer-0 NAME test-tokenizer-0-gemma-4           ARGS ${PROJECT_SOURCE_DIR}/models/ggml-vocab-gemma-4.gguf)
llama_test(test-tokenizer-0 NAME test-tokenizer-0-gpt-2             ARGS ${PROJECT_SOURCE_DIR}/models/ggml-vocab-gpt-2.gguf)
llama_test(test-tokenizer-0 NAME test-tokenizer-0-llama-bpe         ARGS ${PROJECT_SOURCE_DIR}/models/ggml-vocab-llama-bpe.gguf)
llama_test(test-tokenizer-0 NAME test-tokenizer-0-llama-spm         ARGS ${PROJECT_SOURCE_DIR}/models/ggml-vocab-llama-spm.gguf)
llama_test(test-tokenizer-0 NAME test-tokenizer-0-mpt               ARGS ${PROJECT_SOURCE_DIR}/models/ggml-vocab-mpt.gguf)
llama_test(test-tokenizer-0 NAME test-tokenizer-0-phi-3             ARGS ${PROJECT_SOURCE_DIR}/models/ggml-vocab-phi-3.gguf)
llama_test(test-tokenizer-0 NAME test-tokenizer-0-qwen2             ARGS ${PROJECT_SOURCE_DIR}/models/ggml-vocab-qwen2.gguf)
llama_test(test-tokenizer-0 NAME test-tokenizer-0-qwen35            ARGS ${PROJECT_SOURCE_DIR}/models/ggml-vocab-qwen35.gguf)
llama_test(test-tokenizer-0 NAME test-tokenizer-0-refact            ARGS ${PROJECT_SOURCE_DIR}/models/ggml-vocab-refact.gguf)
llama_test(test-tokenizer-0 NAME test-tokenizer-0-starcoder         ARGS ${PROJECT_SOURCE_DIR}/models/ggml-vocab-starcoder.gguf)

if (NOT WIN32)
    llama_test_cmd(
        ${CMAKE_CURRENT_SOURCE_DIR}/test-tokenizers-repo.sh
        NAME test-tokenizers-ggml-vocabs
        WORKING_DIRECTORY ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}
        ARGS https://huggingface.co/ggml-org/vocabs ${PROJECT_SOURCE_DIR}/models/ggml-vocabs
    )
endif()

if (LLAMA_LLGUIDANCE)
    llama_build_and_test(test-grammar-llguidance.cpp ARGS ${PROJECT_SOURCE_DIR}/models/ggml-vocab-llama-bpe.gguf)
endif ()

if (NOT WIN32 OR NOT BUILD_SHARED_LIBS)
    # these tests are disabled on Windows because they use internal functions not exported with LLAMA_API (when building with shared libraries)
    llama_build_and_test(test-sampling.cpp)
    llama_build_and_test(test-reasoning-budget.cpp)
    llama_build_and_test(test-grammar-parser.cpp)
    llama_build_and_test(test-grammar-integration.cpp)
    llama_build_and_test(test-llama-grammar.cpp)
    llama_build_and_test(test-chat.cpp WORKING_DIRECTORY ${PROJECT_SOURCE_DIR})
    target_include_directories(test-chat PRIVATE ${PROJECT_SOURCE_DIR}/tools/server)
    target_link_libraries(test-chat PRIVATE server-context)
    # TODO: disabled on loongarch64 because the ggml-ci node lacks Python 3.8
    if (NOT ${CMAKE_SYSTEM_PROCESSOR} MATCHES "loongarch64")
        llama_build_and_test(test-json-schema-to-grammar.cpp   WORKING_DIRECTORY ${PROJECT_SOURCE_DIR})
        target_include_directories(test-json-schema-to-grammar PRIVATE ${PROJECT_SOURCE_DIR}/tools/server)
    endif()

    if (NOT GGML_BACKEND_DL)
        llama_build(test-quantize-stats.cpp)
    endif()

    llama_build(test-gbnf-validator.cpp)

    # build test-tokenizer-1-bpe target once and add many tests
    llama_build(test-tokenizer-1-bpe.cpp)

    # TODO: disabled due to slowness
    #llama_test(test-tokenizer-1-bpe NAME test-tokenizer-1-aquila    ARGS ${PROJECT_SOURCE_DIR}/models/ggml-vocab-aquila.gguf)
    #llama_test(test-tokenizer-1-bpe NAME test-tokenizer-1-falcon    ARGS ${PROJECT_SOURCE_DIR}/models/ggml-vocab-falcon.gguf)
    #llama_test(test-tokenizer-1-bpe NAME test-tokenizer-1-gpt-2     ARGS ${PROJECT_SOURCE_DIR}/models/ggml-vocab-gpt-2.gguf)
    #llama_test(test-tokenizer-1-bpe NAME test-tokenizer-1-gpt-neox  ARGS ${PROJECT_SOURCE_DIR}/models/ggml-vocab-gpt-neox.gguf)
    #llama_test(test-tokenizer-1-bpe NAME test-tokenizer-1-llama-bpe ARGS ${PROJECT_SOURCE_DIR}/models/ggml-vocab-llama-bpe.gguf --ignore-merges)
    #llama_test(test-tokenizer-1-bpe NAME test-tokenizer-1-mpt       ARGS ${PROJECT_SOURCE_DIR}/models/ggml-vocab-mpt.gguf)
    #llama_test(test-tokenizer-1-bpe NAME test-tokenizer-1-refact    ARGS ${PROJECT_SOURCE_DIR}/models/ggml-vocab-refact.gguf)
    #llama_test(test-tokenizer-1-bpe NAME test-tokenizer-1-starcoder ARGS ${PROJECT_SOURCE_DIR}/models/ggml-vocab-starcoder.gguf)

    # build test-tokenizer-1-spm target once and add many tests
    llama_build(test-tokenizer-1-spm.cpp)

    llama_test(test-tokenizer-1-spm  NAME test-tokenizer-1-llama-spm ARGS ${PROJECT_SOURCE_DIR}/models/ggml-vocab-llama-spm.gguf)
    #llama_test(test-tokenizer-1-spm  NAME test-tokenizer-1-baichuan  ARGS ${PROJECT_SOURCE_DIR}/models/ggml-vocab-baichuan.gguf)

    # llama_build_and_test(test-double-float.cpp) # SLOW

    llama_build_and_test(test-llama-archs.cpp)
endif()

llama_build_and_test(test-chat-peg-parser.cpp peg-parser/simple-tokenize.cpp)
llama_build_and_test(test-jinja.cpp)
llama_test(test-jinja NAME test-jinja-py ARGS -py LABEL python)
llama_build_and_test(test-chat-auto-parser.cpp WORKING_DIRECTORY ${PROJECT_SOURCE_DIR})
llama_build_and_test(test-chat-template.cpp)
llama_build_and_test(test-json-partial.cpp)
llama_build_and_test(test-log.cpp)
llama_build_and_test(
    test-peg-parser.cpp
    peg-parser/simple-tokenize.cpp
    peg-parser/test-basic.cpp
    peg-parser/test-gbnf-generation.cpp
    peg-parser/test-json-parser.cpp
    peg-parser/test-json-serialization.cpp
    peg-parser/test-python-dict-parser.cpp
    peg-parser/test-unicode.cpp
    peg-parser/tests.h
)
llama_build_and_test(test-regex-partial.cpp)

if (NOT ${CMAKE_SYSTEM_PROCESSOR} MATCHES "s390x")
    set(MODEL_NAME "tinyllamas/stories15M-q4_0.gguf")
    set(MODEL_HASH "SHA256=66967fbece6dbe97886593fdbb73589584927e29119ec31f08090732d1861739")
else()
    set(MODEL_NAME "tinyllamas/stories15M-be.Q4_0.gguf")
    set(MODEL_HASH "SHA256=9aec857937849d976f30397e97eb1cabb53eb9dcb1ce4611ba8247fb5f44c65d")
endif()
set(MODEL_DEST "${CMAKE_BINARY_DIR}/${MODEL_NAME}")

add_test(NAME test-download-model COMMAND ${CMAKE_COMMAND}
    -DDEST=${MODEL_DEST}
    -DNAME=${MODEL_NAME}
    -DHASH=${MODEL_HASH}
    -P ${CMAKE_SOURCE_DIR}/cmake/download-models.cmake
)
set_tests_properties(test-download-model PROPERTIES FIXTURES_SETUP test-download-model)

llama_build_and_test(test-thread-safety.cpp ARGS -m "${MODEL_DEST}" -ngl 99 -p "The meaning of life is" -n 128 -c 256 -ub 32 -np 4 -t 2)
set_tests_properties(test-thread-safety PROPERTIES FIXTURES_REQUIRED test-download-model)

llama_build_and_test(test-arg-parser.cpp)

if (NOT LLAMA_SANITIZE_ADDRESS AND NOT GGML_SCHED_NO_REALLOC)
  # TODO: repair known memory leaks
  llama_build_and_test(test-opt.cpp)
endif()
llama_build_and_test(test-gguf.cpp)
llama_build_and_test(test-backend-ops.cpp)

if (GGML_HIP AND GGML_RPC)
    llama_build(test-halofpx-rocmfpx-q6-view.cpp)
    target_link_libraries(test-halofpx-rocmfpx-q6-view PRIVATE ggml-rpc)
endif()

if (GGML_RPC)
    llama_build(test-halofpx-placement-probe.cpp)
    target_include_directories(test-halofpx-placement-probe PRIVATE ${CMAKE_SOURCE_DIR}/src)
    llama_test(test-halofpx-placement-probe ARGS --self-test)
    set_property(TEST test-halofpx-placement-probe PROPERTY LABELS "main;halofpx")

    llama_build(test-halofpx-primary-allocation-plan.cpp)
    target_include_directories(test-halofpx-primary-allocation-plan PRIVATE
        ${CMAKE_SOURCE_DIR}/src
        ${CMAKE_SOURCE_DIR}/examples/gguf-hash/deps
        ${CMAKE_SOURCE_DIR}/examples/gguf-hash/deps/sha256)
    target_sources(test-halofpx-primary-allocation-plan PRIVATE
        ${CMAKE_SOURCE_DIR}/examples/gguf-hash/deps/sha256/sha256.c)
    llama_test(test-halofpx-primary-allocation-plan ARGS --self-test)
    set_property(TEST test-halofpx-primary-allocation-plan PROPERTY LABELS "main;halofpx")

    llama_build(test-halofpx-rpc-allocation-refusal.cpp)
    target_link_libraries(test-halofpx-rpc-allocation-refusal PRIVATE ggml-rpc)
endif()

if (HALOFPX_MINIMAX_M2_EXPERT_PARTITION_CANARY)
    target_compile_definitions(test-backend-ops PRIVATE
        HALOFPX_MINIMAX_M2_EXPERT_PARTITION_CANARY)
    target_link_libraries(test-backend-ops PRIVATE
        halofpx-minimax-m2-expert-partition)
    llama_build(test-halofpx-minimax-m2-expert-partition.cpp)
    target_link_libraries(test-halofpx-minimax-m2-expert-partition PRIVATE
        halofpx-minimax-m2-expert-partition)
    llama_test(test-halofpx-minimax-m2-expert-partition)
    set_property(TEST test-halofpx-minimax-m2-expert-partition PROPERTY
        LABELS "halofpx;performance-canary")
endif()

if (HALOFPX_MINIMAX_M2_Q6_PRIVATE_HIP_CANARY)
    llama_build(test-halofpx-minimax-m2-q6-private-hip.cpp)
    target_link_libraries(test-halofpx-minimax-m2-q6-private-hip PRIVATE ggml)
    llama_test(test-halofpx-minimax-m2-q6-private-hip)
    set_property(TEST test-halofpx-minimax-m2-q6-private-hip PROPERTY
        LABELS "halofpx;performance-canary")
endif()

llama_build_and_test(test-model-load-cancel.cpp LABEL "model")
llama_build_and_test(test-autorelease.cpp       LABEL "model")
llama_build_and_test(test-backend-sampler.cpp   LABEL "model")

# Test for state restore with fragmented KV cache
# Requires a model, uses same args pattern as test-thread-safety
llama_build_and_test(test-state-restore-fragmented.cpp LABEL "model" ARGS -m "${MODEL_DEST}")
set_tests_properties(test-state-restore-fragmented PROPERTIES FIXTURES_REQUIRED test-download-model)

if (NOT GGML_BACKEND_DL)
    # these tests use the backends directly and cannot be built with dynamic loading
    llama_build_and_test(test-barrier.cpp)
    llama_build_and_test(test-quantize-fns.cpp)
    llama_build_and_test(test-quantize-perf.cpp)
    llama_build_and_test(test-rope.cpp)
    llama_build_and_test(test-col2im-1d.cpp)

    # TurboQuant CPU reference tests (FWHT, MSE, bitpack)
    llama_build(test-turboquant.cpp)
    llama_test(test-turboquant)
endif()

# libmtmd
set(LLAMA_TEST_NAME test-mtmd-c-api)
llama_build_and_test(test-mtmd-c-api.c)
target_link_libraries(${LLAMA_TEST_NAME} PRIVATE mtmd)
unset(LLAMA_TEST_NAME)

# GGUF model data fetcher library for tests that need real model metadata
# Only compile when cpp-httplib has SSL support (CPPHTTPLIB_OPENSSL_SUPPORT)
if (TARGET cpp-httplib)
    get_target_property(_cpp_httplib_defs cpp-httplib INTERFACE_COMPILE_DEFINITIONS)
    if (_cpp_httplib_defs MATCHES "CPPHTTPLIB_OPENSSL_SUPPORT")
        add_library(gguf-model-data STATIC gguf-model-data.cpp)
        target_link_libraries(gguf-model-data PRIVATE llama-common cpp-httplib)
        target_include_directories(gguf-model-data PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

        add_executable(test-gguf-model-data test-gguf-model-data.cpp)
        target_link_libraries(test-gguf-model-data PRIVATE gguf-model-data llama-common)
        llama_test(test-gguf-model-data LABEL "model")

        # test-quant-type-selection requires gguf-model-data for remote model metadata
        llama_build_and_test(test-quant-type-selection.cpp LABEL "model")
        target_link_libraries(test-quant-type-selection PRIVATE gguf-model-data)
        target_compile_definitions(test-quant-type-selection PRIVATE
            SNAPSHOT_DIR="${CMAKE_CURRENT_SOURCE_DIR}/snapshots")
    endif()
endif()

# dummy executable - not installed
get_filename_component(TEST_TARGET test-c.c NAME_WE)
add_executable(${TEST_TARGET} test-c.c)
target_link_libraries(${TEST_TARGET} PRIVATE llama)

llama_build_and_test(test-alloc.cpp)
target_include_directories(test-alloc PRIVATE ${PROJECT_SOURCE_DIR}/ggml/src)

llama_build_and_test(test-halofpx-loader-partition.cpp)
target_include_directories(test-halofpx-loader-partition PRIVATE
    ${PROJECT_SOURCE_DIR}/src
    ${PROJECT_SOURCE_DIR}/ggml/src)
if (GGML_RPC)
    target_link_libraries(test-halofpx-loader-partition PRIVATE ggml-rpc)
endif()

if (GGML_RPC_HALOFPX_LOCAL_STATE)
    add_executable(test-halofpx-rpc-graph-authority test-halofpx-rpc-graph-authority.cpp)
    target_link_libraries(test-halofpx-rpc-graph-authority PRIVATE llama ggml-rpc)
    add_executable(test-halofpx-scheduler-authority test-halofpx-scheduler-authority.cpp)
    target_link_libraries(test-halofpx-scheduler-authority PRIVATE llama ggml-rpc)
    add_executable(test-halofpx-rpc-mutable-authority test-halofpx-rpc-mutable-authority.cpp)
    target_link_libraries(test-halofpx-rpc-mutable-authority PRIVATE llama ggml-rpc)
    add_executable(test-halofpx-l81-postcompute-diagnostic test_halofpx_l81_postcompute_diagnostic.cpp)
    target_include_directories(test-halofpx-l81-postcompute-diagnostic PRIVATE ${PROJECT_SOURCE_DIR}/src)
    target_link_libraries(test-halofpx-l81-postcompute-diagnostic PRIVATE ggml)
    add_executable(test-halofpx-l83-preprepare-diagnostic test_halofpx_l83_preprepare_diagnostic.cpp)
    target_include_directories(test-halofpx-l83-preprepare-diagnostic PRIVATE ${PROJECT_SOURCE_DIR}/src)
    target_link_libraries(test-halofpx-l83-preprepare-diagnostic PRIVATE ggml)
endif()

llama_build(export-graph-ops.cpp)
target_include_directories(export-graph-ops PRIVATE ${PROJECT_SOURCE_DIR}/ggml/src)
if (TARGET gguf-model-data)
    target_link_libraries(export-graph-ops PRIVATE gguf-model-data)
    target_compile_definitions(export-graph-ops PRIVATE LLAMA_HF_FETCH)
endif()
