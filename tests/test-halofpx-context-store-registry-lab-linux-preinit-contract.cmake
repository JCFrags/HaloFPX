if(NOT DEFINED SOURCE_ROOT)
    message(FATAL_ERROR "SOURCE_ROOT is required")
endif()

file(READ "${SOURCE_ROOT}/CMakeLists.txt" TOP_CMAKE)
file(READ "${SOURCE_ROOT}/tools/server/CMakeLists.txt" SERVER_CMAKE)
file(READ "${SOURCE_ROOT}/tools/server/halofpx-context-store-registry-lab-linux-preinit-internal.h" HEADER)
file(READ "${SOURCE_ROOT}/tools/server/halofpx-context-store-registry-lab-linux-preinit.cpp" SOURCE)
file(READ "${SOURCE_ROOT}/tests/test-halofpx-context-store-registry-lab-linux-preinit.cpp" TEST_SOURCE)

foreach(REQUIRED IN ITEMS
        "option(HALOFPX_REGISTRY_LAB_LINUX_PREINIT"
        "option(HALOFPX_REGISTRY_LAB_LINUX_MUTATION"
        "HALOFPX_REGISTRY_LAB_LINUX_MUTATION is not admitted by ADR-0025"
        "HALOFPX_REGISTRY_LAB_LINUX_PREINIT is available only on Linux")
    string(FIND "${TOP_CMAKE}" "${REQUIRED}" POSITION)
    if(POSITION EQUAL -1)
        message(FATAL_ERROR "missing top-level feature gate: ${REQUIRED}")
    endif()
endforeach()
foreach(REQUIRED IN ITEMS
        "--inherited-alias-probe"
        "--exec-cloexec-probe"
        "--exec-lock-sentinel"
        "prepare_provider_contender(argv, \"busy\""
        "prepare_provider_contender(argv, \"ok_non_authoritative\""
        "run_alias_holder"
        "PR_SET_CHILD_SUBREAPER"
        "kill(holder, SIGKILL)"
        "busy.elapsed_ns < 5000000000ULL"
        "kill(alias, 0)"
        "kill(alias, SIGKILL)"
        "provider_result_is(success, \"status=ok_non_authoritative \""
        "stat_fixture_lock"
        "run_exec_holder"
        "O_RDWR | O_CLOEXEC | O_NOFOLLOW"
        "flags & ~FD_CLOEXEC"
        "no_fd_identity_alias"
        "--exec-lock-sentinel"
        "provider_result_is(acquired, \"status=ok_non_authoritative \"")
    string(FIND "${TEST_SOURCE}" "${REQUIRED}" POSITION)
    if(POSITION EQUAL -1)
        message(FATAL_ERROR "missing inherited-alias/CLOEXEC probe contract: ${REQUIRED}")
    endif()
endforeach()
foreach(REQUIRED IN ITEMS
        "probe_pid role=holder pid=%lld"
        "probe_pid role=alias pid=%lld"
        "probe_pid role=busy_provider pid=%lld"
        "probe_pid role=success_provider pid=%lld"
        "probe_pid role=provider pid=%lld")
    string(FIND "${TEST_SOURCE}" "${REQUIRED}" POSITION)
    if(POSITION EQUAL -1)
        message(FATAL_ERROR "missing retained public PID evidence marker: ${REQUIRED}")
    endif()
endforeach()
foreach(REQUIRED IN ITEMS
        "read_exact_with_deadline"
        "cleanup_prepared_provider"
        "terminate_and_reap"
        "run_prepared_provider(busy_provider, busy, 7000000000ULL)"
        "run_prepared_provider(success_provider, success, 3000000000ULL)"
        "run_prepared_provider(provider, acquired, 3000000000ULL)"
        "fstat(lock_fd, &value)"
        "value.st_nlink != 1"
        "same_lock_snapshot(before_exec, after_acquire, input)"
        "same_lock_snapshot(before_release, after_release, input)")
    string(FIND "${TEST_SOURCE}" "${REQUIRED}" POSITION)
    if(POSITION EQUAL -1)
        message(FATAL_ERROR "missing bounded cleanup/exact-inode probe contract: ${REQUIRED}")
    endif()
endforeach()

foreach(REQUIRED IN ITEMS
        "--qualify"
        "--launch-qualify"
        "--launch-guard-probe"
        "--guard-probe"
        "All argv fields are public launcher-pinned facts"
        "The credential package,"
        "is accepted only from already-installed fd 3"
        "status=%s credential_admitted=%u credential_before_root=%u")
    string(FIND "${TEST_SOURCE}" "${REQUIRED}" POSITION)
    if(POSITION EQUAL -1)
        message(FATAL_ERROR "missing bounded qualification CLI contract: ${REQUIRED}")
    endif()
endforeach()
foreach(REQUIRED IN ITEMS
        "run_guard_holder"
        "F_OFD_SETLK"
        "std::thread active"
        "readlink(\"/proc/self/fd/3\""
        "/memfd:halofpx-registry-lab-credential (deleted)"
        "const audit same_root = qualify_once(input)"
        "const audit different_root = qualify_once(different_input)"
        "same_root.result == status::busy"
        "same_root.credential_syscall_count == 0"
        "different_root.result == status::invalid_request"
        "different_root.credential_syscall_count == 0")
    string(FIND "${TEST_SOURCE}" "${REQUIRED}" POSITION)
    if(POSITION EQUAL -1)
        message(FATAL_ERROR "missing same-process guard probe contract: ${REQUIRED}")
    endif()
endforeach()
foreach(REQUIRED IN ITEMS
        "getrandom(output + offset, size - offset, 0)"
        "MFD_ALLOW_SEALING | MFD_CLOEXEC"
        "F_ADD_SEALS"
        "dup3(memfd, 3, 0)"
        "no_launcher_alias(3)"
        "wipe_test_bytes(package.data(), package.size())"
        "wipe_test_bytes(secret.data(), secret.size())"
        "execv(\"/proc/self/exe\", argv)")
    string(FIND "${TEST_SOURCE}" "${REQUIRED}" POSITION)
    if(POSITION EQUAL -1)
        message(FATAL_ERROR "missing self-launch credential contract: ${REQUIRED}")
    endif()
endforeach()
foreach(FORBIDDEN IN ITEMS "getenv(" "putenv(" "setenv(" "--secret" "SECRET=")
    string(FIND "${TEST_SOURCE}" "${FORBIDDEN}" POSITION)
    if(NOT POSITION EQUAL -1)
        message(FATAL_ERROR "forbidden qualification credential channel: ${FORBIDDEN}")
    endif()
endforeach()

foreach(REQUIRED IN ITEMS
        "CMAKE_SYSTEM_NAME STREQUAL \"Linux\""
        "HALOFPX_REGISTRY_LAB_LINUX_PREINIT"
        "halofpx-context-store-registry-lab-linux-preinit STATIC EXCLUDE_FROM_ALL"
        "test-halofpx-context-store-registry-lab-linux-preinit"
        "halofpx-context-store-registry-lab-linux-preinit Threads::Threads")
    string(FIND "${SERVER_CMAKE}" "${REQUIRED}" POSITION)
    if(POSITION EQUAL -1)
        message(FATAL_ERROR "missing Linux-only target contract: ${REQUIRED}")
    endif()
endforeach()

foreach(FORBIDDEN IN ITEMS
        "install(TARGETS halofpx-context-store-registry-lab-linux-preinit"
        "llama_build(test-halofpx-context-store-registry-lab-linux-preinit"
        "target_link_libraries(halofpx-context-store-registry-lab-linux-preinit PRIVATE halofpx-context-store-registry-lab-wire"
        "target_link_libraries(halofpx-context-store-registry-lab-linux-preinit PUBLIC")
    string(FIND "${SERVER_CMAKE}" "${FORBIDDEN}" POSITION)
    if(NOT POSITION EQUAL -1)
        message(FATAL_ERROR "forbidden target edge: ${FORBIDDEN}")
    endif()
endforeach()

foreach(REQUIRED IN ITEMS
        "audit qualify_once(const request & input) noexcept;"
        "ok_non_authoritative"
        "invalid_request"
        "unsupported"
        "busy"
        "unavailable"
        "io_failure")
    string(FIND "${HEADER}" "${REQUIRED}" POSITION)
    if(POSITION EQUAL -1)
        message(FATAL_ERROR "missing internal API contract: ${REQUIRED}")
    endif()
endforeach()

foreach(FORBIDDEN IN ITEMS
        "concrete_registry_lab_observation"
        "preflight_context_v1"
        "halofpx-context-store-registry-lab-wire"
        "halofpx-context-store-registry-lab-read-only"
        "halofpx-context-store-publication"
        "server-context"
        "llama-server")
    string(FIND "${HEADER}${SOURCE}" "${FORBIDDEN}" POSITION)
    if(NOT POSITION EQUAL -1)
        message(FATAL_ERROR "forbidden provider/source edge: ${FORBIDDEN}")
    endif()
endforeach()

foreach(REQUIRED IN ITEMS
        "F_GET_SEALS"
        "F_SEAL_SEAL | F_SEAL_SHRINK | F_SEAL_GROW | F_SEAL_WRITE"
        "SYS_openat2"
        "RESOLVE_BENEATH | RESOLVE_NO_SYMLINKS"
        "RESOLVE_NO_MAGICLINKS | RESOLVE_NO_XDEV"
        "BTRFS_IOC_FS_INFO"
        "BTRFS_IOC_GET_SUBVOL_INFO"
        "SYS_statx"
        "F_OFD_SETLK"
        "CLOCK_MONOTONIC"
        "O_RDWR | O_CLOEXEC | O_NOFOLLOW"
        "primitive.lock"
        "root_guard_device"
        "root_guard_inode"
        "revalidate_parent(parent_fd, input.parent, output)"
        "revalidate_opened_directory(parent_fd, input.parent,"
        "result = inspect_layout(reopened, fixture, output)"
        "result = reserve_and_read_only(reopened, expected, output)"
        "result = inspect_lock_file(reopened_lock, input, output)"
        "if (errno == EBADF)"
        "leaves all credential audit facts false")
    string(FIND "${SOURCE}" "${REQUIRED}" POSITION)
    if(POSITION EQUAL -1)
        message(FATAL_ERROR "missing primitive contract: ${REQUIRED}")
    endif()
endforeach()

foreach(FORBIDDEN IN ITEMS
        "::fcntl(fd, F_SETLK"
        "flock("
        "openat("
        "O_CREAT"
        "O_TRUNC"
        "rename("
        "unlink("
        "fsync("
        "fdatasync("
        "write("
        "pwrite("
        "mkdir("
        "ftruncate(")
    string(FIND "${SOURCE}" "${FORBIDDEN}" POSITION)
    if(NOT POSITION EQUAL -1)
        message(FATAL_ERROR "mutating or fallback primitive found: ${FORBIDDEN}")
    endif()
endforeach()
