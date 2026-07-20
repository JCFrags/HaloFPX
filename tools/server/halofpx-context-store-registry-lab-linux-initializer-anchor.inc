#include "halofpx-context-store-registry-lab-linux-initializer-internal.h"

#if !defined(__linux__)
#error "The HaloFPX registry-lab writer-lock anchor is Linux-only"
#endif

#include <array>
#include <atomic>
#include <cerrno>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <limits>

#include <fcntl.h>
#include <linux/btrfs.h>
#include <linux/fs.h>
#include <linux/magic.h>
#include <linux/openat2.h>
#include <linux/stat.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/statfs.h>
#include <sys/statvfs.h>
#include <sys/syscall.h>
#include <time.h>
#include <unistd.h>

#if !defined(SYS_fchmodat2)
#if defined(__x86_64__)
#define SYS_fchmodat2 452
#else
#error "HaloFPX L05x requires a target syscall number for fchmodat2"
#endif
#endif

namespace halofpx::registry_lab::linux_initializer {
namespace {

constexpr std::uint64_t required_resolve = RESOLVE_BENEATH | RESOLVE_NO_SYMLINKS |
                                           RESOLVE_NO_MAGICLINKS | RESOLVE_NO_XDEV;
constexpr std::uint64_t retry_nanoseconds = 10ULL * 1000ULL * 1000ULL;
constexpr std::uint64_t deadline_nanoseconds = 5ULL * 1000ULL * 1000ULL * 1000ULL;
constexpr std::size_t max_directory_entries = 64;
constexpr std::size_t mountinfo_capacity = 256 * 1024;

std::atomic<std::uint8_t> root_guard_state { 0 };
std::atomic<std::uint64_t> root_guard_device { 0 };
std::atomic<std::uint64_t> root_guard_inode { 0 };

enum class root_layout : std::uint8_t {
    empty,
    writer_lock_only,
    nonempty,
    io_failure,
};

enum class initialization_extent : std::uint8_t {
    writer_lock_anchor,
    directory_prefix,
    initializing_marker,
    predecessor_envelope,
};

struct created_directory_identity {
    std::uint64_t device = 0;
    std::uint64_t inode = 0;
    std::uint64_t mount_id = 0;
};

struct created_file_identity {
    std::uint64_t device = 0;
    std::uint64_t inode = 0;
    std::uint64_t mount_id = 0;
};

bool close_fd(int & fd) noexcept {
    if (fd < 0) {
        return true;
    }
    const int owned = fd;
    fd = -1;
    return ::close(owned) == 0;
}

bool same_16(const std::uint8_t lhs[16], const std::uint8_t rhs[16]) noexcept {
    return std::memcmp(lhs, rhs, 16) == 0;
}

bool fixed_path_valid(const initialization_pinned_path_identity & identity) noexcept {
    const std::size_t size = identity.path_length;
    if (size == 0 || size > initialization_max_path_bytes ||
        identity.canonical_path[0] != '/' || identity.canonical_path[size] != '\0' ||
        ::strnlen(identity.canonical_path, initialization_max_path_bytes + 1) != size ||
        (size > 1 && identity.canonical_path[size - 1] == '/')) {
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

bool equal_or_descendant(const initialization_pinned_path_identity & identity,
                         const char * protected_path) noexcept {
    const std::size_t protected_size = std::strlen(protected_path);
    return identity.path_length >= protected_size &&
           std::memcmp(identity.canonical_path, protected_path, protected_size) == 0 &&
           (identity.path_length == protected_size ||
            identity.canonical_path[protected_size] == '/');
}

bool protected_path(const initialization_pinned_path_identity & identity) noexcept {
    if (identity.path_length == 1) {
        return true;
    }
    constexpr const char * protected_paths[] = {
        "/boot", "/dev", "/etc", "/home", "/opt", "/proc", "/root", "/run",
        "/srv", "/sys", "/usr", "/var/cache", "/var/lib", "/var/log",
    };
    for (const char * path : protected_paths) {
        if (equal_or_descendant(identity, path)) {
            return true;
        }
    }
    return false;
}

bool immediate_child(const initialization_pinned_path_identity & parent,
                     const initialization_pinned_path_identity & child) noexcept {
    if (child.path_length <= parent.path_length + 1 ||
        std::memcmp(parent.canonical_path, child.canonical_path, parent.path_length) != 0 ||
        child.canonical_path[parent.path_length] != '/') {
        return false;
    }
    for (std::size_t i = parent.path_length + 1; i < child.path_length; ++i) {
        if (child.canonical_path[i] == '/') {
            return false;
        }
    }
    return true;
}

const char * child_name(const initialization_pinned_path_identity & parent,
                        const initialization_pinned_path_identity & child) noexcept {
    return child.canonical_path + parent.path_length + 1;
}

bool request_valid(const initialization_request & input) noexcept {
    if (!fixed_path_valid(input.parent) || !fixed_path_valid(input.candidate_root) ||
        !fixed_path_valid(input.fixture) || protected_path(input.parent) ||
        protected_path(input.candidate_root) || protected_path(input.fixture) ||
        !immediate_child(input.parent, input.candidate_root) ||
        !immediate_child(input.parent, input.fixture) ||
        (input.candidate_root.path_length == input.fixture.path_length &&
         std::memcmp(input.candidate_root.canonical_path, input.fixture.canonical_path,
                     input.candidate_root.path_length) == 0)) {
        return false;
    }
    return input.parent.owner_uid == input.candidate_root.owner_uid &&
           input.parent.owner_uid == input.fixture.owner_uid &&
           input.parent.owner_uid == static_cast<std::uint64_t>(::geteuid()) &&
           input.parent.device == input.candidate_root.device &&
           input.parent.device == input.fixture.device &&
           input.parent.mount_id == input.candidate_root.mount_id &&
           input.parent.mount_id == input.fixture.mount_id &&
           same_16(input.parent.filesystem_uuid, input.candidate_root.filesystem_uuid) &&
           same_16(input.parent.filesystem_uuid, input.fixture.filesystem_uuid) &&
           same_16(input.parent.subvolume_uuid, input.candidate_root.subvolume_uuid) &&
           same_16(input.parent.subvolume_uuid, input.fixture.subvolume_uuid) &&
           input.candidate_root.mode == 0700 && input.fixture.mode == 0700 &&
           input.fixture_lock_device == input.fixture.device &&
           input.parent.inode != 0 && input.candidate_root.inode != 0 &&
           input.fixture.inode != 0 && input.fixture_lock_inode != 0;
}

initialization_status map_error(int value) noexcept {
    switch (value) {
        case ENOSYS:
        case EINVAL:
            return initialization_status::unsupported_no_mutation;
        case ELOOP:
        case EXDEV:
        case ENOENT:
        case ENOTDIR:
        case EACCES:
        case EPERM:
        case EEXIST:
            return initialization_status::invalid_request_no_mutation;
        case ENOSPC:
        case EDQUOT:
        case EROFS:
            return initialization_status::unavailable_no_mutation;
        default:
            return initialization_status::io_failure_no_mutation;
    }
}

int open_contained(int parent_fd, const char * name, std::uint64_t flags,
                   std::uint64_t mode, initialization_audit & output,
                   initialization_status & error) noexcept {
    struct open_how how {};
    how.flags = flags;
    how.mode = mode;
    how.resolve = required_resolve;
    ++output.root_fixture_syscall_count;
    const int result = static_cast<int>(::syscall(
        SYS_openat2, parent_fd, name, &how, sizeof(how)));
    if (result < 0) {
        error = map_error(errno);
    }
    return result;
}

initialization_status fd_canonical_path(
        int fd, const initialization_pinned_path_identity & expected,
        initialization_audit & output) noexcept {
    std::array<char, initialization_max_path_bytes + 1> path {};
    std::array<char, 32> proc_name {};
    const int written = std::snprintf(proc_name.data(), proc_name.size(),
                                      "/proc/self/fd/%d", fd);
    if (written <= 0 || static_cast<std::size_t>(written) >= proc_name.size()) {
        return initialization_status::io_failure_no_mutation;
    }
    ++output.root_fixture_syscall_count;
    const ssize_t count = ::readlink(proc_name.data(), path.data(), path.size());
    return count == static_cast<ssize_t>(expected.path_length) &&
                   std::memcmp(path.data(), expected.canonical_path,
                               expected.path_length) == 0
               ? initialization_status::initialization_discard_required
               : initialization_status::invalid_request_no_mutation;
}

initialization_status inspect_directory(
        int fd, const initialization_pinned_path_identity & expected,
        std::uint32_t required_mode, initialization_audit & output) noexcept {
    struct stat value {};
    ++output.root_fixture_syscall_count;
    if (::fstat(fd, &value) != 0) {
        return initialization_status::io_failure_no_mutation;
    }
    if (!S_ISDIR(value.st_mode) || static_cast<std::uint64_t>(value.st_dev) != expected.device ||
        static_cast<std::uint64_t>(value.st_ino) != expected.inode ||
        static_cast<std::uint64_t>(value.st_uid) != expected.owner_uid ||
        static_cast<std::uint32_t>(value.st_mode & 07777) != required_mode ||
        expected.mode != required_mode) {
        return initialization_status::invalid_request_no_mutation;
    }
    struct statx extended {};
    ++output.root_fixture_syscall_count;
    if (::syscall(SYS_statx, fd, "", AT_EMPTY_PATH | AT_STATX_SYNC_AS_STAT,
                  STATX_BASIC_STATS | STATX_MNT_ID, &extended) != 0) {
        return errno == ENOSYS || errno == EINVAL
                   ? initialization_status::unsupported_no_mutation
                   : initialization_status::io_failure_no_mutation;
    }
    if ((extended.stx_mask & STATX_MNT_ID) == 0) {
        return initialization_status::unsupported_no_mutation;
    }
    if (extended.stx_mnt_id != expected.mount_id) {
        return initialization_status::invalid_request_no_mutation;
    }
    struct statfs filesystem {};
    ++output.root_fixture_syscall_count;
    if (::fstatfs(fd, &filesystem) != 0) {
        return initialization_status::io_failure_no_mutation;
    }
    if (static_cast<unsigned long>(filesystem.f_type) != BTRFS_SUPER_MAGIC) {
        return initialization_status::unsupported_no_mutation;
    }
    struct btrfs_ioctl_fs_info_args filesystem_info {};
    ++output.root_fixture_syscall_count;
    if (::ioctl(fd, BTRFS_IOC_FS_INFO, &filesystem_info) != 0) {
        return errno == ENOTTY || errno == EOPNOTSUPP || errno == EINVAL
                   ? initialization_status::unsupported_no_mutation
                   : initialization_status::io_failure_no_mutation;
    }
    if (std::memcmp(filesystem_info.fsid, expected.filesystem_uuid, 16) != 0) {
        return initialization_status::invalid_request_no_mutation;
    }
    struct btrfs_ioctl_get_subvol_info_args subvolume_info {};
    ++output.root_fixture_syscall_count;
    if (::ioctl(fd, BTRFS_IOC_GET_SUBVOL_INFO, &subvolume_info) != 0) {
        return errno == ENOTTY || errno == EOPNOTSUPP || errno == EINVAL
                   ? initialization_status::unsupported_no_mutation
                   : initialization_status::io_failure_no_mutation;
    }
    if (std::memcmp(subvolume_info.uuid, expected.subvolume_uuid, 16) != 0) {
        return initialization_status::invalid_request_no_mutation;
    }
    return fd_canonical_path(fd, expected, output);
}

root_layout inspect_root_layout(int fd, bool expect_writer,
                                initialization_audit & output) noexcept {
    std::array<char, 4096> bytes {};
    std::size_t entries = 0;
    bool saw_writer = false;
    for (;;) {
        ++output.root_fixture_syscall_count;
        const long count = ::syscall(SYS_getdents64, fd, bytes.data(), bytes.size());
        if (count < 0) {
            if (errno == EINTR) {
                continue;
            }
            return root_layout::io_failure;
        }
        if (count == 0) {
            break;
        }
        long offset = 0;
        while (offset < count) {
            const auto * entry = reinterpret_cast<const linux_dirent64 *>(bytes.data() + offset);
            if (entry->d_reclen < offsetof(linux_dirent64, d_name) + 1 ||
                offset + entry->d_reclen > count) {
                return root_layout::io_failure;
            }
            const std::size_t capacity = entry->d_reclen - offsetof(linux_dirent64, d_name);
            if (::strnlen(entry->d_name, capacity) == capacity) {
                return root_layout::io_failure;
            }
            offset += entry->d_reclen;
            if (std::strcmp(entry->d_name, ".") == 0 ||
                std::strcmp(entry->d_name, "..") == 0) {
                continue;
            }
            ++entries;
            if (entries > max_directory_entries) {
                return root_layout::nonempty;
            }
            if (std::strcmp(entry->d_name, "writer.lock") == 0) {
                if (saw_writer) {
                    return root_layout::nonempty;
                }
                saw_writer = true;
            } else {
                return root_layout::nonempty;
            }
        }
    }
    if (!expect_writer && entries == 0) {
        return root_layout::empty;
    }
    if (expect_writer && entries == 1 && saw_writer) {
        return root_layout::writer_lock_only;
    }
    return root_layout::nonempty;
}

initialization_status inspect_accumulated_prefix_layout(
        int fd, unsigned expected_directories, bool expect_marker,
        initialization_audit & output, bool retry_eintr = true) noexcept {
    if (expected_directories == 0 || expected_directories > 3) {
        return initialization_status::invalid_request_no_mutation;
    }
    std::array<char, 4096> bytes {};
    std::size_t entries = 0;
    bool saw_writer = false;
    bool saw_envelopes = false;
    bool saw_attempts = false;
    bool saw_staging = false;
    bool saw_marker = false;
    for (;;) {
        ++output.root_fixture_syscall_count;
        const long count = ::syscall(SYS_getdents64, fd, bytes.data(), bytes.size());
        if (count < 0) {
            if (errno == EINTR && retry_eintr) {
                continue;
            }
            return initialization_status::io_failure_no_mutation;
        }
        if (count == 0) {
            break;
        }
        long offset = 0;
        while (offset < count) {
            const auto * entry = reinterpret_cast<const linux_dirent64 *>(bytes.data() + offset);
            if (entry->d_reclen < offsetof(linux_dirent64, d_name) + 1 ||
                offset + entry->d_reclen > count) {
                return initialization_status::io_failure_no_mutation;
            }
            const std::size_t capacity = entry->d_reclen - offsetof(linux_dirent64, d_name);
            if (::strnlen(entry->d_name, capacity) == capacity) {
                return initialization_status::io_failure_no_mutation;
            }
            offset += entry->d_reclen;
            if (std::strcmp(entry->d_name, ".") == 0 ||
                std::strcmp(entry->d_name, "..") == 0) {
                continue;
            }
            if (++entries > expected_directories + 1 + (expect_marker ? 1U : 0U)) {
                return initialization_status::invalid_request_no_mutation;
            }
            bool * seen = nullptr;
            if (std::strcmp(entry->d_name, "writer.lock") == 0) {
                seen = &saw_writer;
            } else if (std::strcmp(entry->d_name, "envelopes") == 0) {
                seen = &saw_envelopes;
            } else if (std::strcmp(entry->d_name, "attempts") == 0) {
                seen = &saw_attempts;
            } else if (std::strcmp(entry->d_name, "staging") == 0) {
                seen = &saw_staging;
            } else if (expect_marker && std::strcmp(entry->d_name, "root.marker") == 0) {
                seen = &saw_marker;
            } else {
                return initialization_status::invalid_request_no_mutation;
            }
            if (*seen) {
                return initialization_status::invalid_request_no_mutation;
            }
            *seen = true;
        }
    }
    return entries == expected_directories + 1 + (expect_marker ? 1U : 0U) &&
                   saw_writer && saw_envelopes &&
                   saw_attempts == (expected_directories >= 2) &&
                   saw_staging == (expected_directories >= 3) &&
                   saw_marker == expect_marker
               ? initialization_status::initialization_discard_required
               : initialization_status::invalid_request_no_mutation;
}

initialization_status inspect_empty_directory(
        int fd, initialization_audit & output,
        bool retry_eintr = true) noexcept {
    std::array<char, 4096> bytes {};
    for (;;) {
        ++output.root_fixture_syscall_count;
        const long count = ::syscall(SYS_getdents64, fd, bytes.data(), bytes.size());
        if (count < 0) {
            if (errno == EINTR && retry_eintr) {
                continue;
            }
            return initialization_status::io_failure_no_mutation;
        }
        if (count == 0) {
            return initialization_status::initialization_discard_required;
        }
        long offset = 0;
        while (offset < count) {
            const auto * entry = reinterpret_cast<const linux_dirent64 *>(bytes.data() + offset);
            if (entry->d_reclen < offsetof(linux_dirent64, d_name) + 1 ||
                offset + entry->d_reclen > count) {
                return initialization_status::io_failure_no_mutation;
            }
            const std::size_t capacity = entry->d_reclen - offsetof(linux_dirent64, d_name);
            if (::strnlen(entry->d_name, capacity) == capacity) {
                return initialization_status::io_failure_no_mutation;
            }
            offset += entry->d_reclen;
            if (std::strcmp(entry->d_name, ".") != 0 &&
                std::strcmp(entry->d_name, "..") != 0) {
                return initialization_status::invalid_request_no_mutation;
            }
        }
    }
}

initialization_status inspect_staging_initializing_marker_temp(
        int fd, initialization_audit & output) noexcept {
    std::array<char, 4096> bytes {};
    bool saw_temp = false;
    for (;;) {
        ++output.root_fixture_syscall_count;
        const long count = ::syscall(SYS_getdents64, fd, bytes.data(), bytes.size());
        if (count < 0) {
            if (errno == EINTR) {
                continue;
            }
            return initialization_status::io_failure_no_mutation;
        }
        if (count == 0) {
            return saw_temp ? initialization_status::initialization_discard_required
                            : initialization_status::invalid_request_no_mutation;
        }
        long offset = 0;
        while (offset < count) {
            const auto * entry = reinterpret_cast<const linux_dirent64 *>(bytes.data() + offset);
            if (entry->d_reclen < offsetof(linux_dirent64, d_name) + 1 ||
                offset + entry->d_reclen > count) {
                return initialization_status::io_failure_no_mutation;
            }
            const std::size_t capacity = entry->d_reclen - offsetof(linux_dirent64, d_name);
            if (::strnlen(entry->d_name, capacity) == capacity) {
                return initialization_status::io_failure_no_mutation;
            }
            offset += entry->d_reclen;
            if (std::strcmp(entry->d_name, ".") == 0 ||
                std::strcmp(entry->d_name, "..") == 0) {
                continue;
            }
            if (saw_temp || std::strcmp(entry->d_name, "initialize-root.tmp") != 0) {
                return initialization_status::invalid_request_no_mutation;
            }
            saw_temp = true;
        }
    }
}

initialization_status inspect_single_directory_entry(
        int fd, const char * expected, initialization_audit & output) noexcept {
    if (!expected || expected[0] == '\0') {
        return initialization_status::invalid_request_no_mutation;
    }
    for (const char * cursor = expected; *cursor != '\0'; ++cursor) {
        if (*cursor == '/') {
            return initialization_status::invalid_request_no_mutation;
        }
    }
    std::array<char, 4096> bytes {};
    bool saw_expected = false;
    for (;;) {
        ++output.root_fixture_syscall_count;
        const long count = ::syscall(SYS_getdents64, fd, bytes.data(), bytes.size());
        if (count < 0) {
            return initialization_status::io_failure_no_mutation;
        }
        if (count == 0) {
            return saw_expected
                       ? initialization_status::initialization_discard_required
                       : initialization_status::invalid_request_no_mutation;
        }
        long offset = 0;
        while (offset < count) {
            const auto * entry = reinterpret_cast<const linux_dirent64 *>(
                bytes.data() + offset);
            if (entry->d_reclen < offsetof(linux_dirent64, d_name) + 1 ||
                offset + entry->d_reclen > count) {
                return initialization_status::io_failure_no_mutation;
            }
            const std::size_t capacity =
                entry->d_reclen - offsetof(linux_dirent64, d_name);
            if (::strnlen(entry->d_name, capacity) == capacity) {
                return initialization_status::io_failure_no_mutation;
            }
            offset += entry->d_reclen;
            if (std::strcmp(entry->d_name, ".") == 0 ||
                std::strcmp(entry->d_name, "..") == 0) {
                continue;
            }
            if (saw_expected || std::strcmp(entry->d_name, expected) != 0) {
                return initialization_status::invalid_request_no_mutation;
            }
            saw_expected = true;
        }
    }
}

initialization_status inspect_created_directory(
        int fd, const char * name, const initialization_request & input,
        created_directory_identity & identity, bool pin,
        bool require_final_mode_and_btrfs_identity,
        initialization_audit & output) noexcept {
    struct stat value {};
    ++output.root_fixture_syscall_count;
    if (::fstat(fd, &value) != 0) {
        return initialization_status::io_failure_no_mutation;
    }
    struct statx extended {};
    ++output.root_fixture_syscall_count;
    if (::syscall(SYS_statx, fd, "", AT_EMPTY_PATH | AT_STATX_SYNC_AS_STAT,
                  STATX_BASIC_STATS | STATX_MNT_ID, &extended) != 0) {
        return errno == ENOSYS || errno == EINVAL
                   ? initialization_status::unsupported_no_mutation
                   : initialization_status::io_failure_no_mutation;
    }
    if (!S_ISDIR(value.st_mode) ||
        static_cast<std::uint64_t>(value.st_uid) != input.candidate_root.owner_uid ||
        static_cast<std::uint64_t>(value.st_dev) != input.candidate_root.device ||
        (!require_final_mode_and_btrfs_identity &&
         ((static_cast<std::uint32_t>(value.st_mode) & 07777U) & ~0700U) != 0) ||
        (require_final_mode_and_btrfs_identity &&
         static_cast<std::uint32_t>(value.st_mode & 07777) != 0700) ||
        (extended.stx_mask & STATX_MNT_ID) == 0 ||
        extended.stx_mnt_id != input.candidate_root.mount_id) {
        return initialization_status::invalid_request_no_mutation;
    }
    struct statfs filesystem {};
    ++output.root_fixture_syscall_count;
    if (::fstatfs(fd, &filesystem) != 0) {
        return initialization_status::io_failure_no_mutation;
    }
    if (static_cast<unsigned long>(filesystem.f_type) != BTRFS_SUPER_MAGIC) {
        return initialization_status::unsupported_no_mutation;
    }
    if (require_final_mode_and_btrfs_identity) {
        struct btrfs_ioctl_fs_info_args filesystem_info {};
        ++output.root_fixture_syscall_count;
        if (::ioctl(fd, BTRFS_IOC_FS_INFO, &filesystem_info) != 0) {
            return errno == ENOTTY || errno == EOPNOTSUPP || errno == EINVAL
                       ? initialization_status::unsupported_no_mutation
                       : initialization_status::io_failure_no_mutation;
        }
        struct btrfs_ioctl_get_subvol_info_args subvolume_info {};
        ++output.root_fixture_syscall_count;
        if (::ioctl(fd, BTRFS_IOC_GET_SUBVOL_INFO, &subvolume_info) != 0) {
            return errno == ENOTTY || errno == EOPNOTSUPP || errno == EINVAL
                       ? initialization_status::unsupported_no_mutation
                       : initialization_status::io_failure_no_mutation;
        }
        if (std::memcmp(filesystem_info.fsid,
                        input.candidate_root.filesystem_uuid, 16) != 0 ||
            std::memcmp(subvolume_info.uuid,
                        input.candidate_root.subvolume_uuid, 16) != 0) {
            return initialization_status::invalid_request_no_mutation;
        }
    }
    const std::size_t name_size = std::strlen(name);
    if (input.candidate_root.path_length + 1 + name_size >
        initialization_max_path_bytes) {
        return initialization_status::invalid_request_no_mutation;
    }
    std::array<char, initialization_max_path_bytes + 1> expected_path {};
    std::memcpy(expected_path.data(), input.candidate_root.canonical_path,
                input.candidate_root.path_length);
    expected_path[input.candidate_root.path_length] = '/';
    std::memcpy(expected_path.data() + input.candidate_root.path_length + 1,
                name, name_size);
    const std::size_t expected_size =
        input.candidate_root.path_length + 1 + name_size;
    std::array<char, initialization_max_path_bytes + 1> observed_path {};
    std::array<char, 32> proc_name {};
    const int proc_size = std::snprintf(proc_name.data(), proc_name.size(),
                                        "/proc/self/fd/%d", fd);
    if (proc_size <= 0 || static_cast<std::size_t>(proc_size) >= proc_name.size()) {
        return initialization_status::io_failure_no_mutation;
    }
    ++output.root_fixture_syscall_count;
    const ssize_t observed_size = ::readlink(
        proc_name.data(), observed_path.data(), observed_path.size());
    if (observed_size != static_cast<ssize_t>(expected_size) ||
        std::memcmp(observed_path.data(), expected_path.data(), expected_size) != 0) {
        return initialization_status::invalid_request_no_mutation;
    }
    if (pin) {
        identity.device = static_cast<std::uint64_t>(value.st_dev);
        identity.inode = static_cast<std::uint64_t>(value.st_ino);
        identity.mount_id = extended.stx_mnt_id;
        return identity.inode != 0
                   ? initialization_status::initialization_discard_required
                   : initialization_status::invalid_request_no_mutation;
    }
    return identity.device == static_cast<std::uint64_t>(value.st_dev) &&
                   identity.inode == static_cast<std::uint64_t>(value.st_ino) &&
                   identity.mount_id == extended.stx_mnt_id
               ? initialization_status::initialization_discard_required
               : initialization_status::invalid_request_no_mutation;
}

initialization_status compare_open_descriptions(int held, int current,
                                                initialization_audit & output) noexcept;

initialization_status create_directory_once(
        int root_fd, const char * name, const initialization_request & input,
        created_directory_identity & identity,
        bool & created, bool & validated, bool & synchronized,
        initialization_audit & output) noexcept {
    int mkdir_result;
    do {
        ++output.root_fixture_syscall_count;
        mkdir_result = static_cast<int>(::syscall(
            SYS_mkdirat, root_fd, name, 0700));
    } while (mkdir_result != 0 && errno == EINTR);
    if (mkdir_result != 0) {
        return map_error(errno);
    }
    created = true;
    initialization_status result = initialization_status::io_failure_no_mutation;
    int path_fd = open_contained(root_fd, name,
                                 O_PATH | O_DIRECTORY | O_CLOEXEC | O_NOFOLLOW,
                                 0, output, result);
    if (path_fd < 0) {
        return result;
    }
    result = inspect_created_directory(path_fd, name, input,
                                       identity, true, false, output);
    if (result != initialization_status::initialization_discard_required) {
        (void) close_fd(path_fd);
        return result;
    }
    // mkdir mode is filtered by the caller's umask. O_PATH pins even a mode-0000
    // directory; the raw fd-bound repair keeps exactly one mode mutation and
    // avoids process-global umask or path-based chmod authority.
    int chmod_result;
    do {
        ++output.root_fixture_syscall_count;
        chmod_result = static_cast<int>(::syscall(
            SYS_fchmodat2, path_fd, "", 0700, AT_EMPTY_PATH));
    } while (chmod_result != 0 && errno == EINTR);
    if (chmod_result != 0) {
        const int chmod_error = errno;
        (void) close_fd(path_fd);
        return chmod_error == ENOSYS || chmod_error == EINVAL ||
                       chmod_error == EOPNOTSUPP
                   ? initialization_status::unsupported_no_mutation
                   : map_error(chmod_error);
    }
    int directory_fd = open_contained(root_fd, name,
                                      O_RDONLY | O_DIRECTORY | O_CLOEXEC | O_NOFOLLOW,
                                      0, output, result);
    if (directory_fd < 0) {
        (void) close_fd(path_fd);
        return result;
    }
    result = compare_open_descriptions(path_fd, directory_fd, output);
    if (result == initialization_status::initialization_discard_required) {
        result = inspect_created_directory(directory_fd, name, input,
                                           identity, false, true, output);
    }
    if (!close_fd(path_fd) &&
        result == initialization_status::initialization_discard_required) {
        result = initialization_status::io_failure_no_mutation;
    }
    if (result == initialization_status::initialization_discard_required) {
        result = inspect_empty_directory(directory_fd, output);
    }
    if (result != initialization_status::initialization_discard_required) {
        (void) close_fd(directory_fd);
        return result;
    }
    validated = true;
    ++output.root_fixture_syscall_count;
    if (::fsync(directory_fd) != 0) {
        (void) close_fd(directory_fd);
        return initialization_status::io_failure_no_mutation;
    }
    synchronized = true;
    return close_fd(directory_fd)
               ? initialization_status::initialization_discard_required
               : initialization_status::io_failure_no_mutation;
}

initialization_status inspect_marker_file(
        int fd, const char * relative_suffix,
        const initialization_request & input,
        created_file_identity & identity, bool pin,
        std::uint64_t expected_size, bool allow_umask_subset,
        initialization_audit & output) noexcept {
    struct stat value {};
    ++output.root_fixture_syscall_count;
    if (::fstat(fd, &value) != 0) {
        return initialization_status::io_failure_no_mutation;
    }
    struct statx extended {};
    ++output.root_fixture_syscall_count;
    if (::syscall(SYS_statx, fd, "", AT_EMPTY_PATH | AT_STATX_SYNC_AS_STAT,
                  STATX_BASIC_STATS | STATX_MNT_ID, &extended) != 0) {
        return errno == ENOSYS || errno == EINVAL
                   ? initialization_status::unsupported_no_mutation
                   : initialization_status::io_failure_no_mutation;
    }
    const std::uint32_t observed_mode =
        static_cast<std::uint32_t>(value.st_mode & 07777);
    if (!S_ISREG(value.st_mode) || value.st_nlink != 1 || value.st_size < 0 ||
        static_cast<std::uint64_t>(value.st_size) != expected_size ||
        static_cast<std::uint64_t>(value.st_uid) != input.candidate_root.owner_uid ||
        static_cast<std::uint64_t>(value.st_dev) != input.candidate_root.device ||
        (allow_umask_subset ? ((observed_mode & ~0600U) != 0) : observed_mode != 0600) ||
        (extended.stx_mask & STATX_MNT_ID) == 0 ||
        extended.stx_mnt_id != input.candidate_root.mount_id) {
        return initialization_status::invalid_request_no_mutation;
    }
    const std::size_t suffix_size = std::strlen(relative_suffix);
    if (input.candidate_root.path_length + suffix_size > initialization_max_path_bytes) {
        return initialization_status::invalid_request_no_mutation;
    }
    std::array<char, initialization_max_path_bytes + 1> expected_path {};
    std::memcpy(expected_path.data(), input.candidate_root.canonical_path,
                input.candidate_root.path_length);
    std::memcpy(expected_path.data() + input.candidate_root.path_length,
                relative_suffix, suffix_size);
    const std::size_t expected_path_size = input.candidate_root.path_length + suffix_size;
    std::array<char, initialization_max_path_bytes + 1> observed_path {};
    std::array<char, 32> proc_name {};
    const int proc_size = std::snprintf(proc_name.data(), proc_name.size(),
                                        "/proc/self/fd/%d", fd);
    if (proc_size <= 0 || static_cast<std::size_t>(proc_size) >= proc_name.size()) {
        return initialization_status::io_failure_no_mutation;
    }
    ++output.root_fixture_syscall_count;
    const ssize_t observed_path_size = ::readlink(
        proc_name.data(), observed_path.data(), observed_path.size());
    if (observed_path_size != static_cast<ssize_t>(expected_path_size) ||
        std::memcmp(observed_path.data(), expected_path.data(), expected_path_size) != 0) {
        return initialization_status::invalid_request_no_mutation;
    }
    if (pin) {
        identity.device = static_cast<std::uint64_t>(value.st_dev);
        identity.inode = static_cast<std::uint64_t>(value.st_ino);
        identity.mount_id = extended.stx_mnt_id;
        return identity.inode != 0
                   ? initialization_status::initialization_discard_required
                   : initialization_status::invalid_request_no_mutation;
    }
    return identity.device == static_cast<std::uint64_t>(value.st_dev) &&
                   identity.inode == static_cast<std::uint64_t>(value.st_ino) &&
                   identity.mount_id == extended.stx_mnt_id
               ? initialization_status::initialization_discard_required
               : initialization_status::invalid_request_no_mutation;
}

initialization_status write_exact_at_zero(
        int fd, const std::uint8_t * bytes, std::size_t size,
        initialization_audit & output) noexcept {
    if (!bytes || size == 0 || size > 1024) {
        return initialization_status::invalid_request_no_mutation;
    }
    std::size_t total = 0;
    while (total < size) {
        ++output.root_fixture_syscall_count;
        const ssize_t count = ::pwrite(fd, bytes + total, size - total,
                                       static_cast<off_t>(total));
        if (count < 0) {
            if (errno == EINTR) {
                continue;
            }
            return map_error(errno);
        }
        if (count == 0 || static_cast<std::size_t>(count) > size - total) {
            return initialization_status::io_failure_no_mutation;
        }
        total += static_cast<std::size_t>(count);
    }
    return initialization_status::initialization_discard_required;
}

initialization_status read_exact_at_zero(
        int fd, std::uint8_t * bytes, std::size_t size,
        initialization_audit & output) noexcept {
    if (!bytes || size == 0 || size > 1024) {
        return initialization_status::invalid_request_no_mutation;
    }
    std::size_t total = 0;
    while (total < size) {
        ++output.root_fixture_syscall_count;
        const ssize_t count = ::pread(fd, bytes + total, size - total,
                                      static_cast<off_t>(total));
        if (count < 0) {
            if (errno == EINTR) {
                continue;
            }
            return initialization_status::io_failure_no_mutation;
        }
        if (count == 0 || static_cast<std::size_t>(count) > size - total) {
            return initialization_status::io_failure_no_mutation;
        }
        total += static_cast<std::size_t>(count);
    }
    std::uint8_t trailing = 0;
    ssize_t trailing_count;
    do {
        ++output.root_fixture_syscall_count;
        trailing_count = ::pread(fd, &trailing, 1, static_cast<off_t>(size));
    } while (trailing_count < 0 && errno == EINTR);
    if (trailing_count < 0) {
        return initialization_status::io_failure_no_mutation;
    }
    return trailing_count == 0
               ? initialization_status::initialization_discard_required
               : initialization_status::invalid_request_no_mutation;
}

bool predecessor_facts_match_request(
        const context_store_protected_registry_facts & facts,
        const sealed_input_request & input) noexcept {
    return same_id(facts.body.registry_id, input.expected_registry_id) &&
           facts.body.registry_epoch == input.expected_registry_epoch &&
           same_digest(facts.body.authority_base_scope_commitment,
                       input.expected_authority_base_scope_commitment) &&
           same_digest(facts.body.policy_commitment,
                       input.expected_policy_commitment) &&
           facts.body.last_consumed_sequence ==
               input.expected_predecessor_high_water &&
           same_id(facts.key_id, input.expected_key_id) &&
           facts.key_generation == input.expected_key_generation &&
           same_digest(facts.key_continuity_commitment,
                       input.expected_key_continuity_commitment);
}

bool authenticate_predecessor_bytes(
        const std::uint8_t * bytes, std::size_t size,
        const authenticated_input_session & session,
        const sealed_input_request & input,
        const context_store_format_digest & expected_digest) noexcept {
    if (!bytes || size == 0 || size > 1024 || !session.wire_credential) {
        return false;
    }
    context_store_protected_registry_key_record key {};
    key.disposition = context_store_key_disposition::active;
    key.key_id = session.wire_credential->key_id;
    key.generation = session.wire_credential->generation;
    key.master_key = {
        session.wire_credential->secret.data(),
        session.wire_credential->secret.size(),
    };
    const auto verified = context_store_verify_protected_registry_facts_v1(
        bytes, size, key);
    key.master_key = {};
    const auto * facts = verified.authenticated_facts();
    context_store_format_digest digest {};
    const bool matched = facts && predecessor_facts_match_request(*facts, input) &&
        context_store_registry_lab_linux_initializer_predecessor_digest_v1(
            bytes, size, digest) && same_digest(digest, expected_digest);
    wipe(digest.data(), digest.size());
    return matched;
}

bool make_predecessor_envelope_name(
        const context_store_format_digest & digest,
        std::array<char, 72> & basename) noexcept {
    constexpr char alphabet[] = "0123456789abcdef";
    basename.fill('\0');
    basename[0] = 'e';
    basename[1] = '-';
    for (std::size_t index = 0; index < digest.size(); ++index) {
        basename[2 + index * 2] = alphabet[digest[index] >> 4];
        basename[3 + index * 2] = alphabet[digest[index] & 0x0f];
    }
    basename[66] = '.';
    basename[67] = 'c';
    basename[68] = 'b';
    basename[69] = 'o';
    basename[70] = 'r';
    return basename[71] == '\0' && std::strlen(basename.data()) == 71;
}

initialization_status inspect_fixture_layout(
        int fd, initialization_audit & output,
        bool retry_eintr = true) noexcept {
    std::array<char, 4096> bytes {};
    std::size_t entries = 0;
    bool saw_lock = false;
    for (;;) {
        ++output.root_fixture_syscall_count;
        const long count = ::syscall(SYS_getdents64, fd, bytes.data(), bytes.size());
        if (count < 0) {
            if (errno == EINTR && retry_eintr) {
                continue;
            }
            return initialization_status::io_failure_no_mutation;
        }
        if (count == 0) {
            break;
        }
        long offset = 0;
        while (offset < count) {
            const auto * entry = reinterpret_cast<const linux_dirent64 *>(bytes.data() + offset);
            if (entry->d_reclen < offsetof(linux_dirent64, d_name) + 1 ||
                offset + entry->d_reclen > count) {
                return initialization_status::io_failure_no_mutation;
            }
            const std::size_t capacity = entry->d_reclen - offsetof(linux_dirent64, d_name);
            if (::strnlen(entry->d_name, capacity) == capacity) {
                return initialization_status::io_failure_no_mutation;
            }
            offset += entry->d_reclen;
            if (std::strcmp(entry->d_name, ".") == 0 ||
                std::strcmp(entry->d_name, "..") == 0) {
                continue;
            }
            if (++entries > 1 || saw_lock ||
                std::strcmp(entry->d_name, "primitive.lock") != 0) {
                return initialization_status::invalid_request_no_mutation;
            }
            saw_lock = true;
        }
    }
    return entries == 1 && saw_lock
               ? initialization_status::initialization_discard_required
               : initialization_status::invalid_request_no_mutation;
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

initialization_status mountinfo_read_only(std::uint64_t mount_id, bool & read_only,
                                          initialization_audit & output,
                                          bool retry_eintr = true) noexcept {
    std::array<char, mountinfo_capacity + 1> bytes {};
    ++output.root_fixture_syscall_count;
    int fd = ::open("/proc/self/mountinfo", O_RDONLY | O_CLOEXEC);
    if (fd < 0) {
        return initialization_status::io_failure_no_mutation;
    }
    std::size_t total = 0;
    for (;;) {
        ++output.root_fixture_syscall_count;
        const ssize_t count = ::read(fd, bytes.data() + total, mountinfo_capacity - total);
        if (count < 0) {
            if (errno == EINTR && retry_eintr) {
                continue;
            }
            ::close(fd);
            return initialization_status::io_failure_no_mutation;
        }
        if (count == 0) {
            break;
        }
        total += static_cast<std::size_t>(count);
        if (total == mountinfo_capacity) {
            char extra = 0;
            ++output.root_fixture_syscall_count;
            if (::read(fd, &extra, 1) != 0) {
                ::close(fd);
                return initialization_status::io_failure_no_mutation;
            }
            break;
        }
    }
    if (::close(fd) != 0) {
        return initialization_status::io_failure_no_mutation;
    }
    bytes[total] = '\0';
    const char * cursor = bytes.data();
    const char * finish = bytes.data() + total;
    while (cursor < finish) {
        const char * line_end = static_cast<const char *>(
            std::memchr(cursor, '\n', static_cast<std::size_t>(finish - cursor)));
        if (!line_end) {
            line_end = finish;
        }
        std::uint64_t parsed = 0;
        const char * p = cursor;
        while (p < line_end && *p >= '0' && *p <= '9') {
            if (parsed > (std::numeric_limits<std::uint64_t>::max() - (*p - '0')) / 10) {
                return initialization_status::io_failure_no_mutation;
            }
            parsed = parsed * 10 + static_cast<unsigned>(*p - '0');
            ++p;
        }
        if (p < line_end && *p == ' ' && parsed == mount_id) {
            const char * fields[6] {};
            std::size_t field_count = 0;
            const char * field = cursor;
            while (field < line_end && field_count < 6) {
                fields[field_count++] = field;
                const char * space = static_cast<const char *>(
                    std::memchr(field, ' ', static_cast<std::size_t>(line_end - field)));
                if (!space) {
                    break;
                }
                field = space + 1;
            }
            if (field_count < 6) {
                return initialization_status::io_failure_no_mutation;
            }
            const char * mount_options_end = static_cast<const char *>(
                std::memchr(fields[5], ' ', static_cast<std::size_t>(line_end - fields[5])));
            if (!mount_options_end) {
                return initialization_status::io_failure_no_mutation;
            }
            const char * separator = nullptr;
            for (const char * q = mount_options_end; q + 2 < line_end; ++q) {
                if (q[0] == ' ' && q[1] == '-' && q[2] == ' ') {
                    separator = q;
                    break;
                }
            }
            if (!separator) {
                return initialization_status::io_failure_no_mutation;
            }
            const char * super = separator + 3;
            for (int i = 0; i < 2; ++i) {
                super = static_cast<const char *>(
                    std::memchr(super, ' ', static_cast<std::size_t>(line_end - super)));
                if (!super) {
                    return initialization_status::io_failure_no_mutation;
                }
                ++super;
            }
            read_only = csv_has_ro(fields[5], mount_options_end) || csv_has_ro(super, line_end);
            return initialization_status::initialization_discard_required;
        }
        cursor = line_end < finish ? line_end + 1 : finish;
    }
    return initialization_status::invalid_request_no_mutation;
}

initialization_status reserve_and_read_only(
        int root_fd, const initialization_pinned_path_identity & root,
        initialization_audit & output, bool retry_eintr = true) noexcept {
    struct statvfs values {};
    ++output.root_fixture_syscall_count;
    if (::fstatvfs(root_fd, &values) != 0) {
        return initialization_status::io_failure_no_mutation;
    }
    if (values.f_frsize != 0 && values.f_bavail >
            std::numeric_limits<std::uint64_t>::max() / values.f_frsize) {
        return initialization_status::io_failure_no_mutation;
    }
    output.observed_filesystem_reserve =
        static_cast<std::uint64_t>(values.f_bavail) * values.f_frsize;
    bool mount_read_only = false;
    const auto result = mountinfo_read_only(
        root.mount_id, mount_read_only, output, retry_eintr);
    if (result != initialization_status::initialization_discard_required) {
        return result;
    }
    if ((values.f_flag & ST_RDONLY) != 0 || mount_read_only) {
        return initialization_status::unavailable_no_mutation;
    }
    output.filesystem_not_reported_read_only = true;
    return output.observed_filesystem_reserve >= initialization_required_filesystem_reserve
               ? initialization_status::initialization_discard_required
               : initialization_status::unavailable_no_mutation;
}

initialization_status inspect_fixture_lock(int fd, const initialization_request & input,
                                           initialization_audit & output) noexcept {
    struct stat value {};
    ++output.root_fixture_syscall_count;
    if (::fstat(fd, &value) != 0) {
        return initialization_status::io_failure_no_mutation;
    }
    if (!S_ISREG(value.st_mode) || static_cast<std::uint64_t>(value.st_uid) != input.fixture.owner_uid ||
        static_cast<std::uint32_t>(value.st_mode & 07777) != 0600 || value.st_nlink != 1 ||
        value.st_size != 0 || static_cast<std::uint64_t>(value.st_dev) != input.fixture_lock_device ||
        static_cast<std::uint64_t>(value.st_ino) != input.fixture_lock_inode) {
        return initialization_status::invalid_request_no_mutation;
    }
    struct statx extended {};
    ++output.root_fixture_syscall_count;
    if (::syscall(SYS_statx, fd, "", AT_EMPTY_PATH | AT_STATX_SYNC_AS_STAT,
                  STATX_BASIC_STATS | STATX_MNT_ID, &extended) != 0) {
        return errno == ENOSYS || errno == EINVAL
                   ? initialization_status::unsupported_no_mutation
                   : initialization_status::io_failure_no_mutation;
    }
    if ((extended.stx_mask & STATX_MNT_ID) == 0 || extended.stx_mnt_id != input.fixture.mount_id) {
        return initialization_status::invalid_request_no_mutation;
    }
    output.fixture_lock_identity_matched = true;
    return initialization_status::initialization_discard_required;
}

initialization_status inspect_writer_lock(int fd, const initialization_request & input,
                                          const struct stat & created,
                                          initialization_audit & output) noexcept {
    struct stat value {};
    ++output.root_fixture_syscall_count;
    if (::fstat(fd, &value) != 0) {
        return initialization_status::io_failure_no_mutation;
    }
    if (!S_ISREG(value.st_mode) || value.st_uid != input.candidate_root.owner_uid ||
        (value.st_mode & 07777) != 0600 || value.st_nlink != 1 || value.st_size != 0 ||
        value.st_dev != created.st_dev || value.st_ino != created.st_ino ||
        static_cast<std::uint64_t>(value.st_dev) != input.candidate_root.device) {
        return initialization_status::invalid_request_no_mutation;
    }
    struct statx extended {};
    ++output.root_fixture_syscall_count;
    if (::syscall(SYS_statx, fd, "", AT_EMPTY_PATH | AT_STATX_SYNC_AS_STAT,
                  STATX_BASIC_STATS | STATX_MNT_ID, &extended) != 0) {
        return errno == ENOSYS || errno == EINVAL
                   ? initialization_status::unsupported_no_mutation
                   : initialization_status::io_failure_no_mutation;
    }
    return (extended.stx_mask & STATX_MNT_ID) != 0 &&
                   extended.stx_mnt_id == input.candidate_root.mount_id
               ? initialization_status::initialization_discard_required
               : initialization_status::invalid_request_no_mutation;
}

std::uint64_t timespec_ns(const struct timespec & value) noexcept {
    return static_cast<std::uint64_t>(value.tv_sec) * 1000000000ULL +
           static_cast<std::uint64_t>(value.tv_nsec);
}

initialization_status acquire_ofd_lock(int fd, std::uint32_t & attempts) noexcept {
    struct timespec start {};
    if (::clock_gettime(CLOCK_MONOTONIC, &start) != 0) {
        return initialization_status::io_failure_no_mutation;
    }
    const std::uint64_t start_ns = timespec_ns(start);
    if (start_ns > std::numeric_limits<std::uint64_t>::max() - deadline_nanoseconds) {
        return initialization_status::io_failure_no_mutation;
    }
    const std::uint64_t deadline = start_ns + deadline_nanoseconds;
    struct flock lock {};
    lock.l_type = F_WRLCK;
    lock.l_whence = SEEK_SET;
    lock.l_start = 0;
    lock.l_len = 0;
    bool first = true;
    for (;;) {
        if (!first) {
            struct timespec before {};
            if (::clock_gettime(CLOCK_MONOTONIC, &before) != 0) {
                return initialization_status::io_failure_no_mutation;
            }
            if (timespec_ns(before) >= deadline) {
                return initialization_status::busy_no_mutation;
            }
        }
        first = false;
        ++attempts;
        if (::fcntl(fd, F_OFD_SETLK, &lock) == 0) {
            return initialization_status::initialization_discard_required;
        }
        const int error = errno;
        if (error == EINTR) {
            continue;
        }
        if (error == EINVAL || error == ENOSYS) {
            return initialization_status::unsupported_no_mutation;
        }
        if (error != EAGAIN && error != EACCES) {
            return initialization_status::io_failure_no_mutation;
        }
        struct timespec now {};
        if (::clock_gettime(CLOCK_MONOTONIC, &now) != 0) {
            return initialization_status::io_failure_no_mutation;
        }
        const std::uint64_t now_ns = timespec_ns(now);
        if (now_ns >= deadline) {
            return initialization_status::busy_no_mutation;
        }
        const std::uint64_t sleep_ns =
            (deadline - now_ns) < retry_nanoseconds ? deadline - now_ns : retry_nanoseconds;
        struct timespec wake {
            static_cast<time_t>((now_ns + sleep_ns) / 1000000000ULL),
            static_cast<long>((now_ns + sleep_ns) % 1000000000ULL),
        };
        int sleep_result;
        do {
            sleep_result = ::clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, &wake, nullptr);
        } while (sleep_result == EINTR);
        if (sleep_result != 0) {
            return initialization_status::io_failure_no_mutation;
        }
    }
}

initialization_status compare_open_descriptions(int held, int current,
                                                initialization_audit & output) noexcept {
    struct stat held_stat {};
    struct stat current_stat {};
    ++output.root_fixture_syscall_count;
    const bool held_ok = ::fstat(held, &held_stat) == 0;
    ++output.root_fixture_syscall_count;
    const bool current_ok = ::fstat(current, &current_stat) == 0;
    if (!held_ok || !current_ok) {
        return initialization_status::io_failure_no_mutation;
    }
    return held_stat.st_dev == current_stat.st_dev && held_stat.st_ino == current_stat.st_ino
               ? initialization_status::initialization_discard_required
               : initialization_status::invalid_request_no_mutation;
}

initialization_status revalidate_parent(int held,
                                        const initialization_pinned_path_identity & expected,
                                        initialization_audit & output) noexcept {
    ++output.root_fixture_syscall_count;
    int reopened = ::open(expected.canonical_path,
                          O_RDONLY | O_DIRECTORY | O_CLOEXEC | O_NOFOLLOW);
    if (reopened < 0) {
        const auto mapped = map_error(errno);
        return mapped == initialization_status::unsupported_no_mutation
                   ? mapped : initialization_status::invalid_request_no_mutation;
    }
    auto result = compare_open_descriptions(held, reopened, output);
    if (result == initialization_status::initialization_discard_required) {
        result = inspect_directory(reopened, expected, expected.mode, output);
    }
    if (::close(reopened) != 0 &&
        result == initialization_status::initialization_discard_required) {
        result = initialization_status::io_failure_no_mutation;
    }
    return result;
}

initialization_status reopen_directory(
        int parent_fd, const initialization_pinned_path_identity & parent,
        const initialization_pinned_path_identity & expected, int held,
        int & reopened, initialization_audit & output) noexcept {
    initialization_status error = initialization_status::io_failure_no_mutation;
    reopened = open_contained(parent_fd, child_name(parent, expected),
                              O_RDONLY | O_DIRECTORY | O_CLOEXEC | O_NOFOLLOW,
                              0, output, error);
    if (reopened < 0) {
        return error == initialization_status::unsupported_no_mutation
                   ? error : initialization_status::invalid_request_no_mutation;
    }
    auto result = compare_open_descriptions(held, reopened, output);
    if (result == initialization_status::initialization_discard_required) {
        result = inspect_directory(reopened, expected, 0700, output);
    }
    return result;
}

initialization_status reopen_created_directory(
        int root_fd, const char * name, int held_root_fd,
        const initialization_request & input,
        created_directory_identity & identity,
        bool require_empty,
        initialization_audit & output,
        bool retry_eintr = true) noexcept {
    initialization_status result = initialization_status::io_failure_no_mutation;
    int reopened = open_contained(root_fd, name,
                                  O_RDONLY | O_DIRECTORY | O_CLOEXEC | O_NOFOLLOW,
                                  0, output, result);
    if (reopened < 0) {
        return result;
    }
    result = compare_open_descriptions(root_fd, held_root_fd, output);
    if (result == initialization_status::initialization_discard_required) {
        result = inspect_created_directory(reopened, name, input,
                                           identity, false, true, output);
    }
    if (result == initialization_status::initialization_discard_required && require_empty) {
        result = inspect_empty_directory(reopened, output, retry_eintr);
    }
    if (!close_fd(reopened) &&
        result == initialization_status::initialization_discard_required) {
        result = initialization_status::io_failure_no_mutation;
    }
    return result;
}

initialization_status revalidate_accumulated_prefix(
        int parent_fd, int root_fd, int fixture_fd,
        int fixture_lock_fd, int writer_fd,
        const initialization_request & input,
        const struct stat & writer_identity,
        unsigned expected_directories,
        created_directory_identity & envelopes_identity,
        created_directory_identity & attempts_identity,
        created_directory_identity & staging_identity,
        bool expect_marker, bool allow_nonempty_staging, bool final_revalidation,
        initialization_audit & output,
        bool retry_non_io_eintr = true) noexcept {
    auto result = revalidate_parent(parent_fd, input.parent, output);
    int reopened_root = -1;
    if (result == initialization_status::initialization_discard_required) {
        result = reopen_directory(parent_fd, input.parent, input.candidate_root,
                                  root_fd, reopened_root, output);
    }
    if (result == initialization_status::initialization_discard_required) {
        result = reserve_and_read_only(
            reopened_root, input.candidate_root, output, retry_non_io_eintr);
    }
    if (result == initialization_status::initialization_discard_required) {
        result = inspect_accumulated_prefix_layout(
            reopened_root, expected_directories, expect_marker, output,
            retry_non_io_eintr);
    }
    if (result == initialization_status::initialization_discard_required) {
        initialization_status reopen_error = initialization_status::io_failure_no_mutation;
        int reopened_writer = open_contained(reopened_root, "writer.lock",
                                             O_RDWR | O_CLOEXEC | O_NOFOLLOW,
                                             0, output, reopen_error);
        if (reopened_writer < 0) {
            result = reopen_error;
        } else {
            result = compare_open_descriptions(writer_fd, reopened_writer, output);
            if (result == initialization_status::initialization_discard_required) {
                result = inspect_writer_lock(reopened_writer, input,
                                             writer_identity, output);
            }
            if (::close(reopened_writer) != 0 &&
                result == initialization_status::initialization_discard_required) {
                result = initialization_status::io_failure_no_mutation;
            }
        }
    }
    if (result == initialization_status::initialization_discard_required &&
        expected_directories >= 1) {
        result = reopen_created_directory(reopened_root, "envelopes", root_fd,
                                          input, envelopes_identity, true, output,
                                          retry_non_io_eintr);
    }
    if (result == initialization_status::initialization_discard_required &&
        expected_directories >= 2) {
        result = reopen_created_directory(reopened_root, "attempts", root_fd,
                                          input, attempts_identity, true, output,
                                          retry_non_io_eintr);
    }
    if (result == initialization_status::initialization_discard_required &&
        expected_directories >= 3) {
        result = reopen_created_directory(reopened_root, "staging", root_fd,
                                          input, staging_identity,
                                          !allow_nonempty_staging, output,
                                          retry_non_io_eintr);
    }
    if (!close_fd(reopened_root) &&
        result == initialization_status::initialization_discard_required) {
        result = initialization_status::io_failure_no_mutation;
    }
    int reopened_fixture = -1;
    if (result == initialization_status::initialization_discard_required) {
        result = reopen_directory(parent_fd, input.parent, input.fixture,
                                  fixture_fd, reopened_fixture, output);
    }
    if (result == initialization_status::initialization_discard_required) {
        result = inspect_fixture_layout(
            reopened_fixture, output, retry_non_io_eintr);
    }
    if (result == initialization_status::initialization_discard_required) {
        initialization_status reopen_error = initialization_status::io_failure_no_mutation;
        int reopened_fixture_lock = open_contained(
            reopened_fixture, "primitive.lock", O_RDWR | O_CLOEXEC | O_NOFOLLOW,
            0, output, reopen_error);
        if (reopened_fixture_lock < 0) {
            result = reopen_error;
        } else {
            result = compare_open_descriptions(
                fixture_lock_fd, reopened_fixture_lock, output);
            if (result == initialization_status::initialization_discard_required) {
                result = inspect_fixture_lock(reopened_fixture_lock, input, output);
            }
            if (::close(reopened_fixture_lock) != 0 &&
                result == initialization_status::initialization_discard_required) {
                result = initialization_status::io_failure_no_mutation;
            }
        }
    }
    if (!close_fd(reopened_fixture) &&
        result == initialization_status::initialization_discard_required) {
        result = initialization_status::io_failure_no_mutation;
    }
    if (result == initialization_status::initialization_discard_required &&
        final_revalidation) {
        output.envelopes_directory_final_revalidation_matched = true;
        output.attempts_directory_final_revalidation_matched = true;
        output.staging_directory_final_revalidation_matched = true;
    }
    return result;
}

initialization_status revalidate_predecessor_final_layout(
        int parent_fd, int root_fd, int fixture_fd, int fixture_lock_fd,
        int writer_fd, const initialization_request & input,
        const struct stat & writer_identity,
        created_directory_identity & envelopes_identity,
        created_directory_identity & attempts_identity,
        created_directory_identity & staging_identity,
        const char * envelope_basename,
        initialization_audit & output) noexcept {
    auto result = revalidate_parent(parent_fd, input.parent, output);
    int reopened_root = -1;
    if (result == initialization_status::initialization_discard_required) {
        result = reopen_directory(parent_fd, input.parent, input.candidate_root,
                                  root_fd, reopened_root, output);
    }
    if (result == initialization_status::initialization_discard_required) {
        result = reserve_and_read_only(
            reopened_root, input.candidate_root, output, false);
    }
    if (result == initialization_status::initialization_discard_required) {
        result = inspect_accumulated_prefix_layout(
            reopened_root, 3, true, output, false);
    }
    if (result == initialization_status::initialization_discard_required) {
        initialization_status reopen_error = initialization_status::io_failure_no_mutation;
        int reopened_writer = open_contained(
            reopened_root, "writer.lock", O_RDWR | O_CLOEXEC | O_NOFOLLOW,
            0, output, reopen_error);
        if (reopened_writer < 0) {
            result = reopen_error;
        } else {
            result = compare_open_descriptions(writer_fd, reopened_writer, output);
            if (result == initialization_status::initialization_discard_required) {
                result = inspect_writer_lock(reopened_writer, input,
                                             writer_identity, output);
            }
            if (::close(reopened_writer) != 0 &&
                result == initialization_status::initialization_discard_required) {
                result = initialization_status::io_failure_no_mutation;
            }
        }
    }
    auto validate_child = [&](const char * name,
                              created_directory_identity & identity,
                              bool expected_envelope) noexcept {
        if (result != initialization_status::initialization_discard_required) {
            return;
        }
        initialization_status open_error = initialization_status::io_failure_no_mutation;
        int reopened = open_contained(
            reopened_root, name,
            O_RDONLY | O_DIRECTORY | O_CLOEXEC | O_NOFOLLOW,
            0, output, open_error);
        if (reopened < 0) {
            result = open_error;
            return;
        }
        result = inspect_created_directory(reopened, name, input,
                                           identity, false, true, output);
        if (result == initialization_status::initialization_discard_required) {
            result = expected_envelope
                         ? inspect_single_directory_entry(
                               reopened, envelope_basename, output)
                         : inspect_empty_directory(reopened, output, false);
        }
        if (!close_fd(reopened) &&
            result == initialization_status::initialization_discard_required) {
            result = initialization_status::io_failure_no_mutation;
        }
    };
    validate_child("envelopes", envelopes_identity, true);
    validate_child("attempts", attempts_identity, false);
    validate_child("staging", staging_identity, false);
    if (!close_fd(reopened_root) &&
        result == initialization_status::initialization_discard_required) {
        result = initialization_status::io_failure_no_mutation;
    }
    int reopened_fixture = -1;
    if (result == initialization_status::initialization_discard_required) {
        result = reopen_directory(parent_fd, input.parent, input.fixture,
                                  fixture_fd, reopened_fixture, output);
    }
    if (result == initialization_status::initialization_discard_required) {
        result = inspect_fixture_layout(reopened_fixture, output, false);
    }
    if (result == initialization_status::initialization_discard_required) {
        initialization_status open_error = initialization_status::io_failure_no_mutation;
        int reopened_lock = open_contained(
            reopened_fixture, "primitive.lock", O_RDWR | O_CLOEXEC | O_NOFOLLOW,
            0, output, open_error);
        if (reopened_lock < 0) {
            result = open_error;
        } else {
            result = compare_open_descriptions(
                fixture_lock_fd, reopened_lock, output);
            if (result == initialization_status::initialization_discard_required) {
                result = inspect_fixture_lock(reopened_lock, input, output);
            }
            if (::close(reopened_lock) != 0 &&
                result == initialization_status::initialization_discard_required) {
                result = initialization_status::io_failure_no_mutation;
            }
        }
    }
    if (!close_fd(reopened_fixture) &&
        result == initialization_status::initialization_discard_required) {
        result = initialization_status::io_failure_no_mutation;
    }
    return result;
}

initialization_status generate_identities(initialization_audit & output) noexcept {
    std::array<std::uint8_t, 32> root_id {};
    std::array<std::uint8_t, 16> store_uuid {};
    auto fill = [&](std::uint8_t * bytes, std::size_t size) noexcept {
        std::size_t total = 0;
        while (total < size) {
            ++output.root_fixture_syscall_count;
            const ssize_t count = ::syscall(SYS_getrandom, bytes + total, size - total, 0);
            if (count < 0) {
                if (errno == EINTR) {
                    continue;
                }
                return errno == ENOSYS
                    ? initialization_status::unsupported_no_mutation
                    : initialization_status::io_failure_no_mutation;
            }
            if (count == 0) {
                return initialization_status::io_failure_no_mutation;
            }
            total += static_cast<std::size_t>(count);
        }
        return initialization_status::initialization_discard_required;
    };
    auto fill_result = fill(root_id.data(), root_id.size());
    if (fill_result == initialization_status::initialization_discard_required) {
        fill_result = fill(store_uuid.data(), store_uuid.size());
    }
    const bool root_nonzero =
        fill_result == initialization_status::initialization_discard_required &&
        nonzero(root_id.data(), root_id.size());
    const bool store_nonzero =
        fill_result == initialization_status::initialization_discard_required &&
        nonzero(store_uuid.data(), store_uuid.size());
    if (root_nonzero && store_nonzero) {
        output.generated_root_id = root_id;
        output.generated_store_uuid = store_uuid;
        output.generated_root_id_nonzero = true;
        output.generated_store_uuid_nonzero = true;
    }
    wipe(root_id.data(), root_id.size());
    wipe(store_uuid.data(), store_uuid.size());
    output.generated_identity_scratch_wiped =
        !nonzero(root_id.data(), root_id.size()) &&
        !nonzero(store_uuid.data(), store_uuid.size());
    if (fill_result != initialization_status::initialization_discard_required) {
        return fill_result;
    }
    return root_nonzero && store_nonzero && output.generated_identity_scratch_wiped
        ? initialization_status::initialization_discard_required
        : initialization_status::io_failure_no_mutation;
}

initialization_status sealed_status(const sealed_input_audit & input) noexcept {
    switch (input.result) {
        case sealed_input_status::predecessor_authenticated_pins_matched_no_root_access:
            return initialization_status::initialization_discard_required;
        case sealed_input_status::unsupported_no_mutation:
            return initialization_status::unsupported_no_mutation;
        case sealed_input_status::unavailable_no_mutation:
            return initialization_status::unavailable_no_mutation;
        case sealed_input_status::io_failure_no_mutation:
            return initialization_status::io_failure_no_mutation;
        case sealed_input_status::transport_validated_no_root_access:
        case sealed_input_status::invalid_request_no_mutation:
            return initialization_status::invalid_request_no_mutation;
    }
    return initialization_status::invalid_request_no_mutation;
}

bool unlock_and_close(int & fd, bool acquired, bool retry_eintr,
                      bool & released) noexcept {
    if (fd < 0) {
        return true;
    }
    bool ok = true;
    if (acquired) {
        struct flock unlock {};
        unlock.l_type = F_UNLCK;
        unlock.l_whence = SEEK_SET;
        unlock.l_start = 0;
        unlock.l_len = 0;
        int value = ::fcntl(fd, F_OFD_SETLK, &unlock);
        while (retry_eintr && value != 0 && errno == EINTR) {
            value = ::fcntl(fd, F_OFD_SETLK, &unlock);
        }
        released = value == 0;
        ok = released;
    }
    return close_fd(fd) && ok;
}

void clear_pre_latch_positive_audit(initialization_audit & output) noexcept {
    output.parent_identity_matched = false;
    output.candidate_root_identity_matched = false;
    output.fixture_identity_matched = false;
    output.fixture_lock_identity_matched = false;
    output.fixture_lock_acquired = false;
    output.empty_root_final_revalidation_matched = false;
    output.generated_root_id_nonzero = false;
    output.generated_store_uuid_nonzero = false;
    wipe(output.generated_root_id.data(), output.generated_root_id.size());
    wipe(output.generated_store_uuid.data(), output.generated_store_uuid.size());
    output.writer_lock_created_no_replace = false;
    output.writer_lock_mode_revalidated = false;
    output.writer_lock_synchronized = false;
    output.writer_lock_ofd_acquired = false;
    output.writer_lock_root_sole_entry = false;
    output.lock_anchor_qualified = false;
    output.envelopes_directory_created_no_replace = false;
    output.envelopes_directory_validated = false;
    output.envelopes_directory_synchronized = false;
    output.attempts_directory_created_no_replace = false;
    output.attempts_directory_validated = false;
    output.attempts_directory_synchronized = false;
    output.staging_directory_created_no_replace = false;
    output.staging_directory_validated = false;
    output.staging_directory_synchronized = false;
    output.envelopes_directory_final_revalidation_matched = false;
    output.attempts_directory_final_revalidation_matched = false;
    output.staging_directory_final_revalidation_matched = false;
    output.root_directory_synchronized = false;
    output.directory_prefix_qualified = false;
    output.path_policy_commitment_computed = false;
    output.initializing_marker_admitted = false;
    output.initializing_marker_encoded = false;
    output.initializing_marker_self_verified = false;
    output.initializing_marker_temp_created_no_replace = false;
    output.initializing_marker_mode_revalidated = false;
    output.initializing_marker_write_completed = false;
    output.initializing_marker_readback_exact = false;
    output.initializing_marker_readback_authenticated = false;
    output.initializing_marker_synchronized = false;
    output.initializing_marker_readonly_reopen_matched = false;
    output.initializing_marker_pre_publication_revalidation_matched = false;
    output.initializing_marker_published_no_replace = false;
    output.root_synchronized_after_marker = false;
    output.staging_synchronized_after_marker = false;
    output.initializing_marker_final_revalidation_matched = false;
    output.staging_empty_after_marker_publication = false;
    output.initializing_marker_qualified = false;
    wipe(output.path_policy_commitment.data(), output.path_policy_commitment.size());
    wipe(output.initializing_marker_content_digest.data(),
         output.initializing_marker_content_digest.size());
    output.initializing_marker_device = 0;
    output.initializing_marker_inode = 0;
    output.initializing_marker_mount_id = 0;
    output.initializing_marker_size = 0;
    output.initializing_marker_phase_ordinal = 0;
    output.predecessor_envelope_authenticated = false;
    output.predecessor_envelope_digest_name_computed = false;
    output.predecessor_envelope_path_admitted = false;
    output.predecessor_envelope_logical_bound_admitted = false;
    output.initializing_marker_write_descriptor_closed = false;
    output.predecessor_envelope_pre_mutation_revalidation_matched = false;
    output.predecessor_envelope_temp_created_no_replace = false;
    output.predecessor_envelope_mode_revalidated = false;
    output.predecessor_envelope_write_completed = false;
    output.predecessor_envelope_readback_exact = false;
    output.predecessor_envelope_readback_authenticated = false;
    output.predecessor_envelope_synchronized = false;
    output.predecessor_envelope_readonly_reopen_matched = false;
    output.predecessor_envelope_pre_publication_revalidation_matched = false;
    output.predecessor_envelope_published_no_replace = false;
    output.envelopes_synchronized_after_predecessor = false;
    output.staging_synchronized_after_predecessor = false;
    output.predecessor_envelope_final_revalidation_matched = false;
    output.initializing_marker_unchanged_after_predecessor = false;
    output.predecessor_envelope_final_layout_matched = false;
    output.predecessor_envelope_qualified = false;
    wipe(output.predecessor_envelope_digest.data(),
         output.predecessor_envelope_digest.size());
    wipe(output.predecessor_envelope_basename.data(),
         output.predecessor_envelope_basename.size());
    output.predecessor_envelope_device = 0;
    output.predecessor_envelope_inode = 0;
    output.predecessor_envelope_mount_id = 0;
    output.predecessor_envelope_size = 0;
    output.predecessor_envelope_phase_ordinal = 0;
    output.filesystem_not_reported_read_only = false;
    output.observed_filesystem_reserve = 0;
}

initialization_audit finish(initialization_audit output, initialization_status result,
                            int & writer_fd, int & fixture_lock_fd, int & fixture_fd,
                            int & root_fd, int & parent_fd, bool guard_owned,
                            authenticated_input_session & input_session) noexcept {
    bool close_ok = close_fd(fixture_fd);
    close_ok = close_fd(root_fd) && close_ok;
    close_ok = close_fd(parent_fd) && close_ok;
    if (input_session.authenticated) {
        close_ok = cleanup_authenticated_input_storage(input_session,
                                                       output.sealed_inputs) && close_ok;
    }
    const bool retry_cleanup_unlock_eintr =
        !output.initializing_marker_write_descriptor_closed;
    close_ok = unlock_and_close(writer_fd, output.writer_lock_ofd_acquired,
                                retry_cleanup_unlock_eintr,
                                output.writer_lock_released) && close_ok;
    close_ok = unlock_and_close(fixture_lock_fd, output.fixture_lock_acquired,
                                retry_cleanup_unlock_eintr,
                                output.fixture_lock_released) && close_ok;
    if (guard_owned) {
        root_guard_device.store(0, std::memory_order_relaxed);
        root_guard_inode.store(0, std::memory_order_relaxed);
        root_guard_state.store(0, std::memory_order_release);
        output.root_guard_released = true;
    }
    if (!restore_authenticated_input_signal_mask(input_session, output.sealed_inputs)) {
        close_ok = false;
    }
    if (!close_ok) {
        output.lock_anchor_qualified = false;
        output.directory_prefix_qualified = false;
        output.initializing_marker_qualified = false;
        output.predecessor_envelope_qualified = false;
    }
    if (!close_ok && !output.discard_required_latched &&
        result != initialization_status::preexisting_root_discard_required) {
        result = initialization_status::io_failure_no_mutation;
    }
    if (!output.discard_required_latched) {
        clear_pre_latch_positive_audit(output);
    }
    if (output.discard_required_latched) {
        result = initialization_status::initialization_discard_required;
    }
    output.result = result;
    return output;
}

} // namespace

namespace {

initialization_audit initialize_once(
        const initialization_request & input,
        initialization_extent extent) noexcept {
    initialization_audit output {};
    int parent_fd = -1;
    int root_fd = -1;
    int fixture_fd = -1;
    int fixture_lock_fd = -1;
    int writer_fd = -1;
    bool guard_owned = false;

    authenticated_input_session input_session {};
    if (!request_valid(input)) {
        return finish(output, initialization_status::invalid_request_no_mutation,
                      writer_fd, fixture_lock_fd, fixture_fd, root_fd, parent_fd,
                      guard_owned, input_session);
    }

    for (;;) {
        std::uint8_t expected = 0;
        if (root_guard_state.compare_exchange_strong(expected, 1,
                                                     std::memory_order_acq_rel,
                                                     std::memory_order_acquire)) {
            root_guard_device.store(input.candidate_root.device, std::memory_order_relaxed);
            root_guard_inode.store(input.candidate_root.inode, std::memory_order_relaxed);
            root_guard_state.store(2, std::memory_order_release);
            guard_owned = true;
            output.root_guard_acquired = true;
            break;
        }
        if (expected == 1) {
            continue;
        }
        const auto device = root_guard_device.load(std::memory_order_relaxed);
        const auto inode = root_guard_inode.load(std::memory_order_relaxed);
        if (root_guard_state.load(std::memory_order_acquire) != 2) {
            continue;
        }
        const auto result =
            device == input.candidate_root.device && inode == input.candidate_root.inode
                ? initialization_status::busy_no_mutation
                : initialization_status::invalid_request_no_mutation;
        return finish(output, result, writer_fd, fixture_lock_fd, fixture_fd,
                      root_fd, parent_fd, guard_owned, input_session);
    }

    const auto authenticated_status = authenticate_sealed_inputs_for_session(
        input.sealed_inputs, input_session, output.sealed_inputs);
    output.sealed_inputs_preceded_root_access =
        output.sealed_inputs.root_or_fixture_syscall_count == 0 &&
        !output.sealed_inputs.root_or_fixture_accessed &&
        output.root_fixture_syscall_count == 0;
    auto result = sealed_status(output.sealed_inputs);
    if (authenticated_status !=
            sealed_input_status::predecessor_authenticated_pins_matched_no_root_access ||
        result != initialization_status::initialization_discard_required ||
        !output.sealed_inputs_preceded_root_access || !input_session.authenticated ||
        !output.sealed_inputs.descriptors_closed) {
        return finish(output, result, writer_fd, fixture_lock_fd, fixture_fd,
                      root_fd, parent_fd, guard_owned, input_session);
    }

    ++output.root_fixture_syscall_count;
    parent_fd = ::open(input.parent.canonical_path,
                       O_RDONLY | O_DIRECTORY | O_CLOEXEC | O_NOFOLLOW);
    if (parent_fd < 0) {
        return finish(output, map_error(errno), writer_fd, fixture_lock_fd,
                      fixture_fd, root_fd, parent_fd, guard_owned, input_session);
    }
    result = inspect_directory(parent_fd, input.parent, input.parent.mode, output);
    if (result != initialization_status::initialization_discard_required) {
        return finish(output, result, writer_fd, fixture_lock_fd, fixture_fd,
                      root_fd, parent_fd, guard_owned, input_session);
    }
    output.parent_identity_matched = true;

    root_fd = open_contained(parent_fd, child_name(input.parent, input.candidate_root),
                             O_RDONLY | O_DIRECTORY | O_CLOEXEC | O_NOFOLLOW,
                             0, output, result);
    if (root_fd < 0) {
        return finish(output, result, writer_fd, fixture_lock_fd, fixture_fd,
                      root_fd, parent_fd, guard_owned, input_session);
    }
    result = inspect_directory(root_fd, input.candidate_root, 0700, output);
    if (result != initialization_status::initialization_discard_required) {
        return finish(output, result, writer_fd, fixture_lock_fd, fixture_fd,
                      root_fd, parent_fd, guard_owned, input_session);
    }
    output.candidate_root_identity_matched = true;
    const auto initial_layout = inspect_root_layout(root_fd, false, output);
    if (initial_layout == root_layout::io_failure) {
        return finish(output, initialization_status::io_failure_no_mutation,
                      writer_fd, fixture_lock_fd, fixture_fd, root_fd, parent_fd,
                      guard_owned, input_session);
    }
    if (initial_layout != root_layout::empty) {
        return finish(output, initialization_status::preexisting_root_discard_required,
                      writer_fd, fixture_lock_fd, fixture_fd, root_fd, parent_fd,
                      guard_owned, input_session);
    }
    result = reserve_and_read_only(root_fd, input.candidate_root, output);
    if (result != initialization_status::initialization_discard_required) {
        return finish(output, result, writer_fd, fixture_lock_fd, fixture_fd,
                      root_fd, parent_fd, guard_owned, input_session);
    }

    fixture_fd = open_contained(parent_fd, child_name(input.parent, input.fixture),
                                O_RDONLY | O_DIRECTORY | O_CLOEXEC | O_NOFOLLOW,
                                0, output, result);
    if (fixture_fd < 0) {
        return finish(output, result, writer_fd, fixture_lock_fd, fixture_fd,
                      root_fd, parent_fd, guard_owned, input_session);
    }
    result = inspect_directory(fixture_fd, input.fixture, 0700, output);
    if (result == initialization_status::initialization_discard_required) {
        output.fixture_identity_matched = true;
        result = inspect_fixture_layout(fixture_fd, output);
    }
    if (result != initialization_status::initialization_discard_required) {
        return finish(output, result, writer_fd, fixture_lock_fd, fixture_fd,
                      root_fd, parent_fd, guard_owned, input_session);
    }
    fixture_lock_fd = open_contained(fixture_fd, "primitive.lock",
                                     O_RDWR | O_CLOEXEC | O_NOFOLLOW, 0, output, result);
    if (fixture_lock_fd < 0) {
        return finish(output, result, writer_fd, fixture_lock_fd, fixture_fd,
                      root_fd, parent_fd, guard_owned, input_session);
    }
    result = inspect_fixture_lock(fixture_lock_fd, input, output);
    if (result == initialization_status::initialization_discard_required) {
        result = acquire_ofd_lock(fixture_lock_fd, output.fixture_ofd_attempt_count);
    }
    if (result != initialization_status::initialization_discard_required) {
        return finish(output, result, writer_fd, fixture_lock_fd, fixture_fd,
                      root_fd, parent_fd, guard_owned, input_session);
    }
    output.fixture_lock_acquired = true;

    result = revalidate_parent(parent_fd, input.parent, output);
    int reopened_root = -1;
    if (result == initialization_status::initialization_discard_required) {
        result = reopen_directory(parent_fd, input.parent, input.candidate_root,
                                  root_fd, reopened_root, output);
    }
    if (result == initialization_status::initialization_discard_required) {
        const auto layout = inspect_root_layout(reopened_root, false, output);
        if (layout == root_layout::io_failure) {
            result = initialization_status::io_failure_no_mutation;
        } else if (layout != root_layout::empty) {
            result = initialization_status::preexisting_root_discard_required;
        }
    }
    if (result == initialization_status::initialization_discard_required) {
        result = reserve_and_read_only(reopened_root, input.candidate_root, output);
    }
    if (!close_fd(reopened_root) &&
        result == initialization_status::initialization_discard_required) {
        result = initialization_status::io_failure_no_mutation;
    }
    int reopened_fixture = -1;
    if (result == initialization_status::initialization_discard_required) {
        result = reopen_directory(parent_fd, input.parent, input.fixture,
                                  fixture_fd, reopened_fixture, output);
    }
    if (result == initialization_status::initialization_discard_required) {
        result = inspect_fixture_layout(reopened_fixture, output);
    }
    if (!close_fd(reopened_fixture) &&
        result == initialization_status::initialization_discard_required) {
        result = initialization_status::io_failure_no_mutation;
    }
    if (result == initialization_status::initialization_discard_required) {
        initialization_status reopen_error = initialization_status::io_failure_no_mutation;
        int reopened_lock = open_contained(fixture_fd, "primitive.lock",
                                           O_RDWR | O_CLOEXEC | O_NOFOLLOW,
                                           0, output, reopen_error);
        if (reopened_lock < 0) {
            result = reopen_error;
        } else {
            result = compare_open_descriptions(fixture_lock_fd, reopened_lock, output);
            if (result == initialization_status::initialization_discard_required) {
                result = inspect_fixture_lock(reopened_lock, input, output);
            }
            if (::close(reopened_lock) != 0 &&
                result == initialization_status::initialization_discard_required) {
                result = initialization_status::io_failure_no_mutation;
            }
        }
    }
    if (result != initialization_status::initialization_discard_required) {
        return finish(output, result, writer_fd, fixture_lock_fd, fixture_fd,
                      root_fd, parent_fd, guard_owned, input_session);
    }
    output.empty_root_final_revalidation_matched = true;

    result = generate_identities(output);
    if (result != initialization_status::initialization_discard_required) {
        return finish(output, result, writer_fd, fixture_lock_fd, fixture_fd,
                      root_fd, parent_fd, guard_owned, input_session);
    }

    output.discard_required_latched = true;
    writer_fd = open_contained(root_fd, "writer.lock",
                               O_CREAT | O_EXCL | O_RDWR | O_CLOEXEC | O_NOFOLLOW,
                               0600, output, result);
    if (writer_fd < 0) {
        return finish(output, result, writer_fd, fixture_lock_fd, fixture_fd,
                      root_fd, parent_fd, guard_owned, input_session);
    }
    output.writer_lock_created_no_replace = true;
    ++output.root_fixture_syscall_count;
    if (::fchmod(writer_fd, 0600) != 0) {
        return finish(output, initialization_status::io_failure_no_mutation,
                      writer_fd, fixture_lock_fd, fixture_fd, root_fd, parent_fd,
                      guard_owned, input_session);
    }
    struct stat writer_identity {};
    ++output.root_fixture_syscall_count;
    if (::fstat(writer_fd, &writer_identity) != 0) {
        return finish(output, initialization_status::io_failure_no_mutation,
                      writer_fd, fixture_lock_fd, fixture_fd, root_fd, parent_fd,
                      guard_owned, input_session);
    }
    result = inspect_writer_lock(writer_fd, input, writer_identity, output);
    if (result != initialization_status::initialization_discard_required) {
        return finish(output, result, writer_fd, fixture_lock_fd, fixture_fd,
                      root_fd, parent_fd, guard_owned, input_session);
    }
    output.writer_lock_mode_revalidated = true;
    ++output.root_fixture_syscall_count;
    if (::fsync(writer_fd) != 0) {
        return finish(output, initialization_status::io_failure_no_mutation,
                      writer_fd, fixture_lock_fd, fixture_fd, root_fd, parent_fd,
                      guard_owned, input_session);
    }
    output.writer_lock_synchronized = true;
    result = acquire_ofd_lock(writer_fd, output.writer_ofd_attempt_count);
    if (result != initialization_status::initialization_discard_required) {
        return finish(output, result, writer_fd, fixture_lock_fd, fixture_fd,
                      root_fd, parent_fd, guard_owned, input_session);
    }
    output.writer_lock_ofd_acquired = true;

    result = revalidate_parent(parent_fd, input.parent, output);
    reopened_root = -1;
    if (result == initialization_status::initialization_discard_required) {
        result = reopen_directory(parent_fd, input.parent, input.candidate_root,
                                  root_fd, reopened_root, output);
    }
    if (result == initialization_status::initialization_discard_required) {
        result = reserve_and_read_only(reopened_root, input.candidate_root, output);
    }
    if (result == initialization_status::initialization_discard_required) {
        const auto layout = inspect_root_layout(reopened_root, true, output);
        result = layout == root_layout::writer_lock_only
                     ? initialization_status::initialization_discard_required
                     : (layout == root_layout::io_failure
                            ? initialization_status::io_failure_no_mutation
                            : initialization_status::invalid_request_no_mutation);
    }
    if (result == initialization_status::initialization_discard_required) {
        initialization_status reopen_error = initialization_status::io_failure_no_mutation;
        int reopened_writer = open_contained(reopened_root, "writer.lock",
                                             O_RDWR | O_CLOEXEC | O_NOFOLLOW,
                                             0, output, reopen_error);
        if (reopened_writer < 0) {
            result = reopen_error;
        } else {
            result = compare_open_descriptions(writer_fd, reopened_writer, output);
            if (result == initialization_status::initialization_discard_required) {
                result = inspect_writer_lock(reopened_writer, input, writer_identity, output);
            }
            if (::close(reopened_writer) != 0 &&
                result == initialization_status::initialization_discard_required) {
                result = initialization_status::io_failure_no_mutation;
            }
        }
    }
    if (!close_fd(reopened_root) &&
        result == initialization_status::initialization_discard_required) {
        result = initialization_status::io_failure_no_mutation;
    }
    reopened_fixture = -1;
    if (result == initialization_status::initialization_discard_required) {
        result = reopen_directory(parent_fd, input.parent, input.fixture,
                                  fixture_fd, reopened_fixture, output);
    }
    if (result == initialization_status::initialization_discard_required) {
        result = inspect_fixture_layout(reopened_fixture, output);
    }
    if (result == initialization_status::initialization_discard_required) {
        initialization_status reopen_error = initialization_status::io_failure_no_mutation;
        int reopened_fixture_lock = open_contained(
            reopened_fixture, "primitive.lock", O_RDWR | O_CLOEXEC | O_NOFOLLOW,
            0, output, reopen_error);
        if (reopened_fixture_lock < 0) {
            result = reopen_error;
        } else {
            result = compare_open_descriptions(
                fixture_lock_fd, reopened_fixture_lock, output);
            if (result == initialization_status::initialization_discard_required) {
                result = inspect_fixture_lock(reopened_fixture_lock, input, output);
            }
            if (::close(reopened_fixture_lock) != 0 &&
                result == initialization_status::initialization_discard_required) {
                result = initialization_status::io_failure_no_mutation;
            }
        }
    }
    if (!close_fd(reopened_fixture) &&
        result == initialization_status::initialization_discard_required) {
        result = initialization_status::io_failure_no_mutation;
    }
    if (result == initialization_status::initialization_discard_required) {
        output.writer_lock_root_sole_entry = true;
        output.lock_anchor_qualified = true;
    }
    if (result != initialization_status::initialization_discard_required ||
        extent == initialization_extent::writer_lock_anchor) {
        return finish(output, result, writer_fd, fixture_lock_fd, fixture_fd,
                      root_fd, parent_fd, guard_owned, input_session);
    }

    created_directory_identity envelopes_identity {};
    created_directory_identity attempts_identity {};
    created_directory_identity staging_identity {};
    result = create_directory_once(
        root_fd, "envelopes", input, envelopes_identity,
        output.envelopes_directory_created_no_replace,
        output.envelopes_directory_validated,
        output.envelopes_directory_synchronized, output);
    if (result == initialization_status::initialization_discard_required) {
        result = revalidate_accumulated_prefix(
            parent_fd, root_fd, fixture_fd, fixture_lock_fd, writer_fd,
            input, writer_identity, 1, envelopes_identity, attempts_identity,
             staging_identity, false, false, false, output);
    }
    if (result == initialization_status::initialization_discard_required) {
        result = create_directory_once(
            root_fd, "attempts", input, attempts_identity,
            output.attempts_directory_created_no_replace,
            output.attempts_directory_validated,
            output.attempts_directory_synchronized, output);
    }
    if (result == initialization_status::initialization_discard_required) {
        result = revalidate_accumulated_prefix(
            parent_fd, root_fd, fixture_fd, fixture_lock_fd, writer_fd,
            input, writer_identity, 2, envelopes_identity, attempts_identity,
             staging_identity, false, false, false, output);
    }
    if (result == initialization_status::initialization_discard_required) {
        result = create_directory_once(
            root_fd, "staging", input, staging_identity,
            output.staging_directory_created_no_replace,
            output.staging_directory_validated,
            output.staging_directory_synchronized, output);
    }
    if (result == initialization_status::initialization_discard_required) {
        result = revalidate_accumulated_prefix(
            parent_fd, root_fd, fixture_fd, fixture_lock_fd, writer_fd,
            input, writer_identity, 3, envelopes_identity, attempts_identity,
             staging_identity, false, false, false, output);
    }
    if (result != initialization_status::initialization_discard_required) {
        return finish(output, result, writer_fd, fixture_lock_fd, fixture_fd,
                      root_fd, parent_fd, guard_owned, input_session);
    }
    ++output.root_fixture_syscall_count;
    if (::fsync(root_fd) != 0) {
        return finish(output, initialization_status::io_failure_no_mutation,
                      writer_fd, fixture_lock_fd, fixture_fd, root_fd, parent_fd,
                      guard_owned, input_session);
    }
    output.root_directory_synchronized = true;
    result = revalidate_accumulated_prefix(
        parent_fd, root_fd, fixture_fd, fixture_lock_fd, writer_fd,
        input, writer_identity, 3, envelopes_identity, attempts_identity,
         staging_identity, false, false, true, output);
    if (result == initialization_status::initialization_discard_required) {
        output.directory_prefix_qualified = true;
    }
    if (result != initialization_status::initialization_discard_required ||
        extent == initialization_extent::directory_prefix) {
        return finish(output, result, writer_fd, fixture_lock_fd, fixture_fd,
                      root_fd, parent_fd, guard_owned, input_session);
    }

    if (!input_session.secure || !input_session.authenticated) {
        return finish(output, initialization_status::io_failure_no_mutation,
                      writer_fd, fixture_lock_fd, fixture_fd, root_fd, parent_fd,
                      guard_owned, input_session);
    }
    auto & marker_bytes = input_session.secure->initializing_marker;
    auto & readback_bytes = input_session.secure->initializing_marker_readback;
    int staging_fd = -1;
    int staging_validation_fd = -1;
    int staging_final_fd = -1;
    int marker_fd = -1;
    int marker_readonly_fd = -1;
    int published_marker_fd = -1;
    auto finish_marker = [&](initialization_status marker_result) noexcept {
        bool close_ok = close_fd(published_marker_fd);
        close_ok = close_fd(marker_readonly_fd) && close_ok;
        close_ok = close_fd(marker_fd) && close_ok;
        close_ok = close_fd(staging_final_fd) && close_ok;
        close_ok = close_fd(staging_validation_fd) && close_ok;
        close_ok = close_fd(staging_fd) && close_ok;
        if (!close_ok) {
            output.initializing_marker_qualified = false;
            marker_result = initialization_status::io_failure_no_mutation;
        }
        return finish(output, marker_result, writer_fd, fixture_lock_fd, fixture_fd,
                      root_fd, parent_fd, guard_owned, input_session);
    };

    std::array<std::uint8_t, 16> filesystem_uuid {};
    std::array<std::uint8_t, 16> subvolume_uuid {};
    std::copy_n(input.candidate_root.filesystem_uuid, filesystem_uuid.size(),
                filesystem_uuid.begin());
    std::copy_n(input.candidate_root.subvolume_uuid, subvolume_uuid.size(),
                subvolume_uuid.begin());
    if (!context_store_registry_lab_path_policy_v1(
            reinterpret_cast<const std::uint8_t *>(input.parent.canonical_path),
            input.parent.path_length,
            reinterpret_cast<const std::uint8_t *>(input.candidate_root.canonical_path),
            input.candidate_root.path_length,
            filesystem_uuid, subvolume_uuid, input.candidate_root.mount_id,
            input.candidate_root.device, input.candidate_root.owner_uid,
            output.path_policy_commitment)) {
        return finish_marker(initialization_status::invalid_request_no_mutation);
    }
    output.path_policy_commitment_computed = true;
    output.initializing_marker_phase_ordinal = 1;

    context_store_registry_lab_root_value_v1 marker_value {};
    marker_value.scope.root_id = output.generated_root_id;
    marker_value.scope.path_policy_commitment = output.path_policy_commitment;
    marker_value.scope.registry_id = input.sealed_inputs.expected_registry_id;
    marker_value.scope.registry_epoch = input.sealed_inputs.expected_registry_epoch;
    marker_value.store_uuid = output.generated_store_uuid;
    marker_value.filesystem_uuid = filesystem_uuid;
    marker_value.mount_id = input.candidate_root.mount_id;
    marker_value.owner_uid = input.candidate_root.owner_uid;
    marker_value.lock_st_dev = static_cast<std::uint64_t>(writer_identity.st_dev);
    marker_value.lock_st_ino = static_cast<std::uint64_t>(writer_identity.st_ino);
    marker_value.state = context_store_registry_lab_root_state_v1::initializing;
    marker_value.initial_head_digest = {};

    context_store_registry_lab_root_witness marker_witness;
    if (!context_store_registry_lab_admit_root_v1(
            marker_value, *input_session.wire_credential, {}, marker_witness)) {
        return finish_marker(initialization_status::invalid_request_no_mutation);
    }
    output.initializing_marker_admitted = true;
    output.initializing_marker_phase_ordinal = 2;
    std::size_t marker_size = 0;
    const auto encoded_marker = context_store_registry_lab_encode_root_v1(
        marker_value, *input_session.wire_credential, marker_witness,
        marker_bytes.data(), marker_bytes.size(), marker_size);
    if (!encoded_marker.authenticated() || marker_size == 0 ||
        marker_size > marker_bytes.size() || marker_size > UINT32_MAX) {
        return finish_marker(initialization_status::invalid_request_no_mutation);
    }
    output.initializing_marker_encoded = true;
    output.initializing_marker_size = static_cast<std::uint32_t>(marker_size);
    output.initializing_marker_content_digest = encoded_marker.content_digest;
    output.initializing_marker_phase_ordinal = 3;

    context_store_registry_lab_expectation marker_expectation {};
    marker_expectation.root_id = marker_value.scope.root_id;
    marker_expectation.path_policy_commitment = marker_value.scope.path_policy_commitment;
    marker_expectation.store_uuid = marker_value.store_uuid;
    marker_expectation.filesystem_uuid = marker_value.filesystem_uuid;
    marker_expectation.registry_id = marker_value.scope.registry_id;
    marker_expectation.registry_epoch = marker_value.scope.registry_epoch;
    marker_expectation.mount_id = marker_value.mount_id;
    marker_expectation.owner_uid = marker_value.owner_uid;
    marker_expectation.lock_st_dev = marker_value.lock_st_dev;
    marker_expectation.lock_st_ino = marker_value.lock_st_ino;
    marker_expectation.initial_head_digest = {};
    const auto verified_marker = context_store_registry_lab_verify_v1(
        context_store_registry_lab_kind::root, marker_bytes.data(), marker_size,
        *input_session.wire_credential, marker_expectation);
    if (!verified_marker.authenticated() ||
        !same_digest(verified_marker.content_digest,
                     output.initializing_marker_content_digest)) {
        return finish_marker(initialization_status::invalid_request_no_mutation);
    }
    output.initializing_marker_self_verified = true;
    output.initializing_marker_phase_ordinal = 4;

    result = revalidate_accumulated_prefix(
        parent_fd, root_fd, fixture_fd, fixture_lock_fd, writer_fd,
        input, writer_identity, 3, envelopes_identity, attempts_identity,
        staging_identity, false, false, false, output);
    if (result != initialization_status::initialization_discard_required) {
        return finish_marker(result);
    }
    staging_fd = open_contained(
        root_fd, "staging", O_RDONLY | O_DIRECTORY | O_CLOEXEC | O_NOFOLLOW,
        0, output, result);
    if (staging_fd < 0) {
        return finish_marker(result);
    }
    result = inspect_created_directory(staging_fd, "staging", input,
                                       staging_identity, false, true, output);
    if (result == initialization_status::initialization_discard_required) {
        result = inspect_empty_directory(staging_fd, output);
    }
    if (result != initialization_status::initialization_discard_required) {
        return finish_marker(result);
    }

    constexpr char staging_marker_suffix[] = "/staging/initialize-root.tmp";
    constexpr char published_marker_suffix[] = "/root.marker";
    if (input.candidate_root.path_length + sizeof(staging_marker_suffix) - 1 >
            initialization_max_path_bytes ||
        input.candidate_root.path_length + sizeof(published_marker_suffix) - 1 >
            initialization_max_path_bytes) {
        return finish_marker(initialization_status::invalid_request_no_mutation);
    }
    marker_fd = open_contained(
        staging_fd, "initialize-root.tmp",
        O_CREAT | O_EXCL | O_RDWR | O_CLOEXEC | O_NOFOLLOW,
        0600, output, result);
    if (marker_fd < 0) {
        return finish_marker(result);
    }
    output.initializing_marker_temp_created_no_replace = true;
    output.initializing_marker_phase_ordinal = 5;
    created_file_identity marker_identity {};
    result = inspect_marker_file(marker_fd, staging_marker_suffix,
                                 input, marker_identity, true, 0, true, output);
    if (result != initialization_status::initialization_discard_required) {
        return finish_marker(result);
    }
    ++output.root_fixture_syscall_count;
    if (::fchmod(marker_fd, 0600) != 0) {
        return finish_marker(map_error(errno));
    }
    result = inspect_marker_file(marker_fd, staging_marker_suffix,
                                 input, marker_identity, false, 0, false, output);
    if (result != initialization_status::initialization_discard_required) {
        return finish_marker(result);
    }
    output.initializing_marker_mode_revalidated = true;
    output.initializing_marker_phase_ordinal = 6;

    result = write_exact_at_zero(marker_fd, marker_bytes.data(), marker_size, output);
    if (result != initialization_status::initialization_discard_required) {
        return finish_marker(result);
    }
    result = inspect_marker_file(marker_fd, staging_marker_suffix,
                                 input, marker_identity, false, marker_size,
                                 false, output);
    if (result != initialization_status::initialization_discard_required) {
        return finish_marker(result);
    }
    output.initializing_marker_write_completed = true;
    output.initializing_marker_phase_ordinal = 7;
    result = read_exact_at_zero(marker_fd, readback_bytes.data(), marker_size, output);
    if (result != initialization_status::initialization_discard_required ||
        std::memcmp(marker_bytes.data(), readback_bytes.data(), marker_size) != 0) {
        return finish_marker(result == initialization_status::initialization_discard_required
                                 ? initialization_status::invalid_request_no_mutation
                                 : result);
    }
    output.initializing_marker_readback_exact = true;
    const auto verified_readback = context_store_registry_lab_verify_v1(
        context_store_registry_lab_kind::root, readback_bytes.data(), marker_size,
        *input_session.wire_credential, marker_expectation);
    if (!verified_readback.authenticated() ||
        !same_digest(verified_readback.content_digest,
                     output.initializing_marker_content_digest)) {
        return finish_marker(initialization_status::invalid_request_no_mutation);
    }
    output.initializing_marker_readback_authenticated = true;
    output.initializing_marker_phase_ordinal = 8;
    ++output.root_fixture_syscall_count;
    if (::fsync(marker_fd) != 0) {
        return finish_marker(map_error(errno));
    }
    output.initializing_marker_synchronized = true;
    output.initializing_marker_phase_ordinal = 9;

    result = revalidate_accumulated_prefix(
        parent_fd, root_fd, fixture_fd, fixture_lock_fd, writer_fd,
        input, writer_identity, 3, envelopes_identity, attempts_identity,
        staging_identity, false, true, false, output);
    if (result == initialization_status::initialization_discard_required) {
        staging_validation_fd = open_contained(
            root_fd, "staging", O_RDONLY | O_DIRECTORY | O_CLOEXEC | O_NOFOLLOW,
            0, output, result);
        if (staging_validation_fd < 0) {
            return finish_marker(result);
        }
        result = compare_open_descriptions(staging_fd, staging_validation_fd, output);
    }
    if (result == initialization_status::initialization_discard_required) {
        result = inspect_created_directory(staging_validation_fd, "staging", input,
                                           staging_identity, false, true, output);
    }
    if (result == initialization_status::initialization_discard_required) {
        result = inspect_staging_initializing_marker_temp(staging_validation_fd, output);
    }
    if (result != initialization_status::initialization_discard_required) {
        return finish_marker(result);
    }

    // Select and fully authenticate the exact staging inode immediately before
    // the no-replace rename. No name-based inspection may intervene.
    marker_readonly_fd = open_contained(
        staging_fd, "initialize-root.tmp", O_RDONLY | O_CLOEXEC | O_NOFOLLOW,
        0, output, result);
    if (marker_readonly_fd < 0) {
        return finish_marker(result);
    }
    result = compare_open_descriptions(marker_fd, marker_readonly_fd, output);
    if (result == initialization_status::initialization_discard_required) {
        result = inspect_marker_file(marker_readonly_fd, staging_marker_suffix,
                                     input, marker_identity, false, marker_size,
                                     false, output);
    }
    if (result == initialization_status::initialization_discard_required) {
        wipe(readback_bytes.data(), readback_bytes.size());
        result = read_exact_at_zero(marker_readonly_fd, readback_bytes.data(),
                                    marker_size, output);
    }
    if (result != initialization_status::initialization_discard_required ||
        std::memcmp(marker_bytes.data(), readback_bytes.data(), marker_size) != 0) {
        return finish_marker(result == initialization_status::initialization_discard_required
                                 ? initialization_status::invalid_request_no_mutation
                                 : result);
    }
    const auto verified_readonly = context_store_registry_lab_verify_v1(
        context_store_registry_lab_kind::root, readback_bytes.data(), marker_size,
        *input_session.wire_credential, marker_expectation);
    if (!verified_readonly.authenticated() ||
        !same_digest(verified_readonly.content_digest,
                     output.initializing_marker_content_digest)) {
        return finish_marker(initialization_status::invalid_request_no_mutation);
    }
    output.initializing_marker_readonly_reopen_matched = true;
    output.initializing_marker_pre_publication_revalidation_matched = true;

    ++output.root_fixture_syscall_count;
    if (::syscall(SYS_renameat2, staging_fd, "initialize-root.tmp",
                  root_fd, "root.marker", RENAME_NOREPLACE) != 0) {
        const int rename_error = errno;
        return finish_marker(rename_error == ENOSYS || rename_error == EINVAL
                                 ? initialization_status::unsupported_no_mutation
                                 : map_error(rename_error));
    }
    output.initializing_marker_published_no_replace = true;
    output.initializing_marker_phase_ordinal = 10;
    ++output.root_fixture_syscall_count;
    if (::fsync(root_fd) != 0) {
        return finish_marker(map_error(errno));
    }
    output.root_synchronized_after_marker = true;
    output.initializing_marker_phase_ordinal = 11;
    ++output.root_fixture_syscall_count;
    if (::fsync(staging_fd) != 0) {
        return finish_marker(map_error(errno));
    }
    output.staging_synchronized_after_marker = true;
    output.initializing_marker_phase_ordinal = 12;

    published_marker_fd = open_contained(
        root_fd, "root.marker", O_RDONLY | O_CLOEXEC | O_NOFOLLOW,
        0, output, result);
    if (published_marker_fd < 0) {
        return finish_marker(result);
    }
    result = compare_open_descriptions(marker_fd, published_marker_fd, output);
    if (result == initialization_status::initialization_discard_required) {
        result = inspect_marker_file(published_marker_fd, published_marker_suffix, input,
                                     marker_identity, false, marker_size,
                                     false, output);
    }
    if (result == initialization_status::initialization_discard_required) {
        wipe(readback_bytes.data(), readback_bytes.size());
        result = read_exact_at_zero(published_marker_fd, readback_bytes.data(),
                                    marker_size, output);
    }
    if (result != initialization_status::initialization_discard_required ||
        std::memcmp(marker_bytes.data(), readback_bytes.data(), marker_size) != 0) {
        return finish_marker(result == initialization_status::initialization_discard_required
                                 ? initialization_status::invalid_request_no_mutation
                                 : result);
    }
    const auto verified_published = context_store_registry_lab_verify_v1(
        context_store_registry_lab_kind::root, readback_bytes.data(), marker_size,
        *input_session.wire_credential, marker_expectation);
    if (!verified_published.authenticated() ||
        !same_digest(verified_published.content_digest,
                     output.initializing_marker_content_digest)) {
        return finish_marker(initialization_status::invalid_request_no_mutation);
    }
    result = revalidate_accumulated_prefix(
        parent_fd, root_fd, fixture_fd, fixture_lock_fd, writer_fd,
        input, writer_identity, 3, envelopes_identity, attempts_identity,
        staging_identity, true, false, false, output);
    if (result == initialization_status::initialization_discard_required) {
        staging_final_fd = open_contained(
            root_fd, "staging", O_RDONLY | O_DIRECTORY | O_CLOEXEC | O_NOFOLLOW,
            0, output, result);
        if (staging_final_fd < 0) {
            return finish_marker(result);
        }
        result = compare_open_descriptions(staging_fd, staging_final_fd, output);
    }
    if (result == initialization_status::initialization_discard_required) {
        result = inspect_created_directory(staging_final_fd, "staging", input,
                                           staging_identity, false, true, output);
    }
    if (result == initialization_status::initialization_discard_required) {
        result = inspect_empty_directory(staging_final_fd, output);
    }
    if (result != initialization_status::initialization_discard_required) {
        return finish_marker(result);
    }
    output.initializing_marker_device = marker_identity.device;
    output.initializing_marker_inode = marker_identity.inode;
    output.initializing_marker_mount_id = marker_identity.mount_id;
    output.initializing_marker_final_revalidation_matched = true;
    output.staging_empty_after_marker_publication = true;
    output.initializing_marker_qualified = true;
    output.initializing_marker_phase_ordinal = 13;
    if (extent == initialization_extent::initializing_marker) {
        return finish_marker(result);
    }

    // L05z begins only after the complete L05y barrier. Preserve the exact
    // authenticated fd4 bytes and independently derive their registry-lab
    // digest name without decoding or re-encoding them.
    const std::size_t predecessor_size =
        output.sealed_inputs.predecessor_envelope_size;
    auto & predecessor_bytes = input_session.secure->predecessor;
    auto & predecessor_readback = input_session.secure->predecessor_readback;
    if (predecessor_size == 0 || predecessor_size > predecessor_bytes.size() ||
        !authenticate_predecessor_bytes(
            predecessor_bytes.data(), predecessor_size, input_session,
            input.sealed_inputs, input.sealed_inputs.expected_predecessor_digest)) {
        return finish_marker(initialization_status::invalid_request_no_mutation);
    }
    output.predecessor_envelope_authenticated = true;
    output.predecessor_envelope_phase_ordinal = 1;
    if (!context_store_registry_lab_linux_initializer_predecessor_digest_v1(
            predecessor_bytes.data(), predecessor_size,
            output.predecessor_envelope_digest) ||
        !same_digest(output.predecessor_envelope_digest,
                     input.sealed_inputs.expected_predecessor_digest) ||
        !make_predecessor_envelope_name(
            output.predecessor_envelope_digest,
            output.predecessor_envelope_basename)) {
        return finish_marker(initialization_status::invalid_request_no_mutation);
    }
    output.predecessor_envelope_digest_name_computed = true;
    output.predecessor_envelope_size =
        static_cast<std::uint32_t>(predecessor_size);
    output.predecessor_envelope_phase_ordinal = 2;

    std::array<char, initialization_max_path_bytes + 1> envelope_path {};
    constexpr char envelope_directory_suffix[] = "/envelopes/";
    const std::size_t fixed_suffix_size = sizeof(envelope_directory_suffix) - 1;
    const std::size_t basename_size =
        std::strlen(output.predecessor_envelope_basename.data());
    if (basename_size != 71 ||
        !initialization_predecessor_envelope_path_admitted(
            input.candidate_root.path_length) ||
        input.candidate_root.path_length >
            envelope_path.size() - 1 - fixed_suffix_size - basename_size) {
        return finish_marker(initialization_status::invalid_request_no_mutation);
    }
    const std::size_t envelope_path_size = input.candidate_root.path_length +
        fixed_suffix_size + basename_size;
    std::memcpy(envelope_path.data(), input.candidate_root.canonical_path,
                input.candidate_root.path_length);
    std::memcpy(envelope_path.data() + input.candidate_root.path_length,
                envelope_directory_suffix, fixed_suffix_size);
    std::memcpy(envelope_path.data() + input.candidate_root.path_length +
                    fixed_suffix_size,
                output.predecessor_envelope_basename.data(), basename_size);
    if (envelope_path_size > initialization_max_path_bytes ||
        envelope_path[envelope_path_size] != '\0') {
        return finish_marker(initialization_status::invalid_request_no_mutation);
    }
    output.predecessor_envelope_path_admitted = true;
    if (marker_size > initialization_future_logical_authority_bound ||
        predecessor_size > initialization_future_logical_authority_bound - marker_size) {
        return finish_marker(initialization_status::invalid_request_no_mutation);
    }
    output.predecessor_envelope_logical_bound_admitted = true;
    output.predecessor_envelope_phase_ordinal = 3;

    result = revalidate_accumulated_prefix(
        parent_fd, root_fd, fixture_fd, fixture_lock_fd, writer_fd,
        input, writer_identity, 3, envelopes_identity, attempts_identity,
        staging_identity, true, false, false, output, false);
    int marker_after_fd = -1;
    if (result == initialization_status::initialization_discard_required) {
        marker_after_fd = open_contained(
            root_fd, "root.marker", O_RDONLY | O_CLOEXEC | O_NOFOLLOW,
            0, output, result);
        if (marker_after_fd < 0) {
            return finish_marker(result);
        }
        result = inspect_marker_file(
            marker_after_fd, published_marker_suffix, input, marker_identity,
            false, marker_size, false, output);
    }
    if (result == initialization_status::initialization_discard_required) {
        wipe(readback_bytes.data(), readback_bytes.size());
        result = read_exact_at_zero(
            marker_after_fd, readback_bytes.data(), marker_size, output);
    }
    if (result != initialization_status::initialization_discard_required ||
        std::memcmp(marker_bytes.data(), readback_bytes.data(), marker_size) != 0) {
        (void) close_fd(marker_after_fd);
        return finish_marker(result == initialization_status::initialization_discard_required
                                 ? initialization_status::invalid_request_no_mutation
                                 : result);
    }
    const auto verified_marker_before_envelope = context_store_registry_lab_verify_v1(
        context_store_registry_lab_kind::root, readback_bytes.data(), marker_size,
        *input_session.wire_credential, marker_expectation);
    const bool marker_before_envelope_matched =
        verified_marker_before_envelope.authenticated() &&
        same_digest(verified_marker_before_envelope.content_digest,
                    output.initializing_marker_content_digest);
    const bool marker_after_closed = close_fd(marker_after_fd);
    if (!marker_before_envelope_matched || !marker_after_closed) {
        return finish_marker(initialization_status::io_failure_no_mutation);
    }

    // Close the inherited writable marker open description exactly once before
    // the first L05z mutation. All subsequent marker access is read-only.
    if (!close_fd(marker_fd)) {
        return finish_marker(initialization_status::io_failure_no_mutation);
    }
    output.initializing_marker_write_descriptor_closed = true;
    if (!close_fd(marker_readonly_fd) || !close_fd(published_marker_fd) ||
        !close_fd(staging_validation_fd) || !close_fd(staging_final_fd)) {
        return finish_marker(initialization_status::io_failure_no_mutation);
    }
    output.predecessor_envelope_pre_mutation_revalidation_matched = true;
    output.predecessor_envelope_phase_ordinal = 4;

    int envelopes_fd = -1;
    int envelope_fd = -1;
    int envelope_staging_validation_fd = -1;
    int barrier_marker_fd = -1;
    int envelope_readonly_fd = -1;
    int published_envelope_fd = -1;
    int final_envelope_confirmation_fd = -1;
    int final_marker_fd = -1;
    int final_marker_confirmation_fd = -1;
    auto finish_envelope = [&](initialization_status envelope_result) noexcept {
        bool close_ok = close_fd(final_marker_fd);
        close_ok = close_fd(final_marker_confirmation_fd) && close_ok;
        close_ok = close_fd(final_envelope_confirmation_fd) && close_ok;
        close_ok = close_fd(published_envelope_fd) && close_ok;
        close_ok = close_fd(envelope_readonly_fd) && close_ok;
        close_ok = close_fd(barrier_marker_fd) && close_ok;
        close_ok = close_fd(envelope_staging_validation_fd) && close_ok;
        close_ok = close_fd(envelope_fd) && close_ok;
        close_ok = close_fd(envelopes_fd) && close_ok;
        if (!close_ok) {
            output.predecessor_envelope_qualified = false;
            envelope_result = initialization_status::io_failure_no_mutation;
        }
        return finish_marker(envelope_result);
    };

    envelopes_fd = open_contained(
        root_fd, "envelopes", O_RDONLY | O_DIRECTORY | O_CLOEXEC | O_NOFOLLOW,
        0, output, result);
    if (envelopes_fd < 0) {
        return finish_envelope(result);
    }
    result = inspect_created_directory(envelopes_fd, "envelopes", input,
                                       envelopes_identity, false, true, output);
    if (result == initialization_status::initialization_discard_required) {
        result = inspect_empty_directory(envelopes_fd, output, false);
    }
    if (result != initialization_status::initialization_discard_required) {
        return finish_envelope(result);
    }

    constexpr char staging_envelope_suffix[] =
        "/staging/initialize-envelope.tmp";
    std::array<char, initialization_predecessor_envelope_suffix_bytes + 1>
        published_envelope_suffix {};
    std::memcpy(published_envelope_suffix.data(), envelope_directory_suffix,
                fixed_suffix_size);
    std::memcpy(published_envelope_suffix.data() + fixed_suffix_size,
                output.predecessor_envelope_basename.data(), basename_size);
    if (std::strlen(published_envelope_suffix.data()) !=
            initialization_predecessor_envelope_suffix_bytes ||
        input.candidate_root.path_length + sizeof(staging_envelope_suffix) - 1 >
            initialization_max_path_bytes) {
        return finish_envelope(initialization_status::invalid_request_no_mutation);
    }
    envelope_fd = open_contained(
        staging_fd, "initialize-envelope.tmp",
        O_CREAT | O_EXCL | O_RDWR | O_CLOEXEC | O_NOFOLLOW,
        0600, output, result);
    if (envelope_fd < 0) {
        return finish_envelope(result);
    }
    output.predecessor_envelope_temp_created_no_replace = true;
    output.predecessor_envelope_phase_ordinal = 5;
    created_file_identity envelope_identity {};
    result = inspect_marker_file(
        envelope_fd, staging_envelope_suffix, input, envelope_identity,
        true, 0, true, output);
    if (result != initialization_status::initialization_discard_required) {
        return finish_envelope(result);
    }
    ++output.root_fixture_syscall_count;
    if (::fchmod(envelope_fd, 0600) != 0) {
        return finish_envelope(map_error(errno));
    }
    result = inspect_marker_file(
        envelope_fd, staging_envelope_suffix, input, envelope_identity,
        false, 0, false, output);
    if (result != initialization_status::initialization_discard_required) {
        return finish_envelope(result);
    }
    output.predecessor_envelope_mode_revalidated = true;
    output.predecessor_envelope_phase_ordinal = 6;

    result = write_exact_at_zero(
        envelope_fd, predecessor_bytes.data(), predecessor_size, output);
    if (result == initialization_status::initialization_discard_required) {
        result = inspect_marker_file(
            envelope_fd, staging_envelope_suffix, input, envelope_identity,
            false, predecessor_size, false, output);
    }
    if (result != initialization_status::initialization_discard_required) {
        return finish_envelope(result);
    }
    output.predecessor_envelope_write_completed = true;
    output.predecessor_envelope_phase_ordinal = 7;
    wipe(predecessor_readback.data(), predecessor_readback.size());
    result = read_exact_at_zero(
        envelope_fd, predecessor_readback.data(), predecessor_size, output);
    if (result != initialization_status::initialization_discard_required ||
        std::memcmp(predecessor_bytes.data(), predecessor_readback.data(),
                    predecessor_size) != 0) {
        return finish_envelope(result == initialization_status::initialization_discard_required
                                   ? initialization_status::invalid_request_no_mutation
                                   : result);
    }
    output.predecessor_envelope_readback_exact = true;
    if (!authenticate_predecessor_bytes(
            predecessor_readback.data(), predecessor_size, input_session,
            input.sealed_inputs, output.predecessor_envelope_digest)) {
        return finish_envelope(initialization_status::invalid_request_no_mutation);
    }
    output.predecessor_envelope_readback_authenticated = true;
    output.predecessor_envelope_phase_ordinal = 8;
    ++output.root_fixture_syscall_count;
    if (::fsync(envelope_fd) != 0) {
        return finish_envelope(map_error(errno));
    }
    output.predecessor_envelope_synchronized = true;
    output.predecessor_envelope_phase_ordinal = 9;

    result = revalidate_accumulated_prefix(
        parent_fd, root_fd, fixture_fd, fixture_lock_fd, writer_fd,
        input, writer_identity, 3, envelopes_identity, attempts_identity,
        staging_identity, true, true, false, output, false);
    if (result == initialization_status::initialization_discard_required) {
        barrier_marker_fd = open_contained(
            root_fd, "root.marker", O_RDONLY | O_CLOEXEC | O_NOFOLLOW,
            0, output, result);
        if (barrier_marker_fd < 0) {
            return finish_envelope(result);
        }
        result = inspect_marker_file(
            barrier_marker_fd, published_marker_suffix, input, marker_identity,
            false, marker_size, false, output);
    }
    if (result == initialization_status::initialization_discard_required) {
        wipe(readback_bytes.data(), readback_bytes.size());
        result = read_exact_at_zero(
            barrier_marker_fd, readback_bytes.data(), marker_size, output);
    }
    if (result == initialization_status::initialization_discard_required &&
        std::memcmp(marker_bytes.data(), readback_bytes.data(), marker_size) == 0) {
        const auto verified_barrier_marker = context_store_registry_lab_verify_v1(
            context_store_registry_lab_kind::root,
            readback_bytes.data(), marker_size,
            *input_session.wire_credential, marker_expectation);
        if (!verified_barrier_marker.authenticated() ||
            !same_digest(verified_barrier_marker.content_digest,
                         output.initializing_marker_content_digest)) {
            result = initialization_status::invalid_request_no_mutation;
        }
    } else if (result == initialization_status::initialization_discard_required) {
        result = initialization_status::invalid_request_no_mutation;
    }
    if (!close_fd(barrier_marker_fd) &&
        result == initialization_status::initialization_discard_required) {
        result = initialization_status::io_failure_no_mutation;
    }
    if (result == initialization_status::initialization_discard_required) {
        envelope_staging_validation_fd = open_contained(
            root_fd, "staging",
            O_RDONLY | O_DIRECTORY | O_CLOEXEC | O_NOFOLLOW,
            0, output, result);
        if (envelope_staging_validation_fd < 0) {
            return finish_envelope(result);
        }
        result = compare_open_descriptions(
            staging_fd, envelope_staging_validation_fd, output);
    }
    if (result == initialization_status::initialization_discard_required) {
        result = inspect_created_directory(
            envelope_staging_validation_fd, "staging", input,
            staging_identity, false, true, output);
    }
    if (result == initialization_status::initialization_discard_required) {
        result = inspect_single_directory_entry(
            envelope_staging_validation_fd, "initialize-envelope.tmp", output);
    }
    if (result != initialization_status::initialization_discard_required) {
        return finish_envelope(result);
    }

    // This read-only selection is the final name-based operation before the
    // sole no-replace publication syscall.
    envelope_readonly_fd = open_contained(
        staging_fd, "initialize-envelope.tmp",
        O_RDONLY | O_CLOEXEC | O_NOFOLLOW, 0, output, result);
    if (envelope_readonly_fd < 0) {
        return finish_envelope(result);
    }
    result = compare_open_descriptions(
        envelope_fd, envelope_readonly_fd, output);
    if (result == initialization_status::initialization_discard_required) {
        result = inspect_marker_file(
            envelope_readonly_fd, staging_envelope_suffix, input,
            envelope_identity, false, predecessor_size, false, output);
    }
    if (result == initialization_status::initialization_discard_required) {
        wipe(predecessor_readback.data(), predecessor_readback.size());
        result = read_exact_at_zero(
            envelope_readonly_fd, predecessor_readback.data(),
            predecessor_size, output);
    }
    if (result != initialization_status::initialization_discard_required ||
        std::memcmp(predecessor_bytes.data(), predecessor_readback.data(),
                    predecessor_size) != 0 ||
        !authenticate_predecessor_bytes(
            predecessor_readback.data(), predecessor_size, input_session,
            input.sealed_inputs, output.predecessor_envelope_digest)) {
        return finish_envelope(result == initialization_status::initialization_discard_required
                                   ? initialization_status::invalid_request_no_mutation
                                   : result);
    }
    output.predecessor_envelope_readonly_reopen_matched = true;
    output.predecessor_envelope_pre_publication_revalidation_matched = true;
    ++output.root_fixture_syscall_count;
    if (::syscall(SYS_renameat2, staging_fd, "initialize-envelope.tmp",
                  envelopes_fd, output.predecessor_envelope_basename.data(),
                  RENAME_NOREPLACE) != 0) {
        const int rename_error = errno;
        return finish_envelope(rename_error == ENOSYS || rename_error == EINVAL
                                   ? initialization_status::unsupported_no_mutation
                                   : map_error(rename_error));
    }
    output.predecessor_envelope_published_no_replace = true;
    output.predecessor_envelope_phase_ordinal = 10;
    ++output.root_fixture_syscall_count;
    if (::fsync(envelopes_fd) != 0) {
        return finish_envelope(map_error(errno));
    }
    output.envelopes_synchronized_after_predecessor = true;
    output.predecessor_envelope_phase_ordinal = 11;
    ++output.root_fixture_syscall_count;
    if (::fsync(staging_fd) != 0) {
        return finish_envelope(map_error(errno));
    }
    output.staging_synchronized_after_predecessor = true;
    output.predecessor_envelope_phase_ordinal = 12;

    published_envelope_fd = open_contained(
        envelopes_fd, output.predecessor_envelope_basename.data(),
        O_RDONLY | O_CLOEXEC | O_NOFOLLOW, 0, output, result);
    if (published_envelope_fd < 0) {
        return finish_envelope(result);
    }
    result = compare_open_descriptions(
        envelope_fd, published_envelope_fd, output);
    if (result == initialization_status::initialization_discard_required) {
        result = inspect_marker_file(
            published_envelope_fd, published_envelope_suffix.data(), input,
            envelope_identity, false, predecessor_size, false, output);
    }
    if (result == initialization_status::initialization_discard_required) {
        wipe(predecessor_readback.data(), predecessor_readback.size());
        result = read_exact_at_zero(
            published_envelope_fd, predecessor_readback.data(),
            predecessor_size, output);
    }
    if (result != initialization_status::initialization_discard_required ||
        std::memcmp(predecessor_bytes.data(), predecessor_readback.data(),
                    predecessor_size) != 0 ||
        !authenticate_predecessor_bytes(
            predecessor_readback.data(), predecessor_size, input_session,
            input.sealed_inputs, output.predecessor_envelope_digest)) {
        return finish_envelope(result == initialization_status::initialization_discard_required
                                   ? initialization_status::invalid_request_no_mutation
                                   : result);
    }
    output.predecessor_envelope_final_revalidation_matched = true;

    final_marker_fd = open_contained(
        root_fd, "root.marker", O_RDONLY | O_CLOEXEC | O_NOFOLLOW,
        0, output, result);
    if (final_marker_fd < 0) {
        return finish_envelope(result);
    }
    result = inspect_marker_file(
        final_marker_fd, published_marker_suffix, input, marker_identity,
        false, marker_size, false, output);
    if (result == initialization_status::initialization_discard_required) {
        wipe(readback_bytes.data(), readback_bytes.size());
        result = read_exact_at_zero(
            final_marker_fd, readback_bytes.data(), marker_size, output);
    }
    const auto final_marker_verification =
        result == initialization_status::initialization_discard_required
            ? context_store_registry_lab_verify_v1(
                  context_store_registry_lab_kind::root,
                  readback_bytes.data(), marker_size,
                  *input_session.wire_credential, marker_expectation)
            : context_store_registry_lab_wire_result {};
    if (result != initialization_status::initialization_discard_required ||
        std::memcmp(marker_bytes.data(), readback_bytes.data(), marker_size) != 0 ||
        !final_marker_verification.authenticated() ||
        !same_digest(final_marker_verification.content_digest,
                     output.initializing_marker_content_digest)) {
        return finish_envelope(result == initialization_status::initialization_discard_required
                                   ? initialization_status::invalid_request_no_mutation
                                   : result);
    }
    output.initializing_marker_unchanged_after_predecessor = true;
    result = revalidate_predecessor_final_layout(
        parent_fd, root_fd, fixture_fd, fixture_lock_fd, writer_fd,
        input, writer_identity, envelopes_identity, attempts_identity,
        staging_identity, output.predecessor_envelope_basename.data(), output);
    if (result != initialization_status::initialization_discard_required) {
        return finish_envelope(result);
    }
    final_envelope_confirmation_fd = open_contained(
        envelopes_fd, output.predecessor_envelope_basename.data(),
        O_RDONLY | O_CLOEXEC | O_NOFOLLOW, 0, output, result);
    if (final_envelope_confirmation_fd < 0) {
        return finish_envelope(result);
    }
    result = compare_open_descriptions(
        published_envelope_fd, final_envelope_confirmation_fd, output);
    if (result == initialization_status::initialization_discard_required) {
        result = inspect_marker_file(
            final_envelope_confirmation_fd, published_envelope_suffix.data(), input,
            envelope_identity, false, predecessor_size, false, output);
    }
    if (result == initialization_status::initialization_discard_required) {
        wipe(predecessor_readback.data(), predecessor_readback.size());
        result = read_exact_at_zero(
            final_envelope_confirmation_fd, predecessor_readback.data(),
            predecessor_size, output);
    }
    if (result != initialization_status::initialization_discard_required ||
        std::memcmp(predecessor_bytes.data(), predecessor_readback.data(),
                    predecessor_size) != 0 ||
        !authenticate_predecessor_bytes(
            predecessor_readback.data(), predecessor_size, input_session,
            input.sealed_inputs, output.predecessor_envelope_digest)) {
        return finish_envelope(result == initialization_status::initialization_discard_required
                                   ? initialization_status::invalid_request_no_mutation
                                   : result);
    }
    final_marker_confirmation_fd = open_contained(
        root_fd, "root.marker", O_RDONLY | O_CLOEXEC | O_NOFOLLOW,
        0, output, result);
    if (final_marker_confirmation_fd < 0) {
        return finish_envelope(result);
    }
    result = inspect_marker_file(
        final_marker_confirmation_fd, published_marker_suffix, input,
        marker_identity, false, marker_size, false, output);
    if (result == initialization_status::initialization_discard_required) {
        wipe(readback_bytes.data(), readback_bytes.size());
        result = read_exact_at_zero(
            final_marker_confirmation_fd, readback_bytes.data(),
            marker_size, output);
    }
    const auto confirmed_final_marker =
        result == initialization_status::initialization_discard_required
            ? context_store_registry_lab_verify_v1(
                  context_store_registry_lab_kind::root,
                  readback_bytes.data(), marker_size,
                  *input_session.wire_credential, marker_expectation)
            : context_store_registry_lab_wire_result {};
    const bool final_marker_confirmed =
        result == initialization_status::initialization_discard_required &&
        std::memcmp(marker_bytes.data(), readback_bytes.data(), marker_size) == 0 &&
        confirmed_final_marker.authenticated() &&
        same_digest(confirmed_final_marker.content_digest,
                    output.initializing_marker_content_digest);
    const bool final_marker_confirmation_closed =
        close_fd(final_marker_confirmation_fd);
    if (!final_marker_confirmed || !final_marker_confirmation_closed) {
        return finish_envelope(
            result == initialization_status::initialization_discard_required
                ? initialization_status::invalid_request_no_mutation : result);
    }
    output.predecessor_envelope_device = envelope_identity.device;
    output.predecessor_envelope_inode = envelope_identity.inode;
    output.predecessor_envelope_mount_id = envelope_identity.mount_id;
    output.predecessor_envelope_final_layout_matched = true;
    output.predecessor_envelope_qualified = true;
    output.predecessor_envelope_phase_ordinal = 13;
    return finish_envelope(result);
}

} // namespace

initialization_audit initialize_writer_lock_anchor_once(
        const initialization_request & input) noexcept {
    return initialize_once(input, initialization_extent::writer_lock_anchor);
}

initialization_audit initialize_directory_prefix_once(
        const initialization_request & input) noexcept {
    return initialize_once(input, initialization_extent::directory_prefix);
}

initialization_audit initialize_initializing_marker_once(
        const initialization_request & input) noexcept {
    return initialize_once(input, initialization_extent::initializing_marker);
}

initialization_audit initialize_predecessor_envelope_once(
        const initialization_request & input) noexcept {
    return initialize_once(input, initialization_extent::predecessor_envelope);
}

} // namespace halofpx::registry_lab::linux_initializer
