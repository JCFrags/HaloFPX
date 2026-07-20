#include "halofpx-context-store-linux-direct.h"

#if defined(__linux__)

#include <algorithm>
#include <array>
#include <cerrno>
#include <climits>
#include <cstdio>
#include <cstring>
#include <dirent.h>
#include <fcntl.h>
#include <limits>
#include <linux/fs.h>
#include <linux/stat.h>
#include <new>
#include <string>
#include <utility>
#include <sys/file.h>
#include <sys/random.h>
#include <sys/stat.h>
#include <sys/statvfs.h>
#include <sys/syscall.h>
#include <unistd.h>

namespace halofpx {
namespace {

constexpr std::array<uint8_t, 8> manifest_magic = { 'H','F','P','X','L','D','0','1' };
constexpr uint16_t manifest_version = 1;
constexpr size_t manifest_auth_bytes = 196;
constexpr size_t manifest_bytes = manifest_auth_bytes + 32;
constexpr mode_t directory_mode = 0700;
constexpr mode_t file_mode = 0600;
constexpr char lock_name[] = ".writer-lock";
constexpr char staging_name[] = ".staging";
constexpr char manifest_name[] = "manifest";
constexpr char tokens_name[] = "tokens";
constexpr char state_name[] = "state";

void wipe(void * memory, size_t size) noexcept {
    volatile uint8_t * bytes = static_cast<volatile uint8_t *>(memory);
    while (size-- != 0) *bytes++ = 0;
}

template <typename T>
void wipe_vector(std::vector<T> & value) noexcept {
    if (!value.empty()) wipe(value.data(), value.size() * sizeof(T));
    value.clear();
}

struct byte_vector_wipe_on_exit {
    std::vector<uint8_t> & value;
    ~byte_vector_wipe_on_exit() { wipe_vector(value); }
};

bool nonzero(const context_store_format_digest & value) noexcept {
    uint8_t combined = 0;
    for (const uint8_t byte : value) combined |= byte;
    return combined != 0;
}

bool constant_equal(const uint8_t * left, const uint8_t * right, size_t size) noexcept {
    uint8_t difference = 0;
    for (size_t index = 0; index < size; ++index) difference |= left[index] ^ right[index];
    return difference == 0;
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

bool exact_hex_name(const char * name) noexcept {
    if (name == nullptr || std::strlen(name) != 64) return false;
    for (size_t index = 0; index < 64; ++index) {
        const char value = name[index];
        if (!((value >= '0' && value <= '9') || (value >= 'a' && value <= 'f'))) return false;
    }
    return true;
}

bool retry_close(int fd) noexcept {
    if (fd < 0) return true;
    int result;
    do result = ::close(fd); while (result != 0 && errno == EINTR);
    return result == 0;
}

bool exact_identity(int fd, uint64_t device, uint32_t uid, mode_t mode, bool directory) noexcept {
    struct stat value {};
    if (::fstat(fd, &value) != 0) return false;
    return (directory ? S_ISDIR(value.st_mode) : S_ISREG(value.st_mode)) &&
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

void put_u16(std::array<uint8_t, manifest_bytes> & output, size_t & offset, uint16_t value) noexcept {
    output[offset++] = static_cast<uint8_t>(value >> 8);
    output[offset++] = static_cast<uint8_t>(value);
}

void put_u64(std::array<uint8_t, manifest_bytes> & output, size_t & offset, uint64_t value) noexcept {
    for (size_t index = 0; index < 8; ++index) output[offset + 7 - index] = static_cast<uint8_t>(value >> (index * 8));
    offset += 8;
}

uint16_t get_u16(const uint8_t * input) noexcept {
    return static_cast<uint16_t>((static_cast<uint16_t>(input[0]) << 8) | input[1]);
}

uint64_t get_u64(const uint8_t * input) noexcept {
    uint64_t value = 0;
    for (size_t index = 0; index < 8; ++index) value = (value << 8) | input[index];
    return value;
}

bool encode_tokens(const int32_t * tokens, size_t count, std::vector<uint8_t> & output) {
    if ((tokens == nullptr && count != 0) || count > context_store_linux_direct_max_tokens) return false;
    output.resize(count * 4);
    for (size_t index = 0; index < count; ++index) {
        const uint32_t value = static_cast<uint32_t>(tokens[index]);
        output[index * 4] = static_cast<uint8_t>(value >> 24);
        output[index * 4 + 1] = static_cast<uint8_t>(value >> 16);
        output[index * 4 + 2] = static_cast<uint8_t>(value >> 8);
        output[index * 4 + 3] = static_cast<uint8_t>(value);
    }
    return true;
}

bool build_manifest(
        const context_store_format_digest & key,
        const context_store_format_digest & scope,
        const context_store_format_digest & session,
        const context_store_format_digest & compatibility,
        const std::vector<uint8_t> & token_bytes,
        const uint8_t * state, size_t state_size,
        std::array<uint8_t, manifest_bytes> & output) noexcept {
    context_store_format_digest token_digest {}, state_digest {}, tag {};
    if (!context_store_sha256_bounded(token_bytes.data(), token_bytes.size(),
            context_store_linux_direct_max_tokens * 4ULL, token_digest) ||
        !context_store_sha256_bounded(state, state_size,
            context_store_linux_direct_max_state_bytes, state_digest)) return false;
    size_t offset = 0;
    std::copy(manifest_magic.begin(), manifest_magic.end(), output.begin() + offset); offset += manifest_magic.size();
    put_u16(output, offset, manifest_version);
    put_u16(output, offset, static_cast<uint16_t>(manifest_bytes));
    std::copy(scope.begin(), scope.end(), output.begin() + offset); offset += scope.size();
    std::copy(session.begin(), session.end(), output.begin() + offset); offset += session.size();
    std::copy(compatibility.begin(), compatibility.end(), output.begin() + offset); offset += compatibility.size();
    put_u64(output, offset, token_bytes.size() / 4);
    put_u64(output, offset, token_bytes.size());
    put_u64(output, offset, state_size);
    std::copy(token_digest.begin(), token_digest.end(), output.begin() + offset); offset += token_digest.size();
    std::copy(state_digest.begin(), state_digest.end(), output.begin() + offset); offset += state_digest.size();
    if (offset != manifest_auth_bytes || !context_store_hmac_sha256(
            key.data(), key.size(), output.data(), manifest_auth_bytes, tag)) return false;
    std::copy(tag.begin(), tag.end(), output.begin() + offset);
    wipe(token_digest.data(), token_digest.size());
    wipe(state_digest.data(), state_digest.size());
    wipe(tag.data(), tag.size());
    return true;
}

bool valid_file(int fd, uint64_t device, uint32_t uid, off_t size) noexcept {
    struct stat value {};
    return ::fstat(fd, &value) == 0 && S_ISREG(value.st_mode) &&
        static_cast<uint64_t>(value.st_dev) == device &&
        static_cast<uint32_t>(value.st_uid) == uid && (value.st_mode & 07777) == file_mode &&
        value.st_nlink == 1 && value.st_size == size;
}

bool session_layout_exact(int session_fd) noexcept {
    const int duplicate = ::fcntl(session_fd, F_DUPFD_CLOEXEC, 0);
    if (duplicate < 0) return false;
    DIR * directory = ::fdopendir(duplicate);
    if (directory == nullptr) { retry_close(duplicate); return false; }
    bool manifest = false, tokens = false, state = false;
    size_t count = 0;
    bool read_ok = true;
    for (;;) {
        errno = 0;
        dirent * entry = ::readdir(directory);
        if (entry == nullptr) { read_ok = errno == 0; break; }
        if (std::strcmp(entry->d_name, ".") == 0 || std::strcmp(entry->d_name, "..") == 0) continue;
        ++count;
        if (std::strcmp(entry->d_name, manifest_name) == 0 && !manifest) manifest = true;
        else if (std::strcmp(entry->d_name, tokens_name) == 0 && !tokens) tokens = true;
        else if (std::strcmp(entry->d_name, state_name) == 0 && !state) state = true;
        else { ::closedir(directory); return false; }
    }
    ::closedir(directory);
    return read_ok && count == 3 && manifest && tokens && state;
}

bool add_bytes(uint64_t value, uint64_t & total) noexcept {
    if (value > std::numeric_limits<uint64_t>::max() - total) return false;
    total += value;
    return true;
}

bool scan_files(int directory_fd, uint64_t & bytes, size_t & nodes, size_t node_limit) noexcept {
    const int duplicate = ::fcntl(directory_fd, F_DUPFD_CLOEXEC, 0);
    if (duplicate < 0) return false;
    DIR * directory = ::fdopendir(duplicate);
    if (directory == nullptr) { retry_close(duplicate); return false; }
    bool ok = true;
    for (;;) {
        errno = 0;
        dirent * entry = ::readdir(directory);
        if (entry == nullptr) { ok = errno == 0; break; }
        if (std::strcmp(entry->d_name, ".") == 0 || std::strcmp(entry->d_name, "..") == 0) continue;
        if (++nodes > node_limit) { ::closedir(directory); return false; }
        struct stat value {};
        if (::fstatat(directory_fd, entry->d_name, &value, AT_SYMLINK_NOFOLLOW) != 0 || value.st_size < 0 ||
            !add_bytes(static_cast<uint64_t>(value.st_size), bytes)) { ::closedir(directory); return false; }
    }
    ::closedir(directory);
    return ok;
}

bool scan_staging(int staging_fd, uint64_t & bytes, size_t & nodes, size_t node_limit) noexcept {
    const int duplicate = ::fcntl(staging_fd, F_DUPFD_CLOEXEC, 0);
    if (duplicate < 0) return false;
    DIR * directory = ::fdopendir(duplicate);
    if (directory == nullptr) { retry_close(duplicate); return false; }
    bool ok = true;
    for (;;) {
        errno = 0;
        dirent * entry = ::readdir(directory);
        if (entry == nullptr) { ok = errno == 0; break; }
        if (std::strcmp(entry->d_name, ".") == 0 || std::strcmp(entry->d_name, "..") == 0) continue;
        if (!exact_hex_name(entry->d_name) || ++nodes > node_limit) { ok = false; break; }
        const int stage_fd = open_directory_at(staging_fd, entry->d_name);
        if (stage_fd < 0 || !scan_files(stage_fd, bytes, nodes, node_limit)) {
            retry_close(stage_fd); ok = false; break;
        }
        retry_close(stage_fd);
    }
    ::closedir(directory);
    return ok;
}

bool scan_store(int root_fd, int staging_fd, size_t max_entries,
                uint64_t & bytes, size_t & entries) noexcept {
    bytes = 0; entries = 0;
    size_t nodes = 0;
    const size_t node_limit = max_entries * 8 + 16;
    if (!scan_staging(staging_fd, bytes, nodes, node_limit)) return false;
    const int duplicate = ::fcntl(root_fd, F_DUPFD_CLOEXEC, 0);
    if (duplicate < 0) return false;
    DIR * root = ::fdopendir(duplicate);
    if (root == nullptr) { retry_close(duplicate); return false; }
    bool root_ok = true;
    for (;;) {
        errno = 0;
        dirent * scope_entry = ::readdir(root);
        if (scope_entry == nullptr) { root_ok = errno == 0; break; }
        const char * name = scope_entry->d_name;
        if (std::strcmp(name, ".") == 0 || std::strcmp(name, "..") == 0 ||
            std::strcmp(name, lock_name) == 0 || std::strcmp(name, staging_name) == 0) continue;
        if (!exact_hex_name(name) || ++nodes > node_limit) { ::closedir(root); return false; }
        const int scope_fd = open_directory_at(root_fd, name);
        if (scope_fd < 0) { ::closedir(root); return false; }
        const int scope_duplicate = ::fcntl(scope_fd, F_DUPFD_CLOEXEC, 0);
        if (scope_duplicate < 0) { retry_close(scope_fd); ::closedir(root); return false; }
        DIR * scope = ::fdopendir(scope_duplicate);
        if (scope == nullptr) { retry_close(scope_duplicate); retry_close(scope_fd); ::closedir(root); return false; }
        bool scope_ok = true;
        for (;;) {
            errno = 0;
            dirent * session_entry = ::readdir(scope);
            if (session_entry == nullptr) { scope_ok = errno == 0; break; }
            const char * session_name = session_entry->d_name;
            if (std::strcmp(session_name, ".") == 0 || std::strcmp(session_name, "..") == 0) continue;
            if (!exact_hex_name(session_name) || ++entries > max_entries || ++nodes > node_limit) {
                ::closedir(scope); retry_close(scope_fd); ::closedir(root); return false;
            }
            const int session_fd = open_directory_at(scope_fd, session_name);
            if (session_fd < 0 || !scan_files(session_fd, bytes, nodes, node_limit)) {
                retry_close(session_fd); ::closedir(scope); retry_close(scope_fd); ::closedir(root); return false;
            }
            retry_close(session_fd);
        }
        ::closedir(scope); retry_close(scope_fd);
        if (!scope_ok) { ::closedir(root); return false; }
    }
    ::closedir(root);
    return root_ok;
}

bool reserve_allows(int root_fd, uint64_t reserve, uint64_t incoming) noexcept {
    struct statvfs value {};
    if (::fstatvfs(root_fd, &value) != 0 || value.f_frsize == 0 ||
        value.f_bavail > std::numeric_limits<uint64_t>::max() / value.f_frsize) return false;
    const uint64_t available = static_cast<uint64_t>(value.f_bavail) * value.f_frsize;
    return incoming <= available && reserve <= available - incoming;
}

bool create_synced_file(int directory_fd, const char * name,
                        const uint8_t * data, size_t size) noexcept {
    const int fd = open_regular_at(directory_fd, name, O_WRONLY | O_CREAT | O_EXCL, file_mode);
    if (fd < 0) return false;
    const bool ok = full_write(fd, data, size) && ::fdatasync(fd) == 0;
    const bool closed = retry_close(fd);
    return ok && closed;
}

void cleanup_stage(int staging_fd, const char * name, int stage_fd) noexcept {
    if (stage_fd >= 0) {
        ::unlinkat(stage_fd, manifest_name, 0);
        ::unlinkat(stage_fd, tokens_name, 0);
        ::unlinkat(stage_fd, state_name, 0);
        retry_close(stage_fd);
    }
    ::unlinkat(staging_fd, name, AT_REMOVEDIR);
}

bool random_stage_name(std::array<char, 65> & output) noexcept {
    std::array<uint8_t, 32> random {};
    size_t offset = 0;
    while (offset < random.size()) {
        const ssize_t count = ::getrandom(random.data() + offset, random.size() - offset, 0);
        if (count < 0 && errno == EINTR) continue;
        if (count <= 0) { wipe(random.data(), random.size()); return false; }
        offset += static_cast<size_t>(count);
    }
    context_store_format_digest digest {};
    const bool ok = context_store_sha256(random.data(), random.size(), digest);
    wipe(random.data(), random.size());
    if (!ok) return false;
    output = hex_name(digest);
    wipe(digest.data(), digest.size());
    return true;
}

} // namespace

context_store_linux_direct_identity_status context_store_linux_direct_inspect_root(
        const char * path, context_store_linux_direct_root_identity & output) noexcept {
    output = {};
    if (path == nullptr || path[0] != '/') return context_store_linux_direct_identity_status::invalid_path;
    const int fd = ::open(path, O_RDONLY | O_DIRECTORY | O_CLOEXEC | O_NOFOLLOW);
    if (fd < 0) return errno == ELOOP || errno == ENOTDIR ?
        context_store_linux_direct_identity_status::rejected : context_store_linux_direct_identity_status::io_error;
    struct stat value {};
    uint64_t mount = 0;
    const bool stat_ok = ::fstat(fd, &value) == 0;
    const bool mount_ok = mount_id_for_fd(fd, mount);
    retry_close(fd);
    if (!stat_ok) return context_store_linux_direct_identity_status::io_error;
    if (!S_ISDIR(value.st_mode) || (value.st_mode & 07777) != directory_mode)
        return context_store_linux_direct_identity_status::rejected;
    if (!mount_ok) return context_store_linux_direct_identity_status::unsupported;
    output.device = value.st_dev;
    output.mount_id = mount;
    output.owner_uid = value.st_uid;
    return context_store_linux_direct_identity_status::inspected;
}

context_store_linux_direct::~context_store_linux_direct() noexcept {
    wipe(master_key_.data(), master_key_.size());
    retry_close(lock_fd_); retry_close(staging_fd_); retry_close(root_fd_);
}

context_store_linux_direct::context_store_linux_direct(context_store_linux_direct && other) noexcept {
    *this = std::move(other);
}

context_store_linux_direct & context_store_linux_direct::operator=(context_store_linux_direct && other) noexcept {
    if (this != &other) {
        wipe(master_key_.data(), master_key_.size());
        retry_close(lock_fd_); retry_close(staging_fd_); retry_close(root_fd_);
        root_fd_ = other.root_fd_; staging_fd_ = other.staging_fd_; lock_fd_ = other.lock_fd_;
        device_ = other.device_; mount_id_ = other.mount_id_; owner_uid_ = other.owner_uid_;
        quota_bytes_ = other.quota_bytes_; reserve_bytes_ = other.reserve_bytes_;
        accounted_bytes_ = other.accounted_bytes_; max_entries_ = other.max_entries_;
        entry_count_ = other.entry_count_; master_key_ = other.master_key_;
        other.root_fd_ = other.staging_fd_ = other.lock_fd_ = -1;
        other.device_ = other.mount_id_ = other.owner_uid_ = other.quota_bytes_ = 0;
        other.reserve_bytes_ = other.accounted_bytes_ = other.max_entries_ = other.entry_count_ = 0;
        wipe(other.master_key_.data(), other.master_key_.size());
    }
    return *this;
}

bool context_store_linux_direct::available() const noexcept { return root_fd_ >= 0 && lock_fd_ >= 0; }
uint64_t context_store_linux_direct::accounted_bytes() const noexcept { return accounted_bytes_; }
size_t context_store_linux_direct::entry_count() const noexcept { return entry_count_; }

context_store_linux_direct_open_status context_store_linux_direct_open(
        const context_store_linux_direct_config & config,
        context_store_linux_direct & output) noexcept {
    if (output.available() || config.root_path == nullptr || config.root_path[0] != '/' ||
        config.master_key == nullptr || config.master_key_size != context_store_linux_direct_master_key_bytes ||
        config.quota_bytes == 0 || config.max_entries == 0 ||
        config.max_entries > context_store_linux_direct_max_entries_limit ||
        config.expected_root.device == 0 || config.expected_root.mount_id == 0) {
        return context_store_linux_direct_open_status::invalid_configuration;
    }
    const int root_fd = ::open(config.root_path, O_RDONLY | O_DIRECTORY | O_CLOEXEC | O_NOFOLLOW);
    if (root_fd < 0) return context_store_linux_direct_open_status::root_rejected;
    if (!exact_identity(root_fd, config.expected_root.device, config.expected_root.owner_uid, directory_mode, true) ||
        !same_mount(root_fd, config.expected_root.mount_id)) {
        retry_close(root_fd); return context_store_linux_direct_open_status::root_rejected;
    }
    const int lock_fd = open_regular_at(root_fd, lock_name, O_RDWR | O_CREAT, file_mode);
    if (lock_fd < 0 || !exact_identity(lock_fd, config.expected_root.device,
            config.expected_root.owner_uid, file_mode, false)) {
        retry_close(lock_fd); retry_close(root_fd); return context_store_linux_direct_open_status::root_rejected;
    }
    struct flock lock {};
    lock.l_type = F_WRLCK; lock.l_whence = SEEK_SET;
    int lock_result;
    do lock_result = ::fcntl(lock_fd, F_OFD_SETLK, &lock); while (lock_result != 0 && errno == EINTR);
    if (lock_result != 0 && (errno == EINVAL || errno == ENOSYS))
        do lock_result = ::flock(lock_fd, LOCK_EX | LOCK_NB); while (lock_result != 0 && errno == EINTR);
    if (lock_result != 0) {
        const int error = errno; retry_close(lock_fd); retry_close(root_fd);
        return error == EAGAIN || error == EACCES || error == EWOULDBLOCK ?
            context_store_linux_direct_open_status::writer_busy : context_store_linux_direct_open_status::io_error;
    }
    if (::mkdirat(root_fd, staging_name, directory_mode) != 0 && errno != EEXIST) {
        retry_close(lock_fd); retry_close(root_fd); return context_store_linux_direct_open_status::io_error;
    }
    const int staging_fd = open_directory_at(root_fd, staging_name);
    if (staging_fd < 0 || !exact_identity(staging_fd, config.expected_root.device,
            config.expected_root.owner_uid, directory_mode, true) ||
        !same_mount(staging_fd, config.expected_root.mount_id)) {
        retry_close(staging_fd); retry_close(lock_fd); retry_close(root_fd);
        return context_store_linux_direct_open_status::root_rejected;
    }
    if (::fsync(root_fd) != 0) {
        retry_close(staging_fd); retry_close(lock_fd); retry_close(root_fd);
        return context_store_linux_direct_open_status::io_error;
    }
    uint64_t accounted = 0; size_t entries = 0;
    if (!scan_store(root_fd, staging_fd, config.max_entries, accounted, entries) ||
        accounted > config.quota_bytes) {
        retry_close(staging_fd); retry_close(lock_fd); retry_close(root_fd);
        return context_store_linux_direct_open_status::accounting_rejected;
    }
    output.root_fd_ = root_fd; output.staging_fd_ = staging_fd; output.lock_fd_ = lock_fd;
    output.device_ = config.expected_root.device; output.mount_id_ = config.expected_root.mount_id;
    output.owner_uid_ = config.expected_root.owner_uid; output.quota_bytes_ = config.quota_bytes;
    output.reserve_bytes_ = config.reserve_bytes; output.accounted_bytes_ = accounted;
    output.max_entries_ = config.max_entries; output.entry_count_ = entries;
    std::copy_n(config.master_key, output.master_key_.size(), output.master_key_.begin());
    return context_store_linux_direct_open_status::opened;
}

context_store_linux_direct_lookup_status context_store_linux_direct::lookup(
        const context_store_format_digest & scope,
        const context_store_format_digest & session,
        const context_store_format_digest & compatibility,
        context_store_linux_direct_value & output) const noexcept {
    wipe_vector(output.tokens); wipe_vector(output.state);
    if (!available()) return context_store_linux_direct_lookup_status::unavailable;
    if (!nonzero(scope) || !nonzero(session) || !nonzero(compatibility))
        return context_store_linux_direct_lookup_status::invalid_request;
    const auto scope_name_value = hex_name(scope), session_name_value = hex_name(session);
    const int scope_fd = open_directory_at(root_fd_, scope_name_value.data());
    if (scope_fd < 0) return errno == ENOENT ? context_store_linux_direct_lookup_status::miss_not_found :
        context_store_linux_direct_lookup_status::miss_corrupt;
    if (!exact_identity(scope_fd, device_, owner_uid_, directory_mode, true) || !same_mount(scope_fd, mount_id_)) {
        retry_close(scope_fd); return context_store_linux_direct_lookup_status::miss_corrupt;
    }
    const int session_fd = open_directory_at(scope_fd, session_name_value.data());
    retry_close(scope_fd);
    if (session_fd < 0) return errno == ENOENT ? context_store_linux_direct_lookup_status::miss_not_found :
        context_store_linux_direct_lookup_status::miss_corrupt;
    if (!exact_identity(session_fd, device_, owner_uid_, directory_mode, true) ||
        !same_mount(session_fd, mount_id_) || !session_layout_exact(session_fd)) {
        retry_close(session_fd); return context_store_linux_direct_lookup_status::miss_corrupt;
    }
    std::array<uint8_t, manifest_bytes> manifest {};
    const int manifest_fd = open_regular_at(session_fd, manifest_name, O_RDONLY);
    if (manifest_fd < 0 || !valid_file(manifest_fd, device_, owner_uid_, manifest.size()) ||
        !exact_read(manifest_fd, manifest.data(), manifest.size())) {
        retry_close(manifest_fd); retry_close(session_fd); return context_store_linux_direct_lookup_status::miss_corrupt;
    }
    retry_close(manifest_fd);
    if (!std::equal(manifest_magic.begin(), manifest_magic.end(), manifest.begin()) ||
        get_u16(manifest.data() + 8) != manifest_version || get_u16(manifest.data() + 10) != manifest_bytes ||
        !constant_equal(manifest.data() + 12, scope.data(), 32) ||
        !constant_equal(manifest.data() + 44, session.data(), 32)) {
        retry_close(session_fd); return context_store_linux_direct_lookup_status::miss_corrupt;
    }
    const uint64_t token_count = get_u64(manifest.data() + 108);
    const uint64_t token_size = get_u64(manifest.data() + 116);
    const uint64_t state_size = get_u64(manifest.data() + 124);
    if (token_count > context_store_linux_direct_max_tokens || token_size != token_count * 4 ||
        state_size > context_store_linux_direct_max_state_bytes || token_size > SIZE_MAX || state_size > SIZE_MAX) {
        retry_close(session_fd); return context_store_linux_direct_lookup_status::miss_corrupt;
    }
    context_store_format_digest tag {};
    if (!context_store_hmac_sha256(master_key_.data(), master_key_.size(), manifest.data(), manifest_auth_bytes, tag) ||
        !constant_equal(tag.data(), manifest.data() + manifest_auth_bytes, tag.size())) {
        wipe(tag.data(), tag.size()); retry_close(session_fd); return context_store_linux_direct_lookup_status::miss_corrupt;
    }
    wipe(tag.data(), tag.size());
    if (!constant_equal(manifest.data() + 76, compatibility.data(), 32)) {
        retry_close(session_fd); return context_store_linux_direct_lookup_status::miss_incompatible;
    }
    try {
        std::vector<uint8_t> token_bytes(static_cast<size_t>(token_size));
        byte_vector_wipe_on_exit token_bytes_guard { token_bytes };
        output.state.resize(static_cast<size_t>(state_size));
        const int tokens_fd = open_regular_at(session_fd, tokens_name, O_RDONLY);
        const int state_fd = open_regular_at(session_fd, state_name, O_RDONLY);
        const bool files_ok = tokens_fd >= 0 && state_fd >= 0 &&
            valid_file(tokens_fd, device_, owner_uid_, static_cast<off_t>(token_size)) &&
            valid_file(state_fd, device_, owner_uid_, static_cast<off_t>(state_size)) &&
            exact_read(tokens_fd, token_bytes.data(), token_bytes.size()) &&
            exact_read(state_fd, output.state.data(), output.state.size());
        retry_close(tokens_fd); retry_close(state_fd); retry_close(session_fd);
        if (!files_ok) { wipe_vector(output.state); return context_store_linux_direct_lookup_status::miss_corrupt; }
        context_store_format_digest token_digest {}, state_digest {};
        const bool digests_ok = context_store_sha256_bounded(token_bytes.data(), token_bytes.size(),
                context_store_linux_direct_max_tokens * 4ULL, token_digest) &&
            context_store_sha256_bounded(output.state.data(), output.state.size(),
                context_store_linux_direct_max_state_bytes, state_digest) &&
            constant_equal(token_digest.data(), manifest.data() + 132, 32) &&
            constant_equal(state_digest.data(), manifest.data() + 164, 32);
        wipe(token_digest.data(), token_digest.size()); wipe(state_digest.data(), state_digest.size());
        if (!digests_ok) { wipe_vector(output.state); return context_store_linux_direct_lookup_status::miss_corrupt; }
        output.tokens.resize(static_cast<size_t>(token_count));
        for (size_t index = 0; index < output.tokens.size(); ++index) {
            const uint32_t value = (static_cast<uint32_t>(token_bytes[index * 4]) << 24) |
                (static_cast<uint32_t>(token_bytes[index * 4 + 1]) << 16) |
                (static_cast<uint32_t>(token_bytes[index * 4 + 2]) << 8) |
                token_bytes[index * 4 + 3];
            output.tokens[index] = static_cast<int32_t>(value);
        }
        return context_store_linux_direct_lookup_status::hit;
    } catch (...) {
        retry_close(session_fd); wipe_vector(output.tokens); wipe_vector(output.state);
        return context_store_linux_direct_lookup_status::unavailable;
    }
}

context_store_linux_direct_publish_status context_store_linux_direct::publish(
        const context_store_format_digest & scope,
        const context_store_format_digest & session,
        const context_store_format_digest & compatibility,
        const int32_t * tokens, size_t token_count,
        const uint8_t * state, size_t state_size) noexcept {
    if (!available()) return context_store_linux_direct_publish_status::unavailable;
    if (!nonzero(scope) || !nonzero(session) || !nonzero(compatibility) ||
        (tokens == nullptr && token_count != 0) || (state == nullptr && state_size != 0) ||
        (token_count == 0 && state_size == 0) || token_count > context_store_linux_direct_max_tokens ||
        state_size > context_store_linux_direct_max_state_bytes)
        return context_store_linux_direct_publish_status::invalid_request;
    try {
        const auto classify_existing = [&]() noexcept {
            context_store_linux_direct_value existing;
            const auto status = lookup(scope, session, compatibility, existing);
            bool equal = status == context_store_linux_direct_lookup_status::hit &&
                existing.tokens.size() == token_count && existing.state.size() == state_size;
            for (size_t index = 0; equal && index < token_count; ++index) {
                equal = existing.tokens[index] == tokens[index];
            }
            if (equal && state_size != 0) {
                equal = constant_equal(existing.state.data(), state, state_size);
            }
            wipe_vector(existing.tokens);
            wipe_vector(existing.state);
            if (status == context_store_linux_direct_lookup_status::miss_not_found) {
                return context_store_linux_direct_publish_status::published;
            }
            if (status == context_store_linux_direct_lookup_status::hit) {
                return equal ? context_store_linux_direct_publish_status::already_exists :
                               context_store_linux_direct_publish_status::conflict;
            }
            if (status == context_store_linux_direct_lookup_status::miss_incompatible ||
                status == context_store_linux_direct_lookup_status::miss_corrupt) {
                return context_store_linux_direct_publish_status::conflict;
            }
            return context_store_linux_direct_publish_status::unavailable;
        };
        const auto existing = classify_existing();
        if (existing != context_store_linux_direct_publish_status::published) return existing;
        if (entry_count_ >= max_entries_) return context_store_linux_direct_publish_status::quota_exceeded;
        std::vector<uint8_t> token_bytes;
        byte_vector_wipe_on_exit token_bytes_guard { token_bytes };
        if (!encode_tokens(tokens, token_count, token_bytes)) return context_store_linux_direct_publish_status::invalid_request;
        std::array<uint8_t, manifest_bytes> manifest {};
        struct manifest_wipe_on_exit {
            std::array<uint8_t, manifest_bytes> & value;
            ~manifest_wipe_on_exit() { wipe(value.data(), value.size()); }
        } manifest_guard { manifest };
        if (!build_manifest(master_key_, scope, session, compatibility, token_bytes, state, state_size, manifest))
            return context_store_linux_direct_publish_status::io_error;
        uint64_t incoming = manifest.size();
        if (!add_bytes(token_bytes.size(), incoming) || !add_bytes(state_size, incoming))
            return context_store_linux_direct_publish_status::invalid_request;
        if (incoming > quota_bytes_ || accounted_bytes_ > quota_bytes_ - incoming)
            return context_store_linux_direct_publish_status::quota_exceeded;
        if (!reserve_allows(root_fd_, reserve_bytes_, incoming))
            return context_store_linux_direct_publish_status::reserve_exhausted;
        const auto scope_name_value = hex_name(scope), session_name_value = hex_name(session);
        if (::mkdirat(root_fd_, scope_name_value.data(), directory_mode) != 0 && errno != EEXIST)
            return context_store_linux_direct_publish_status::io_error;
        const int scope_fd = open_directory_at(root_fd_, scope_name_value.data());
        if (scope_fd < 0 || !exact_identity(scope_fd, device_, owner_uid_, directory_mode, true) ||
            !same_mount(scope_fd, mount_id_)) {
            retry_close(scope_fd); return context_store_linux_direct_publish_status::io_error;
        }
        if (::fsync(root_fd_) != 0) { retry_close(scope_fd); return context_store_linux_direct_publish_status::io_error; }
        std::array<char, 65> stage_name {};
        int stage_fd = -1;
        for (size_t attempt = 0; attempt < 8 && stage_fd < 0; ++attempt) {
            if (!random_stage_name(stage_name)) { retry_close(scope_fd); return context_store_linux_direct_publish_status::io_error; }
            if (::mkdirat(staging_fd_, stage_name.data(), directory_mode) == 0)
                stage_fd = open_directory_at(staging_fd_, stage_name.data());
            else if (errno != EEXIST) { retry_close(scope_fd); return context_store_linux_direct_publish_status::io_error; }
        }
        if (stage_fd < 0 || !exact_identity(stage_fd, device_, owner_uid_, directory_mode, true) ||
            !same_mount(stage_fd, mount_id_)) {
            cleanup_stage(staging_fd_, stage_name.data(), stage_fd); retry_close(scope_fd);
            return context_store_linux_direct_publish_status::io_error;
        }
        if (!create_synced_file(stage_fd, tokens_name, token_bytes.data(), token_bytes.size()) ||
            !create_synced_file(stage_fd, state_name, state, state_size) ||
            !create_synced_file(stage_fd, manifest_name, manifest.data(), manifest.size()) ||
            ::fsync(stage_fd) != 0) {
            cleanup_stage(staging_fd_, stage_name.data(), stage_fd); retry_close(scope_fd);
            return context_store_linux_direct_publish_status::io_error;
        }
        if (!reserve_allows(root_fd_, reserve_bytes_, 0)) {
            cleanup_stage(staging_fd_, stage_name.data(), stage_fd); retry_close(scope_fd);
            return context_store_linux_direct_publish_status::reserve_exhausted;
        }
        retry_close(stage_fd); stage_fd = -1;
        int renamed;
        do renamed = static_cast<int>(::syscall(SYS_renameat2, staging_fd_, stage_name.data(),
            scope_fd, session_name_value.data(), RENAME_NOREPLACE)); while (renamed != 0 && errno == EINTR);
        if (renamed != 0) {
            const int error = errno; cleanup_stage(staging_fd_, stage_name.data(), -1); retry_close(scope_fd);
            if (error == EEXIST) {
                const auto collided = classify_existing();
                return collided == context_store_linux_direct_publish_status::published
                    ? context_store_linux_direct_publish_status::io_error
                    : collided;
            }
            if (error == ENOSYS || error == EINVAL) return context_store_linux_direct_publish_status::unsupported;
            return context_store_linux_direct_publish_status::io_error;
        }
        if (::fsync(scope_fd) != 0) { retry_close(scope_fd); return context_store_linux_direct_publish_status::io_error; }
        retry_close(scope_fd);
        accounted_bytes_ += incoming; ++entry_count_;
        return context_store_linux_direct_publish_status::published;
    } catch (...) {
        return context_store_linux_direct_publish_status::unavailable;
    }
}

const char * context_store_linux_direct_open_status_name(context_store_linux_direct_open_status status) noexcept {
    switch (status) {
        case context_store_linux_direct_open_status::opened: return "opened";
        case context_store_linux_direct_open_status::invalid_configuration: return "invalid-configuration";
        case context_store_linux_direct_open_status::root_rejected: return "root-rejected";
        case context_store_linux_direct_open_status::writer_busy: return "writer-busy";
        case context_store_linux_direct_open_status::accounting_rejected: return "accounting-rejected";
        case context_store_linux_direct_open_status::unsupported: return "unsupported";
        case context_store_linux_direct_open_status::io_error: return "io-error";
    } return "unknown";
}
const char * context_store_linux_direct_lookup_status_name(context_store_linux_direct_lookup_status status) noexcept {
    switch (status) {
        case context_store_linux_direct_lookup_status::hit: return "hit";
        case context_store_linux_direct_lookup_status::miss_not_found: return "miss-not-found";
        case context_store_linux_direct_lookup_status::miss_incompatible: return "miss-incompatible";
        case context_store_linux_direct_lookup_status::miss_corrupt: return "miss-corrupt";
        case context_store_linux_direct_lookup_status::invalid_request: return "invalid-request";
        case context_store_linux_direct_lookup_status::unavailable: return "unavailable";
    } return "unknown";
}
const char * context_store_linux_direct_publish_status_name(context_store_linux_direct_publish_status status) noexcept {
    switch (status) {
        case context_store_linux_direct_publish_status::published: return "published";
        case context_store_linux_direct_publish_status::already_exists: return "already-exists";
        case context_store_linux_direct_publish_status::conflict: return "conflict";
        case context_store_linux_direct_publish_status::invalid_request: return "invalid-request";
        case context_store_linux_direct_publish_status::quota_exceeded: return "quota-exceeded";
        case context_store_linux_direct_publish_status::reserve_exhausted: return "reserve-exhausted";
        case context_store_linux_direct_publish_status::unavailable: return "unavailable";
        case context_store_linux_direct_publish_status::unsupported: return "unsupported";
        case context_store_linux_direct_publish_status::io_error: return "io-error";
    } return "unknown";
}

} // namespace halofpx

#else

namespace halofpx {
context_store_linux_direct_identity_status context_store_linux_direct_inspect_root(
    const char *, context_store_linux_direct_root_identity & output) noexcept {
    output = {}; return context_store_linux_direct_identity_status::unsupported;
}
context_store_linux_direct::~context_store_linux_direct() noexcept = default;
context_store_linux_direct::context_store_linux_direct(context_store_linux_direct &&) noexcept = default;
context_store_linux_direct & context_store_linux_direct::operator=(context_store_linux_direct &&) noexcept = default;
bool context_store_linux_direct::available() const noexcept { return false; }
uint64_t context_store_linux_direct::accounted_bytes() const noexcept { return 0; }
size_t context_store_linux_direct::entry_count() const noexcept { return 0; }
context_store_linux_direct_lookup_status context_store_linux_direct::lookup(
    const context_store_format_digest &, const context_store_format_digest &,
    const context_store_format_digest &, context_store_linux_direct_value & output) const noexcept {
    output = {}; return context_store_linux_direct_lookup_status::unavailable;
}
context_store_linux_direct_publish_status context_store_linux_direct::publish(
    const context_store_format_digest &, const context_store_format_digest &,
    const context_store_format_digest &, const int32_t *, size_t, const uint8_t *, size_t) noexcept {
    return context_store_linux_direct_publish_status::unavailable;
}
context_store_linux_direct_open_status context_store_linux_direct_open(
    const context_store_linux_direct_config &, context_store_linux_direct &) noexcept {
    return context_store_linux_direct_open_status::unsupported;
}
const char * context_store_linux_direct_open_status_name(context_store_linux_direct_open_status) noexcept { return "unsupported"; }
const char * context_store_linux_direct_lookup_status_name(context_store_linux_direct_lookup_status) noexcept { return "unavailable"; }
const char * context_store_linux_direct_publish_status_name(context_store_linux_direct_publish_status) noexcept { return "unavailable"; }
} // namespace halofpx

#endif
