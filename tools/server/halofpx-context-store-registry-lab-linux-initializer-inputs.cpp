#include "halofpx-context-store-registry-lab-linux-initializer-internal.h"
#include "halofpx-context-store-protected-registry.h"

#if !defined(__linux__)
#error "The HaloFPX registry-lab initializer input transport is Linux-only"
#endif

#include <algorithm>
#include <array>
#include <atomic>
#include <cerrno>
#include <climits>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <new>

#include <fcntl.h>
#include <linux/magic.h>
#include <linux/memfd.h>
#include <linux/sched.h>
#include <signal.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/statfs.h>
#include <sys/syscall.h>
#include <unistd.h>

namespace halofpx::registry_lab::linux_initializer {
namespace {

constexpr int credential_fd = 3;
constexpr int predecessor_fd = 4;
constexpr std::size_t credential_capacity = context_store_registry_lab_credential_max_bytes;
constexpr std::size_t predecessor_capacity = 1024;
constexpr std::size_t credential_minimum = 16 + 2 + 1 + 8 + 2 + 32;
constexpr std::size_t max_fd_scan_entries = 4096;
constexpr std::array<std::uint8_t, 16> credential_magic = {
    'H', 'a', 'l', 'o', 'F', 'P', 'X', 'R', 'e', 'g', 'K', 'e', 'y', '0', '1', 0,
};
constexpr char credential_link[] = "/memfd:halofpx-registry-lab-credential (deleted)";
constexpr char predecessor_link[] = "/memfd:halofpx-registry-lab-predecessor (deleted)";
constexpr int exact_seals = F_SEAL_SEAL | F_SEAL_SHRINK | F_SEAL_GROW | F_SEAL_WRITE;

std::atomic_flag session_consumed = ATOMIC_FLAG_INIT;

struct linux_dirent64 {
    std::uint64_t d_ino;
    std::int64_t d_off;
    unsigned short d_reclen;
    unsigned char d_type;
    char d_name[1];
};

struct descriptor_identity {
    dev_t device = 0;
    ino_t inode = 0;
};

struct alignas(64) secure_inputs {
    std::array<std::uint8_t, credential_capacity> credential {};
    std::array<std::uint8_t, predecessor_capacity> predecessor {};
    std::array<std::uint8_t, 32> secret {};
    context_store_protected_registry_facts predecessor_facts {};
};

struct execution_context {
    sigset_t previous_mask {};
    bool signal_mask_changed = false;
    bool fd_table_unshared = false;
    bool exclusive = false;
};

void wipe(void * data, std::size_t size) noexcept {
    auto * bytes = static_cast<volatile std::uint8_t *>(data);
    while (size-- != 0) {
        *bytes++ = 0;
    }
}

bool all_zero(const void * data, std::size_t size) noexcept {
    const auto * bytes = static_cast<const std::uint8_t *>(data);
    std::uint8_t aggregate = 0;
    for (std::size_t i = 0; i < size; ++i) {
        aggregate |= bytes[i];
    }
    return aggregate == 0;
}

bool nonzero(const std::uint8_t * data, std::size_t size) noexcept {
    return !all_zero(data, size);
}

bool same_digest(const context_store_format_digest & lhs,
                 const context_store_format_digest & rhs) noexcept {
    std::uint8_t difference = 0;
    for (std::size_t i = 0; i < lhs.size(); ++i) {
        difference |= lhs[i] ^ rhs[i];
    }
    return difference == 0;
}

bool same_id(const context_store_registered_id & lhs,
             const context_store_registered_id & rhs) noexcept {
    if (lhs.size != rhs.size) {
        return false;
    }
    std::uint8_t difference = 0;
    for (std::size_t i = 0; i < lhs.size; ++i) {
        difference |= static_cast<std::uint8_t>(lhs.bytes[i] ^ rhs.bytes[i]);
    }
    return difference == 0;
}

bool successful(sealed_input_status value) noexcept {
    return value == sealed_input_status::transport_validated_no_root_access ||
           value == sealed_input_status::predecessor_authenticated_pins_matched_no_root_access;
}

bool registered_id_valid(const context_store_registered_id & value) noexcept {
    if (value.size == 0 || value.size > value.bytes.size()) {
        return false;
    }
    for (std::size_t i = 0; i < value.size; ++i) {
        if (value.bytes[i] < 0x21 || value.bytes[i] > 0x7e) {
            return false;
        }
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

bool close_owned_descriptor(int fd, sealed_input_audit & output) noexcept {
    ++output.input_syscall_count;
    if (::close(fd) == 0 || errno == EBADF) {
        return true;
    }
    return false;
}

sealed_input_status establish_exclusive_context(
        execution_context & context, sealed_input_audit & output) noexcept {
    sigset_t blocked {};
    if (::sigfillset(&blocked) != 0 ||
        ::sigprocmask(SIG_SETMASK, &blocked, &context.previous_mask) != 0) {
        return sealed_input_status::io_failure_no_mutation;
    }
    context.signal_mask_changed = true;

    ++output.input_syscall_count;
    if (::syscall(SYS_unshare, CLONE_FILES) != 0) {
        return errno == ENOSYS || errno == EPERM
            ? sealed_input_status::unsupported_no_mutation
            : sealed_input_status::io_failure_no_mutation;
    }
    context.fd_table_unshared = true;
    output.fd_table_unshared = true;

    ++output.input_syscall_count;
    int task_fd = ::open("/proc/self/task", O_RDONLY | O_DIRECTORY | O_CLOEXEC);
    if (task_fd < 0) {
        return sealed_input_status::io_failure_no_mutation;
    }
    if (task_fd == credential_fd || task_fd == predecessor_fd) {
        ++output.input_syscall_count;
        const int moved = ::fcntl(task_fd, F_DUPFD_CLOEXEC, 5);
        if (moved < 0) {
            ::close(task_fd);
            return sealed_input_status::io_failure_no_mutation;
        }
        if (::close(task_fd) != 0) {
            ::close(moved);
            return sealed_input_status::io_failure_no_mutation;
        }
        task_fd = moved;
    }

    std::array<char, 4096> bytes {};
    std::size_t tasks = 0;
    sealed_input_status result = sealed_input_status::transport_validated_no_root_access;
    for (;;) {
        ++output.input_syscall_count;
        const long count = ::syscall(SYS_getdents64, task_fd, bytes.data(), bytes.size());
        if (count < 0) {
            if (errno == EINTR) {
                continue;
            }
            result = sealed_input_status::io_failure_no_mutation;
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
                result = sealed_input_status::io_failure_no_mutation;
                break;
            }
            offset += entry->d_reclen;
            char * end = nullptr;
            errno = 0;
            const long task = std::strtol(entry->d_name, &end, 10);
            if (errno == 0 && end != entry->d_name && *end == '\0' && task > 0) {
                ++tasks;
                if (tasks > 1) {
                    result = sealed_input_status::unsupported_no_mutation;
                    break;
                }
            }
        }
        if (result != sealed_input_status::transport_validated_no_root_access) {
            break;
        }
    }
    if (::close(task_fd) != 0 &&
        result == sealed_input_status::transport_validated_no_root_access) {
        result = sealed_input_status::io_failure_no_mutation;
    }
    if (result == sealed_input_status::transport_validated_no_root_access && tasks != 1) {
        result = sealed_input_status::unsupported_no_mutation;
    }
    if (result == sealed_input_status::transport_validated_no_root_access) {
        context.exclusive = true;
        output.exclusive_execution_context = true;
    }
    return result;
}

sealed_input_status inspect_descriptor(
        int fd, const char * proc_path, const char * expected_link,
        std::size_t minimum, std::size_t maximum,
        descriptor_identity & identity, std::size_t & size,
        sealed_input_audit & output) noexcept {
    ++output.input_syscall_count;
    const int flags = ::fcntl(fd, F_GETFD);
    if (flags < 0) {
        return errno == EBADF ? sealed_input_status::invalid_request_no_mutation
                              : sealed_input_status::io_failure_no_mutation;
    }
    if ((flags & FD_CLOEXEC) == 0) {
        return sealed_input_status::invalid_request_no_mutation;
    }

    struct stat value {};
    ++output.input_syscall_count;
    if (::fstat(fd, &value) != 0) {
        return sealed_input_status::io_failure_no_mutation;
    }
    if (!S_ISREG(value.st_mode) || value.st_nlink != 0 ||
        value.st_size < static_cast<off_t>(minimum) ||
        value.st_size > static_cast<off_t>(maximum)) {
        return sealed_input_status::invalid_request_no_mutation;
    }

    struct statfs filesystem {};
    ++output.input_syscall_count;
    if (::fstatfs(fd, &filesystem) != 0) {
        return sealed_input_status::io_failure_no_mutation;
    }
    if (static_cast<unsigned long>(filesystem.f_type) != TMPFS_MAGIC) {
        return sealed_input_status::invalid_request_no_mutation;
    }

    std::array<char, 96> link {};
    ++output.input_syscall_count;
    const ssize_t link_size = ::readlink(proc_path, link.data(), link.size());
    const std::size_t expected_size = std::strlen(expected_link);
    if (link_size != static_cast<ssize_t>(expected_size) ||
        std::memcmp(link.data(), expected_link, expected_size) != 0) {
        return sealed_input_status::invalid_request_no_mutation;
    }

    ++output.input_syscall_count;
    const int seals = ::fcntl(fd, F_GET_SEALS);
    if (seals < 0) {
        return errno == EINVAL ? sealed_input_status::unsupported_no_mutation
                               : sealed_input_status::io_failure_no_mutation;
    }
    if (seals != exact_seals) {
        return sealed_input_status::invalid_request_no_mutation;
    }

    identity.device = value.st_dev;
    identity.inode = value.st_ino;
    size = static_cast<std::size_t>(value.st_size);
    return sealed_input_status::transport_validated_no_root_access;
}

sealed_input_status scan_aliases(
        const descriptor_identity & credential,
        const descriptor_identity & predecessor,
        sealed_input_audit & output) noexcept {
    ++output.input_syscall_count;
    const int directory_fd = ::open("/proc/self/fd", O_RDONLY | O_DIRECTORY | O_CLOEXEC);
    if (directory_fd < 0) {
        return sealed_input_status::io_failure_no_mutation;
    }

    sealed_input_status result = sealed_input_status::transport_validated_no_root_access;
    std::array<char, 4096> bytes {};
    std::size_t entries = 0;
    for (;;) {
        ++output.input_syscall_count;
        const long count = ::syscall(SYS_getdents64, directory_fd, bytes.data(), bytes.size());
        if (count < 0) {
            if (errno == EINTR) {
                continue;
            }
            result = sealed_input_status::io_failure_no_mutation;
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
                result = sealed_input_status::io_failure_no_mutation;
                break;
            }
            offset += entry->d_reclen;
            if (++entries > max_fd_scan_entries) {
                result = sealed_input_status::invalid_request_no_mutation;
                break;
            }
            char * end = nullptr;
            errno = 0;
            const long candidate = std::strtol(entry->d_name, &end, 10);
            if (errno != 0 || end == entry->d_name || *end != '\0' || candidate < 0 ||
                candidate > INT_MAX || candidate == credential_fd ||
                candidate == predecessor_fd || candidate == directory_fd) {
                continue;
            }
            struct stat value {};
            ++output.input_syscall_count;
            if (::fstat(static_cast<int>(candidate), &value) != 0) {
                if (errno == EBADF) {
                    continue;
                }
                result = sealed_input_status::io_failure_no_mutation;
                break;
            }
            const bool credential_alias = value.st_dev == credential.device &&
                                          value.st_ino == credential.inode;
            const bool predecessor_alias = value.st_dev == predecessor.device &&
                                           value.st_ino == predecessor.inode;
            if (credential_alias || predecessor_alias) {
                result = sealed_input_status::invalid_request_no_mutation;
                break;
            }
        }
        if (result != sealed_input_status::transport_validated_no_root_access) {
            break;
        }
    }
    if (::close(directory_fd) != 0 &&
        result == sealed_input_status::transport_validated_no_root_access) {
        result = sealed_input_status::io_failure_no_mutation;
    }
    return result;
}

sealed_input_status read_exact(int fd, std::uint8_t * output_bytes, std::size_t size,
                               sealed_input_audit & output) noexcept {
    std::size_t total = 0;
    while (total < size) {
        ++output.input_syscall_count;
        const ssize_t count = ::pread(fd, output_bytes + total, size - total,
                                      static_cast<off_t>(total));
        if (count < 0) {
            if (errno == EINTR) {
                continue;
            }
            return sealed_input_status::io_failure_no_mutation;
        }
        if (count == 0) {
            return sealed_input_status::io_failure_no_mutation;
        }
        total += static_cast<std::size_t>(count);
    }
    std::uint8_t trailing = 0;
    ssize_t count = 0;
    do {
        ++output.input_syscall_count;
        count = ::pread(fd, &trailing, 1, static_cast<off_t>(size));
    } while (count < 0 && errno == EINTR);
    if (count < 0) {
        return sealed_input_status::io_failure_no_mutation;
    }
    return count == 0 ? sealed_input_status::transport_validated_no_root_access
                      : sealed_input_status::invalid_request_no_mutation;
}

sealed_input_status parse_credential_shape(
        const sealed_input_request & input, secure_inputs & secure,
        std::size_t size, sealed_input_audit & output) noexcept {
    if (std::memcmp(secure.credential.data(), credential_magic.data(),
                    credential_magic.size()) != 0) {
        return sealed_input_status::invalid_request_no_mutation;
    }
    std::size_t offset = credential_magic.size();
    const std::size_t key_size = read_u16_be(secure.credential.data() + offset);
    offset += 2;
    if (key_size == 0 || key_size > input.expected_key_id.bytes.size() ||
        size != credential_magic.size() + 2 + key_size + 8 + 2 + secure.secret.size()) {
        return sealed_input_status::invalid_request_no_mutation;
    }
    for (std::size_t i = 0; i < key_size; ++i) {
        if (secure.credential[offset + i] < 0x21 || secure.credential[offset + i] > 0x7e) {
            return sealed_input_status::invalid_request_no_mutation;
        }
    }
    const bool key_matches = key_size == input.expected_key_id.size &&
        std::memcmp(secure.credential.data() + offset,
                    input.expected_key_id.bytes.data(), key_size) == 0;
    offset += key_size;
    const std::uint64_t generation = read_u64_be(secure.credential.data() + offset);
    offset += 8;
    if (generation == 0 || read_u16_be(secure.credential.data() + offset) != 32) {
        return sealed_input_status::invalid_request_no_mutation;
    }
    offset += 2;
    std::copy_n(secure.credential.data() + offset, secure.secret.size(), secure.secret.begin());
    if (!nonzero(secure.secret.data(), secure.secret.size())) {
        return sealed_input_status::invalid_request_no_mutation;
    }
    output.expected_key_tuple_matched = key_matches &&
                                        generation == input.expected_key_generation;
    return output.expected_key_tuple_matched
        ? sealed_input_status::transport_validated_no_root_access
        : sealed_input_status::invalid_request_no_mutation;
}

sealed_input_audit finish(sealed_input_audit output, void * mapping, std::size_t mapping_size,
                          bool storage_locked, execution_context & context,
                          sealed_input_status result) noexcept {
    bool credential_closed = false;
    bool predecessor_closed = false;
    if (context.fd_table_unshared) {
        credential_closed = close_owned_descriptor(credential_fd, output);
        predecessor_closed = close_owned_descriptor(predecessor_fd, output);
    }
    output.descriptors_closed = context.fd_table_unshared &&
                                credential_closed && predecessor_closed;
    if (!output.descriptors_closed && successful(result)) {
        result = sealed_input_status::io_failure_no_mutation;
    }

    if (mapping != MAP_FAILED && mapping != nullptr && mapping_size != 0) {
        wipe(mapping, mapping_size);
        output.secure_storage_wiped = all_zero(mapping, mapping_size);
    } else {
        output.secure_storage_wiped = true;
    }
    if (storage_locked && mapping != MAP_FAILED && mapping != nullptr) {
        output.secure_storage_unlocked = ::munlock(mapping, mapping_size) == 0;
    } else {
        output.secure_storage_unlocked = true;
    }
    if ((!output.secure_storage_wiped || !output.secure_storage_unlocked) &&
        successful(result)) {
        result = sealed_input_status::io_failure_no_mutation;
    }
    if (mapping != MAP_FAILED && mapping != nullptr && mapping_size != 0) {
        output.secure_storage_unmapped = ::munmap(mapping, mapping_size) == 0;
    } else {
        output.secure_storage_unmapped = true;
    }
    if (!output.secure_storage_unmapped && successful(result)) {
        result = sealed_input_status::io_failure_no_mutation;
    }
    if (context.signal_mask_changed) {
        output.signal_mask_restored =
            ::sigprocmask(SIG_SETMASK, &context.previous_mask, nullptr) == 0;
    } else {
        output.signal_mask_restored = true;
    }
    if (!output.signal_mask_restored && successful(result)) {
        result = sealed_input_status::io_failure_no_mutation;
    }
    output.root_or_fixture_accessed = output.root_or_fixture_syscall_count != 0;
    if (output.root_or_fixture_accessed) {
        result = sealed_input_status::io_failure_no_mutation;
    }
    if (result !=
        sealed_input_status::predecessor_authenticated_pins_matched_no_root_access) {
        output.predecessor_authenticated_under_supplied_credential = false;
        output.launcher_receipt_matched = false;
    }
    output.result = result;
    return output;
}

} // namespace

namespace {

// File-private, non-copyable authority for the one initializer invocation.
// No declaration escapes this translation unit and no general callback seam is
// admitted. The locked mapping remains live until the root helper finishes.
struct authenticated_input_session {
    secure_inputs * secure = nullptr;
    void * mapping = MAP_FAILED;
    std::size_t mapping_size = 0;
    bool storage_locked = false;
    execution_context context {};
    bool authenticated = false;

    authenticated_input_session() = default;
    authenticated_input_session(const authenticated_input_session &) = delete;
    authenticated_input_session & operator=(const authenticated_input_session &) = delete;
};

sealed_input_status authenticate_sealed_inputs_for_session(
        const sealed_input_request & input, authenticated_input_session & session,
        sealed_input_audit & output) noexcept {
    auto fail = [&](sealed_input_status value) noexcept {
        output = finish(output, session.mapping, session.mapping_size,
                        session.storage_locked, session.context, value);
        session.secure = nullptr;
        session.mapping = MAP_FAILED;
        session.mapping_size = 0;
        session.storage_locked = false;
        session.context.signal_mask_changed = false;
        return output.result;
    };

    if (session_consumed.test_and_set(std::memory_order_acq_rel)) {
        output.secure_storage_wiped = true;
        output.secure_storage_unlocked = true;
        output.secure_storage_unmapped = true;
        output.signal_mask_restored = true;
        return output.result;
    }

    auto result = establish_exclusive_context(session.context, output);
    if (result != sealed_input_status::transport_validated_no_root_access) {
        return fail(result);
    }
    if (!registered_id_valid(input.expected_key_id) ||
        input.expected_key_generation == 0 ||
        !nonzero(input.expected_predecessor_digest.data(),
                 input.expected_predecessor_digest.size()) ||
        !registered_id_valid(input.expected_registry_id) ||
        input.expected_registry_epoch == 0 ||
        !nonzero(input.expected_authority_base_scope_commitment.data(),
                 input.expected_authority_base_scope_commitment.size()) ||
        !nonzero(input.expected_policy_commitment.data(),
                 input.expected_policy_commitment.size()) ||
        input.expected_predecessor_high_water == UINT64_MAX ||
        !nonzero(input.expected_key_continuity_commitment.data(),
                 input.expected_key_continuity_commitment.size())) {
        return fail(sealed_input_status::invalid_request_no_mutation);
    }

    ++output.input_syscall_count;
    const long page_size = ::sysconf(_SC_PAGESIZE);
    if (page_size < 4096 || page_size > 65536 ||
        (page_size & (page_size - 1)) != 0) {
        return fail(sealed_input_status::unsupported_no_mutation);
    }
    session.mapping_size =
        (sizeof(secure_inputs) + static_cast<std::size_t>(page_size) - 1) &
        ~(static_cast<std::size_t>(page_size) - 1);
    session.mapping = ::mmap(nullptr, session.mapping_size, PROT_READ | PROT_WRITE,
                             MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (session.mapping == MAP_FAILED) {
        return fail(sealed_input_status::unavailable_no_mutation);
    }
    session.secure = ::new (session.mapping) secure_inputs {};
    if (::mlock(session.mapping, session.mapping_size) != 0) {
        return fail(sealed_input_status::unsupported_no_mutation);
    }
    session.storage_locked = true;
    output.secure_storage_locked = true;

    descriptor_identity credential_identity {};
    descriptor_identity predecessor_identity {};
    std::size_t credential_size = 0;
    std::size_t predecessor_size = 0;
    result = inspect_descriptor(
        credential_fd, "/proc/self/fd/3", credential_link,
        credential_minimum, credential_capacity,
        credential_identity, credential_size, output);
    if (result != sealed_input_status::transport_validated_no_root_access) {
        return fail(result);
    }
    output.credential_transport_validated = true;
    result = inspect_descriptor(
        predecessor_fd, "/proc/self/fd/4", predecessor_link,
        1, predecessor_capacity,
        predecessor_identity, predecessor_size, output);
    if (result != sealed_input_status::transport_validated_no_root_access) {
        return fail(result);
    }
    output.predecessor_transport_validated = true;
    output.descriptor_identities_distinct =
        credential_identity.device != predecessor_identity.device ||
        credential_identity.inode != predecessor_identity.inode;
    if (!output.descriptor_identities_distinct) {
        return fail(sealed_input_status::invalid_request_no_mutation);
    }
    result = scan_aliases(credential_identity, predecessor_identity, output);
    if (result != sealed_input_status::transport_validated_no_root_access) {
        return fail(result);
    }
    output.descriptor_aliases_absent = true;
    result = read_exact(credential_fd, session.secure->credential.data(),
                        credential_size, output);
    if (result != sealed_input_status::transport_validated_no_root_access) {
        return fail(result);
    }
    result = read_exact(predecessor_fd, session.secure->predecessor.data(),
                        predecessor_size, output);
    if (result != sealed_input_status::transport_validated_no_root_access) {
        return fail(result);
    }
    output.credential_package_size = static_cast<std::uint32_t>(credential_size);
    output.predecessor_envelope_size = static_cast<std::uint32_t>(predecessor_size);
    result = parse_credential_shape(input, *session.secure, credential_size, output);
    if (result != sealed_input_status::transport_validated_no_root_access) {
        return fail(result);
    }

    context_store_format_digest observed_digest {};
    if (!context_store_registry_lab_linux_initializer_predecessor_digest_v1(
            session.secure->predecessor.data(), predecessor_size, observed_digest)) {
        wipe(observed_digest.data(), observed_digest.size());
        return fail(sealed_input_status::invalid_request_no_mutation);
    }
    output.predecessor_digest_matched =
        same_digest(observed_digest, input.expected_predecessor_digest);
    wipe(observed_digest.data(), observed_digest.size());
    if (!output.predecessor_digest_matched) {
        return fail(sealed_input_status::invalid_request_no_mutation);
    }

    context_store_protected_registry_key_record authentication_key;
    authentication_key.disposition = context_store_key_disposition::active;
    authentication_key.key_id = input.expected_key_id;
    authentication_key.generation = input.expected_key_generation;
    authentication_key.master_key = {
        session.secure->secret.data(), session.secure->secret.size(),
    };
    const auto authentication_result = context_store_verify_protected_registry_facts_v1(
        session.secure->predecessor.data(), predecessor_size, authentication_key);
    authentication_key.master_key = {};
    const auto * authenticated_facts = authentication_result.authenticated_facts();
    if (!authenticated_facts) {
        return fail(sealed_input_status::invalid_request_no_mutation);
    }
    session.secure->predecessor_facts = *authenticated_facts;
    const bool launcher_receipt_matched =
        same_id(session.secure->predecessor_facts.body.registry_id,
                input.expected_registry_id) &&
        session.secure->predecessor_facts.body.registry_epoch ==
            input.expected_registry_epoch &&
        same_digest(session.secure->predecessor_facts.body.authority_base_scope_commitment,
                    input.expected_authority_base_scope_commitment) &&
        same_digest(session.secure->predecessor_facts.body.policy_commitment,
                    input.expected_policy_commitment) &&
        session.secure->predecessor_facts.body.last_consumed_sequence ==
            input.expected_predecessor_high_water &&
        same_digest(session.secure->predecessor_facts.key_continuity_commitment,
                    input.expected_key_continuity_commitment) &&
        output.expected_key_tuple_matched && output.predecessor_digest_matched &&
        same_id(session.secure->predecessor_facts.key_id, input.expected_key_id) &&
        session.secure->predecessor_facts.key_generation ==
            input.expected_key_generation;
    if (!launcher_receipt_matched) {
        return fail(sealed_input_status::invalid_request_no_mutation);
    }
    output.predecessor_authenticated_under_supplied_credential = true;
    output.launcher_receipt_matched = true;

    descriptor_identity final_credential_identity {};
    descriptor_identity final_predecessor_identity {};
    std::size_t final_credential_size = 0;
    std::size_t final_predecessor_size = 0;
    result = inspect_descriptor(
        credential_fd, "/proc/self/fd/3", credential_link,
        credential_minimum, credential_capacity,
        final_credential_identity, final_credential_size, output);
    if (result == sealed_input_status::transport_validated_no_root_access) {
        result = inspect_descriptor(
            predecessor_fd, "/proc/self/fd/4", predecessor_link,
            1, predecessor_capacity,
            final_predecessor_identity, final_predecessor_size, output);
    }
    if (result != sealed_input_status::transport_validated_no_root_access ||
        final_credential_identity.device != credential_identity.device ||
        final_credential_identity.inode != credential_identity.inode ||
        final_predecessor_identity.device != predecessor_identity.device ||
        final_predecessor_identity.inode != predecessor_identity.inode ||
        final_credential_size != credential_size ||
        final_predecessor_size != predecessor_size) {
        return fail(result == sealed_input_status::transport_validated_no_root_access
                        ? sealed_input_status::invalid_request_no_mutation
                        : result);
    }
    result = scan_aliases(final_credential_identity, final_predecessor_identity, output);
    if (result != sealed_input_status::transport_validated_no_root_access) {
        return fail(result);
    }
    output.transport_final_revalidation_matched = true;

    const bool credential_closed = close_owned_descriptor(credential_fd, output);
    const bool predecessor_closed = close_owned_descriptor(predecessor_fd, output);
    output.descriptors_closed = credential_closed && predecessor_closed;
    if (!output.descriptors_closed) {
        return fail(sealed_input_status::io_failure_no_mutation);
    }
    session.authenticated = true;
    output.result = sealed_input_status::predecessor_authenticated_pins_matched_no_root_access;
    return output.result;
}

bool cleanup_authenticated_input_storage(authenticated_input_session & session,
                                         sealed_input_audit & output) noexcept {
    bool ok = true;
    if (session.mapping != MAP_FAILED && session.mapping != nullptr &&
        session.mapping_size != 0) {
        wipe(session.mapping, session.mapping_size);
        output.secure_storage_wiped = all_zero(session.mapping, session.mapping_size);
        ok = output.secure_storage_wiped;
    } else {
        output.secure_storage_wiped = true;
    }
    if (session.storage_locked && session.mapping != MAP_FAILED &&
        session.mapping != nullptr) {
        output.secure_storage_unlocked =
            ::munlock(session.mapping, session.mapping_size) == 0;
        ok = output.secure_storage_unlocked && ok;
    } else {
        output.secure_storage_unlocked = true;
    }
    if (session.mapping != MAP_FAILED && session.mapping != nullptr &&
        session.mapping_size != 0) {
        output.secure_storage_unmapped =
            ::munmap(session.mapping, session.mapping_size) == 0;
        ok = output.secure_storage_unmapped && ok;
    } else {
        output.secure_storage_unmapped = true;
    }
    session.secure = nullptr;
    session.mapping = MAP_FAILED;
    session.mapping_size = 0;
    session.storage_locked = false;
    session.authenticated = false;
    return ok;
}

bool restore_authenticated_input_signal_mask(authenticated_input_session & session,
                                             sealed_input_audit & output) noexcept {
    if (session.context.signal_mask_changed) {
        output.signal_mask_restored =
            ::sigprocmask(SIG_SETMASK, &session.context.previous_mask, nullptr) == 0;
        session.context.signal_mask_changed = false;
    } else {
        output.signal_mask_restored = true;
    }
    return output.signal_mask_restored;
}

sealed_input_audit destroy_authenticated_input_session(
        authenticated_input_session & session, sealed_input_audit output) noexcept {
    const bool storage_ok = cleanup_authenticated_input_storage(session, output);
    const bool signal_ok = restore_authenticated_input_signal_mask(session, output);
    auto result = output.result;
    if ((!storage_ok || !signal_ok) && successful(result)) {
        result = sealed_input_status::io_failure_no_mutation;
    }
    output.root_or_fixture_accessed = output.root_or_fixture_syscall_count != 0;
    if (output.root_or_fixture_accessed) {
        result = sealed_input_status::io_failure_no_mutation;
    }
    if (result !=
        sealed_input_status::predecessor_authenticated_pins_matched_no_root_access) {
        output.predecessor_authenticated_under_supplied_credential = false;
        output.launcher_receipt_matched = false;
    }
    output.result = result;
    return output;
}

} // namespace

sealed_input_audit inspect_sealed_inputs_once(const sealed_input_request & input) noexcept {
    sealed_input_audit output {};
    authenticated_input_session session {};
    const auto result = authenticate_sealed_inputs_for_session(input, session, output);
    if (result ==
            sealed_input_status::predecessor_authenticated_pins_matched_no_root_access &&
        session.authenticated) {
        return destroy_authenticated_input_session(session, output);
    }
    return output;
}

} // namespace halofpx::registry_lab::linux_initializer

#include "halofpx-context-store-registry-lab-linux-initializer-anchor.inc"
