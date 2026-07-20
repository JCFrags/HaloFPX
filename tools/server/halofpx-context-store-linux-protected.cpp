#include "halofpx-context-store-linux-protected.h"

#if defined(__linux__)

#include <algorithm>
#include <array>
#include <cerrno>
#include <climits>
#include <cstring>
#include <cstdlib>
#include <fcntl.h>
#include <linux/fs.h>
#include <linux/stat.h>
#include <string>
#include <sys/file.h>
#include <sys/random.h>
#include <sys/stat.h>
#include <sys/syscall.h>
#include <unistd.h>
#include <utility>
#include <vector>

namespace halofpx {
namespace {

constexpr mode_t directory_mode = 0700;
constexpr mode_t file_mode = 0600;
constexpr char lock_name[] = ".writer-lock";
constexpr char staging_name[] = ".staging";
constexpr char direct_key_id[] = "halofpx-protected-direct-v1";
constexpr char anchor_key_id[] = "halofpx-protected-anchor-v1";
constexpr char scope_root_domain[] = "halofpx.protected-canary.scope-key.v1";
constexpr char direct_root_domain[] = "halofpx.protected-canary.direct-root-key.v1";
constexpr char direct_manifest_domain[] = "halofpx.protected-canary.direct-manifest-key.v1";
constexpr char anchor_root_domain[] = "halofpx.protected-canary.anchor-root-key.v1";
constexpr char anchor_master_domain[] = "halofpx.protected-canary.anchor-master.v1";

void wipe(void * memory, size_t size) noexcept {
    volatile uint8_t * bytes = static_cast<volatile uint8_t *>(memory);
    while (size-- != 0) *bytes++ = 0;
}

template <typename T>
void wipe_vector(std::vector<T> & value) noexcept {
    if (!value.empty()) wipe(value.data(), value.size() * sizeof(T));
    value.clear();
}

bool constant_equal(const uint8_t * left, const uint8_t * right, size_t size) noexcept {
    uint8_t difference = 0;
    for (size_t index = 0; index < size; ++index) difference |= left[index] ^ right[index];
    return difference == 0;
}

bool nonzero(const uint8_t * data, size_t size) noexcept {
    uint8_t combined = 0;
    for (size_t index = 0; index < size; ++index) combined |= data[index];
    return combined != 0;
}

bool retry_close(int fd) noexcept {
    if (fd < 0) return true;
    int result;
    do result = ::close(fd); while (result != 0 && errno == EINTR);
    return result == 0;
}

bool exact_identity(int fd, uint64_t device, uint32_t uid, mode_t mode, bool directory) noexcept {
    struct stat value {};
    return ::fstat(fd, &value) == 0 &&
        (directory ? S_ISDIR(value.st_mode) : S_ISREG(value.st_mode)) &&
        static_cast<uint64_t>(value.st_dev) == device &&
        static_cast<uint32_t>(value.st_uid) == uid &&
        (value.st_mode & 07777) == mode && (directory || value.st_nlink == 1);
}

bool mount_id_for_fd(int fd, uint64_t & output) noexcept {
    struct statx value {};
    if (::syscall(SYS_statx, fd, "", AT_EMPTY_PATH | AT_STATX_SYNC_AS_STAT,
            STATX_BASIC_STATS | STATX_MNT_ID, &value) != 0 ||
        (value.stx_mask & STATX_MNT_ID) == 0 || value.stx_mnt_id == 0) return false;
    output = value.stx_mnt_id;
    return true;
}

bool same_mount(int fd, uint64_t expected) noexcept {
    uint64_t observed = 0;
    return mount_id_for_fd(fd, observed) && observed == expected;
}

int open_directory_at(int parent, const char * name) noexcept {
    int fd;
    do fd = ::openat(parent, name, O_RDONLY | O_DIRECTORY | O_CLOEXEC | O_NOFOLLOW);
    while (fd < 0 && errno == EINTR);
    return fd;
}

int open_regular_at(int parent, const char * name, int flags, mode_t mode = 0) noexcept {
    int fd;
    do fd = ::openat(parent, name, flags | O_CLOEXEC | O_NOFOLLOW, mode);
    while (fd < 0 && errno == EINTR);
    return fd;
}

bool canonical_absolute_path(const char * input, std::array<char, PATH_MAX> & output) noexcept {
    output.fill(0);
    if (input == nullptr || input[0] != '/' || ::realpath(input, output.data()) == nullptr) return false;
    return std::strcmp(input, output.data()) == 0;
}

bool path_is_nested(const char * parent, const char * candidate) noexcept {
    const size_t parent_size = std::strlen(parent);
    return std::strncmp(parent, candidate, parent_size) == 0 &&
        candidate[parent_size] == '/';
}

std::array<char, 65> hex_name(const context_store_format_digest & digest) noexcept {
    constexpr char digits[] = "0123456789abcdef";
    std::array<char, 65> output {};
    for (size_t index = 0; index < digest.size(); ++index) {
        output[index * 2] = digits[digest[index] >> 4];
        output[index * 2 + 1] = digits[digest[index] & 0x0f];
    }
    return output;
}

std::array<char, 72> anchor_name(const context_store_format_digest & session) noexcept {
    const auto base = hex_name(session);
    std::array<char, 72> output {};
    std::copy_n(base.data(), 64, output.data());
    std::copy_n(".anchor", 7, output.data() + 64);
    return output;
}

bool full_write(int fd, const uint8_t * data, size_t size) noexcept {
    size_t offset = 0;
    while (offset < size) {
        const ssize_t count = ::write(fd, data + offset, size - offset);
        if (count < 0 && errno == EINTR) continue;
        if (count <= 0) return false;
        offset += static_cast<size_t>(count);
    }
    return true;
}

bool exact_read(int fd, uint8_t * data, size_t size) noexcept {
    size_t offset = 0;
    while (offset < size) {
        const ssize_t count = ::read(fd, data + offset, size - offset);
        if (count < 0 && errno == EINTR) continue;
        if (count <= 0) return false;
        offset += static_cast<size_t>(count);
    }
    uint8_t extra = 0;
    ssize_t count;
    do count = ::read(fd, &extra, 1); while (count < 0 && errno == EINTR);
    return count == 0;
}

bool append_bytes(std::vector<uint8_t> & output, const void * data, size_t size) {
    const auto * bytes = static_cast<const uint8_t *>(data);
    output.insert(output.end(), bytes, bytes + size);
    return true;
}

void append_u16(std::vector<uint8_t> & output, uint16_t value) {
    output.push_back(static_cast<uint8_t>(value >> 8));
    output.push_back(static_cast<uint8_t>(value));
}

void append_u64(std::vector<uint8_t> & output, uint64_t value) {
    for (size_t index = 0; index < 8; ++index) {
        output.push_back(static_cast<uint8_t>(value >> ((7 - index) * 8)));
    }
}

bool derive_root(
        const uint8_t * operator_key,
        const char * domain,
        size_t domain_size,
        const std::array<uint8_t, 16> & store_uuid,
        context_store_format_digest & output) {
    std::vector<uint8_t> preimage;
    try {
        preimage.reserve(domain_size + store_uuid.size());
        append_bytes(preimage, domain, domain_size);
        append_bytes(preimage, store_uuid.data(), store_uuid.size());
        const bool ok = context_store_hmac_sha256(operator_key,
            context_store_linux_protected_operator_key_bytes,
            preimage.data(), preimage.size(), output);
        wipe_vector(preimage);
        return ok;
    } catch (...) {
        wipe_vector(preimage);
        return false;
    }
}

bool derive_namespace_key(
        const context_store_format_digest & root_key,
        const char * domain,
        size_t domain_size,
        const context_store_format_digest & scope,
        const char * key_id,
        size_t key_id_size,
        context_store_format_digest & output) noexcept {
    std::vector<uint8_t> preimage;
    try {
        preimage.reserve(domain_size + scope.size() + 2 + key_id_size + 8);
        append_bytes(preimage, domain, domain_size);
        append_bytes(preimage, scope.data(), scope.size());
        append_u16(preimage, static_cast<uint16_t>(key_id_size));
        append_bytes(preimage, key_id, key_id_size);
        append_u64(preimage, 1);
        const bool ok = context_store_hmac_sha256(root_key.data(), root_key.size(),
            preimage.data(), preimage.size(), output);
        wipe_vector(preimage);
        return ok;
    } catch (...) {
        wipe_vector(preimage);
        return false;
    }
}

context_store_registered_id registered_id(const char * text, size_t size) noexcept {
    context_store_registered_id output;
    if (size <= context_store_registered_id_max_bytes) {
        std::copy_n(text, size, output.bytes.begin());
        output.size = static_cast<uint8_t>(size);
    }
    return output;
}

context_store_protected_canary_anchor_body anchor_body(
        const std::array<uint8_t, 16> & store_uuid,
        const context_store_linux_direct_receipt & receipt) noexcept {
    context_store_protected_canary_anchor_body body;
    body.store_uuid = store_uuid;
    body.namespace_id = receipt.scope;
    body.policy_epoch = 1;
    body.checkpoint_lineage_id = receipt.session;
    body.manifest_key_generation = 1;
    body.authority_epoch = 1;
    body.generation = 1;
    body.selected_manifest_digest = receipt.selected_digest;
    body.has_predecessor = false;
    return body;
}

bool encode_expected_anchor(
        const std::array<uint8_t, 16> & store_uuid,
        const context_store_format_digest & anchor_root_key,
        const context_store_linux_direct_receipt & receipt,
        std::array<uint8_t, context_store_protected_canary_anchor_max_bytes> & output,
        size_t & output_size,
        context_store_format_digest & namespace_key) noexcept {
    output.fill(0);
    output_size = 0;
    namespace_key.fill(0);
    if (!derive_namespace_key(anchor_root_key, anchor_master_domain, sizeof(anchor_master_domain),
            receipt.scope, anchor_key_id, sizeof(anchor_key_id) - 1, namespace_key)) return false;
    context_store_protected_canary_anchor_key key;
    key.key_id = registered_id(anchor_key_id, sizeof(anchor_key_id) - 1);
    key.generation = 1;
    key.master_key = { namespace_key.data(), namespace_key.size() };
    const auto result = context_store_protected_canary_anchor_encode_v1(
        anchor_body(store_uuid, receipt), key, output.data(), output.size());
    output_size = result.encoded_size;
    return result.authenticated();
}

bool anchor_scope_directory(
        int root_fd, uint64_t device, uint64_t mount_id, uint32_t uid,
        const context_store_format_digest & scope, bool create, int & output) noexcept {
    output = -1;
    const auto scope_name = hex_name(scope);
    if (create && ::mkdirat(root_fd, scope_name.data(), directory_mode) != 0 && errno != EEXIST) return false;
    const int fd = open_directory_at(root_fd, scope_name.data());
    if (fd < 0 || !exact_identity(fd, device, uid, directory_mode, true) || !same_mount(fd, mount_id)) {
        if (fd >= 0) errno = EUCLEAN;
        retry_close(fd);
        return false;
    }
    if (create && ::fsync(root_fd) != 0) {
        retry_close(fd);
        return false;
    }
    output = fd;
    return true;
}

enum class anchor_observation : uint8_t { exact, absent, malformed, io_error };

anchor_observation observe_anchor(
        int scope_fd, uint64_t device, uint32_t uid,
        const char * name,
        const uint8_t * expected, size_t expected_size,
        const context_store_protected_canary_anchor_body & expected_body,
        const context_store_protected_canary_anchor_key & key) noexcept {
    const int fd = open_regular_at(scope_fd, name, O_RDONLY);
    if (fd < 0) return errno == ENOENT ? anchor_observation::absent : anchor_observation::io_error;
    struct stat value {};
    const bool stat_ok = ::fstat(fd, &value) == 0 && S_ISREG(value.st_mode) &&
        static_cast<uint64_t>(value.st_dev) == device && static_cast<uint32_t>(value.st_uid) == uid &&
        (value.st_mode & 07777) == file_mode && value.st_nlink == 1 &&
        value.st_size > 0 && value.st_size <=
            static_cast<off_t>(context_store_protected_canary_anchor_max_bytes);
    std::array<uint8_t, context_store_protected_canary_anchor_max_bytes> observed {};
    const size_t size = stat_ok ? static_cast<size_t>(value.st_size) : 0;
    const bool read_ok = stat_ok && exact_read(fd, observed.data(), size);
    const bool close_ok = retry_close(fd);
    if (!read_ok || !close_ok) {
        wipe(observed.data(), observed.size());
        return stat_ok ? anchor_observation::io_error : anchor_observation::malformed;
    }
    const auto verified = context_store_protected_canary_anchor_verify_v1(
        observed.data(), size, expected_body, key);
    const bool exact = verified.authenticated() &&
        context_store_protected_canary_anchor_exact_envelope_equal(
            observed.data(), size, expected, expected_size);
    wipe(observed.data(), observed.size());
    return exact ? anchor_observation::exact : anchor_observation::malformed;
}

bool random_stage_name(std::array<char, 70> & output) noexcept {
    constexpr char digits[] = "0123456789abcdef";
    std::array<uint8_t, 32> random {};
    size_t offset = 0;
    while (offset < random.size()) {
        const ssize_t count = ::getrandom(random.data() + offset, random.size() - offset, 0);
        if (count < 0 && errno == EINTR) continue;
        if (count <= 0) { wipe(random.data(), random.size()); return false; }
        offset += static_cast<size_t>(count);
    }
    output.fill(0);
    for (size_t index = 0; index < random.size(); ++index) {
        output[index * 2] = digits[random[index] >> 4];
        output[index * 2 + 1] = digits[random[index] & 0x0f];
    }
    std::copy_n(".tmp", 4, output.data() + 64);
    wipe(random.data(), random.size());
    return true;
}

bool receipt_equal(
        const context_store_linux_direct_receipt & left,
        const context_store_linux_direct_receipt & right) noexcept {
    return constant_equal(left.manifest.data(), right.manifest.data(), left.manifest.size()) &&
        constant_equal(left.selected_digest.data(), right.selected_digest.data(), left.selected_digest.size()) &&
        constant_equal(left.scope.data(), right.scope.data(), left.scope.size()) &&
        constant_equal(left.session.data(), right.session.data(), left.session.size()) &&
        constant_equal(left.compatibility.data(), right.compatibility.data(), left.compatibility.size());
}

void wipe_receipt(context_store_linux_direct_receipt & receipt) noexcept {
    wipe(&receipt, sizeof(receipt));
}

context_store_linux_protected_lookup_status map_lookup(
        context_store_linux_direct_lookup_status status) noexcept {
    switch (status) {
        case context_store_linux_direct_lookup_status::hit:
            return context_store_linux_protected_lookup_status::hit;
        case context_store_linux_direct_lookup_status::miss_not_found:
            return context_store_linux_protected_lookup_status::miss_not_found;
        case context_store_linux_direct_lookup_status::miss_incompatible:
            return context_store_linux_protected_lookup_status::miss_incompatible;
        case context_store_linux_direct_lookup_status::miss_corrupt:
            return context_store_linux_protected_lookup_status::miss_corrupt;
        case context_store_linux_direct_lookup_status::invalid_request:
            return context_store_linux_protected_lookup_status::invalid_request;
        case context_store_linux_direct_lookup_status::unavailable:
            return context_store_linux_protected_lookup_status::unavailable;
    }
    return context_store_linux_protected_lookup_status::unavailable;
}

context_store_linux_protected_publish_status map_publish(
        context_store_linux_direct_publish_status status) noexcept {
    switch (status) {
        case context_store_linux_direct_publish_status::published:
            return context_store_linux_protected_publish_status::published;
        case context_store_linux_direct_publish_status::already_exists:
            return context_store_linux_protected_publish_status::already_exists;
        case context_store_linux_direct_publish_status::conflict:
            return context_store_linux_protected_publish_status::conflict;
        case context_store_linux_direct_publish_status::invalid_request:
            return context_store_linux_protected_publish_status::invalid_request;
        case context_store_linux_direct_publish_status::quota_exceeded:
            return context_store_linux_protected_publish_status::quota_exceeded;
        case context_store_linux_direct_publish_status::reserve_exhausted:
            return context_store_linux_protected_publish_status::reserve_exhausted;
        case context_store_linux_direct_publish_status::unavailable:
            return context_store_linux_protected_publish_status::unavailable;
        case context_store_linux_direct_publish_status::unsupported:
            return context_store_linux_protected_publish_status::unsupported;
        case context_store_linux_direct_publish_status::io_error:
            return context_store_linux_protected_publish_status::io_error;
    }
    return context_store_linux_protected_publish_status::io_error;
}

} // namespace

bool context_store_linux_protected_derive_root_authority(
        const uint8_t * operator_key, size_t operator_key_size,
        const std::array<uint8_t, 16> & store_uuid,
        context_store_linux_protected_root_authority & output) noexcept {
    wipe(&output, sizeof(output));
    if (operator_key == nullptr || operator_key_size != context_store_linux_protected_operator_key_bytes ||
        !nonzero(operator_key, operator_key_size) || !nonzero(store_uuid.data(), store_uuid.size())) return false;
    const bool ok = derive_root(operator_key, scope_root_domain, sizeof(scope_root_domain), store_uuid,
            output.scope_key) &&
        derive_root(operator_key, direct_root_domain, sizeof(direct_root_domain), store_uuid,
            output.direct_root_key) &&
        derive_root(operator_key, anchor_root_domain, sizeof(anchor_root_domain), store_uuid,
            output.anchor_root_key);
    if (!ok) wipe(&output, sizeof(output));
    return ok;
}

context_store_linux_protected::~context_store_linux_protected() noexcept {
    wipe(direct_root_key_.data(), direct_root_key_.size());
    wipe(anchor_root_key_.data(), anchor_root_key_.size());
    retry_close(anchor_lock_fd_);
    retry_close(anchor_staging_fd_);
    retry_close(anchor_root_fd_);
}

context_store_linux_protected::context_store_linux_protected(
        context_store_linux_protected && other) noexcept {
    *this = std::move(other);
}

context_store_linux_protected & context_store_linux_protected::operator=(
        context_store_linux_protected && other) noexcept {
    if (this != &other) {
        wipe(direct_root_key_.data(), direct_root_key_.size());
        wipe(anchor_root_key_.data(), anchor_root_key_.size());
        retry_close(anchor_lock_fd_);
        retry_close(anchor_staging_fd_);
        retry_close(anchor_root_fd_);
        direct_ = std::move(other.direct_);
        anchor_root_fd_ = other.anchor_root_fd_;
        anchor_staging_fd_ = other.anchor_staging_fd_;
        anchor_lock_fd_ = other.anchor_lock_fd_;
        anchor_device_ = other.anchor_device_;
        anchor_mount_id_ = other.anchor_mount_id_;
        anchor_owner_uid_ = other.anchor_owner_uid_;
        store_uuid_ = other.store_uuid_;
        direct_root_key_ = other.direct_root_key_;
        anchor_root_key_ = other.anchor_root_key_;
        test_failpoint_ = other.test_failpoint_;
        other.anchor_root_fd_ = other.anchor_staging_fd_ = other.anchor_lock_fd_ = -1;
        other.anchor_device_ = other.anchor_mount_id_ = other.anchor_owner_uid_ = 0;
        other.store_uuid_.fill(0);
        other.direct_root_key_.fill(0);
        other.anchor_root_key_.fill(0);
        other.test_failpoint_ = context_store_linux_protected_test_failpoint::none;
    }
    return *this;
}

bool context_store_linux_protected::available() const noexcept {
    return direct_.available() && anchor_root_fd_ >= 0 && anchor_staging_fd_ >= 0 && anchor_lock_fd_ >= 0;
}

context_store_linux_protected_open_status context_store_linux_protected_open(
        const context_store_linux_protected_config & config,
        context_store_linux_protected & output) noexcept {
    if (output.available() || config.direct.root_path == nullptr || config.anchor_root_path == nullptr ||
        config.direct_root_key == nullptr || config.direct_root_key_size != 32 ||
        config.anchor_root_key == nullptr || config.anchor_root_key_size != 32 ||
        !nonzero(config.store_uuid.data(), config.store_uuid.size()) ||
        !nonzero(config.direct_root_key, config.direct_root_key_size) ||
        !nonzero(config.anchor_root_key, config.anchor_root_key_size) ||
        config.expected_anchor_root.device == 0 || config.expected_anchor_root.mount_id == 0) {
        return context_store_linux_protected_open_status::invalid_configuration;
    }
    std::array<char, PATH_MAX> direct_path {}, anchor_path {};
    if (!canonical_absolute_path(config.direct.root_path, direct_path) ||
        !canonical_absolute_path(config.anchor_root_path, anchor_path) ||
        std::strcmp(direct_path.data(), anchor_path.data()) == 0 ||
        path_is_nested(direct_path.data(), anchor_path.data()) ||
        path_is_nested(anchor_path.data(), direct_path.data())) {
        return context_store_linux_protected_open_status::roots_rejected;
    }
    const int root_fd = ::open(config.anchor_root_path,
        O_RDONLY | O_DIRECTORY | O_CLOEXEC | O_NOFOLLOW);
    if (root_fd < 0 || !exact_identity(root_fd, config.expected_anchor_root.device,
            config.expected_anchor_root.owner_uid, directory_mode, true) ||
        !same_mount(root_fd, config.expected_anchor_root.mount_id)) {
        retry_close(root_fd);
        return context_store_linux_protected_open_status::roots_rejected;
    }
    const int lock_fd = open_regular_at(root_fd, lock_name, O_RDWR | O_CREAT, file_mode);
    if (lock_fd < 0 || !exact_identity(lock_fd, config.expected_anchor_root.device,
            config.expected_anchor_root.owner_uid, file_mode, false)) {
        retry_close(lock_fd); retry_close(root_fd);
        return context_store_linux_protected_open_status::roots_rejected;
    }
    struct flock lock {};
    lock.l_type = F_WRLCK;
    lock.l_whence = SEEK_SET;
    int lock_result;
    do lock_result = ::fcntl(lock_fd, F_OFD_SETLK, &lock); while (lock_result != 0 && errno == EINTR);
    if (lock_result != 0 && (errno == EINVAL || errno == ENOSYS)) {
        do lock_result = ::flock(lock_fd, LOCK_EX | LOCK_NB); while (lock_result != 0 && errno == EINTR);
    }
    if (lock_result != 0) {
        const int error = errno;
        retry_close(lock_fd); retry_close(root_fd);
        return error == EAGAIN || error == EACCES || error == EWOULDBLOCK ?
            context_store_linux_protected_open_status::writer_busy :
            context_store_linux_protected_open_status::io_error;
    }
    if (::mkdirat(root_fd, staging_name, directory_mode) != 0 && errno != EEXIST) {
        retry_close(lock_fd); retry_close(root_fd);
        return context_store_linux_protected_open_status::io_error;
    }
    const int staging_fd = open_directory_at(root_fd, staging_name);
    if (staging_fd < 0 || !exact_identity(staging_fd, config.expected_anchor_root.device,
            config.expected_anchor_root.owner_uid, directory_mode, true) ||
        !same_mount(staging_fd, config.expected_anchor_root.mount_id) || ::fsync(root_fd) != 0) {
        retry_close(staging_fd); retry_close(lock_fd); retry_close(root_fd);
        return context_store_linux_protected_open_status::roots_rejected;
    }
    context_store_linux_direct direct;
    auto direct_config = config.direct;
    direct_config.master_key = config.direct_root_key;
    direct_config.master_key_size = config.direct_root_key_size;
    const auto direct_status = context_store_linux_direct_open(direct_config, direct);
    if (direct_status != context_store_linux_direct_open_status::opened) {
        retry_close(staging_fd); retry_close(lock_fd); retry_close(root_fd);
        if (direct_status == context_store_linux_direct_open_status::unsupported)
            return context_store_linux_protected_open_status::unsupported;
        if (direct_status == context_store_linux_direct_open_status::writer_busy)
            return context_store_linux_protected_open_status::writer_busy;
        return context_store_linux_protected_open_status::direct_unavailable;
    }
    output.direct_ = std::move(direct);
    output.anchor_root_fd_ = root_fd;
    output.anchor_staging_fd_ = staging_fd;
    output.anchor_lock_fd_ = lock_fd;
    output.anchor_device_ = config.expected_anchor_root.device;
    output.anchor_mount_id_ = config.expected_anchor_root.mount_id;
    output.anchor_owner_uid_ = config.expected_anchor_root.owner_uid;
    output.store_uuid_ = config.store_uuid;
    std::copy_n(config.direct_root_key, output.direct_root_key_.size(), output.direct_root_key_.begin());
    std::copy_n(config.anchor_root_key, output.anchor_root_key_.size(), output.anchor_root_key_.begin());
    output.test_failpoint_ = config.test_failpoint;
    return context_store_linux_protected_open_status::opened;
}

context_store_linux_protected_lookup_status context_store_linux_protected::lookup(
        const context_store_format_digest & scope,
        const context_store_format_digest & session,
        const context_store_format_digest & compatibility,
        context_store_linux_direct_value & output) const noexcept {
    wipe_vector(output.tokens);
    wipe_vector(output.state);
    if (!available()) return context_store_linux_protected_lookup_status::unavailable;
    context_store_format_digest direct_key {};
    context_store_format_digest anchor_key_bytes {};
    context_store_linux_direct_receipt receipt;
    if (!derive_namespace_key(direct_root_key_, direct_manifest_domain, sizeof(direct_manifest_domain),
            scope, direct_key_id, sizeof(direct_key_id) - 1, direct_key)) {
        return context_store_linux_protected_lookup_status::unavailable;
    }
    const auto direct_status = direct_.inspect_manifest(direct_key, scope, session, compatibility, receipt);
    if (direct_status != context_store_linux_direct_lookup_status::hit) {
        wipe(direct_key.data(), direct_key.size());
        wipe_receipt(receipt);
        return map_lookup(direct_status);
    }
    std::array<uint8_t, context_store_protected_canary_anchor_max_bytes> expected {};
    size_t expected_size = 0;
    if (!encode_expected_anchor(store_uuid_, anchor_root_key_, receipt,
            expected, expected_size, anchor_key_bytes)) {
        wipe(direct_key.data(), direct_key.size());
        wipe(anchor_key_bytes.data(), anchor_key_bytes.size());
        wipe_receipt(receipt);
        return context_store_linux_protected_lookup_status::unavailable;
    }
    context_store_protected_canary_anchor_key key;
    key.key_id = registered_id(anchor_key_id, sizeof(anchor_key_id) - 1);
    key.generation = 1;
    key.master_key = { anchor_key_bytes.data(), anchor_key_bytes.size() };
    int scope_fd = -1;
    if (!anchor_scope_directory(anchor_root_fd_, anchor_device_, anchor_mount_id_,
            anchor_owner_uid_, scope, false, scope_fd)) {
        const int error = errno;
        wipe(expected.data(), expected.size()); wipe(anchor_key_bytes.data(), anchor_key_bytes.size());
        wipe(direct_key.data(), direct_key.size()); wipe_receipt(receipt);
        return error == ENOENT ? context_store_linux_protected_lookup_status::miss_unanchored :
            context_store_linux_protected_lookup_status::lineage_quarantined;
    }
    const auto name = anchor_name(session);
    const auto observed = observe_anchor(scope_fd, anchor_device_, anchor_owner_uid_, name.data(),
        expected.data(), expected_size, anchor_body(store_uuid_, receipt), key);
    retry_close(scope_fd);
    if (observed != anchor_observation::exact) {
        wipe(expected.data(), expected.size()); wipe(anchor_key_bytes.data(), anchor_key_bytes.size());
        wipe(direct_key.data(), direct_key.size()); wipe_receipt(receipt);
        if (observed == anchor_observation::absent) return context_store_linux_protected_lookup_status::miss_unanchored;
        return observed == anchor_observation::malformed ?
            context_store_linux_protected_lookup_status::miss_corrupt :
            context_store_linux_protected_lookup_status::lineage_quarantined;
    }
    const auto load_status = direct_.authorized_load(direct_key, receipt, output);
    wipe(expected.data(), expected.size()); wipe(anchor_key_bytes.data(), anchor_key_bytes.size());
    wipe(direct_key.data(), direct_key.size()); wipe_receipt(receipt);
    return map_lookup(load_status);
}

context_store_linux_protected_publish_status context_store_linux_protected::publish(
        const context_store_format_digest & scope,
        const context_store_format_digest & session,
        const context_store_format_digest & compatibility,
        const int32_t * tokens, size_t token_count,
        const uint8_t * state, size_t state_size) noexcept {
    if (!available()) return context_store_linux_protected_publish_status::unavailable;
    context_store_format_digest direct_key {};
    context_store_format_digest anchor_key_bytes {};
    context_store_linux_direct_receipt receipt;
    if (!derive_namespace_key(direct_root_key_, direct_manifest_domain, sizeof(direct_manifest_domain),
            scope, direct_key_id, sizeof(direct_key_id) - 1, direct_key)) {
        return context_store_linux_protected_publish_status::unavailable;
    }
    const auto direct_status = direct_.publish_with_receipt(direct_key, scope, session, compatibility,
        tokens, token_count, state, state_size, receipt);
    if (direct_status != context_store_linux_direct_publish_status::published &&
        direct_status != context_store_linux_direct_publish_status::already_exists) {
        wipe(direct_key.data(), direct_key.size()); wipe_receipt(receipt);
        return map_publish(direct_status);
    }
    std::array<uint8_t, context_store_protected_canary_anchor_max_bytes> expected {};
    size_t expected_size = 0;
    if (!encode_expected_anchor(store_uuid_, anchor_root_key_, receipt,
            expected, expected_size, anchor_key_bytes)) {
        wipe(direct_key.data(), direct_key.size()); wipe(anchor_key_bytes.data(), anchor_key_bytes.size());
        wipe_receipt(receipt);
        return context_store_linux_protected_publish_status::unavailable;
    }
    context_store_protected_canary_anchor_key key;
    key.key_id = registered_id(anchor_key_id, sizeof(anchor_key_id) - 1);
    key.generation = 1;
    key.master_key = { anchor_key_bytes.data(), anchor_key_bytes.size() };
    int scope_fd = -1;
    if (!anchor_scope_directory(anchor_root_fd_, anchor_device_, anchor_mount_id_,
            anchor_owner_uid_, scope, true, scope_fd)) {
        wipe(expected.data(), expected.size()); wipe(anchor_key_bytes.data(), anchor_key_bytes.size());
        wipe(direct_key.data(), direct_key.size()); wipe_receipt(receipt);
        return context_store_linux_protected_publish_status::io_error;
    }
    const auto name = anchor_name(session);
    const auto body = anchor_body(store_uuid_, receipt);
    const auto existing = observe_anchor(scope_fd, anchor_device_, anchor_owner_uid_, name.data(),
        expected.data(), expected_size, body, key);
    if (existing == anchor_observation::exact) {
        context_store_linux_direct_receipt reopened;
        const auto reopened_status = direct_.inspect_manifest(direct_key, scope, session, compatibility, reopened);
        const bool exact = reopened_status == context_store_linux_direct_lookup_status::hit && receipt_equal(receipt, reopened);
        wipe_receipt(reopened); retry_close(scope_fd);
        wipe(expected.data(), expected.size()); wipe(anchor_key_bytes.data(), anchor_key_bytes.size());
        wipe(direct_key.data(), direct_key.size()); wipe_receipt(receipt);
        return exact ? context_store_linux_protected_publish_status::already_exists :
            context_store_linux_protected_publish_status::lineage_quarantined;
    }
    if (existing == anchor_observation::malformed) {
        retry_close(scope_fd); wipe(expected.data(), expected.size());
        wipe(anchor_key_bytes.data(), anchor_key_bytes.size()); wipe(direct_key.data(), direct_key.size());
        wipe_receipt(receipt);
        return context_store_linux_protected_publish_status::conflict;
    }
    if (existing == anchor_observation::io_error) {
        retry_close(scope_fd); wipe(expected.data(), expected.size());
        wipe(anchor_key_bytes.data(), anchor_key_bytes.size()); wipe(direct_key.data(), direct_key.size());
        wipe_receipt(receipt);
        return context_store_linux_protected_publish_status::io_error;
    }

    std::array<char, 70> stage_name {};
    int stage_fd = -1;
    for (size_t attempt = 0; attempt < 8 && stage_fd < 0; ++attempt) {
        if (!random_stage_name(stage_name)) break;
        stage_fd = open_regular_at(anchor_staging_fd_, stage_name.data(),
            O_RDWR | O_CREAT | O_EXCL, file_mode);
        if (stage_fd < 0 && errno != EEXIST) break;
    }
    bool staged = stage_fd >= 0 && exact_identity(stage_fd, anchor_device_, anchor_owner_uid_, file_mode, false) &&
        full_write(stage_fd, expected.data(), expected_size) && ::fdatasync(stage_fd) == 0 &&
        ::lseek(stage_fd, 0, SEEK_SET) == 0;
    std::array<uint8_t, context_store_protected_canary_anchor_max_bytes> readback {};
    staged = staged && exact_read(stage_fd, readback.data(), expected_size) &&
        context_store_protected_canary_anchor_exact_envelope_equal(
            readback.data(), expected_size, expected.data(), expected_size);
    wipe(readback.data(), readback.size());
    retry_close(stage_fd);
    if (!staged) {
        ::unlinkat(anchor_staging_fd_, stage_name.data(), 0); retry_close(scope_fd);
        wipe(expected.data(), expected.size()); wipe(anchor_key_bytes.data(), anchor_key_bytes.size());
        wipe(direct_key.data(), direct_key.size()); wipe_receipt(receipt);
        return context_store_linux_protected_publish_status::io_error;
    }

    bool ambiguous = test_failpoint_ ==
        context_store_linux_protected_test_failpoint::ambiguous_before_anchor_rename;
    int renamed = -1;
    if (!ambiguous) {
        do renamed = static_cast<int>(::syscall(SYS_renameat2, anchor_staging_fd_, stage_name.data(),
            scope_fd, name.data(), RENAME_NOREPLACE)); while (renamed != 0 && errno == EINTR);
        if (renamed == 0 && test_failpoint_ ==
                context_store_linux_protected_test_failpoint::ambiguous_after_anchor_rename) ambiguous = true;
    }
    if (test_failpoint_ == context_store_linux_protected_test_failpoint::ambiguous_before_anchor_rename) {
        ::unlinkat(anchor_staging_fd_, stage_name.data(), 0);
    }
    if (!ambiguous && renamed != 0) {
        const int error = errno;
        ::unlinkat(anchor_staging_fd_, stage_name.data(), 0);
        const auto collision = observe_anchor(scope_fd, anchor_device_, anchor_owner_uid_, name.data(),
            expected.data(), expected_size, body, key);
        bool exact_material = false;
        if (error == EEXIST && collision == anchor_observation::exact) {
            context_store_linux_direct_receipt reopened;
            const auto reopened_status = direct_.inspect_manifest(
                direct_key, scope, session, compatibility, reopened);
            exact_material = reopened_status == context_store_linux_direct_lookup_status::hit &&
                receipt_equal(receipt, reopened);
            wipe_receipt(reopened);
        }
        retry_close(scope_fd); wipe(expected.data(), expected.size());
        wipe(anchor_key_bytes.data(), anchor_key_bytes.size()); wipe(direct_key.data(), direct_key.size());
        wipe_receipt(receipt);
        if (error == EEXIST) {
            if (collision != anchor_observation::exact)
                return context_store_linux_protected_publish_status::conflict;
            return exact_material ? context_store_linux_protected_publish_status::already_exists :
                context_store_linux_protected_publish_status::lineage_quarantined;
        }
        if (error == ENOSYS || error == EINVAL) return context_store_linux_protected_publish_status::unsupported;
        return context_store_linux_protected_publish_status::io_error;
    }

    bool durable = false;
    if (!ambiguous) {
        durable = ::fsync(scope_fd) == 0;
    }
    if (ambiguous || !durable) {
        const auto first = observe_anchor(scope_fd, anchor_device_, anchor_owner_uid_, name.data(),
            expected.data(), expected_size, body, key);
        if (first == anchor_observation::exact && ::fsync(scope_fd) == 0) {
            const auto second = observe_anchor(scope_fd, anchor_device_, anchor_owner_uid_, name.data(),
                expected.data(), expected_size, body, key);
            durable = second == anchor_observation::exact;
            if (durable) {
                context_store_linux_direct_receipt reopened;
                const auto reopened_status = direct_.inspect_manifest(direct_key, scope, session, compatibility, reopened);
                durable = reopened_status == context_store_linux_direct_lookup_status::hit && receipt_equal(receipt, reopened);
                wipe_receipt(reopened);
            }
            retry_close(scope_fd); wipe(expected.data(), expected.size());
            wipe(anchor_key_bytes.data(), anchor_key_bytes.size()); wipe(direct_key.data(), direct_key.size());
            wipe_receipt(receipt);
            return durable ? context_store_linux_protected_publish_status::recovered_durable :
                context_store_linux_protected_publish_status::lineage_quarantined;
        }
        retry_close(scope_fd); wipe(expected.data(), expected.size());
        wipe(anchor_key_bytes.data(), anchor_key_bytes.size()); wipe(direct_key.data(), direct_key.size());
        wipe_receipt(receipt);
        return first == anchor_observation::absent ? context_store_linux_protected_publish_status::unreachable :
            context_store_linux_protected_publish_status::lineage_quarantined;
    }

    const auto confirmed = observe_anchor(scope_fd, anchor_device_, anchor_owner_uid_, name.data(),
        expected.data(), expected_size, body, key);
    context_store_linux_direct_receipt reopened;
    const auto reopened_status = direct_.inspect_manifest(direct_key, scope, session, compatibility, reopened);
    const bool exact = confirmed == anchor_observation::exact &&
        reopened_status == context_store_linux_direct_lookup_status::hit && receipt_equal(receipt, reopened);
    wipe_receipt(reopened); retry_close(scope_fd); wipe(expected.data(), expected.size());
    wipe(anchor_key_bytes.data(), anchor_key_bytes.size()); wipe(direct_key.data(), direct_key.size());
    wipe_receipt(receipt);
    return exact ? context_store_linux_protected_publish_status::published :
        context_store_linux_protected_publish_status::lineage_quarantined;
}

const char * context_store_linux_protected_open_status_name(
        context_store_linux_protected_open_status status) noexcept {
    switch (status) {
        case context_store_linux_protected_open_status::opened: return "opened";
        case context_store_linux_protected_open_status::invalid_configuration: return "invalid-configuration";
        case context_store_linux_protected_open_status::roots_rejected: return "roots-rejected";
        case context_store_linux_protected_open_status::writer_busy: return "writer-busy";
        case context_store_linux_protected_open_status::direct_unavailable: return "direct-unavailable";
        case context_store_linux_protected_open_status::unsupported: return "unsupported";
        case context_store_linux_protected_open_status::io_error: return "io-error";
    }
    return "unknown";
}

const char * context_store_linux_protected_lookup_status_name(
        context_store_linux_protected_lookup_status status) noexcept {
    switch (status) {
        case context_store_linux_protected_lookup_status::hit: return "hit";
        case context_store_linux_protected_lookup_status::miss_not_found: return "miss-not-found";
        case context_store_linux_protected_lookup_status::miss_unanchored: return "miss-unanchored";
        case context_store_linux_protected_lookup_status::miss_incompatible: return "miss-incompatible";
        case context_store_linux_protected_lookup_status::miss_corrupt: return "miss-corrupt";
        case context_store_linux_protected_lookup_status::lineage_quarantined: return "lineage-quarantined";
        case context_store_linux_protected_lookup_status::invalid_request: return "invalid-request";
        case context_store_linux_protected_lookup_status::unavailable: return "unavailable";
    }
    return "unknown";
}

const char * context_store_linux_protected_publish_status_name(
        context_store_linux_protected_publish_status status) noexcept {
    switch (status) {
        case context_store_linux_protected_publish_status::published: return "published";
        case context_store_linux_protected_publish_status::recovered_durable: return "recovered-durable";
        case context_store_linux_protected_publish_status::already_exists: return "already-exists";
        case context_store_linux_protected_publish_status::conflict: return "conflict";
        case context_store_linux_protected_publish_status::unreachable: return "unreachable";
        case context_store_linux_protected_publish_status::lineage_quarantined: return "lineage-quarantined";
        case context_store_linux_protected_publish_status::invalid_request: return "invalid-request";
        case context_store_linux_protected_publish_status::quota_exceeded: return "quota-exceeded";
        case context_store_linux_protected_publish_status::reserve_exhausted: return "reserve-exhausted";
        case context_store_linux_protected_publish_status::unavailable: return "unavailable";
        case context_store_linux_protected_publish_status::unsupported: return "unsupported";
        case context_store_linux_protected_publish_status::io_error: return "io-error";
    }
    return "unknown";
}

} // namespace halofpx

#else

namespace halofpx {
bool context_store_linux_protected_derive_root_authority(
    const uint8_t *, size_t, const std::array<uint8_t, 16> &,
    context_store_linux_protected_root_authority & output) noexcept { output = {}; return false; }
context_store_linux_protected::~context_store_linux_protected() noexcept = default;
context_store_linux_protected::context_store_linux_protected(context_store_linux_protected &&) noexcept = default;
context_store_linux_protected & context_store_linux_protected::operator=(context_store_linux_protected &&) noexcept = default;
bool context_store_linux_protected::available() const noexcept { return false; }
context_store_linux_protected_lookup_status context_store_linux_protected::lookup(
    const context_store_format_digest &, const context_store_format_digest &,
    const context_store_format_digest &, context_store_linux_direct_value & output) const noexcept {
    output = {}; return context_store_linux_protected_lookup_status::unavailable;
}
context_store_linux_protected_publish_status context_store_linux_protected::publish(
    const context_store_format_digest &, const context_store_format_digest &,
    const context_store_format_digest &, const int32_t *, size_t, const uint8_t *, size_t) noexcept {
    return context_store_linux_protected_publish_status::unavailable;
}
context_store_linux_protected_open_status context_store_linux_protected_open(
    const context_store_linux_protected_config &, context_store_linux_protected &) noexcept {
    return context_store_linux_protected_open_status::unsupported;
}
const char * context_store_linux_protected_open_status_name(context_store_linux_protected_open_status) noexcept { return "unsupported"; }
const char * context_store_linux_protected_lookup_status_name(context_store_linux_protected_lookup_status) noexcept { return "unavailable"; }
const char * context_store_linux_protected_publish_status_name(context_store_linux_protected_publish_status) noexcept { return "unavailable"; }
} // namespace halofpx

#endif
