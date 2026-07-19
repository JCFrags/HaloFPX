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
};

struct created_directory_identity {
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
        int fd, unsigned expected_directories,
        initialization_audit & output) noexcept {
    if (expected_directories == 0 || expected_directories > 3) {
        return initialization_status::invalid_request_no_mutation;
    }
    std::array<char, 4096> bytes {};
    std::size_t entries = 0;
    bool saw_writer = false;
    bool saw_envelopes = false;
    bool saw_attempts = false;
    bool saw_staging = false;
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
            if (++entries > expected_directories + 1) {
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
            } else {
                return initialization_status::invalid_request_no_mutation;
            }
            if (*seen) {
                return initialization_status::invalid_request_no_mutation;
            }
            *seen = true;
        }
    }
    return entries == expected_directories + 1 && saw_writer && saw_envelopes &&
                   saw_attempts == (expected_directories >= 2) &&
                   saw_staging == (expected_directories >= 3)
               ? initialization_status::initialization_discard_required
               : initialization_status::invalid_request_no_mutation;
}

initialization_status inspect_empty_directory(
        int fd, initialization_audit & output) noexcept {
    std::array<char, 4096> bytes {};
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

initialization_status inspect_fixture_layout(int fd,
                                             initialization_audit & output) noexcept {
    std::array<char, 4096> bytes {};
    std::size_t entries = 0;
    bool saw_lock = false;
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
                                          initialization_audit & output) noexcept {
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
            if (errno == EINTR) {
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
        initialization_audit & output) noexcept {
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
    const auto result = mountinfo_read_only(root.mount_id, mount_read_only, output);
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
        initialization_audit & output) noexcept {
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
    if (result == initialization_status::initialization_discard_required) {
        result = inspect_empty_directory(reopened, output);
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
        bool final_revalidation,
        initialization_audit & output) noexcept {
    auto result = revalidate_parent(parent_fd, input.parent, output);
    int reopened_root = -1;
    if (result == initialization_status::initialization_discard_required) {
        result = reopen_directory(parent_fd, input.parent, input.candidate_root,
                                  root_fd, reopened_root, output);
    }
    if (result == initialization_status::initialization_discard_required) {
        result = reserve_and_read_only(reopened_root, input.candidate_root, output);
    }
    if (result == initialization_status::initialization_discard_required) {
        result = inspect_accumulated_prefix_layout(
            reopened_root, expected_directories, output);
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
                                          input, envelopes_identity, output);
    }
    if (result == initialization_status::initialization_discard_required &&
        expected_directories >= 2) {
        result = reopen_created_directory(reopened_root, "attempts", root_fd,
                                          input, attempts_identity, output);
    }
    if (result == initialization_status::initialization_discard_required &&
        expected_directories >= 3) {
        result = reopen_created_directory(reopened_root, "staging", root_fd,
                                          input, staging_identity, output);
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

bool unlock_and_close(int & fd, bool acquired, bool & released) noexcept {
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
        int value;
        do {
            value = ::fcntl(fd, F_OFD_SETLK, &unlock);
        } while (value != 0 && errno == EINTR);
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
    close_ok = unlock_and_close(writer_fd, output.writer_lock_ofd_acquired,
                                output.writer_lock_released) && close_ok;
    close_ok = unlock_and_close(fixture_lock_fd, output.fixture_lock_acquired,
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
            staging_identity, false, output);
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
            staging_identity, false, output);
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
            staging_identity, false, output);
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
        staging_identity, true, output);
    if (result == initialization_status::initialization_discard_required) {
        output.directory_prefix_qualified = true;
    }
    return finish(output, result, writer_fd, fixture_lock_fd, fixture_fd,
                  root_fd, parent_fd, guard_owned, input_session);
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

} // namespace halofpx::registry_lab::linux_initializer
