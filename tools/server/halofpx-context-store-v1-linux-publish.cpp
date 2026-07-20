#include "halofpx-context-store-v1-linux-publish.h"

#if !defined(__linux__)
#error "The HaloFPX full-v1 synthetic snapshot materializer is Linux-only"
#endif

#include <linux/openat2.h>
#include <linux/stat.h>
#include <sys/stat.h>
#include <sys/statfs.h>
#include <sys/syscall.h>
#include <unistd.h>

#include <atomic>
#include <algorithm>
#include <cerrno>
#include <climits>
#include <cstddef>
#include <cstring>
#include <fcntl.h>
#include <limits>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace halofpx {
namespace {

constexpr uint32_t directory_mode = 0700;
constexpr uint32_t object_mode = 0600;

class fd_owner {
public:
    explicit fd_owner(int fd = -1) noexcept : fd_(fd) {}
    ~fd_owner() { if (fd_ >= 0) ::close(fd_); }
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

bool inspect_fd(int fd, file_identity & output) noexcept {
    struct stat value {};
    struct statx extended {};
    if (::fstat(fd, &value) != 0 || value.st_size < 0 ||
        ::syscall(SYS_statx, fd, "", AT_EMPTY_PATH | AT_STATX_SYNC_AS_STAT,
                  STATX_BASIC_STATS | STATX_MNT_ID, &extended) != 0 ||
        (extended.stx_mask & STATX_MNT_ID) == 0) return false;
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

bool same_identity(const file_identity & a, const file_identity & b) noexcept {
    return a.device == b.device && a.inode == b.inode && a.mount_id == b.mount_id &&
        a.owner_uid == b.owner_uid && a.mode == b.mode && a.links == b.links &&
        a.size == b.size && a.mtime_sec == b.mtime_sec && a.mtime_nsec == b.mtime_nsec &&
        a.ctime_sec == b.ctime_sec && a.ctime_nsec == b.ctime_nsec;
}

bool nonzero(const context_store_v1_publish_attempt_id & id) noexcept {
    uint8_t value = 0;
    for (uint8_t byte : id) value |= byte;
    return value != 0;
}

std::string hex(const uint8_t * data, size_t size) {
    static constexpr char alphabet[] = "0123456789abcdef";
    std::string result;
    result.reserve(size * 2);
    for (size_t i = 0; i < size; ++i) {
        result.push_back(alphabet[data[i] >> 4]);
        result.push_back(alphabet[data[i] & 15]);
    }
    return result;
}

std::string object_name(const context_store_format_digest & digest) {
    return "o-" + hex(digest.data(), digest.size()) + ".bin";
}

std::string manifest_name(const context_store_format_digest & digest) {
    return "m-" + hex(digest.data(), digest.size()) + ".cbor";
}

int open_contained(int parent, const char * name, uint64_t flags, mode_t mode = 0) noexcept {
    struct open_how how {};
    how.flags = flags | O_CLOEXEC | O_NOFOLLOW;
    how.mode = mode;
    how.resolve = RESOLVE_BENEATH | RESOLVE_NO_SYMLINKS |
                  RESOLVE_NO_MAGICLINKS | RESOLVE_NO_XDEV;
    const long result = ::syscall(SYS_openat2, parent, name, &how, sizeof(how));
    return result >= 0 && result <= INT_MAX ? static_cast<int>(result) : -1;
}

context_store_v1_linux_publish_status open_failure() noexcept {
    switch (errno) {
        case ENOSYS: case E2BIG: case EINVAL: return context_store_v1_linux_publish_status::unsupported;
        case EEXIST: return context_store_v1_linux_publish_status::conflict;
        default: return context_store_v1_linux_publish_status::storage;
    }
}

bool sync_fd(int fd) noexcept {
    int result;
    do result = ::fsync(fd); while (result != 0 && errno == EINTR);
    return result == 0;
}

bool exact_file_bytes(int fd, const uint8_t * expected, size_t size) noexcept {
    std::array<uint8_t, 4096> buffer {};
    size_t offset = 0;
    while (offset < size) {
        const size_t wanted = std::min(buffer.size(), size - offset);
        const ssize_t count = ::pread(fd, buffer.data(), wanted, static_cast<off_t>(offset));
        if (count < 0 && errno == EINTR) continue;
        if (count <= 0 || static_cast<size_t>(count) > wanted ||
            std::memcmp(buffer.data(), expected + offset, static_cast<size_t>(count)) != 0)
            return false;
        offset += static_cast<size_t>(count);
    }
    uint8_t trailing = 0;
    ssize_t trailing_count;
    do trailing_count = ::pread(fd, &trailing, 1, static_cast<off_t>(size));
    while (trailing_count < 0 && errno == EINTR);
    return trailing_count == 0;
}

enum class existing_state { absent, equal, conflict, storage };

existing_state inspect_existing_file(int directory, const std::string & name,
        const uint8_t * data, size_t size,
        const context_store_linux_root_identity_v1 & root) noexcept {
    fd_owner file(open_contained(directory, name.c_str(), O_RDONLY));
    if (file.get() < 0) {
        if (errno == ENOENT) return existing_state::absent;
        if (errno == ELOOP || errno == EXDEV || errno == ENOTDIR)
            return existing_state::conflict;
        return existing_state::storage;
    }
    file_identity before, after;
    struct stat type {};
    if (!inspect_fd(file.get(), before) || ::fstat(file.get(), &type) != 0 ||
        !S_ISREG(type.st_mode) || before.device != root.device ||
        before.mount_id != root.mount_id || before.owner_uid != root.owner_uid ||
        before.mode != object_mode || before.links != 1 || before.size != size ||
        !exact_file_bytes(file.get(), data, size)) return existing_state::conflict;
    if (!sync_fd(file.get())) return existing_state::storage;
    return inspect_fd(file.get(), after) && same_identity(before, after)
        ? existing_state::equal : existing_state::conflict;
}

struct owned_material {
    std::vector<uint8_t> manifest;
    std::vector<std::vector<uint8_t>> frames;
    std::vector<context_store_v1_frame_view> views;
    std::vector<context_store_object_reference> admission_objects;
    context_store_authenticated_manifest_metadata admission_manifest;
    context_store_format_digest selected_manifest_digest {};
};

bool basic_source_valid(const context_store_v1_read_only_source & source) noexcept {
    if (!source.manifest_data || source.manifest_size == 0 ||
        source.manifest_size > context_store_manifest_max_bytes || !source.frames ||
        source.frame_count == 0 || source.frame_count > context_store_manifest_max_objects ||
        !source.admission.objects || source.admission.object_count != source.frame_count ||
        !source.verification_policy.key.master_key.data ||
        source.verification_policy.key.master_key.size == 0 ||
        source.verification_policy.key.master_key.size > context_store_master_key_max_bytes ||
        source.object_limits.max_frame_bytes == 0 ||
        source.object_limits.max_payload_bytes == 0 || source.max_total_frame_bytes == 0) return false;
    uint64_t total = 0;
    for (size_t i = 0; i < source.frame_count; ++i) {
        if (!source.frames[i].data || source.frames[i].size == 0 ||
            source.frames[i].size > source.object_limits.max_frame_bytes ||
            source.frames[i].size > source.max_total_frame_bytes - total) return false;
        total += source.frames[i].size;
    }
    return true;
}

context_store_v1_linux_publish_status validate_and_copy(
        const context_store_v1_read_only_source & source, owned_material & material) noexcept {
    if (!basic_source_valid(source)) return context_store_v1_linux_publish_status::invalid;
    try {
        material.manifest.assign(source.manifest_data, source.manifest_data + source.manifest_size);
        material.admission_manifest = source.admission.manifest;
        material.admission_objects.assign(source.admission.objects,
            source.admission.objects + source.admission.object_count);
        material.frames.reserve(source.frame_count);
        material.views.reserve(source.frame_count);
        for (size_t i = 0; i < source.frame_count; ++i) {
            material.frames.emplace_back(source.frames[i].data,
                source.frames[i].data + source.frames[i].size);
        }
        for (const auto & frame : material.frames) material.views.push_back({frame.data(), frame.size()});

        context_store_v1_read_only_source copy = source;
        copy.manifest_data = material.manifest.data();
        copy.manifest_size = material.manifest.size();
        copy.admission.manifest = material.admission_manifest;
        copy.admission.objects = material.admission_objects.data();
        copy.admission.object_count = material.admission_objects.size();
        copy.frames = material.views.data();
        copy.frame_count = material.views.size();
        auto provider = make_context_store_v1_read_only_provider(copy);
        context_store_lookup_request request;
        request.identity.compatibility_root = material.admission_manifest.compatibility_root;
        request.identity.scope_namespace = material.admission_manifest.scope_namespace;
        request.identity.checkpoint_lineage_id = material.admission_manifest.checkpoint_lineage_id;
        request.identity.policy_epoch = material.admission_manifest.policy_epoch;
        auto result = provider->lookup(request);
        const auto * candidate = dynamic_cast<const context_store_v1_read_only_candidate *>(result.candidate());
        if (!result.is_hit() || !candidate ||
            candidate->manifest_digest() != source.verification_policy.anchor.selected_manifest_digest ||
            candidate->object_count() != material.frames.size()) {
            return context_store_v1_linux_publish_status::verification;
        }
        material.selected_manifest_digest = candidate->manifest_digest();
        return context_store_v1_linux_publish_status::materialized_non_authoritative;
    } catch (const std::invalid_argument &) {
        return context_store_v1_linux_publish_status::invalid;
    } catch (...) {
        return context_store_v1_linux_publish_status::storage;
    }
}

class temporary_file {
public:
    temporary_file(int directory, const std::string & name) noexcept : directory_(directory), name_(name) {}
    ~temporary_file() { if (armed_) ::unlinkat(directory_, name_.c_str(), 0); }
    void disarm() noexcept { armed_ = false; }
private:
    int directory_;
    const std::string & name_;
    bool armed_ = true;
};

context_store_v1_linux_publish_status publish_file(
        int staging, int destination, const std::string & temporary,
        const std::string & final_name, const uint8_t * data, size_t size,
        const context_store_linux_root_identity_v1 & root) noexcept {
    fd_owner file(open_contained(staging, temporary.c_str(),
        O_RDWR | O_CREAT | O_EXCL, object_mode));
    if (file.get() < 0) return open_failure();
    temporary_file cleanup(staging, temporary);
    file_identity created;
    struct stat type {};
    if (!inspect_fd(file.get(), created) || ::fstat(file.get(), &type) != 0 ||
        !S_ISREG(type.st_mode) || created.device != root.device ||
        created.mount_id != root.mount_id || created.owner_uid != root.owner_uid ||
        created.mode != object_mode || created.links != 1 || created.size != 0 ||
        size > static_cast<size_t>(std::numeric_limits<off_t>::max()))
        return context_store_v1_linux_publish_status::verification;
    size_t offset = 0;
    while (offset < size) {
        const size_t chunk = std::min(size - offset,
            static_cast<size_t>(std::numeric_limits<ssize_t>::max()));
        const ssize_t count = ::pwrite(file.get(), data + offset, chunk, static_cast<off_t>(offset));
        if (count < 0 && errno == EINTR) continue;
        if (count <= 0) return context_store_v1_linux_publish_status::storage;
        offset += static_cast<size_t>(count);
    }
    file_identity before_sync, after_sync;
    if (!inspect_fd(file.get(), before_sync) || before_sync.device != root.device ||
        before_sync.mount_id != root.mount_id || before_sync.owner_uid != root.owner_uid ||
        before_sync.mode != object_mode || before_sync.links != 1 || before_sync.size != size)
        return context_store_v1_linux_publish_status::verification;
    if (!sync_fd(file.get())) return context_store_v1_linux_publish_status::synchronization;
    if (!exact_file_bytes(file.get(), data, size) || !inspect_fd(file.get(), after_sync) ||
        !same_identity(before_sync, after_sync))
        return context_store_v1_linux_publish_status::verification;
    if (::syscall(SYS_renameat2, staging, temporary.c_str(), destination,
                  final_name.c_str(), RENAME_NOREPLACE) != 0) {
        const int rename_error = errno;
        if (rename_error == EEXIST &&
            inspect_existing_file(destination, final_name, data, size, root) == existing_state::equal)
            return context_store_v1_linux_publish_status::already_equal_non_authoritative;
        errno = rename_error;
        return open_failure();
    }
    cleanup.disarm();
    return context_store_v1_linux_publish_status::materialized_non_authoritative;
}

} // namespace

class context_store_v1_linux_snapshot_materializer::implementation {
public:
    implementation(int fd, context_store_linux_root_identity_v1 identity)
        : root(fd), expected(identity) {}

    bool root_matches() const noexcept {
        file_identity actual;
        struct stat type {};
        struct statfs filesystem {};
        return inspect_fd(root.get(), actual) && ::fstat(root.get(), &type) == 0 &&
            ::fstatfs(root.get(), &filesystem) == 0 && S_ISDIR(type.st_mode) &&
            actual.device == expected.device && actual.inode == expected.inode &&
            actual.mount_id == expected.mount_id && actual.owner_uid == expected.owner_uid &&
            actual.mode == expected.mode &&
            static_cast<uint64_t>(filesystem.f_type) == expected.filesystem_type;
    }

    bool directory_matches(int fd) const noexcept {
        file_identity actual;
        struct stat type {};
        struct statfs filesystem {};
        return inspect_fd(fd, actual) && ::fstat(fd, &type) == 0 &&
            ::fstatfs(fd, &filesystem) == 0 && S_ISDIR(type.st_mode) &&
            actual.device == expected.device && actual.mount_id == expected.mount_id &&
            actual.owner_uid == expected.owner_uid && actual.mode == directory_mode &&
            static_cast<uint64_t>(filesystem.f_type) == expected.filesystem_type;
    }

    fd_owner root;
    context_store_linux_root_identity_v1 expected;
    std::atomic_flag writer = ATOMIC_FLAG_INIT;
};

context_store_v1_linux_snapshot_materializer::context_store_v1_linux_snapshot_materializer(
        std::unique_ptr<implementation> implementation) noexcept
    : implementation_(std::move(implementation)) {}

context_store_v1_linux_snapshot_materializer::~context_store_v1_linux_snapshot_materializer() = default;

context_store_v1_linux_publish_status context_store_v1_linux_snapshot_materializer::publish(
        const context_store_v1_publish_attempt_id & attempt_id,
        const context_store_v1_read_only_source & source) noexcept {
    if (!implementation_ || !nonzero(attempt_id)) return context_store_v1_linux_publish_status::invalid;
    if (implementation_->writer.test_and_set(std::memory_order_acquire))
        return context_store_v1_linux_publish_status::busy;
    struct release_fence {
        std::atomic_flag & flag;
        ~release_fence() { flag.clear(std::memory_order_release); }
    } release { implementation_->writer };
    bool any_material_visible = false;
    try {
        owned_material material;
        auto status = validate_and_copy(source, material);
        if (status != context_store_v1_linux_publish_status::materialized_non_authoritative) return status;
        if (!implementation_->root_matches()) return context_store_v1_linux_publish_status::verification;

        fd_owner writer_lock(open_contained(implementation_->root.get(), "writer.lock", O_RDWR));
        if (writer_lock.get() < 0) return open_failure();
        file_identity lock_identity;
        struct stat lock_type {};
        if (!inspect_fd(writer_lock.get(), lock_identity) || ::fstat(writer_lock.get(), &lock_type) != 0 ||
            !S_ISREG(lock_type.st_mode) || lock_identity.device != implementation_->expected.device ||
            lock_identity.mount_id != implementation_->expected.mount_id ||
            lock_identity.owner_uid != implementation_->expected.owner_uid ||
            lock_identity.mode != object_mode || lock_identity.links != 1)
            return context_store_v1_linux_publish_status::verification;
        struct flock lock {};
        lock.l_type = F_WRLCK;
        lock.l_whence = SEEK_SET;
        lock.l_start = 0;
        lock.l_len = 0;
        int lock_result;
        do lock_result = ::fcntl(writer_lock.get(), F_OFD_SETLK, &lock);
        while (lock_result != 0 && errno == EINTR);
        if (lock_result != 0) {
            if (errno == EAGAIN || errno == EACCES) return context_store_v1_linux_publish_status::busy;
            if (errno == EINVAL) return context_store_v1_linux_publish_status::unsupported;
            return context_store_v1_linux_publish_status::storage;
        }

        fd_owner staging(open_contained(implementation_->root.get(), "staging", O_RDONLY | O_DIRECTORY));
        if (staging.get() < 0) return open_failure();
        fd_owner objects(open_contained(implementation_->root.get(), "objects", O_RDONLY | O_DIRECTORY));
        if (objects.get() < 0) return open_failure();
        fd_owner manifests(open_contained(implementation_->root.get(), "manifests", O_RDONLY | O_DIRECTORY));
        if (manifests.get() < 0) return open_failure();
        if (!implementation_->directory_matches(staging.get()) ||
            !implementation_->directory_matches(objects.get()) ||
            !implementation_->directory_matches(manifests.get()))
            return context_store_v1_linux_publish_status::verification;

        const std::string attempt = hex(attempt_id.data(), attempt_id.size());
        const std::string final_manifest = manifest_name(material.selected_manifest_digest);
        size_t equal_files = 0;
        size_t absent_files = 0;
        for (size_t i = 0; i < material.frames.size(); ++i) {
            const existing_state state = inspect_existing_file(objects.get(),
                object_name(material.admission_objects[i].object_id),
                material.frames[i].data(), material.frames[i].size(), implementation_->expected);
            if (state == existing_state::equal) ++equal_files;
            else if (state == existing_state::absent) ++absent_files;
            else return state == existing_state::storage
                ? context_store_v1_linux_publish_status::storage
                : context_store_v1_linux_publish_status::conflict;
        }
        const existing_state manifest_state = inspect_existing_file(manifests.get(), final_manifest,
            material.manifest.data(), material.manifest.size(), implementation_->expected);
        if (manifest_state == existing_state::equal) ++equal_files;
        else if (manifest_state == existing_state::absent) ++absent_files;
        else return manifest_state == existing_state::storage
            ? context_store_v1_linux_publish_status::storage
            : context_store_v1_linux_publish_status::conflict;
        if (equal_files != 0 || absent_files != material.frames.size() + 1) {
            if (absent_files != 0) return context_store_v1_linux_publish_status::conflict;
            if (!sync_fd(objects.get()) || !sync_fd(manifests.get()) ||
                !implementation_->directory_matches(objects.get()) ||
                !implementation_->directory_matches(manifests.get()))
                return context_store_v1_linux_publish_status::incomplete_or_uncertain_discard_root;
            return context_store_v1_linux_publish_status::already_equal_non_authoritative;
        }

        for (size_t i = 0; i < material.frames.size(); ++i) {
            const auto & digest = material.admission_objects[i].object_id;
            const std::string final_name = object_name(digest);
            const std::string temporary = "p-" + attempt + "-" + final_name + ".tmp";
            status = publish_file(staging.get(), objects.get(), temporary, final_name,
                material.frames[i].data(), material.frames[i].size(), implementation_->expected);
            if (status == context_store_v1_linux_publish_status::materialized_non_authoritative) {
                any_material_visible = true;
            } else {
                return context_store_v1_linux_publish_status::incomplete_or_uncertain_discard_root;
            }
        }
        if (!sync_fd(objects.get()) || !sync_fd(staging.get()))
            return context_store_v1_linux_publish_status::incomplete_or_uncertain_discard_root;
        if (!implementation_->root_matches() || !implementation_->directory_matches(manifests.get()))
            return context_store_v1_linux_publish_status::incomplete_or_uncertain_discard_root;

        const std::string temporary_manifest = "p-" + attempt + "-" + final_manifest + ".tmp";
        status = publish_file(staging.get(), manifests.get(), temporary_manifest, final_manifest,
            material.manifest.data(), material.manifest.size(), implementation_->expected);
        if (status == context_store_v1_linux_publish_status::materialized_non_authoritative) {
            any_material_visible = true;
        } else {
            return context_store_v1_linux_publish_status::incomplete_or_uncertain_discard_root;
        }
        if (!sync_fd(manifests.get()) || !sync_fd(staging.get()))
            return context_store_v1_linux_publish_status::incomplete_or_uncertain_discard_root;
        return context_store_v1_linux_publish_status::materialized_non_authoritative;
    } catch (...) {
        return any_material_visible
            ? context_store_v1_linux_publish_status::incomplete_or_uncertain_discard_root
            : context_store_v1_linux_publish_status::storage;
    }
}

std::unique_ptr<context_store_v1_linux_snapshot_materializer>
make_context_store_v1_linux_snapshot_materializer(const context_store_v1_linux_publish_root & root) {
    if (root.root_fd < 0 || root.identity.device == 0 || root.identity.inode == 0 ||
        root.identity.mount_id == 0 ||
        root.identity.mode != directory_mode || root.identity.filesystem_type == 0)
        throw std::invalid_argument("invalid Linux full-v1 publication root");
    const int duplicate = ::fcntl(root.root_fd, F_DUPFD_CLOEXEC, 3);
    if (duplicate < 0) throw std::invalid_argument("cannot duplicate Linux full-v1 publication root");
    auto implementation = std::make_unique<context_store_v1_linux_snapshot_materializer::implementation>(
        duplicate, root.identity);
    if (!implementation->root_matches())
        throw std::invalid_argument("Linux full-v1 publication root identity mismatch");
    return std::unique_ptr<context_store_v1_linux_snapshot_materializer>(
        new context_store_v1_linux_snapshot_materializer(std::move(implementation)));
}

const char * context_store_v1_linux_publish_status_name(
        context_store_v1_linux_publish_status status) noexcept {
    switch (status) {
        case context_store_v1_linux_publish_status::materialized_non_authoritative: return "materialized_non_authoritative";
        case context_store_v1_linux_publish_status::already_equal_non_authoritative: return "already_equal_non_authoritative";
        case context_store_v1_linux_publish_status::invalid: return "invalid";
        case context_store_v1_linux_publish_status::busy: return "busy";
        case context_store_v1_linux_publish_status::unsupported: return "unsupported";
        case context_store_v1_linux_publish_status::conflict: return "conflict";
        case context_store_v1_linux_publish_status::storage: return "storage";
        case context_store_v1_linux_publish_status::synchronization: return "synchronization";
        case context_store_v1_linux_publish_status::verification: return "verification";
        case context_store_v1_linux_publish_status::incomplete_or_uncertain_discard_root: return "incomplete_or_uncertain_discard_root";
    }
    return "unknown";
}

} // namespace halofpx
