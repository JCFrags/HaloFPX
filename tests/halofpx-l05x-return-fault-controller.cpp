#if !defined(__linux__) || !defined(__x86_64__)
#error "The L05x returned-fault controller currently requires Linux x86-64"
#endif

int halofpx_l05x_crash_controller_unused_main(int, char **);

// Reuse the already reviewed authority-pinning and tracee-inspection primitives
// from the crash controller.  Renaming its entry point keeps this qualification
// executable separate: it injects returned errors and never changes the
// production archive or child.  Receipt publication and post-fault retention
// are implemented independently below to meet the stricter returned-fault gate.
#define unlinkat halofpx_l05x_forbidden_unlinkat
#define main halofpx_l05x_crash_controller_unused_main
#include "halofpx-l05x-ptrace-controller.cpp"
#undef main
#undef unlinkat

#include <linux/btrfs.h>

// Even the unreachable, renamed crash-controller entry point imported above is
// prevented from unlinking individual objects.  ADR-0026 permits only disposal
// of the whole externally verified mount/image after its evidence is retained.
extern "C" int halofpx_l05x_forbidden_unlinkat(
        int, const char *, int) noexcept {
    errno = EPERM;
    return -1;
}

#include <sys/user.h>

namespace {
namespace return_fault {

enum class injection_mode { pre_mutation, late_completion, eintr_once };
enum class generic_syscall {
    openat2_call, statx_call, fstat_call, fstatfs_call, fstatvfs_call,
    ioctl_call, getdents64_call, readlink_call, close_call, ofd_unlock_call,
};

struct arguments {
    const char * target = nullptr;
    const char * golden = nullptr;
    const char * parent = nullptr;
    const char * root = nullptr;
    const char * fixture = nullptr;
    const char * disposable_root = nullptr;
    const char * receipt_parent = nullptr;
    boundary point {};
    injection_mode mode = injection_mode::pre_mutation;
    int returned_errno = 0;
    generic_syscall generic = generic_syscall::openat2_call;
    unsigned occurrence = 0;
    unsigned expected_prefix = 0;
    std::array<unsigned, 3> expected_modes {};
    unsigned expected_mode_count = 0;
    unsigned expected_facts = 0;
    bool point_set = false;
    bool generic_set = false;
    bool occurrence_set = false;
    bool expected_prefix_set = false;
    bool expected_modes_set = false;
    bool expected_facts_set = false;
    bool mode_set = false;
    bool errno_set = false;
};

struct fault_state {
    bool pending_return = false;
    bool first_return_replaced = false;
    bool retry_entry_seen = false;
    bool retry_succeeded = false;
    unsigned matching_entries = 0;
};

[[noreturn]] void return_usage(const char * program) {
    std::fprintf(stderr,
        "usage: %s --target PATH --golden PATH --parent PATH --root PATH "
        "--fixture PATH --disposable-root PATH "
        "--boundary mkdirat-envelopes|mkdirat-attempts|mkdirat-staging|"
        "fchmodat2-envelopes|fchmodat2-attempts|fchmodat2-staging|"
        "fsync-envelopes|fsync-attempts|fsync-staging|fsync-root "
        "or --syscall openat2|statx|fstat|fstatfs|fstatvfs|ioctl|"
        "getdents64|readlink|close|ofd-unlock --occurrence N "
        "--expected-prefix 0..3 --expected-modes none|000[,700...] "
        "--expected-facts 0x000..0x3ff "
        "--mode pre|late|eintr-once --errno "
        "EIO|ENOSPC|EDQUOT|EROFS|EEXIST|EINTR --receipt-parent PATH\n",
        program);
    std::exit(2);
}

bool parse_errno_name(const char * value, int & output) {
    struct named_errno { const char * name; int value; };
    constexpr named_errno admitted[] {
        { "EIO", EIO }, { "ENOSPC", ENOSPC }, { "EDQUOT", EDQUOT },
        { "EROFS", EROFS }, { "EEXIST", EEXIST }, { "EINTR", EINTR },
    };
    for (const auto & item : admitted) {
        if (std::strcmp(value, item.name) == 0) {
            output = item.value;
            return true;
        }
    }
    return false;
}

const char * errno_name(int value) {
    switch (value) {
        case EIO: return "EIO";
        case ENOSPC: return "ENOSPC";
        case EDQUOT: return "EDQUOT";
        case EROFS: return "EROFS";
        case EEXIST: return "EEXIST";
        case EINTR: return "EINTR";
        default: return "unknown";
    }
}

const char * mode_name(injection_mode value) {
    switch (value) {
        case injection_mode::pre_mutation: return "pre";
        case injection_mode::late_completion: return "late";
        case injection_mode::eintr_once: return "eintr-once";
    }
    return "unknown";
}

bool parse_generic_syscall(const char * value, generic_syscall & output) {
    struct named_call { const char * name; generic_syscall value; };
    constexpr named_call admitted[] {
        { "openat2", generic_syscall::openat2_call },
        { "statx", generic_syscall::statx_call },
        { "fstat", generic_syscall::fstat_call },
        { "fstatfs", generic_syscall::fstatfs_call },
        { "fstatvfs", generic_syscall::fstatvfs_call },
        { "ioctl", generic_syscall::ioctl_call },
        { "getdents64", generic_syscall::getdents64_call },
        { "readlink", generic_syscall::readlink_call },
        { "close", generic_syscall::close_call },
        { "ofd-unlock", generic_syscall::ofd_unlock_call },
    };
    for (const auto & item : admitted) {
        if (std::strcmp(value, item.name) == 0) {
            output = item.value;
            return true;
        }
    }
    return false;
}

const char * generic_syscall_name(generic_syscall value) {
    switch (value) {
        case generic_syscall::openat2_call: return "openat2";
        case generic_syscall::statx_call: return "statx";
        case generic_syscall::fstat_call: return "fstat";
        case generic_syscall::fstatfs_call: return "fstatfs";
        case generic_syscall::fstatvfs_call: return "fstatvfs";
        case generic_syscall::ioctl_call: return "ioctl";
        case generic_syscall::getdents64_call: return "getdents64";
        case generic_syscall::readlink_call: return "readlink";
        case generic_syscall::close_call: return "close";
        case generic_syscall::ofd_unlock_call: return "ofd-unlock";
    }
    return "unknown";
}

bool parse_bounded_unsigned(const char * value, unsigned maximum,
                            unsigned & output, int base = 10) {
    if (value == nullptr || value[0] == '\0' || value[0] == '-') return false;
    errno = 0;
    char * end = nullptr;
    const unsigned long parsed = std::strtoul(value, &end, base);
    if (errno != 0 || end == value || *end != '\0' || parsed > maximum) return false;
    output = static_cast<unsigned>(parsed);
    return true;
}

bool parse_expected_modes(const char * value, arguments & output) {
    if (std::strcmp(value, "none") == 0) {
        output.expected_mode_count = 0;
        return true;
    }
    std::string modes(value);
    std::size_t begin = 0;
    while (begin <= modes.size() && output.expected_mode_count < 3) {
        const std::size_t comma = modes.find(',', begin);
        const std::string item = modes.substr(
            begin, comma == std::string::npos ? std::string::npos : comma - begin);
        if (item != "000" && item != "700") return false;
        output.expected_modes[output.expected_mode_count++] =
            item == "000" ? 0000U : 0700U;
        if (comma == std::string::npos) return true;
        begin = comma + 1;
    }
    return false;
}

bool parse_arguments(int argc, char ** argv, arguments & output) {
    for (int i = 1; i < argc; i += 2) {
        if (i + 1 >= argc) return false;
        const char * key = argv[i];
        const char * value = argv[i + 1];
        if (std::strcmp(key, "--target") == 0) {
            if (!set_once(output.target, value)) return false;
        } else if (std::strcmp(key, "--golden") == 0) {
            if (!set_once(output.golden, value)) return false;
        } else if (std::strcmp(key, "--parent") == 0) {
            if (!set_once(output.parent, value)) return false;
        } else if (std::strcmp(key, "--root") == 0) {
            if (!set_once(output.root, value)) return false;
        } else if (std::strcmp(key, "--fixture") == 0) {
            if (!set_once(output.fixture, value)) return false;
        } else if (std::strcmp(key, "--disposable-root") == 0) {
            if (!set_once(output.disposable_root, value)) return false;
        } else if (std::strcmp(key, "--receipt-parent") == 0) {
            if (!set_once(output.receipt_parent, value)) return false;
        } else if (std::strcmp(key, "--boundary") == 0) {
            if (output.point_set || !parse_boundary(value, output.point)) return false;
            output.point_set = true;
        } else if (std::strcmp(key, "--syscall") == 0) {
            if (output.generic_set || !parse_generic_syscall(value, output.generic)) return false;
            output.generic_set = true;
        } else if (std::strcmp(key, "--occurrence") == 0) {
            if (output.occurrence_set ||
                !parse_bounded_unsigned(value, 1000000U, output.occurrence) ||
                output.occurrence == 0) return false;
            output.occurrence_set = true;
        } else if (std::strcmp(key, "--expected-prefix") == 0) {
            if (output.expected_prefix_set ||
                !parse_bounded_unsigned(value, 3U, output.expected_prefix)) return false;
            output.expected_prefix_set = true;
        } else if (std::strcmp(key, "--expected-modes") == 0) {
            if (output.expected_modes_set || !parse_expected_modes(value, output)) return false;
            output.expected_modes_set = true;
        } else if (std::strcmp(key, "--expected-facts") == 0) {
            if (output.expected_facts_set ||
                !parse_bounded_unsigned(value, 0x3ffU, output.expected_facts, 0)) return false;
            output.expected_facts_set = true;
        } else if (std::strcmp(key, "--mode") == 0) {
            if (output.mode_set) return false;
            if (std::strcmp(value, "pre") == 0) {
                output.mode = injection_mode::pre_mutation;
            } else if (std::strcmp(value, "late") == 0) {
                output.mode = injection_mode::late_completion;
            } else if (std::strcmp(value, "eintr-once") == 0) {
                output.mode = injection_mode::eintr_once;
            } else {
                return false;
            }
            output.mode_set = true;
        } else if (std::strcmp(key, "--errno") == 0) {
            if (output.errno_set || !parse_errno_name(value, output.returned_errno)) return false;
            output.errno_set = true;
        } else {
            return false;
        }
    }
    if (!output.target || !output.golden || !output.parent || !output.root ||
        !output.fixture || !output.disposable_root || !output.receipt_parent ||
        !output.mode_set || !output.errno_set || output.point_set == output.generic_set) return false;
    if (output.generic_set) {
        return output.occurrence_set && output.expected_prefix_set &&
               output.expected_modes_set && output.expected_facts_set &&
               output.expected_mode_count == output.expected_prefix &&
               output.mode != injection_mode::eintr_once &&
               (output.returned_errno == EIO || output.returned_errno == EROFS);
    }
    if (output.mode == injection_mode::eintr_once) {
        return output.returned_errno == EINTR &&
               (output.point.op == operation::mkdir_directory ||
                output.point.op == operation::fchmodat2_directory);
    }
    return output.returned_errno != EINTR;
}

bool replace_syscall_with_error_at_entry(pid_t pid) {
    struct user_regs_struct registers {};
    if (::ptrace(PTRACE_GETREGS, pid, nullptr, &registers) != 0) return false;
    registers.orig_rax = static_cast<decltype(registers.orig_rax)>(-1LL);
    return ::ptrace(PTRACE_SETREGS, pid, nullptr, &registers) == 0;
}

bool replace_syscall_return(pid_t pid, int returned_errno) {
    struct user_regs_struct registers {};
    if (::ptrace(PTRACE_GETREGS, pid, nullptr, &registers) != 0) return false;
    registers.rax = static_cast<decltype(registers.rax)>(
        -static_cast<long long>(returned_errno));
    return ::ptrace(PTRACE_SETREGS, pid, nullptr, &registers) == 0;
}

bool exact_audit_failure(const std::string & audit) {
    return !audit.empty() && audit.back() == '\n' &&
           audit.find('\n') == audit.size() - 1 &&
           audit.rfind("result=0 sealed_result=0 ", 0) == 0 &&
           audit.find("latch=1") != std::string::npos &&
           audit.find("writer_released=1") != std::string::npos &&
           audit.find("fixture_released=1") != std::string::npos &&
           audit.find("guard_released=1") != std::string::npos &&
           audit.find("prefix_qualified=0") != std::string::npos &&
           audit.find("prefix_qualified=1") == std::string::npos;
}

bool exact_audit_success(const std::string & audit) {
    return !audit.empty() && audit.back() == '\n' &&
           audit.find('\n') == audit.size() - 1 &&
           audit.rfind("result=0 sealed_result=0 ", 0) == 0 &&
           audit.find("latch=1") != std::string::npos &&
           audit.find("writer_released=1") != std::string::npos &&
           audit.find("fixture_released=1") != std::string::npos &&
           audit.find("guard_released=1") != std::string::npos &&
           audit.find("root_synced=1") != std::string::npos &&
           audit.find("prefix_qualified=1") != std::string::npos;
}

bool exact_generic_audit_failure(const std::string & audit) {
    return !audit.empty() && audit.back() == '\n' &&
           audit.find('\n') == audit.size() - 1 &&
           audit.rfind("result=0 sealed_result=0 ", 0) == 0 &&
           audit.find("latch=1") != std::string::npos &&
           audit.find("guard_released=1") != std::string::npos &&
           audit.find("prefix_qualified=0") != std::string::npos &&
           audit.find("prefix_qualified=1") == std::string::npos;
}

bool read_pipe_to_end(int fd, std::string & output) {
    output.clear();
    std::array<char, 4096> buffer {};
    for (;;) {
        ssize_t count;
        do { count = ::read(fd, buffer.data(), buffer.size()); }
        while (count < 0 && errno == EINTR);
        if (count == 0) return true;
        if (count < 0) return false;
        if (output.size() > 65536U - static_cast<std::size_t>(count)) return false;
        output.append(buffer.data(), static_cast<std::size_t>(count));
    }
}

constexpr char receipt_basename[] = "return-fault-receipt.jsonl";

int open_receipt_parent_componentwise(const char * path) {
    if (path == nullptr || path[0] != '/' || path[1] == '\0') return -1;
    int current = ::open("/", O_RDONLY | O_DIRECTORY | O_CLOEXEC | O_NOFOLLOW);
    if (current < 0) return -1;
    const char * cursor = path + 1;
    while (*cursor != '\0') {
        const char * slash = std::strchr(cursor, '/');
        const std::size_t length = slash == nullptr
            ? std::strlen(cursor) : static_cast<std::size_t>(slash - cursor);
        if (length == 0 || length > NAME_MAX ||
            (length == 1 && cursor[0] == '.') ||
            (length == 2 && cursor[0] == '.' && cursor[1] == '.')) {
            (void) ::close(current);
            return -1;
        }
        std::string component(cursor, length);
        struct open_how how {};
        how.flags = O_RDONLY | O_DIRECTORY | O_CLOEXEC | O_NOFOLLOW;
        how.resolve = RESOLVE_BENEATH | RESOLVE_NO_MAGICLINKS |
                      RESOLVE_NO_SYMLINKS;
        int next;
        do {
            next = static_cast<int>(::syscall(
                SYS_openat2, current, component.c_str(), &how, sizeof(how)));
        } while (next < 0 && errno == EINTR);
        const bool closed = ::close(current) == 0;
        if (next < 0 || !closed) {
            if (next >= 0) (void) ::close(next);
            return -1;
        }
        current = next;
        if (slash == nullptr) break;
        cursor = slash + 1;
    }
    std::array<char, PATH_MAX> proc_path {};
    std::array<char, PATH_MAX> observed {};
    const int count = std::snprintf(proc_path.data(), proc_path.size(),
                                    "/proc/self/fd/%d", current);
    const std::size_t expected = std::strlen(path);
    if (count <= 0 || static_cast<std::size_t>(count) >= proc_path.size() ||
        expected >= observed.size() ||
        ::readlink(proc_path.data(), observed.data(), observed.size()) !=
            static_cast<ssize_t>(expected) ||
        std::memcmp(observed.data(), path, expected) != 0) {
        (void) ::close(current);
        return -1;
    }
    return current;
}

bool fd_has_exact_path(int fd, const char * expected_path) {
    std::array<char, PATH_MAX> proc_path {};
    std::array<char, PATH_MAX> observed {};
    const int count = std::snprintf(proc_path.data(), proc_path.size(),
                                    "/proc/self/fd/%d", fd);
    const std::size_t expected = std::strlen(expected_path);
    return count > 0 && static_cast<std::size_t>(count) < proc_path.size() &&
           expected < observed.size() &&
           ::readlink(proc_path.data(), observed.data(), observed.size()) ==
               static_cast<ssize_t>(expected) &&
           std::memcmp(observed.data(), expected_path, expected) == 0;
}

bool open_pinned_receipt(const char * parent_path, int & parent_fd,
                         struct stat & parent_identity,
                         std::uint64_t & parent_mount,
                         struct stat & receipt_identity) {
    parent_fd = open_receipt_parent_componentwise(parent_path);
    if (parent_fd < 0 || ::fstat(parent_fd, &parent_identity) != 0 ||
        !fd_mount_id(parent_fd, parent_mount) ||
        !S_ISDIR(parent_identity.st_mode) || parent_identity.st_uid != ::geteuid() ||
        (parent_identity.st_mode & 07777) != 0700) return false;
    struct open_how how {};
    how.flags = O_WRONLY | O_CREAT | O_EXCL | O_CLOEXEC | O_NOFOLLOW;
    how.mode = 0600;
    how.resolve = RESOLVE_BENEATH | RESOLVE_NO_MAGICLINKS |
                  RESOLVE_NO_SYMLINKS | RESOLVE_NO_XDEV;
    do {
        receipt_fd = static_cast<int>(::syscall(
            SYS_openat2, parent_fd, receipt_basename, &how, sizeof(how)));
    } while (receipt_fd < 0 && errno == EINTR);
    if (receipt_fd < 0 || ::fchmod(receipt_fd, 0600) != 0 ||
        ::fstat(receipt_fd, &receipt_identity) != 0 ||
        !S_ISREG(receipt_identity.st_mode) || receipt_identity.st_nlink != 1 ||
        receipt_identity.st_size != 0 || receipt_identity.st_uid != ::geteuid() ||
        (receipt_identity.st_mode & 07777) != 0600) return false;
    return true;
}

bool synchronize_pinned_receipt(int parent_fd,
                                const char * parent_path,
                                const struct stat & parent_identity,
                                std::uint64_t parent_mount,
                                const struct stat & receipt_identity) {
    struct stat parent_before {}, parent_after {}, receipt_before {}, receipt_after {};
    std::uint64_t parent_mount_before = 0;
    std::uint64_t parent_mount_after = 0;
    if (::fstat(parent_fd, &parent_before) != 0 ||
        !same_object(parent_identity, parent_before) ||
        !fd_has_exact_path(parent_fd, parent_path) ||
        !fd_mount_id(parent_fd, parent_mount_before) ||
        parent_mount_before != parent_mount ||
        ::fstat(receipt_fd, &receipt_before) != 0 ||
        !same_object(receipt_identity, receipt_before) ||
        !S_ISREG(receipt_before.st_mode) || receipt_before.st_nlink != 1 ||
        receipt_before.st_uid != ::geteuid() ||
        (receipt_before.st_mode & 07777) != 0600 ||
        ::fsync(receipt_fd) != 0 || ::fsync(parent_fd) != 0 ||
        ::fstat(parent_fd, &parent_after) != 0 ||
        !same_object(parent_identity, parent_after) ||
        !fd_has_exact_path(parent_fd, parent_path) ||
        !fd_mount_id(parent_fd, parent_mount_after) ||
        parent_mount_after != parent_mount ||
        ::fstat(receipt_fd, &receipt_after) != 0 ||
        !same_object(receipt_identity, receipt_after) ||
        receipt_after.st_nlink != 1 || receipt_after.st_uid != ::geteuid() ||
        (receipt_after.st_mode & 07777) != 0600) return false;
    return true;
}

#if defined(SYS_fchmodat2)
constexpr long fchmodat2_syscall = SYS_fchmodat2;
#else
// Linux x86-64 assigned fchmodat2 syscall number 452.
constexpr long fchmodat2_syscall = 452;
#endif

bool pin_new_directory_o_path(int root_fd, int index,
                              pinned_directory & output) {
    struct open_how how {};
    how.flags = O_PATH | O_DIRECTORY | O_CLOEXEC | O_NOFOLLOW;
    how.resolve = RESOLVE_BENEATH | RESOLVE_NO_MAGICLINKS |
                  RESOLVE_NO_SYMLINKS | RESOLVE_NO_XDEV;
    int fd;
    do {
        fd = static_cast<int>(::syscall(SYS_openat2, root_fd,
            directory_names[static_cast<std::size_t>(index)], &how, sizeof(how)));
    } while (fd < 0 && errno == EINTR);
    struct stat value {};
    std::uint64_t mount_id = 0;
    const bool valid = fd >= 0 && ::fstat(fd, &value) == 0 &&
        S_ISDIR(value.st_mode) && fd_mount_id(fd, mount_id) &&
        value.st_uid == ::geteuid() &&
        (value.st_mode & 07777) == 0000;
    const bool closed = fd < 0 || ::close(fd) == 0;
    if (!valid || !closed) return false;
    output.pinned = true;
    output.identity = value;
    output.mount_id = mount_id;
    return true;
}

bool selected_return_entry(pid_t pid, const tracee_state & state,
                           const boundary & point, const struct stat & root,
                           std::uint64_t root_mount,
                           const std::array<pinned_directory, 3> & directories) {
    const int index = point.directory_index;
    switch (point.op) {
        case operation::mkdir_directory: {
            if (state.nr != SYS_mkdirat || state.args[2] != 0700) return false;
            std::string name;
            struct stat observed_root {};
            std::uint64_t observed_mount = 0;
            return read_tracee_string(pid, state.args[1], name) &&
                   name == directory_names[static_cast<std::size_t>(index)] &&
                   tracee_fd_identity(pid, static_cast<int>(state.args[0]),
                                      observed_root, observed_mount) &&
                   S_ISDIR(observed_root.st_mode) && same_object(root, observed_root) &&
                   observed_mount == root_mount;
        }
        case operation::fchmodat2_directory: {
            std::string empty_path;
            return state.nr == static_cast<std::uint64_t>(fchmodat2_syscall) &&
                   state.args[2] == 0700 && state.args[3] == AT_EMPTY_PATH &&
                   read_tracee_string(pid, state.args[1], empty_path) &&
                   empty_path.empty() &&
                   exact_directory_fd(pid, static_cast<int>(state.args[0]),
                                      directories[static_cast<std::size_t>(index)]);
        }
        case operation::fsync_directory:
            return state.nr == SYS_fsync &&
                   exact_directory_fd(pid, static_cast<int>(state.args[0]),
                                      directories[static_cast<std::size_t>(index)]);
        case operation::fsync_root: {
            struct stat observed {};
            std::uint64_t observed_mount = 0;
            return state.nr == SYS_fsync &&
                   tracee_fd_identity(pid, static_cast<int>(state.args[0]),
                                      observed, observed_mount) &&
                   S_ISDIR(observed.st_mode) && same_object(root, observed) &&
                   observed_mount == root_mount;
        }
    }
    return false;
}

bool generic_entry_matches(pid_t pid, const tracee_state & state,
                           generic_syscall selected) {
    struct stat identity {};
    std::uint64_t mount_id = 0;
    const auto exact_fd = [&](std::uint64_t raw_fd) {
        return tracee_fd_identity(pid, static_cast<int>(raw_fd), identity, mount_id) &&
               identity.st_ino != 0 && mount_id != 0;
    };
    switch (selected) {
        case generic_syscall::openat2_call: {
            if (state.nr != SYS_openat2 || state.args[3] != sizeof(struct open_how)) return false;
            struct open_how how {};
            if (!read_tracee(pid, state.args[2], &how, sizeof(how))) return false;
            constexpr std::uint64_t exact_resolve = RESOLVE_BENEATH |
                RESOLVE_NO_MAGICLINKS | RESOLVE_NO_SYMLINKS | RESOLVE_NO_XDEV;
            return how.resolve == exact_resolve && how.mode == 0 &&
                   (how.flags & (O_CLOEXEC | O_NOFOLLOW)) ==
                       (O_CLOEXEC | O_NOFOLLOW) &&
                   (how.flags & (O_CREAT | O_EXCL)) == 0 && exact_fd(state.args[0]);
        }
        case generic_syscall::statx_call: {
            std::string empty;
            return state.nr == SYS_statx && state.args[2] ==
                       (AT_EMPTY_PATH | AT_STATX_SYNC_AS_STAT) &&
                   read_tracee_string(pid, state.args[1], empty) && empty.empty() &&
                   exact_fd(state.args[0]);
        }
        case generic_syscall::fstat_call:
            return state.nr == SYS_fstat && exact_fd(state.args[0]) &&
                   (S_ISREG(identity.st_mode) || S_ISDIR(identity.st_mode));
        case generic_syscall::fstatfs_call:
        case generic_syscall::fstatvfs_call:
            return state.nr == SYS_fstatfs && exact_fd(state.args[0]);
        case generic_syscall::ioctl_call:
            return state.nr == SYS_ioctl &&
                   (state.args[1] == BTRFS_IOC_FS_INFO ||
                    state.args[1] == BTRFS_IOC_GET_SUBVOL_INFO) &&
                   exact_fd(state.args[0]);
        case generic_syscall::getdents64_call:
            return state.nr == SYS_getdents64 && state.args[2] == 4096 &&
                   exact_fd(state.args[0]) && S_ISDIR(identity.st_mode);
        case generic_syscall::readlink_call: {
            if (state.nr != SYS_readlink || state.args[2] != 4097) return false;
            std::string path;
            return read_tracee_string(pid, state.args[0], path) &&
                   path.rfind("/proc/self/fd/", 0) == 0 && path.size() > 14;
        }
        case generic_syscall::close_call:
            return state.nr == SYS_close && exact_fd(state.args[0]);
        case generic_syscall::ofd_unlock_call: {
            if (state.nr != SYS_fcntl ||
                static_cast<int>(state.args[1]) != F_OFD_SETLK ||
                !exact_fd(state.args[0])) return false;
            struct flock lock {};
            return read_tracee(pid, state.args[2], &lock, sizeof(lock)) &&
                   lock.l_type == F_UNLCK && lock.l_whence == SEEK_SET &&
                   lock.l_start == 0 && lock.l_len == 0;
        }
    }
    return false;
}

unsigned expected_directory_mode(const arguments & input, int index) {
    if (input.generic_set) {
        return input.expected_modes[static_cast<std::size_t>(index)];
    }
    if (input.mode != injection_mode::eintr_once &&
        input.point.directory_index == index &&
        ((input.point.op == operation::mkdir_directory &&
          input.mode == injection_mode::late_completion) ||
         (input.point.op == operation::fchmodat2_directory &&
          input.mode == injection_mode::pre_mutation))) return 0000;
    return 0700;
}

bool exact_expected_facts(const std::string & audit, unsigned facts) {
    const auto bit = [&](unsigned index) { return (facts >> index) & 1U; };
    const std::string expected =
        "envelopes=" + std::to_string(bit(0)) + "/" +
        std::to_string(bit(1)) + "/" + std::to_string(bit(2)) +
        " attempts=" + std::to_string(bit(3)) + "/" +
        std::to_string(bit(4)) + "/" + std::to_string(bit(5)) +
        " staging=" + std::to_string(bit(6)) + "/" +
        std::to_string(bit(7)) + "/" + std::to_string(bit(8)) +
        " final_dirs=";
    const std::string root = " root_synced=" + std::to_string(bit(9)) +
        " prefix_qualified=0\n";
    const std::size_t offset = audit.find(expected);
    return offset != std::string::npos && audit.find(root, offset + expected.size()) !=
        std::string::npos;
}

bool inventory_returned_root(int root_fd, int expected_count,
                             const struct stat & writer,
                             const std::array<pinned_directory, 3> & directories,
                             const arguments & input) {
    const int duplicate = ::openat(
        root_fd, ".", O_RDONLY | O_DIRECTORY | O_CLOEXEC | O_NOFOLLOW);
    if (duplicate < 0) return false;
    DIR * stream = ::fdopendir(duplicate);
    if (stream == nullptr) {
        (void) ::close(duplicate);
        return false;
    }
    std::vector<std::string> names;
    errno = 0;
    for (;;) {
        dirent * entry = ::readdir(stream);
        if (entry == nullptr) break;
        if (std::strcmp(entry->d_name, ".") != 0 &&
            std::strcmp(entry->d_name, "..") != 0) names.emplace_back(entry->d_name);
        errno = 0;
    }
    const bool read_ok = errno == 0;
    const bool stream_closed = ::closedir(stream) == 0;
    if (!read_ok || !stream_closed ||
        names.size() != static_cast<std::size_t>(expected_count + 1)) return false;
    std::sort(names.begin(), names.end());
    std::vector<std::string> expected { "writer.lock" };
    for (int index = 0; index < expected_count; ++index) {
        expected.emplace_back(directory_names[static_cast<std::size_t>(index)]);
    }
    std::sort(expected.begin(), expected.end());
    if (names != expected) return false;

    int writer_fd = -1;
    if (!open_named(root_fd, "writer.lock", O_RDONLY, writer_fd)) return false;
    struct stat observed_writer {};
    const bool writer_valid = ::fstat(writer_fd, &observed_writer) == 0 &&
        S_ISREG(observed_writer.st_mode) && same_object(writer, observed_writer) &&
        observed_writer.st_nlink == 1 && observed_writer.st_size == 0 &&
        observed_writer.st_uid == ::geteuid() &&
        (observed_writer.st_mode & 07777) == 0600;
    const bool writer_closed = ::close(writer_fd) == 0;
    if (!writer_valid || !writer_closed) return false;

    for (int index = 0; index < expected_count; ++index) {
        const auto & pinned = directories[static_cast<std::size_t>(index)];
        if (!pinned.pinned) return false;
        struct open_how how {};
        how.flags = O_PATH | O_DIRECTORY | O_CLOEXEC | O_NOFOLLOW;
        how.resolve = RESOLVE_BENEATH | RESOLVE_NO_MAGICLINKS |
                      RESOLVE_NO_SYMLINKS | RESOLVE_NO_XDEV;
        int fd;
        do {
            fd = static_cast<int>(::syscall(SYS_openat2, root_fd,
                directory_names[static_cast<std::size_t>(index)], &how, sizeof(how)));
        } while (fd < 0 && errno == EINTR);
        struct stat observed {};
        const unsigned expected_mode = expected_directory_mode(input, index);
        const bool valid = fd >= 0 && ::fstat(fd, &observed) == 0 &&
            S_ISDIR(observed.st_mode) && same_object(pinned.identity, observed) &&
            observed.st_uid == ::geteuid() &&
            static_cast<unsigned>(observed.st_mode & 07777) == expected_mode;
        const bool closed = fd < 0 || ::close(fd) == 0;
        if (!valid || !closed) return false;
        if (expected_mode == 0700) {
            int readable_fd = -1;
            if (!open_named(root_fd,
                    directory_names[static_cast<std::size_t>(index)],
                    O_RDONLY | O_DIRECTORY, readable_fd)) return false;
            const bool empty = directory_empty(readable_fd);
            const bool readable_closed = ::close(readable_fd) == 0;
            if (!empty || !readable_closed) return false;
        }
    }
    return true;
}

int expected_returned_prefix(const boundary & point, injection_mode mode) {
    if (mode == injection_mode::eintr_once) return 3;
    if (point.op == operation::mkdir_directory &&
        mode == injection_mode::pre_mutation) return point.directory_index;
    if (point.op == operation::fsync_root) return 3;
    return point.directory_index + 1;
}

int run(const arguments & input) {
    if (std::strcmp(input.root, input.disposable_root) != 0 ||
        !canonical_existing_path(input.target) || !canonical_existing_path(input.golden) ||
        !canonical_existing_path(input.parent) || !canonical_existing_path(input.root) ||
        !canonical_existing_path(input.fixture) ||
        !canonical_existing_path(input.receipt_parent) ||
        !direct_child(input.root, input.parent) ||
        !direct_child(input.fixture, input.parent) ||
        same_or_descendant(input.receipt_parent, input.parent) ||
        same_or_descendant(input.parent, input.receipt_parent)) {
        std::fprintf(stderr, "non-canonical authority or non-disposable receipt scope\n");
        return 2;
    }
    receipt_ok = true;
    receipt_sequence = 0;
    int receipt_parent_fd = -1;
    struct stat receipt_parent_identity {}, receipt_identity {};
    std::uint64_t receipt_parent_mount = 0;
    if (!open_pinned_receipt(input.receipt_parent, receipt_parent_fd,
                             receipt_parent_identity, receipt_parent_mount,
                             receipt_identity)) return 2;

    const int target_fd = ::open(input.target, O_RDONLY | O_CLOEXEC | O_NOFOLLOW);
    const int parent_fd = ::open(input.parent, O_RDONLY | O_DIRECTORY | O_CLOEXEC | O_NOFOLLOW);
    const int root_fd = ::open(input.root, O_RDONLY | O_DIRECTORY | O_CLOEXEC | O_NOFOLLOW);
    const int fixture_fd = ::open(input.fixture, O_RDONLY | O_DIRECTORY | O_CLOEXEC | O_NOFOLLOW);
    const int fixture_lock_fd = fixture_fd >= 0
        ? ::openat(fixture_fd, "primitive.lock", O_RDONLY | O_CLOEXEC | O_NOFOLLOW) : -1;
    struct stat target_stat {}, root_stat {}, fixture_stat {}, fixture_lock_stat {};
    struct stat parent_stat {};
    std::uint64_t parent_mount = 0;
    std::uint64_t root_mount = 0;
    std::uint64_t fixture_mount = 0;
    if (target_fd < 0 || ::fstat(target_fd, &target_stat) != 0 ||
        !S_ISREG(target_stat.st_mode) || (target_stat.st_mode & 0111) == 0 ||
        parent_fd < 0 || ::fstat(parent_fd, &parent_stat) != 0 ||
        !fd_mount_id(parent_fd, parent_mount) || !S_ISDIR(parent_stat.st_mode) ||
        parent_stat.st_uid != ::geteuid() || (parent_stat.st_mode & 07777) != 0700 ||
        root_fd < 0 || ::fstat(root_fd, &root_stat) != 0 ||
        !S_ISDIR(root_stat.st_mode) || !fd_mount_id(root_fd, root_mount) ||
        root_stat.st_uid != ::geteuid() || (root_stat.st_mode & 07777) != 0700 ||
        fixture_fd < 0 || !fd_mount_id(fixture_fd, fixture_mount) ||
        ::fstat(fixture_fd, &fixture_stat) != 0 || !S_ISDIR(fixture_stat.st_mode) ||
        fixture_stat.st_uid != ::geteuid() || (fixture_stat.st_mode & 07777) != 0700 ||
        fixture_lock_fd < 0 || ::fstat(fixture_lock_fd, &fixture_lock_stat) != 0 ||
        !S_ISREG(fixture_lock_stat.st_mode) || fixture_lock_stat.st_nlink != 1 ||
        fixture_lock_stat.st_size != 0 || fixture_lock_stat.st_uid != ::geteuid() ||
        (fixture_lock_stat.st_mode & 07777) != 0600) {
        if (fixture_lock_fd >= 0) (void) ::close(fixture_lock_fd);
        return 2;
    }
    (void) ::close(fixture_lock_fd);
    const std::string selected_name = input.generic_set
        ? std::string("generic:") + generic_syscall_name(input.generic) +
              "#" + std::to_string(input.occurrence)
        : boundary_name(input.point);
    if (!emit(std::string("\"event\":\"start\",\"boundary\":\"") +
              selected_name + "\",\"mode\":\"" +
              mode_name(input.mode) + "\",\"errno\":\"" +
              errno_name(input.returned_errno) + "\"")) return 2;

    int audit_pipe[2] { -1, -1 };
    if (::pipe2(audit_pipe, O_CLOEXEC) != 0) return 2;
    const pid_t launcher = ::fork();
    if (launcher < 0) return 2;
    if (launcher == 0) {
        (void) ::close(audit_pipe[0]);
        if (::dup2(audit_pipe[1], STDOUT_FILENO) < 0) _exit(125);
        (void) ::close(audit_pipe[1]);
        if (::ptrace(PTRACE_TRACEME, 0, nullptr, nullptr) != 0) _exit(125);
        if (::raise(SIGSTOP) != 0) _exit(125);
        (void) ::umask(0777);
        char executable[64];
        const int count = std::snprintf(executable, sizeof(executable),
                                        "/proc/self/fd/%d", target_fd);
        if (count <= 0 || static_cast<std::size_t>(count) >= sizeof(executable)) _exit(125);
        ::execl(executable, input.target, "--live-directory-controller", input.golden,
                input.parent, input.root, input.fixture, static_cast<char *>(nullptr));
        _exit(127);
    }
    (void) ::close(audit_pipe[1]);
    (void) ::close(target_fd);

    int status = 0;
    if (::waitpid(launcher, &status, 0) != launcher || !WIFSTOPPED(status)) return 2;
    constexpr long ptrace_options = PTRACE_O_TRACESYSGOOD | PTRACE_O_TRACEFORK |
        PTRACE_O_TRACEVFORK | PTRACE_O_TRACECLONE | PTRACE_O_TRACEEXEC | PTRACE_O_EXITKILL;
    if (::ptrace(PTRACE_SETOPTIONS, launcher, nullptr,
                 reinterpret_cast<void *>(ptrace_options)) != 0 ||
        !resume_syscalls(launcher)) return 2;

    std::unordered_map<pid_t, tracee_state> tracees;
    std::unordered_map<pid_t, bool> selected_entries;
    tracees.emplace(launcher, tracee_state {});
    selected_entries.emplace(launcher, false);
    std::array<pinned_directory, 3> directories {};
    fault_state fault {};
    pid_t live_child = -1;
    int live_child_exit = -1;
    int launcher_exit = -1;
    bool launcher_exec_seen = false;
    bool live_child_exec_seen = false;
    bool controller_error = false;
    bool bounded_cleanup = false;
    bool post_latch_seen = false;
    bool writer_pinned = false;
    struct stat writer_identity {};
    const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(20);

    while (!tracees.empty() && std::chrono::steady_clock::now() < deadline) {
        const pid_t pid = ::waitpid(-1, &status, __WALL | WNOHANG);
        if (pid == 0) {
            const struct timespec pause { 0, 1000000 };
            (void) ::nanosleep(&pause, nullptr);
            continue;
        }
        if (pid < 0) {
            if (errno == EINTR) continue;
            if (errno == ECHILD) break;
            controller_error = true;
            break;
        }
        if (WIFEXITED(status) || WIFSIGNALED(status)) {
            const int code = WIFEXITED(status) ? WEXITSTATUS(status) : 128 + WTERMSIG(status);
            if (pid == live_child) live_child_exit = code;
            if (pid == launcher) launcher_exit = code;
            tracees.erase(pid);
            selected_entries.erase(pid);
            continue;
        }
        if (!WIFSTOPPED(status)) continue;
        const int stop_signal = WSTOPSIG(status);
        const unsigned event = static_cast<unsigned>(status) >> 16;
        if (stop_signal == SIGTRAP && event != 0) {
            if (event == PTRACE_EVENT_FORK || event == PTRACE_EVENT_VFORK ||
                event == PTRACE_EVENT_CLONE) {
                unsigned long child_value = 0;
                if (::ptrace(PTRACE_GETEVENTMSG, pid, nullptr, &child_value) != 0) {
                    controller_error = true;
                    break;
                }
                tracees[static_cast<pid_t>(child_value)].initial_sigstop_expected = true;
                selected_entries[static_cast<pid_t>(child_value)] = false;
            } else if (event == PTRACE_EVENT_EXEC && pid == launcher) {
                if (launcher_exec_seen || !same_executable(pid, target_stat)) {
                    controller_error = true;
                    break;
                }
                launcher_exec_seen = true;
            } else if (event == PTRACE_EVENT_EXEC) {
                if (live_child != -1 || !same_executable(pid, target_stat) ||
                    !exact_live_child_argv(pid)) {
                    controller_error = true;
                    break;
                }
                live_child = pid;
                live_child_exec_seen = true;
                tracees[pid].live_child = true;
            }
            if (!resume_syscalls(pid)) controller_error = true;
            continue;
        }
        if (stop_signal == (SIGTRAP | 0x80)) {
            struct __ptrace_syscall_info info {};
            const long size = ::ptrace(PTRACE_GET_SYSCALL_INFO, pid, sizeof(info), &info);
            if (size < 0 || info.arch != AUDIT_ARCH_X86_64) {
                controller_error = true;
                break;
            }
            auto & state = tracees[pid];
            if (info.op == PTRACE_SYSCALL_INFO_ENTRY) {
                state.have_entry = true;
                state.nr = info.entry.nr;
                std::memcpy(state.args, info.entry.args, sizeof(state.args));
                bool selected = false;
                const bool awaiting_eintr_retry =
                    input.mode == injection_mode::eintr_once &&
                    fault.first_return_replaced && !fault.retry_entry_seen &&
                    fault.matching_entries == 1;
                if (state.live_child && !fault.pending_return &&
                    (!fault.first_return_replaced || awaiting_eintr_retry)) {
                    if (input.generic_set) {
                        if (post_latch_seen &&
                            generic_entry_matches(pid, state, input.generic)) {
                            ++fault.matching_entries;
                            selected = fault.matching_entries == input.occurrence;
                        }
                    } else if (selected_return_entry(
                            pid, state, input.point, root_stat,
                            root_mount, directories)) {
                        ++fault.matching_entries;
                        selected = true;
                    }
                }
                if (selected) {
                    selected_entries[pid] = true;
                    if (input.mode == injection_mode::pre_mutation ||
                        (input.mode == injection_mode::eintr_once &&
                         fault.matching_entries == 1)) {
                        if (!replace_syscall_with_error_at_entry(pid)) {
                            controller_error = true;
                            break;
                        }
                        fault.pending_return = true;
                    } else if (input.mode == injection_mode::eintr_once &&
                               fault.matching_entries == 2) {
                        fault.retry_entry_seen = true;
                    }
                }
            } else if (info.op == PTRACE_SYSCALL_INFO_EXIT) {
                if (state.live_child && state.have_entry) {
                    const bool match = selected_entries[pid];
                    if (state.nr == SYS_openat2 &&
                        writer_openat2_entry(pid, state, root_stat, root_mount) &&
                        !info.exit.is_error && info.exit.rval >= 0) {
                        struct stat writer {};
                        std::uint64_t writer_mount = 0;
                        if (!tracee_fd_identity(pid, static_cast<int>(info.exit.rval),
                                               writer, writer_mount) ||
                            !S_ISREG(writer.st_mode) || writer.st_nlink != 1 ||
                            writer.st_size != 0 || writer.st_uid != ::geteuid() ||
                            writer_mount != root_mount) {
                            controller_error = true;
                            break;
                        }
                        writer_identity = writer;
                        writer_pinned = true;
                        post_latch_seen = true;
                    }
                    if (state.nr == SYS_mkdirat && !info.exit.is_error && info.exit.rval == 0) {
                        std::string name;
                        if (!read_tracee_string(pid, state.args[1], name)) {
                            controller_error = true;
                            break;
                        }
                        for (int index = 0; index < 3; ++index) {
                            if (name == directory_names[static_cast<std::size_t>(index)] &&
                                !pin_new_directory_o_path(root_fd, index,
                                    directories[static_cast<std::size_t>(index)])) {
                                controller_error = true;
                                break;
                            }
                        }
                        if (controller_error) break;
                    }
                    if (fault.pending_return) {
                        if (!info.exit.is_error || info.exit.rval != -ENOSYS ||
                            !replace_syscall_return(pid, input.returned_errno)) {
                            controller_error = true;
                            break;
                        }
                        fault.pending_return = false;
                        fault.first_return_replaced = true;
                    } else if (match && input.mode == injection_mode::late_completion &&
                               fault.matching_entries ==
                                   (input.generic_set ? input.occurrence : 1U)) {
                        const bool successful_return = !info.exit.is_error &&
                            (input.generic_set ? info.exit.rval >= 0 : info.exit.rval == 0);
                        if (!successful_return ||
                            !replace_syscall_return(pid, input.returned_errno)) {
                            controller_error = true;
                            break;
                        }
                        fault.first_return_replaced = true;
                    } else if (match && input.mode == injection_mode::eintr_once &&
                               fault.retry_entry_seen && fault.matching_entries == 2) {
                        fault.retry_succeeded = !info.exit.is_error && info.exit.rval == 0;
                    }
                }
                selected_entries[pid] = false;
                state.have_entry = false;
            } else {
                controller_error = true;
                break;
            }
            if (!resume_syscalls(pid)) controller_error = true;
            continue;
        }
        auto item = tracees.find(pid);
        if (stop_signal == SIGSTOP && item != tracees.end() &&
            item->second.initial_sigstop_expected) {
            item->second.initial_sigstop_expected = false;
            if (!resume_syscalls(pid)) controller_error = true;
            continue;
        }
        if (stop_signal == SIGSTOP || stop_signal == SIGTRAP) {
            controller_error = true;
            break;
        }
        if (!resume_syscalls(pid, stop_signal)) controller_error = true;
    }

    if (!tracees.empty()) {
        bounded_cleanup = true;
        for (const auto & item : tracees) (void) ::kill(item.first, SIGKILL);
        while (::waitpid(-1, &status, __WALL) > 0 || errno == EINTR) {}
        tracees.clear();
        selected_entries.clear();
    }
    std::string audit;
    const bool audit_captured = read_pipe_to_end(audit_pipe[0], audit);
    const bool audit_pipe_closed = ::close(audit_pipe[0]) == 0;

    struct stat parent_now {}, root_now {}, fixture_now {};
    std::uint64_t parent_mount_now = 0;
    std::uint64_t root_mount_now = 0;
    std::uint64_t fixture_mount_now = 0;
    const bool authorities_pinned = ::fstat(parent_fd, &parent_now) == 0 &&
        same_object(parent_stat, parent_now) &&
        fd_mount_id(parent_fd, parent_mount_now) && parent_mount_now == parent_mount &&
        ::fstat(root_fd, &root_now) == 0 &&
        ::fstat(fixture_fd, &fixture_now) == 0 && same_object(root_stat, root_now) &&
        same_object(fixture_stat, fixture_now) &&
        fd_mount_id(root_fd, root_mount_now) && root_mount_now == root_mount &&
        fd_mount_id(fixture_fd, fixture_mount_now) &&
        fixture_mount_now == fixture_mount;
    const int prefix = input.generic_set
        ? static_cast<int>(input.expected_prefix)
        : expected_returned_prefix(input.point, input.mode);
    const bool inventory_exact = authorities_pinned && writer_pinned &&
        inventory_returned_root(
            root_fd, prefix, writer_identity, directories, input);
    const bool fixture_released = prove_ofd_released(
        fixture_fd, "primitive.lock", fixture_lock_stat);
    const bool writer_released = writer_pinned && prove_ofd_released(
        root_fd, "writer.lock", writer_identity);
    const bool retry_mode = input.mode == injection_mode::eintr_once;
    const bool injection_contract = fault.first_return_replaced &&
        (input.generic_set
             ? fault.matching_entries == input.occurrence
             : retry_mode ? fault.retry_entry_seen && fault.retry_succeeded &&
                                fault.matching_entries == 2
                          : fault.matching_entries == 1);
    const bool process_contract = input.generic_set
        ? live_child_exit != 0 && launcher_exit != 0 &&
              exact_generic_audit_failure(audit) &&
              exact_expected_facts(audit, input.expected_facts)
        : retry_mode
        ? live_child_exit == 0 && launcher_exit == 0 && exact_audit_success(audit)
        : live_child_exit != 0 && launcher_exit != 0 && exact_audit_failure(audit);
    const bool qualification_pass = !controller_error && !bounded_cleanup &&
        launcher_exec_seen && live_child_exec_seen && tracees.empty() &&
        audit_captured && audit_pipe_closed && injection_contract && process_contract &&
        inventory_exact && fixture_released && writer_released;

    emit(std::string("\"event\":\"post-fault\",\"pass\":") +
         (qualification_pass ? "true" : "false") +
         ",\"prefix\":" + std::to_string(prefix) +
         ",\"matching_entries\":" + std::to_string(fault.matching_entries) +
         ",\"return_replaced\":" +
         (fault.first_return_replaced ? "true" : "false") +
         ",\"launcher_exec_seen\":" + (launcher_exec_seen ? "true" : "false") +
         ",\"live_child_exec_seen\":" + (live_child_exec_seen ? "true" : "false") +
         ",\"live_child_status\":" + std::to_string(live_child_exit) +
         ",\"writer_pinned\":" + (writer_pinned ? "true" : "false") +
         ",\"inventory_exact\":" + (inventory_exact ? "true" : "false") +
         ",\"fixture_lock_released\":" + (fixture_released ? "true" : "false") +
         ",\"writer_lock_released\":" + (writer_released ? "true" : "false") +
         ",\"launcher_status\":" + std::to_string(launcher_exit));
    emit(std::string("\"event\":\"summary\",\"pass\":") +
         (qualification_pass ? "true" : "false") +
         ",\"exact_tree_retained\":" + (inventory_exact ? "true" : "false") +
         ",\"mount_discard_required\":true" +
         ",\"audit_prefix_qualified\":" +
         (audit.find("prefix_qualified=1") != std::string::npos ? "true" : "false"));

    const bool synchronized = synchronize_pinned_receipt(
        receipt_parent_fd, input.receipt_parent, receipt_parent_identity,
        receipt_parent_mount,
        receipt_identity);
    const bool receipt_closed = ::close(receipt_fd) == 0;
    receipt_fd = -1;
    const bool receipt_parent_closed = ::close(receipt_parent_fd) == 0;
    const bool fixture_closed = ::close(fixture_fd) == 0;
    const bool root_closed = ::close(root_fd) == 0;
    const bool parent_closed = ::close(parent_fd) == 0;
    return qualification_pass && receipt_ok && synchronized &&
           receipt_closed && receipt_parent_closed && fixture_closed && root_closed &&
           parent_closed ? 0 : 1;
}

} // namespace return_fault
} // namespace

int main(int argc, char ** argv) {
    return_fault::arguments input {};
    if (!return_fault::parse_arguments(argc, argv, input)) {
        return_fault::return_usage(argv[0]);
    }
    return return_fault::run(input);
}
