#include "halofpx-context-store-registry-lab-linux-preinit-internal.h"

#include <array>
#ifdef NDEBUG
#undef NDEBUG
#endif
#include <cassert>
#include <cerrno>
#include <climits>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <limits>
#include <thread>

#include <dirent.h>
#include <fcntl.h>
#include <linux/memfd.h>
#include <linux/openat2.h>
#include <poll.h>
#include <sys/random.h>
#include <sys/prctl.h>
#include <sys/stat.h>
#include <sys/syscall.h>
#include <sys/wait.h>
#include <unistd.h>

using namespace halofpx::registry_lab::linux_preinit;

namespace {

enum class test_case : unsigned {
    malformed_magic,
    missing_credential,
    forbidden_fd4,
    missing_seals,
    same_process_alias,
    wrong_key_tuple,
    valid_credential_then_missing_root,
};

void write_u16_be(std::uint8_t * output, std::uint16_t value) {
    output[0] = static_cast<std::uint8_t>(value >> 8);
    output[1] = static_cast<std::uint8_t>(value);
}

void write_u64_be(std::uint8_t * output, std::uint64_t value) {
    for (int i = 7; i >= 0; --i) {
        output[i] = static_cast<std::uint8_t>(value);
        value >>= 8;
    }
}

request make_request() {
    request value{};
    const auto set_path = [](pinned_path_identity & identity, const char * path,
                             std::uint64_t inode, std::uint32_t mode) {
        identity.path_length = static_cast<std::uint32_t>(std::strlen(path));
        std::memcpy(identity.canonical_path, path, identity.path_length + 1);
        identity.device = 17;
        identity.inode = inode;
        identity.mount_id = 23;
        identity.owner_uid = static_cast<std::uint64_t>(::geteuid());
        identity.mode = mode;
        for (std::size_t i = 0; i < 16; ++i) {
            identity.filesystem_uuid[i] = static_cast<std::uint8_t>(i + 1);
            identity.subvolume_uuid[i] = static_cast<std::uint8_t>(i + 33);
        }
    };
    set_path(value.parent, "/var/tmp/halofpx-l05s-nonexistent-parent", 101, 0700);
    set_path(value.candidate_root,
             "/var/tmp/halofpx-l05s-nonexistent-parent/candidate", 102, 0700);
    set_path(value.fixture,
             "/var/tmp/halofpx-l05s-nonexistent-parent/fixture", 103, 0700);
    value.fixture_lock_device = value.fixture.device;
    value.fixture_lock_inode = 104;
    constexpr char key_id[] = "halofpx-l05s-public-test-key";
    value.expected_key_id_length = sizeof(key_id) - 1;
    std::memcpy(value.expected_key_id, key_id, sizeof(key_id));
    value.expected_key_generation = 7;
    return value;
}

std::array<std::uint8_t, 16 + 2 + max_key_id_bytes + 8 + 2 + 32>
make_package(const request & input, const std::uint8_t secret[32], std::size_t & size) {
    std::array<std::uint8_t, 16 + 2 + max_key_id_bytes + 8 + 2 + 32> bytes{};
    constexpr std::uint8_t magic[16] = {
        'H', 'a', 'l', 'o', 'F', 'P', 'X', 'R', 'e', 'g', 'K', 'e', 'y', '0', '1', 0,
    };
    std::memcpy(bytes.data(), magic, sizeof(magic));
    std::size_t offset = sizeof(magic);
    write_u16_be(bytes.data() + offset, input.expected_key_id_length);
    offset += 2;
    std::memcpy(bytes.data() + offset, input.expected_key_id,
                input.expected_key_id_length);
    offset += input.expected_key_id_length;
    write_u64_be(bytes.data() + offset, input.expected_key_generation);
    offset += 8;
    write_u16_be(bytes.data() + offset, 32);
    offset += 2;
    std::memcpy(bytes.data() + offset, secret, 32);
    size = offset + 32;
    return bytes;
}

bool write_complete(int fd, const std::uint8_t * bytes, std::size_t size) {
    std::size_t offset = 0;
    while (offset < size) {
        const ssize_t count = ::write(fd, bytes + offset, size - offset);
        if (count < 0 && errno == EINTR) {
            continue;
        }
        if (count <= 0) {
            return false;
        }
        offset += static_cast<std::size_t>(count);
    }
    return true;
}

int install_credential(const request & input, bool malformed, bool seal) {
    int fd = static_cast<int>(::syscall(SYS_memfd_create,
        "halofpx-registry-lab-credential", MFD_ALLOW_SEALING | MFD_CLOEXEC));
    if (fd < 0) {
        return -1;
    }
    std::size_t size = 0;
    std::array<std::uint8_t, 32> secret{};
    for (std::size_t i = 0; i < secret.size(); ++i) {
        secret[i] = static_cast<std::uint8_t>(0xa0 + i);
    }
    auto bytes = make_package(input, secret.data(), size);
    if (malformed) {
        bytes[0] ^= 0x80;
    }
    if (!write_complete(fd, bytes.data(), size)) {
        ::close(fd);
        return -1;
    }
    if (seal && ::fcntl(fd, F_ADD_SEALS,
                        F_SEAL_SEAL | F_SEAL_SHRINK | F_SEAL_GROW | F_SEAL_WRITE) != 0) {
        ::close(fd);
        return -1;
    }
    if (fd != 3) {
        if (::dup2(fd, 3) != 3) {
            ::close(fd);
            return -1;
        }
        ::close(fd);
    }
    return 3;
}

int run_child(test_case which) {
    ::close(3);
    ::close(4);
    ::close(5);
    request input = make_request();
    bool expect_credential = false;
    bool expect_root = false;

    if (which != test_case::missing_credential) {
        const bool malformed = which == test_case::malformed_magic;
        const bool seal = which != test_case::missing_seals;
        if (install_credential(input, malformed, seal) < 0) {
            return 90;
        }
    }
    if (which == test_case::forbidden_fd4 && ::dup2(3, 4) != 4) {
        return 91;
    }
    if (which == test_case::same_process_alias && ::dup2(3, 5) != 5) {
        return 92;
    }
    if (which == test_case::wrong_key_tuple) {
        input.expected_key_generation += 1;
    }
    if (which == test_case::valid_credential_then_missing_root) {
        expect_credential = true;
        expect_root = true;
    }

    const audit observed = qualify_once(input);
    if (observed.result != status::invalid_request ||
        observed.credential_admitted != expect_credential ||
        observed.credential_preceded_root_access != expect_credential ||
        (observed.root_fixture_syscall_count != 0) != expect_root ||
        !observed.credential_scratch_wiped || !observed.credential_owner_wiped ||
        !observed.credential_owner_unlocked || observed.logical_authority_bound !=
            future_logical_authority_bound || ::fcntl(3, F_GETFD) != -1 || errno != EBADF) {
        return 93;
    }
    return 0;
}

bool parse_u64(const char * text, int base, std::uint64_t & output) {
    if (text == nullptr || *text == '\0' || *text == '-') {
        return false;
    }
    char * end = nullptr;
    errno = 0;
    const unsigned long long value = std::strtoull(text, &end, base);
    if (errno != 0 || end == text || *end != '\0' ||
        value > std::numeric_limits<std::uint64_t>::max()) {
        return false;
    }
    output = static_cast<std::uint64_t>(value);
    return true;
}

bool parse_uuid(const char * text, std::uint8_t output[16]) {
    if (text == nullptr || std::strlen(text) != 32) {
        return false;
    }
    const auto hex = [](char value) -> int {
        if (value >= '0' && value <= '9') return value - '0';
        if (value >= 'a' && value <= 'f') return value - 'a' + 10;
        if (value >= 'A' && value <= 'F') return value - 'A' + 10;
        return -1;
    };
    for (std::size_t i = 0; i < 16; ++i) {
        const int high = hex(text[i * 2]);
        const int low = hex(text[i * 2 + 1]);
        if (high < 0 || low < 0) {
            return false;
        }
        output[i] = static_cast<std::uint8_t>((high << 4) | low);
    }
    return true;
}

bool parse_identity(char ** argv, int start, pinned_path_identity & output) {
    const std::size_t path_size = std::strlen(argv[start]);
    std::uint64_t mode = 0;
    if (path_size == 0 || path_size > max_path_bytes ||
        !parse_u64(argv[start + 1], 10, output.device) ||
        !parse_u64(argv[start + 2], 10, output.inode) ||
        !parse_u64(argv[start + 3], 10, output.mount_id) ||
        !parse_u64(argv[start + 4], 10, output.owner_uid) ||
        !parse_u64(argv[start + 5], 8, mode) || mode > 07777 ||
        !parse_uuid(argv[start + 6], output.filesystem_uuid) ||
        !parse_uuid(argv[start + 7], output.subvolume_uuid)) {
        return false;
    }
    output.path_length = static_cast<std::uint32_t>(path_size);
    std::memcpy(output.canonical_path, argv[start], path_size + 1);
    output.mode = static_cast<std::uint32_t>(mode);
    return true;
}

const char * status_name(status value) {
    switch (value) {
        case status::ok_non_authoritative: return "ok_non_authoritative";
        case status::invalid_request: return "invalid_request";
        case status::unsupported: return "unsupported";
        case status::busy: return "busy";
        case status::unavailable: return "unavailable";
        case status::io_failure: return "io_failure";
    }
    return "invalid-status";
}

bool parse_status(const char * text, status & output) {
    constexpr status values[] = {
        status::ok_non_authoritative, status::invalid_request, status::unsupported,
        status::busy, status::unavailable, status::io_failure,
    };
    for (status value : values) {
        if (std::strcmp(text, status_name(value)) == 0) {
            output = value;
            return true;
        }
    }
    return false;
}

bool parse_public_request(int argc, char ** argv, status & expected, request & input) {
    if (argc != 31) {
        return false;
    }
    std::uint64_t generation = 0;
    const std::size_t key_size = std::strlen(argv[29]);
    if (!parse_status(argv[2], expected) ||
        !parse_identity(argv, 3, input.parent) ||
        !parse_identity(argv, 11, input.candidate_root) ||
        !parse_identity(argv, 19, input.fixture) ||
        !parse_u64(argv[27], 10, input.fixture_lock_device) ||
        !parse_u64(argv[28], 10, input.fixture_lock_inode) ||
        key_size == 0 || key_size > max_key_id_bytes ||
        !parse_u64(argv[30], 10, generation) || generation == 0) {
        return false;
    }
    input.expected_key_id_length = static_cast<std::uint16_t>(key_size);
    std::memcpy(input.expected_key_id, argv[29], key_size + 1);
    input.expected_key_generation = generation;
    return true;
}

void wipe_test_bytes(void * data, std::size_t size) {
    volatile auto * bytes = static_cast<volatile std::uint8_t *>(data);
    while (size-- != 0) {
        *bytes++ = 0;
    }
}

bool fill_random(std::uint8_t * output, std::size_t size) {
    std::size_t offset = 0;
    while (offset < size) {
        const ssize_t count = ::getrandom(output + offset, size - offset, 0);
        if (count < 0 && errno == EINTR) {
            continue;
        }
        if (count <= 0) {
            return false;
        }
        offset += static_cast<std::size_t>(count);
    }
    return true;
}

bool no_launcher_alias(int credential) {
    struct stat expected {};
    if (::fstat(credential, &expected) != 0) {
        return false;
    }
    DIR * directory = ::opendir("/proc/self/fd");
    if (directory == nullptr) {
        return false;
    }
    const int scan_fd = ::dirfd(directory);
    std::size_t count = 0;
    bool result = true;
    for (;;) {
        errno = 0;
        const dirent * entry = ::readdir(directory);
        if (entry == nullptr) {
            if (errno != 0) {
                result = false;
            }
            break;
        }
        if (++count > 4096) {
            result = false;
            break;
        }
        char * end = nullptr;
        errno = 0;
        const long candidate = std::strtol(entry->d_name, &end, 10);
        if (errno != 0 || end == entry->d_name || *end != '\0' || candidate < 0 ||
            candidate > std::numeric_limits<int>::max() || candidate == credential ||
            candidate == scan_fd) {
            continue;
        }
        struct stat observed {};
        if (::fstat(static_cast<int>(candidate), &observed) == 0 &&
            observed.st_dev == expected.st_dev && observed.st_ino == expected.st_ino) {
            result = false;
            break;
        }
    }
    if (::closedir(directory) != 0) {
        result = false;
    }
    return result;
}

int run_self_launcher(int argc, char ** argv, const char * exec_mode) {
    status expected = status::invalid_request;
    request input{};
    if (!parse_public_request(argc, argv, expected, input)) {
        return 64;
    }
    (void) expected;
    std::array<std::uint8_t, 32> secret{};
    if (!fill_random(secret.data(), secret.size())) {
        wipe_test_bytes(secret.data(), secret.size());
        return 70;
    }
    std::size_t package_size = 0;
    auto package = make_package(input, secret.data(), package_size);

    ::close(3);
    ::close(4);
    int memfd = static_cast<int>(::syscall(SYS_memfd_create,
        "halofpx-registry-lab-credential", MFD_ALLOW_SEALING | MFD_CLOEXEC));
    bool ready = memfd >= 0 && write_complete(memfd, package.data(), package_size) &&
                 ::fcntl(memfd, F_ADD_SEALS,
                         F_SEAL_SEAL | F_SEAL_SHRINK | F_SEAL_GROW | F_SEAL_WRITE) == 0;
    if (ready && memfd != 3) {
        ready = ::dup3(memfd, 3, 0) == 3;
        const bool closed = ::close(memfd) == 0;
        memfd = 3;
        ready = ready && closed;
    } else if (ready) {
        const int flags = ::fcntl(3, F_GETFD);
        ready = flags >= 0 && ::fcntl(3, F_SETFD, flags & ~FD_CLOEXEC) == 0;
    }
    if (ready) {
        errno = 0;
        ready = ::fcntl(4, F_GETFD) < 0 && errno == EBADF && no_launcher_alias(3);
    }
    wipe_test_bytes(package.data(), package.size());
    wipe_test_bytes(secret.data(), secret.size());
    if (!ready) {
        if (memfd >= 0) {
            ::close(memfd);
        }
        return 71;
    }
    errno = 0;
    if (::fcntl(4, F_GETFD) >= 0 || errno != EBADF) {
        ::close(3);
        return 72;
    }
    argv[1] = const_cast<char *>(exec_mode);
    ::execv("/proc/self/exe", argv);
    ::close(3);
    return 73;
}

void print_audit(const char * prefix, const audit & observed) {
    std::printf(
        "%sstatus=%s credential_admitted=%u credential_before_root=%u key_tuple=%u "
        "parent=%u root=%u fixture=%u lock_identity=%u root_empty=%u "
        "fixture_layout=%u not_reported_read_only=%u ofd_lock=%u "
        "scratch_wiped=%u owner_wiped=%u owner_unlocked=%u "
        "credential_syscalls=%u root_fixture_syscalls=%u ofd_attempts=%u "
        "filesystem_reserve=%llu logical_authority_bound=%llu\n",
        prefix, status_name(observed.result), observed.credential_admitted,
        observed.credential_preceded_root_access, observed.expected_key_tuple_matched,
        observed.parent_identity_matched, observed.candidate_root_identity_matched,
        observed.fixture_identity_matched, observed.fixture_lock_identity_matched,
        observed.candidate_root_empty, observed.fixture_layout_exact,
        observed.filesystem_not_reported_read_only, observed.ofd_lock_acquired,
        observed.credential_scratch_wiped, observed.credential_owner_wiped,
        observed.credential_owner_unlocked, observed.credential_syscall_count,
        observed.root_fixture_syscall_count, observed.ofd_attempt_count,
        static_cast<unsigned long long>(observed.observed_filesystem_reserve),
        static_cast<unsigned long long>(observed.logical_authority_bound));
}

bool make_high_pipe(int output[2]) {
    int temporary[2] { -1, -1 };
    if (::pipe2(temporary, O_CLOEXEC) != 0) {
        return false;
    }
    output[0] = ::fcntl(temporary[0], F_DUPFD_CLOEXEC, 100);
    output[1] = ::fcntl(temporary[1], F_DUPFD_CLOEXEC, 100);
    const bool closed0 = ::close(temporary[0]) == 0;
    const bool closed1 = ::close(temporary[1]) == 0;
    if (output[0] < 0 || output[1] < 0 || !closed0 || !closed1) {
        if (output[0] >= 0) ::close(output[0]);
        if (output[1] >= 0) ::close(output[1]);
        output[0] = -1;
        output[1] = -1;
        return false;
    }
    return true;
}

int run_guard_holder(const request & input, int ready_write, int release_read) {
    ::close(3);
    int fixture_fd = ::open(input.fixture.canonical_path,
                            O_RDONLY | O_DIRECTORY | O_CLOEXEC | O_NOFOLLOW);
    if (fixture_fd < 0) return 80;
    struct open_how how {};
    how.flags = O_RDWR | O_CLOEXEC | O_NOFOLLOW;
    how.resolve = RESOLVE_BENEATH | RESOLVE_NO_SYMLINKS |
                  RESOLVE_NO_MAGICLINKS | RESOLVE_NO_XDEV;
    int lock_fd = static_cast<int>(::syscall(SYS_openat2, fixture_fd,
                                              "primitive.lock", &how, sizeof(how)));
    if (lock_fd < 0) return 81;
    struct flock lock {};
    lock.l_type = F_WRLCK;
    lock.l_whence = SEEK_SET;
    lock.l_start = 0;
    lock.l_len = 0;
    if (::fcntl(lock_fd, F_OFD_SETLK, &lock) != 0) return 82;
    const char ready = 'R';
    if (::write(ready_write, &ready, 1) != 1) return 83;
    char release = 0;
    ssize_t count;
    do {
        count = ::read(release_read, &release, 1);
    } while (count < 0 && errno == EINTR);
    if (count != 1 || release != 'X') return 84;
    return 0;
}

int run_guard_probe(int argc, char ** argv) {
    status expected = status::invalid_request;
    request input{};
    if (!parse_public_request(argc, argv, expected, input)) {
        return 64;
    }
    int ready[2] { -1, -1 };
    int release[2] { -1, -1 };
    if (!make_high_pipe(ready) || !make_high_pipe(release)) {
        return 74;
    }
    const pid_t holder = ::fork();
    if (holder < 0) return 75;
    if (holder == 0) {
        ::close(ready[0]);
        ::close(release[1]);
        _exit(run_guard_holder(input, ready[1], release[0]));
    }
    ::close(ready[1]);
    ::close(release[0]);
    char ready_byte = 0;
    ssize_t ready_count;
    do {
        ready_count = ::read(ready[0], &ready_byte, 1);
    } while (ready_count < 0 && errno == EINTR);
    ::close(ready[0]);
    if (ready_count != 1 || ready_byte != 'R') return 76;

    audit first{};
    std::thread active([&]() { first = qualify_once(input); });
    bool credential_consumed = false;
    constexpr char credential_link[] =
        "/memfd:halofpx-registry-lab-credential (deleted)";
    for (unsigned i = 0; i < 2000; ++i) {
        std::array<char, 96> link_target{};
        errno = 0;
        const ssize_t link_size = ::readlink("/proc/self/fd/3", link_target.data(),
                                             link_target.size());
        if (link_size != static_cast<ssize_t>(sizeof(credential_link) - 1) ||
            std::memcmp(link_target.data(), credential_link,
                        sizeof(credential_link) - 1) != 0) {
            if (link_size < 0 && errno != ENOENT && errno != EBADF) {
                break;
            }
            credential_consumed = true;
            break;
        }
        struct timespec delay { 0, 1000000 };
        while (::nanosleep(&delay, &delay) != 0 && errno == EINTR) {
        }
    }
    if (!credential_consumed) {
        const char release_byte = 'X';
        ::write(release[1], &release_byte, 1);
        active.join();
        return 77;
    }

    const audit same_root = qualify_once(input);
    request different_input = input;
    ++different_input.candidate_root.inode;
    const audit different_root = qualify_once(different_input);
    const char release_byte = 'X';
    const bool released = ::write(release[1], &release_byte, 1) == 1;
    ::close(release[1]);
    active.join();
    int holder_status = -1;
    pid_t waited;
    do {
        waited = ::waitpid(holder, &holder_status, 0);
    } while (waited < 0 && errno == EINTR);

    print_audit("first_", first);
    print_audit("same_root_", same_root);
    print_audit("different_root_", different_root);
    const bool same_clean = same_root.result == status::busy &&
                            !same_root.credential_admitted &&
                            same_root.credential_syscall_count == 0 &&
                            same_root.root_fixture_syscall_count == 0;
    const bool different_clean = different_root.result == status::invalid_request &&
                                 !different_root.credential_admitted &&
                                 different_root.credential_syscall_count == 0 &&
                                 different_root.root_fixture_syscall_count == 0;
    return released && waited == holder && WIFEXITED(holder_status) &&
                   WEXITSTATUS(holder_status) == 0 && first.result == expected &&
                   same_clean && different_clean
               ? 0
               : 78;
}

struct prepared_provider {
    pid_t pid = -1;
    int start_write = -1;
    int output_read = -1;
};

struct provider_result {
    int wait_status = -1;
    std::uint64_t elapsed_ns = 0;
    std::array<char, 4096> output{};
    std::size_t output_size = 0;
};

std::uint64_t monotonic_ns() {
    struct timespec value {};
    if (::clock_gettime(CLOCK_MONOTONIC, &value) != 0) return 0;
    return static_cast<std::uint64_t>(value.tv_sec) * 1000000000ULL +
           static_cast<std::uint64_t>(value.tv_nsec);
}

bool prepare_provider_contender(char ** argv, const char * expected,
                                prepared_provider & output) {
    int start[2] { -1, -1 };
    int capture[2] { -1, -1 };
    if (!make_high_pipe(start) || !make_high_pipe(capture)) return false;
    const pid_t child = ::fork();
    if (child < 0) return false;
    if (child == 0) {
        ::close(start[1]);
        ::close(capture[0]);
        char start_byte = 0;
        ssize_t count;
        do {
            count = ::read(start[0], &start_byte, 1);
        } while (count < 0 && errno == EINTR);
        if (count != 1 || start_byte != 'S') _exit(85);
        ::close(start[0]);
        if (::dup2(capture[1], STDOUT_FILENO) != STDOUT_FILENO) _exit(86);
        ::close(capture[1]);
        argv[1] = const_cast<char *>("--launch-qualify");
        argv[2] = const_cast<char *>(expected);
        ::execv("/proc/self/exe", argv);
        _exit(87);
    }
    ::close(start[0]);
    ::close(capture[1]);
    output.pid = child;
    output.start_write = start[1];
    output.output_read = capture[0];
    return true;
}

bool run_prepared_provider(prepared_provider & prepared, provider_result & output,
                           std::uint64_t timeout_ns) {
    const char start = 'S';
    const std::uint64_t begin = monotonic_ns();
    if (begin == 0 || ::write(prepared.start_write, &start, 1) != 1) return false;
    ::close(prepared.start_write);
    prepared.start_write = -1;
    const std::uint64_t deadline = begin <=
            std::numeric_limits<std::uint64_t>::max() - timeout_ns
        ? begin + timeout_ns
        : std::numeric_limits<std::uint64_t>::max();
    for (;;) {
        const std::uint64_t now = monotonic_ns();
        if (now == 0 || now >= deadline) return false;
        const std::uint64_t remaining_ns = deadline - now;
        const std::uint64_t remaining_ms = (remaining_ns + 999999ULL) / 1000000ULL;
        struct pollfd descriptor { prepared.output_read, POLLIN | POLLHUP, 0 };
        int poll_result;
        do {
            poll_result = ::poll(&descriptor, 1,
                remaining_ms > static_cast<std::uint64_t>(INT_MAX)
                    ? INT_MAX : static_cast<int>(remaining_ms));
        } while (poll_result < 0 && errno == EINTR);
        if (poll_result <= 0 || (descriptor.revents & (POLLERR | POLLNVAL)) != 0)
            return false;
        if (output.output_size == output.output.size() - 1) {
            char overflow = 0;
            const ssize_t count = ::read(prepared.output_read, &overflow, 1);
            if (count != 0) return false;
            break;
        }
        const ssize_t count = ::read(prepared.output_read,
                                     output.output.data() + output.output_size,
                                     output.output.size() - 1 - output.output_size);
        if (count < 0 && errno == EINTR) continue;
        if (count < 0) return false;
        if (count == 0) break;
        output.output_size += static_cast<std::size_t>(count);
    }
    output.output[output.output_size] = '\0';
    ::close(prepared.output_read);
    prepared.output_read = -1;
    pid_t waited = 0;
    while (waited == 0) {
        do {
            waited = ::waitpid(prepared.pid, &output.wait_status, WNOHANG);
        } while (waited < 0 && errno == EINTR);
        if (waited != 0) break;
        const std::uint64_t now = monotonic_ns();
        if (now == 0 || now >= deadline) return false;
        struct timespec delay { 0, 1000000 };
        while (::nanosleep(&delay, &delay) != 0 && errno == EINTR) {
        }
    }
    const std::uint64_t finish = monotonic_ns();
    output.elapsed_ns = finish >= begin ? finish - begin : 0;
    if (waited != prepared.pid) return false;
    prepared.pid = -1;
    return true;
}

void terminate_and_reap(pid_t & child) {
    if (child <= 0) return;
    if (::kill(child, SIGKILL) != 0 && errno != ESRCH) {
    }
    int value = 0;
    pid_t waited;
    do {
        waited = ::waitpid(child, &value, 0);
    } while (waited < 0 && errno == EINTR);
    child = -1;
}

void cleanup_prepared_provider(prepared_provider & prepared) {
    if (prepared.start_write >= 0) {
        ::close(prepared.start_write);
        prepared.start_write = -1;
    }
    if (prepared.output_read >= 0) {
        ::close(prepared.output_read);
        prepared.output_read = -1;
    }
    terminate_and_reap(prepared.pid);
}

bool provider_result_is(const provider_result & value, const char * expected,
                        unsigned expected_attempts, bool exact_attempts) {
    if (!WIFEXITED(value.wait_status) || WEXITSTATUS(value.wait_status) != 0 ||
        std::strstr(value.output.data(), expected) == nullptr) return false;
    const char * attempts = std::strstr(value.output.data(), "ofd_attempts=");
    if (attempts == nullptr) return false;
    attempts += std::strlen("ofd_attempts=");
    char * end = nullptr;
    const unsigned long observed = std::strtoul(attempts, &end, 10);
    if (end == attempts || (*end != ' ' && *end != '\n')) return false;
    return exact_attempts ? observed == expected_attempts : observed > expected_attempts;
}

int open_and_lock_fixture(const request & input) {
    int fixture_fd = ::open(input.fixture.canonical_path,
                            O_RDONLY | O_DIRECTORY | O_CLOEXEC | O_NOFOLLOW);
    if (fixture_fd < 0) return -1;
    struct open_how how {};
    how.flags = O_RDWR | O_CLOEXEC | O_NOFOLLOW;
    how.resolve = RESOLVE_BENEATH | RESOLVE_NO_SYMLINKS |
                  RESOLVE_NO_MAGICLINKS | RESOLVE_NO_XDEV;
    int lock_fd = static_cast<int>(::syscall(SYS_openat2, fixture_fd,
                                              "primitive.lock", &how, sizeof(how)));
    ::close(fixture_fd);
    if (lock_fd < 0) return -1;
    struct flock lock {};
    lock.l_type = F_WRLCK;
    lock.l_whence = SEEK_SET;
    lock.l_start = 0;
    lock.l_len = 0;
    if (::fcntl(lock_fd, F_OFD_SETLK, &lock) != 0) {
        ::close(lock_fd);
        return -1;
    }
    struct stat value {};
    if (::fstat(lock_fd, &value) != 0 || !S_ISREG(value.st_mode) ||
        static_cast<std::uint64_t>(value.st_dev) != input.fixture_lock_device ||
        static_cast<std::uint64_t>(value.st_ino) != input.fixture_lock_inode ||
        static_cast<std::uint32_t>(value.st_mode & 07777) != 0600 ||
        value.st_nlink != 1 || value.st_size != 0) {
        ::close(lock_fd);
        return -1;
    }
    return lock_fd;
}

bool same_lock_snapshot(const struct stat & lhs, const struct stat & rhs,
                        const request & input) {
    return static_cast<std::uint64_t>(lhs.st_dev) == input.fixture_lock_device &&
           static_cast<std::uint64_t>(lhs.st_ino) == input.fixture_lock_inode &&
           static_cast<std::uint64_t>(rhs.st_dev) == input.fixture_lock_device &&
           static_cast<std::uint64_t>(rhs.st_ino) == input.fixture_lock_inode &&
           lhs.st_dev == rhs.st_dev && lhs.st_ino == rhs.st_ino &&
           lhs.st_size == 0 && rhs.st_size == 0 && lhs.st_nlink == 1 &&
           rhs.st_nlink == 1 && (lhs.st_mode & 07777) == 0600 &&
           (rhs.st_mode & 07777) == 0600;
}

bool stat_fixture_lock(const request & input, struct stat & value) {
    int fixture_fd = ::open(input.fixture.canonical_path,
                            O_RDONLY | O_DIRECTORY | O_CLOEXEC | O_NOFOLLOW);
    if (fixture_fd < 0) return false;
    struct open_how how {};
    how.flags = O_RDONLY | O_CLOEXEC | O_NOFOLLOW;
    how.resolve = RESOLVE_BENEATH | RESOLVE_NO_SYMLINKS |
                  RESOLVE_NO_MAGICLINKS | RESOLVE_NO_XDEV;
    int lock_fd = static_cast<int>(::syscall(SYS_openat2, fixture_fd,
                                              "primitive.lock", &how, sizeof(how)));
    ::close(fixture_fd);
    if (lock_fd < 0) return false;
    const bool result = ::fstat(lock_fd, &value) == 0;
    return ::close(lock_fd) == 0 && result;
}

bool wait_for_signal_death(pid_t & child) {
    int value = -1;
    pid_t waited;
    do {
        waited = ::waitpid(child, &value, 0);
    } while (waited < 0 && errno == EINTR);
    const bool result = waited == child && WIFSIGNALED(value) &&
                        WTERMSIG(value) == SIGKILL;
    if (waited == child) child = -1;
    return result;
}

bool read_exact_with_deadline(int fd, void * output, std::size_t size,
                              std::uint64_t timeout_ns) {
    const std::uint64_t begin = monotonic_ns();
    if (begin == 0 || begin > std::numeric_limits<std::uint64_t>::max() - timeout_ns)
        return false;
    const std::uint64_t deadline = begin + timeout_ns;
    std::size_t received = 0;
    while (received < size) {
        const std::uint64_t now = monotonic_ns();
        if (now == 0 || now >= deadline) return false;
        const std::uint64_t milliseconds = (deadline - now + 999999ULL) / 1000000ULL;
        struct pollfd descriptor { fd, POLLIN | POLLHUP, 0 };
        int poll_result;
        do {
            poll_result = ::poll(&descriptor, 1,
                milliseconds > static_cast<std::uint64_t>(INT_MAX)
                    ? INT_MAX : static_cast<int>(milliseconds));
        } while (poll_result < 0 && errno == EINTR);
        if (poll_result <= 0 || (descriptor.revents & (POLLERR | POLLNVAL)) != 0)
            return false;
        const ssize_t count = ::read(fd, static_cast<char *>(output) + received,
                                     size - received);
        if (count < 0 && errno == EINTR) continue;
        if (count <= 0) return false;
        received += static_cast<std::size_t>(count);
    }
    return true;
}

int run_alias_holder(const request & input, int ready_write) {
    const int lock_fd = open_and_lock_fixture(input);
    if (lock_fd < 0) return 88;
    const pid_t alias = ::fork();
    if (alias < 0) return 89;
    if (alias == 0) {
        ::close(ready_write);
        for (;;) ::pause();
    }
    if (::write(ready_write, &alias, sizeof(alias)) !=
        static_cast<ssize_t>(sizeof(alias))) {
        pid_t child = alias;
        terminate_and_reap(child);
        return 90;
    }
    ::close(ready_write);
    for (;;) ::pause();
}

int run_inherited_alias_probe(int argc, char ** argv) {
    status ignored = status::invalid_request;
    request input{};
    if (!parse_public_request(argc, argv, ignored, input) ||
        ::prctl(PR_SET_CHILD_SUBREAPER, 1, 0, 0, 0) != 0) return 64;

    prepared_provider busy_provider{};
    prepared_provider success_provider{};
    int ready[2] { -1, -1 };
    pid_t holder = -1;
    pid_t alias = -1;
    const auto finish = [&](int code) {
        if (ready[0] >= 0) ::close(ready[0]);
        if (ready[1] >= 0) ::close(ready[1]);
        cleanup_prepared_provider(busy_provider);
        cleanup_prepared_provider(success_provider);
        terminate_and_reap(holder);
        terminate_and_reap(alias);
        return code;
    };
    if (!prepare_provider_contender(argv, "busy", busy_provider) ||
        !prepare_provider_contender(argv, "ok_non_authoritative", success_provider))
        return finish(91);
    const pid_t busy_provider_pid = busy_provider.pid;
    const pid_t success_provider_pid = success_provider.pid;
    if (!make_high_pipe(ready)) return finish(92);
    holder = ::fork();
    if (holder < 0) return finish(93);
    if (holder == 0) {
        ::close(ready[0]);
        _exit(run_alias_holder(input, ready[1]));
    }
    const pid_t holder_pid = holder;
    ::close(ready[1]);
    ready[1] = -1;
    if (!read_exact_with_deadline(ready[0], &alias, sizeof(alias), 2000000000ULL))
        return finish(94);
    ::close(ready[0]);
    ready[0] = -1;
    if (alias <= 0) return finish(95);
    const pid_t alias_pid = alias;
    if (::kill(holder, SIGKILL) != 0 ||
        !wait_for_signal_death(holder) || ::kill(alias, 0) != 0) return finish(95);

    provider_result busy{};
    if (!run_prepared_provider(busy_provider, busy, 7000000000ULL) ||
        !provider_result_is(busy, "status=busy ", 1, false) ||
        busy.elapsed_ns < 5000000000ULL || busy.elapsed_ns > 6000000000ULL ||
        ::kill(alias, 0) != 0) return finish(96);

    struct stat before_release {};
    if (!stat_fixture_lock(input, before_release) ||
        ::kill(alias, SIGKILL) != 0 || !wait_for_signal_death(alias)) return finish(97);

    provider_result success{};
    if (!run_prepared_provider(success_provider, success, 3000000000ULL) ||
        !provider_result_is(success, "status=ok_non_authoritative ", 1, true) ||
        success.elapsed_ns > 2000000000ULL) return finish(98);
    struct stat after_release {};
    if (!stat_fixture_lock(input, after_release) ||
        !same_lock_snapshot(before_release, after_release, input)) return finish(99);

    std::printf("probe_pid role=holder pid=%lld\n", static_cast<long long>(holder_pid));
    std::printf("probe_pid role=alias pid=%lld\n", static_cast<long long>(alias_pid));
    std::printf("probe_pid role=busy_provider pid=%lld\n",
                static_cast<long long>(busy_provider_pid));
    std::printf("probe_pid role=success_provider pid=%lld\n",
                static_cast<long long>(success_provider_pid));
    std::printf("alias_busy_elapsed_ns=%llu %s",
                static_cast<unsigned long long>(busy.elapsed_ns), busy.output.data());
    std::printf("alias_reacquire_elapsed_ns=%llu %s",
                static_cast<unsigned long long>(success.elapsed_ns), success.output.data());
    return finish(0);
}

bool no_fd_identity_alias(std::uint64_t device, std::uint64_t inode) {
    DIR * directory = ::opendir("/proc/self/fd");
    if (directory == nullptr) return false;
    const int scan_fd = ::dirfd(directory);
    bool clean = true;
    std::size_t count = 0;
    for (;;) {
        errno = 0;
        const dirent * entry = ::readdir(directory);
        if (entry == nullptr) {
            if (errno != 0) clean = false;
            break;
        }
        if (++count > 4096) { clean = false; break; }
        char * end = nullptr;
        const long candidate = std::strtol(entry->d_name, &end, 10);
        if (end == entry->d_name || *end != '\0' || candidate < 0 ||
            candidate > std::numeric_limits<int>::max() || candidate == scan_fd) continue;
        struct stat value {};
        if (::fstat(static_cast<int>(candidate), &value) != 0) {
            if (errno != EBADF) clean = false;
            continue;
        }
        if (static_cast<std::uint64_t>(value.st_dev) == device &&
            static_cast<std::uint64_t>(value.st_ino) == inode) {
            clean = false;
            break;
        }
    }
    if (::closedir(directory) != 0) clean = false;
    return clean;
}

int run_exec_lock_sentinel(int argc, char ** argv) {
    if (argc != 7) return 100;
    std::uint64_t ready = 0, release = 0, lock_fd = 0, device = 0, inode = 0;
    if (!parse_u64(argv[2], 10, ready) || !parse_u64(argv[3], 10, release) ||
        !parse_u64(argv[4], 10, lock_fd) || !parse_u64(argv[5], 10, device) ||
        !parse_u64(argv[6], 10, inode) || ready > INT_MAX || release > INT_MAX ||
        lock_fd > INT_MAX || !no_fd_identity_alias(device, inode)) return 101;
    const char value = 'E';
    if (::write(static_cast<int>(ready), &value, 1) != 1) return 103;
    char finish = 0;
    ssize_t count;
    do {
        count = ::read(static_cast<int>(release), &finish, 1);
    } while (count < 0 && errno == EINTR);
    return count == 1 && finish == 'X' ? 0 : 104;
}

int run_exec_holder(const request & input, int ready_write, int release_read) {
    const int lock_fd = open_and_lock_fixture(input);
    if (lock_fd < 0) return 105;
    for (int fd : { ready_write, release_read }) {
        const int flags = ::fcntl(fd, F_GETFD);
        if (flags < 0 || ::fcntl(fd, F_SETFD, flags & ~FD_CLOEXEC) != 0) return 106;
    }
    std::array<char, 32> ready_text{}, release_text{}, lock_text{}, device_text{}, inode_text{};
    std::snprintf(ready_text.data(), ready_text.size(), "%d", ready_write);
    std::snprintf(release_text.data(), release_text.size(), "%d", release_read);
    std::snprintf(lock_text.data(), lock_text.size(), "%d", lock_fd);
    std::snprintf(device_text.data(), device_text.size(), "%llu",
                  static_cast<unsigned long long>(input.fixture_lock_device));
    std::snprintf(inode_text.data(), inode_text.size(), "%llu",
                  static_cast<unsigned long long>(input.fixture_lock_inode));
    ::execl("/proc/self/exe", "/proc/self/exe", "--exec-lock-sentinel",
            ready_text.data(), release_text.data(), lock_text.data(),
            device_text.data(), inode_text.data(), static_cast<char *>(nullptr));
    return 107;
}

int run_exec_cloexec_probe(int argc, char ** argv) {
    status ignored = status::invalid_request;
    request input{};
    if (!parse_public_request(argc, argv, ignored, input)) return 64;
    prepared_provider provider{};
    int ready[2] { -1, -1 }, release[2] { -1, -1 };
    pid_t holder = -1;
    const auto finish = [&](int code) {
        for (int & fd : ready) if (fd >= 0) { ::close(fd); fd = -1; }
        for (int & fd : release) if (fd >= 0) { ::close(fd); fd = -1; }
        cleanup_prepared_provider(provider);
        terminate_and_reap(holder);
        return code;
    };
    if (!prepare_provider_contender(argv, "ok_non_authoritative", provider))
        return finish(108);
    const pid_t provider_pid = provider.pid;
    if (!make_high_pipe(ready) || !make_high_pipe(release)) return finish(109);
    struct stat before_exec {};
    if (!stat_fixture_lock(input, before_exec)) return finish(109);
    holder = ::fork();
    if (holder < 0) return finish(110);
    if (holder == 0) {
        ::close(ready[0]);
        ::close(release[1]);
        _exit(run_exec_holder(input, ready[1], release[0]));
    }
    const pid_t holder_pid = holder;
    ::close(ready[1]);
    ready[1] = -1;
    ::close(release[0]);
    release[0] = -1;
    char ready_byte = 0;
    const bool sentinel_ready = read_exact_with_deadline(
        ready[0], &ready_byte, 1, 2000000000ULL);
    ::close(ready[0]);
    ready[0] = -1;
    if (!sentinel_ready || ready_byte != 'E') return finish(111);

    provider_result acquired{};
    if (!run_prepared_provider(provider, acquired, 3000000000ULL) ||
        !provider_result_is(acquired, "status=ok_non_authoritative ", 1, true) ||
        acquired.elapsed_ns > 2000000000ULL) return finish(112);
    struct stat after_acquire {};
    if (!stat_fixture_lock(input, after_acquire) ||
        !same_lock_snapshot(before_exec, after_acquire, input)) return finish(112);
    const char release_byte = 'X';
    if (::write(release[1], &release_byte, 1) != 1) return finish(113);
    ::close(release[1]);
    release[1] = -1;
    int holder_status = -1;
    pid_t waited;
    do {
        waited = ::waitpid(holder, &holder_status, 0);
    } while (waited < 0 && errno == EINTR);
    if (waited != holder || !WIFEXITED(holder_status) || WEXITSTATUS(holder_status) != 0)
        return finish(114);
    holder = -1;
    std::printf("probe_pid role=holder pid=%lld\n", static_cast<long long>(holder_pid));
    std::printf("probe_pid role=provider pid=%lld\n",
                static_cast<long long>(provider_pid));
    std::printf("cloexec_reacquire_elapsed_ns=%llu %s",
                static_cast<unsigned long long>(acquired.elapsed_ns),
                acquired.output.data());
    return finish(0);
}

int run_qualification_cli(int argc, char ** argv) {
    // All argv fields are public launcher-pinned facts. The credential package,
    // including its secret, is accepted only from already-installed fd 3.
    if (argc != 31 || std::strcmp(argv[1], "--qualify") != 0) {
        std::fprintf(stderr,
            "usage: %s --qualify EXPECTED "
            "PARENT_PATH DEV INO MOUNT UID MODE FSUUID SUBVOLUUID "
            "ROOT_PATH DEV INO MOUNT UID MODE FSUUID SUBVOLUUID "
            "FIXTURE_PATH DEV INO MOUNT UID MODE FSUUID SUBVOLUUID "
            "LOCK_DEV LOCK_INO KEY_ID KEY_GENERATION\n", argv[0]);
        return 64;
    }
    status expected = status::invalid_request;
    request input{};
    if (!parse_public_request(argc, argv, expected, input)) {
        return 64;
    }

    const audit observed = qualify_once(input);
    print_audit("", observed);
    return observed.result == expected ? 0 : 1;
}

} // namespace

int main(int argc, char ** argv) {
    if (argc != 1) {
        if (std::strcmp(argv[1], "--inherited-alias-probe") == 0) {
            return run_inherited_alias_probe(argc, argv);
        }
        if (std::strcmp(argv[1], "--exec-cloexec-probe") == 0) {
            return run_exec_cloexec_probe(argc, argv);
        }
        if (std::strcmp(argv[1], "--exec-lock-sentinel") == 0) {
            return run_exec_lock_sentinel(argc, argv);
        }
        if (std::strcmp(argv[1], "--launch-qualify") == 0) {
            return run_self_launcher(argc, argv, "--qualify");
        }
        if (std::strcmp(argv[1], "--launch-guard-probe") == 0) {
            return run_self_launcher(argc, argv, "--guard-probe");
        }
        if (std::strcmp(argv[1], "--guard-probe") == 0) {
            return run_guard_probe(argc, argv);
        }
        return run_qualification_cli(argc, argv);
    }
    constexpr test_case cases[] = {
        test_case::malformed_magic,
        test_case::missing_credential,
        test_case::forbidden_fd4,
        test_case::missing_seals,
        test_case::same_process_alias,
        test_case::wrong_key_tuple,
        test_case::valid_credential_then_missing_root,
    };
    for (test_case which : cases) {
        const pid_t child = ::fork();
        assert(child >= 0);
        if (child == 0) {
            _exit(run_child(which));
        }
        int child_status = -1;
        pid_t waited;
        do {
            waited = ::waitpid(child, &child_status, 0);
        } while (waited < 0 && errno == EINTR);
        if (waited != child) {
            std::fprintf(stderr, "L05s waitpid failed for case %u: errno=%d\n",
                         static_cast<unsigned>(which), errno);
            return 1;
        }
        if (!WIFEXITED(child_status) || WEXITSTATUS(child_status) != 0) {
            std::fprintf(stderr, "L05s child case %u failed with status %d\n",
                         static_cast<unsigned>(which),
                         WIFEXITED(child_status) ? WEXITSTATUS(child_status) : -1);
            return 1;
        }
    }
    return 0;
}
