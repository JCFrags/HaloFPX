#if !defined(__linux__) || !defined(__x86_64__)
#error "The L05w ptrace controller currently requires Linux x86-64"
#endif

#include <cerrno>
#include <chrono>
#include <cinttypes>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fcntl.h>
#include <linux/audit.h>
#include <linux/openat2.h>
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

enum class boundary { openat2_writer_lock, fchmod_writer_lock,
                      fsync_writer_lock, ofd_setlk_writer_lock };
enum class phase { entry, exit };

struct options {
    const char * target = nullptr;
    const char * golden = nullptr;
    const char * parent = nullptr;
    const char * root = nullptr;
    const char * fixture = nullptr;
    const char * receipt = nullptr;
    boundary point = boundary::openat2_writer_lock;
    phase when = phase::entry;
    bool point_set = false;
    bool phase_set = false;
};

struct tracee_state {
    bool live_child = false;
    bool initial_sigstop_expected = false;
    bool have_entry = false;
    std::uint64_t nr = 0;
    std::uint64_t args[6] {};
};

int receipt_fd = -1;
std::uint64_t receipt_sequence = 0;
bool receipt_ok = true;

[[noreturn]] void usage(const char * program) {
    std::fprintf(stderr,
        "usage: %s --target PATH --golden PATH --parent PATH --root PATH "
        "--fixture PATH --boundary openat2|fchmod|fsync|ofd-setlk "
        "--phase entry|exit --receipt NEW-PATH\n", program);
    std::exit(2);
}

bool set_once(const char *& output, const char * value) {
    if (output != nullptr || value == nullptr || value[0] != '/') {
        return false;
    }
    output = value;
    return true;
}

bool parse(int argc, char ** argv, options & output) {
    for (int i = 1; i < argc; i += 2) {
        if (i + 1 >= argc) {
            return false;
        }
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
            if (output.point_set) return false;
            if (std::strcmp(value, "openat2") == 0) {
                output.point = boundary::openat2_writer_lock;
            } else if (std::strcmp(value, "fchmod") == 0) {
                output.point = boundary::fchmod_writer_lock;
            } else if (std::strcmp(value, "fsync") == 0) {
                output.point = boundary::fsync_writer_lock;
            } else if (std::strcmp(value, "ofd-setlk") == 0) {
                output.point = boundary::ofd_setlk_writer_lock;
            } else return false;
            output.point_set = true;
        } else if (std::strcmp(key, "--phase") == 0) {
            if (output.phase_set) return false;
            if (std::strcmp(value, "entry") == 0) output.when = phase::entry;
            else if (std::strcmp(value, "exit") == 0) output.when = phase::exit;
            else return false;
            output.phase_set = true;
        } else return false;
    }
    return output.target && output.golden && output.parent && output.root &&
           output.fixture && output.receipt && output.point_set && output.phase_set;
}

bool same_or_descendant(const char * candidate, const char * directory) {
    const std::size_t size = std::strlen(directory);
    return size != 0 && std::strncmp(candidate, directory, size) == 0 &&
           (candidate[size] == '\0' || directory[size - 1] == '/' ||
            candidate[size] == '/');
}

const char * boundary_name(boundary value) {
    switch (value) {
        case boundary::openat2_writer_lock: return "openat2(writer.lock)";
        case boundary::fchmod_writer_lock: return "fchmod(writer.lock)";
        case boundary::fsync_writer_lock: return "fsync(writer.lock)";
        case boundary::ofd_setlk_writer_lock: return "F_OFD_SETLK(writer.lock)";
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
        const std::size_t copied = size - offset < sizeof(word)
            ? size - offset : sizeof(word);
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
           ::stat(path, &observed) == 0 &&
           expected.st_dev == observed.st_dev && expected.st_ino == observed.st_ino;
}

bool same_object(const struct stat & expected, const struct stat & observed) {
    return expected.st_dev == observed.st_dev && expected.st_ino == observed.st_ino;
}

bool stat_tracee_fd(pid_t pid, int fd, struct stat & output) {
    char path[96];
    const int count = std::snprintf(path, sizeof(path), "/proc/%ld/fd/%d",
                                    static_cast<long>(pid), fd);
    return count > 0 && static_cast<std::size_t>(count) < sizeof(path) &&
           ::stat(path, &output) == 0;
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
    constexpr char expected[] = "halofpx-l05w-live-child\0--live-child\0";
    return closed && count == static_cast<ssize_t>(sizeof(expected) - 1) &&
           std::memcmp(bytes, expected, sizeof(expected) - 1) == 0;
}

bool writer_openat2_entry(pid_t pid, const tracee_state & state,
                          const struct stat & expected_root) {
    if (state.nr != SYS_openat2) return false;
    std::string name;
    struct open_how how {};
    struct stat root {};
    if (!read_tracee_string(pid, state.args[1], name) || name != "writer.lock" ||
        state.args[3] != sizeof(how) || !read_tracee(pid, state.args[2], &how, sizeof(how))) {
        return false;
    }
    constexpr std::uint64_t exact_flags =
        O_CREAT | O_EXCL | O_RDWR | O_CLOEXEC | O_NOFOLLOW;
    constexpr std::uint64_t exact_resolve =
        RESOLVE_BENEATH | RESOLVE_NO_MAGICLINKS | RESOLVE_NO_SYMLINKS | RESOLVE_NO_XDEV;
    return stat_tracee_fd(pid, static_cast<int>(state.args[0]), root) &&
           same_object(expected_root, root) && S_ISDIR(root.st_mode) &&
           how.flags == exact_flags && how.mode == 0600 && how.resolve == exact_resolve;
}

bool selected_entry(pid_t pid, const tracee_state & state, boundary point,
                    int writer_fd, const struct stat & expected_root) {
    switch (point) {
        case boundary::openat2_writer_lock:
            return writer_openat2_entry(pid, state, expected_root);
        case boundary::fchmod_writer_lock:
            return writer_fd >= 0 && state.nr == SYS_fchmod &&
                   static_cast<int>(state.args[0]) == writer_fd && state.args[1] == 0600;
        case boundary::fsync_writer_lock:
            return writer_fd >= 0 && state.nr == SYS_fsync &&
                   static_cast<int>(state.args[0]) == writer_fd;
        case boundary::ofd_setlk_writer_lock:
            if (writer_fd >= 0 && state.nr == SYS_fcntl &&
                static_cast<int>(state.args[0]) == writer_fd &&
                static_cast<int>(state.args[1]) == F_OFD_SETLK) {
                struct flock lock {};
                return read_tracee(pid, state.args[2], &lock, sizeof(lock)) &&
                       lock.l_type == F_WRLCK && lock.l_whence == SEEK_SET &&
                       lock.l_start == 0 && lock.l_len == 0;
            }
            return false;
    }
    return false;
}

bool resume_syscalls(pid_t pid, int signal_number = 0) {
    return ::ptrace(PTRACE_SYSCALL, pid, nullptr,
                    reinterpret_cast<void *>(static_cast<intptr_t>(signal_number))) == 0;
}

bool prove_ofd_released(const char * directory, const struct stat & expected_directory,
                        const char * name, const struct stat * expected_file,
                        bool allow_missing, bool & present) {
    present = false;
    const int directory_fd = ::open(
        directory, O_RDONLY | O_DIRECTORY | O_CLOEXEC | O_NOFOLLOW);
    struct stat directory_identity {};
    if (directory_fd < 0 || ::fstat(directory_fd, &directory_identity) != 0 ||
        !S_ISDIR(directory_identity.st_mode) ||
        !same_object(expected_directory, directory_identity)) {
        if (directory_fd >= 0) (void) ::close(directory_fd);
        return false;
    }
    struct open_how how {};
    how.flags = O_RDWR | O_CLOEXEC | O_NOFOLLOW;
    how.resolve = RESOLVE_BENEATH | RESOLVE_NO_MAGICLINKS |
                  RESOLVE_NO_SYMLINKS | RESOLVE_NO_XDEV;
    int fd;
    do {
        fd = static_cast<int>(::syscall(
            SYS_openat2, directory_fd, name, &how, sizeof(how)));
    } while (fd < 0 && errno == EINTR);
    if (fd < 0) {
        const bool missing = errno == ENOENT;
        const bool closed = ::close(directory_fd) == 0;
        return allow_missing && missing && closed;
    }
    present = true;
    struct stat file_identity {};
    if (::fstat(fd, &file_identity) != 0 || !S_ISREG(file_identity.st_mode) ||
        expected_file == nullptr || !same_object(*expected_file, file_identity) ||
        file_identity.st_nlink != expected_file->st_nlink ||
        file_identity.st_size != expected_file->st_size ||
        file_identity.st_uid != expected_file->st_uid ||
        (file_identity.st_mode & 07777) != (expected_file->st_mode & 07777)) {
        (void) ::close(fd);
        (void) ::close(directory_fd);
        return false;
    }
    struct flock lock {};
    lock.l_type = F_WRLCK;
    lock.l_whence = SEEK_SET;
    lock.l_start = 0;
    lock.l_len = 0;
    int acquired;
    do { acquired = ::fcntl(fd, F_OFD_SETLK, &lock); }
    while (acquired < 0 && errno == EINTR);
    bool unlocked = false;
    if (acquired == 0) {
        lock.l_type = F_UNLCK;
        int result;
        do { result = ::fcntl(fd, F_OFD_SETLK, &lock); }
        while (result < 0 && errno == EINTR);
        unlocked = result == 0;
    }
    const bool file_closed = ::close(fd) == 0;
    const bool directory_closed = ::close(directory_fd) == 0;
    return acquired == 0 && unlocked && file_closed && directory_closed;
}

int controller(const options & input) {
    // Evidence must never become a mutation inside the qualification authority
    // tree. The caller creates and pins its evidence directory separately.
    if (same_or_descendant(input.receipt, input.parent)) {
        std::fprintf(stderr, "receipt must be outside the authority parent\n");
        return 2;
    }
    receipt_ok = true;
    receipt_sequence = 0;
    receipt_fd = ::open(input.receipt,
        O_WRONLY | O_CREAT | O_EXCL | O_CLOEXEC | O_NOFOLLOW, 0600);
    if (receipt_fd < 0) {
        std::perror("open receipt");
        return 2;
    }
    const int target_fd = ::open(input.target, O_RDONLY | O_CLOEXEC | O_NOFOLLOW);
    struct stat target_stat {};
    struct stat root_stat {};
    struct stat fixture_stat {};
    struct stat fixture_lock_stat {};
    const int root_fd = ::open(input.root, O_RDONLY | O_DIRECTORY | O_CLOEXEC | O_NOFOLLOW);
    const int fixture_fd = ::open(input.fixture, O_RDONLY | O_DIRECTORY | O_CLOEXEC | O_NOFOLLOW);
    const int fixture_lock_fd = fixture_fd >= 0
        ? ::openat(fixture_fd, "primitive.lock", O_RDONLY | O_CLOEXEC | O_NOFOLLOW) : -1;
    if (target_fd < 0 || ::fstat(target_fd, &target_stat) != 0 ||
        !S_ISREG(target_stat.st_mode) || (target_stat.st_mode & 0111) == 0 ||
        root_fd < 0 || ::fstat(root_fd, &root_stat) != 0 || !S_ISDIR(root_stat.st_mode) ||
        fixture_fd < 0 || ::fstat(fixture_fd, &fixture_stat) != 0 || !S_ISDIR(fixture_stat.st_mode) ||
        fixture_lock_fd < 0 || ::fstat(fixture_lock_fd, &fixture_lock_stat) != 0 ||
        !S_ISREG(fixture_lock_stat.st_mode) || fixture_lock_stat.st_nlink != 1 ||
        fixture_lock_stat.st_size != 0 || fixture_lock_stat.st_uid != ::geteuid() ||
        static_cast<unsigned>(fixture_lock_stat.st_mode & 07777) != 0600) {
        emit("\"event\":\"invalid-target\"");
        if (fixture_lock_fd >= 0) (void) ::close(fixture_lock_fd);
        if (fixture_fd >= 0) (void) ::close(fixture_fd);
        if (root_fd >= 0) (void) ::close(root_fd);
        if (target_fd >= 0) (void) ::close(target_fd);
        ::close(receipt_fd);
        return 2;
    }
    (void) ::close(fixture_lock_fd);
    (void) ::close(fixture_fd);
    (void) ::close(root_fd);
    if (!emit(std::string("\"event\":\"start\",\"boundary\":\"") +
         boundary_name(input.point) + "\",\"phase\":\"" +
         (input.when == phase::entry ? "entry" : "exit") + "\"")) {
        (void) ::close(target_fd);
        (void) ::close(receipt_fd);
        return 2;
    }

    const pid_t launcher = ::fork();
    if (launcher < 0) {
        emit("\"event\":\"fork-failed\"");
        ::close(receipt_fd);
        return 2;
    }
    if (launcher == 0) {
        if (::ptrace(PTRACE_TRACEME, 0, nullptr, nullptr) != 0) _exit(125);
        if (::raise(SIGSTOP) != 0) _exit(125);
        char executable[64];
        const int count = std::snprintf(executable, sizeof(executable),
                                        "/proc/self/fd/%d", target_fd);
        if (count <= 0 || static_cast<std::size_t>(count) >= sizeof(executable)) _exit(125);
        ::execl(executable, input.target, "--live-controller", input.golden,
                input.parent, input.root, input.fixture, static_cast<char *>(nullptr));
        _exit(127);
    }
    (void) ::close(target_fd);

    int status = 0;
    if (::waitpid(launcher, &status, 0) != launcher || !WIFSTOPPED(status)) {
        emit("\"event\":\"initial-stop-failed\"");
        (void) ::kill(launcher, SIGKILL);
        (void) ::waitpid(launcher, nullptr, 0);
        ::close(receipt_fd);
        return 2;
    }
    constexpr long ptrace_options = PTRACE_O_TRACESYSGOOD | PTRACE_O_TRACEFORK |
        PTRACE_O_TRACEVFORK | PTRACE_O_TRACECLONE | PTRACE_O_TRACEEXEC |
        PTRACE_O_EXITKILL;
    if (::ptrace(PTRACE_SETOPTIONS, launcher, nullptr,
                 reinterpret_cast<void *>(ptrace_options)) != 0 ||
        !resume_syscalls(launcher)) {
        emit("\"event\":\"ptrace-setup-failed\"");
        (void) ::kill(launcher, SIGKILL);
        (void) ::waitpid(launcher, nullptr, 0);
        ::close(receipt_fd);
        return 2;
    }

    std::unordered_map<pid_t, tracee_state> tracees;
    tracees.emplace(launcher, tracee_state {});
    pid_t live_child = -1;
    int writer_fd = -1;
    struct stat writer_identity {};
    bool writer_identity_pinned = false;
    bool injected = false;
    bool target_sigkilled = false;
    bool launcher_exited = false;
    bool launcher_exec_seen = false;
    int launcher_exit = -1;
    bool controller_error = false;
    bool bounded_cleanup_used = false;
    const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(20);

    while (!tracees.empty() && std::chrono::steady_clock::now() < deadline) {
        const pid_t pid = ::waitpid(-1, &status, __WALL | WNOHANG);
        if (pid == 0) {
            struct timespec pause { 0, 1000000 };
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
                launcher_exit = WIFEXITED(status) ? WEXITSTATUS(status) : 128 + WTERMSIG(status);
                emit(std::string("\"event\":\"launcher-exit\",\"status\":") +
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
                child_state.initial_sigstop_expected = true;
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
            const long info_size = ::ptrace(PTRACE_GET_SYSCALL_INFO, pid,
                sizeof(info), &info);
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
                        pid, state, input.point, writer_fd, root_stat);
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
                        pid, state, input.point, writer_fd, root_stat);
                    if (state.nr == SYS_openat2 &&
                        writer_openat2_entry(pid, state, root_stat) &&
                        !info.exit.is_error && info.exit.rval >= 0) {
                        writer_fd = static_cast<int>(info.exit.rval);
                        struct stat observed_writer {};
                        if (!stat_tracee_fd(pid, writer_fd, observed_writer) ||
                            !S_ISREG(observed_writer.st_mode) || observed_writer.st_nlink != 1 ||
                            observed_writer.st_size != 0 || observed_writer.st_uid != ::geteuid()) {
                            emit("\"event\":\"writer-fd-identity-failed\"");
                            controller_error = true;
                            break;
                        }
                        writer_identity = observed_writer;
                        writer_identity_pinned = true;
                        emit(std::string("\"event\":\"writer-fd\",\"fd\":") +
                             std::to_string(writer_fd));
                    }
                    if (state.nr == SYS_fchmod && writer_fd >= 0 &&
                        static_cast<int>(state.args[0]) == writer_fd &&
                        !info.exit.is_error && info.exit.rval == 0) {
                        struct stat observed_writer {};
                        if (!stat_tracee_fd(pid, writer_fd, observed_writer) ||
                            !writer_identity_pinned ||
                            !same_object(writer_identity, observed_writer)) {
                            emit("\"event\":\"writer-fchmod-identity-failed\"");
                            controller_error = true;
                            break;
                        }
                        writer_identity = observed_writer;
                    }
                    if (match && input.when == phase::exit && !info.exit.is_error) {
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
            if (!resume_syscalls(pid)) {
                if (!(injected && errno == ESRCH)) controller_error = true;
            }
            continue;
        }

        auto state_iterator = tracees.find(pid);
        if (stop_signal == SIGSTOP && state_iterator != tracees.end() &&
            state_iterator->second.initial_sigstop_expected) {
            state_iterator->second.initial_sigstop_expected = false;
            emit(std::string("\"event\":\"descendant-initial-stop\",\"pid\":") +
                 std::to_string(pid));
            if (!resume_syscalls(pid)) controller_error = true;
            continue;
        }
        emit(std::string("\"event\":\"signal-delivery-stop\",\"pid\":") +
             std::to_string(pid) + ",\"signal\":" + std::to_string(stop_signal));
        if (stop_signal == SIGSTOP || stop_signal == SIGTRAP) {
            controller_error = true;
            break;
        }
        if (!resume_syscalls(pid, stop_signal)) {
            if (!(injected && errno == ESRCH)) controller_error = true;
        }
    }

    if (!tracees.empty()) {
        bounded_cleanup_used = true;
        emit("\"event\":\"bounded-cleanup\"");
        for (const auto & item : tracees) (void) ::kill(item.first, SIGKILL);
        const auto reap_deadline = std::chrono::steady_clock::now() + std::chrono::seconds(2);
        while (!tracees.empty() && std::chrono::steady_clock::now() < reap_deadline) {
            const pid_t pid = ::waitpid(-1, &status, __WALL | WNOHANG);
            if (pid > 0) tracees.erase(pid);
            else if (pid == 0) {
                struct timespec pause { 0, 1000000 };
                (void) ::nanosleep(&pause, nullptr);
            } else if (errno != EINTR) break;
        }
    }
    bool fixture_present = false;
    bool writer_present = false;
    const bool fixture_released = tracees.empty() &&
        prove_ofd_released(input.fixture, fixture_stat, "primitive.lock",
                           &fixture_lock_stat, false, fixture_present);
    const bool writer_released = tracees.empty() &&
        prove_ofd_released(input.root, root_stat, "writer.lock",
                           writer_identity_pinned ? &writer_identity : nullptr,
                           true, writer_present);
    const bool writer_expected = !(input.point == boundary::openat2_writer_lock &&
                                   input.when == phase::entry);
    const bool lock_release_proved = fixture_released && fixture_present &&
                                     writer_released && writer_present == writer_expected;
    emit(std::string("\"event\":\"ofd-release\",\"fixture\":") +
         (fixture_released && fixture_present ? "true" : "false") +
         ",\"writer_present\":" + (writer_present ? "true" : "false") +
         ",\"writer\":" + (writer_released ? "true" : "false"));
    const bool pass = !controller_error && receipt_ok && injected && target_sigkilled &&
                      launcher_exec_seen && !bounded_cleanup_used &&
                      launcher_exited && launcher_exit != 0 && tracees.empty() &&
                      lock_release_proved;
    emit(std::string("\"event\":\"summary\",\"pass\":") +
         (pass ? "true" : "false") + ",\"injected\":" +
         (injected ? "true" : "false") + ",\"target_sigkill\":" +
         (target_sigkilled ? "true" : "false") + ",\"launcher_status\":" +
         std::to_string(launcher_exit));
    const bool synced = ::fsync(receipt_fd) == 0;
    const bool closed = ::close(receipt_fd) == 0;
    receipt_fd = -1;
    return pass && receipt_ok && synced && closed ? 0 : 1;
}

} // namespace

int main(int argc, char ** argv) {
    options input {};
    if (!parse(argc, argv, input)) usage(argv[0]);
    return controller(input);
}
