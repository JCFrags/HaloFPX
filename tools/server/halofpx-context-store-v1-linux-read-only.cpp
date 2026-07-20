#include "halofpx-context-store-v1-linux-read-only.h"

#if !defined(__linux__)
#error "The HaloFPX full-v1 filesystem reader is Linux-only"
#endif

#include <linux/openat2.h>
#include <linux/stat.h>
#include <sys/stat.h>
#include <sys/statfs.h>
#include <sys/syscall.h>
#include <unistd.h>

#include <array>
#include <cerrno>
#include <climits>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <fcntl.h>
#include <limits>
#include <memory>
#include <new>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace halofpx {
namespace {

constexpr uint32_t directory_mode = 0700;
constexpr uint32_t object_mode = 0600;

void wipe(void * data, size_t size) noexcept {
    volatile uint8_t * cursor = static_cast<volatile uint8_t *>(data);
    while (size-- != 0) {
        *cursor++ = 0;
    }
}

class fd_owner {
public:
    explicit fd_owner(int fd = -1) noexcept : fd_(fd) {}
    ~fd_owner() { if (fd_ >= 0) { ::close(fd_); } }
    fd_owner(const fd_owner &) = delete;
    fd_owner & operator=(const fd_owner &) = delete;
    fd_owner(fd_owner && other) noexcept : fd_(std::exchange(other.fd_, -1)) {}
    int get() const noexcept { return fd_; }
private:
    int fd_;
};

struct file_identity {
    uint64_t device = 0;
    uint64_t inode = 0;
    uint64_t mount_id = 0;
    uint64_t owner_uid = 0;
    uint32_t mode = 0;
    uint64_t links = 0;
    uint64_t size = 0;
    int64_t mtime_sec = 0;
    uint32_t mtime_nsec = 0;
    int64_t ctime_sec = 0;
    uint32_t ctime_nsec = 0;
};

struct io_result {
    context_store_lookup_status status = context_store_lookup_status::miss_storage;
    std::vector<uint8_t> bytes;
    bool ok = false;
};

bool same_digest(const context_store_format_digest & lhs,
                 const context_store_format_digest & rhs) noexcept {
    return std::memcmp(lhs.data(), rhs.data(), lhs.size()) == 0;
}

bool same_identity(const file_identity & lhs, const file_identity & rhs) noexcept {
    return lhs.device == rhs.device && lhs.inode == rhs.inode &&
        lhs.mount_id == rhs.mount_id && lhs.owner_uid == rhs.owner_uid &&
        lhs.mode == rhs.mode && lhs.links == rhs.links && lhs.size == rhs.size &&
        lhs.mtime_sec == rhs.mtime_sec && lhs.mtime_nsec == rhs.mtime_nsec &&
        lhs.ctime_sec == rhs.ctime_sec && lhs.ctime_nsec == rhs.ctime_nsec;
}

bool inspect_fd(int fd, file_identity & output) noexcept {
    struct stat value {};
    if (::fstat(fd, &value) != 0 || value.st_size < 0) {
        return false;
    }
    struct statx extended {};
    if (::syscall(SYS_statx, fd, "", AT_EMPTY_PATH | AT_STATX_SYNC_AS_STAT,
                  STATX_BASIC_STATS | STATX_MNT_ID, &extended) != 0 ||
        (extended.stx_mask & STATX_MNT_ID) == 0) {
        return false;
    }
    output.device = static_cast<uint64_t>(value.st_dev);
    output.inode = static_cast<uint64_t>(value.st_ino);
    output.mount_id = extended.stx_mnt_id;
    output.owner_uid = static_cast<uint64_t>(value.st_uid);
    output.mode = static_cast<uint32_t>(value.st_mode & 07777);
    output.links = static_cast<uint64_t>(value.st_nlink);
    output.size = static_cast<uint64_t>(value.st_size);
    output.mtime_sec = value.st_mtim.tv_sec;
    output.mtime_nsec = static_cast<uint32_t>(value.st_mtim.tv_nsec);
    output.ctime_sec = value.st_ctim.tv_sec;
    output.ctime_nsec = static_cast<uint32_t>(value.st_ctim.tv_nsec);
    return true;
}

context_store_lookup_status open_error(int value, bool missing_is_incomplete) noexcept {
    switch (value) {
        case ENOENT:
            return missing_is_incomplete ? context_store_lookup_status::miss_incomplete
                                         : context_store_lookup_status::miss_corrupt;
        case ENOSYS:
        case E2BIG:
        case EINVAL:
            return context_store_lookup_status::miss_unsupported;
        case ELOOP:
        case EXDEV:
        case ENOTDIR:
            return context_store_lookup_status::miss_corrupt;
        default:
            return context_store_lookup_status::miss_storage;
    }
}

int open_contained(int parent_fd, const char * name, uint64_t flags,
                   context_store_lookup_status & status,
                   bool missing_is_incomplete) noexcept {
    struct open_how how {};
    how.flags = flags | O_CLOEXEC | O_NOFOLLOW;
    how.resolve = RESOLVE_BENEATH | RESOLVE_NO_SYMLINKS |
                  RESOLVE_NO_MAGICLINKS | RESOLVE_NO_XDEV;
    const long result = ::syscall(SYS_openat2, parent_fd, name, &how, sizeof(how));
    if (result < 0 || result > INT_MAX) {
        status = open_error(errno, missing_is_incomplete);
        return -1;
    }
    return static_cast<int>(result);
}

std::string digest_name(const char * prefix, const context_store_format_digest & digest,
                        const char * suffix) {
    static constexpr char alphabet[] = "0123456789abcdef";
    std::string value(prefix);
    value.reserve(std::strlen(prefix) + digest.size() * 2 + std::strlen(suffix));
    for (uint8_t byte : digest) {
        value.push_back(alphabet[byte >> 4]);
        value.push_back(alphabet[byte & 0x0f]);
    }
    value.append(suffix);
    return value;
}

context_store_lookup_status verify_status(context_store_manifest_verify_status status) noexcept {
    switch (status) {
        case context_store_manifest_verify_status::authenticated_unadmitted:
            return context_store_lookup_status::hit;
        case context_store_manifest_verify_status::unknown_key:
        case context_store_manifest_verify_status::revoked_key:
        case context_store_manifest_verify_status::read_disabled_key:
        case context_store_manifest_verify_status::key_generation_mismatch:
        case context_store_manifest_verify_status::authority_mismatch:
            return context_store_lookup_status::miss_unauthorized;
        case context_store_manifest_verify_status::replay_mismatch:
            return context_store_lookup_status::miss_replay;
        case context_store_manifest_verify_status::compatibility_mismatch:
            return context_store_lookup_status::miss_incompatible;
        case context_store_manifest_verify_status::invalid_policy:
            return context_store_lookup_status::miss_unsupported;
        default:
            return context_store_lookup_status::miss_corrupt;
    }
}

io_result read_regular(int directory_fd, const std::string & name, uint64_t exact_size,
                       uint64_t maximum_size, uint64_t root_device,
                       uint64_t root_mount_id, uint64_t owner_uid) noexcept {
    io_result result;
    context_store_lookup_status status = context_store_lookup_status::miss_storage;
    fd_owner fd(open_contained(directory_fd, name.c_str(), O_RDONLY, status, true));
    if (fd.get() < 0) {
        result.status = status;
        return result;
    }
    file_identity before;
    if (!inspect_fd(fd.get(), before)) {
        result.status = errno == ENOSYS ? context_store_lookup_status::miss_unsupported
                                       : context_store_lookup_status::miss_storage;
        return result;
    }
    struct stat type_check {};
    if (::fstat(fd.get(), &type_check) != 0) {
        result.status = context_store_lookup_status::miss_storage;
        return result;
    }
    if (!S_ISREG(type_check.st_mode) || before.mode != object_mode || before.links != 1 ||
        before.owner_uid != owner_uid || before.device != root_device ||
        before.mount_id != root_mount_id || before.size == 0 ||
        before.size > maximum_size || (exact_size != 0 && before.size != exact_size) ||
        before.size > static_cast<uint64_t>(std::numeric_limits<size_t>::max()) ||
        before.size > static_cast<uint64_t>(std::numeric_limits<off_t>::max())) {
        result.status = before.size > maximum_size
            ? context_store_lookup_status::miss_unsupported
            : context_store_lookup_status::miss_corrupt;
        return result;
    }
    try {
        result.bytes.resize(static_cast<size_t>(before.size));
    } catch (...) {
        result.status = context_store_lookup_status::miss_storage;
        return result;
    }
    size_t offset = 0;
    while (offset < result.bytes.size()) {
        const ssize_t count = ::pread(fd.get(), result.bytes.data() + offset,
                                      result.bytes.size() - offset,
                                      static_cast<off_t>(offset));
        if (count < 0 && errno == EINTR) {
            continue;
        }
        if (count <= 0) {
            result.bytes.clear();
            result.status = count == 0 ? context_store_lookup_status::miss_incomplete
                                       : context_store_lookup_status::miss_storage;
            return result;
        }
        offset += static_cast<size_t>(count);
    }
    uint8_t trailing = 0;
    ssize_t trailing_count;
    do {
        trailing_count = ::pread(fd.get(), &trailing, 1, static_cast<off_t>(before.size));
    } while (trailing_count < 0 && errno == EINTR);
    if (trailing_count != 0) {
        result.bytes.clear();
        result.status = trailing_count < 0 ? context_store_lookup_status::miss_storage
                                           : context_store_lookup_status::miss_corrupt;
        return result;
    }
    file_identity after;
    if (!inspect_fd(fd.get(), after) || !same_identity(before, after)) {
        result.bytes.clear();
        result.status = context_store_lookup_status::miss_corrupt;
        return result;
    }
    result.status = context_store_lookup_status::hit;
    result.ok = true;
    return result;
}

class linux_v1_read_only_provider final : public context_store_provider {
public:
    explicit linux_v1_read_only_provider(const context_store_v1_linux_read_only_source & source)
        : root_fd_(::fcntl(source.root_fd, F_DUPFD_CLOEXEC, 3)),
          root_identity_(source.root_identity), policy_(source.verification_policy),
          admission_manifest_(source.admission.manifest),
          admission_objects_(source.admission.objects,
                             source.admission.objects + source.admission.object_count),
          object_limits_(source.object_limits), max_total_frame_bytes_(source.max_total_frame_bytes) {
        if (root_fd_.get() < 0 || source.verification_policy.key.master_key.data == nullptr ||
            source.verification_policy.key.master_key.size == 0 ||
            source.verification_policy.key.master_key.size > context_store_master_key_max_bytes ||
            source.admission.objects == nullptr || source.admission.object_count == 0 ||
            source.admission.object_count > context_store_manifest_max_objects ||
            source.root_identity.device == 0 || source.root_identity.inode == 0 ||
            source.root_identity.mount_id == 0 || source.root_identity.mode != directory_mode ||
            source.root_identity.filesystem_type == 0 || source.object_limits.max_frame_bytes == 0 ||
            source.object_limits.max_payload_bytes == 0 || source.max_total_frame_bytes == 0) {
            throw std::invalid_argument("invalid Linux full-v1 read-only source");
        }
        if (!root_matches()) {
            throw std::invalid_argument("Linux full-v1 snapshot root identity mismatch");
        }
        // Copy the secret only after every non-secret validation that can throw.
        // This is the constructor's final allocating action.
        key_.assign(source.verification_policy.key.master_key.data,
                    source.verification_policy.key.master_key.data +
                    source.verification_policy.key.master_key.size);
        policy_.key.master_key = { key_.data(), key_.size() };
    }

    ~linux_v1_read_only_provider() override {
        if (!key_.empty()) {
            wipe(key_.data(), key_.size());
        }
    }

    const char * name() const noexcept override { return "halofpx-v1-linux-read-only"; }
    context_store_capabilities capabilities() const noexcept override { return {}; }
    context_store_publish_status publish(const context_store_publish_request &) const noexcept override {
        return context_store_publish_status::disabled;
    }

    context_store_lookup_result lookup(const context_store_lookup_request & request) const noexcept override {
        try {
            if (!root_matches()) {
                return context_store_lookup_result::miss(context_store_lookup_status::miss_corrupt);
            }
            context_store_lookup_status status = context_store_lookup_status::miss_storage;
            fd_owner manifests(open_contained(root_fd_.get(), "manifests",
                O_RDONLY | O_DIRECTORY, status, false));
            if (manifests.get() < 0 || !directory_matches(manifests.get())) {
                return context_store_lookup_result::miss(manifests.get() < 0 ? status : context_store_lookup_status::miss_corrupt);
            }
            fd_owner objects(open_contained(root_fd_.get(), "objects",
                O_RDONLY | O_DIRECTORY, status, false));
            if (objects.get() < 0 || !directory_matches(objects.get())) {
                return context_store_lookup_result::miss(objects.get() < 0 ? status : context_store_lookup_status::miss_corrupt);
            }

            const std::string manifest_name = digest_name("m-", policy_.anchor.selected_manifest_digest, ".cbor");
            io_result manifest = read_regular(manifests.get(), manifest_name, 0,
                context_store_manifest_max_bytes, root_identity_.device,
                root_identity_.mount_id, root_identity_.owner_uid);
            if (!manifest.ok) {
                return context_store_lookup_result::miss(manifest.status);
            }
            const context_store_manifest_verify_result verified = context_store_verify_manifest_v1(
                manifest.bytes.data(), manifest.bytes.size(), policy_);
            const context_store_lookup_status verified_status = verify_status(verified.status);
            if (verified_status != context_store_lookup_status::hit ||
                !same_digest(verified.manifest_digest, policy_.anchor.selected_manifest_digest)) {
                return context_store_lookup_result::miss(
                    verified_status == context_store_lookup_status::hit
                        ? context_store_lookup_status::miss_corrupt : verified_status);
            }

            std::vector<std::vector<uint8_t>> frames;
            std::vector<context_store_v1_frame_view> views;
            frames.reserve(verified.authenticated_object_count());
            views.reserve(verified.authenticated_object_count());
            uint64_t aggregate = 0;
            for (size_t i = 0; i < verified.authenticated_object_count(); ++i) {
                const context_store_object_reference * descriptor = verified.authenticated_object_reference(i);
                if (descriptor == nullptr || descriptor->frame_bytes == 0 ||
                    descriptor->frame_bytes > object_limits_.max_frame_bytes ||
                    descriptor->frame_bytes > max_total_frame_bytes_ - aggregate) {
                    return context_store_lookup_result::miss(context_store_lookup_status::miss_unsupported);
                }
                aggregate += descriptor->frame_bytes;
                const std::string object_name = digest_name("o-", descriptor->object_id, ".bin");
                io_result frame = read_regular(objects.get(), object_name, descriptor->frame_bytes,
                    object_limits_.max_frame_bytes, root_identity_.device,
                    root_identity_.mount_id, root_identity_.owner_uid);
                if (!frame.ok) {
                    return context_store_lookup_result::miss(frame.status);
                }
                frames.push_back(std::move(frame.bytes));
            }
            for (const auto & frame : frames) {
                views.push_back({ frame.data(), frame.size() });
            }
            context_store_v1_read_only_source source;
            source.manifest_data = manifest.bytes.data();
            source.manifest_size = manifest.bytes.size();
            source.verification_policy = policy_;
            source.admission.manifest = admission_manifest_;
            source.admission.objects = admission_objects_.data();
            source.admission.object_count = admission_objects_.size();
            source.frames = views.data();
            source.frame_count = views.size();
            source.object_limits = object_limits_;
            source.max_total_frame_bytes = max_total_frame_bytes_;
            auto provider = make_context_store_v1_read_only_provider(source);
            return provider->lookup(request);
        } catch (...) {
            return context_store_lookup_result::miss(context_store_lookup_status::miss_storage);
        }
    }

private:
    bool root_matches() const noexcept {
        file_identity actual;
        struct stat value {};
        struct statfs filesystem {};
        return inspect_fd(root_fd_.get(), actual) && ::fstat(root_fd_.get(), &value) == 0 &&
            ::fstatfs(root_fd_.get(), &filesystem) == 0 && S_ISDIR(value.st_mode) &&
            actual.device == root_identity_.device && actual.inode == root_identity_.inode &&
            actual.mount_id == root_identity_.mount_id && actual.owner_uid == root_identity_.owner_uid &&
            actual.mode == root_identity_.mode &&
            static_cast<uint64_t>(filesystem.f_type) == root_identity_.filesystem_type;
    }

    bool directory_matches(int fd) const noexcept {
        file_identity actual;
        struct stat value {};
        return inspect_fd(fd, actual) && ::fstat(fd, &value) == 0 && S_ISDIR(value.st_mode) &&
            actual.device == root_identity_.device && actual.mount_id == root_identity_.mount_id &&
            actual.owner_uid == root_identity_.owner_uid && actual.mode == directory_mode;
    }

    fd_owner root_fd_;
    context_store_linux_root_identity_v1 root_identity_;
    context_store_manifest_verification_policy policy_;
    context_store_authenticated_manifest_metadata admission_manifest_;
    std::vector<context_store_object_reference> admission_objects_;
    context_store_object_limits object_limits_;
    uint64_t max_total_frame_bytes_;
    std::vector<uint8_t> key_;
};

} // namespace

std::unique_ptr<context_store_provider> make_context_store_v1_linux_read_only_provider(
        const context_store_v1_linux_read_only_source & source) {
    if (source.root_fd < 0 || source.verification_policy.key.master_key.data == nullptr ||
        source.verification_policy.key.master_key.size == 0 ||
        source.verification_policy.key.master_key.size > context_store_master_key_max_bytes ||
        source.admission.objects == nullptr || source.admission.object_count == 0 ||
        source.admission.object_count > context_store_manifest_max_objects ||
        source.root_identity.device == 0 || source.root_identity.inode == 0 ||
        source.root_identity.mount_id == 0 || source.root_identity.mode != directory_mode ||
        source.root_identity.filesystem_type == 0 || source.object_limits.max_frame_bytes == 0 ||
        source.object_limits.max_payload_bytes == 0 || source.max_total_frame_bytes == 0) {
        throw std::invalid_argument("invalid Linux full-v1 read-only source pointers");
    }
    return std::make_unique<linux_v1_read_only_provider>(source);
}

} // namespace halofpx
