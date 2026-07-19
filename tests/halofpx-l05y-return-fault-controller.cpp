#if !defined(__linux__) || !defined(__x86_64__)
#error "The L05y returned-fault controller requires Linux x86-64"
#endif

// Reuse only the already reviewed L05x process, path, receipt, and fd-identity
// inspection primitives.  Its entry point is renamed; this controller neither
// calls the L05x controller nor weakens its exact directory-prefix checks.
int halofpx_l05y_imported_crash_controller_main(int, char **);
#define main halofpx_l05y_imported_crash_controller_main
#include "halofpx-l05x-ptrace-controller.cpp"
#undef main

#include <linux/fs.h>
#include <sstream>
#include <sys/statfs.h>
#include <sys/user.h>

namespace {
namespace marker_fault {

enum class boundary {
    transient_open,
    transient_fchmod,
    marker_pwrite,
    marker_pread,
    marker_pread_eof,
    marker_fsync,
    readonly_temp_open,
    marker_rename,
    root_fsync,
    staging_fsync,
    final_open,
    final_pread,
    final_pread_eof,
    marker_close,
    staging_close,
    writer_unlock,
    fixture_unlock,
    reserve_revalidation,
    final_validation,
};

enum class mode {
    pre_error,
    late_error,
    eintr_once,
    short_pre,
    short_late,
    zero_pre,
    zero_late,
    corrupt,
    truncate,
    append,
    unexpected_name,
    hardlink,
    symlink,
    inode_substitute,
    temp_inode_substitute,
    collision,
    reserve_loss,
};

struct arguments {
    const char * target = nullptr;
    const char * golden = nullptr;
    const char * parent = nullptr;
    const char * root = nullptr;
    const char * fixture = nullptr;
    const char * receipt = nullptr;
    boundary point = boundary::transient_open;
    mode injection = mode::pre_error;
    int returned_errno = EIO;
    unsigned occurrence = 1;
    unsigned short_count = 1;
    bool point_set = false;
    bool mode_set = false;
    bool errno_set = false;
    bool occurrence_set = false;
    bool short_set = false;
};

struct fault_state {
    bool pending_replacement = false;
    bool first_replaced = false;
    bool retry_seen = false;
    bool retry_succeeded = false;
    bool retry_window_open = false;
    bool mutation_applied = false;
    unsigned matches = 0;
};

enum class retry_window_result { outside, closed, selected, excessive };

retry_window_result observe_retry_window(fault_state & fault,
                                         bool exact_boundary,
                                         unsigned occurrence) {
    if (!fault.retry_window_open) return retry_window_result::outside;
    if (!exact_boundary) {
        fault.retry_window_open = false;
        return retry_window_result::closed;
    }
    ++fault.matches;
    return fault.matches > occurrence + 1
        ? retry_window_result::excessive
        : retry_window_result::selected;
}

bool retry_window_self_check() {
    fault_state extra {};
    extra.matches = 4;
    extra.retry_window_open = true;
    if (observe_retry_window(extra, true, 4) != retry_window_result::selected ||
        extra.matches != 5 || !extra.retry_window_open ||
        observe_retry_window(extra, true, 4) != retry_window_result::excessive ||
        extra.matches != 6) return false;
    fault_state phase_reuse {};
    phase_reuse.matches = 4;
    phase_reuse.retry_window_open = true;
    return observe_retry_window(phase_reuse, true, 4) ==
               retry_window_result::selected &&
           observe_retry_window(phase_reuse, false, 4) ==
               retry_window_result::closed &&
           observe_retry_window(phase_reuse, true, 4) ==
               retry_window_result::outside &&
           phase_reuse.matches == 5 && !phase_reuse.retry_window_open;
}

struct marker_state {
    bool transient_seen = false;
    bool published_seen = false;
    bool final_open_seen = false;
    bool marker_pinned = false;
    bool temp_substitute_open_seen = false;
    std::size_t encoded_size = 0;
    std::size_t retained_size = 0;
    std::array<std::uint8_t, 1024> intended_bytes {};
    std::array<std::uint8_t, 1025> retained_bytes {};
    std::array<bool, 1024> intended_known {};
    std::array<bool, 1025> retained_known {};
    struct stat identity {};
    std::uint64_t mount_id = 0;
};

[[noreturn]] void usage_marker(const char * program) {
    std::fprintf(stderr,
        "usage: %s --target PATH --golden PATH --parent PATH --root PATH "
        "--fixture PATH --receipt NEW-PATH --boundary "
        "open-temp|fchmod-temp|pwrite-marker|pread-marker|pread-marker-eof|"
        "fsync-marker|open-readonly-temp|rename-marker|fsync-root|"
        "fsync-staging|open-final|"
        "pread-final|pread-final-eof|close-marker|close-staging|"
        "unlock-writer|unlock-fixture|"
        "reserve-revalidation|final-validation --mode "
        "pre|late|eintr-once|short-pre|short-late|zero-pre|zero-late|"
        "corrupt|truncate|append|unexpected-name|hardlink|symlink|"
        "inode-substitute|temp-inode-substitute|collision|reserve-loss --errno "
        "EIO|ENOSPC|EDQUOT|EROFS|EEXIST|EXDEV|ENOSYS|EINVAL|EINTR "
        "[--occurrence N] [--short-count N]\n", program);
    std::exit(2);
}

bool parse_boundary_name(const char * value, boundary & output) {
    struct row { const char * name; boundary value; };
    constexpr row rows[] {
        { "open-temp", boundary::transient_open },
        { "fchmod-temp", boundary::transient_fchmod },
        { "pwrite-marker", boundary::marker_pwrite },
        { "pread-marker", boundary::marker_pread },
        { "pread-marker-eof", boundary::marker_pread_eof },
        { "fsync-marker", boundary::marker_fsync },
        { "open-readonly-temp", boundary::readonly_temp_open },
        { "rename-marker", boundary::marker_rename },
        { "fsync-root", boundary::root_fsync },
        { "fsync-staging", boundary::staging_fsync },
        { "open-final", boundary::final_open },
        { "pread-final", boundary::final_pread },
        { "pread-final-eof", boundary::final_pread_eof },
        { "close-marker", boundary::marker_close },
        { "close-staging", boundary::staging_close },
        { "unlock-writer", boundary::writer_unlock },
        { "unlock-fixture", boundary::fixture_unlock },
        { "reserve-revalidation", boundary::reserve_revalidation },
        { "final-validation", boundary::final_validation },
    };
    for (const auto & row : rows) {
        if (std::strcmp(value, row.name) == 0) {
            output = row.value;
            return true;
        }
    }
    return false;
}

bool parse_mode_name(const char * value, mode & output) {
    struct row { const char * name; mode value; };
    constexpr row rows[] {
        { "pre", mode::pre_error }, { "late", mode::late_error },
        { "eintr-once", mode::eintr_once },
        { "short-pre", mode::short_pre }, { "short-late", mode::short_late },
        { "zero-pre", mode::zero_pre }, { "zero-late", mode::zero_late },
        { "corrupt", mode::corrupt }, { "truncate", mode::truncate },
        { "append", mode::append },
        { "unexpected-name", mode::unexpected_name },
        { "hardlink", mode::hardlink }, { "symlink", mode::symlink },
        { "inode-substitute", mode::inode_substitute },
        { "temp-inode-substitute", mode::temp_inode_substitute },
        { "collision", mode::collision },
        { "reserve-loss", mode::reserve_loss },
    };
    for (const auto & row : rows) {
        if (std::strcmp(value, row.name) == 0) {
            output = row.value;
            return true;
        }
    }
    return false;
}

bool parse_errno_name(const char * value, int & output) {
    struct row { const char * name; int value; };
    constexpr row rows[] {
        { "EIO", EIO }, { "ENOSPC", ENOSPC }, { "EDQUOT", EDQUOT },
        { "EROFS", EROFS }, { "EEXIST", EEXIST }, { "EXDEV", EXDEV },
        { "ENOSYS", ENOSYS }, { "EINVAL", EINVAL }, { "EINTR", EINTR },
    };
    for (const auto & row : rows) {
        if (std::strcmp(value, row.name) == 0) {
            output = row.value;
            return true;
        }
    }
    return false;
}

bool parse_unsigned(const char * value, unsigned maximum, unsigned & output) {
    if (value == nullptr || value[0] == '\0' || value[0] == '-') return false;
    errno = 0;
    char * end = nullptr;
    const unsigned long parsed = std::strtoul(value, &end, 10);
    if (errno != 0 || end == value || *end != '\0' || parsed == 0 ||
        parsed > maximum) return false;
    output = static_cast<unsigned>(parsed);
    return true;
}

bool parse_arguments(int argc, char ** argv, arguments & output) {
    for (int index = 1; index < argc; index += 2) {
        if (index + 1 >= argc) return false;
        const char * key = argv[index];
        const char * value = argv[index + 1];
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
        } else if (std::strcmp(key, "--receipt") == 0) {
            if (!set_once(output.receipt, value)) return false;
        } else if (std::strcmp(key, "--boundary") == 0) {
            if (output.point_set || !parse_boundary_name(value, output.point)) return false;
            output.point_set = true;
        } else if (std::strcmp(key, "--mode") == 0) {
            if (output.mode_set || !parse_mode_name(value, output.injection)) return false;
            output.mode_set = true;
        } else if (std::strcmp(key, "--errno") == 0) {
            if (output.errno_set || !parse_errno_name(value, output.returned_errno)) return false;
            output.errno_set = true;
        } else if (std::strcmp(key, "--occurrence") == 0) {
            if (output.occurrence_set || !parse_unsigned(value, 1000000, output.occurrence)) return false;
            output.occurrence_set = true;
        } else if (std::strcmp(key, "--short-count") == 0) {
            if (output.short_set || !parse_unsigned(value, 1023, output.short_count)) return false;
            output.short_set = true;
        } else {
            return false;
        }
    }
    if (!output.target || !output.golden || !output.parent || !output.root ||
        !output.fixture || !output.receipt || !output.point_set || !output.mode_set) {
        return false;
    }
    const bool error_mode = output.injection == mode::pre_error ||
        output.injection == mode::late_error || output.injection == mode::eintr_once;
    if (error_mode != output.errno_set) return false;
    if (output.injection == mode::eintr_once && output.returned_errno != EINTR) return false;
    const bool count_mode = output.injection == mode::short_pre ||
        output.injection == mode::short_late;
    if (count_mode != output.short_set) return false;
    const bool byte_count_boundary = output.point == boundary::marker_pwrite ||
        output.point == boundary::marker_pread ||
        output.point == boundary::final_pread;
    if ((count_mode || output.injection == mode::zero_pre ||
         output.injection == mode::zero_late) && !byte_count_boundary) return false;
    const bool mutation_mode = output.injection >= mode::corrupt;
    if (output.injection == mode::reserve_loss) {
        if (output.point != boundary::reserve_revalidation) return false;
    } else if (output.injection == mode::collision) {
        if (output.point != boundary::transient_open &&
            output.point != boundary::marker_rename) return false;
    } else if (output.injection == mode::temp_inode_substitute) {
        if (output.point != boundary::readonly_temp_open) return false;
    } else if (mutation_mode && output.point != boundary::final_validation) {
        return false;
    }
    if (output.point == boundary::final_validation && !mutation_mode) return false;
    return true;
}

bool eintr_is_retryable(boundary value) {
    switch (value) {
        case boundary::marker_pwrite:
        case boundary::marker_pread:
        case boundary::marker_pread_eof:
        case boundary::final_pread:
        case boundary::final_pread_eof:
        case boundary::writer_unlock:
        case boundary::fixture_unlock:
            return true;
        case boundary::transient_open:
        case boundary::transient_fchmod:
        case boundary::marker_fsync:
        case boundary::readonly_temp_open:
        case boundary::marker_rename:
        case boundary::root_fsync:
        case boundary::staging_fsync:
        case boundary::final_open:
        case boundary::marker_close:
        case boundary::staging_close:
        case boundary::reserve_revalidation:
        case boundary::final_validation:
            return false;
    }
    return false;
}

bool returned_eintr_must_retry(const arguments & input) {
    return input.returned_errno == EINTR && eintr_is_retryable(input.point) &&
        (input.injection == mode::pre_error ||
         input.injection == mode::late_error ||
         input.injection == mode::eintr_once);
}

const char * boundary_name(boundary value) {
    switch (value) {
        case boundary::transient_open: return "open-temp";
        case boundary::transient_fchmod: return "fchmod-temp";
        case boundary::marker_pwrite: return "pwrite-marker";
        case boundary::marker_pread: return "pread-marker";
        case boundary::marker_pread_eof: return "pread-marker-eof";
        case boundary::marker_fsync: return "fsync-marker";
        case boundary::readonly_temp_open: return "open-readonly-temp";
        case boundary::marker_rename: return "rename-marker";
        case boundary::root_fsync: return "fsync-root";
        case boundary::staging_fsync: return "fsync-staging";
        case boundary::final_open: return "open-final";
        case boundary::final_pread: return "pread-final";
        case boundary::final_pread_eof: return "pread-final-eof";
        case boundary::marker_close: return "close-marker";
        case boundary::staging_close: return "close-staging";
        case boundary::writer_unlock: return "unlock-writer";
        case boundary::fixture_unlock: return "unlock-fixture";
        case boundary::reserve_revalidation: return "reserve-revalidation";
        case boundary::final_validation: return "final-validation";
    }
    return "unknown";
}

const char * mode_name(mode value) {
    switch (value) {
        case mode::pre_error: return "pre";
        case mode::late_error: return "late";
        case mode::eintr_once: return "eintr-once";
        case mode::short_pre: return "short-pre";
        case mode::short_late: return "short-late";
        case mode::zero_pre: return "zero-pre";
        case mode::zero_late: return "zero-late";
        case mode::corrupt: return "corrupt";
        case mode::truncate: return "truncate";
        case mode::append: return "append";
        case mode::unexpected_name: return "unexpected-name";
        case mode::hardlink: return "hardlink";
        case mode::symlink: return "symlink";
        case mode::inode_substitute: return "inode-substitute";
        case mode::temp_inode_substitute: return "temp-inode-substitute";
        case mode::collision: return "collision";
        case mode::reserve_loss: return "reserve-loss";
    }
    return "unknown";
}

bool replace_entry_with_enosys(pid_t pid) {
    struct user_regs_struct registers {};
    if (::ptrace(PTRACE_GETREGS, pid, nullptr, &registers) != 0) return false;
    registers.orig_rax = static_cast<decltype(registers.orig_rax)>(-1LL);
    return ::ptrace(PTRACE_SETREGS, pid, nullptr, &registers) == 0;
}

bool replace_return(pid_t pid, long long value) {
    struct user_regs_struct registers {};
    if (::ptrace(PTRACE_GETREGS, pid, nullptr, &registers) != 0) return false;
    registers.rax = static_cast<decltype(registers.rax)>(value);
    return ::ptrace(PTRACE_SETREGS, pid, nullptr, &registers) == 0;
}

bool write_tracee(pid_t pid, std::uint64_t address,
                  const void * input, std::size_t size) {
    const auto * bytes = static_cast<const unsigned char *>(input);
    for (std::size_t offset = 0; offset < size; offset += sizeof(long)) {
        const std::size_t copied = std::min(sizeof(long), size - offset);
        long word = 0;
        if (copied != sizeof(long)) {
            errno = 0;
            word = ::ptrace(PTRACE_PEEKDATA, pid,
                reinterpret_cast<void *>(address + offset), nullptr);
            if (word == -1 && errno != 0) return false;
        }
        std::memcpy(&word, bytes + offset, copied);
        if (::ptrace(PTRACE_POKEDATA, pid,
                     reinterpret_cast<void *>(address + offset), word) != 0) return false;
    }
    return true;
}

bool exact_marker_child_argv(pid_t pid) {
    char path[64];
    const int size = std::snprintf(path, sizeof(path), "/proc/%ld/cmdline",
                                   static_cast<long>(pid));
    if (size <= 0 || static_cast<std::size_t>(size) >= sizeof(path)) return false;
    const int fd = ::open(path, O_RDONLY | O_CLOEXEC | O_NOFOLLOW);
    if (fd < 0) return false;
    std::array<char, 128> bytes {};
    ssize_t count;
    do { count = ::read(fd, bytes.data(), bytes.size()); } while (count < 0 && errno == EINTR);
    const bool closed = ::close(fd) == 0;
    constexpr char expected[] = "halofpx-l05y-live-child\0--live-marker-child\0";
    return closed && count == static_cast<ssize_t>(sizeof(expected) - 1) &&
           std::memcmp(bytes.data(), expected, sizeof(expected) - 1) == 0;
}

bool exact_fd(pid_t pid, std::uint64_t raw_fd, const struct stat & expected,
              std::uint64_t expected_mount) {
    struct stat observed {};
    std::uint64_t mount = 0;
    return tracee_fd_identity(pid, static_cast<int>(raw_fd), observed, mount) &&
           same_object(expected, observed) && mount == expected_mount;
}

bool read_how(pid_t pid, const tracee_state & state, struct open_how & how,
              std::string & name) {
    return state.nr == SYS_openat2 && state.args[3] == sizeof(how) &&
           read_tracee(pid, state.args[2], &how, sizeof(how)) &&
           read_tracee_string(pid, state.args[1], name);
}

bool matches_boundary(pid_t pid, const tracee_state & state,
                      const arguments & input, const struct stat & root,
                      std::uint64_t root_mount, const pinned_directory & staging,
                      const marker_state & marker, bool writer_pinned,
                      const struct stat & writer, const struct stat & fixture_lock,
                      std::uint64_t fixture_mount) {
    struct open_how how {};
    std::string name;
    const auto marker_fd = [&]() {
        return marker.marker_pinned && exact_fd(
            pid, state.args[0], marker.identity, marker.mount_id);
    };
    switch (input.point) {
        case boundary::transient_open:
            return exact_directory_fd(pid, static_cast<int>(state.args[0]), staging) &&
                   read_how(pid, state, how, name) && name == "initialize-root.tmp" &&
                   how.flags == static_cast<std::uint64_t>(
                       O_CREAT | O_EXCL | O_RDWR | O_CLOEXEC | O_NOFOLLOW) &&
                   how.mode == 0600 && how.resolve ==
                       static_cast<std::uint64_t>(RESOLVE_BENEATH | RESOLVE_NO_MAGICLINKS |
                                                  RESOLVE_NO_SYMLINKS | RESOLVE_NO_XDEV);
        case boundary::transient_fchmod:
            return state.nr == SYS_fchmod && state.args[1] == 0600 && marker_fd();
        case boundary::marker_pwrite:
            return state.nr == SYS_pwrite64 && state.args[3] <= 1024 &&
                   state.args[2] > 0 && state.args[2] <= 1024 - state.args[3] &&
                   marker_fd();
        case boundary::marker_pread:
            return state.nr == SYS_pread64 && state.args[3] == 0 &&
                   state.args[2] == marker.encoded_size && marker.encoded_size != 0 &&
                   marker_fd() && !marker.final_open_seen;
        case boundary::marker_pread_eof:
            return state.nr == SYS_pread64 && state.args[2] == 1 &&
                   state.args[3] == marker.encoded_size && marker.encoded_size != 0 &&
                   marker_fd() && !marker.final_open_seen;
        case boundary::marker_fsync:
            return state.nr == SYS_fsync && marker_fd();
        case boundary::readonly_temp_open:
            return marker.transient_seen && !marker.published_seen &&
                   exact_directory_fd(pid, static_cast<int>(state.args[0]), staging) &&
                   read_how(pid, state, how, name) &&
                   name == "initialize-root.tmp" &&
                   (how.flags & static_cast<std::uint64_t>(O_ACCMODE)) == O_RDONLY &&
                   (how.flags & static_cast<std::uint64_t>(O_CREAT | O_TRUNC)) == 0;
        case boundary::marker_rename: {
            std::string old_name, new_name;
            return state.nr == SYS_renameat2 && state.args[4] == RENAME_NOREPLACE &&
                   exact_directory_fd(pid, static_cast<int>(state.args[0]), staging) &&
                   exact_fd(pid, state.args[2], root, root_mount) &&
                   read_tracee_string(pid, state.args[1], old_name) &&
                   read_tracee_string(pid, state.args[3], new_name) &&
                   old_name == "initialize-root.tmp" && new_name == "root.marker";
        }
        case boundary::root_fsync:
            return marker.published_seen && state.nr == SYS_fsync &&
                   exact_fd(pid, state.args[0], root, root_mount);
        case boundary::staging_fsync:
            return marker.transient_seen && state.nr == SYS_fsync &&
                   exact_directory_fd(pid, static_cast<int>(state.args[0]), staging);
        case boundary::final_open:
            return marker.published_seen && exact_fd(pid, state.args[0], root, root_mount) &&
                   read_how(pid, state, how, name) && name == "root.marker" &&
                   (how.flags & static_cast<std::uint64_t>(O_ACCMODE)) == O_RDONLY &&
                   (how.flags & static_cast<std::uint64_t>(O_CREAT | O_TRUNC)) == 0;
        case boundary::final_pread:
            return marker.final_open_seen && state.nr == SYS_pread64 &&
                   state.args[3] == 0 && state.args[2] == marker.encoded_size && marker_fd();
        case boundary::final_pread_eof:
            return marker.final_open_seen && state.nr == SYS_pread64 &&
                   state.args[2] == 1 && state.args[3] == marker.encoded_size && marker_fd();
        case boundary::marker_close:
            return state.nr == SYS_close && marker_fd();
        case boundary::staging_close:
            return marker.published_seen && state.nr == SYS_close &&
                   exact_directory_fd(pid, static_cast<int>(state.args[0]), staging);
        case boundary::writer_unlock:
        case boundary::fixture_unlock: {
            if (state.nr != SYS_fcntl || state.args[1] != F_OFD_SETLK) return false;
            struct flock lock {};
            const bool exact_lock = input.point == boundary::writer_unlock
                ? writer_pinned && exact_fd(pid, state.args[0], writer, root_mount)
                : exact_fd(pid, state.args[0], fixture_lock, fixture_mount);
            return exact_lock && read_tracee(pid, state.args[2], &lock, sizeof(lock)) &&
                   lock.l_type == F_UNLCK && lock.l_whence == SEEK_SET &&
                   lock.l_start == 0 && lock.l_len == 0;
        }
        case boundary::reserve_revalidation:
            return marker.transient_seen && state.nr == SYS_fstatfs &&
                   exact_fd(pid, state.args[0], root, root_mount);
        case boundary::final_validation:
            return marker.published_seen && marker.final_open_seen &&
                   state.nr == SYS_pread64 && state.args[3] == 0 && marker_fd();
    }
    return false;
}

bool pin_marker_fd(pid_t pid, int fd, marker_state & marker) {
    struct stat observed {};
    std::uint64_t mount = 0;
    if (!tracee_fd_identity(pid, fd, observed, mount) ||
        !S_ISREG(observed.st_mode) || observed.st_nlink != 1 ||
        observed.st_uid != ::geteuid() ||
        ((observed.st_mode & 07777) != 0000 &&
         (observed.st_mode & 07777) != 0600)) return false;
    if (marker.marker_pinned && !same_object(marker.identity, observed)) return false;
    marker.marker_pinned = true;
    marker.identity = observed;
    marker.mount_id = mount;
    return true;
}

bool capture_pwrite_intent(pid_t pid, const tracee_state & state,
                           marker_state & marker) {
    if (state.nr != SYS_pwrite64 || !marker.marker_pinned ||
        !exact_fd(pid, state.args[0], marker.identity, marker.mount_id) ||
        state.args[3] > 1024 || state.args[2] == 0 ||
        state.args[2] > 1024 - state.args[3]) return true;
    const std::size_t offset = static_cast<std::size_t>(state.args[3]);
    const std::size_t count = static_cast<std::size_t>(state.args[2]);
    if (!read_tracee(pid, state.args[1], marker.intended_bytes.data() + offset,
                     count)) return false;
    std::fill_n(marker.intended_known.begin() + static_cast<std::ptrdiff_t>(offset),
                count, true);
    marker.encoded_size = std::max(marker.encoded_size, offset + count);
    return true;
}

bool capture_completed_pwrite(const tracee_state & state, long long result,
                              marker_state & marker) {
    if (state.nr != SYS_pwrite64 || result <= 0) return true;
    const std::size_t offset = static_cast<std::size_t>(state.args[3]);
    const std::size_t count = static_cast<std::size_t>(result);
    if (offset > 1024 || count > 1024 - offset ||
        !std::all_of(marker.intended_known.begin() +
                         static_cast<std::ptrdiff_t>(offset),
                     marker.intended_known.begin() +
                         static_cast<std::ptrdiff_t>(offset + count),
                     [](bool known) { return known; })) return false;
    std::copy_n(marker.intended_bytes.begin() +
                    static_cast<std::ptrdiff_t>(offset),
                count, marker.retained_bytes.begin() +
                           static_cast<std::ptrdiff_t>(offset));
    std::fill_n(marker.retained_known.begin() +
                    static_cast<std::ptrdiff_t>(offset),
                count, true);
    marker.retained_size = std::max(marker.retained_size, offset + count);
    return true;
}

bool exact_byte_coverage(const arguments & input, const marker_state & marker,
                         bool qualified) {
    if (marker.encoded_size == 0) {
        return marker.retained_size == 0 && !qualified;
    }
    if (marker.encoded_size > marker.intended_known.size() ||
        marker.retained_size > marker.retained_known.size() ||
        !std::all_of(marker.intended_known.begin(),
                     marker.intended_known.begin() +
                         static_cast<std::ptrdiff_t>(marker.encoded_size),
                     [](bool known) { return known; })) return false;
    if (input.injection == mode::short_pre &&
        input.point == boundary::marker_pwrite) {
        if (marker.retained_size != marker.encoded_size ||
            input.short_count >= marker.encoded_size) return false;
        for (std::size_t index = 0; index < marker.encoded_size; ++index) {
            const bool prefix_hole = index < input.short_count;
            if (marker.retained_known[index] == prefix_hole ||
                (prefix_hole && marker.retained_bytes[index] != 0)) return false;
        }
        return !qualified;
    }
    if (marker.retained_size != 0 &&
        !std::all_of(marker.retained_known.begin(),
                     marker.retained_known.begin() +
                         static_cast<std::ptrdiff_t>(marker.retained_size),
                     [](bool known) { return known; })) return false;
    if (qualified) {
        return marker.retained_size == marker.encoded_size;
    }
    if (input.injection == mode::inode_substitute ||
        input.injection == mode::temp_inode_substitute) {
        return marker.retained_size == marker.encoded_size;
    }
    return true;
}

bool apply_retained_mutation(const arguments & input, int root_fd, int staging_fd,
                             marker_state & marker) {
    if (!marker.published_seen || marker.encoded_size == 0) return false;
    if (input.injection == mode::unexpected_name) {
        struct open_how how {};
        how.flags = O_CREAT | O_EXCL | O_WRONLY | O_CLOEXEC | O_NOFOLLOW;
        how.mode = 0600;
        how.resolve = RESOLVE_BENEATH | RESOLVE_NO_MAGICLINKS |
                      RESOLVE_NO_SYMLINKS | RESOLVE_NO_XDEV;
        const int fd = static_cast<int>(::syscall(
            SYS_openat2, staging_fd, "unexpected.retained", &how, sizeof(how)));
        return fd >= 0 && ::fchmod(fd, 0600) == 0 && ::fsync(fd) == 0 &&
               ::close(fd) == 0;
    }
    if (input.injection == mode::hardlink) {
        return ::linkat(root_fd, "root.marker", staging_fd,
                        "root.marker.hardlink", 0) == 0;
    }
    if (input.injection == mode::symlink) {
        return ::symlinkat("../root.marker", staging_fd,
                           "root.marker.symlink") == 0;
    }
    if (input.injection == mode::inode_substitute) {
        if (::syscall(SYS_renameat2, root_fd, "root.marker", root_fd,
                      "root.marker.original", RENAME_NOREPLACE) != 0) return false;
        struct open_how how {};
        how.flags = O_CREAT | O_EXCL | O_WRONLY | O_CLOEXEC | O_NOFOLLOW;
        how.mode = 0600;
        how.resolve = RESOLVE_BENEATH | RESOLVE_NO_MAGICLINKS |
                      RESOLVE_NO_SYMLINKS | RESOLVE_NO_XDEV;
        const int replacement = static_cast<int>(::syscall(
            SYS_openat2, root_fd, "root.marker", &how, sizeof(how)));
        return replacement >= 0 && ::fchmod(replacement, 0600) == 0 &&
               ::fsync(replacement) == 0 && ::close(replacement) == 0;
    }
    int fd = -1;
    if (!open_named(root_fd, "root.marker", O_RDWR, fd)) return false;
    bool mutated = false;
    if (input.injection == mode::corrupt) {
        std::uint8_t byte = 0;
        mutated = ::pread(fd, &byte, 1, 0) == 1 &&
                  (++byte, ::pwrite(fd, &byte, 1, 0) == 1);
        if (mutated && marker.retained_size != 0 && marker.retained_known[0]) {
            ++marker.retained_bytes[0];
        }
    } else if (input.injection == mode::truncate) {
        mutated = marker.encoded_size > 1 &&
                  ::ftruncate(fd, static_cast<off_t>(marker.encoded_size - 1)) == 0;
        if (mutated) marker.retained_size = marker.encoded_size - 1;
    } else if (input.injection == mode::append) {
        const std::uint8_t trailing = 0xa5;
        mutated = ::pwrite(fd, &trailing, 1,
                           static_cast<off_t>(marker.encoded_size)) == 1;
        if (mutated) {
            marker.retained_bytes[marker.encoded_size] = trailing;
            marker.retained_known[marker.encoded_size] = true;
            marker.retained_size = marker.encoded_size + 1;
        }
    }
    const bool synchronized = mutated && ::fsync(fd) == 0;
    const bool closed = ::close(fd) == 0;
    return synchronized && closed;
}

bool apply_collision(const arguments & input, int root_fd, int staging_fd) {
    const int parent = input.point == boundary::transient_open ? staging_fd : root_fd;
    const char * name = input.point == boundary::transient_open
        ? "initialize-root.tmp" : "root.marker";
    struct open_how how {};
    how.flags = O_CREAT | O_EXCL | O_WRONLY | O_CLOEXEC | O_NOFOLLOW;
    how.mode = 0600;
    how.resolve = RESOLVE_BENEATH | RESOLVE_NO_MAGICLINKS |
                  RESOLVE_NO_SYMLINKS | RESOLVE_NO_XDEV;
    const int fd = static_cast<int>(::syscall(
        SYS_openat2, parent, name, &how, sizeof(how)));
    return fd >= 0 && ::fchmod(fd, 0600) == 0 && ::fsync(fd) == 0 &&
           ::close(fd) == 0;
}

bool apply_temp_inode_substitution(int staging_fd) {
    if (::syscall(SYS_renameat2, staging_fd, "initialize-root.tmp", staging_fd,
                  "initialize-root.original", RENAME_NOREPLACE) != 0) return false;
    struct open_how how {};
    how.flags = O_CREAT | O_EXCL | O_WRONLY | O_CLOEXEC | O_NOFOLLOW;
    how.mode = 0600;
    how.resolve = RESOLVE_BENEATH | RESOLVE_NO_MAGICLINKS |
                  RESOLVE_NO_SYMLINKS | RESOLVE_NO_XDEV;
    const int replacement = static_cast<int>(::syscall(
        SYS_openat2, staging_fd, "initialize-root.tmp", &how, sizeof(how)));
    return replacement >= 0 && ::fchmod(replacement, 0600) == 0 &&
           ::fsync(replacement) == 0 && ::close(replacement) == 0;
}

bool exact_audit(const std::string & audit, bool expect_qualified,
                 const arguments & input, const marker_state & observed_marker) {
    const boundary point = input.point;
    if (audit.empty() || audit.back() != '\n' ||
        audit.find('\n') != audit.size() - 1) return false;
    std::unordered_map<std::string, std::string> fields;
    std::istringstream stream(audit.substr(0, audit.size() - 1));
    std::string token;
    while (stream >> token) {
        const std::size_t equals = token.find('=');
        if (equals == std::string::npos || equals == 0 ||
            equals + 1 == token.size() || token.find('=', equals + 1) !=
                std::string::npos ||
            !fields.emplace(token.substr(0, equals), token.substr(equals + 1)).second) {
            return false;
        }
    }
    constexpr const char * required[] {
        "result", "sealed_result", "sealed_before_root", "latch", "qualified",
        "root_syscalls", "reserve", "root_id_nonzero", "store_uuid_nonzero",
        "writer_created", "writer_synced", "writer_ofd", "sole_entry",
        "writer_released", "fixture_released", "guard_released", "envelopes",
        "attempts", "staging", "final_dirs", "root_synced", "prefix_qualified",
        "path_policy_computed", "marker", "marker_sync", "marker_final",
        "marker_qualified", "root_id", "store_uuid", "path_policy",
        "marker_digest", "marker_dev", "marker_inode", "marker_mount",
        "marker_size", "marker_phase",
    };
    if (fields.size() != std::size(required)) return false;
    for (const char * key : required) if (fields.find(key) == fields.end()) return false;
    const auto exact = [&](const char * key, const char * value) {
        return fields.at(key) == value;
    };
    const bool cleanup_fault = !expect_qualified &&
        (point == boundary::writer_unlock || point == boundary::fixture_unlock);
    if (!exact("result", "0") || !exact("sealed_result", "0") ||
        !exact("sealed_before_root", "1") || !exact("latch", "1") ||
        !exact("root_id_nonzero", "1") || !exact("store_uuid_nonzero", "1") ||
        !exact("writer_created", "1") || !exact("writer_synced", "1") ||
        !exact("writer_ofd", "1") || !exact("sole_entry", "1") ||
        !exact("guard_released", "1") || !exact("envelopes", "1/1/1") ||
        !exact("attempts", "1/1/1") || !exact("staging", "1/1/1") ||
        !exact("final_dirs", "1/1/1") || !exact("root_synced", "1") ||
        !exact("path_policy_computed", "1") ||
        !exact("qualified", cleanup_fault ? "0" : "1") ||
        !exact("prefix_qualified", cleanup_fault ? "0" : "1") ||
        !exact("writer_released",
               point == boundary::writer_unlock && !expect_qualified ? "0" : "1") ||
        !exact("fixture_released",
               point == boundary::fixture_unlock && !expect_qualified ? "0" : "1") ||
        !exact("marker_qualified", expect_qualified ? "1" : "0")) return false;
    const auto parse_bits = [](const std::string & value, std::size_t count,
                               std::array<unsigned, 10> & output) {
        std::size_t begin = 0;
        for (std::size_t index = 0; index < count; ++index) {
            if (begin >= value.size() || (value[begin] != '0' && value[begin] != '1'))
                return false;
            output[index] = static_cast<unsigned>(value[begin] - '0');
            ++begin;
            if (index + 1 != count) {
                if (begin >= value.size() || value[begin] != '/') return false;
                ++begin;
            }
        }
        return begin == value.size();
    };
    std::array<unsigned, 10> marker_bits {}, sync_bits {}, final_bits {};
    if (!parse_bits(fields.at("marker"), 10, marker_bits) ||
        !parse_bits(fields.at("marker_sync"), 4, sync_bits) ||
        !parse_bits(fields.at("marker_final"), 2, final_bits)) return false;
    for (std::size_t index = 1; index < marker_bits.size(); ++index) {
        if (marker_bits[index] > marker_bits[index - 1]) return false;
    }
    if (marker_bits[0] != 1 || marker_bits[1] != 1 || marker_bits[2] != 1 ||
        (sync_bits[0] && !marker_bits[8]) ||
        (sync_bits[1] && !sync_bits[0]) ||
        (marker_bits[9] && !sync_bits[1]) ||
        (sync_bits[2] && !marker_bits[9]) ||
        (sync_bits[3] && !sync_bits[2]) ||
        (final_bits[0] && !sync_bits[3]) ||
        (final_bits[1] && !final_bits[0])) return false;
    unsigned expected_phase = 4;
    if (marker_bits[3]) expected_phase = 5;
    if (marker_bits[4]) expected_phase = 6;
    if (marker_bits[5]) expected_phase = 7;
    if (marker_bits[7]) expected_phase = 8;
    if (marker_bits[8]) expected_phase = 9;
    if (marker_bits[9]) expected_phase = 10;
    if (sync_bits[2]) expected_phase = 11;
    if (sync_bits[3]) expected_phase = 12;
    if (final_bits[0] && final_bits[1]) expected_phase = 13;
    unsigned boundary_phase = 0;
    if (expect_qualified) {
        boundary_phase = 13;
    } else {
        switch (point) {
            case boundary::transient_open: boundary_phase = 4; break;
            case boundary::transient_fchmod: boundary_phase = 5; break;
            case boundary::marker_pwrite:
                boundary_phase = input.injection == mode::short_pre ? 7 : 6;
                break;
            case boundary::marker_pread:
            case boundary::marker_pread_eof: boundary_phase = 7; break;
            case boundary::marker_fsync: boundary_phase = 8; break;
            case boundary::readonly_temp_open:
            case boundary::marker_rename: boundary_phase = 9; break;
            case boundary::root_fsync: boundary_phase = 10; break;
            case boundary::staging_fsync: boundary_phase = 11; break;
            case boundary::final_open:
            case boundary::final_pread:
            case boundary::final_pread_eof:
            case boundary::final_validation: boundary_phase = 12; break;
            case boundary::marker_close:
            case boundary::writer_unlock:
            case boundary::fixture_unlock: boundary_phase = 13; break;
            case boundary::staging_close: boundary_phase = 12; break;
            case boundary::reserve_revalidation:
                boundary_phase = observed_marker.published_seen ? 12 : 9;
                break;
        }
    }
    unsigned marker_size = 0, marker_phase = 0, root_syscalls = 0;
    std::uint64_t reserve = 0;
    const auto parse_number = [](const std::string & value, std::uint64_t maximum,
                                 std::uint64_t & output) {
        if (value.empty() || value[0] == '-') return false;
        errno = 0;
        char * end = nullptr;
        const unsigned long long parsed = std::strtoull(value.c_str(), &end, 10);
        if (errno != 0 || end == value.c_str() || *end != '\0' || parsed > maximum)
            return false;
        output = static_cast<std::uint64_t>(parsed);
        return true;
    };
    std::uint64_t parsed = 0;
    if (!parse_number(fields.at("marker_size"), 1024, parsed) || parsed == 0) return false;
    marker_size = static_cast<unsigned>(parsed);
    if (!parse_number(fields.at("marker_phase"), 13, parsed)) return false;
    marker_phase = static_cast<unsigned>(parsed);
    if (!parse_number(fields.at("root_syscalls"), UINT32_MAX, parsed) || parsed == 0)
        return false;
    root_syscalls = static_cast<unsigned>(parsed);
    if (!parse_number(fields.at("reserve"), UINT64_MAX, reserve) ||
        marker_phase != expected_phase || marker_phase != boundary_phase ||
        marker_size == 0 || root_syscalls == 0) return false;
    (void) reserve;
    const auto exact_hex = [](const std::string & value, std::size_t length) {
        return value.size() == length &&
            std::all_of(value.begin(), value.end(), [](char byte) {
                return (byte >= '0' && byte <= '9') || (byte >= 'a' && byte <= 'f');
            }) && value.find_first_not_of('0') != std::string::npos;
    };
    if (!exact_hex(fields.at("root_id"), 64) ||
        !exact_hex(fields.at("store_uuid"), 32) ||
        !exact_hex(fields.at("path_policy"), 64) ||
        !exact_hex(fields.at("marker_digest"), 64)) return false;
    std::uint64_t marker_device = 0, marker_inode = 0, marker_mount = 0;
    if (!parse_number(fields.at("marker_dev"), UINT64_MAX, marker_device) ||
        !parse_number(fields.at("marker_inode"), UINT64_MAX, marker_inode) ||
        !parse_number(fields.at("marker_mount"), UINT64_MAX, marker_mount)) return false;
    const bool final_identity = marker_device != 0 && marker_inode != 0 &&
        marker_mount != 0;
    if (final_identity != (final_bits[0] == 1 && final_bits[1] == 1)) return false;
    std::array<unsigned, 10> expected_marker {};
    expected_marker[0] = expected_marker[1] = expected_marker[2] = 1;
    if (boundary_phase >= 5) expected_marker[3] = 1;
    if (boundary_phase >= 6) expected_marker[4] = 1;
    if (boundary_phase >= 7) expected_marker[5] = 1;
    if (boundary_phase >= 8) expected_marker[6] = expected_marker[7] = 1;
    if (boundary_phase >= 9) expected_marker[8] = 1;
    if (boundary_phase >= 10) expected_marker[9] = 1;
    std::array<unsigned, 10> expected_sync {};
    if (boundary_phase >= 10 || point == boundary::marker_rename) {
        expected_sync[0] = expected_sync[1] = 1;
    }
    if (boundary_phase >= 11) expected_sync[2] = 1;
    if (boundary_phase >= 12) expected_sync[3] = 1;
    std::array<unsigned, 10> expected_final {};
    if (boundary_phase >= 13) expected_final[0] = expected_final[1] = 1;
    if (marker_bits != expected_marker ||
        !std::equal(sync_bits.begin(), sync_bits.begin() + 4,
                    expected_sync.begin()) ||
        final_bits[0] != expected_final[0] ||
        final_bits[1] != expected_final[1]) return false;
    if (observed_marker.encoded_size != 0 &&
        marker_size != observed_marker.encoded_size) return false;
    if (final_identity &&
        (marker_device != static_cast<std::uint64_t>(observed_marker.identity.st_dev) ||
         marker_inode != static_cast<std::uint64_t>(observed_marker.identity.st_ino) ||
         marker_mount != observed_marker.mount_id)) return false;
    if (expect_qualified) {
        return std::all_of(marker_bits.begin(), marker_bits.end(),
                           [](unsigned bit) { return bit == 1; }) &&
               std::all_of(sync_bits.begin(), sync_bits.begin() + 4,
                           [](unsigned bit) { return bit == 1; }) &&
               final_bits[0] == 1 && final_bits[1] == 1 && marker_phase == 13;
    }
    return true;
}

bool list_names(int fd, std::vector<std::string> & names) {
    const int duplicate = ::openat(
        fd, ".", O_RDONLY | O_DIRECTORY | O_CLOEXEC | O_NOFOLLOW);
    if (duplicate < 0) return false;
    DIR * stream = ::fdopendir(duplicate);
    if (!stream) { (void) ::close(duplicate); return false; }
    names.clear();
    errno = 0;
    for (;;) {
        dirent * entry = ::readdir(stream);
        if (!entry) break;
        if (std::strcmp(entry->d_name, ".") != 0 &&
            std::strcmp(entry->d_name, "..") != 0) names.emplace_back(entry->d_name);
        errno = 0;
    }
    const bool ok = errno == 0;
    const bool closed = ::closedir(stream) == 0;
    std::sort(names.begin(), names.end());
    return ok && closed;
}

bool exact_file_bytes(int fd, const marker_state & marker,
                      std::size_t expected_size) {
    if (expected_size > marker.retained_bytes.size()) return false;
    std::array<std::uint8_t, 1025> observed {};
    std::size_t total = 0;
    while (total < expected_size) {
        ssize_t count;
        do {
            count = ::pread(fd, observed.data() + total, expected_size - total,
                            static_cast<off_t>(total));
        } while (count < 0 && errno == EINTR);
        if (count <= 0) return false;
        total += static_cast<std::size_t>(count);
    }
    std::uint8_t trailing = 0;
    ssize_t trailing_count;
    do {
        trailing_count = ::pread(fd, &trailing, 1,
                                 static_cast<off_t>(expected_size));
    } while (trailing_count < 0 && errno == EINTR);
    return trailing_count == 0 &&
           std::memcmp(observed.data(), marker.retained_bytes.data(),
                       expected_size) == 0;
}

bool exact_inventory(int root_fd, const marker_state & marker,
                     const arguments & input, const struct stat & writer,
                     bool writer_pinned,
                     const std::array<pinned_directory, 3> & directories,
                     bool & whole_root_discard) {
    int staging_fd = -1;
    if (!open_named(root_fd, "staging", O_RDONLY | O_DIRECTORY, staging_fd)) {
        return false;
    }
    if (staging_fd < 0) return false;
    std::vector<std::string> root_names, staging_names;
    const bool listed = list_names(root_fd, root_names) && list_names(staging_fd, staging_names);
    std::vector<std::string> expected_root {
        "attempts", "envelopes", "staging", "writer.lock"
    };
    std::vector<std::string> expected_staging;
    if (marker.published_seen) expected_root.emplace_back("root.marker");
    else if (marker.transient_seen) expected_staging.emplace_back("initialize-root.tmp");
    if (input.injection == mode::collision &&
        input.point == boundary::marker_rename) expected_root.emplace_back("root.marker");
    if (input.injection == mode::unexpected_name)
        expected_staging.emplace_back("unexpected.retained");
    if (input.injection == mode::hardlink)
        expected_staging.emplace_back("root.marker.hardlink");
    if (input.injection == mode::symlink)
        expected_staging.emplace_back("root.marker.symlink");
    if (input.injection == mode::inode_substitute)
        expected_root.emplace_back("root.marker.original");
    if (input.injection == mode::temp_inode_substitute)
        expected_staging.emplace_back("initialize-root.original");
    std::sort(expected_root.begin(), expected_root.end());
    std::sort(expected_staging.begin(), expected_staging.end());
    int writer_fd = -1;
    struct stat observed_writer {};
    std::uint64_t writer_mount = 0, root_mount = 0;
    const bool writer_shape = writer_pinned &&
        fd_mount_id(root_fd, root_mount) &&
        open_named(root_fd, "writer.lock", O_RDONLY, writer_fd) &&
        ::fstat(writer_fd, &observed_writer) == 0 &&
        fd_mount_id(writer_fd, writer_mount) &&
        same_object(writer, observed_writer) && writer_mount == root_mount &&
        observed_writer.st_nlink == 1 &&
        observed_writer.st_size == 0 && observed_writer.st_uid == ::geteuid() &&
        (observed_writer.st_mode & 07777) == 0600;
    const bool writer_closed = writer_fd >= 0 && ::close(writer_fd) == 0;
    bool directory_shapes = true;
    std::array<bool, 3> directory_shape_bits {};
    for (int index = 0; index < 3; ++index) {
        int fd = -1;
        struct stat observed {};
        std::uint64_t mount = 0;
        const auto & expected = directories[static_cast<std::size_t>(index)];
        const bool opened = open_named(
            root_fd, directory_names[static_cast<std::size_t>(index)],
            O_RDONLY | O_DIRECTORY, fd);
        bool shape = expected.pinned && opened &&
            ::fstat(fd, &observed) == 0 && fd_mount_id(fd, mount) &&
            S_ISDIR(observed.st_mode) && same_object(expected.identity, observed) &&
            mount == expected.mount_id && mount == root_mount &&
            observed.st_uid == ::geteuid() &&
            (observed.st_mode & 07777) == 0700 &&
            (index == 2 || directory_empty(fd));
        const bool closed = fd >= 0 && ::close(fd) == 0;
        directory_shape_bits[static_cast<std::size_t>(index)] = shape && closed;
        directory_shapes = directory_shapes && shape && closed;
    }
    bool extra_shape = true;
    if (input.injection == mode::unexpected_name) {
        struct stat extra {};
        extra_shape = ::fstatat(staging_fd, "unexpected.retained", &extra,
                                AT_SYMLINK_NOFOLLOW) == 0 &&
            S_ISREG(extra.st_mode) && extra.st_nlink == 1 && extra.st_size == 0 &&
            extra.st_uid == ::geteuid() && (extra.st_mode & 07777) == 0600;
    } else if (input.injection == mode::hardlink) {
        struct stat extra {};
        extra_shape = ::fstatat(staging_fd, "root.marker.hardlink", &extra,
                                AT_SYMLINK_NOFOLLOW) == 0 &&
            S_ISREG(extra.st_mode) && same_object(marker.identity, extra) &&
            extra.st_nlink == 2;
    } else if (input.injection == mode::symlink) {
        struct stat extra {};
        std::array<char, 64> target {};
        constexpr char expected_target[] = "../root.marker";
        extra_shape = ::fstatat(staging_fd, "root.marker.symlink", &extra,
                                AT_SYMLINK_NOFOLLOW) == 0 && S_ISLNK(extra.st_mode) &&
            ::readlinkat(staging_fd, "root.marker.symlink", target.data(),
                         target.size()) ==
                static_cast<ssize_t>(sizeof(expected_target) - 1) &&
            std::memcmp(target.data(), expected_target,
                        sizeof(expected_target) - 1) == 0;
    }
    bool marker_shape = true;
    if (marker.transient_seen || marker.published_seen) {
        const char * name = marker.published_seen ? "root.marker" : "initialize-root.tmp";
        const int parent = marker.published_seen ? root_fd : staging_fd;
        int fd = -1;
        struct stat observed {};
        std::uint64_t mount = 0;
        mode_t expected_mode = 0600;
        if (!marker.published_seen &&
            ((input.point == boundary::transient_open &&
              input.injection == mode::late_error) ||
             (input.point == boundary::transient_fchmod &&
              (input.injection == mode::pre_error ||
               input.injection == mode::eintr_once)))) expected_mode = 0000;
        std::size_t expected_size = marker.retained_size;
        if (input.injection == mode::inode_substitute ||
            input.injection == mode::temp_inode_substitute) expected_size = 0;
        const nlink_t expected_links = input.injection == mode::hardlink ? 2 : 1;
        marker_shape = open_named(parent, name, O_RDONLY, fd) &&
            ::fstat(fd, &observed) == 0 && fd_mount_id(fd, mount) &&
            S_ISREG(observed.st_mode) && observed.st_uid == ::geteuid() &&
            observed.st_nlink == expected_links &&
            (observed.st_mode & 07777) == expected_mode &&
            observed.st_size == static_cast<off_t>(expected_size) &&
            exact_file_bytes(fd, marker, expected_size) &&
            ((input.injection == mode::inode_substitute ||
              input.injection == mode::temp_inode_substitute ||
              (input.injection == mode::collision &&
               input.point == boundary::transient_open)) ||
             (same_object(marker.identity, observed) && mount == marker.mount_id));
        if (fd >= 0) marker_shape = (::close(fd) == 0) && marker_shape;
        if (input.injection == mode::inode_substitute) {
            int original_fd = -1;
            struct stat original {};
            std::uint64_t original_mount = 0;
            const bool original_ok = open_named(
                    root_fd, "root.marker.original", O_RDONLY, original_fd) &&
                ::fstat(original_fd, &original) == 0 &&
                fd_mount_id(original_fd, original_mount) &&
                same_object(marker.identity, original) &&
                original_mount == marker.mount_id &&
                original.st_size == static_cast<off_t>(marker.encoded_size) &&
                exact_file_bytes(original_fd, marker, marker.retained_size) &&
                !same_object(original, observed);
            if (original_fd >= 0) marker_shape =
                (::close(original_fd) == 0) && original_ok && marker_shape;
            else marker_shape = false;
        }
        if (input.injection == mode::temp_inode_substitute) {
            int original_fd = -1;
            struct stat original {};
            std::uint64_t original_mount = 0;
            const bool original_ok = open_named(
                    staging_fd, "initialize-root.original", O_RDONLY, original_fd) &&
                ::fstat(original_fd, &original) == 0 &&
                fd_mount_id(original_fd, original_mount) &&
                same_object(marker.identity, original) &&
                original_mount == marker.mount_id &&
                original.st_size == static_cast<off_t>(marker.retained_size) &&
                exact_file_bytes(original_fd, marker, marker.retained_size) &&
                !same_object(original, observed);
            if (original_fd >= 0) marker_shape =
                (::close(original_fd) == 0) && original_ok && marker_shape;
            else marker_shape = false;
        }
        if (input.injection == mode::collision &&
            input.point == boundary::marker_rename) {
            int collision_fd = -1;
            struct stat collision {};
            std::uint64_t collision_mount = 0;
            const bool collision_ok = open_named(
                    root_fd, "root.marker", O_RDONLY, collision_fd) &&
                ::fstat(collision_fd, &collision) == 0 &&
                fd_mount_id(collision_fd, collision_mount) &&
                S_ISREG(collision.st_mode) && collision.st_nlink == 1 &&
                collision.st_size == 0 && collision.st_uid == ::geteuid() &&
                (collision.st_mode & 07777) == 0600 &&
                collision_mount == marker.mount_id &&
                !same_object(marker.identity, collision);
            if (collision_fd >= 0) marker_shape =
                (::close(collision_fd) == 0) && collision_ok && marker_shape;
            else marker_shape = false;
        }
    }
    const bool closed = ::close(staging_fd) == 0;
    whole_root_discard = listed && writer_shape && writer_closed &&
        directory_shapes && extra_shape && root_names == expected_root &&
        staging_names == expected_staging && marker_shape;
    if (!whole_root_discard) {
        (void) emit(std::string("\"event\":\"inventory-detail\",\"listed\":") +
            (listed ? "true" : "false") + ",\"writer_shape\":" +
            (writer_shape ? "true" : "false") + ",\"writer_closed\":" +
            (writer_closed ? "true" : "false") + ",\"directory_shapes\":" +
            (directory_shapes ? "true" : "false") + ",\"extra_shape\":" +
            (extra_shape ? "true" : "false") + ",\"directory_shape_bits\":\"" +
            (directory_shape_bits[0] ? "1" : "0") + "/" +
            (directory_shape_bits[1] ? "1" : "0") + "/" +
            (directory_shape_bits[2] ? "1" : "0") + "\",\"extra_shape_repeat\":" +
            (extra_shape ? "true" : "false") + ",\"root_names\":" +
            (root_names == expected_root ? "true" : "false") +
            ",\"root_name_count\":" + std::to_string(root_names.size()) +
            ",\"expected_root_name_count\":" +
            std::to_string(expected_root.size()) +
            ",\"staging_names\":" +
            (staging_names == expected_staging ? "true" : "false") +
            ",\"marker_shape\":" + (marker_shape ? "true" : "false"));
    }
    return whole_root_discard && closed;
}

bool open_receipt(const arguments & input, int & parent_fd,
                  struct stat & parent_identity, std::uint64_t & parent_mount,
                  std::string & name) {
    std::string parent_path;
    if (!split_receipt_path(input.receipt, parent_path, name) ||
        !canonical_existing_path(parent_path.c_str()) ||
        same_or_descendant(parent_path.c_str(), input.parent) ||
        same_or_descendant(input.parent, parent_path.c_str())) return false;
    parent_fd = open_absolute_directory_no_symlink(parent_path.c_str());
    if (parent_fd < 0 || ::fstat(parent_fd, &parent_identity) != 0 ||
        !S_ISDIR(parent_identity.st_mode) || parent_identity.st_uid != ::geteuid() ||
        (parent_identity.st_mode & 07777) != 0700 ||
        !fd_mount_id(parent_fd, parent_mount)) return false;
    struct open_how how {};
    how.flags = O_WRONLY | O_CREAT | O_EXCL | O_CLOEXEC | O_NOFOLLOW;
    how.mode = 0600;
    how.resolve = RESOLVE_BENEATH | RESOLVE_NO_MAGICLINKS |
                  RESOLVE_NO_SYMLINKS | RESOLVE_NO_XDEV;
    receipt_fd = static_cast<int>(::syscall(
        SYS_openat2, parent_fd, name.c_str(), &how, sizeof(how)));
    struct stat receipt {};
    return receipt_fd >= 0 && ::fchmod(receipt_fd, 0600) == 0 &&
           ::fstat(receipt_fd, &receipt) == 0 && S_ISREG(receipt.st_mode) &&
           receipt.st_nlink == 1 && receipt.st_size == 0 &&
           receipt.st_uid == ::geteuid() && (receipt.st_mode & 07777) == 0600;
}

int run(const arguments & input) {
    if (!retry_window_self_check()) return 2;
    if (!canonical_existing_path(input.target) || !canonical_existing_path(input.golden) ||
        !canonical_existing_path(input.parent) || !canonical_existing_path(input.root) ||
        !canonical_existing_path(input.fixture) || !direct_child(input.root, input.parent) ||
        !direct_child(input.fixture, input.parent)) {
        std::fprintf(stderr, "non-canonical authority scope\n");
        return 2;
    }
    receipt_ok = true;
    receipt_sequence = 0;
    int receipt_parent_fd = -1;
    struct stat receipt_parent_identity {};
    std::uint64_t receipt_parent_mount = 0;
    std::string receipt_name;
    if (!open_receipt(input, receipt_parent_fd, receipt_parent_identity,
                      receipt_parent_mount, receipt_name)) return 2;

    const int target_fd = ::open(input.target, O_RDONLY | O_CLOEXEC | O_NOFOLLOW);
    const int parent_fd = ::open(input.parent, O_RDONLY | O_DIRECTORY | O_CLOEXEC | O_NOFOLLOW);
    const int root_fd = ::open(input.root, O_RDONLY | O_DIRECTORY | O_CLOEXEC | O_NOFOLLOW);
    const int fixture_fd = ::open(input.fixture, O_RDONLY | O_DIRECTORY | O_CLOEXEC | O_NOFOLLOW);
    int staging_fd = -1;
    const int fixture_lock_fd = fixture_fd >= 0
        ? ::openat(fixture_fd, "primitive.lock", O_RDONLY | O_CLOEXEC | O_NOFOLLOW) : -1;
    struct stat target_identity {}, parent_identity {}, root_identity {}, fixture_identity {};
    struct stat fixture_lock_identity {};
    std::uint64_t parent_mount = 0, root_mount = 0, fixture_mount = 0;
    if (target_fd < 0 || ::fstat(target_fd, &target_identity) != 0 ||
        !S_ISREG(target_identity.st_mode) || (target_identity.st_mode & 0111) == 0 ||
        parent_fd < 0 || ::fstat(parent_fd, &parent_identity) != 0 ||
        !S_ISDIR(parent_identity.st_mode) || parent_identity.st_uid != ::geteuid() ||
        (parent_identity.st_mode & 07777) != 0700 ||
        !fd_mount_id(parent_fd, parent_mount) || root_fd < 0 ||
        ::fstat(root_fd, &root_identity) != 0 || !fd_mount_id(root_fd, root_mount) ||
        !S_ISDIR(root_identity.st_mode) || root_identity.st_uid != ::geteuid() ||
        (root_identity.st_mode & 07777) != 0700 ||
        fixture_fd < 0 || ::fstat(fixture_fd, &fixture_identity) != 0 ||
        !S_ISDIR(fixture_identity.st_mode) || fixture_identity.st_uid != ::geteuid() ||
        (fixture_identity.st_mode & 07777) != 0700 ||
        !fd_mount_id(fixture_fd, fixture_mount) || fixture_lock_fd < 0 ||
        ::fstat(fixture_lock_fd, &fixture_lock_identity) != 0 ||
        !S_ISREG(fixture_lock_identity.st_mode) || fixture_lock_identity.st_nlink != 1 ||
        fixture_lock_identity.st_size != 0 || fixture_lock_identity.st_uid != ::geteuid() ||
        (fixture_lock_identity.st_mode & 07777) != 0600) return 2;
    (void) ::close(fixture_lock_fd);
    pinned_directory staging {};
    std::array<pinned_directory, 3> directories {};
    if (!emit(std::string("\"event\":\"start\",\"boundary\":\"") +
              boundary_name(input.point) + "\",\"mode\":\"" +
              mode_name(input.injection) + "\",\"occurrence\":" +
              std::to_string(input.occurrence))) return 2;

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
        char executable[64];
        const int size = std::snprintf(executable, sizeof(executable),
                                       "/proc/self/fd/%d", target_fd);
        if (size <= 0 || static_cast<std::size_t>(size) >= sizeof(executable)) _exit(125);
        ::execl(executable, input.target, "--live-marker-controller",
                input.golden, input.parent, input.root, input.fixture,
                static_cast<char *>(nullptr));
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
    std::unordered_map<pid_t, bool> selected;
    tracees.emplace(launcher, tracee_state {});
    selected.emplace(launcher, false);
    fault_state fault {};
    marker_state marker {};
    pid_t live_child = -1;
    int launcher_exit = -1, child_exit = -1;
    bool launcher_exec = false, child_exec = false, controller_error = false;
    bool bounded_cleanup = false, writer_pinned = false;
    bool cleanup_syscall_seen = false;
    unsigned publication_attempts = 0;
    struct stat writer_identity {};
    const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(30);

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
            if (pid == launcher) launcher_exit = code;
            if (pid == live_child) child_exit = code;
            tracees.erase(pid);
            selected.erase(pid);
            continue;
        }
        if (!WIFSTOPPED(status)) continue;
        const int signal = WSTOPSIG(status);
        const unsigned event = static_cast<unsigned>(status) >> 16;
        if (signal == SIGTRAP && event != 0) {
            if (event == PTRACE_EVENT_FORK || event == PTRACE_EVENT_VFORK ||
                event == PTRACE_EVENT_CLONE) {
                unsigned long child_value = 0;
                if (::ptrace(PTRACE_GETEVENTMSG, pid, nullptr, &child_value) != 0) {
                    controller_error = true;
                    break;
                }
                tracees[static_cast<pid_t>(child_value)].initial_sigstop_expected = true;
                selected[static_cast<pid_t>(child_value)] = false;
            } else if (event == PTRACE_EVENT_EXEC && pid == launcher) {
                launcher_exec = !launcher_exec && same_executable(pid, target_identity);
                if (!launcher_exec) { controller_error = true; break; }
            } else if (event == PTRACE_EVENT_EXEC) {
                if (live_child != -1 || !same_executable(pid, target_identity) ||
                    !exact_marker_child_argv(pid)) { controller_error = true; break; }
                live_child = pid;
                child_exec = true;
                tracees[pid].live_child = true;
            }
            if (!resume_syscalls(pid)) controller_error = true;
            continue;
        }
        if (signal == (SIGTRAP | 0x80)) {
            struct __ptrace_syscall_info info {};
            const long size = ::ptrace(PTRACE_GET_SYSCALL_INFO, pid, sizeof(info), &info);
            if (size < 0 || info.arch != AUDIT_ARCH_X86_64) {
                controller_error = true;
                break;
            }
            auto & state = tracees[pid];
            const bool retryable_eintr = returned_eintr_must_retry(input);
            if (info.op == PTRACE_SYSCALL_INFO_ENTRY) {
                state.have_entry = true;
                state.nr = info.entry.nr;
                std::memcpy(state.args, info.entry.args, sizeof(state.args));
                if (state.live_child && writer_pinned) {
                    const bool forbidden_cleanup = state.nr == SYS_unlinkat ||
                        state.nr == SYS_unlink || state.nr == SYS_rmdir ||
                        state.nr == SYS_truncate || state.nr == SYS_ftruncate ||
                        state.nr == SYS_rename ||
                        state.nr == SYS_renameat || state.nr == SYS_linkat ||
                        state.nr == SYS_symlinkat || state.nr == SYS_mount ||
                        state.nr == SYS_umount2;
                    if (forbidden_cleanup) {
                        cleanup_syscall_seen = true;
                        controller_error = true;
                        break;
                    }
#if defined(SYS_move_mount)
                    if (state.nr == SYS_move_mount) {
                        cleanup_syscall_seen = true;
                        controller_error = true;
                        break;
                    }
#endif
                    bool truncating_open = false;
                    if (state.nr == SYS_open) {
                        truncating_open = (state.args[1] & O_TRUNC) != 0;
                    } else if (state.nr == SYS_openat) {
                        truncating_open = (state.args[2] & O_TRUNC) != 0;
                    } else if (state.nr == SYS_openat2) {
                        struct open_how observed_how {};
                        truncating_open = state.args[3] == sizeof(observed_how) &&
                            read_tracee(pid, state.args[2], &observed_how,
                                        sizeof(observed_how)) &&
                            (observed_how.flags & O_TRUNC) != 0;
                    }
                    if (truncating_open) {
                        cleanup_syscall_seen = true;
                        controller_error = true;
                        break;
                    }
                    if (state.nr == SYS_renameat2) {
                        std::string old_name, new_name;
                        const bool publication = state.args[4] == RENAME_NOREPLACE &&
                            staging.pinned &&
                            exact_directory_fd(pid, static_cast<int>(state.args[0]),
                                               staging) &&
                            exact_fd(pid, state.args[2], root_identity, root_mount) &&
                            read_tracee_string(pid, state.args[1], old_name) &&
                            read_tracee_string(pid, state.args[3], new_name) &&
                            old_name == "initialize-root.tmp" &&
                            new_name == "root.marker";
                        if (!publication) {
                            cleanup_syscall_seen = true;
                            controller_error = true;
                            break;
                        }
                        ++publication_attempts;
                        if (publication_attempts != 1) {
                            cleanup_syscall_seen = true;
                            controller_error = true;
                            break;
                        }
                    }
                }
                if (state.live_child && !capture_pwrite_intent(pid, state, marker)) {
                    controller_error = true;
                    break;
                }
                bool match = false;
                if (state.live_child && !fault.pending_replacement) {
                    const bool exact_boundary = matches_boundary(
                        pid, state, input, root_identity, root_mount, staging,
                        marker, writer_pinned, writer_identity,
                        fixture_lock_identity, fixture_mount);
                    if (!fault.first_replaced) {
                        if (exact_boundary) {
                            ++fault.matches;
                            match = fault.matches == input.occurrence;
                        }
                    } else if (retryable_eintr) {
                        const retry_window_result observed = observe_retry_window(
                            fault, exact_boundary, input.occurrence);
                        if (observed == retry_window_result::excessive) {
                            controller_error = true;
                            break;
                        }
                        match = observed == retry_window_result::selected;
                    }
                }
                selected[pid] = match;
                if (match &&
                    (input.injection == mode::short_pre ||
                     input.injection == mode::short_late) &&
                    input.short_count >= state.args[2]) {
                    controller_error = true;
                    break;
                }
                const bool skip = match &&
                    ((input.injection == mode::pre_error &&
                      fault.matches == input.occurrence) ||
                     input.injection == mode::short_pre ||
                     input.injection == mode::zero_pre ||
                     (input.injection == mode::eintr_once &&
                      fault.matches == input.occurrence));
                if (skip) {
                    if (!replace_entry_with_enosys(pid)) { controller_error = true; break; }
                    fault.pending_replacement = true;
                } else if (match && retryable_eintr && fault.first_replaced) {
                    fault.retry_seen = true;
                }
                if (match && input.point == boundary::final_validation &&
                    !fault.mutation_applied) {
                    if (input.injection == mode::reserve_loss) {
                        controller_error = true;
                        break;
                    }
                    if (!apply_retained_mutation(input, root_fd, staging_fd, marker)) {
                        controller_error = true;
                        break;
                    }
                    fault.mutation_applied = true;
                }
                if (match && input.injection == mode::collision &&
                    !fault.mutation_applied) {
                    if (!apply_collision(input, root_fd, staging_fd)) {
                        controller_error = true;
                        break;
                    }
                    if (input.point == boundary::transient_open) {
                        marker.transient_seen = true;
                    }
                    fault.mutation_applied = true;
                }
                if (match && input.injection == mode::temp_inode_substitute &&
                    !fault.mutation_applied) {
                    if (!apply_temp_inode_substitution(staging_fd)) {
                        controller_error = true;
                        break;
                    }
                    fault.mutation_applied = true;
                }
            } else if (info.op == PTRACE_SYSCALL_INFO_EXIT) {
                if (state.live_child && state.have_entry) {
                    if (state.nr == SYS_openat2 &&
                        writer_openat2_entry(pid, state, root_identity, root_mount) &&
                        !info.exit.is_error && info.exit.rval >= 0) {
                        std::uint64_t writer_mount = 0;
                        if (!tracee_fd_identity(pid, static_cast<int>(info.exit.rval),
                                               writer_identity, writer_mount) ||
                            writer_mount != root_mount) { controller_error = true; break; }
                        writer_pinned = true;
                    }
                    if (state.nr == SYS_mkdirat && !info.exit.is_error &&
                        info.exit.rval == 0) {
                        std::string created_name;
                        if (!read_tracee_string(pid, state.args[1], created_name)) {
                            controller_error = true;
                            break;
                        }
                        for (int directory = 0; directory < 3; ++directory) {
                            if (created_name == directory_names[
                                    static_cast<std::size_t>(directory)]) {
                                pinned_directory observed {};
                                if (!pin_created_directory(root_fd, directory, 0000,
                                                           observed)) {
                                    controller_error = true;
                                    break;
                                }
                                directories[static_cast<std::size_t>(directory)] = observed;
                                if (directory == 2) {
                                    staging = observed;
                                    if (!open_named(root_fd, "staging",
                                                    O_RDONLY | O_DIRECTORY,
                                                    staging_fd)) {
                                        controller_error = true;
                                        break;
                                    }
                                }
                            }
                        }
                        if (controller_error) break;
                    }
                    struct open_how how {};
                    std::string name;
                    if (read_how(pid, state, how, name) &&
                        (name == "initialize-root.tmp" || name == "root.marker") &&
                        !info.exit.is_error && info.exit.rval >= 0) {
                        const bool substituted_temp =
                            input.injection == mode::temp_inode_substitute &&
                            input.point == boundary::readonly_temp_open &&
                            fault.mutation_applied && selected[pid] &&
                            name == "initialize-root.tmp";
                        if (substituted_temp) {
                            struct stat replacement {};
                            std::uint64_t replacement_mount = 0;
                            if (!tracee_fd_identity(
                                    pid, static_cast<int>(info.exit.rval),
                                    replacement, replacement_mount) ||
                                !S_ISREG(replacement.st_mode) ||
                                replacement.st_nlink != 1 || replacement.st_size != 0 ||
                                replacement.st_uid != ::geteuid() ||
                                (replacement.st_mode & 07777) != 0600 ||
                                replacement_mount != marker.mount_id ||
                                same_object(marker.identity, replacement)) {
                                controller_error = true;
                                break;
                            }
                            marker.temp_substitute_open_seen = true;
                        } else {
                            if (!pin_marker_fd(
                                    pid, static_cast<int>(info.exit.rval), marker)) {
                                controller_error = true;
                                break;
                            }
                            if (name == "initialize-root.tmp") marker.transient_seen = true;
                            else marker.final_open_seen = true;
                        }
                    }
                    if (state.nr == SYS_pwrite64 && !info.exit.is_error &&
                        !capture_completed_pwrite(state, info.exit.rval, marker)) {
                        controller_error = true;
                        break;
                    }
                    if (state.nr == SYS_renameat2 && !info.exit.is_error && info.exit.rval == 0) {
                        std::string old_name, new_name;
                        if (read_tracee_string(pid, state.args[1], old_name) &&
                            read_tracee_string(pid, state.args[3], new_name) &&
                            old_name == "initialize-root.tmp" && new_name == "root.marker") {
                            marker.published_seen = true;
                        }
                    }
                    if (fault.pending_replacement) {
                        long long replacement = 0;
                        if (input.injection == mode::pre_error ||
                            input.injection == mode::eintr_once)
                            replacement = -static_cast<long long>(input.returned_errno);
                        else if (input.injection == mode::short_pre)
                            replacement = input.short_count;
                        if (!info.exit.is_error || info.exit.rval != -ENOSYS ||
                            !replace_return(pid, replacement)) { controller_error = true; break; }
                        fault.pending_replacement = false;
                        fault.first_replaced = true;
                        if (retryable_eintr) fault.retry_window_open = true;
                    } else if (selected[pid] &&
                               (input.injection == mode::late_error ||
                                input.injection == mode::short_late ||
                                input.injection == mode::zero_late) &&
                               fault.matches == input.occurrence) {
                        if (info.exit.is_error) { controller_error = true; break; }
                        long long replacement = 0;
                        if (input.injection == mode::late_error)
                            replacement = -static_cast<long long>(input.returned_errno);
                        else if (input.injection == mode::short_late)
                            replacement = input.short_count;
                        if (!replace_return(pid, replacement)) { controller_error = true; break; }
                        fault.first_replaced = true;
                        if (retryable_eintr) fault.retry_window_open = true;
                    } else if (selected[pid] && retryable_eintr && fault.retry_seen) {
                        fault.retry_succeeded = !info.exit.is_error;
                    } else if (selected[pid] && input.point == boundary::reserve_revalidation &&
                               input.injection == mode::reserve_loss && !info.exit.is_error) {
                        struct statfs value {};
                        if (!read_tracee(pid, state.args[1], &value, sizeof(value))) {
                            controller_error = true;
                            break;
                        }
                        value.f_bavail = 0;
                        if (!write_tracee(pid, state.args[1], &value, sizeof(value))) {
                            controller_error = true;
                            break;
                        }
                        fault.first_replaced = true;
                        fault.mutation_applied = true;
                    }
                }
                selected[pid] = false;
                state.have_entry = false;
            } else {
                controller_error = true;
                break;
            }
            if (!resume_syscalls(pid)) controller_error = true;
            continue;
        }
        auto item = tracees.find(pid);
        if (signal == SIGSTOP && item != tracees.end() &&
            item->second.initial_sigstop_expected) {
            item->second.initial_sigstop_expected = false;
            if (!resume_syscalls(pid)) controller_error = true;
            continue;
        }
        if (signal == SIGSTOP || signal == SIGTRAP) {
            controller_error = true;
            break;
        }
        if (!resume_syscalls(pid, signal)) controller_error = true;
    }

    if (!tracees.empty()) {
        bounded_cleanup = true;
        for (const auto & item : tracees) (void) ::kill(item.first, SIGKILL);
        while (::waitpid(-1, &status, __WALL) > 0 || errno == EINTR) {}
    }
    std::string audit;
    std::array<char, 4096> audit_bytes {};
    bool audit_read = true;
    for (;;) {
        ssize_t audit_count;
        do { audit_count = ::read(audit_pipe[0], audit_bytes.data(), audit_bytes.size()); }
        while (audit_count < 0 && errno == EINTR);
        if (audit_count == 0) break;
        if (audit_count < 0 || audit.size() >
                65536U - static_cast<std::size_t>(audit_count)) {
            audit_read = false;
            break;
        }
        audit.append(audit_bytes.data(), static_cast<std::size_t>(audit_count));
    }
    const bool audit_closed = ::close(audit_pipe[0]) == 0;

    bool whole_root_discard = false;
    struct stat parent_now {}, root_now {}, fixture_now {};
    std::uint64_t parent_mount_now = 0, root_mount_now = 0, fixture_mount_now = 0;
    const auto exact_directory_authority = [](const struct stat & value) {
        return S_ISDIR(value.st_mode) && value.st_uid == ::geteuid() &&
               (value.st_mode & 07777) == 0700;
    };
    const auto reopened_matches = [](const char * path, const struct stat & expected,
                                     std::uint64_t expected_mount) {
        const int fd = open_absolute_directory_no_symlink(path);
        struct stat observed {};
        std::uint64_t mount = 0;
        const bool matched = fd >= 0 && ::fstat(fd, &observed) == 0 &&
            fd_mount_id(fd, mount) && same_object(expected, observed) &&
            mount == expected_mount && S_ISDIR(observed.st_mode) &&
            observed.st_uid == ::geteuid() && (observed.st_mode & 07777) == 0700;
        const bool closed = fd >= 0 && ::close(fd) == 0;
        return matched && closed;
    };
    const bool authorities_pinned =
        ::fstat(parent_fd, &parent_now) == 0 && exact_directory_authority(parent_now) &&
        same_object(parent_identity, parent_now) &&
        fd_mount_id(parent_fd, parent_mount_now) && parent_mount_now == parent_mount &&
        ::fstat(root_fd, &root_now) == 0 && exact_directory_authority(root_now) &&
        same_object(root_identity, root_now) &&
        fd_mount_id(root_fd, root_mount_now) && root_mount_now == root_mount &&
        ::fstat(fixture_fd, &fixture_now) == 0 &&
        exact_directory_authority(fixture_now) && same_object(fixture_identity, fixture_now) &&
        fd_mount_id(fixture_fd, fixture_mount_now) && fixture_mount_now == fixture_mount &&
        reopened_matches(input.parent, parent_identity, parent_mount) &&
        reopened_matches(input.root, root_identity, root_mount) &&
        reopened_matches(input.fixture, fixture_identity, fixture_mount);
    std::vector<std::string> parent_names, fixture_names;
    const bool authority_inventory = list_names(parent_fd, parent_names) &&
        list_names(fixture_fd, fixture_names) &&
        parent_names == std::vector<std::string>({ "fixture", "root" }) &&
        fixture_names == std::vector<std::string>({ "primitive.lock" });
    const bool inventory = exact_inventory(
        root_fd, marker, input, writer_identity, writer_pinned,
        directories, whole_root_discard);
    const bool fixture_released = prove_ofd_released(
        fixture_fd, "primitive.lock", fixture_lock_identity);
    const bool writer_released = writer_pinned && prove_ofd_released(
        root_fd, "writer.lock", writer_identity);
    const bool mutation_mode = input.injection >= mode::corrupt;
    const bool eintr_success = returned_eintr_must_retry(input);
    const bool short_late_success = input.injection == mode::short_late;
    const bool late_eintr_success = input.injection == mode::late_error &&
        input.returned_errno == EINTR && eintr_is_retryable(input.point);
    const bool injection = mutation_mode
        ? fault.mutation_applied &&
            (input.injection != mode::temp_inode_substitute ||
             marker.temp_substitute_open_seen)
        : eintr_success
            ? fault.first_replaced && fault.retry_seen && fault.retry_succeeded &&
                fault.matches == input.occurrence + 1 &&
                !fault.retry_window_open
            : fault.first_replaced;
    const bool qualified = eintr_success || short_late_success || late_eintr_success;
    const bool byte_coverage = exact_byte_coverage(input, marker, qualified);
    const unsigned expected_publication_attempts =
        input.point == boundary::marker_rename || marker.published_seen ? 1U : 0U;
    const bool audit_exact = exact_audit(audit, qualified, input, marker);
    const bool process = qualified
        ? child_exit == 0 && launcher_exit == 0 && audit_exact
        : child_exit != 0 && launcher_exit != 0 && audit_exact;
    const bool pass = !controller_error && !bounded_cleanup && launcher_exec &&
        child_exec && audit_read && audit_closed && !cleanup_syscall_seen &&
        publication_attempts == expected_publication_attempts &&
        injection && byte_coverage && process &&
        authorities_pinned && authority_inventory && inventory &&
        whole_root_discard && fixture_released && writer_released;
    emit(std::string("\"event\":\"post-fault\",\"pass\":") +
         (pass ? "true" : "false") + ",\"matches\":" +
         std::to_string(fault.matches) + ",\"encoded_size\":" +
         std::to_string(marker.encoded_size) + ",\"transient_retained\":" +
         (marker.transient_seen && !marker.published_seen ? "true" : "false") +
         ",\"published_retained\":" + (marker.published_seen ? "true" : "false") +
         ",\"inventory_exact\":" + (inventory ? "true" : "false") +
         ",\"authorities_pinned\":" + (authorities_pinned ? "true" : "false") +
         ",\"authority_inventory_exact\":" +
         (authority_inventory ? "true" : "false") +
         ",\"injection_exact\":" + (injection ? "true" : "false") +
         ",\"audit_exact\":" + (audit_exact ? "true" : "false") +
         ",\"child_status\":" + std::to_string(child_exit) +
         ",\"launcher_status\":" + std::to_string(launcher_exit) +
         ",\"publication_attempts\":" +
         std::to_string(publication_attempts) +
         ",\"byte_coverage_exact\":" + (byte_coverage ? "true" : "false") +
         ",\"fixture_lock_released\":" + (fixture_released ? "true" : "false") +
         ",\"writer_lock_released\":" + (writer_released ? "true" : "false"));
    emit(std::string("\"event\":\"summary\",\"pass\":") +
         (pass ? "true" : "false") +
         ",\"initialization_discard_required\":true" +
         ",\"whole_root_discard_required\":true" +
         ",\"individual_cleanup_performed\":" +
         (cleanup_syscall_seen ? "true" : "false") +
         ",\"marker_qualified\":" + (qualified ? "true" : "false"));

    const bool receipt_synced = synchronize_receipt(
        receipt_parent_fd, receipt_parent_identity, receipt_parent_mount, receipt_name);
    const bool receipt_closed = ::close(receipt_fd) == 0;
    receipt_fd = -1;
    const bool receipt_parent_closed = ::close(receipt_parent_fd) == 0;
    const bool staging_closed = staging_fd >= 0 && ::close(staging_fd) == 0;
    const bool fixture_closed = ::close(fixture_fd) == 0;
    const bool root_closed = ::close(root_fd) == 0;
    const bool parent_closed = ::close(parent_fd) == 0;
    return pass && receipt_ok && receipt_synced && receipt_closed &&
        receipt_parent_closed && staging_closed && fixture_closed &&
        root_closed && parent_closed ? 0 : 1;
}

} // namespace marker_fault
} // namespace

int main(int argc, char ** argv) {
    marker_fault::arguments input {};
    if (!marker_fault::parse_arguments(argc, argv, input)) {
        marker_fault::usage_marker(argv[0]);
    }
    return marker_fault::run(input);
}
