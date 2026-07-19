#include "halofpx-context-store-registry-lab-linux-preinit-internal.h"

#if !defined(__linux__)
#error "The HaloFPX registry-lab pre-initialization provider is Linux-only"
#endif

#include <array>
#include <atomic>
#include <cerrno>
#include <climits>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <limits>

#include <fcntl.h>
#include <linux/btrfs.h>
#include <linux/magic.h>
#include <linux/openat2.h>
#include <linux/stat.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/statfs.h>
#include <sys/statvfs.h>
#include <sys/syscall.h>
#include <time.h>
#include <unistd.h>

namespace halofpx::registry_lab::linux_preinit {
namespace {

constexpr int credential_fd = 3;
constexpr int forbidden_fd = 4;
constexpr std::size_t credential_magic_size = 16;
constexpr std::array<std::uint8_t, credential_magic_size> credential_magic = {
    'H', 'a', 'l', 'o', 'F', 'P', 'X', 'R', 'e', 'g', 'K', 'e', 'y', '0', '1', 0,
};
constexpr std::size_t credential_min_size = credential_magic_size + 2 + 1 + 8 + 2 + 32;
constexpr std::size_t credential_max_size = credential_magic_size + 2 + max_key_id_bytes + 8 + 2 + 32;
constexpr std::uint64_t required_resolve = RESOLVE_BENEATH | RESOLVE_NO_SYMLINKS |
                                           RESOLVE_NO_MAGICLINKS | RESOLVE_NO_XDEV;
constexpr std::uint64_t lock_retry_nanoseconds = 10ULL * 1000ULL * 1000ULL;
constexpr std::uint64_t lock_deadline_nanoseconds = 5ULL * 1000ULL * 1000ULL * 1000ULL;
constexpr std::size_t max_fd_scan_entries = 4096;
constexpr std::size_t max_directory_entries = 64;
constexpr std::size_t mountinfo_capacity = 256 * 1024;

std::atomic_flag session_consumed = ATOMIC_FLAG_INIT;
std::atomic<std::uint8_t> root_guard_state { 0 };
std::atomic<std::uint64_t> root_guard_device { 0 };
std::atomic<std::uint64_t> root_guard_inode { 0 };

struct linux_dirent64 {
    std::uint64_t d_ino;
    std::int64_t d_off;
    unsigned short d_reclen;
    unsigned char d_type;
    char d_name[1];
};

struct credential_state {
    std::array<std::uint8_t, credential_max_size + 1> scratch{};
    alignas(64) std::array<std::uint8_t, 32> owner{};
    bool owner_locked = false;
    bool fd_present = true;
};

void wipe(void * data, std::size_t size) noexcept {
    volatile std::uint8_t * p = static_cast<volatile std::uint8_t *>(data);
    while (size-- != 0) {
        *p++ = 0;
    }
}

bool all_zero(const void * data, std::size_t size) noexcept {
    const auto * p = static_cast<const std::uint8_t *>(data);
    std::uint8_t value = 0;
    for (std::size_t i = 0; i < size; ++i) {
        value |= p[i];
    }
    return value == 0;
}

bool close_fd(int & fd) noexcept {
    if (fd < 0) {
        return true;
    }
    const int value = fd;
    fd = -1;
    return ::close(value) == 0;
}

bool printable_registered_ascii(const char * bytes, std::size_t size) noexcept {
    if (size == 0 || size > max_key_id_bytes) {
        return false;
    }
    for (std::size_t i = 0; i < size; ++i) {
        const unsigned char value = static_cast<unsigned char>(bytes[i]);
        if (value < 0x21 || value > 0x7e) {
            return false;
        }
    }
    return true;
}

bool fixed_path_valid(const pinned_path_identity & identity) noexcept {
    const std::size_t size = identity.path_length;
    if (size == 0 || size > max_path_bytes || identity.canonical_path[0] != '/' ||
        identity.canonical_path[size] != '\0') {
        return false;
    }
    if (::strnlen(identity.canonical_path, max_path_bytes + 1) != size) {
        return false;
    }
    if (size > 1 && identity.canonical_path[size - 1] == '/') {
        return false;
    }
    bool component_start = true;
    for (std::size_t i = 1; i < size; ++i) {
        const unsigned char value = static_cast<unsigned char>(identity.canonical_path[i]);
        if (value < 0x21 || value > 0x7e) {
            return false;
        }
        if (value == '/') {
            if (component_start) {
                return false;
            }
            component_start = true;
            continue;
        }
        if (component_start && value == '.') {
            const bool dot = i + 1 == size || identity.canonical_path[i + 1] == '/';
            const bool dotdot = i + 1 < size && identity.canonical_path[i + 1] == '.' &&
                                (i + 2 == size || identity.canonical_path[i + 2] == '/');
            if (dot || dotdot) {
                return false;
            }
        }
        component_start = false;
    }
    return !component_start;
}

bool is_equal_or_descendant(const char * path, std::size_t path_size,
                            const char * protected_path) noexcept {
    const std::size_t protected_size = std::strlen(protected_path);
    if (path_size < protected_size ||
        std::memcmp(path, protected_path, protected_size) != 0) {
        return false;
    }
    return path_size == protected_size || path[protected_size] == '/';
}

bool is_protected(const pinned_path_identity & identity) noexcept {
    if (identity.path_length == 1 && identity.canonical_path[0] == '/') {
        return true;
    }
    constexpr const char * protected_paths[] = {
        "/boot", "/dev", "/etc", "/home", "/opt", "/proc", "/root", "/run",
        "/srv", "/sys", "/usr", "/var/cache", "/var/lib", "/var/log",
    };
    for (const char * path : protected_paths) {
        if (is_equal_or_descendant(identity.canonical_path, identity.path_length, path)) {
            return true;
        }
    }
    return false;
}

bool immediate_child(const pinned_path_identity & parent,
                     const pinned_path_identity & child) noexcept {
    const std::size_t parent_size = parent.path_length;
    const std::size_t child_size = child.path_length;
    if (parent_size == 0 || child_size <= parent_size + 1 ||
        std::memcmp(parent.canonical_path, child.canonical_path, parent_size) != 0 ||
        child.canonical_path[parent_size] != '/') {
        return false;
    }
    for (std::size_t i = parent_size + 1; i < child_size; ++i) {
        if (child.canonical_path[i] == '/') {
            return false;
        }
    }
    return true;
}

const char * child_name(const pinned_path_identity & parent,
                        const pinned_path_identity & child) noexcept {
    return child.canonical_path + parent.path_length + 1;
}

bool same_16(const std::uint8_t lhs[16], const std::uint8_t rhs[16]) noexcept {
    return std::memcmp(lhs, rhs, 16) == 0;
}

bool request_lexically_valid(const request & input) noexcept {
    if (!fixed_path_valid(input.parent) || !fixed_path_valid(input.candidate_root) ||
        !fixed_path_valid(input.fixture) || is_protected(input.parent) ||
        is_protected(input.candidate_root) || is_protected(input.fixture) ||
        !immediate_child(input.parent, input.candidate_root) ||
        !immediate_child(input.parent, input.fixture) ||
        (input.candidate_root.path_length == input.fixture.path_length &&
         std::memcmp(input.candidate_root.canonical_path, input.fixture.canonical_path,
                     input.candidate_root.path_length) == 0)) {
        return false;
    }
    if (input.expected_key_generation == 0 ||
        input.expected_key_id_length == 0 ||
        input.expected_key_id_length > max_key_id_bytes ||
        input.expected_key_id[input.expected_key_id_length] != '\0' ||
        ::strnlen(input.expected_key_id, max_key_id_bytes + 1) !=
            input.expected_key_id_length ||
        !printable_registered_ascii(input.expected_key_id,
                                    input.expected_key_id_length)) {
        return false;
    }
    if (input.parent.owner_uid != input.candidate_root.owner_uid ||
        input.parent.owner_uid != input.fixture.owner_uid ||
        input.parent.owner_uid != static_cast<std::uint64_t>(::geteuid()) ||
        input.parent.device != input.candidate_root.device ||
        input.parent.device != input.fixture.device ||
        input.parent.mount_id != input.candidate_root.mount_id ||
        input.parent.mount_id != input.fixture.mount_id ||
        !same_16(input.parent.filesystem_uuid, input.candidate_root.filesystem_uuid) ||
        !same_16(input.parent.filesystem_uuid, input.fixture.filesystem_uuid) ||
        !same_16(input.parent.subvolume_uuid, input.candidate_root.subvolume_uuid) ||
        !same_16(input.parent.subvolume_uuid, input.fixture.subvolume_uuid) ||
        input.candidate_root.mode != 0700 || input.fixture.mode != 0700 ||
        input.fixture_lock_device != input.fixture.device ||
        input.parent.inode == 0 || input.candidate_root.inode == 0 ||
        input.fixture.inode == 0 || input.fixture_lock_inode == 0) {
        return false;
    }
    return true;
}

std::uint16_t read_u16_be(const std::uint8_t * value) noexcept {
    return static_cast<std::uint16_t>((static_cast<std::uint16_t>(value[0]) << 8) |
                                      static_cast<std::uint16_t>(value[1]));
}

std::uint64_t read_u64_be(const std::uint8_t * value) noexcept {
    std::uint64_t result = 0;
    for (unsigned i = 0; i < 8; ++i) {
        result = (result << 8) | value[i];
    }
    return result;
}

bool fd_alias_exists(const struct stat & credential_stat, audit & output,
                     status & error) noexcept {
    ++output.credential_syscall_count;
    int directory_fd = ::open("/proc/self/fd", O_RDONLY | O_DIRECTORY | O_CLOEXEC);
    if (directory_fd < 0) {
        error = status::io_failure;
        return false;
    }
    std::array<char, 4096> bytes{};
    std::size_t entries = 0;
    bool alias = false;
    for (;;) {
        ++output.credential_syscall_count;
        const long count = ::syscall(SYS_getdents64, directory_fd, bytes.data(), bytes.size());
        if (count < 0) {
            if (errno == EINTR) {
                continue;
            }
            error = status::io_failure;
            break;
        }
        if (count == 0) {
            break;
        }
        long offset = 0;
        while (offset < count) {
            const auto * entry = reinterpret_cast<const linux_dirent64 *>(bytes.data() + offset);
            if (entry->d_reclen < offsetof(linux_dirent64, d_name) + 1 ||
                offset + entry->d_reclen > count) {
                error = status::io_failure;
                offset = count;
                break;
            }
            offset += entry->d_reclen;
            if (++entries > max_fd_scan_entries) {
                error = status::invalid_request;
                offset = count;
                break;
            }
            char * end = nullptr;
            errno = 0;
            const long candidate = std::strtol(entry->d_name, &end, 10);
            if (errno != 0 || end == entry->d_name || *end != '\0' || candidate < 0 ||
                candidate > INT_MAX || candidate == credential_fd ||
                candidate == directory_fd) {
                continue;
            }
            struct stat value {};
            ++output.credential_syscall_count;
            if (::fstat(static_cast<int>(candidate), &value) != 0) {
                if (errno == EBADF) {
                    continue;
                }
                error = status::io_failure;
                offset = count;
                break;
            }
            if (value.st_dev == credential_stat.st_dev && value.st_ino == credential_stat.st_ino) {
                alias = true;
                error = status::invalid_request;
                offset = count;
                break;
            }
        }
        if (alias || error != status::ok_non_authoritative) {
            break;
        }
    }
    if (::close(directory_fd) != 0 && error == status::ok_non_authoritative) {
        error = status::io_failure;
    }
    return alias;
}

void wipe_credential(credential_state & state, audit & output) noexcept {
    wipe(state.scratch.data(), state.scratch.size());
    output.credential_scratch_wiped = all_zero(state.scratch.data(), state.scratch.size());
    wipe(state.owner.data(), state.owner.size());
    output.credential_owner_wiped = all_zero(state.owner.data(), state.owner.size());
    if (state.owner_locked) {
        output.credential_owner_unlocked = ::munlock(state.owner.data(), state.owner.size()) == 0;
        state.owner_locked = false;
    } else {
        output.credential_owner_unlocked = true;
    }
}

status admit_credential(const request & input, credential_state & state,
                        audit & output) noexcept {
    ++output.credential_syscall_count;
    errno = 0;
    if (::fcntl(forbidden_fd, F_GETFD) >= 0 || errno != EBADF) {
        return status::invalid_request;
    }

    ++output.credential_syscall_count;
    const int descriptor_flags = ::fcntl(credential_fd, F_GETFD);
    if (descriptor_flags < 0) {
        return errno == EBADF ? status::invalid_request : status::io_failure;
    }
    ++output.credential_syscall_count;
    if (::fcntl(credential_fd, F_SETFD, descriptor_flags | FD_CLOEXEC) != 0) {
        return status::io_failure;
    }

    struct stat descriptor_stat {};
    ++output.credential_syscall_count;
    if (::fstat(credential_fd, &descriptor_stat) != 0) {
        return status::io_failure;
    }
    if (!S_ISREG(descriptor_stat.st_mode) || descriptor_stat.st_nlink != 0 ||
        descriptor_stat.st_size < static_cast<off_t>(credential_min_size) ||
        descriptor_stat.st_size > static_cast<off_t>(credential_max_size)) {
        return status::invalid_request;
    }

    struct statfs filesystem_stat {};
    ++output.credential_syscall_count;
    if (::fstatfs(credential_fd, &filesystem_stat) != 0) {
        return status::io_failure;
    }
    if (static_cast<unsigned long>(filesystem_stat.f_type) != TMPFS_MAGIC) {
        return status::invalid_request;
    }

    std::array<char, 96> link_target{};
    ++output.credential_syscall_count;
    const ssize_t link_size = ::readlink("/proc/self/fd/3", link_target.data(),
                                         link_target.size());
    constexpr char expected_link[] = "/memfd:halofpx-registry-lab-credential (deleted)";
    if (link_size != static_cast<ssize_t>(sizeof(expected_link) - 1) ||
        std::memcmp(link_target.data(), expected_link, sizeof(expected_link) - 1) != 0) {
        return status::invalid_request;
    }

    ++output.credential_syscall_count;
    const int seals = ::fcntl(credential_fd, F_GET_SEALS);
    constexpr int expected_seals = F_SEAL_SEAL | F_SEAL_SHRINK | F_SEAL_GROW | F_SEAL_WRITE;
    if (seals < 0) {
        return errno == EINVAL ? status::unsupported : status::io_failure;
    }
    if (seals != expected_seals) {
        return status::invalid_request;
    }

    status scan_status = status::ok_non_authoritative;
    if (fd_alias_exists(descriptor_stat, output, scan_status)) {
        return status::invalid_request;
    }
    if (scan_status != status::ok_non_authoritative) {
        return scan_status;
    }

    const std::size_t package_size = static_cast<std::size_t>(descriptor_stat.st_size);
    std::size_t total = 0;
    while (total < package_size) {
        ++output.credential_syscall_count;
        const ssize_t count = ::pread(credential_fd, state.scratch.data() + total,
                                      package_size - total, static_cast<off_t>(total));
        if (count < 0) {
            if (errno == EINTR) {
                continue;
            }
            return status::io_failure;
        }
        if (count == 0) {
            return status::io_failure;
        }
        total += static_cast<std::size_t>(count);
    }
    ++output.credential_syscall_count;
    std::uint8_t trailing = 0;
    ssize_t trailing_count;
    do {
        trailing_count = ::pread(credential_fd, &trailing, 1, static_cast<off_t>(package_size));
    } while (trailing_count < 0 && errno == EINTR);
    if (trailing_count < 0) {
        return status::io_failure;
    }
    if (trailing_count != 0) {
        return status::invalid_request;
    }

    if (std::memcmp(state.scratch.data(), credential_magic.data(), credential_magic.size()) != 0) {
        return status::invalid_request;
    }
    std::size_t offset = credential_magic.size();
    const std::size_t key_size = read_u16_be(state.scratch.data() + offset);
    offset += 2;
    if (key_size == 0 || key_size > max_key_id_bytes ||
        package_size != credential_magic.size() + 2 + key_size + 8 + 2 + 32 ||
        !printable_registered_ascii(
            reinterpret_cast<const char *>(state.scratch.data() + offset), key_size)) {
        return status::invalid_request;
    }
    const bool key_matches = key_size == input.expected_key_id_length &&
                             std::memcmp(state.scratch.data() + offset,
                                         input.expected_key_id, key_size) == 0;
    offset += key_size;
    const std::uint64_t generation = read_u64_be(state.scratch.data() + offset);
    offset += 8;
    if (generation == 0 || read_u16_be(state.scratch.data() + offset) != 32) {
        return status::invalid_request;
    }
    offset += 2;
    output.expected_key_tuple_matched = key_matches &&
                                        generation == input.expected_key_generation;
    if (!output.expected_key_tuple_matched) {
        return status::invalid_request;
    }

    ++output.credential_syscall_count;
    if (::mlock(state.owner.data(), state.owner.size()) != 0) {
        return status::unsupported;
    }
    state.owner_locked = true;
    std::memcpy(state.owner.data(), state.scratch.data() + offset, state.owner.size());
    wipe(state.scratch.data(), state.scratch.size());
    output.credential_scratch_wiped = all_zero(state.scratch.data(), state.scratch.size());

    ++output.credential_syscall_count;
    if (::close(credential_fd) != 0) {
        state.fd_present = false;
        return status::io_failure;
    }
    state.fd_present = false;
    output.credential_admitted = true;
    output.credential_preceded_root_access = output.root_fixture_syscall_count == 0;
    return status::ok_non_authoritative;
}

status map_containment_error(int value) noexcept {
    switch (value) {
        case ENOSYS:
        case EINVAL:
            return status::unsupported;
        case ELOOP:
        case EXDEV:
        case ENOENT:
        case ENOTDIR:
        case EACCES:
        case EPERM:
            return status::invalid_request;
        default:
            return status::io_failure;
    }
}

int open_contained(int parent_fd, const char * name, std::uint64_t flags,
                   audit & output, status & error) noexcept {
    struct open_how how {};
    how.flags = flags;
    how.mode = 0;
    how.resolve = required_resolve;
    ++output.root_fixture_syscall_count;
    const int result = static_cast<int>(::syscall(SYS_openat2, parent_fd, name, &how,
                                                  sizeof(how)));
    if (result < 0) {
        error = map_containment_error(errno);
    }
    return result;
}

status fd_canonical_path(int fd, const pinned_path_identity & expected,
                         audit & output) noexcept {
    std::array<char, max_path_bytes + 1> path{};
    std::array<char, 32> proc_name{};
    const int written = std::snprintf(proc_name.data(), proc_name.size(),
                                      "/proc/self/fd/%d", fd);
    if (written <= 0 || static_cast<std::size_t>(written) >= proc_name.size()) {
        return status::io_failure;
    }
    ++output.root_fixture_syscall_count;
    const ssize_t count = ::readlink(proc_name.data(), path.data(), path.size());
    if (count < 0) {
        return status::io_failure;
    }
    if (count != static_cast<ssize_t>(expected.path_length) ||
        std::memcmp(path.data(), expected.canonical_path, expected.path_length) != 0) {
        return status::invalid_request;
    }
    return status::ok_non_authoritative;
}

status inspect_directory(int fd, const pinned_path_identity & expected,
                         std::uint32_t required_mode, audit & output) noexcept {
    struct stat value {};
    ++output.root_fixture_syscall_count;
    if (::fstat(fd, &value) != 0) {
        return status::io_failure;
    }
    if (!S_ISDIR(value.st_mode) || static_cast<std::uint64_t>(value.st_dev) != expected.device ||
        static_cast<std::uint64_t>(value.st_ino) != expected.inode ||
        static_cast<std::uint64_t>(value.st_uid) != expected.owner_uid ||
        static_cast<std::uint32_t>(value.st_mode & 07777) != required_mode ||
        expected.mode != required_mode) {
        return status::invalid_request;
    }

    struct statx extended {};
    ++output.root_fixture_syscall_count;
    if (::syscall(SYS_statx, fd, "", AT_EMPTY_PATH | AT_STATX_SYNC_AS_STAT,
                  STATX_BASIC_STATS | STATX_MNT_ID, &extended) != 0) {
        return errno == ENOSYS || errno == EINVAL ? status::unsupported : status::io_failure;
    }
    if ((extended.stx_mask & STATX_MNT_ID) == 0) {
        return status::unsupported;
    }
    if (extended.stx_mnt_id != expected.mount_id) {
        return status::invalid_request;
    }

    struct statfs filesystem_stat {};
    ++output.root_fixture_syscall_count;
    if (::fstatfs(fd, &filesystem_stat) != 0) {
        return status::io_failure;
    }
    if (static_cast<unsigned long>(filesystem_stat.f_type) != BTRFS_SUPER_MAGIC) {
        return status::unsupported;
    }

    struct btrfs_ioctl_fs_info_args filesystem_info {};
    ++output.root_fixture_syscall_count;
    if (::ioctl(fd, BTRFS_IOC_FS_INFO, &filesystem_info) != 0) {
        return errno == ENOTTY || errno == EOPNOTSUPP || errno == EINVAL
                   ? status::unsupported
                   : status::io_failure;
    }
    if (std::memcmp(filesystem_info.fsid, expected.filesystem_uuid, 16) != 0) {
        return status::invalid_request;
    }

    struct btrfs_ioctl_get_subvol_info_args subvolume_info {};
    ++output.root_fixture_syscall_count;
    if (::ioctl(fd, BTRFS_IOC_GET_SUBVOL_INFO, &subvolume_info) != 0) {
        return errno == ENOTTY || errno == EOPNOTSUPP || errno == EINVAL
                   ? status::unsupported
                   : status::io_failure;
    }
    if (std::memcmp(subvolume_info.uuid, expected.subvolume_uuid, 16) != 0) {
        return status::invalid_request;
    }
    return fd_canonical_path(fd, expected, output);
}

status inspect_layout(int fd, bool fixture, audit & output) noexcept {
    std::array<char, 4096> bytes{};
    std::size_t entries = 0;
    bool saw_lock = false;
    for (;;) {
        ++output.root_fixture_syscall_count;
        const long count = ::syscall(SYS_getdents64, fd, bytes.data(), bytes.size());
        if (count < 0) {
            if (errno == EINTR) {
                continue;
            }
            return status::io_failure;
        }
        if (count == 0) {
            break;
        }
        long offset = 0;
        while (offset < count) {
            const auto * entry = reinterpret_cast<const linux_dirent64 *>(bytes.data() + offset);
            if (entry->d_reclen < offsetof(linux_dirent64, d_name) + 1 ||
                offset + entry->d_reclen > count) {
                return status::io_failure;
            }
            const std::size_t name_capacity = entry->d_reclen - offsetof(linux_dirent64, d_name);
            if (::strnlen(entry->d_name, name_capacity) == name_capacity) {
                return status::io_failure;
            }
            offset += entry->d_reclen;
            if (std::strcmp(entry->d_name, ".") == 0 || std::strcmp(entry->d_name, "..") == 0) {
                continue;
            }
            if (++entries > max_directory_entries) {
                return status::invalid_request;
            }
            if (!fixture || saw_lock || std::strcmp(entry->d_name, "primitive.lock") != 0) {
                return status::invalid_request;
            }
            saw_lock = true;
        }
    }
    if (fixture) {
        output.fixture_layout_exact = entries == 1 && saw_lock;
        return output.fixture_layout_exact ? status::ok_non_authoritative : status::invalid_request;
    }
    output.candidate_root_empty = entries == 0;
    return output.candidate_root_empty ? status::ok_non_authoritative : status::invalid_request;
}

bool csv_has_ro(const char * begin, const char * end) noexcept {
    const char * item = begin;
    while (item < end) {
        const char * comma = item;
        while (comma < end && *comma != ',') {
            ++comma;
        }
        if (comma - item == 2 && item[0] == 'r' && item[1] == 'o') {
            return true;
        }
        item = comma < end ? comma + 1 : end;
    }
    return false;
}

status mountinfo_read_only(std::uint64_t mount_id, bool & read_only,
                           audit & output) noexcept {
    std::array<char, mountinfo_capacity + 1> bytes{};
    ++output.root_fixture_syscall_count;
    int fd = ::open("/proc/self/mountinfo", O_RDONLY | O_CLOEXEC);
    if (fd < 0) {
        return status::io_failure;
    }
    std::size_t total = 0;
    for (;;) {
        ++output.root_fixture_syscall_count;
        const ssize_t count = ::read(fd, bytes.data() + total, mountinfo_capacity - total);
        if (count < 0) {
            if (errno == EINTR) {
                continue;
            }
            ::close(fd);
            return status::io_failure;
        }
        if (count == 0) {
            break;
        }
        total += static_cast<std::size_t>(count);
        if (total == mountinfo_capacity) {
            char extra = 0;
            ++output.root_fixture_syscall_count;
            const ssize_t extra_count = ::read(fd, &extra, 1);
            if (extra_count != 0) {
                ::close(fd);
                return status::io_failure;
            }
            break;
        }
    }
    if (::close(fd) != 0) {
        return status::io_failure;
    }
    bytes[total] = '\0';

    const char * cursor = bytes.data();
    const char * finish = bytes.data() + total;
    while (cursor < finish) {
        const char * line_end = static_cast<const char *>(std::memchr(cursor, '\n', finish - cursor));
        if (line_end == nullptr) {
            line_end = finish;
        }
        std::uint64_t parsed = 0;
        const char * p = cursor;
        while (p < line_end && *p >= '0' && *p <= '9') {
            if (parsed > (std::numeric_limits<std::uint64_t>::max() - (*p - '0')) / 10) {
                return status::io_failure;
            }
            parsed = parsed * 10 + static_cast<unsigned>(*p - '0');
            ++p;
        }
        if (p < line_end && *p == ' ' && parsed == mount_id) {
            const char * fields[6]{};
            std::size_t field_count = 0;
            const char * field = cursor;
            while (field < line_end && field_count < 6) {
                fields[field_count++] = field;
                const char * space = static_cast<const char *>(std::memchr(field, ' ', line_end - field));
                if (space == nullptr) {
                    break;
                }
                field = space + 1;
            }
            if (field_count < 6) {
                return status::io_failure;
            }
            const char * mount_options_end = static_cast<const char *>(
                std::memchr(fields[5], ' ', line_end - fields[5]));
            if (mount_options_end == nullptr) {
                return status::io_failure;
            }
            const char * separator = nullptr;
            for (const char * q = mount_options_end; q + 2 < line_end; ++q) {
                if (q[0] == ' ' && q[1] == '-' && q[2] == ' ') {
                    separator = q;
                    break;
                }
            }
            if (separator == nullptr) {
                return status::io_failure;
            }
            const char * super = separator + 3;
            for (int field_index = 0; field_index < 2; ++field_index) {
                super = static_cast<const char *>(std::memchr(super, ' ', line_end - super));
                if (super == nullptr) {
                    return status::io_failure;
                }
                ++super;
            }
            read_only = csv_has_ro(fields[5], mount_options_end) ||
                        csv_has_ro(super, line_end);
            return status::ok_non_authoritative;
        }
        cursor = line_end < finish ? line_end + 1 : finish;
    }
    return status::invalid_request;
}

status reserve_and_read_only(int root_fd, const pinned_path_identity & root,
                             audit & output) noexcept {
    struct statvfs values {};
    ++output.root_fixture_syscall_count;
    if (::fstatvfs(root_fd, &values) != 0) {
        return status::io_failure;
    }
    if (values.f_frsize != 0 && values.f_bavail >
            std::numeric_limits<std::uint64_t>::max() / values.f_frsize) {
        return status::io_failure;
    }
    output.observed_filesystem_reserve =
        static_cast<std::uint64_t>(values.f_bavail) * values.f_frsize;
    bool mount_read_only = false;
    status result = mountinfo_read_only(root.mount_id, mount_read_only, output);
    if (result != status::ok_non_authoritative) {
        return result;
    }
    if ((values.f_flag & ST_RDONLY) != 0 || mount_read_only) {
        return status::unavailable;
    }
    output.filesystem_not_reported_read_only = true;
    return output.observed_filesystem_reserve >= required_filesystem_reserve
               ? status::ok_non_authoritative
               : status::unavailable;
}

status inspect_lock_file(int lock_fd, const request & input,
                         audit & output) noexcept {
    struct stat value {};
    ++output.root_fixture_syscall_count;
    if (::fstat(lock_fd, &value) != 0) {
        return status::io_failure;
    }
    if (!S_ISREG(value.st_mode) || static_cast<std::uint64_t>(value.st_uid) != input.fixture.owner_uid ||
        static_cast<std::uint32_t>(value.st_mode & 07777) != 0600 || value.st_nlink != 1 ||
        value.st_size != 0 || static_cast<std::uint64_t>(value.st_dev) != input.fixture_lock_device ||
        static_cast<std::uint64_t>(value.st_ino) != input.fixture_lock_inode ||
        static_cast<std::uint64_t>(value.st_dev) != input.fixture.device) {
        return status::invalid_request;
    }
    struct statx extended {};
    ++output.root_fixture_syscall_count;
    if (::syscall(SYS_statx, lock_fd, "", AT_EMPTY_PATH | AT_STATX_SYNC_AS_STAT,
                  STATX_BASIC_STATS | STATX_MNT_ID, &extended) != 0) {
        return errno == ENOSYS || errno == EINVAL ? status::unsupported : status::io_failure;
    }
    if ((extended.stx_mask & STATX_MNT_ID) == 0) {
        return status::unsupported;
    }
    if (extended.stx_mnt_id != input.fixture.mount_id) {
        return status::invalid_request;
    }
    output.fixture_lock_identity_matched = true;
    return status::ok_non_authoritative;
}

std::uint64_t timespec_ns(const struct timespec & value) noexcept {
    return static_cast<std::uint64_t>(value.tv_sec) * 1000000000ULL +
           static_cast<std::uint64_t>(value.tv_nsec);
}

status acquire_ofd_lock(int fd, audit & output) noexcept {
    struct timespec start {};
    if (::clock_gettime(CLOCK_MONOTONIC, &start) != 0) {
        return status::io_failure;
    }
    const std::uint64_t start_ns = timespec_ns(start);
    if (start_ns > std::numeric_limits<std::uint64_t>::max() - lock_deadline_nanoseconds) {
        return status::io_failure;
    }
    const std::uint64_t deadline = start_ns + lock_deadline_nanoseconds;
    struct flock lock {};
    lock.l_type = F_WRLCK;
    lock.l_whence = SEEK_SET;
    lock.l_start = 0;
    lock.l_len = 0;

    bool first_attempt = true;
    for (;;) {
        if (!first_attempt) {
            struct timespec before_attempt {};
            if (::clock_gettime(CLOCK_MONOTONIC, &before_attempt) != 0) {
                return status::io_failure;
            }
            if (timespec_ns(before_attempt) >= deadline) {
                return status::busy;
            }
        }
        first_attempt = false;
        ++output.ofd_attempt_count;
        if (::fcntl(fd, F_OFD_SETLK, &lock) == 0) {
            output.ofd_lock_acquired = true;
            return status::ok_non_authoritative;
        }
        const int error = errno;
        if (error == EINTR) {
            continue;
        }
        if (error == EINVAL || error == ENOSYS) {
            return status::unsupported;
        }
        if (error != EAGAIN && error != EACCES) {
            return status::io_failure;
        }
        struct timespec now {};
        if (::clock_gettime(CLOCK_MONOTONIC, &now) != 0) {
            return status::io_failure;
        }
        if (timespec_ns(now) >= deadline) {
            return status::busy;
        }
        const std::uint64_t now_ns = timespec_ns(now);
        const std::uint64_t remaining = deadline - now_ns;
        const std::uint64_t sleep_ns = remaining < lock_retry_nanoseconds
                                           ? remaining
                                           : lock_retry_nanoseconds;
        struct timespec wake {
            static_cast<time_t>((now_ns + sleep_ns) / 1000000000ULL),
            static_cast<long>((now_ns + sleep_ns) % 1000000000ULL),
        };
        int sleep_result;
        do {
            sleep_result = ::clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME,
                                             &wake, nullptr);
        } while (sleep_result == EINTR);
        if (sleep_result != 0) {
            return status::io_failure;
        }
    }
}

status compare_open_descriptions(int held_fd, int current_fd,
                                 audit & output) noexcept {
    struct stat held {};
    struct stat current {};
    ++output.root_fixture_syscall_count;
    const bool held_ok = ::fstat(held_fd, &held) == 0;
    ++output.root_fixture_syscall_count;
    const bool current_ok = ::fstat(current_fd, &current) == 0;
    if (!held_ok || !current_ok) {
        return status::io_failure;
    }
    return held.st_dev == current.st_dev && held.st_ino == current.st_ino
               ? status::ok_non_authoritative
               : status::invalid_request;
}

status revalidate_parent(int held_parent_fd, const pinned_path_identity & expected,
                         audit & output) noexcept {
    ++output.root_fixture_syscall_count;
    int reopened = ::open(expected.canonical_path,
                          O_RDONLY | O_DIRECTORY | O_CLOEXEC | O_NOFOLLOW);
    if (reopened < 0) {
        const status mapped = map_containment_error(errno);
        return mapped == status::unsupported ? mapped : status::invalid_request;
    }
    status result = compare_open_descriptions(held_parent_fd, reopened, output);
    if (result == status::ok_non_authoritative) {
        result = inspect_directory(reopened, expected, expected.mode, output);
    }
    if (::close(reopened) != 0 && result == status::ok_non_authoritative) {
        result = status::io_failure;
    }
    return result;
}

status revalidate_opened_directory(int parent_fd,
                                   const pinned_path_identity & parent,
                                   const pinned_path_identity & expected,
                                   int held_fd, bool fixture, audit & output) noexcept {
    status error = status::io_failure;
    int reopened = open_contained(
        parent_fd, child_name(parent, expected),
        O_RDONLY | O_DIRECTORY | O_CLOEXEC | O_NOFOLLOW, output, error);
    if (reopened < 0) {
        return error == status::unsupported ? error : status::invalid_request;
    }
    status result = compare_open_descriptions(held_fd, reopened, output);
    if (result == status::ok_non_authoritative) {
        result = inspect_directory(reopened, expected, 0700, output);
    }
    if (result == status::ok_non_authoritative) {
        result = inspect_layout(reopened, fixture, output);
    }
    if (result == status::ok_non_authoritative && !fixture) {
        result = reserve_and_read_only(reopened, expected, output);
    }
    if (::close(reopened) != 0 && result == status::ok_non_authoritative) {
        result = status::io_failure;
    }
    return result;
}

audit finish(audit output, credential_state & credential, status result,
             int & lock_fd, int & fixture_fd, int & root_fd, int & parent_fd,
             bool root_guard_owned) noexcept {
    wipe_credential(credential, output);
    if (credential.fd_present) {
        if (::close(credential_fd) != 0 && errno != EBADF && result == status::ok_non_authoritative) {
            result = status::io_failure;
        }
        credential.fd_present = false;
    }
    if (!close_fd(lock_fd) || !close_fd(fixture_fd) || !close_fd(root_fd) ||
        !close_fd(parent_fd)) {
        if (result == status::ok_non_authoritative) {
            result = status::io_failure;
        }
    }
    if (root_guard_owned) {
        root_guard_device.store(0, std::memory_order_relaxed);
        root_guard_inode.store(0, std::memory_order_relaxed);
        root_guard_state.store(0, std::memory_order_release);
    }
    output.result = result;
    return output;
}

void clear_positive_storage_audit(audit & output) noexcept {
    output.parent_identity_matched = false;
    output.candidate_root_identity_matched = false;
    output.fixture_identity_matched = false;
    output.fixture_lock_identity_matched = false;
    output.candidate_root_empty = false;
    output.fixture_layout_exact = false;
    output.filesystem_not_reported_read_only = false;
    output.ofd_lock_acquired = false;
    output.observed_filesystem_reserve = 0;
}

} // namespace

audit qualify_once(const request & input) noexcept {
    audit output{};
    credential_state credential{};
    int parent_fd = -1;
    int root_fd = -1;
    int fixture_fd = -1;
    int lock_fd = -1;
    bool guard_owned = false;

    for (;;) {
        std::uint8_t expected_guard_state = 0;
        if (root_guard_state.compare_exchange_strong(expected_guard_state, 1,
                                                     std::memory_order_acq_rel,
                                                     std::memory_order_acquire)) {
            root_guard_device.store(input.candidate_root.device, std::memory_order_relaxed);
            root_guard_inode.store(input.candidate_root.inode, std::memory_order_relaxed);
            root_guard_state.store(2, std::memory_order_release);
            guard_owned = true;
            break;
        }
        if (expected_guard_state == 1) {
            continue;
        }
        const std::uint64_t guarded_device =
            root_guard_device.load(std::memory_order_relaxed);
        const std::uint64_t guarded_inode =
            root_guard_inode.load(std::memory_order_relaxed);
        if (root_guard_state.load(std::memory_order_acquire) != 2) {
            continue;
        }
        // This is not a second credential session: file descriptors are
        // process-wide, so closing or claiming work on fd 3 here would corrupt
        // the active one-shot owner. The fast path performs no credential or
        // root syscall and leaves all credential audit facts false.
        output.result = guarded_device == input.candidate_root.device &&
                                guarded_inode == input.candidate_root.inode
                            ? status::busy
                            : status::invalid_request;
        return output;
    }

    if (!request_lexically_valid(input)) {
        return finish(output, credential, status::invalid_request, lock_fd, fixture_fd,
                      root_fd, parent_fd, guard_owned);
    }

    if (session_consumed.test_and_set(std::memory_order_acq_rel)) {
        return finish(output, credential, status::invalid_request, lock_fd, fixture_fd,
                      root_fd, parent_fd, guard_owned);
    }

    status result = admit_credential(input, credential, output);
    if (result != status::ok_non_authoritative) {
        return finish(output, credential, result, lock_fd, fixture_fd, root_fd,
                      parent_fd, guard_owned);
    }

    ++output.root_fixture_syscall_count;
    parent_fd = ::open(input.parent.canonical_path,
                       O_RDONLY | O_DIRECTORY | O_CLOEXEC | O_NOFOLLOW);
    if (parent_fd < 0) {
        result = map_containment_error(errno);
        return finish(output, credential, result, lock_fd, fixture_fd, root_fd,
                      parent_fd, guard_owned);
    }
    result = inspect_directory(parent_fd, input.parent, input.parent.mode, output);
    if (result != status::ok_non_authoritative) {
        return finish(output, credential, result, lock_fd, fixture_fd, root_fd,
                      parent_fd, guard_owned);
    }
    output.parent_identity_matched = true;

    root_fd = open_contained(parent_fd, child_name(input.parent, input.candidate_root),
                             O_RDONLY | O_DIRECTORY | O_CLOEXEC | O_NOFOLLOW,
                             output, result);
    if (root_fd < 0) {
        return finish(output, credential, result, lock_fd, fixture_fd, root_fd,
                      parent_fd, guard_owned);
    }
    result = inspect_directory(root_fd, input.candidate_root, 0700, output);
    if (result == status::ok_non_authoritative) {
        output.candidate_root_identity_matched = true;
        result = inspect_layout(root_fd, false, output);
    }
    if (result == status::ok_non_authoritative) {
        result = reserve_and_read_only(root_fd, input.candidate_root, output);
    }
    if (result != status::ok_non_authoritative) {
        return finish(output, credential, result, lock_fd, fixture_fd, root_fd,
                      parent_fd, guard_owned);
    }

    fixture_fd = open_contained(parent_fd, child_name(input.parent, input.fixture),
                                O_RDONLY | O_DIRECTORY | O_CLOEXEC | O_NOFOLLOW,
                                output, result);
    if (fixture_fd < 0) {
        return finish(output, credential, result, lock_fd, fixture_fd, root_fd,
                      parent_fd, guard_owned);
    }
    result = inspect_directory(fixture_fd, input.fixture, 0700, output);
    if (result == status::ok_non_authoritative) {
        output.fixture_identity_matched = true;
        result = inspect_layout(fixture_fd, true, output);
    }
    if (result != status::ok_non_authoritative) {
        return finish(output, credential, result, lock_fd, fixture_fd, root_fd,
                      parent_fd, guard_owned);
    }

    lock_fd = open_contained(fixture_fd, "primitive.lock",
                             O_RDWR | O_CLOEXEC | O_NOFOLLOW, output, result);
    if (lock_fd < 0) {
        return finish(output, credential, result, lock_fd, fixture_fd, root_fd,
                      parent_fd, guard_owned);
    }
    result = inspect_lock_file(lock_fd, input, output);
    if (result != status::ok_non_authoritative) {
        return finish(output, credential, result, lock_fd, fixture_fd, root_fd,
                      parent_fd, guard_owned);
    }

    result = acquire_ofd_lock(lock_fd, output);
    if (result != status::ok_non_authoritative) {
        return finish(output, credential, result, lock_fd, fixture_fd, root_fd,
                      parent_fd, guard_owned);
    }

    result = revalidate_parent(parent_fd, input.parent, output);
    if (result == status::ok_non_authoritative) {
        result = revalidate_opened_directory(parent_fd, input.parent,
                                             input.candidate_root, root_fd,
                                             false, output);
    }
    if (result == status::ok_non_authoritative) {
        result = revalidate_opened_directory(parent_fd, input.parent,
                                             input.fixture, fixture_fd,
                                             true, output);
    }
    if (result == status::ok_non_authoritative) {
        status reopen_error = status::io_failure;
        int reopened_lock = open_contained(fixture_fd, "primitive.lock",
                                           O_RDWR | O_CLOEXEC | O_NOFOLLOW,
                                           output, reopen_error);
        if (reopened_lock < 0) {
            result = reopen_error == status::unsupported ? reopen_error
                                                          : status::invalid_request;
        } else {
            result = compare_open_descriptions(lock_fd, reopened_lock, output);
            if (result == status::ok_non_authoritative) {
                result = inspect_lock_file(reopened_lock, input, output);
            }
            if (::close(reopened_lock) != 0 && result == status::ok_non_authoritative) {
                result = status::io_failure;
            }
        }
    }

    if (result != status::ok_non_authoritative) {
        clear_positive_storage_audit(output);
    }

    return finish(output, credential, result, lock_fd, fixture_fd, root_fd,
                  parent_fd, guard_owned);
}

} // namespace halofpx::registry_lab::linux_preinit
