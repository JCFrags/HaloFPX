#include "halofpx-context-store-v1-linux-generation-one.h"

#if !defined(__linux__)
#error "The HaloFPX generation-one full-v1 authority is Linux-only"
#endif

#include <linux/openat2.h>
#include <linux/stat.h>
#include <sys/random.h>
#include <sys/stat.h>
#include <sys/statfs.h>
#include <sys/syscall.h>
#include <unistd.h>

#include <algorithm>
#include <array>
#include <cerrno>
#include <climits>
#include <cstring>
#include <fcntl.h>
#include <limits>
#include <new>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace halofpx {
namespace {

constexpr uint32_t directory_mode = 0700;
constexpr uint32_t file_mode = 0600;
constexpr char pending_name[] = "pending.v1";
constexpr char terminal_name[] = "terminal.v1";
constexpr char anchor_name[] = "anchor.v1";
constexpr char lock_name[] = "writer.lock";

void wipe(void * data, size_t size) noexcept {
    volatile uint8_t * cursor = static_cast<volatile uint8_t *>(data);
    while (size-- != 0) *cursor++ = 0;
}

class fd_owner {
public:
    explicit fd_owner(int fd = -1) noexcept : fd_(fd) {}
    ~fd_owner() { if (fd_ >= 0) ::close(fd_); }
    fd_owner(const fd_owner &) = delete;
    fd_owner & operator=(const fd_owner &) = delete;
    fd_owner(fd_owner && other) noexcept : fd_(std::exchange(other.fd_, -1)) {}
    int get() const noexcept { return fd_; }
    int release() noexcept { return std::exchange(fd_, -1); }
private:
    int fd_;
};

bool nonzero(const uint8_t * data, size_t size) noexcept {
    uint8_t value = 0;
    for (size_t i = 0; i < size; ++i) value |= data[i];
    return value != 0;
}

bool same_digest(const context_store_format_digest & a,
                 const context_store_format_digest & b) noexcept {
    return std::memcmp(a.data(), b.data(), a.size()) == 0;
}

bool same_root(const context_store_linux_root_identity_v1 & a,
               const context_store_linux_root_identity_v1 & b) noexcept {
    return a.device == b.device && a.inode == b.inode && a.mount_id == b.mount_id &&
        a.owner_uid == b.owner_uid && a.mode == b.mode &&
        a.filesystem_type == b.filesystem_type;
}

bool inspect_root(int fd, const context_store_linux_root_identity_v1 & expected) noexcept {
    struct stat value {};
    struct statx extended {};
    struct statfs filesystem {};
    return ::fstat(fd, &value) == 0 && S_ISDIR(value.st_mode) &&
        ::syscall(SYS_statx, fd, "", AT_EMPTY_PATH | AT_STATX_SYNC_AS_STAT,
                  STATX_BASIC_STATS | STATX_MNT_ID, &extended) == 0 &&
        (extended.stx_mask & STATX_MNT_ID) != 0 && ::fstatfs(fd, &filesystem) == 0 &&
        static_cast<uint64_t>(value.st_dev) == expected.device &&
        static_cast<uint64_t>(value.st_ino) == expected.inode &&
        extended.stx_mnt_id == expected.mount_id &&
        static_cast<uint64_t>(value.st_uid) == expected.owner_uid &&
        static_cast<uint32_t>(value.st_mode & 07777) == expected.mode &&
        expected.mode == directory_mode &&
        static_cast<uint64_t>(filesystem.f_type) == expected.filesystem_type;
}

bool canonical_fd_path(int fd, std::string & output) {
    const std::string link = "/proc/self/fd/" + std::to_string(fd);
    std::array<char, PATH_MAX + 1> buffer {};
    const ssize_t size = ::readlink(link.c_str(), buffer.data(), PATH_MAX);
    if (size <= 0 || size >= PATH_MAX || buffer[0] != '/') return false;
    output.assign(buffer.data(), static_cast<size_t>(size));
    return output.size() == 1 || output.back() != '/';
}

bool path_nested(const std::string & parent, const std::string & child) noexcept {
    return child.size() > parent.size() && child.compare(0, parent.size(), parent) == 0 &&
        (parent == "/" || child[parent.size()] == '/');
}

bool distinct_non_nested_roots(int data_fd, int anchor_fd) {
    std::string data;
    std::string anchor;
    return canonical_fd_path(data_fd, data) && canonical_fd_path(anchor_fd, anchor) &&
        data != anchor && !path_nested(data, anchor) && !path_nested(anchor, data);
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

bool exact_regular(int fd, const context_store_linux_root_identity_v1 & root,
                   uint64_t exact_size) noexcept {
    struct stat value {};
    struct statx extended {};
    return ::fstat(fd, &value) == 0 && S_ISREG(value.st_mode) && value.st_nlink == 1 &&
        value.st_size >= 0 && static_cast<uint64_t>(value.st_size) == exact_size &&
        static_cast<uint64_t>(value.st_dev) == root.device &&
        static_cast<uint64_t>(value.st_uid) == root.owner_uid &&
        static_cast<uint32_t>(value.st_mode & 07777) == file_mode &&
        ::syscall(SYS_statx, fd, "", AT_EMPTY_PATH | AT_STATX_SYNC_AS_STAT,
                  STATX_BASIC_STATS | STATX_MNT_ID, &extended) == 0 &&
        (extended.stx_mask & STATX_MNT_ID) != 0 && extended.stx_mnt_id == root.mount_id;
}

bool sync_fd(int fd) noexcept {
    int result;
    do result = ::fsync(fd); while (result != 0 && errno == EINTR);
    return result == 0;
}

bool write_all(int fd, const uint8_t * data, size_t size) noexcept {
    size_t offset = 0;
    while (offset < size) {
        const ssize_t count = ::pwrite(fd, data + offset, size - offset,
                                       static_cast<off_t>(offset));
        if (count < 0 && errno == EINTR) continue;
        if (count <= 0) return false;
        offset += static_cast<size_t>(count);
    }
    return true;
}

bool read_exact(int fd, uint8_t * data, size_t size) noexcept {
    size_t offset = 0;
    while (offset < size) {
        const ssize_t count = ::pread(fd, data + offset, size - offset,
                                      static_cast<off_t>(offset));
        if (count < 0 && errno == EINTR) continue;
        if (count <= 0) return false;
        offset += static_cast<size_t>(count);
    }
    uint8_t trailing = 0;
    ssize_t count;
    do count = ::pread(fd, &trailing, 1, static_cast<off_t>(size));
    while (count < 0 && errno == EINTR);
    return count == 0;
}

enum class observation { absent, exact, malformed, storage };

struct record_read {
    observation state = observation::storage;
    std::vector<uint8_t> bytes;
};

record_read read_record(int root, const context_store_linux_root_identity_v1 & identity,
                        const char * name, size_t maximum) noexcept {
    record_read result;
    fd_owner file(open_contained(root, name, O_RDONLY));
    if (file.get() < 0) {
        result.state = errno == ENOENT ? observation::absent : observation::storage;
        return result;
    }
    struct stat value {};
    if (::fstat(file.get(), &value) != 0 || value.st_size <= 0 ||
        static_cast<uint64_t>(value.st_size) > maximum ||
        !exact_regular(file.get(), identity, static_cast<uint64_t>(value.st_size))) {
        result.state = observation::malformed;
        return result;
    }
    try { result.bytes.resize(static_cast<size_t>(value.st_size)); }
    catch (...) { result.state = observation::storage; return result; }
    if (!read_exact(file.get(), result.bytes.data(), result.bytes.size())) {
        result.bytes.clear(); result.state = observation::malformed; return result;
    }
    result.state = observation::exact;
    return result;
}

struct cbor_cursor { const uint8_t * data; size_t size; size_t offset = 0; };

bool cbor_length(cbor_cursor & cursor, uint8_t expected_major, uint64_t & value) noexcept {
    if (cursor.offset >= cursor.size) return false;
    const uint8_t head = cursor.data[cursor.offset++];
    if ((head >> 5) != expected_major) return false;
    const uint8_t additional = head & 31;
    if (additional < 24) { value = additional; return true; }
    const size_t count = additional == 24 ? 1 : additional == 25 ? 2 :
        additional == 26 ? 4 : additional == 27 ? 8 : 0;
    if (count == 0 || count > cursor.size - cursor.offset) return false;
    value = 0;
    for (size_t i = 0; i < count; ++i) value = (value << 8) | cursor.data[cursor.offset++];
    return true;
}

bool cbor_skip(cbor_cursor & cursor, unsigned depth = 0) noexcept {
    if (depth > 4 || cursor.offset >= cursor.size) return false;
    const uint8_t major = cursor.data[cursor.offset] >> 5;
    if (major == 0) { uint64_t ignored; return cbor_length(cursor, 0, ignored); }
    if (major == 2) {
        uint64_t length;
        if (!cbor_length(cursor, 2, length) || length > cursor.size - cursor.offset) return false;
        cursor.offset += static_cast<size_t>(length); return true;
    }
    if (major == 5) {
        uint64_t count;
        if (!cbor_length(cursor, 5, count) || count > 64) return false;
        for (uint64_t i = 0; i < count; ++i)
            if (!cbor_skip(cursor, depth + 1) || !cbor_skip(cursor, depth + 1)) return false;
        return true;
    }
    if (cursor.data[cursor.offset] == 0xf6) { ++cursor.offset; return true; }
    return false;
}

bool extract_source_commitment(const uint8_t * wire, size_t size,
                               context_store_format_digest & digest) noexcept {
    cbor_cursor cursor { wire, size };
    uint64_t outer = 0, key = 0, fields = 0;
    if (!cbor_length(cursor, 5, outer) || outer != 2 ||
        !cbor_length(cursor, 0, key) || key != 0 ||
        !cbor_length(cursor, 5, fields) || fields != 16) return false;
    bool found = false;
    for (uint64_t i = 0; i < fields; ++i) {
        if (!cbor_length(cursor, 0, key)) return false;
        if (key == 11) {
            uint64_t length = 0;
            if (!cbor_length(cursor, 2, length) || length != digest.size() ||
                length > cursor.size - cursor.offset) return false;
            std::memcpy(digest.data(), cursor.data + cursor.offset, digest.size());
            cursor.offset += digest.size(); found = true;
        } else if (!cbor_skip(cursor)) return false;
    }
    return found;
}

observation observe_exact(int root, const context_store_linux_root_identity_v1 & identity,
                          const char * name, const uint8_t * expected, size_t size) noexcept {
    fd_owner file(open_contained(root, name, O_RDONLY));
    if (file.get() < 0) return errno == ENOENT ? observation::absent : observation::storage;
    if (!exact_regular(file.get(), identity, size)) return observation::malformed;
    std::vector<uint8_t> bytes;
    try { bytes.resize(size); } catch (...) { return observation::storage; }
    const bool exact = read_exact(file.get(), bytes.data(), bytes.size()) &&
        std::memcmp(bytes.data(), expected, size) == 0;
    wipe(bytes.data(), bytes.size());
    return exact ? observation::exact : observation::malformed;
}

context_store_v1_linux_generation_one_status publish_fixed(
        int root, const context_store_linux_root_identity_v1 & identity,
        const char * stage, const char * final_name,
        const uint8_t * data, size_t size) noexcept {
    fd_owner file(open_contained(root, stage, O_RDWR | O_CREAT | O_EXCL, file_mode));
    if (file.get() < 0) return errno == EEXIST
        ? context_store_v1_linux_generation_one_status::conflict
        : context_store_v1_linux_generation_one_status::storage;
    bool ok = exact_regular(file.get(), identity, 0) && write_all(file.get(), data, size) &&
        exact_regular(file.get(), identity, size) && sync_fd(file.get());
    std::vector<uint8_t> readback;
    try { readback.resize(size); } catch (...) { ok = false; }
    ok = ok && read_exact(file.get(), readback.data(), readback.size()) &&
        std::memcmp(readback.data(), data, size) == 0;
    wipe(readback.data(), readback.size());
    if (!ok) {
        ::unlinkat(root, stage, 0);
        return context_store_v1_linux_generation_one_status::synchronization;
    }
    int renamed;
    do renamed = static_cast<int>(::syscall(SYS_renameat2, root, stage, root,
                                             final_name, RENAME_NOREPLACE));
    while (renamed != 0 && errno == EINTR);
    if (renamed != 0) {
        const int error = errno;
        ::unlinkat(root, stage, 0);
        if (error == ENOSYS || error == EINVAL)
            return context_store_v1_linux_generation_one_status::unsupported;
        return error == EEXIST ? context_store_v1_linux_generation_one_status::conflict
                               : context_store_v1_linux_generation_one_status::storage;
    }
    if (!sync_fd(root)) return context_store_v1_linux_generation_one_status::synchronization;
    return observe_exact(root, identity, final_name, data, size) == observation::exact
        ? context_store_v1_linux_generation_one_status::published
        : context_store_v1_linux_generation_one_status::quarantined;
}

void append_u64(std::vector<uint8_t> & bytes, uint64_t value) {
    for (int shift = 56; shift >= 0; shift -= 8)
        bytes.push_back(static_cast<uint8_t>(value >> shift));
}

void append_bytes(std::vector<uint8_t> & out, const uint8_t * data, size_t size) {
    out.insert(out.end(), data, data + size);
}

void append_id(std::vector<uint8_t> & out, const context_store_registered_id & id) {
    out.push_back(id.size);
    append_bytes(out, reinterpret_cast<const uint8_t *>(id.bytes.data()), id.size);
}

bool root_commitment(const context_store_linux_root_identity_v1 & root,
                     context_store_format_digest & digest) {
    static constexpr uint8_t domain[] = "halofpx.v1.root-identity.v1\0";
    std::vector<uint8_t> bytes(domain, domain + sizeof(domain));
    bytes.reserve(bytes.size() + 48);
    append_u64(bytes, root.device); append_u64(bytes, root.inode);
    append_u64(bytes, root.mount_id); append_u64(bytes, root.owner_uid);
    append_u64(bytes, root.mode); append_u64(bytes, root.filesystem_type);
    return context_store_sha256(bytes.data(), bytes.size(), digest);
}

bool object_commitment(const context_store_v1_read_only_admission & admission,
                       context_store_format_digest & digest) {
    static constexpr uint8_t domain[] = "halofpx.v1.ordered-object-set.v1\0";
    std::vector<uint8_t> bytes(domain, domain + sizeof(domain));
    append_u64(bytes, admission.object_count);
    for (size_t i = 0; i < admission.object_count; ++i) {
        const auto & object = admission.objects[i];
        append_bytes(bytes, object.object_id.data(), object.object_id.size());
        append_id(bytes, object.stream_type); append_id(bytes, object.codec_id);
        append_u64(bytes, object.codec_schema_major); append_u64(bytes, object.codec_schema_minor);
        bytes.push_back(object.required ? 1 : 0); append_u64(bytes, object.frame_bytes);
        append_bytes(bytes, object.token_sequence_digest.data(), object.token_sequence_digest.size());
        append_u64(bytes, object.logical_position); append_u64(bytes, object.output_boundary);
        bytes.push_back(object.has_logical_rank ? 1 : 0); append_u64(bytes, object.logical_rank);
        append_bytes(bytes, object.rank_ownership_digest.data(), object.rank_ownership_digest.size());
        append_bytes(bytes, object.compatibility_root.data(), object.compatibility_root.size());
    }
    return context_store_sha256(bytes.data(), bytes.size(), digest);
}

bool source_commitment(const context_store_v1_read_only_source & source,
                       context_store_format_digest & digest) {
    static constexpr uint8_t domain[] = "halofpx.v1.aggregate-source.v1\0";
    struct wiping_buffer {
        std::vector<uint8_t> bytes;
        ~wiping_buffer() { wipe(bytes.data(), bytes.size()); }
    } input;
    size_t required = sizeof(domain) + sizeof(uint64_t) * 2;
    if (source.manifest_size > SIZE_MAX - required) return false;
    required += source.manifest_size;
    for (size_t i = 0; i < source.frame_count; ++i) {
        if (required > SIZE_MAX - sizeof(uint64_t) ||
            source.frames[i].size > SIZE_MAX - required - sizeof(uint64_t)) return false;
        required += sizeof(uint64_t) + source.frames[i].size;
    }
    input.bytes.reserve(required);
    append_bytes(input.bytes, domain, sizeof(domain));
    append_u64(input.bytes, source.manifest_size);
    append_bytes(input.bytes, source.manifest_data, source.manifest_size);
    append_u64(input.bytes, source.frame_count);
    for (size_t i = 0; i < source.frame_count; ++i) {
        append_u64(input.bytes, source.frames[i].size);
        append_bytes(input.bytes, source.frames[i].data, source.frames[i].size);
    }
    return input.bytes.size() == required &&
        context_store_sha256(input.bytes.data(), input.bytes.size(), digest);
}

bool random_attempt(context_store_v1_publish_attempt_id & id) noexcept {
    size_t offset = 0;
    while (offset < id.size()) {
        const ssize_t count = ::getrandom(id.data() + offset, id.size() - offset, 0);
        if (count < 0 && errno == EINTR) continue;
        if (count <= 0) return false;
        offset += static_cast<size_t>(count);
    }
    return nonzero(id.data(), id.size());
}

context_store_lookup_request authority_request(
        const context_store_authenticated_manifest_metadata & admission) {
    context_store_lookup_request request;
    request.identity.compatibility_root = admission.compatibility_root;
    request.identity.scope_namespace = admission.scope_namespace;
    request.identity.checkpoint_lineage_id = admission.checkpoint_lineage_id;
    request.identity.policy_epoch = admission.policy_epoch;
    return request;
}

} // namespace

class context_store_v1_linux_generation_one::implementation {
public:
    implementation(int data_fd, int anchor_fd, int lock_fd,
                   const context_store_v1_linux_generation_one_config & config)
        : data_root(data_fd), anchor_root(anchor_fd), writer_lock(lock_fd),
          data_identity(config.data_root.identity), anchor_identity(config.anchor_root.identity),
          policy(config.verification_policy), admission_manifest(config.admission.manifest),
          admission_objects(config.admission.objects,
                            config.admission.objects + config.admission.object_count),
          limits(config.object_limits), max_total(config.max_total_frame_bytes),
          anchor_body(config.anchor_body), anchor_key(config.anchor_key),
          failpoint(config.test_failpoint) {
        admission.objects = admission_objects.data();
        admission.object_count = admission_objects.size();
        admission.manifest = admission_manifest;

        const auto encoded = context_store_protected_canary_anchor_encode_v1(
            anchor_body, anchor_key, anchor.data(), anchor.size());
        if (!encoded.authenticated()) throw std::invalid_argument("invalid generation-one anchor");
        anchor_size = encoded.encoded_size;
        if (!root_commitment(data_identity, data_commitment) ||
            !root_commitment(anchor_identity, anchor_commitment) ||
            !object_commitment(admission, objects_commitment))
            throw std::invalid_argument("invalid generation-one commitments");

        struct guarded_secrets {
            std::vector<uint8_t> verification;
            std::vector<uint8_t> anchor;
            std::vector<uint8_t> attempt;
            ~guarded_secrets() {
                wipe(verification.data(), verification.size());
                wipe(anchor.data(), anchor.size());
                wipe(attempt.data(), attempt.size());
            }
        } secrets;
        secrets.verification.assign(config.verification_policy.key.master_key.data,
            config.verification_policy.key.master_key.data +
            config.verification_policy.key.master_key.size);
        secrets.anchor.assign(config.anchor_key.master_key.data,
            config.anchor_key.master_key.data + config.anchor_key.master_key.size);
        secrets.attempt.assign(config.attempt_key.master_key.data,
            config.attempt_key.master_key.data + config.attempt_key.master_key.size);
        verification_key = std::move(secrets.verification);
        anchor_master = std::move(secrets.anchor);
        attempt_master = std::move(secrets.attempt);
        policy.key.master_key = { verification_key.data(), verification_key.size() };
        anchor_key.master_key = { anchor_master.data(), anchor_master.size() };
        attempt_key.master_key = { attempt_master.data(), attempt_master.size() };
    }

    ~implementation() {
        wipe(verification_key.data(), verification_key.size());
        wipe(anchor_master.data(), anchor_master.size());
        wipe(attempt_master.data(), attempt_master.size());
        wipe(anchor.data(), anchor.size());
    }

    context_store_v1_linux_read_only_source filesystem_source() const {
        context_store_v1_linux_read_only_source source;
        source.root_fd = data_root.get();
        source.root_identity = data_identity;
        source.verification_policy = policy;
        source.admission = admission;
        source.object_limits = limits;
        source.max_total_frame_bytes = max_total;
        return source;
    }

    context_store_v1_attempt_body body(const context_store_v1_publish_attempt_id & id,
                                       const context_store_format_digest & source) const noexcept {
        context_store_v1_attempt_body value;
        value.attempt_id = id;
        value.store_uuid = anchor_body.store_uuid;
        value.namespace_id = anchor_body.namespace_id;
        value.checkpoint_lineage_id = anchor_body.checkpoint_lineage_id;
        value.manifest_digest = anchor_body.selected_manifest_digest;
        value.ordered_object_set_commitment = objects_commitment;
        value.aggregate_source_commitment = source;
        value.data_root_identity_commitment = data_commitment;
        value.anchor_root_identity_commitment = anchor_commitment;
        value.proposed_anchor_envelope = { anchor.data(), anchor_size };
        return value;
    }

    bool material_hit() const noexcept {
        try {
            auto provider = make_context_store_v1_linux_read_only_provider(filesystem_source());
            return provider->lookup(authority_request(admission_manifest)).is_hit();
        } catch (...) { return false; }
    }

    bool roots_current() const noexcept {
        return inspect_root(data_root.get(), data_identity) &&
            inspect_root(anchor_root.get(), anchor_identity);
    }

    bool decode_record(const record_read & record, bool pending,
                       context_store_v1_attempt_body & decoded,
                       context_store_v1_publish_attempt_id & attempt) const noexcept {
        if (record.state != observation::exact ||
            record.bytes.size() <= context_store_v1_attempt_id_bytes) return false;
        std::copy_n(record.bytes.data(), attempt.size(), attempt.begin());
        context_store_format_digest aggregate {};
        const uint8_t * wire = record.bytes.data() + attempt.size();
        const size_t wire_size = record.bytes.size() - attempt.size();
        if (!nonzero(attempt.data(), attempt.size()) ||
            !extract_source_commitment(wire, wire_size, aggregate)) return false;
        decoded = body(attempt, aggregate);
        return pending
            ? context_store_v1_attempt_pending_verify(
                wire, wire_size, decoded, attempt_key).authenticated()
            : (context_store_v1_attempt_terminal_verify(
                wire, wire_size, decoded,
                context_store_v1_attempt_terminal_status::success,
                attempt_key).authenticated() ||
               context_store_v1_attempt_terminal_verify(
                wire, wire_size, decoded,
                context_store_v1_attempt_terminal_status::aborted,
                attempt_key).authenticated());
    }

    context_store_v1_linux_generation_one_status write_terminal(
            const context_store_v1_attempt_body & body,
            context_store_v1_attempt_terminal_status terminal_status) noexcept {
        if (!roots_current())
            return context_store_v1_linux_generation_one_status::quarantined;
        std::array<uint8_t, context_store_v1_attempt_wire_max_bytes> wire {};
        const auto encoded = context_store_v1_attempt_terminal_encode(
            body, terminal_status, attempt_key, wire.data(), wire.size());
        if (!encoded.authenticated()) return context_store_v1_linux_generation_one_status::invalid;
        std::vector<uint8_t> record;
        try {
            record.reserve(body.attempt_id.size() + encoded.encoded_size);
            record.insert(record.end(), body.attempt_id.begin(), body.attempt_id.end());
            record.insert(record.end(), wire.begin(), wire.begin() + encoded.encoded_size);
        }
        catch (...) { return context_store_v1_linux_generation_one_status::storage; }
        const observation existing = observe_exact(anchor_root.get(), anchor_identity,
            terminal_name, record.data(), record.size());
        if (existing == observation::exact) return context_store_v1_linux_generation_one_status::published;
        if (existing != observation::absent) return context_store_v1_linux_generation_one_status::quarantined;
        return publish_fixed(anchor_root.get(), anchor_identity, "terminal.stage", terminal_name,
                             record.data(), record.size());
    }

    context_store_v1_linux_generation_one_status clear_pending(
            context_store_v1_linux_generation_one_status success) noexcept {
        if (!roots_current())
            return context_store_v1_linux_generation_one_status::quarantined;
        if (::unlinkat(anchor_root.get(), pending_name, 0) != 0 || !sync_fd(anchor_root.get()))
            return context_store_v1_linux_generation_one_status::synchronization;
        return success;
    }

    context_store_v1_linux_generation_one_status reconcile() noexcept {
        if (!roots_current()) {
            sticky_quarantine = true;
            return context_store_v1_linux_generation_one_status::quarantined;
        }
        record_read pending = read_record(anchor_root.get(), anchor_identity, pending_name,
            context_store_v1_attempt_id_bytes + context_store_v1_attempt_wire_max_bytes);
        if (pending.state == observation::storage || pending.state == observation::malformed) {
            sticky_quarantine = true; return context_store_v1_linux_generation_one_status::quarantined;
        }
        const observation observed_anchor = observe_exact(anchor_root.get(), anchor_identity,
            anchor_name, anchor.data(), anchor_size);
        if (pending.state == observation::absent) {
            record_read terminal = read_record(anchor_root.get(), anchor_identity, terminal_name,
                context_store_v1_attempt_id_bytes + context_store_v1_attempt_wire_max_bytes);
            context_store_v1_attempt_body terminal_body;
            context_store_v1_publish_attempt_id terminal_id {};
            bool terminal_success = false;
            bool terminal_aborted = false;
            if (terminal.state == observation::exact) {
                if (!decode_record(terminal, false, terminal_body, terminal_id)) {
                    sticky_quarantine = true;
                    return context_store_v1_linux_generation_one_status::quarantined;
                }
                const uint8_t * wire = terminal.bytes.data() + terminal_id.size();
                const size_t wire_size = terminal.bytes.size() - terminal_id.size();
                terminal_success = context_store_v1_attempt_terminal_verify(
                    wire, wire_size, terminal_body,
                    context_store_v1_attempt_terminal_status::success, attempt_key).authenticated();
                terminal_aborted = context_store_v1_attempt_terminal_verify(
                    wire, wire_size, terminal_body,
                    context_store_v1_attempt_terminal_status::aborted, attempt_key).authenticated();
            } else if (terminal.state != observation::absent) {
                sticky_quarantine = true;
                return context_store_v1_linux_generation_one_status::quarantined;
            }
            if (observed_anchor == observation::exact) {
                if (!terminal_success || terminal_aborted || !material_hit()) {
                    sticky_quarantine = true;
                    return context_store_v1_linux_generation_one_status::quarantined;
                }
                return context_store_v1_linux_generation_one_status::recovered_success;
            }
            if (observed_anchor != observation::absent) {
                sticky_quarantine = true; return context_store_v1_linux_generation_one_status::quarantined;
            }
            if (terminal.state == observation::absent) return context_store_v1_linux_generation_one_status::ready;
            if (terminal_aborted && !terminal_success)
                return context_store_v1_linux_generation_one_status::recovered_aborted;
            sticky_quarantine = true;
            return context_store_v1_linux_generation_one_status::quarantined;
        }

        context_store_v1_attempt_body pending_body;
        context_store_v1_publish_attempt_id pending_id {};
        if (!decode_record(pending, true, pending_body, pending_id)) {
            sticky_quarantine = true; return context_store_v1_linux_generation_one_status::quarantined;
        }
        if (observed_anchor == observation::exact) {
            if (!material_hit() || write_terminal(pending_body,
                    context_store_v1_attempt_terminal_status::success) !=
                    context_store_v1_linux_generation_one_status::published) {
                sticky_quarantine = true; return context_store_v1_linux_generation_one_status::quarantined;
            }
            return clear_pending(context_store_v1_linux_generation_one_status::recovered_success);
        }
        if (observed_anchor == observation::absent) {
            if (write_terminal(pending_body, context_store_v1_attempt_terminal_status::aborted) !=
                    context_store_v1_linux_generation_one_status::published) {
                sticky_quarantine = true; return context_store_v1_linux_generation_one_status::quarantined;
            }
            return clear_pending(context_store_v1_linux_generation_one_status::recovered_aborted);
        }
        sticky_quarantine = true;
        return context_store_v1_linux_generation_one_status::quarantined;
    }

    fd_owner data_root;
    fd_owner anchor_root;
    fd_owner writer_lock;
    context_store_linux_root_identity_v1 data_identity;
    context_store_linux_root_identity_v1 anchor_identity;
    context_store_manifest_verification_policy policy;
    context_store_authenticated_manifest_metadata admission_manifest;
    std::vector<context_store_object_reference> admission_objects;
    context_store_v1_read_only_admission admission;
    context_store_object_limits limits;
    uint64_t max_total = 0;
    context_store_protected_canary_anchor_body anchor_body;
    context_store_protected_canary_anchor_key anchor_key;
    context_store_v1_attempt_key attempt_key;
    std::vector<uint8_t> verification_key;
    std::vector<uint8_t> anchor_master;
    std::vector<uint8_t> attempt_master;
    std::array<uint8_t, context_store_protected_canary_anchor_max_bytes> anchor {};
    size_t anchor_size = 0;
    context_store_format_digest data_commitment {};
    context_store_format_digest anchor_commitment {};
    context_store_format_digest objects_commitment {};
    context_store_v1_linux_generation_one_failpoint failpoint;
    context_store_v1_linux_generation_one_status current =
        context_store_v1_linux_generation_one_status::ready;
    bool sticky_quarantine = false;
};

context_store_v1_linux_generation_one::context_store_v1_linux_generation_one(
        std::unique_ptr<implementation> implementation) noexcept
    : implementation_(std::move(implementation)) {}

context_store_v1_linux_generation_one::~context_store_v1_linux_generation_one() = default;

context_store_v1_linux_generation_one_status
context_store_v1_linux_generation_one::status() const noexcept {
    return implementation_ ? implementation_->current
                           : context_store_v1_linux_generation_one_status::invalid;
}

bool context_store_v1_linux_generation_one::quarantined() const noexcept {
    return !implementation_ || implementation_->sticky_quarantine;
}

context_store_v1_linux_generation_one_status
context_store_v1_linux_generation_one::publish(
        const context_store_v1_read_only_source & source) noexcept {
    if (!implementation_ || implementation_->sticky_quarantine)
        return context_store_v1_linux_generation_one_status::quarantined;
    if (!implementation_->roots_current()) {
        implementation_->sticky_quarantine = true;
        return context_store_v1_linux_generation_one_status::quarantined;
    }
    if (implementation_->current != context_store_v1_linux_generation_one_status::ready)
        return context_store_v1_linux_generation_one_status::conflict;
    try {
        if (!source.manifest_data || source.manifest_size == 0 || !source.frames ||
            source.frame_count != implementation_->admission_objects.size())
            return context_store_v1_linux_generation_one_status::invalid;
        if (source.manifest_size > context_store_manifest_max_bytes)
            return context_store_v1_linux_generation_one_status::invalid;
        uint64_t preflight_total = 0;
        for (size_t index = 0; index < source.frame_count; ++index) {
            const auto & frame = source.frames[index];
            if (!frame.data || frame.size == 0 ||
                frame.size > implementation_->limits.max_frame_bytes ||
                preflight_total > implementation_->max_total ||
                frame.size > implementation_->max_total - preflight_total)
                return context_store_v1_linux_generation_one_status::invalid;
            preflight_total += frame.size;
        }
        context_store_v1_read_only_source bound_source = source;
        bound_source.verification_policy = implementation_->policy;
        bound_source.admission = implementation_->admission;
        bound_source.object_limits = implementation_->limits;
        bound_source.max_total_frame_bytes = implementation_->max_total;
        auto memory = make_context_store_v1_read_only_provider(bound_source);
        if (!memory->lookup(authority_request(implementation_->admission_manifest)).is_hit())
            return context_store_v1_linux_generation_one_status::source_mismatch;
        // The provider validates all configured pointer/count/byte caps before
        // copying or authenticating. Only then may the aggregate commitment
        // traverse the caller-owned buffers.
        context_store_format_digest manifest_digest {};
        context_store_format_digest aggregate {};
        if (!context_store_manifest_digest_v1(source.manifest_data, source.manifest_size,
                                               manifest_digest) ||
            !same_digest(manifest_digest, implementation_->anchor_body.selected_manifest_digest) ||
            !source_commitment(bound_source, aggregate))
            return context_store_v1_linux_generation_one_status::source_mismatch;

        context_store_v1_publish_attempt_id attempt {};
        if (!random_attempt(attempt)) return context_store_v1_linux_generation_one_status::storage;
        const auto body = implementation_->body(attempt, aggregate);
        std::array<uint8_t, context_store_v1_attempt_wire_max_bytes> pending_wire {};
        const auto encoded = context_store_v1_attempt_pending_encode(body,
            implementation_->attempt_key, pending_wire.data(), pending_wire.size());
        if (!encoded.authenticated()) return context_store_v1_linux_generation_one_status::invalid;
        std::vector<uint8_t> pending(attempt.begin(), attempt.end());
        pending.insert(pending.end(), pending_wire.begin(), pending_wire.begin() + encoded.encoded_size);
        auto result = publish_fixed(implementation_->anchor_root.get(),
            implementation_->anchor_identity, "pending.stage", pending_name,
            pending.data(), pending.size());
        if (result != context_store_v1_linux_generation_one_status::published) return result;
        if (implementation_->failpoint == context_store_v1_linux_generation_one_failpoint::after_pending) {
            implementation_->current = context_store_v1_linux_generation_one_status::interrupted;
            return implementation_->current;
        }

        auto materializer = make_context_store_v1_linux_snapshot_materializer(
            { implementation_->data_root.get(), implementation_->data_identity });
        const auto material = materializer->publish(attempt, bound_source);
        if (material != context_store_v1_linux_publish_status::materialized_non_authoritative &&
            material != context_store_v1_linux_publish_status::already_equal_non_authoritative) {
            implementation_->sticky_quarantine = material ==
                context_store_v1_linux_publish_status::incomplete_or_uncertain_discard_root;
            return implementation_->sticky_quarantine
                ? context_store_v1_linux_generation_one_status::quarantined
                : context_store_v1_linux_generation_one_status::storage;
        }
        if (implementation_->failpoint == context_store_v1_linux_generation_one_failpoint::after_material) {
            implementation_->current = context_store_v1_linux_generation_one_status::interrupted;
            return implementation_->current;
        }

        if (!implementation_->roots_current()) {
            implementation_->sticky_quarantine = true;
            return context_store_v1_linux_generation_one_status::quarantined;
        }

        result = publish_fixed(implementation_->anchor_root.get(),
            implementation_->anchor_identity, "anchor.stage", anchor_name,
            implementation_->anchor.data(), implementation_->anchor_size);
        if (result != context_store_v1_linux_generation_one_status::published) {
            implementation_->sticky_quarantine = result != context_store_v1_linux_generation_one_status::conflict;
            return result;
        }
        if (implementation_->failpoint == context_store_v1_linux_generation_one_failpoint::after_anchor) {
            implementation_->current = context_store_v1_linux_generation_one_status::interrupted;
            return implementation_->current;
        }
        if (!implementation_->roots_current() || !implementation_->material_hit()) {
            implementation_->sticky_quarantine = true;
            return context_store_v1_linux_generation_one_status::quarantined;
        }

        std::array<uint8_t, context_store_v1_attempt_wire_max_bytes> terminal_wire {};
        const auto terminal_encoded = context_store_v1_attempt_terminal_encode(body,
            context_store_v1_attempt_terminal_status::success, implementation_->attempt_key,
            terminal_wire.data(), terminal_wire.size());
        if (!terminal_encoded.authenticated()) return context_store_v1_linux_generation_one_status::invalid;
        std::vector<uint8_t> terminal(attempt.begin(), attempt.end());
        terminal.insert(terminal.end(), terminal_wire.begin(),
                        terminal_wire.begin() + terminal_encoded.encoded_size);
        result = publish_fixed(implementation_->anchor_root.get(),
            implementation_->anchor_identity, "terminal.stage", terminal_name,
            terminal.data(), terminal.size());
        if (result != context_store_v1_linux_generation_one_status::published) return result;
        if (::unlinkat(implementation_->anchor_root.get(), pending_name, 0) != 0 ||
            !sync_fd(implementation_->anchor_root.get()))
            return context_store_v1_linux_generation_one_status::synchronization;
        implementation_->current = context_store_v1_linux_generation_one_status::published;
        return implementation_->current;
    } catch (const std::invalid_argument &) {
        return context_store_v1_linux_generation_one_status::source_mismatch;
    } catch (...) {
        return context_store_v1_linux_generation_one_status::storage;
    }
}

context_store_lookup_result context_store_v1_linux_generation_one::lookup(
        const context_store_lookup_request & request) const noexcept {
    if (!implementation_ || implementation_->sticky_quarantine)
        return context_store_lookup_result::miss(context_store_lookup_status::miss_corrupt);
    if (!implementation_->roots_current())
        return context_store_lookup_result::miss(context_store_lookup_status::miss_corrupt);
    if (observe_exact(implementation_->anchor_root.get(), implementation_->anchor_identity,
            anchor_name, implementation_->anchor.data(), implementation_->anchor_size) != observation::exact)
        return context_store_lookup_result::miss(context_store_lookup_status::miss_incomplete);
    try {
        auto provider = make_context_store_v1_linux_read_only_provider(
            implementation_->filesystem_source());
        return provider->lookup(request);
    } catch (...) {
        return context_store_lookup_result::miss(context_store_lookup_status::miss_storage);
    }
}

context_store_v1_linux_generation_one_open_result
make_context_store_v1_linux_generation_one(
        const context_store_v1_linux_generation_one_config & config) noexcept {
    context_store_v1_linux_generation_one_open_result result;
    try {
        const auto & replay = config.verification_policy.anchor;
        const auto & admitted = config.admission.manifest;
        if (config.data_root.root_fd < 0 || config.anchor_root.root_fd < 0 ||
            same_root(config.data_root.identity, config.anchor_root.identity) ||
            config.data_root.identity.inode == config.anchor_root.identity.inode ||
            !config.admission.objects || config.admission.object_count == 0 ||
            config.admission.object_count > context_store_manifest_max_objects ||
            !config.verification_policy.key.master_key.data ||
            config.verification_policy.key.master_key.size == 0 ||
            config.verification_policy.key.master_key.size > context_store_master_key_max_bytes ||
            !config.anchor_key.master_key.data ||
            config.anchor_key.master_key.size != context_store_protected_canary_anchor_master_key_bytes ||
            !config.attempt_key.master_key.data ||
            config.attempt_key.master_key.size != context_store_v1_attempt_master_key_bytes ||
            config.object_limits.max_frame_bytes == 0 ||
            config.object_limits.max_payload_bytes == 0 || config.max_total_frame_bytes == 0 ||
            config.anchor_body.generation != 1 || config.anchor_body.has_predecessor ||
            config.anchor_body.policy_epoch != 1 || config.anchor_body.manifest_key_generation != 1 ||
            config.anchor_body.authority_epoch != 1 || config.anchor_key.generation != 1 ||
            !nonzero(config.anchor_body.store_uuid.data(), config.anchor_body.store_uuid.size()) ||
            config.anchor_body.store_uuid != replay.store_uuid ||
            config.anchor_body.store_uuid != admitted.store_uuid ||
            !same_digest(config.anchor_body.namespace_id, replay.namespace_id) ||
            !same_digest(config.anchor_body.namespace_id, admitted.scope_namespace) ||
            !same_digest(config.anchor_body.checkpoint_lineage_id, replay.checkpoint_lineage_id) ||
            !same_digest(config.anchor_body.checkpoint_lineage_id, admitted.checkpoint_lineage_id) ||
            config.anchor_body.policy_epoch != replay.policy_epoch ||
            config.anchor_body.policy_epoch != admitted.policy_epoch ||
            config.anchor_body.manifest_key_generation != replay.key_generation ||
            config.anchor_body.manifest_key_generation != config.verification_policy.key.generation ||
            config.anchor_body.generation != replay.generation ||
            config.anchor_body.generation != admitted.generation ||
            replay.has_predecessor || admitted.has_predecessor ||
            !same_digest(config.anchor_body.selected_manifest_digest,
                         replay.selected_manifest_digest)) {
            return result;
        }
        fd_owner data(::fcntl(config.data_root.root_fd, F_DUPFD_CLOEXEC, 3));
        fd_owner anchor(::fcntl(config.anchor_root.root_fd, F_DUPFD_CLOEXEC, 3));
        if (data.get() < 0 || anchor.get() < 0 ||
            !inspect_root(data.get(), config.data_root.identity) ||
            !inspect_root(anchor.get(), config.anchor_root.identity) ||
            !distinct_non_nested_roots(data.get(), anchor.get())) {
            result.status = context_store_v1_linux_generation_one_status::invalid;
            return result;
        }
        fd_owner lock(open_contained(anchor.get(), lock_name, O_RDWR));
        if (lock.get() < 0 || !exact_regular(lock.get(), config.anchor_root.identity, 0)) {
            result.status = context_store_v1_linux_generation_one_status::invalid;
            return result;
        }
        struct flock flock {};
        flock.l_type = F_WRLCK; flock.l_whence = SEEK_SET;
        int locked;
        do locked = ::fcntl(lock.get(), F_OFD_SETLK, &flock); while (locked != 0 && errno == EINTR);
        if (locked != 0) {
            result.status = errno == EAGAIN || errno == EACCES
                ? context_store_v1_linux_generation_one_status::busy
                : errno == EINVAL ? context_store_v1_linux_generation_one_status::unsupported
                                  : context_store_v1_linux_generation_one_status::storage;
            return result;
        }
        auto impl = std::make_unique<context_store_v1_linux_generation_one::implementation>(
            data.release(), anchor.release(), lock.release(), config);
        result.authority.reset(new context_store_v1_linux_generation_one(std::move(impl)));
        result.status = result.authority->implementation_->reconcile();
        result.authority->implementation_->current = result.status;
        return result;
    } catch (const std::invalid_argument &) {
        result.status = context_store_v1_linux_generation_one_status::invalid;
    } catch (const std::bad_alloc &) {
        result.status = context_store_v1_linux_generation_one_status::storage;
    } catch (...) {
        result.status = context_store_v1_linux_generation_one_status::storage;
    }
    return result;
}

const char * context_store_v1_linux_generation_one_status_name(
        context_store_v1_linux_generation_one_status status) noexcept {
    switch (status) {
        case context_store_v1_linux_generation_one_status::ready: return "ready";
        case context_store_v1_linux_generation_one_status::published: return "published";
        case context_store_v1_linux_generation_one_status::recovered_success: return "recovered_success";
        case context_store_v1_linux_generation_one_status::recovered_aborted: return "recovered_aborted";
        case context_store_v1_linux_generation_one_status::interrupted: return "interrupted";
        case context_store_v1_linux_generation_one_status::busy: return "busy";
        case context_store_v1_linux_generation_one_status::invalid: return "invalid";
        case context_store_v1_linux_generation_one_status::unsupported: return "unsupported";
        case context_store_v1_linux_generation_one_status::source_mismatch: return "source_mismatch";
        case context_store_v1_linux_generation_one_status::conflict: return "conflict";
        case context_store_v1_linux_generation_one_status::storage: return "storage";
        case context_store_v1_linux_generation_one_status::synchronization: return "synchronization";
        case context_store_v1_linux_generation_one_status::quarantined: return "quarantined";
    }
    return "unknown";
}

} // namespace halofpx
