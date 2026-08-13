#include "halofpx-context-store-v1-catalog.h"

#if !defined(__linux__)
#error "The HaloFPX exact-key catalog is Linux-only"
#endif

#include <linux/fs.h>
#include <linux/stat.h>
#include <dirent.h>
#include <sys/stat.h>
#include <sys/statvfs.h>
#include <sys/syscall.h>
#include <unistd.h>

#include <algorithm>
#include <array>
#include <cerrno>
#include <cstring>
#include <fcntl.h>
#include <limits>
#include <new>
#include <string>
#include <utility>
#include <vector>

namespace halofpx {
namespace {

constexpr mode_t directory_mode = 0700;
constexpr mode_t file_mode = 0600;
constexpr char catalog_key_domain[] = "halofpx.full-v1-catalog.record-key.v1";
constexpr char manifest_key_domain[] = "halofpx.full-v1-canary.manifest-key.v1";
constexpr char manifest_key_id[] = "halofpx-full-v1-canary-manifest-v1";
constexpr std::array<uint8_t, 8> record_magic {{'H','F','P','X','C','A','T','1'}};
constexpr size_t record_body_bytes = 464;
constexpr size_t record_bytes = record_body_bytes + 32;

void wipe(void * data, size_t size) noexcept {
    volatile uint8_t * p = static_cast<volatile uint8_t *>(data);
    while (size-- != 0) *p++ = 0;
}

class digest_pair_wiper {
public:
    digest_pair_wiper(context_store_format_digest & first,
                      context_store_format_digest & second) noexcept
        : first_(first), second_(second) {}
    ~digest_pair_wiper() {
        wipe(first_.data(), first_.size());
        wipe(second_.data(), second_.size());
    }
private:
    context_store_format_digest & first_;
    context_store_format_digest & second_;
};

class fd_owner {
public:
    explicit fd_owner(int value = -1) noexcept : value_(value) {}
    ~fd_owner() { if (value_ >= 0) ::close(value_); }
    fd_owner(const fd_owner &) = delete;
    fd_owner & operator=(const fd_owner &) = delete;
    fd_owner(fd_owner && other) noexcept : value_(std::exchange(other.value_, -1)) {}
    int get() const noexcept { return value_; }
    int release() noexcept { return std::exchange(value_, -1); }
private:
    int value_;
};

bool nonzero(const uint8_t * p, size_t n) noexcept {
    uint8_t v = 0;
    while (n-- != 0) v |= *p++;
    return v != 0;
}
template <size_t N> bool nonzero(const std::array<uint8_t, N> & v) noexcept {
    return nonzero(v.data(), v.size());
}

void put_u32(uint8_t *& p, uint32_t v) noexcept {
    for (int shift = 24; shift >= 0; shift -= 8) *p++ = static_cast<uint8_t>(v >> shift);
}
void put_u64(uint8_t *& p, uint64_t v) noexcept {
    for (int shift = 56; shift >= 0; shift -= 8) *p++ = static_cast<uint8_t>(v >> shift);
}
uint32_t get_u32(const uint8_t *& p) noexcept {
    uint32_t v = 0; for (int i = 0; i != 4; ++i) v = (v << 8) | *p++; return v;
}
uint64_t get_u64(const uint8_t *& p) noexcept {
    uint64_t v = 0; for (int i = 0; i != 8; ++i) v = (v << 8) | *p++; return v;
}

bool derive(const context_store_key_view & key, const std::array<uint8_t, 16> & uuid,
            const char * domain, size_t domain_size, context_store_format_digest & out) noexcept {
    std::array<uint8_t, 128> input {};
    if (!key.data || key.size != context_store_v1_server_canary_operator_key_bytes ||
        domain_size + uuid.size() > input.size()) return false;
    std::memcpy(input.data(), domain, domain_size);
    std::copy(uuid.begin(), uuid.end(), input.begin() + domain_size);
    const bool ok = context_store_hmac_sha256(key.data, key.size, input.data(),
                                               domain_size + uuid.size(), out);
    wipe(input.data(), input.size());
    return ok;
}

bool same_digest(const context_store_format_digest & a,
                 const context_store_format_digest & b) noexcept {
    uint8_t difference = 0;
    for (size_t i = 0; i != a.size(); ++i) difference |= a[i] ^ b[i];
    return difference == 0;
}

struct root_identity {
    uint64_t device = 0, inode = 0, mount = 0, uid = 0, fs = 0;
    uint32_t mode = 0;
};

bool inspect_root(int fd, root_identity & out) noexcept {
    struct stat value {};
    struct statx extended {};
    struct statvfs vfs {};
    if (::fstat(fd, &value) != 0 || !S_ISDIR(value.st_mode) ||
        (value.st_mode & 07777) != directory_mode ||
        ::syscall(SYS_statx, fd, "", AT_EMPTY_PATH | AT_STATX_SYNC_AS_STAT,
                  STATX_BASIC_STATS | STATX_MNT_ID, &extended) != 0 ||
        !(extended.stx_mask & STATX_MNT_ID) || ::fstatvfs(fd, &vfs) != 0) return false;
    out.device = value.st_dev; out.inode = value.st_ino; out.mount = extended.stx_mnt_id;
    out.uid = value.st_uid; out.fs = vfs.f_fsid; out.mode = value.st_mode & 07777;
    return out.device && out.inode && out.mount;
}

int acquire_writer_lock(int root, const root_identity & identity) noexcept {
    fd_owner lock(::openat(root, "writer.lock", O_RDWR | O_CLOEXEC | O_NOFOLLOW));
    struct stat value {};
    struct flock exclusive {};
    exclusive.l_type = F_WRLCK;
    exclusive.l_whence = SEEK_SET;
    if (lock.get() < 0 || ::fstat(lock.get(), &value) != 0 ||
        !S_ISREG(value.st_mode) || value.st_nlink != 1 || value.st_size != 0 ||
        static_cast<uint64_t>(value.st_dev) != identity.device ||
        static_cast<uint64_t>(value.st_uid) != identity.uid ||
        static_cast<uint32_t>(value.st_mode & 07777) != file_mode ||
        ::fcntl(lock.get(), F_OFD_SETLK, &exclusive) != 0) {
        return -1;
    }
    return lock.release();
}

struct record {
    uint8_t kind = 0; // 1 reservation, 2 final
    uint32_t capacity = 0, ordinal = 0;
    std::array<uint8_t, 16> uuid {};
    root_identity catalog, data, anchor;
    context_store_identity identity {};
    context_store_format_digest manifest {};
    context_store_format_digest producer {}, global_plan {}, rank_ownership {}, rank_placement {};
    uint64_t topology_epoch = 0;
};

std::array<uint8_t, record_bytes> encode_record(
        const record & r, const context_store_format_digest & key) noexcept {
    std::array<uint8_t, record_bytes> bytes {};
    uint8_t * p = bytes.data();
    std::copy(record_magic.begin(), record_magic.end(), p); p += record_magic.size();
    *p++ = 1; *p++ = r.kind; *p++ = 0; *p++ = 0;
    put_u32(p, r.capacity); put_u32(p, r.ordinal);
    std::copy(r.uuid.begin(), r.uuid.end(), p); p += r.uuid.size();
    auto root = [&p](const root_identity & x) {
        put_u64(p, x.device); put_u64(p, x.inode); put_u64(p, x.mount);
        put_u64(p, x.uid); put_u64(p, x.fs); put_u32(p, x.mode); put_u32(p, 0);
    };
    root(r.catalog); root(r.data); root(r.anchor);
    std::copy(r.identity.scope_namespace.begin(), r.identity.scope_namespace.end(), p); p += 32;
    std::copy(r.identity.checkpoint_lineage_id.begin(), r.identity.checkpoint_lineage_id.end(), p); p += 32;
    std::copy(r.identity.compatibility_root.begin(), r.identity.compatibility_root.end(), p); p += 32;
    put_u64(p, r.identity.policy_epoch);
    std::copy(r.manifest.begin(), r.manifest.end(), p); p += 32;
    std::copy(r.producer.begin(), r.producer.end(), p); p += 32;
    std::copy(r.global_plan.begin(), r.global_plan.end(), p); p += 32;
    std::copy(r.rank_ownership.begin(), r.rank_ownership.end(), p); p += 32;
    std::copy(r.rank_placement.begin(), r.rank_placement.end(), p); p += 32;
    put_u64(p, r.topology_epoch);
    // Remaining canonical reserved bytes are already zero.
    context_store_format_digest tag {};
    if (!context_store_hmac_sha256(key.data(), key.size(), bytes.data(), record_body_bytes, tag))
        bytes.fill(0);
    else std::copy(tag.begin(), tag.end(), bytes.begin() + record_body_bytes);
    wipe(tag.data(), tag.size());
    return bytes;
}

bool decode_record(const uint8_t * bytes, size_t size,
                   const context_store_format_digest & key, record & out) noexcept {
    if (!bytes || size != record_bytes) return false;
    context_store_format_digest tag {};
    if (!context_store_hmac_sha256(key.data(), key.size(), bytes, record_body_bytes, tag)) return false;
    uint8_t difference = 0;
    for (size_t i = 0; i != tag.size(); ++i) difference |= tag[i] ^ bytes[record_body_bytes + i];
    wipe(tag.data(), tag.size());
    if (difference || !std::equal(record_magic.begin(), record_magic.end(), bytes)) return false;
    const uint8_t * p = bytes + record_magic.size();
    if (*p++ != 1) return false;
    out.kind = *p++;
    if (*p++ != 0 || *p++ != 0 || (out.kind != 1 && out.kind != 2)) return false;
    out.capacity = get_u32(p); out.ordinal = get_u32(p);
    std::copy_n(p, 16, out.uuid.begin()); p += 16;
    auto root = [&p](root_identity & x) {
        x.device = get_u64(p); x.inode = get_u64(p); x.mount = get_u64(p);
        x.uid = get_u64(p); x.fs = get_u64(p); x.mode = get_u32(p); return get_u32(p) == 0;
    };
    if (!root(out.catalog) || !root(out.data) || !root(out.anchor)) return false;
    std::copy_n(p, 32, out.identity.scope_namespace.begin()); p += 32;
    std::copy_n(p, 32, out.identity.checkpoint_lineage_id.begin()); p += 32;
    std::copy_n(p, 32, out.identity.compatibility_root.begin()); p += 32;
    out.identity.policy_epoch = get_u64(p);
    std::copy_n(p, 32, out.manifest.begin()); p += 32;
    std::copy_n(p, 32, out.producer.begin()); p += 32;
    std::copy_n(p, 32, out.global_plan.begin()); p += 32;
    std::copy_n(p, 32, out.rank_ownership.begin()); p += 32;
    std::copy_n(p, 32, out.rank_placement.begin()); p += 32;
    out.topology_epoch = get_u64(p);
    while (p != bytes + record_body_bytes) if (*p++ != 0) return false;
    return true;
}

bool same_root(const root_identity & a, const root_identity & b) noexcept {
    return a.device == b.device && a.inode == b.inode && a.mount == b.mount &&
        a.uid == b.uid && a.fs == b.fs && a.mode == b.mode;
}
bool same_identity(const context_store_identity & a, const context_store_identity & b) noexcept {
    return same_digest(a.scope_namespace, b.scope_namespace) &&
        same_digest(a.checkpoint_lineage_id, b.checkpoint_lineage_id) &&
        same_digest(a.compatibility_root, b.compatibility_root) && a.policy_epoch == b.policy_epoch;
}

std::string name(size_t ordinal, const char * suffix) {
    char value[64];
    std::snprintf(value, sizeof(value), "slot-%02zu.%s.v1", ordinal, suffix);
    return value;
}

bool catalog_name_allowed(const std::string & value, size_t capacity) {
    if (value == "writer.lock") return true;
    constexpr std::array<const char *, 4> suffixes {{
        "reserve", "final", "reserve-pending", "final-pending"
    }};
    for (size_t i = 0; i != capacity; ++i)
        for (const char * suffix : suffixes)
            if (value == name(i, suffix)) return true;
    return false;
}

bool catalog_slot_directory(const std::string & value, size_t capacity, size_t & ordinal) {
    char expected[32];
    for (size_t i = 0; i != capacity; ++i) {
        std::snprintf(expected, sizeof(expected), "slot-%02zu", i);
        if (value == expected) { ordinal = i; return true; }
    }
    return false;
}

bool validate_catalog_layout(int root, const root_identity & identity, size_t capacity,
                             uint64_t quota_bytes,
                             const std::vector<root_identity> * slot_anchors = nullptr) noexcept {
    fd_owner scan_fd(::openat(root, ".", O_RDONLY | O_DIRECTORY | O_CLOEXEC | O_NOFOLLOW));
    DIR * raw = scan_fd.get() < 0 ? nullptr : ::fdopendir(scan_fd.release());
    if (!raw) return false;
    size_t count = 0;
    uint64_t logical = 0, allocated = 0;
    std::vector<std::pair<uint64_t, uint64_t>> inodes;
    bool ok = true;
    errno = 0;
    while (dirent * entry = ::readdir(raw)) {
        const std::string value(entry->d_name);
        if (value == "." || value == "..") continue;
        size_t directory_ordinal = capacity;
        const bool slot_directory = catalog_slot_directory(value, capacity, directory_ordinal);
        if (++count > 1 + capacity * 5 ||
            (!slot_directory && !catalog_name_allowed(value, capacity))) { ok = false; break; }
        fd_owner fd(::openat(root, value.c_str(), O_RDONLY | O_CLOEXEC | O_NOFOLLOW));
        struct stat st {};
        struct statx extended {};
        if (fd.get() < 0 || ::fstat(fd.get(), &st) != 0 ||
            (slot_directory ? !S_ISDIR(st.st_mode) : !S_ISREG(st.st_mode)) ||
            (!slot_directory && st.st_nlink != 1) ||
            (st.st_mode & 07777) != (slot_directory ? directory_mode : file_mode) ||
            static_cast<uint64_t>(st.st_dev) != identity.device ||
            static_cast<uint64_t>(st.st_uid) != identity.uid ||
            ::syscall(SYS_statx, fd.get(), "", AT_EMPTY_PATH | AT_STATX_SYNC_AS_STAT,
                      STATX_BASIC_STATS | STATX_MNT_ID, &extended) != 0 ||
            !(extended.stx_mask & STATX_MNT_ID) || extended.stx_mnt_id != identity.mount ||
            (!slot_directory &&
             (value == "writer.lock" ? st.st_size != 0 :
              st.st_size != static_cast<off_t>(record_bytes)))) {
            ok = false; break;
        }
        if (slot_directory && slot_anchors &&
            (directory_ordinal >= slot_anchors->size() ||
             static_cast<uint64_t>(st.st_dev) != (*slot_anchors)[directory_ordinal].device ||
             static_cast<uint64_t>(st.st_ino) != (*slot_anchors)[directory_ordinal].inode)) {
            ok = false; break;
        }
        const auto inode = std::make_pair(static_cast<uint64_t>(st.st_dev),
                                          static_cast<uint64_t>(st.st_ino));
        if (std::find(inodes.begin(), inodes.end(), inode) != inodes.end()) { ok = false; break; }
        inodes.push_back(inode);
        const uint64_t file_logical = static_cast<uint64_t>(st.st_size);
        const uint64_t blocks = static_cast<uint64_t>(st.st_blocks);
        if (blocks > UINT64_MAX / 512 || logical > UINT64_MAX - file_logical ||
            allocated > UINT64_MAX - blocks * 512) { ok = false; break; }
        logical += file_logical;
        allocated += blocks * 512;
        if (logical > quota_bytes || allocated > quota_bytes) { ok = false; break; }
    }
    if (errno != 0) ok = false;
    ::closedir(raw);
    return ok;
}

enum class file_state { absent, valid, invalid };
file_state read_record(int root, const root_identity & identity,
                       const std::string & filename,
                       const context_store_format_digest & key, record & value) noexcept {
    fd_owner fd(::openat(root, filename.c_str(), O_RDONLY | O_CLOEXEC | O_NOFOLLOW));
    if (fd.get() < 0) return errno == ENOENT ? file_state::absent : file_state::invalid;
    struct stat st {};
    struct statx extended {};
    if (::fstat(fd.get(), &st) || !S_ISREG(st.st_mode) || st.st_nlink != 1 ||
        (st.st_mode & 07777) != file_mode || st.st_size != static_cast<off_t>(record_bytes) ||
        static_cast<uint64_t>(st.st_dev) != identity.device ||
        static_cast<uint64_t>(st.st_uid) != identity.uid ||
        ::syscall(SYS_statx, fd.get(), "", AT_EMPTY_PATH | AT_STATX_SYNC_AS_STAT,
                  STATX_BASIC_STATS | STATX_MNT_ID, &extended) != 0 ||
        !(extended.stx_mask & STATX_MNT_ID) || extended.stx_mnt_id != identity.mount)
        return file_state::invalid;
    std::array<uint8_t, record_bytes> bytes {};
    size_t off = 0;
    while (off != bytes.size()) {
        const ssize_t n = ::pread(fd.get(), bytes.data() + off, bytes.size() - off, off);
        if (n < 0 && errno == EINTR) continue;
        if (n <= 0) return file_state::invalid;
        off += static_cast<size_t>(n);
    }
    return decode_record(bytes.data(), bytes.size(), key, value)
        ? file_state::valid : file_state::invalid;
}

bool publish_record(int root, size_t ordinal, const char * final_suffix,
                    const std::array<uint8_t, record_bytes> & bytes) noexcept {
    const std::string pending = name(ordinal, std::string(final_suffix).append("-pending").c_str());
    const std::string final = name(ordinal, final_suffix);
    fd_owner fd(::openat(root, pending.c_str(), O_WRONLY | O_CREAT | O_EXCL | O_CLOEXEC | O_NOFOLLOW,
                         file_mode));
    if (fd.get() < 0) return false;
    size_t off = 0;
    while (off != bytes.size()) {
        const ssize_t n = ::pwrite(fd.get(), bytes.data() + off, bytes.size() - off, off);
        if (n < 0 && errno == EINTR) continue;
        if (n <= 0) return false;
        off += static_cast<size_t>(n);
    }
    if (::fdatasync(fd.get()) || ::syscall(SYS_renameat2, root, pending.c_str(), root,
                                            final.c_str(), RENAME_NOREPLACE) || ::fsync(root)) return false;
    return true;
}

context_store_registered_id registered_id(const char * text) noexcept {
    context_store_registered_id id;
    const size_t n = std::strlen(text);
    if (n <= id.bytes.size()) { id.size = static_cast<uint8_t>(n); std::copy_n(text, n, id.bytes.begin()); }
    return id;
}

context_store_v1_catalog_status map_status(context_store_v1_server_canary_status s) noexcept {
    switch (s) {
        case context_store_v1_server_canary_status::published: return context_store_v1_catalog_status::published;
        case context_store_v1_server_canary_status::hit: return context_store_v1_catalog_status::hit;
        case context_store_v1_server_canary_status::miss_not_found: return context_store_v1_catalog_status::miss_not_found;
        case context_store_v1_server_canary_status::miss_incompatible: return context_store_v1_catalog_status::miss_incompatible;
        case context_store_v1_server_canary_status::miss_corrupt:
        case context_store_v1_server_canary_status::quarantined: return context_store_v1_catalog_status::miss_corrupt;
        case context_store_v1_server_canary_status::busy: return context_store_v1_catalog_status::busy;
        case context_store_v1_server_canary_status::storage: return context_store_v1_catalog_status::storage;
        default: return context_store_v1_catalog_status::source_rejected;
    }
}

} // namespace

class context_store_v1_catalog::implementation {
public:
    struct slot {
        root_identity data, anchor;
        std::unique_ptr<context_store_v1_server_canary> canary;
    };
    implementation(int fd, int lock_fd, const root_identity & catalog_identity_value,
                   const context_store_v1_catalog_config & c,
                   context_store_format_digest record_key_value,
                   context_store_format_digest manifest_key_value) noexcept
        : root(fd), writer_lock(lock_fd), catalog_identity(catalog_identity_value),
          config(c.child), capacity(c.slot_count),
          record_key(record_key_value), manifest_key(manifest_key_value) {
        // The catalog retains derived keys only; never retain borrowed owner
        // key material, path pointers, or caller-owned descriptors.
        config.operator_key = {};
        config.data_root_path = nullptr;
        config.anchor_root_path = nullptr;
        config.data_root_fd = -1;
        config.anchor_root_fd = -1;
    }
    ~implementation() { wipe(record_key.data(), 32); wipe(manifest_key.data(), 32); }

    bool expected(const record & r, size_t ordinal, uint8_t kind) const noexcept {
        return r.kind == kind && r.capacity == capacity && r.ordinal == ordinal &&
            r.uuid == config.store_uuid && same_root(r.catalog, catalog_identity) &&
            same_root(r.data, slots[ordinal].data) &&
            same_root(r.anchor, slots[ordinal].anchor) && nonzero(r.manifest) &&
            same_digest(r.producer, config.producer_identity) &&
            same_digest(r.global_plan, config.global_plan_digest) &&
            same_digest(r.rank_ownership, config.rank_ownership_digest) &&
            same_digest(r.rank_placement, config.rank_placement_digest) &&
            r.topology_epoch == config.topology_epoch;
    }
    record make_record(size_t ordinal, uint8_t kind, const context_store_identity & identity,
                       const context_store_format_digest & manifest) const noexcept {
        record r; r.kind = kind; r.capacity = capacity; r.ordinal = ordinal;
        r.uuid = config.store_uuid; r.catalog = catalog_identity;
        r.data = slots[ordinal].data; r.anchor = slots[ordinal].anchor;
        r.identity = identity; r.manifest = manifest; r.producer = config.producer_identity;
        r.global_plan = config.global_plan_digest;
        r.rank_ownership = config.rank_ownership_digest;
        r.rank_placement = config.rank_placement_digest;
        r.topology_epoch = config.topology_epoch;
        return r;
    }
    context_store_format_digest predict_manifest(
            const context_store_transformer_snapshot_v1 & snapshot) noexcept {
        context_store_v1_transformer_manifest_parameters p;
        p.store_uuid = config.store_uuid;
        p.compatibility_components = config.compatibility.components;
        p.producer_identity = config.producer_identity; p.global_plan_digest = config.global_plan_digest;
        p.rank_ownership_digest = config.rank_ownership_digest;
        p.rank_placement_digest = config.rank_placement_digest;
        p.generation = 1; p.topology_epoch = config.topology_epoch;
        p.logical_position = snapshot.tokens.size(); p.output_boundary = snapshot.tokens.size();
        p.signing_key.disposition = context_store_key_disposition::active;
        p.signing_key.key_id = registered_id(manifest_key_id); p.signing_key.generation = 1;
        p.signing_key.master_key = {manifest_key.data(), manifest_key.size()};
        auto encoded = context_store_encode_transformer_snapshot_v1(snapshot, p, config.limits);
        return encoded.status == context_store_v1_transformer_codec_status::encoded
            ? encoded.encoded.manifest_digest : context_store_format_digest {};
    }
    bool layout_valid() const noexcept {
        std::vector<root_identity> anchors;
        anchors.reserve(slots.size());
        for (const auto & slot : slots) anchors.push_back(slot.anchor);
        return validate_catalog_layout(root.get(), catalog_identity, capacity,
                                       config.quota_bytes, &anchors);
    }
    bool reserve_available(uint64_t remaining_records) const noexcept {
        struct statvfs s {};
        if (::fstatvfs(root.get(), &s) != 0 || s.f_frsize == 0) return false;
        const uint64_t fragment = s.f_frsize;
        if (record_bytes > UINT64_MAX - (fragment - 1)) return false;
        const uint64_t allocation = ((record_bytes + fragment - 1) / fragment) * fragment;
        if (remaining_records > UINT64_MAX / allocation) return false;
        const uint64_t remaining = remaining_records * allocation;
        if (config.reserve_bytes > UINT64_MAX - remaining) return false;
        const uint64_t blocks = s.f_bavail;
        if (blocks > UINT64_MAX / s.f_frsize) return true;
        return blocks * s.f_frsize >= config.reserve_bytes + remaining;
    }

    fd_owner root;
    fd_owner writer_lock;
    root_identity catalog_identity;
    context_store_v1_server_canary_config config;
    size_t capacity;
    context_store_format_digest record_key, manifest_key;
    std::vector<slot> slots;
};

context_store_v1_catalog::context_store_v1_catalog(std::unique_ptr<implementation> value) noexcept
    : implementation_(std::move(value)) {}
context_store_v1_catalog::~context_store_v1_catalog() = default;

context_store_v1_catalog_mutation_custody
context_store_v1_catalog::acquire_mutation_custody() noexcept {
    return context_store_v1_catalog_mutation_custody(mutation_mutex_);
}

context_store_v1_catalog_publish_result context_store_v1_catalog::publish(
        const context_store_transformer_snapshot_v1 & snapshot) noexcept {
    context_store_v1_catalog_publish_result result;
    std::unique_lock<std::mutex> mutation_lock(mutation_mutex_, std::try_to_lock);
    if (!mutation_lock.owns_lock()) {
        result.status = context_store_v1_catalog_status::busy;
        return result;
    }
    std::unique_lock<std::mutex> lock(operation_mutex_, std::try_to_lock);
    if (!lock.owns_lock()) { result.status = context_store_v1_catalog_status::busy; return result; }
    if (!implementation_) return result;
    if (!implementation_->layout_valid()) {
        result.status = context_store_v1_catalog_status::miss_corrupt; return result;
    }
    const auto predicted = implementation_->predict_manifest(snapshot);
    if (!nonzero(predicted)) return result;
    size_t available = implementation_->capacity;
    bool existing_match = false;
    context_store_format_digest existing_manifest {};
    for (size_t i = 0; i != implementation_->capacity; ++i) {
        record reservation, final, rp, fp;
        const auto rs = read_record(implementation_->root.get(), implementation_->catalog_identity,
                                    name(i, "reserve"), implementation_->record_key, reservation);
        const auto fs = read_record(implementation_->root.get(), implementation_->catalog_identity,
                                    name(i, "final"), implementation_->record_key, final);
        const auto rps = read_record(implementation_->root.get(), implementation_->catalog_identity,
                                     name(i, "reserve-pending"), implementation_->record_key, rp);
        const auto fps = read_record(implementation_->root.get(), implementation_->catalog_identity,
                                     name(i, "final-pending"), implementation_->record_key, fp);
        if (rs == file_state::invalid || fs == file_state::invalid || rps == file_state::invalid || fps == file_state::invalid) {
            result.status = context_store_v1_catalog_status::miss_corrupt; return result;
        }
        const bool occupied = rs != file_state::absent || fs != file_state::absent ||
                              rps != file_state::absent || fps != file_state::absent;
        if (!occupied) { if (available == implementation_->capacity) available = i; continue; }
        if ((rs == file_state::valid && !implementation_->expected(reservation, i, 1)) ||
            (fs == file_state::valid && !implementation_->expected(final, i, 2)) ||
            (rps == file_state::valid && !implementation_->expected(rp, i, 1)) ||
            (fps == file_state::valid && !implementation_->expected(fp, i, 2))) {
            result.status = context_store_v1_catalog_status::miss_corrupt; return result;
        }
        if ((fs == file_state::valid &&
             (rps == file_state::valid || fps == file_state::valid)) ||
            (rs == file_state::valid && rps == file_state::valid)) {
            result.status = context_store_v1_catalog_status::miss_corrupt;
            return result;
        }
        if (fs == file_state::valid &&
            (rs != file_state::valid ||
             !same_identity(reservation.identity, final.identity) ||
             !same_digest(reservation.manifest, final.manifest))) {
            result.status = context_store_v1_catalog_status::miss_corrupt;
            return result;
        }
        if (fs == file_state::valid &&
            same_identity(final.identity, snapshot.compatibility_identity)) {
            if (existing_match || !same_digest(final.manifest, predicted)) {
                result.status = context_store_v1_catalog_status::miss_corrupt;
                return result;
            }
            existing_match = true;
            existing_manifest = final.manifest;
        }
    }
    if (existing_match) {
        result.status = context_store_v1_catalog_status::published;
        result.selected_manifest = existing_manifest;
        return result;
    }
    if (available == implementation_->capacity) {
        result.status = context_store_v1_catalog_status::capacity_exhausted; return result;
    }
    // Before the reservation is visible, retain allocation headroom for both
    // the reservation and the later final catalog record.
    if (!implementation_->reserve_available(2)) {
        result.status = context_store_v1_catalog_status::storage; return result;
    }
    const record reservation = implementation_->make_record(
        available, 1, snapshot.compatibility_identity, predicted);
    if (!publish_record(implementation_->root.get(), available, "reserve",
                        encode_record(reservation, implementation_->record_key))) {
        result.status = context_store_v1_catalog_status::storage; return result;
    }
    const auto child = implementation_->slots[available].canary->publish(snapshot);
    if (child.status != context_store_v1_server_canary_status::published ||
        !same_digest(child.selected_manifest, predicted)) {
        result.status = map_status(child.status); return result;
    }
    if (!implementation_->reserve_available(1)) {
        result.status = context_store_v1_catalog_status::storage; return result;
    }
    const record final = implementation_->make_record(
        available, 2, snapshot.compatibility_identity, child.selected_manifest);
    if (!publish_record(implementation_->root.get(), available, "final",
                        encode_record(final, implementation_->record_key))) {
        result.status = context_store_v1_catalog_status::storage; return result;
    }
    result.status = context_store_v1_catalog_status::published;
    result.selected_manifest = child.selected_manifest;
    return result;
}

context_store_v1_catalog_restore_result context_store_v1_catalog::restore_exact(
        const llama_token * expected_tokens, size_t expected_token_count,
        const context_store_identity & identity,
        const context_store_transformer_profile_v1 & profile) noexcept {
    context_store_v1_catalog_restore_result result;
    std::unique_lock<std::mutex> lock(operation_mutex_, std::try_to_lock);
    if (!lock.owns_lock()) { result.status = context_store_v1_catalog_status::busy; return result; }
    if (!implementation_ || !expected_tokens || expected_token_count == 0) return result;
    if (!implementation_->layout_valid()) {
        result.status = context_store_v1_catalog_status::miss_corrupt; return result;
    }
    size_t selected = implementation_->capacity;
    context_store_format_digest selected_manifest {};
    bool has_empty_position = false;
    bool authenticated_incomplete_identity = false;
    for (size_t i = 0; i != implementation_->capacity; ++i) {
        record reservation, final, reserve_pending, final_pending;
        const auto reservation_state = read_record(
            implementation_->root.get(), implementation_->catalog_identity, name(i, "reserve"),
            implementation_->record_key, reservation);
        const auto state = read_record(implementation_->root.get(), implementation_->catalog_identity,
                                       name(i, "final"),
                                       implementation_->record_key, final);
        const auto reserve_pending_state = read_record(
            implementation_->root.get(), implementation_->catalog_identity,
            name(i, "reserve-pending"),
            implementation_->record_key, reserve_pending);
        const auto final_pending_state = read_record(
            implementation_->root.get(), implementation_->catalog_identity,
            name(i, "final-pending"),
            implementation_->record_key, final_pending);
        if (reservation_state == file_state::invalid || state == file_state::invalid ||
            reserve_pending_state == file_state::invalid ||
            final_pending_state == file_state::invalid) {
            result.status = context_store_v1_catalog_status::miss_corrupt;
            return result;
        }
        if ((reservation_state == file_state::valid &&
             !implementation_->expected(reservation, i, 1)) ||
            (state == file_state::valid && !implementation_->expected(final, i, 2)) ||
            (reserve_pending_state == file_state::valid &&
             !implementation_->expected(reserve_pending, i, 1)) ||
            (final_pending_state == file_state::valid &&
             !implementation_->expected(final_pending, i, 2))) {
            result.status = context_store_v1_catalog_status::miss_corrupt;
            return result;
        }
        if ((state == file_state::valid &&
             (reserve_pending_state == file_state::valid ||
              final_pending_state == file_state::valid)) ||
            (reservation_state == file_state::valid &&
             reserve_pending_state == file_state::valid)) {
            result.status = context_store_v1_catalog_status::miss_corrupt;
            return result;
        }
        if (reservation_state == file_state::absent && state == file_state::absent &&
            reserve_pending_state == file_state::absent &&
            final_pending_state == file_state::absent) {
            has_empty_position = true;
        }
        if (state == file_state::absent) {
            // A valid reservation or pending record still authenticates its
            // identity.  If it is the requested identity, its incomplete
            // publication is terminal uncertainty rather than evidence that
            // the caller may safely try a shorter prefix.
            const bool incomplete_match =
                (reservation_state == file_state::valid &&
                 same_identity(reservation.identity, identity)) ||
                (reserve_pending_state == file_state::valid &&
                 same_identity(reserve_pending.identity, identity)) ||
                (final_pending_state == file_state::valid &&
                 same_identity(final_pending.identity, identity));
            if (incomplete_match) {
                result.authenticated_record_selected = true;
                if (authenticated_incomplete_identity ||
                    selected != implementation_->capacity) {
                    result.status = context_store_v1_catalog_status::miss_corrupt;
                    return result;
                }
                authenticated_incomplete_identity = true;
            }
            continue;
        }
        if (reservation_state != file_state::valid ||
            !same_identity(reservation.identity, final.identity) ||
            !same_digest(reservation.manifest, final.manifest)) {
            result.status = context_store_v1_catalog_status::miss_corrupt;
            return result;
        }
        if (!same_identity(final.identity, identity)) continue;
        result.authenticated_record_selected = true;
        if (selected != implementation_->capacity) {
            result.status = context_store_v1_catalog_status::miss_corrupt;
            return result;
        }
        selected = i;
        selected_manifest = final.manifest;
    }
    if (authenticated_incomplete_identity) {
        result.status = context_store_v1_catalog_status::miss_corrupt;
        return result;
    }
    if (selected == implementation_->capacity) {
        result.status = has_empty_position
            ? context_store_v1_catalog_status::miss_not_found
            : context_store_v1_catalog_status::capacity_exhausted;
        return result;
    }
    const auto child = implementation_->slots[selected].canary->restore(
        selected_manifest, expected_tokens, expected_token_count, identity, profile);
    if (child.status == context_store_v1_server_canary_status::hit) {
        result.status = context_store_v1_catalog_status::hit;
        result.snapshot = std::move(child.snapshot);
        return result;
    }
    // An exact authenticated record is unique authority. Never fall through
    // after its child fails validation.
    result.status = map_status(child.status);
    return result;
}

context_store_v1_catalog_prefix_result
context_store_v1_catalog::discover_prefix_token_counts(
        const context_store_v1_catalog_prefix_query & query) noexcept {
    context_store_v1_catalog_prefix_result result;
    const auto fail = [&result](context_store_v1_catalog_status status) noexcept {
        result.status = status;
        result.token_counts.fill(0);
        result.token_count = 0;
        return result;
    };
    std::unique_lock<std::mutex> lock(operation_mutex_, std::try_to_lock);
    if (!lock.owns_lock()) {
        return fail(context_store_v1_catalog_status::busy);
    }
    if (!implementation_ ||
        !nonzero(query.compatibility_root) || !nonzero(query.producer_identity) ||
        !nonzero(query.scope_namespace) ||
        query.policy_epoch == 0 || query.max_token_count == 0 ||
        !context_store_transformer_profile_v1_is_admitted(query.profile) ||
        query.profile.world_size != 1 || query.profile.rank != 0 ||
        query.profile.architecture !=
            context_store_transformer_architecture_v1::transformer) {
        return fail(context_store_v1_catalog_status::source_rejected);
    }
    if (!same_digest(query.producer_identity,
                     implementation_->config.producer_identity)) {
        return fail(context_store_v1_catalog_status::miss_incompatible);
    }
    if (!implementation_->layout_valid()) {
        return fail(context_store_v1_catalog_status::miss_corrupt);
    }

    std::array<context_store_identity, context_store_v1_catalog_max_slots> identities {};
    size_t identity_count = 0;
    for (size_t i = 0; i != implementation_->capacity; ++i) {
        record reservation, final, reserve_pending, final_pending;
        const auto reservation_state = read_record(
            implementation_->root.get(), implementation_->catalog_identity,
            name(i, "reserve"), implementation_->record_key, reservation);
        const auto final_state = read_record(
            implementation_->root.get(), implementation_->catalog_identity,
            name(i, "final"), implementation_->record_key, final);
        const auto reserve_pending_state = read_record(
            implementation_->root.get(), implementation_->catalog_identity,
            name(i, "reserve-pending"), implementation_->record_key, reserve_pending);
        const auto final_pending_state = read_record(
            implementation_->root.get(), implementation_->catalog_identity,
            name(i, "final-pending"), implementation_->record_key, final_pending);

        if (reservation_state == file_state::invalid ||
            final_state == file_state::invalid ||
            reserve_pending_state == file_state::invalid ||
            final_pending_state == file_state::invalid ||
            (reservation_state == file_state::valid &&
             !implementation_->expected(reservation, i, 1)) ||
            (final_state == file_state::valid &&
             !implementation_->expected(final, i, 2)) ||
            (reserve_pending_state == file_state::valid &&
             !implementation_->expected(reserve_pending, i, 1)) ||
            (final_pending_state == file_state::valid &&
             !implementation_->expected(final_pending, i, 2)) ||
            (final_state == file_state::valid &&
             (reserve_pending_state == file_state::valid ||
              final_pending_state == file_state::valid)) ||
            (reservation_state == file_state::valid &&
             reserve_pending_state == file_state::valid)) {
            return fail(context_store_v1_catalog_status::miss_corrupt);
        }
        if (final_state == file_state::absent) {
            const auto relevant = [&query](file_state state,
                                           const record & candidate) noexcept {
                return state == file_state::valid &&
                    same_digest(candidate.identity.compatibility_root,
                                query.compatibility_root) &&
                    same_digest(candidate.identity.scope_namespace,
                                query.scope_namespace) &&
                    candidate.identity.policy_epoch == query.policy_epoch;
            };
            if (relevant(reservation_state, reservation) ||
                relevant(reserve_pending_state, reserve_pending) ||
                relevant(final_pending_state, final_pending)) {
                return fail(context_store_v1_catalog_status::miss_corrupt);
            }
            continue;
        }
        if (reservation_state != file_state::valid ||
            !same_identity(reservation.identity, final.identity) ||
            !same_digest(reservation.manifest, final.manifest) ||
            std::any_of(identities.begin(), identities.begin() + identity_count,
                [&final](const context_store_identity & identity) {
                    return same_identity(identity, final.identity);
                })) {
            return fail(context_store_v1_catalog_status::miss_corrupt);
        }
        identities[identity_count++] = final.identity;
        if (!same_digest(final.identity.compatibility_root, query.compatibility_root) ||
            !same_digest(final.identity.scope_namespace, query.scope_namespace) ||
            final.identity.policy_epoch != query.policy_epoch) {
            continue;
        }
        const auto inspected = implementation_->slots[i].canary->inspect_manifest(
            final.manifest, final.identity, query.profile);
        if (inspected.status != context_store_v1_server_canary_status::ready ||
            inspected.token_count == 0) {
            result.status = map_status(inspected.status);
            if (result.status == context_store_v1_catalog_status::ready ||
                result.status == context_store_v1_catalog_status::published ||
                result.status == context_store_v1_catalog_status::hit ||
                result.status == context_store_v1_catalog_status::miss_not_found ||
                result.status == context_store_v1_catalog_status::capacity_exhausted) {
                result.status = context_store_v1_catalog_status::miss_corrupt;
            }
            return fail(result.status);
        }
        if (inspected.token_count > query.max_token_count) continue;
        const auto end = result.token_counts.begin() + result.token_count;
        if (std::find(result.token_counts.begin(), end, inspected.token_count) == end) {
            if (result.token_count == result.token_counts.size()) {
                return fail(context_store_v1_catalog_status::miss_corrupt);
            }
            result.token_counts[result.token_count++] = inspected.token_count;
        }
    }

    std::sort(result.token_counts.begin(),
              result.token_counts.begin() + result.token_count);
    result.status = result.token_count == 0
        ? context_store_v1_catalog_status::miss_not_found
        : context_store_v1_catalog_status::ready;
    return result;
}

context_store_v1_catalog_open_result make_context_store_v1_catalog(
        const context_store_v1_catalog_config & c) noexcept {
    context_store_v1_catalog_open_result result;
    context_store_format_digest record_key {}, manifest_key {};
    digest_pair_wiper wipe_keys(record_key, manifest_key);
    try {
        if (!c.catalog_root_path || !c.slots || c.slot_count < 2 ||
            c.slot_count > context_store_v1_catalog_max_slots ||
            c.child.max_entries != 1)
            return result;
        fd_owner root(::open(c.catalog_root_path, O_RDONLY | O_DIRECTORY | O_CLOEXEC | O_NOFOLLOW));
        root_identity catalog_identity;
        if (root.get() < 0 || !inspect_root(root.get(), catalog_identity) ||
            !derive(c.child.operator_key, c.child.store_uuid, catalog_key_domain,
                    sizeof(catalog_key_domain), record_key) ||
            !derive(c.child.operator_key, c.child.store_uuid, manifest_key_domain,
                    sizeof(manifest_key_domain), manifest_key)) return result;
        fd_owner writer_lock(acquire_writer_lock(root.get(), catalog_identity));
        if (writer_lock.get() < 0) return result;
        struct statvfs catalog_vfs {};
        if (::fstatvfs(root.get(), &catalog_vfs) != 0 || catalog_vfs.f_frsize == 0 ||
            record_bytes > UINT64_MAX - (catalog_vfs.f_frsize - 1)) return result;
        const uint64_t record_allocation =
            ((record_bytes + catalog_vfs.f_frsize - 1) / catalog_vfs.f_frsize) * catalog_vfs.f_frsize;
        if (record_allocation > UINT64_MAX / (c.slot_count * 2)) return result;
        const uint64_t catalog_reservation = record_allocation * c.slot_count * 2;
        if (c.child.quota_bytes <= catalog_reservation ||
            !validate_catalog_layout(root.get(), catalog_identity, c.slot_count, c.child.quota_bytes))
            return result;
        auto impl = std::make_unique<context_store_v1_catalog::implementation>(
            root.release(), writer_lock.release(), catalog_identity,
            c, record_key, manifest_key);
        impl->slots.reserve(c.slot_count);
        const uint64_t child_quota = (c.child.quota_bytes - catalog_reservation) /
                                     c.slot_count;
        for (size_t i = 0; i != c.slot_count; ++i) {
            if (!c.slots[i].data_root_path || !c.slots[i].anchor_root_path) return result;
            fd_owner data(::open(c.slots[i].data_root_path, O_RDONLY | O_DIRECTORY | O_CLOEXEC | O_NOFOLLOW));
            fd_owner anchor(::open(c.slots[i].anchor_root_path, O_RDONLY | O_DIRECTORY | O_CLOEXEC | O_NOFOLLOW));
            context_store_v1_catalog::implementation::slot slot;
            if (data.get() < 0 || anchor.get() < 0 || !inspect_root(data.get(), slot.data) ||
                !inspect_root(anchor.get(), slot.anchor) || same_root(slot.data, slot.anchor)) return result;
            auto child = c.child;
            child.data_root_path = nullptr;
            child.anchor_root_path = nullptr;
            child.data_root_fd = data.get();
            child.anchor_root_fd = anchor.get();
            child.quota_bytes = child_quota;
            // If child publication shares the catalog mount, make the child
            // preserve the final catalog record's rounded allocation in
            // addition to the ordinary reserve.  On a distinct mount the
            // catalog's pre-reservation reserve+2 check independently retains
            // that headroom.
            if (slot.data.mount == catalog_identity.mount ||
                slot.anchor.mount == catalog_identity.mount) {
                if (child.reserve_bytes > UINT64_MAX - record_allocation) return result;
                child.reserve_bytes += record_allocation;
            }
            auto opened = make_context_store_v1_server_canary(child);
            if (!opened.canary) return result;
            slot.canary = std::move(opened.canary);
            impl->slots.push_back(std::move(slot));
        }
        if (!impl->layout_valid()) return result;
        result.catalog.reset(new context_store_v1_catalog(std::move(impl)));
        result.status = context_store_v1_catalog_status::ready;
    } catch (...) {
        result.status = context_store_v1_catalog_status::storage;
    }
    return result;
}

const char * context_store_v1_catalog_status_name(context_store_v1_catalog_status status) noexcept {
    switch (status) {
        case context_store_v1_catalog_status::ready: return "ready";
        case context_store_v1_catalog_status::published: return "published";
        case context_store_v1_catalog_status::hit: return "hit";
        case context_store_v1_catalog_status::miss_not_found: return "miss-not-found";
        case context_store_v1_catalog_status::miss_corrupt: return "miss-corrupt";
        case context_store_v1_catalog_status::miss_incompatible: return "miss-incompatible";
        case context_store_v1_catalog_status::source_rejected: return "source-rejected";
        case context_store_v1_catalog_status::capacity_exhausted: return "capacity-exhausted";
        case context_store_v1_catalog_status::busy: return "busy";
        case context_store_v1_catalog_status::storage: return "storage";
    }
    return "invalid";
}

} // namespace halofpx
