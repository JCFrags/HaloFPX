#if !defined(__linux__) || !defined(__x86_64__)
#error "The L05x ptrace controller currently requires Linux x86-64"
#endif

#include <algorithm>
#include <array>
#include <cerrno>
#include <chrono>
#include <cinttypes>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <dirent.h>
#include <fcntl.h>
#include <linux/audit.h>
#include <linux/openat2.h>
#include <limits.h>
#include <signal.h>
#include <string>
#include <sys/ptrace.h>
#include <sys/stat.h>
#include <sys/syscall.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <unordered_map>
#include <vector>

namespace {

enum class operation { mkdir_directory, fchmodat2_directory, fsync_directory,
                       fsync_root };
enum class phase { entry, exit };

struct boundary {
    operation op = operation::mkdir_directory;
    int directory_index = 0;
};

struct options {
    const char * target = nullptr;
    const char * golden = nullptr;
    const char * parent = nullptr;
    const char * root = nullptr;
    const char * fixture = nullptr;
    const char * receipt = nullptr;
    boundary point {};
    phase when = phase::entry;
    mode_t tracee_umask = 0;
    bool point_set = false;
    bool phase_set = false;
    bool umask_set = false;
};

struct tracee_state {
    bool live_child = false;
    bool initial_sigstop_expected = false;
    bool initial_sigstop_observed = false;
    bool have_entry = false;
    std::uint64_t nr = 0;
    std::uint64_t args[6] {};
};

struct pinned_directory {
    bool pinned = false;
    struct stat identity {};
    std::uint64_t mount_id = 0;
};

constexpr std::array<const char *, 3> directory_names {
    "envelopes", "attempts", "staging"
};

int receipt_fd = -1;
std::uint64_t receipt_sequence = 0;
bool receipt_ok = true;

[[noreturn]] void usage(const char * program) {
    std::fprintf(stderr,
        "usage: %s --target PATH --golden PATH --parent PATH --root PATH "
        "--fixture PATH "
        "--boundary mkdirat-envelopes|mkdirat-attempts|mkdirat-staging|"
        "fchmodat2-envelopes|fchmodat2-attempts|fchmodat2-staging|"
        "fsync-envelopes|fsync-attempts|fsync-staging|fsync-root "
        "--phase entry|exit --tracee-umask OCTAL --receipt NEW-PATH\n", program);
    std::exit(2);
}

bool set_once(const char *& output, const char * value) {
    if (output != nullptr || value == nullptr || value[0] != '/') return false;
    output = value;
    return true;
}

bool parse_boundary(const char * value, boundary & output) {
    for (int i = 0; i < static_cast<int>(directory_names.size()); ++i) {
        const std::string name = directory_names[static_cast<std::size_t>(i)];
        if (value == std::string("mkdirat-") + name) {
            output = { operation::mkdir_directory, i };
            return true;
        }
        if (value == std::string("fchmodat2-") + name) {
            output = { operation::fchmodat2_directory, i };
            return true;
        }
        if (value == std::string("fsync-") + name) {
            output = { operation::fsync_directory, i };
            return true;
        }
    }
    if (std::strcmp(value, "fsync-root") == 0) {
        output = { operation::fsync_root, 3 };
        return true;
    }
    return false;
}

bool parse_umask(const char * value, mode_t & output) {
    if (value == nullptr || std::strlen(value) != 4 || value[0] != '0') return false;
    mode_t parsed = 0;
    for (std::size_t i = 1; i < 4; ++i) {
        if (value[i] < '0' || value[i] > '7') return false;
        parsed = static_cast<mode_t>((parsed << 3) |
            static_cast<mode_t>(value[i] - '0'));
    }
    output = parsed;
    return true;
}

bool parse(int argc, char ** argv, options & output) {
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
        } else if (std::strcmp(key, "--receipt") == 0) {
            if (!set_once(output.receipt, value)) return false;
        } else if (std::strcmp(key, "--boundary") == 0) {
            if (output.point_set || !parse_boundary(value, output.point)) return false;
            output.point_set = true;
        } else if (std::strcmp(key, "--phase") == 0) {
            if (output.phase_set) return false;
            if (std::strcmp(value, "entry") == 0) output.when = phase::entry;
            else if (std::strcmp(value, "exit") == 0) output.when = phase::exit;
            else return false;
            output.phase_set = true;
        } else if (std::strcmp(key, "--tracee-umask") == 0) {
            if (output.umask_set || !parse_umask(value, output.tracee_umask)) return false;
            output.umask_set = true;
        } else return false;
    }
    return output.target && output.golden && output.parent && output.root &&
           output.fixture && output.receipt && output.point_set && output.phase_set &&
           output.umask_set;
}

bool same_object(const struct stat & expected, const struct stat & observed) {
    return expected.st_dev == observed.st_dev && expected.st_ino == observed.st_ino;
}

bool fd_mount_id(int fd, std::uint64_t & output) {
    struct statx extended {};
    if (::syscall(SYS_statx, fd, "", AT_EMPTY_PATH | AT_STATX_SYNC_AS_STAT,
                  STATX_BASIC_STATS | STATX_MNT_ID, &extended) != 0 ||
        (extended.stx_mask & STATX_MNT_ID) == 0 || extended.stx_mnt_id == 0) {
        return false;
    }
    output = static_cast<std::uint64_t>(extended.stx_mnt_id);
    return true;
}

bool canonical_existing_path(const char * path) {
    std::array<char, PATH_MAX> resolved {};
    return ::realpath(path, resolved.data()) != nullptr &&
           std::strcmp(path, resolved.data()) == 0;
}

int open_absolute_directory_no_symlink(const char * path) {
    if (path == nullptr || path[0] != '/') return -1;
    const int filesystem_root = ::open(
        "/", O_PATH | O_DIRECTORY | O_CLOEXEC | O_NOFOLLOW);
    if (filesystem_root < 0) return -1;
    const char * relative = path[1] == '\0' ? "." : path + 1;
    struct open_how how {};
    how.flags = O_RDONLY | O_DIRECTORY | O_CLOEXEC | O_NOFOLLOW;
    how.resolve = RESOLVE_BENEATH | RESOLVE_NO_MAGICLINKS |
                  RESOLVE_NO_SYMLINKS;
    int fd;
    do {
        fd = static_cast<int>(::syscall(
            SYS_openat2, filesystem_root, relative, &how, sizeof(how)));
    } while (fd < 0 && errno == EINTR);
    const bool root_closed = ::close(filesystem_root) == 0;
    if (fd < 0 || !root_closed) {
        if (fd >= 0) (void) ::close(fd);
        return -1;
    }
    std::array<char, PATH_MAX> proc_path {};
    std::array<char, PATH_MAX> observed {};
    const int count = std::snprintf(proc_path.data(), proc_path.size(),
                                    "/proc/self/fd/%d", fd);
    const std::size_t expected = std::strlen(path);
    if (count <= 0 || static_cast<std::size_t>(count) >= proc_path.size() ||
        ::readlink(proc_path.data(), observed.data(), observed.size()) !=
            static_cast<ssize_t>(expected) ||
        std::memcmp(observed.data(), path, expected) != 0) {
        (void) ::close(fd);
        return -1;
    }
    return fd;
}

bool same_or_descendant(const char * candidate, const char * directory) {
    const std::size_t size = std::strlen(directory);
    return size != 0 && std::strncmp(candidate, directory, size) == 0 &&
           (candidate[size] == '\0' || directory[size - 1] == '/' ||
            candidate[size] == '/');
}

bool split_receipt_path(const char * path, std::string & parent, std::string & name) {
    const std::string value(path);
    const std::size_t slash = value.find_last_of('/');
    if (slash == std::string::npos || slash == value.size() - 1) return false;
    parent = slash == 0 ? "/" : value.substr(0, slash);
    name = value.substr(slash + 1);
    return name != "." && name != ".." && name.find('/') == std::string::npos;
}

bool direct_child(const char * child, const char * parent) {
    const std::size_t size = std::strlen(parent);
    if (!same_or_descendant(child, parent) || child[size] != '/') return false;
    const char * suffix = child + size + 1;
    return suffix[0] != '\0' && std::strchr(suffix, '/') == nullptr &&
           std::strcmp(suffix, ".") != 0 && std::strcmp(suffix, "..") != 0;
}

const char * boundary_name(const boundary & point) {
    switch (point.op) {
        case operation::mkdir_directory:
            return point.directory_index == 0 ? "mkdirat(envelopes)" :
                   point.directory_index == 1 ? "mkdirat(attempts)" : "mkdirat(staging)";
        case operation::fchmodat2_directory:
            return point.directory_index == 0 ? "fchmodat2(envelopes)" :
                   point.directory_index == 1 ? "fchmodat2(attempts)" : "fchmodat2(staging)";
        case operation::fsync_directory:
            return point.directory_index == 0 ? "fsync(envelopes)" :
                   point.directory_index == 1 ? "fsync(attempts)" : "fsync(staging)";
        case operation::fsync_root: return "fsync(root)";
    }
    return "unknown";
}

bool write_all(int fd, const char * data, std::size_t size) {
    std::size_t done = 0;
    while (done < size) {
        const ssize_t count = ::write(fd, data + done, size - done);
        if (count < 0 && errno == EINTR) continue;
        if (count <= 0) return false;
        done += static_cast<std::size_t>(count);
    }
    return true;
}

bool emit(const std::string & event) {
    struct timespec now {};
    if (::clock_gettime(CLOCK_REALTIME, &now) != 0 || now.tv_sec < 0) {
        receipt_ok = false;
        return false;
    }
    const std::uint64_t timestamp_ns =
        static_cast<std::uint64_t>(now.tv_sec) * 1000000000ULL +
        static_cast<std::uint64_t>(now.tv_nsec);
    char prefix[160];
    const int count = std::snprintf(prefix, sizeof(prefix),
        "{\"sequence\":%" PRIu64 ",\"timestamp_ns\":%" PRIu64 ",",
        ++receipt_sequence, timestamp_ns);
    if (count <= 0 || static_cast<std::size_t>(count) >= sizeof(prefix)) {
        receipt_ok = false;
        return false;
    }
    const std::string line = std::string(prefix, static_cast<std::size_t>(count)) +
                             event + "}\n";
    receipt_ok = write_all(receipt_fd, line.data(), line.size()) && receipt_ok;
    return receipt_ok;
}

bool read_tracee(pid_t pid, std::uint64_t address, void * output, std::size_t size) {
    auto * bytes = static_cast<unsigned char *>(output);
    for (std::size_t offset = 0; offset < size; offset += sizeof(long)) {
        errno = 0;
        const long word = ::ptrace(PTRACE_PEEKDATA, pid,
            reinterpret_cast<void *>(address + offset), nullptr);
        if (word == -1 && errno != 0) return false;
        const std::size_t copied = std::min(sizeof(word), size - offset);
        std::memcpy(bytes + offset, &word, copied);
    }
    return true;
}

bool read_tracee_string(pid_t pid, std::uint64_t address, std::string & output) {
    output.clear();
    for (std::size_t offset = 0; offset < 64; offset += sizeof(long)) {
        long word = 0;
        if (!read_tracee(pid, address + offset, &word, sizeof(word))) return false;
        for (std::size_t i = 0; i < sizeof(word); ++i) {
            const char byte = reinterpret_cast<const char *>(&word)[i];
            if (byte == '\0') return true;
            output.push_back(byte);
        }
    }
    return false;
}

bool same_executable(pid_t pid, const struct stat & expected) {
    struct stat observed {};
    char path[64];
    const int count = std::snprintf(path, sizeof(path), "/proc/%ld/exe",
                                    static_cast<long>(pid));
    return count > 0 && static_cast<std::size_t>(count) < sizeof(path) &&
           ::stat(path, &observed) == 0 && same_object(expected, observed);
}

bool tracee_fd_identity(pid_t pid, int fd, struct stat & output,
                        std::uint64_t & mount_id) {
    char path[96];
    const int count = std::snprintf(path, sizeof(path), "/proc/%ld/fd/%d",
                                    static_cast<long>(pid), fd);
    if (count <= 0 || static_cast<std::size_t>(count) >= sizeof(path) ||
        ::stat(path, &output) != 0) return false;
    struct statx extended {};
    if (::syscall(SYS_statx, AT_FDCWD, path, AT_STATX_SYNC_AS_STAT,
                  STATX_BASIC_STATS | STATX_MNT_ID, &extended) != 0 ||
        (extended.stx_mask & STATX_MNT_ID) == 0 || extended.stx_mnt_id == 0) {
        return false;
    }
    mount_id = static_cast<std::uint64_t>(extended.stx_mnt_id);
    return true;
}

bool exact_live_child_argv(pid_t pid) {
    char path[64];
    const int path_size = std::snprintf(path, sizeof(path), "/proc/%ld/cmdline",
                                        static_cast<long>(pid));
    if (path_size <= 0 || static_cast<std::size_t>(path_size) >= sizeof(path)) return false;
    const int fd = ::open(path, O_RDONLY | O_CLOEXEC | O_NOFOLLOW);
    if (fd < 0) return false;
    char bytes[128] {};
    ssize_t count;
    do { count = ::read(fd, bytes, sizeof(bytes)); } while (count < 0 && errno == EINTR);
    const bool closed = ::close(fd) == 0;
    constexpr char expected[] = "halofpx-l05x-live-child\0--live-directory-child\0";
    return closed && count == static_cast<ssize_t>(sizeof(expected) - 1) &&
           std::memcmp(bytes, expected, sizeof(expected) - 1) == 0;
}

bool open_named(int root_fd, const char * name, int flags, int & output) {
    struct open_how how {};
    how.flags = static_cast<decltype(how.flags)>(
        static_cast<unsigned>(flags | O_CLOEXEC | O_NOFOLLOW));
    how.resolve = RESOLVE_BENEATH | RESOLVE_NO_MAGICLINKS |
                  RESOLVE_NO_SYMLINKS | RESOLVE_NO_XDEV;
    do {
        output = static_cast<int>(::syscall(SYS_openat2, root_fd, name, &how, sizeof(how)));
    } while (output < 0 && errno == EINTR);
    return output >= 0;
}

bool directory_empty(int fd) {
    const int duplicate = ::fcntl(fd, F_DUPFD_CLOEXEC, 0);
    if (duplicate < 0) return false;
    DIR * stream = ::fdopendir(duplicate);
    if (stream == nullptr) {
        (void) ::close(duplicate);
        return false;
    }
    bool empty = true;
    errno = 0;
    for (;;) {
        dirent * entry = ::readdir(stream);
        if (entry == nullptr) break;
        if (std::strcmp(entry->d_name, ".") != 0 &&
            std::strcmp(entry->d_name, "..") != 0) {
            empty = false;
            break;
        }
        errno = 0;
    }
    const bool read_ok = errno == 0;
    const bool closed = ::closedir(stream) == 0;
    return empty && read_ok && closed;
}

bool pin_created_directory(int root_fd, int index, mode_t expected_mode,
                           pinned_directory & output) {
    int fd = -1;
    if (!open_named(root_fd, directory_names[static_cast<std::size_t>(index)],
                    O_PATH | O_DIRECTORY, fd)) return false;
    struct stat value {};
    std::uint64_t mount_id = 0;
    const bool valid = ::fstat(fd, &value) == 0 && S_ISDIR(value.st_mode) &&
        fd_mount_id(fd, mount_id) && value.st_uid == ::geteuid() &&
        (value.st_mode & 07777) == expected_mode;
    const bool closed = ::close(fd) == 0;
    if (!valid || !closed) return false;
    output.pinned = true;
    output.identity = value;
    output.mount_id = mount_id;
    return true;
}

bool writer_openat2_entry(pid_t pid, const tracee_state & state,
                          const struct stat & expected_root,
                          std::uint64_t expected_root_mount) {
    if (state.nr != SYS_openat2) return false;
    std::string name;
    struct open_how how {};
    struct stat root {};
    std::uint64_t root_mount = 0;
    if (!read_tracee_string(pid, state.args[1], name) || name != "writer.lock" ||
        state.args[3] != sizeof(how) || !read_tracee(pid, state.args[2], &how, sizeof(how))) {
        return false;
    }
    constexpr std::uint64_t exact_flags =
        O_CREAT | O_EXCL | O_RDWR | O_CLOEXEC | O_NOFOLLOW;
    constexpr std::uint64_t exact_resolve =
        RESOLVE_BENEATH | RESOLVE_NO_MAGICLINKS | RESOLVE_NO_SYMLINKS | RESOLVE_NO_XDEV;
    return tracee_fd_identity(pid, static_cast<int>(state.args[0]), root, root_mount) &&
           same_object(expected_root, root) && S_ISDIR(root.st_mode) &&
           root_mount == expected_root_mount &&
           how.flags == exact_flags && how.mode == 0600 && how.resolve == exact_resolve;
}

bool fixture_openat2_entry(pid_t pid, const tracee_state & state,
                           const struct stat & expected_fixture,
                           std::uint64_t expected_fixture_mount) {
    if (state.nr != SYS_openat2) return false;
    std::string name;
    struct open_how how {};
    struct stat fixture {};
    std::uint64_t fixture_mount = 0;
    if (!read_tracee_string(pid, state.args[1], name) || name != "primitive.lock" ||
        state.args[3] != sizeof(how) ||
        !read_tracee(pid, state.args[2], &how, sizeof(how))) return false;
    constexpr std::uint64_t exact_flags = O_RDWR | O_CLOEXEC | O_NOFOLLOW;
    constexpr std::uint64_t exact_resolve = RESOLVE_BENEATH | RESOLVE_NO_MAGICLINKS |
        RESOLVE_NO_SYMLINKS | RESOLVE_NO_XDEV;
    return tracee_fd_identity(pid, static_cast<int>(state.args[0]),
                              fixture, fixture_mount) &&
           S_ISDIR(fixture.st_mode) && same_object(expected_fixture, fixture) &&
           fixture_mount == expected_fixture_mount && how.flags == exact_flags &&
           how.mode == 0 && how.resolve == exact_resolve;
}

bool ofd_write_lock_entry(pid_t pid, const tracee_state & state, int expected_fd,
                          const struct stat & expected_identity,
                          std::uint64_t expected_mount) {
    if (expected_fd < 0 || state.nr != SYS_fcntl ||
        static_cast<int>(state.args[0]) != expected_fd ||
        static_cast<int>(state.args[1]) != F_OFD_SETLK) return false;
    struct flock lock {};
    struct stat observed {};
    std::uint64_t observed_mount = 0;
    return tracee_fd_identity(pid, expected_fd, observed, observed_mount) &&
           same_object(expected_identity, observed) && observed_mount == expected_mount &&
           read_tracee(pid, state.args[2], &lock, sizeof(lock)) &&
           lock.l_type == F_WRLCK && lock.l_whence == SEEK_SET &&
           lock.l_start == 0 && lock.l_len == 0;
}

bool exact_directory_fd(pid_t pid, int fd, const pinned_directory & expected) {
    struct stat observed {};
    std::uint64_t observed_mount = 0;
    return expected.pinned && tracee_fd_identity(pid, fd, observed, observed_mount) &&
           S_ISDIR(observed.st_mode) && same_object(expected.identity, observed) &&
           observed_mount == expected.mount_id;
}

bool selected_entry(pid_t pid, const tracee_state & state, const boundary & point,
                    const struct stat & root, std::uint64_t root_mount,
                    const std::array<pinned_directory, 3> & dirs) {
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
            return state.nr == SYS_fchmodat2 && state.args[2] == 0700 &&
                   state.args[3] == AT_EMPTY_PATH &&
                   read_tracee_string(pid, state.args[1], empty_path) &&
                   empty_path.empty() &&
                   exact_directory_fd(pid, static_cast<int>(state.args[0]),
                                      dirs[static_cast<std::size_t>(index)]);
        }
        case operation::fsync_directory:
            return state.nr == SYS_fsync &&
                   exact_directory_fd(pid, static_cast<int>(state.args[0]),
                                      dirs[static_cast<std::size_t>(index)]);
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

bool resume_syscalls(pid_t pid, int signal_number = 0) {
    return ::ptrace(PTRACE_SYSCALL, pid, nullptr,
                    reinterpret_cast<void *>(static_cast<intptr_t>(signal_number))) == 0;
}

bool prove_ofd_released(int directory_fd, const char * name,
                        const struct stat & expected_file) {
    int fd = -1;
    if (!open_named(directory_fd, name, O_RDWR, fd)) return false;
    struct stat file_identity {};
    if (::fstat(fd, &file_identity) != 0 || !S_ISREG(file_identity.st_mode) ||
        !same_object(expected_file, file_identity) || file_identity.st_nlink != 1 ||
        file_identity.st_size != 0 || file_identity.st_uid != ::geteuid() ||
        (file_identity.st_mode & 07777) != 0600) {
        (void) ::close(fd);
        return false;
    }
    struct flock lock {};
    lock.l_type = F_WRLCK;
    lock.l_whence = SEEK_SET;
    int acquired;
    do { acquired = ::fcntl(fd, F_OFD_SETLK, &lock); } while (acquired < 0 && errno == EINTR);
    bool unlocked = false;
    if (acquired == 0) {
        lock.l_type = F_UNLCK;
        int result;
        do { result = ::fcntl(fd, F_OFD_SETLK, &lock); } while (result < 0 && errno == EINTR);
        unlocked = result == 0;
    }
    const bool closed = ::close(fd) == 0;
    return acquired == 0 && unlocked && closed;
}

int expected_prefix(const boundary & point, phase when) {
    if (point.op == operation::mkdir_directory && when == phase::entry) {
        return point.directory_index;
    }
    if (point.op == operation::fsync_root) return 3;
    return point.directory_index + 1;
}

bool final_directory_mode_at_crash(int index, const boundary & point, phase when) {
    if (point.op == operation::fsync_root || index < point.directory_index) return true;
    if (index != point.directory_index) return false;
    if (point.op == operation::fchmodat2_directory) return when == phase::exit;
    return point.op == operation::fsync_directory;
}

bool inventory_root(int root_fd, int expected_count,
                    const struct stat & writer,
                    std::array<pinned_directory, 3> & dirs,
                    const boundary & point, phase when, mode_t tracee_umask) {
    const int duplicate = ::openat(
        root_fd, ".", O_RDONLY | O_DIRECTORY | O_CLOEXEC | O_NOFOLLOW);
    if (duplicate < 0) {
        emit("\"event\":\"inventory-failure\",\"stage\":\"duplicate-root\"");
        return false;
    }
    DIR * stream = ::fdopendir(duplicate);
    if (stream == nullptr) {
        (void) ::close(duplicate);
        emit("\"event\":\"inventory-failure\",\"stage\":\"open-stream\"");
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
    const bool closed = ::closedir(stream) == 0;
    if (!read_ok || !closed || names.size() != static_cast<std::size_t>(expected_count + 1)) {
        emit(std::string("\"event\":\"inventory-failure\",\"stage\":\"entry-count\",") +
             "\"observed\":" + std::to_string(names.size()) +
             ",\"expected\":" + std::to_string(expected_count + 1));
        return false;
    }
    std::sort(names.begin(), names.end());
    std::vector<std::string> expected { "writer.lock" };
    for (int i = 0; i < expected_count; ++i) {
        expected.emplace_back(directory_names[static_cast<std::size_t>(i)]);
    }
    std::sort(expected.begin(), expected.end());
    if (names != expected) {
        emit("\"event\":\"inventory-failure\",\"stage\":\"entry-names\"");
        return false;
    }

    int writer_fd = -1;
    if (!open_named(root_fd, "writer.lock", O_RDONLY, writer_fd)) {
        emit("\"event\":\"inventory-failure\",\"stage\":\"open-writer\"");
        return false;
    }
    struct stat writer_observed {};
    const bool writer_ok = ::fstat(writer_fd, &writer_observed) == 0 &&
        S_ISREG(writer_observed.st_mode) && same_object(writer, writer_observed) &&
        writer_observed.st_nlink == 1 && writer_observed.st_size == 0 &&
        writer_observed.st_uid == ::geteuid() &&
        (writer_observed.st_mode & 07777) == 0600;
    const bool writer_closed = ::close(writer_fd) == 0;
    if (!writer_ok || !writer_closed) {
        emit("\"event\":\"inventory-failure\",\"stage\":\"writer-identity\"");
        return false;
    }

    for (int i = 0; i < expected_count; ++i) {
        int fd = -1;
        const bool final_mode = final_directory_mode_at_crash(i, point, when);
        if (!dirs[static_cast<std::size_t>(i)].pinned ||
            !open_named(root_fd, directory_names[static_cast<std::size_t>(i)],
                        O_RDONLY | O_DIRECTORY, fd)) {
            emit("\"event\":\"inventory-failure\",\"stage\":\"open-directory\"");
            return false;
        }
        struct stat observed {};
        std::uint64_t observed_mount = 0;
        const mode_t expected_mode = final_mode
            ? static_cast<mode_t>(0700)
            : static_cast<mode_t>(0700 & ~tracee_umask);
        const bool valid = ::fstat(fd, &observed) == 0 && fd_mount_id(fd, observed_mount) &&
            S_ISDIR(observed.st_mode) &&
            same_object(dirs[static_cast<std::size_t>(i)].identity, observed) &&
            observed_mount == dirs[static_cast<std::size_t>(i)].mount_id &&
            observed.st_uid == ::geteuid() && (observed.st_mode & 07777) == expected_mode &&
            directory_empty(fd);
        const bool directory_closed = ::close(fd) == 0;
        if (!valid || !directory_closed) {
            emit("\"event\":\"inventory-failure\",\"stage\":\"directory-identity\"");
            return false;
        }
        dirs[static_cast<std::size_t>(i)].identity = observed;
    }
    return true;
}

bool named_receipt_matches(int parent_fd, const std::string & name,
                           const struct stat & expected) {
    int fd = -1;
    if (!open_named(parent_fd, name.c_str(), O_RDONLY, fd)) return false;
    struct stat observed {};
    const bool matched = ::fstat(fd, &observed) == 0 && S_ISREG(observed.st_mode) &&
        same_object(expected, observed) && observed.st_nlink == 1 &&
        observed.st_size == expected.st_size && observed.st_uid == expected.st_uid &&
        (observed.st_mode & 07777) == (expected.st_mode & 07777);
    const bool closed = ::close(fd) == 0;
    return matched && closed;
}

bool synchronize_receipt(int parent_fd, const struct stat & expected_parent,
                         std::uint64_t expected_parent_mount,
                         const std::string & receipt_name) {
    struct stat expected_receipt {};
    if (::fstat(receipt_fd, &expected_receipt) != 0 ||
        !S_ISREG(expected_receipt.st_mode) || expected_receipt.st_nlink != 1 ||
        expected_receipt.st_uid != ::geteuid() ||
        (expected_receipt.st_mode & 07777) != 0600 ||
        ::fsync(receipt_fd) != 0 ||
        !named_receipt_matches(parent_fd, receipt_name, expected_receipt)) return false;
    struct stat observed_parent {};
    std::uint64_t observed_mount = 0;
    if (::fstat(parent_fd, &observed_parent) != 0 ||
        !fd_mount_id(parent_fd, observed_mount) ||
        !same_object(expected_parent, observed_parent) ||
        observed_mount != expected_parent_mount || ::fsync(parent_fd) != 0) return false;
    return ::fstat(parent_fd, &observed_parent) == 0 &&
           fd_mount_id(parent_fd, observed_mount) &&
           same_object(expected_parent, observed_parent) &&
           observed_mount == expected_parent_mount &&
           named_receipt_matches(parent_fd, receipt_name, expected_receipt);
}

int finalize_receipt(int parent_fd, const struct stat & expected_parent,
                     std::uint64_t expected_parent_mount,
                     const std::string & receipt_name, int intended_status) {
    const bool synchronized = receipt_fd >= 0 &&
        synchronize_receipt(parent_fd, expected_parent, expected_parent_mount,
                            receipt_name);
    const bool receipt_closed = receipt_fd >= 0 && ::close(receipt_fd) == 0;
    receipt_fd = -1;
    const bool parent_closed = parent_fd >= 0 && ::close(parent_fd) == 0;
    return receipt_ok && synchronized && receipt_closed && parent_closed
               ? intended_status : 1;
}

int controller(const options & input) {
    std::string receipt_parent_path;
    std::string receipt_name;
    if (!canonical_existing_path(input.target) || !canonical_existing_path(input.golden) ||
        !canonical_existing_path(input.parent) || !canonical_existing_path(input.root) ||
        !canonical_existing_path(input.fixture) || !direct_child(input.root, input.parent) ||
        !direct_child(input.fixture, input.parent) ||
        !split_receipt_path(input.receipt, receipt_parent_path, receipt_name) ||
        !canonical_existing_path(receipt_parent_path.c_str()) ||
        same_or_descendant(receipt_parent_path.c_str(), input.parent)) {
        std::fprintf(stderr, "non-canonical authority or non-disposable receipt scope\n");
        return 2;
    }
    receipt_ok = true;
    receipt_sequence = 0;
    const int receipt_parent_fd =
        open_absolute_directory_no_symlink(receipt_parent_path.c_str());
    struct stat receipt_parent_stat {};
    std::uint64_t receipt_parent_mount = 0;
    struct open_how receipt_how {};
    receipt_how.flags = O_WRONLY | O_CREAT | O_EXCL | O_CLOEXEC | O_NOFOLLOW;
    receipt_how.mode = 0600;
    receipt_how.resolve = RESOLVE_BENEATH | RESOLVE_NO_MAGICLINKS |
                          RESOLVE_NO_SYMLINKS | RESOLVE_NO_XDEV;
    if (receipt_parent_fd >= 0 &&
        (::fstat(receipt_parent_fd, &receipt_parent_stat) != 0 ||
         !S_ISDIR(receipt_parent_stat.st_mode) ||
         receipt_parent_stat.st_uid != ::geteuid() ||
         (receipt_parent_stat.st_mode & 07777) != 0700 ||
         !fd_mount_id(receipt_parent_fd, receipt_parent_mount))) {
        (void) ::close(receipt_parent_fd);
        std::fprintf(stderr, "unable to pin receipt parent\n");
        return 2;
    }
    do {
        receipt_fd = receipt_parent_fd < 0 ? -1 : static_cast<int>(::syscall(
            SYS_openat2, receipt_parent_fd, receipt_name.c_str(),
            &receipt_how, sizeof(receipt_how)));
    } while (receipt_fd < 0 && errno == EINTR);
    if (receipt_fd < 0) {
        std::perror("open receipt");
        if (receipt_parent_fd >= 0) (void) ::close(receipt_parent_fd);
        return 2;
    }
    struct stat receipt_stat {};
    struct stat receipt_parent_now {};
    std::uint64_t receipt_parent_mount_now = 0;
    if (::fchmod(receipt_fd, 0600) != 0 || ::fstat(receipt_fd, &receipt_stat) != 0 ||
        !S_ISREG(receipt_stat.st_mode) || receipt_stat.st_nlink != 1 ||
        receipt_stat.st_size != 0 || receipt_stat.st_uid != ::geteuid() ||
        (receipt_stat.st_mode & 07777) != 0600 ||
        ::fstat(receipt_parent_fd, &receipt_parent_now) != 0 ||
        !fd_mount_id(receipt_parent_fd, receipt_parent_mount_now) ||
        !same_object(receipt_parent_stat, receipt_parent_now) ||
        receipt_parent_mount != receipt_parent_mount_now) {
        std::fprintf(stderr, "unable to validate receipt authority\n");
        return finalize_receipt(
            receipt_parent_fd, receipt_parent_stat, receipt_parent_mount,
            receipt_name, 2);
    }

    const int target_fd = ::open(input.target, O_RDONLY | O_CLOEXEC | O_NOFOLLOW);
    const int parent_fd = ::open(input.parent, O_RDONLY | O_DIRECTORY | O_CLOEXEC | O_NOFOLLOW);
    const int root_fd = ::open(input.root, O_RDONLY | O_DIRECTORY | O_CLOEXEC | O_NOFOLLOW);
    const int fixture_fd = ::open(input.fixture, O_RDONLY | O_DIRECTORY | O_CLOEXEC | O_NOFOLLOW);
    const int fixture_lock_fd = fixture_fd >= 0
        ? ::openat(fixture_fd, "primitive.lock", O_RDONLY | O_CLOEXEC | O_NOFOLLOW) : -1;
    struct stat target_stat {}, parent_stat {}, root_stat {}, fixture_stat {}, fixture_lock_stat {};
    std::uint64_t parent_mount = 0;
    std::uint64_t root_mount = 0;
    std::uint64_t fixture_mount = 0;
    if (target_fd < 0 || ::fstat(target_fd, &target_stat) != 0 ||
        !S_ISREG(target_stat.st_mode) || (target_stat.st_mode & 0111) == 0 ||
        parent_fd < 0 || ::fstat(parent_fd, &parent_stat) != 0 ||
        !fd_mount_id(parent_fd, parent_mount) || !S_ISDIR(parent_stat.st_mode) ||
        parent_stat.st_uid != ::geteuid() || (parent_stat.st_mode & 07777) != 0700 ||
        root_fd < 0 || ::fstat(root_fd, &root_stat) != 0 ||
        !fd_mount_id(root_fd, root_mount) || !S_ISDIR(root_stat.st_mode) ||
        root_stat.st_uid != ::geteuid() || (root_stat.st_mode & 07777) != 0700 ||
        fixture_fd < 0 || ::fstat(fixture_fd, &fixture_stat) != 0 ||
        !fd_mount_id(fixture_fd, fixture_mount) || !S_ISDIR(fixture_stat.st_mode) ||
        fixture_stat.st_uid != ::geteuid() || (fixture_stat.st_mode & 07777) != 0700 ||
        fixture_lock_fd < 0 || ::fstat(fixture_lock_fd, &fixture_lock_stat) != 0 ||
        !S_ISREG(fixture_lock_stat.st_mode) || fixture_lock_stat.st_nlink != 1 ||
        fixture_lock_stat.st_size != 0 || fixture_lock_stat.st_uid != ::geteuid() ||
        (fixture_lock_stat.st_mode & 07777) != 0600) {
        emit("\"event\":\"invalid-target\"");
        if (fixture_lock_fd >= 0) (void) ::close(fixture_lock_fd);
        if (fixture_fd >= 0) (void) ::close(fixture_fd);
        if (root_fd >= 0) (void) ::close(root_fd);
        if (parent_fd >= 0) (void) ::close(parent_fd);
        if (target_fd >= 0) (void) ::close(target_fd);
        return finalize_receipt(
            receipt_parent_fd, receipt_parent_stat, receipt_parent_mount,
            receipt_name, 2);
    }
    (void) parent_mount;
    (void) ::close(fixture_lock_fd);
    if (!emit(std::string("\"event\":\"start\",\"boundary\":\"") +
              boundary_name(input.point) + "\",\"phase\":\"" +
              (input.when == phase::entry ? "entry" : "exit") +
              "\",\"tracee_umask\":" +
              std::to_string(static_cast<unsigned>(input.tracee_umask)))) {
        (void) ::close(target_fd);
        (void) ::close(fixture_fd);
        (void) ::close(root_fd);
        (void) ::close(parent_fd);
        return finalize_receipt(
            receipt_parent_fd, receipt_parent_stat, receipt_parent_mount,
            receipt_name, 2);
    }

    const pid_t launcher = ::fork();
    if (launcher < 0) {
        emit("\"event\":\"fork-failed\"");
        (void) ::close(target_fd);
        (void) ::close(fixture_fd);
        (void) ::close(root_fd);
        (void) ::close(parent_fd);
        return finalize_receipt(
            receipt_parent_fd, receipt_parent_stat, receipt_parent_mount,
            receipt_name, 2);
    }
    if (launcher == 0) {
        (void) ::umask(input.tracee_umask);
        if (::ptrace(PTRACE_TRACEME, 0, nullptr, nullptr) != 0) _exit(125);
        if (::raise(SIGSTOP) != 0) _exit(125);
        char executable[64];
        const int count = std::snprintf(executable, sizeof(executable),
                                        "/proc/self/fd/%d", target_fd);
        if (count <= 0 || static_cast<std::size_t>(count) >= sizeof(executable)) _exit(125);
        ::execl(executable, input.target, "--live-directory-controller", input.golden,
                input.parent, input.root, input.fixture, static_cast<char *>(nullptr));
        _exit(127);
    }
    (void) ::close(target_fd);

    int status = 0;
    if (::waitpid(launcher, &status, 0) != launcher || !WIFSTOPPED(status)) {
        emit("\"event\":\"initial-stop-failed\"");
        (void) ::kill(launcher, SIGKILL);
        (void) ::waitpid(launcher, nullptr, 0);
        (void) ::close(fixture_fd);
        (void) ::close(root_fd);
        (void) ::close(parent_fd);
        return finalize_receipt(
            receipt_parent_fd, receipt_parent_stat, receipt_parent_mount,
            receipt_name, 2);
    }
    constexpr long ptrace_options = PTRACE_O_TRACESYSGOOD | PTRACE_O_TRACEFORK |
        PTRACE_O_TRACEVFORK | PTRACE_O_TRACECLONE | PTRACE_O_TRACEEXEC | PTRACE_O_EXITKILL;
    if (::ptrace(PTRACE_SETOPTIONS, launcher, nullptr,
                 reinterpret_cast<void *>(ptrace_options)) != 0 || !resume_syscalls(launcher)) {
        emit("\"event\":\"ptrace-setup-failed\"");
        (void) ::kill(launcher, SIGKILL);
        (void) ::waitpid(launcher, nullptr, 0);
        (void) ::close(fixture_fd);
        (void) ::close(root_fd);
        (void) ::close(parent_fd);
        return finalize_receipt(
            receipt_parent_fd, receipt_parent_stat, receipt_parent_mount,
            receipt_name, 2);
    }

    std::unordered_map<pid_t, tracee_state> tracees;
    tracees.emplace(launcher, tracee_state {});
    std::array<pinned_directory, 3> dirs {};
    pid_t live_child = -1;
    int writer_fd_number = -1;
    int fixture_child_fd_number = -1;
    struct stat writer_identity {};
    std::uint64_t writer_mount = 0;
    bool writer_pinned = false;
    bool fixture_child_pinned = false;
    bool fixture_child_lock_acquired = false;
    bool writer_child_lock_acquired = false;
    bool injected = false;
    bool target_sigkilled = false;
    bool launcher_exited = false;
    bool launcher_wifexited = false;
    bool launcher_exec_seen = false;
    int launcher_exit = -1;
    bool controller_error = false;
    bool bounded_process_cleanup = false;
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
            if (pid == live_child && WIFSIGNALED(status) && WTERMSIG(status) == SIGKILL) {
                target_sigkilled = true;
                emit("\"event\":\"target-exit\",\"wifsignaled\":true,\"signal\":9");
            } else if (pid == launcher) {
                launcher_exited = true;
                launcher_wifexited = WIFEXITED(status);
                launcher_exit = WIFEXITED(status) ? WEXITSTATUS(status) : 128 + WTERMSIG(status);
                emit(std::string("\"event\":\"launcher-exit\",\"wifexited\":") +
                     (launcher_wifexited ? "true" : "false") + ",\"status\":" +
                     std::to_string(launcher_exit));
            }
            tracees.erase(pid);
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
                auto & child_state = tracees[static_cast<pid_t>(child_value)];
                child_state.initial_sigstop_expected =
                    !child_state.initial_sigstop_observed;
                emit(std::string("\"event\":\"descendant\",\"parent\":") +
                     std::to_string(pid) + ",\"pid\":" + std::to_string(child_value));
            } else if (event == PTRACE_EVENT_EXEC && pid == launcher) {
                if (launcher_exec_seen || !same_executable(pid, target_stat)) {
                    emit("\"event\":\"unexpected-launcher-exec\"");
                    controller_error = true;
                    break;
                }
                launcher_exec_seen = true;
                emit(std::string("\"event\":\"launcher-exec\",\"pid\":") +
                     std::to_string(pid));
            } else if (event == PTRACE_EVENT_EXEC) {
                if (live_child != -1 || !same_executable(pid, target_stat) ||
                    !exact_live_child_argv(pid)) {
                    emit("\"event\":\"unexpected-descendant-exec\"");
                    controller_error = true;
                    break;
                }
                live_child = pid;
                tracees[pid].live_child = true;
                emit(std::string("\"event\":\"live-child-exec\",\"pid\":") +
                     std::to_string(pid));
            }
            if (!resume_syscalls(pid)) { controller_error = true; break; }
            continue;
        }

        if (stop_signal == (SIGTRAP | 0x80)) {
            struct __ptrace_syscall_info info {};
            const long info_size = ::ptrace(PTRACE_GET_SYSCALL_INFO, pid, sizeof(info), &info);
            if (info_size < 0 || info.arch != AUDIT_ARCH_X86_64) {
                emit("\"event\":\"syscall-info-failed\"");
                controller_error = true;
                break;
            }
            auto & state = tracees[pid];
            if (info.op == PTRACE_SYSCALL_INFO_ENTRY) {
                state.have_entry = true;
                state.nr = info.entry.nr;
                std::memcpy(state.args, info.entry.args, sizeof(state.args));
                if (state.live_child) {
                    emit(std::string("\"event\":\"syscall-entry\",\"pid\":") +
                         std::to_string(pid) + ",\"nr\":" + std::to_string(state.nr) +
                         ",\"arg0\":" + std::to_string(state.args[0]) +
                         ",\"arg1\":" + std::to_string(state.args[1]) +
                         ",\"arg2\":" + std::to_string(state.args[2]) +
                         ",\"arg3\":" + std::to_string(state.args[3]) +
                         ",\"arg4\":" + std::to_string(state.args[4]) +
                         ",\"arg5\":" + std::to_string(state.args[5]));
                    const bool match = selected_entry(
                        pid, state, input.point, root_stat, root_mount, dirs);
                    if (match) {
                        emit(std::string("\"event\":\"boundary-entry\",\"name\":\"") +
                             boundary_name(input.point) + "\"");
                        if (input.when == phase::entry) {
                            injected = ::kill(pid, SIGKILL) == 0;
                            emit(std::string("\"event\":\"inject\",\"ok\":") +
                                 (injected ? "true" : "false"));
                        }
                    }
                }
            } else if (info.op == PTRACE_SYSCALL_INFO_EXIT) {
                if (state.live_child && state.have_entry) {
                    emit(std::string("\"event\":\"syscall-exit\",\"pid\":") +
                         std::to_string(pid) + ",\"nr\":" + std::to_string(state.nr) +
                         ",\"rval\":" + std::to_string(info.exit.rval) +
                         ",\"is_error\":" + (info.exit.is_error ? "true" : "false"));
                    const bool match = selected_entry(
                        pid, state, input.point, root_stat, root_mount, dirs);
                    if (state.nr == SYS_openat2 &&
                        writer_openat2_entry(pid, state, root_stat, root_mount) &&
                        !info.exit.is_error && info.exit.rval >= 0) {
                        writer_fd_number = static_cast<int>(info.exit.rval);
                        struct stat observed_writer {};
                        std::uint64_t observed_writer_mount = 0;
                        if (!tracee_fd_identity(pid, writer_fd_number, observed_writer,
                                               observed_writer_mount) ||
                            !S_ISREG(observed_writer.st_mode) || observed_writer.st_nlink != 1 ||
                            observed_writer.st_size != 0 || observed_writer.st_uid != ::geteuid() ||
                            observed_writer_mount != root_mount) {
                            emit("\"event\":\"writer-fd-identity-failed\"");
                            controller_error = true;
                            break;
                        }
                        writer_identity = observed_writer;
                        writer_mount = observed_writer_mount;
                        writer_pinned = true;
                    }
                    if (state.nr == SYS_openat2 && fixture_child_fd_number < 0 &&
                        fixture_openat2_entry(
                            pid, state, fixture_stat, fixture_mount) &&
                        !info.exit.is_error && info.exit.rval >= 0) {
                        fixture_child_fd_number = static_cast<int>(info.exit.rval);
                        struct stat observed_lock {};
                        std::uint64_t observed_mount = 0;
                        if (!tracee_fd_identity(pid, fixture_child_fd_number,
                                               observed_lock, observed_mount) ||
                            !S_ISREG(observed_lock.st_mode) ||
                            !same_object(fixture_lock_stat, observed_lock) ||
                            observed_mount != fixture_mount) {
                            emit("\"event\":\"fixture-child-identity-failed\"");
                            controller_error = true;
                            break;
                        }
                        fixture_child_pinned = true;
                    }
                    if (ofd_write_lock_entry(pid, state, fixture_child_fd_number,
                                             fixture_lock_stat, fixture_mount) &&
                        !info.exit.is_error && info.exit.rval == 0) {
                        fixture_child_lock_acquired = true;
                    }
                    if (ofd_write_lock_entry(pid, state, writer_fd_number,
                                             writer_identity, writer_mount) &&
                        !info.exit.is_error && info.exit.rval == 0) {
                        writer_child_lock_acquired = true;
                    }
                    if (state.nr == SYS_mkdirat && !info.exit.is_error && info.exit.rval == 0) {
                        std::string name;
                        if (read_tracee_string(pid, state.args[1], name)) {
                            for (int i = 0; i < 3; ++i) {
                                if (name == directory_names[static_cast<std::size_t>(i)] &&
                                    !pin_created_directory(root_fd, i,
                                        static_cast<mode_t>(0700 & ~input.tracee_umask),
                                        dirs[static_cast<std::size_t>(i)])) {
                                    emit("\"event\":\"created-directory-pin-failed\"");
                                    controller_error = true;
                                    break;
                                }
                            }
                        }
                        if (controller_error) break;
                    }
                    if (match && input.when == phase::exit && !info.exit.is_error &&
                        info.exit.rval == 0) {
                        emit(std::string("\"event\":\"boundary-exit\",\"name\":\"") +
                             boundary_name(input.point) + "\"");
                        injected = ::kill(pid, SIGKILL) == 0;
                        emit(std::string("\"event\":\"inject\",\"ok\":") +
                             (injected ? "true" : "false"));
                    }
                }
                state.have_entry = false;
            } else {
                emit("\"event\":\"unexpected-syscall-stop\"");
                controller_error = true;
                break;
            }
            if (!resume_syscalls(pid) && !(injected && errno == ESRCH)) controller_error = true;
            continue;
        }

        auto state_iterator = tracees.find(pid);
        if (stop_signal == SIGSTOP && state_iterator == tracees.end()) {
            tracee_state newborn {};
            newborn.initial_sigstop_observed = true;
            tracees.emplace(pid, newborn);
            emit(std::string("\"event\":\"newborn-initial-stop-first\",\"pid\":") +
                 std::to_string(pid));
            if (!resume_syscalls(pid)) controller_error = true;
            continue;
        }
        if (stop_signal == SIGSTOP && state_iterator != tracees.end() &&
            state_iterator->second.initial_sigstop_expected) {
            state_iterator->second.initial_sigstop_expected = false;
            state_iterator->second.initial_sigstop_observed = true;
            if (!resume_syscalls(pid)) controller_error = true;
            continue;
        }
        if (stop_signal == SIGSTOP || stop_signal == SIGTRAP) {
            controller_error = true;
            break;
        }
        if (!resume_syscalls(pid, stop_signal) && !(injected && errno == ESRCH)) {
            controller_error = true;
        }
    }

    if (!tracees.empty()) {
        bounded_process_cleanup = true;
        emit("\"event\":\"bounded-process-cleanup\"");
        for (const auto & item : tracees) (void) ::kill(item.first, SIGKILL);
        const auto reap_deadline = std::chrono::steady_clock::now() + std::chrono::seconds(2);
        while (!tracees.empty() && std::chrono::steady_clock::now() < reap_deadline) {
            const pid_t pid = ::waitpid(-1, &status, __WALL | WNOHANG);
            if (pid > 0) tracees.erase(pid);
            else if (pid == 0) {
                const struct timespec pause { 0, 1000000 };
                (void) ::nanosleep(&pause, nullptr);
            } else if (errno != EINTR) break;
        }
    }

    struct stat root_now {}, fixture_now {};
    std::uint64_t root_mount_now = 0;
    std::uint64_t fixture_mount_now = 0;
    const bool authorities_pinned = ::fstat(root_fd, &root_now) == 0 &&
        fd_mount_id(root_fd, root_mount_now) &&
        ::fstat(fixture_fd, &fixture_now) == 0 &&
        fd_mount_id(fixture_fd, fixture_mount_now) &&
        same_object(root_stat, root_now) && root_mount_now == root_mount &&
        root_now.st_uid == root_stat.st_uid &&
        (root_now.st_mode & 07777) == (root_stat.st_mode & 07777) &&
        same_object(fixture_stat, fixture_now) && fixture_mount_now == fixture_mount &&
        fixture_now.st_uid == fixture_stat.st_uid &&
        (fixture_now.st_mode & 07777) == (fixture_stat.st_mode & 07777);
    const int prefix = expected_prefix(input.point, input.when);
    const bool inventory_exact = authorities_pinned && writer_pinned &&
        inventory_root(root_fd, prefix, writer_identity, dirs,
                       input.point, input.when, input.tracee_umask);
    const bool fixture_released = prove_ofd_released(fixture_fd, "primitive.lock",
                                                     fixture_lock_stat);
    const bool writer_released = writer_pinned &&
        prove_ofd_released(root_fd, "writer.lock", writer_identity);
    emit(std::string("\"event\":\"post-crash\",\"authorities_pinned\":") +
         (authorities_pinned ? "true" : "false") + ",\"inventory_exact\":" +
         (inventory_exact ? "true" : "false") + ",\"fixture_lock_released\":" +
         (fixture_released ? "true" : "false") + ",\"writer_lock_released\":" +
         (writer_released ? "true" : "false") + ",\"prefix\":" +
         std::to_string(prefix));
    emit(std::string("\"event\":\"identity-inventory\",\"root_device\":") +
         std::to_string(static_cast<std::uint64_t>(root_stat.st_dev)) +
         ",\"root_inode\":" + std::to_string(static_cast<std::uint64_t>(root_stat.st_ino)) +
         ",\"root_mount_id\":" + std::to_string(root_mount) +
         ",\"root_uid\":" + std::to_string(static_cast<std::uint64_t>(root_stat.st_uid)) +
         ",\"root_mode\":" + std::to_string(static_cast<unsigned>(root_stat.st_mode & 07777)) +
         ",\"envelopes_device\":" + std::to_string(dirs[0].pinned ?
             static_cast<std::uint64_t>(dirs[0].identity.st_dev) : 0) +
         ",\"envelopes_inode\":" + std::to_string(dirs[0].pinned ?
             static_cast<std::uint64_t>(dirs[0].identity.st_ino) : 0) +
         ",\"envelopes_mount_id\":" + std::to_string(dirs[0].mount_id) +
         ",\"envelopes_uid\":" + std::to_string(dirs[0].pinned ?
             static_cast<std::uint64_t>(dirs[0].identity.st_uid) : 0) +
         ",\"envelopes_mode\":" + std::to_string(dirs[0].pinned ?
             static_cast<unsigned>(dirs[0].identity.st_mode & 07777) : 0) +
         ",\"envelopes_empty\":" + (inventory_exact && prefix > 0 ? "true" : "false") +
         ",\"attempts_device\":" + std::to_string(dirs[1].pinned ?
             static_cast<std::uint64_t>(dirs[1].identity.st_dev) : 0) +
         ",\"attempts_inode\":" + std::to_string(dirs[1].pinned ?
             static_cast<std::uint64_t>(dirs[1].identity.st_ino) : 0) +
         ",\"attempts_mount_id\":" + std::to_string(dirs[1].mount_id) +
         ",\"attempts_uid\":" + std::to_string(dirs[1].pinned ?
             static_cast<std::uint64_t>(dirs[1].identity.st_uid) : 0) +
         ",\"attempts_mode\":" + std::to_string(dirs[1].pinned ?
             static_cast<unsigned>(dirs[1].identity.st_mode & 07777) : 0) +
         ",\"attempts_empty\":" + (inventory_exact && prefix > 1 ? "true" : "false") +
         ",\"staging_device\":" + std::to_string(dirs[2].pinned ?
             static_cast<std::uint64_t>(dirs[2].identity.st_dev) : 0) +
         ",\"staging_inode\":" + std::to_string(dirs[2].pinned ?
             static_cast<std::uint64_t>(dirs[2].identity.st_ino) : 0) +
         ",\"staging_mount_id\":" + std::to_string(dirs[2].mount_id) +
         ",\"staging_uid\":" + std::to_string(dirs[2].pinned ?
             static_cast<std::uint64_t>(dirs[2].identity.st_uid) : 0) +
         ",\"staging_mode\":" + std::to_string(dirs[2].pinned ?
             static_cast<unsigned>(dirs[2].identity.st_mode & 07777) : 0) +
         ",\"staging_empty\":" + (inventory_exact && prefix > 2 ? "true" : "false"));

    const bool pass = !controller_error && receipt_ok && injected &&
        target_sigkilled && launcher_exec_seen && !bounded_process_cleanup &&
        launcher_exited && launcher_wifexited && launcher_exit != 0 && tracees.empty() &&
        fixture_child_pinned && fixture_child_lock_acquired &&
        writer_child_lock_acquired && inventory_exact && fixture_released && writer_released;
    emit(std::string("\"event\":\"summary\",\"pass\":") +
         (pass ? "true" : "false") + ",\"injected\":" +
         (injected ? "true" : "false") + ",\"target_wifsignaled\":" +
         (target_sigkilled ? "true" : "false") + ",\"launcher_status\":" +
         std::to_string(launcher_exit));

    const bool fixture_closed = ::close(fixture_fd) == 0;
    const bool root_closed = ::close(root_fd) == 0;
    const bool parent_closed = ::close(parent_fd) == 0;
    const int intended_status = pass && fixture_closed && root_closed && parent_closed ? 0 : 1;
    return finalize_receipt(receipt_parent_fd, receipt_parent_stat,
                            receipt_parent_mount, receipt_name, intended_status);
}

} // namespace

int main(int argc, char ** argv) {
    options input {};
    if (!parse(argc, argv, input)) usage(argv[0]);
    return controller(input);
}
