#ifdef NDEBUG
#undef NDEBUG
#endif

#include "halofpx-context-store-registry-lab-linux-initializer-internal.h"

#include <algorithm>
#include <array>
#include <cassert>
#include <cerrno>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <iterator>
#include <string>
#include <vector>

#include <fcntl.h>
#include <linux/btrfs.h>
#include <linux/close_range.h>
#include <linux/magic.h>
#include <linux/memfd.h>
#include <linux/openat2.h>
#include <linux/stat.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/statfs.h>
#include <sys/syscall.h>
#include <sys/wait.h>
#include <signal.h>
#include <time.h>
#include <unistd.h>

using namespace halofpx::registry_lab::linux_initializer;

namespace {

struct controller_linux_dirent64 {
    std::uint64_t inode;
    std::int64_t offset;
    unsigned short record_length;
    unsigned char type;
    char name[1];
};

constexpr std::uint64_t controller_resolve =
    RESOLVE_BENEATH | RESOLVE_NO_SYMLINKS |
    RESOLVE_NO_MAGICLINKS | RESOLVE_NO_XDEV;

std::uint8_t nibble(char value) {
    if (value >= '0' && value <= '9') {
        return static_cast<std::uint8_t>(value - '0');
    }
    if (value >= 'a' && value <= 'f') {
        return static_cast<std::uint8_t>(value - 'a' + 10);
    }
    return 0xff;
}

bool hex_bytes(const std::string & text, std::vector<std::uint8_t> & output) {
    if ((text.size() & 1U) != 0) {
        return false;
    }
    output.resize(text.size() / 2);
    for (std::size_t i = 0; i < output.size(); ++i) {
        const auto high = nibble(text[2 * i]);
        const auto low = nibble(text[2 * i + 1]);
        if (high > 0x0f || low > 0x0f) {
            return false;
        }
        output[i] = static_cast<std::uint8_t>((high << 4) | low);
    }
    return true;
}

bool json_string(const std::string & json, const std::string & key,
                 std::string & output) {
    auto position = json.find("\"" + key + "\"");
    if (position == std::string::npos) {
        return false;
    }
    position = json.find(':', position);
    if (position == std::string::npos) {
        return false;
    }
    position = json.find('"', position);
    if (position == std::string::npos) {
        return false;
    }
    const auto end = json.find('"', position + 1);
    if (end == std::string::npos) {
        return false;
    }
    output = json.substr(position + 1, end - position - 1);
    return true;
}

bool write_all(int fd, const std::vector<std::uint8_t> & bytes) {
    std::size_t offset = 0;
    while (offset < bytes.size()) {
        const ssize_t count = ::write(fd, bytes.data() + offset, bytes.size() - offset);
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

bool install_memfd(const char * name, const std::vector<std::uint8_t> & bytes,
                   int target, bool cloexec = true) {
    int fd = static_cast<int>(::syscall(
        SYS_memfd_create, name,
        MFD_ALLOW_SEALING | (cloexec ? MFD_CLOEXEC : 0)));
    if (fd < 0 || !write_all(fd, bytes)) {
        if (fd >= 0) {
            ::close(fd);
        }
        return false;
    }
    constexpr int seals = F_SEAL_SEAL | F_SEAL_SHRINK | F_SEAL_GROW | F_SEAL_WRITE;
    if (::fcntl(fd, F_ADD_SEALS, seals) != 0) {
        ::close(fd);
        return false;
    }
    if (fd != target) {
        if (::dup3(fd, target, cloexec ? O_CLOEXEC : 0) != target ||
            ::close(fd) != 0) {
            ::close(fd);
            return false;
        }
    }
    return ::fcntl(target, F_SETFD, cloexec ? FD_CLOEXEC : 0) == 0;
}

bool make_sealed_request(const std::string & json,
                         std::vector<std::uint8_t> & package,
                         std::vector<std::uint8_t> & predecessor,
                         sealed_input_request & output) {
    std::string package_hex;
    std::string predecessor_hex;
    std::string digest_hex;
    std::string continuity_hex;
    std::vector<std::uint8_t> digest;
    std::vector<std::uint8_t> continuity;
    if (!json_string(json, "credential_package_hex", package_hex) ||
        !json_string(json, "predecessor_registry_envelope_hex", predecessor_hex) ||
        !json_string(json, "predecessor_registry_envelope_digest_hex", digest_hex) ||
        !json_string(json, "key_continuity_hmac_sha256_hex", continuity_hex) ||
        !hex_bytes(package_hex, package) || !hex_bytes(predecessor_hex, predecessor) ||
        !hex_bytes(digest_hex, digest) || !hex_bytes(continuity_hex, continuity) ||
        package.size() < 16 + 2 + 1 + 8 + 2 + 32 || digest.size() != 32 ||
        continuity.size() != 32) {
        return false;
    }
    const std::size_t key_size =
        static_cast<std::size_t>(package[16] << 8 | package[17]);
    if (key_size == 0 || key_size > output.expected_key_id.bytes.size() ||
        18 + key_size + 8 > package.size()) {
        return false;
    }
    output.expected_key_id.size = static_cast<std::uint8_t>(key_size);
    std::copy_n(package.data() + 18, key_size, output.expected_key_id.bytes.begin());
    std::size_t offset = 18 + key_size;
    for (unsigned i = 0; i < 8; ++i) {
        output.expected_key_generation =
            (output.expected_key_generation << 8) | package[offset + i];
    }
    std::copy(digest.begin(), digest.end(), output.expected_predecessor_digest.begin());
    constexpr char registry_id[] = "registry-v1";
    output.expected_registry_id.size = sizeof(registry_id) - 1;
    std::copy_n(registry_id, output.expected_registry_id.size,
                output.expected_registry_id.bytes.begin());
    output.expected_registry_epoch = 9;
    output.expected_authority_base_scope_commitment.fill(0xaa);
    output.expected_policy_commitment.fill(0xbb);
    output.expected_predecessor_high_water = 40;
    std::copy(continuity.begin(), continuity.end(),
              output.expected_key_continuity_commitment.begin());
    return true;
}

bool derive_path_identity(const char * path,
                          initialization_pinned_path_identity & output) {
    const std::size_t path_size = std::strlen(path);
    if (path_size == 0 || path_size > initialization_max_path_bytes) {
        return false;
    }
    int fd = ::open(path, O_RDONLY | O_DIRECTORY | O_CLOEXEC | O_NOFOLLOW);
    if (fd < 0) {
        return false;
    }
    struct stat value {};
    struct statx extended {};
    struct btrfs_ioctl_fs_info_args filesystem {};
    struct btrfs_ioctl_get_subvol_info_args subvolume {};
    std::array<char, 32> proc_path {};
    std::array<char, initialization_max_path_bytes + 1> observed_path {};
    const int proc_size = std::snprintf(
        proc_path.data(), proc_path.size(), "/proc/self/fd/%d", fd);
    const bool valid =
        ::fstat(fd, &value) == 0 && S_ISDIR(value.st_mode) &&
        ::syscall(SYS_statx, fd, "", AT_EMPTY_PATH | AT_STATX_SYNC_AS_STAT,
                  STATX_BASIC_STATS | STATX_MNT_ID, &extended) == 0 &&
        (extended.stx_mask & STATX_MNT_ID) != 0 &&
        ::ioctl(fd, BTRFS_IOC_FS_INFO, &filesystem) == 0 &&
        ::ioctl(fd, BTRFS_IOC_GET_SUBVOL_INFO, &subvolume) == 0 &&
        proc_size > 0 && static_cast<std::size_t>(proc_size) < proc_path.size() &&
        ::readlink(proc_path.data(), observed_path.data(), observed_path.size()) ==
            static_cast<ssize_t>(path_size) &&
        std::memcmp(observed_path.data(), path, path_size) == 0;
    if (valid) {
        std::memcpy(output.canonical_path, path, path_size + 1);
        output.path_length = static_cast<std::uint32_t>(path_size);
        output.device = static_cast<std::uint64_t>(value.st_dev);
        output.inode = static_cast<std::uint64_t>(value.st_ino);
        output.mount_id = extended.stx_mnt_id;
        output.owner_uid = static_cast<std::uint64_t>(value.st_uid);
        output.mode = static_cast<std::uint32_t>(value.st_mode & 07777);
        std::copy_n(filesystem.fsid, 16, output.filesystem_uuid);
        std::copy_n(subvolume.uuid, 16, output.subvolume_uuid);
    }
    const bool closed = ::close(fd) == 0;
    return valid && closed;
}

bool derive_fixture_lock(const char * fixture_path,
                         const initialization_pinned_path_identity & fixture,
                         std::uint64_t & device, std::uint64_t & inode) {
    int directory_fd = ::open(
        fixture_path, O_RDONLY | O_DIRECTORY | O_CLOEXEC | O_NOFOLLOW);
    if (directory_fd < 0) {
        return false;
    }
    int lock_fd = ::openat(directory_fd, "primitive.lock", O_RDONLY | O_CLOEXEC | O_NOFOLLOW);
    struct stat value {};
    struct statx extended {};
    const bool valid = lock_fd >= 0 && ::fstat(lock_fd, &value) == 0 &&
        S_ISREG(value.st_mode) && value.st_nlink == 1 && value.st_size == 0 &&
        static_cast<std::uint32_t>(value.st_mode & 07777) == 0600 &&
        ::syscall(SYS_statx, lock_fd, "", AT_EMPTY_PATH | AT_STATX_SYNC_AS_STAT,
                  STATX_BASIC_STATS | STATX_MNT_ID, &extended) == 0 &&
        (extended.stx_mask & STATX_MNT_ID) != 0 &&
        extended.stx_mnt_id == fixture.mount_id &&
        static_cast<std::uint64_t>(value.st_dev) == fixture.device;
    if (valid) {
        device = static_cast<std::uint64_t>(value.st_dev);
        inode = static_cast<std::uint64_t>(value.st_ino);
    }
    const bool lock_closed = lock_fd < 0 || ::close(lock_fd) == 0;
    const bool directory_closed = ::close(directory_fd) == 0;
    return valid && lock_closed && directory_closed;
}

int controller_open_contained(int parent_fd, const char * name,
                              std::uint64_t flags) {
    struct open_how how {};
    how.flags = flags;
    how.resolve = controller_resolve;
    int fd;
    do {
        fd = static_cast<int>(::syscall(
            SYS_openat2, parent_fd, name, &how, sizeof(how)));
    } while (fd < 0 && errno == EINTR);
    return fd;
}

bool controller_scan_directory(int fd, unsigned expected_mask) {
    std::array<char, 4096> bytes {};
    unsigned observed_mask = 0;
    for (;;) {
        long count;
        do {
            count = ::syscall(SYS_getdents64, fd, bytes.data(), bytes.size());
        } while (count < 0 && errno == EINTR);
        if (count < 0) {
            return false;
        }
        if (count == 0) {
            return observed_mask == expected_mask;
        }
        long offset = 0;
        while (offset < count) {
            const auto * entry = reinterpret_cast<const controller_linux_dirent64 *>(
                bytes.data() + offset);
            if (entry->record_length <
                    offsetof(controller_linux_dirent64, name) + 1 ||
                offset + entry->record_length > count) {
                return false;
            }
            const std::size_t capacity = entry->record_length -
                offsetof(controller_linux_dirent64, name);
            if (::strnlen(entry->name, capacity) == capacity) {
                return false;
            }
            offset += entry->record_length;
            if (std::strcmp(entry->name, ".") == 0 ||
                std::strcmp(entry->name, "..") == 0) {
                continue;
            }
            unsigned bit = 0;
            if (std::strcmp(entry->name, "writer.lock") == 0) {
                bit = 1U << 0;
            } else if (std::strcmp(entry->name, "envelopes") == 0) {
                bit = 1U << 1;
            } else if (std::strcmp(entry->name, "attempts") == 0) {
                bit = 1U << 2;
            } else if (std::strcmp(entry->name, "staging") == 0) {
                bit = 1U << 3;
            } else {
                return false;
            }
            if ((expected_mask & bit) == 0 || (observed_mask & bit) != 0) {
                return false;
            }
            observed_mask |= bit;
        }
    }
}

bool controller_node_mount_matches(int fd, std::uint64_t mount_id) {
    struct statx extended {};
    return ::syscall(SYS_statx, fd, "", AT_EMPTY_PATH | AT_STATX_SYNC_AS_STAT,
                     STATX_BASIC_STATS | STATX_MNT_ID, &extended) == 0 &&
           (extended.stx_mask & STATX_MNT_ID) != 0 &&
           extended.stx_mnt_id == mount_id;
}

bool inspect_final_directory_prefix(
        const char * root_path, const initialization_request & request) {
    int root_fd = ::open(
        root_path, O_RDONLY | O_DIRECTORY | O_CLOEXEC | O_NOFOLLOW);
    if (root_fd < 0) {
        return false;
    }
    struct stat root {};
    bool valid = ::fstat(root_fd, &root) == 0 && S_ISDIR(root.st_mode) &&
        static_cast<std::uint64_t>(root.st_dev) == request.candidate_root.device &&
        static_cast<std::uint64_t>(root.st_ino) == request.candidate_root.inode &&
        static_cast<std::uint64_t>(root.st_uid) == request.candidate_root.owner_uid &&
        static_cast<std::uint32_t>(root.st_mode & 07777) == 0700 &&
        controller_node_mount_matches(root_fd, request.candidate_root.mount_id) &&
        controller_scan_directory(root_fd, 0x0fU);
    std::array<std::uint64_t, 5> inodes {};
    inodes[0] = static_cast<std::uint64_t>(root.st_ino);

    int writer_fd = -1;
    if (valid) {
        writer_fd = controller_open_contained(
            root_fd, "writer.lock", O_RDONLY | O_CLOEXEC | O_NOFOLLOW);
        struct stat writer {};
        valid = writer_fd >= 0 && ::fstat(writer_fd, &writer) == 0 &&
            S_ISREG(writer.st_mode) && writer.st_nlink == 1 && writer.st_size == 0 &&
            static_cast<std::uint64_t>(writer.st_dev) == request.candidate_root.device &&
            static_cast<std::uint64_t>(writer.st_uid) == request.candidate_root.owner_uid &&
            static_cast<std::uint32_t>(writer.st_mode & 07777) == 0600 &&
            controller_node_mount_matches(writer_fd, request.candidate_root.mount_id);
        if (valid) {
            inodes[1] = static_cast<std::uint64_t>(writer.st_ino);
        }
    }
    if (writer_fd >= 0 && ::close(writer_fd) != 0) {
        valid = false;
    }

    constexpr const char * directory_names[] = {
        "envelopes", "attempts", "staging",
    };
    for (std::size_t index = 0; valid && index < 3; ++index) {
        int directory_fd = controller_open_contained(
            root_fd, directory_names[index],
            O_RDONLY | O_DIRECTORY | O_CLOEXEC | O_NOFOLLOW);
        struct stat directory {};
        valid = directory_fd >= 0 && ::fstat(directory_fd, &directory) == 0 &&
            S_ISDIR(directory.st_mode) &&
            static_cast<std::uint64_t>(directory.st_dev) == request.candidate_root.device &&
            static_cast<std::uint64_t>(directory.st_uid) == request.candidate_root.owner_uid &&
            static_cast<std::uint32_t>(directory.st_mode & 07777) == 0700 &&
            controller_node_mount_matches(directory_fd, request.candidate_root.mount_id) &&
            controller_scan_directory(directory_fd, 0);
        if (valid) {
            inodes[index + 2] = static_cast<std::uint64_t>(directory.st_ino);
        }
        if (directory_fd >= 0 && ::close(directory_fd) != 0) {
            valid = false;
        }
    }
    for (std::size_t left = 0; valid && left < inodes.size(); ++left) {
        valid = inodes[left] != 0;
        for (std::size_t right = left + 1; valid && right < inodes.size(); ++right) {
            valid = inodes[left] != inodes[right];
        }
    }
    if (::close(root_fd) != 0) {
        valid = false;
    }
    return valid;
}

void set_path(initialization_pinned_path_identity & output, const char * value,
              std::uint64_t inode, std::uint32_t mode) {
    const std::size_t size = std::strlen(value);
    assert(size <= initialization_max_path_bytes);
    std::memcpy(output.canonical_path, value, size + 1);
    output.path_length = static_cast<std::uint32_t>(size);
    output.device = 0x11223344;
    output.inode = inode;
    output.mount_id = 0x55667788;
    output.owner_uid = static_cast<std::uint64_t>(::geteuid());
    output.mode = mode;
    for (std::size_t i = 0; i < 16; ++i) {
        output.filesystem_uuid[i] = static_cast<std::uint8_t>(0x10 + i);
        output.subvolume_uuid[i] = static_cast<std::uint8_t>(0x30 + i);
    }
}

initialization_request synthetic_valid_shape() {
    initialization_request output {};
    set_path(output.parent, "/mnt/halofpx-l05w-synthetic", 101, 0700);
    set_path(output.candidate_root, "/mnt/halofpx-l05w-synthetic/root", 102, 0700);
    set_path(output.fixture, "/mnt/halofpx-l05w-synthetic/fixture", 103, 0700);
    output.fixture_lock_device = output.fixture.device;
    output.fixture_lock_inode = 104;
    return output;
}

bool bounded_wait(pid_t child, int & wait_status);

int run_directory_synthetic() {
    (void) ::close(3);
    (void) ::close(4);
    const auto observed = initialize_directory_prefix_once(synthetic_valid_shape());
    assert(observed.result == initialization_status::invalid_request_no_mutation);
    assert(observed.sealed_inputs.result == sealed_input_status::invalid_request_no_mutation);
    assert(observed.sealed_inputs_preceded_root_access);
    assert(observed.root_guard_acquired);
    assert(observed.root_guard_released);
    assert(observed.root_fixture_syscall_count == 0);
    assert(!observed.discard_required_latched);
    assert(!observed.writer_lock_created_no_replace);
    assert(!observed.writer_lock_ofd_acquired);
    assert(!observed.lock_anchor_qualified);
    assert(!observed.envelopes_directory_created_no_replace);
    assert(!observed.envelopes_directory_validated);
    assert(!observed.envelopes_directory_synchronized);
    assert(!observed.attempts_directory_created_no_replace);
    assert(!observed.attempts_directory_validated);
    assert(!observed.attempts_directory_synchronized);
    assert(!observed.staging_directory_created_no_replace);
    assert(!observed.staging_directory_validated);
    assert(!observed.staging_directory_synchronized);
    assert(!observed.envelopes_directory_final_revalidation_matched);
    assert(!observed.attempts_directory_final_revalidation_matched);
    assert(!observed.staging_directory_final_revalidation_matched);
    assert(!observed.root_directory_synchronized);
    assert(!observed.directory_prefix_qualified);
    assert(observed.sealed_inputs.input_syscall_count != 0);
    assert(observed.sealed_inputs.secure_storage_wiped);
    assert(observed.sealed_inputs.secure_storage_unlocked);
    assert(observed.sealed_inputs.secure_storage_unmapped);
    assert(observed.sealed_inputs.signal_mask_restored);
    return 0;
}

int run_synthetic() {
    (void) ::close(3);
    (void) ::close(4);
    const auto observed = initialize_writer_lock_anchor_once(synthetic_valid_shape());
    assert(observed.result == initialization_status::invalid_request_no_mutation);
    assert(observed.sealed_inputs.result == sealed_input_status::invalid_request_no_mutation);
    assert(observed.sealed_inputs_preceded_root_access);
    assert(observed.root_guard_acquired);
    assert(observed.root_guard_released);
    assert(observed.root_fixture_syscall_count == 0);
    assert(!observed.discard_required_latched);
    assert(!observed.writer_lock_created_no_replace);
    assert(!observed.writer_lock_ofd_acquired);
    assert(!observed.lock_anchor_qualified);
    assert(!observed.envelopes_directory_created_no_replace);
    assert(!observed.envelopes_directory_validated);
    assert(!observed.envelopes_directory_synchronized);
    assert(!observed.attempts_directory_created_no_replace);
    assert(!observed.attempts_directory_validated);
    assert(!observed.attempts_directory_synchronized);
    assert(!observed.staging_directory_created_no_replace);
    assert(!observed.staging_directory_validated);
    assert(!observed.staging_directory_synchronized);
    assert(!observed.envelopes_directory_final_revalidation_matched);
    assert(!observed.attempts_directory_final_revalidation_matched);
    assert(!observed.staging_directory_final_revalidation_matched);
    assert(!observed.root_directory_synchronized);
    assert(!observed.directory_prefix_qualified);
    assert(observed.sealed_inputs.input_syscall_count != 0);
    assert(observed.sealed_inputs.secure_storage_wiped);
    assert(observed.sealed_inputs.secure_storage_unlocked);
    assert(observed.sealed_inputs.secure_storage_unmapped);
    assert(observed.sealed_inputs.signal_mask_restored);
    const pid_t child = ::fork();
    if (child < 0) {
        return 2;
    }
    if (child == 0) {
        ::execl("/proc/self/exe", "halofpx-l05x-synthetic-child",
                "--synthetic-directory-child", static_cast<char *>(nullptr));
        _exit(127);
    }
    int wait_status = 0;
    return bounded_wait(child, wait_status) && WIFEXITED(wait_status)
               ? WEXITSTATUS(wait_status) : 2;
}

int print_live_audit(const initialization_audit & observed, bool expect_prefix) {
    std::printf(
        "result=%u sealed_result=%u sealed_before_root=%u latch=%u qualified=%u "
        "root_syscalls=%u reserve=%llu root_id_nonzero=%u store_uuid_nonzero=%u "
        "writer_created=%u writer_synced=%u writer_ofd=%u sole_entry=%u "
        "writer_released=%u fixture_released=%u guard_released=%u "
        "envelopes=%u/%u/%u attempts=%u/%u/%u staging=%u/%u/%u "
        "final_dirs=%u/%u/%u root_synced=%u prefix_qualified=%u\n",
        static_cast<unsigned>(observed.result),
        static_cast<unsigned>(observed.sealed_inputs.result),
        static_cast<unsigned>(observed.sealed_inputs_preceded_root_access),
        static_cast<unsigned>(observed.discard_required_latched),
        static_cast<unsigned>(observed.lock_anchor_qualified),
        observed.root_fixture_syscall_count,
        static_cast<unsigned long long>(observed.observed_filesystem_reserve),
        static_cast<unsigned>(observed.generated_root_id_nonzero),
        static_cast<unsigned>(observed.generated_store_uuid_nonzero),
        static_cast<unsigned>(observed.writer_lock_created_no_replace),
        static_cast<unsigned>(observed.writer_lock_synchronized),
        static_cast<unsigned>(observed.writer_lock_ofd_acquired),
        static_cast<unsigned>(observed.writer_lock_root_sole_entry),
        static_cast<unsigned>(observed.writer_lock_released),
        static_cast<unsigned>(observed.fixture_lock_released),
        static_cast<unsigned>(observed.root_guard_released),
        static_cast<unsigned>(observed.envelopes_directory_created_no_replace),
        static_cast<unsigned>(observed.envelopes_directory_validated),
        static_cast<unsigned>(observed.envelopes_directory_synchronized),
        static_cast<unsigned>(observed.attempts_directory_created_no_replace),
        static_cast<unsigned>(observed.attempts_directory_validated),
        static_cast<unsigned>(observed.attempts_directory_synchronized),
        static_cast<unsigned>(observed.staging_directory_created_no_replace),
        static_cast<unsigned>(observed.staging_directory_validated),
        static_cast<unsigned>(observed.staging_directory_synchronized),
        static_cast<unsigned>(observed.envelopes_directory_final_revalidation_matched),
        static_cast<unsigned>(observed.attempts_directory_final_revalidation_matched),
        static_cast<unsigned>(observed.staging_directory_final_revalidation_matched),
        static_cast<unsigned>(observed.root_directory_synchronized),
        static_cast<unsigned>(observed.directory_prefix_qualified));
    const bool prefix_matched = expect_prefix
        ? observed.envelopes_directory_created_no_replace &&
              observed.envelopes_directory_validated &&
              observed.envelopes_directory_synchronized &&
              observed.attempts_directory_created_no_replace &&
              observed.attempts_directory_validated &&
              observed.attempts_directory_synchronized &&
              observed.staging_directory_created_no_replace &&
              observed.staging_directory_validated &&
              observed.staging_directory_synchronized &&
              observed.envelopes_directory_final_revalidation_matched &&
              observed.attempts_directory_final_revalidation_matched &&
              observed.staging_directory_final_revalidation_matched &&
              observed.root_directory_synchronized &&
              observed.directory_prefix_qualified
        : !observed.envelopes_directory_created_no_replace &&
              !observed.envelopes_directory_validated &&
              !observed.envelopes_directory_synchronized &&
              !observed.attempts_directory_created_no_replace &&
              !observed.attempts_directory_validated &&
              !observed.attempts_directory_synchronized &&
              !observed.staging_directory_created_no_replace &&
              !observed.staging_directory_validated &&
              !observed.staging_directory_synchronized &&
              !observed.envelopes_directory_final_revalidation_matched &&
              !observed.attempts_directory_final_revalidation_matched &&
              !observed.staging_directory_final_revalidation_matched &&
              !observed.root_directory_synchronized &&
              !observed.directory_prefix_qualified;
    const bool sealed_admitted =
        observed.sealed_inputs.result ==
            sealed_input_status::predecessor_authenticated_pins_matched_no_root_access &&
        observed.sealed_inputs_preceded_root_access &&
        observed.sealed_inputs.credential_transport_validated &&
        observed.sealed_inputs.predecessor_transport_validated &&
        observed.sealed_inputs.descriptor_identities_distinct &&
        observed.sealed_inputs.descriptor_aliases_absent &&
        observed.sealed_inputs.expected_key_tuple_matched &&
        observed.sealed_inputs.predecessor_digest_matched &&
        observed.sealed_inputs.predecessor_authenticated_under_supplied_credential &&
        observed.sealed_inputs.launcher_receipt_matched &&
        observed.sealed_inputs.transport_final_revalidation_matched &&
        observed.sealed_inputs.descriptors_closed &&
        observed.sealed_inputs.secure_storage_wiped &&
        observed.sealed_inputs.secure_storage_unlocked &&
        observed.sealed_inputs.secure_storage_unmapped &&
        observed.sealed_inputs.signal_mask_restored;
    const bool l05w_matched =
        observed.parent_identity_matched &&
        observed.candidate_root_identity_matched &&
        observed.fixture_identity_matched &&
        observed.fixture_lock_identity_matched &&
        observed.fixture_lock_acquired &&
        observed.root_guard_acquired &&
        observed.empty_root_final_revalidation_matched &&
        observed.generated_root_id_nonzero &&
        observed.generated_store_uuid_nonzero &&
        std::any_of(observed.generated_root_id.begin(),
                    observed.generated_root_id.end(),
                    [](std::uint8_t value) { return value != 0; }) &&
        std::any_of(observed.generated_store_uuid.begin(),
                    observed.generated_store_uuid.end(),
                    [](std::uint8_t value) { return value != 0; }) &&
        observed.generated_identity_scratch_wiped &&
        observed.discard_required_latched &&
        observed.writer_lock_created_no_replace &&
        observed.writer_lock_mode_revalidated &&
        observed.writer_lock_synchronized &&
        observed.writer_lock_ofd_acquired &&
        observed.writer_lock_root_sole_entry &&
        observed.lock_anchor_qualified &&
        observed.filesystem_not_reported_read_only &&
        observed.observed_filesystem_reserve >= initialization_required_filesystem_reserve;
    return observed.result == initialization_status::initialization_discard_required &&
                   sealed_admitted && l05w_matched &&
                   observed.writer_lock_released && observed.fixture_lock_released &&
                   observed.root_guard_released && prefix_matched
               ? 0 : 1;
}

bool read_live_request_handoff(initialization_request & output) {
    constexpr int request_fd = 5;
    struct stat value {};
    struct statfs filesystem {};
    std::array<char, 96> link {};
    constexpr char expected_link[] =
        "/memfd:halofpx-registry-lab-live-request (deleted)";
    constexpr int exact_seals =
        F_SEAL_SEAL | F_SEAL_SHRINK | F_SEAL_GROW | F_SEAL_WRITE;
    if (::fstat(request_fd, &value) != 0 || !S_ISREG(value.st_mode) ||
        value.st_nlink != 0 || value.st_size != static_cast<off_t>(sizeof(output)) ||
        ::fstatfs(request_fd, &filesystem) != 0 ||
        static_cast<unsigned long>(filesystem.f_type) != TMPFS_MAGIC ||
        ::fcntl(request_fd, F_GET_SEALS) != exact_seals ||
        ::readlink("/proc/self/fd/5", link.data(), link.size()) !=
            static_cast<ssize_t>(sizeof(expected_link) - 1) ||
        std::memcmp(link.data(), expected_link, sizeof(expected_link) - 1) != 0) {
        return false;
    }
    std::size_t total = 0;
    auto * bytes = reinterpret_cast<std::uint8_t *>(&output);
    while (total < sizeof(output)) {
        const ssize_t count = ::pread(
            request_fd, bytes + total, sizeof(output) - total,
            static_cast<off_t>(total));
        if (count < 0 && errno == EINTR) {
            continue;
        }
        if (count <= 0) {
            return false;
        }
        total += static_cast<std::size_t>(count);
    }
    std::uint8_t trailing = 0;
    ssize_t trailing_count;
    do {
        trailing_count = ::pread(
            request_fd, &trailing, 1, static_cast<off_t>(sizeof(output)));
    } while (trailing_count < 0 && errno == EINTR);
    return trailing_count == 0 && ::close(request_fd) == 0;
}

int run_live_child(bool directory_prefix) {
    initialization_request request {};
    if (!read_live_request_handoff(request)) {
        std::fprintf(stderr, "invalid bounded live-request handoff\n");
        return 2;
    }
    for (int fd : { 3, 4 }) {
        const int flags = ::fcntl(fd, F_GETFD);
        if (flags < 0 || ::fcntl(fd, F_SETFD, flags | FD_CLOEXEC) != 0) {
            std::fprintf(stderr, "unable to arm CLOEXEC on sealed input fd %d\n", fd);
            return 2;
        }
    }
    if (directory_prefix) {
        // Force mkdir(0700) to produce mode 0000 and prove the initializer's
        // fd-bound repair is independent of the launcher's global umask.
        (void) ::umask(0777);
    }
    const auto observed = directory_prefix
        ? initialize_directory_prefix_once(request)
        : initialize_writer_lock_anchor_once(request);
    return print_live_audit(observed, directory_prefix);
}

std::uint64_t monotonic_nanoseconds() {
    struct timespec value {};
    if (::clock_gettime(CLOCK_MONOTONIC, &value) != 0) {
        return 0;
    }
    return static_cast<std::uint64_t>(value.tv_sec) * 1000000000ULL +
           static_cast<std::uint64_t>(value.tv_nsec);
}

bool bounded_wait(pid_t child, int & wait_status) {
    constexpr std::uint64_t timeout = 10ULL * 1000ULL * 1000ULL * 1000ULL;
    const auto terminate_and_reap = [&]() {
        (void) ::kill(child, SIGKILL);
        constexpr std::uint64_t reap_timeout = 2ULL * 1000ULL * 1000ULL * 1000ULL;
        const std::uint64_t reap_start = monotonic_nanoseconds();
        if (reap_start == 0 || reap_start > UINT64_MAX - reap_timeout) {
            (void) ::waitpid(child, &wait_status, WNOHANG);
            return false;
        }
        const std::uint64_t reap_deadline = reap_start + reap_timeout;
        for (;;) {
            const pid_t reaped = ::waitpid(child, &wait_status, WNOHANG);
            if (reaped == child || (reaped < 0 && errno == ECHILD)) {
                return true;
            }
            if (reaped < 0 && errno != EINTR) {
                return false;
            }
            const std::uint64_t now = monotonic_nanoseconds();
            if (now == 0 || now >= reap_deadline) {
                return false;
            }
            const std::uint64_t wake_ns =
                now + 10ULL * 1000ULL * 1000ULL < reap_deadline
                    ? now + 10ULL * 1000ULL * 1000ULL : reap_deadline;
            const struct timespec wake {
                static_cast<time_t>(wake_ns / 1000000000ULL),
                static_cast<long>(wake_ns % 1000000000ULL),
            };
            int sleep_result;
            do {
                sleep_result = ::clock_nanosleep(
                    CLOCK_MONOTONIC, TIMER_ABSTIME, &wake, nullptr);
            } while (sleep_result == EINTR);
            if (sleep_result != 0) {
                return false;
            }
        }
    };
    const std::uint64_t start = monotonic_nanoseconds();
    if (start == 0 || start > UINT64_MAX - timeout) {
        terminate_and_reap();
        return false;
    }
    const std::uint64_t deadline = start + timeout;
    for (;;) {
        const pid_t waited = ::waitpid(child, &wait_status, WNOHANG);
        if (waited == child) {
            return true;
        }
        if (waited < 0 && errno != EINTR) {
            terminate_and_reap();
            return false;
        }
        const std::uint64_t now = monotonic_nanoseconds();
        if (now == 0 || now >= deadline) {
            terminate_and_reap();
            return false;
        }
        const std::uint64_t wake_ns =
            now + 10ULL * 1000ULL * 1000ULL < deadline
                ? now + 10ULL * 1000ULL * 1000ULL : deadline;
        const struct timespec wake {
            static_cast<time_t>(wake_ns / 1000000000ULL),
            static_cast<long>(wake_ns % 1000000000ULL),
        };
        int sleep_result;
        do {
            sleep_result = ::clock_nanosleep(
                CLOCK_MONOTONIC, TIMER_ABSTIME, &wake, nullptr);
        } while (sleep_result == EINTR);
        if (sleep_result != 0) {
            terminate_and_reap();
            return false;
        }
    }
}

bool release_gate_without_sigpipe(int fd, std::uint8_t value) {
    sigset_t blocked {};
    sigset_t previous {};
    sigset_t pending {};
    if (::sigemptyset(&blocked) != 0 || ::sigaddset(&blocked, SIGPIPE) != 0) {
        return false;
    }
    if (::sigprocmask(SIG_BLOCK, &blocked, &previous) != 0) {
        return false;
    }
    if (::sigpending(&pending) != 0) {
        (void) ::sigprocmask(SIG_SETMASK, &previous, nullptr);
        return false;
    }
    const int pending_result = ::sigismember(&pending, SIGPIPE);
    if (pending_result < 0) {
        (void) ::sigprocmask(SIG_SETMASK, &previous, nullptr);
        return false;
    }
    const bool was_pending = pending_result == 1;
    ssize_t count;
    do {
        count = ::write(fd, &value, 1);
    } while (count < 0 && errno == EINTR);
    if (count < 0 && errno == EPIPE && !was_pending) {
        const struct timespec no_wait { 0, 0 };
        int consumed;
        do {
            consumed = ::sigtimedwait(&blocked, nullptr, &no_wait);
        } while (consumed < 0 && errno == EINTR);
        if (consumed != SIGPIPE) {
            // Keep SIGPIPE blocked if the newly generated signal could not be
            // consumed; unblocking it could terminate the controller before
            // it emits a controlled failure receipt.
            return false;
        }
    }
    const bool restored = ::sigprocmask(SIG_SETMASK, &previous, nullptr) == 0;
    return count == 1 && restored;
}

int run_live_controller(const char * golden_path, const char * parent_path,
                        const char * root_path, const char * fixture_path,
                        bool directory_prefix) {
    std::string json;
    {
        std::ifstream input(golden_path, std::ios::binary);
        if (!input) {
            std::fprintf(stderr, "unable to open golden vector: %s\n", golden_path);
            return 2;
        }
        json.assign(std::istreambuf_iterator<char>(input), {});
    }
    initialization_request request {};
    std::vector<std::uint8_t> package;
    std::vector<std::uint8_t> predecessor;
    if (!make_sealed_request(json, package, predecessor, request.sealed_inputs)) {
        std::fprintf(stderr, "golden vector does not match the admitted L05o/L05v fixture\n");
        return 2;
    }
    if (!derive_path_identity(parent_path, request.parent) ||
        !derive_path_identity(root_path, request.candidate_root) ||
        !derive_path_identity(fixture_path, request.fixture) ||
        !derive_fixture_lock(fixture_path, request.fixture,
                             request.fixture_lock_device,
                             request.fixture_lock_inode)) {
        std::fprintf(stderr, "live path identity derivation failed\n");
        return 2;
    }
    (void) ::close(3);
    (void) ::close(4);
    (void) ::close(5);
    std::vector<std::uint8_t> request_bytes(sizeof(request));
    std::memcpy(request_bytes.data(), &request, sizeof(request));
    if (!install_memfd("halofpx-registry-lab-credential", package, 3, false) ||
        !install_memfd("halofpx-registry-lab-predecessor", predecessor, 4, false) ||
        !install_memfd("halofpx-registry-lab-live-request", request_bytes, 5, false)) {
        std::fprintf(stderr, "sealed live input installation failed\n");
        return 2;
    }
    int gate[2] { -1, -1 };
    if (::pipe2(gate, O_CLOEXEC) != 0) {
        std::fprintf(stderr, "unable to create controller relinquishment gate\n");
        return 2;
    }
    if (::syscall(SYS_close_range, 6U, ~0U, CLOSE_RANGE_CLOEXEC) != 0) {
        std::fprintf(stderr, "unable to bound inherited live-child descriptors\n");
        return 2;
    }
    const pid_t child = ::fork();
    if (child < 0) {
        std::fprintf(stderr, "unable to fork live child\n");
        return 2;
    }
    if (child == 0) {
        ::close(gate[1]);
        std::uint8_t released = 0;
        ssize_t count;
        do {
            count = ::read(gate[0], &released, 1);
        } while (count < 0 && errno == EINTR);
        ::close(gate[0]);
        if (count != 1 || released != 0xa5) {
            _exit(126);
        }
        const char * child_mode = directory_prefix
            ? "--live-directory-child" : "--live-child";
        const char * child_name = directory_prefix
            ? "halofpx-l05x-live-child" : "halofpx-l05w-live-child";
        ::execl("/proc/self/exe", child_name, child_mode,
                static_cast<char *>(nullptr));
        _exit(127);
    }
    ::close(gate[0]);
    const bool credential_closed = ::close(3) == 0;
    const bool predecessor_closed = ::close(4) == 0;
    const bool request_closed = ::close(5) == 0;
    const bool relinquished =
        credential_closed && predecessor_closed && request_closed;
    const std::uint8_t released = 0xa5;
    const bool signaled = relinquished &&
        release_gate_without_sigpipe(gate[1], released);
    ::close(gate[1]);
    int wait_status = 0;
    if (!bounded_wait(child, wait_status) || !signaled ||
        !WIFEXITED(wait_status)) {
        return 2;
    }
    const int child_status = WEXITSTATUS(wait_status);
    if (child_status != 0) {
        return child_status;
    }
    if (directory_prefix && !inspect_final_directory_prefix(root_path, request)) {
        std::fprintf(stderr, "independent L05x final-layout inspection failed\n");
        return 2;
    }
    return 0;
}

} // namespace

int main(int argc, char ** argv) {
    if (argc == 1) {
        return run_synthetic();
    }
    if (argc == 6 && std::strcmp(argv[1], "--live-controller") == 0) {
        return run_live_controller(argv[2], argv[3], argv[4], argv[5], false);
    }
    if (argc == 6 && std::strcmp(argv[1], "--live-directory-controller") == 0) {
        return run_live_controller(argv[2], argv[3], argv[4], argv[5], true);
    }
    if (argc == 2 && std::strcmp(argv[1], "--live-child") == 0) {
        return run_live_child(false);
    }
    if (argc == 2 && std::strcmp(argv[1], "--live-directory-child") == 0) {
        return run_live_child(true);
    }
    if (argc == 2 && std::strcmp(argv[1], "--synthetic-directory-child") == 0) {
        return run_directory_synthetic();
    }
    std::fprintf(stderr,
        "usage: %s [--live-controller|--live-directory-controller "
        "golden.json canonical-parent "
        "canonical-root canonical-fixture]\n",
        argv[0]);
    return 2;
}
