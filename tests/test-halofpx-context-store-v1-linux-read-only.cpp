#include "halofpx-context-store-v1-linux-read-only.h"

// Reuse the exact target-owned two-object fixture admitted by the preceding
// memory-only seam. Renaming its test entry point keeps this focused adapter
// test from maintaining a second encoding/signing implementation.
int halofpx_v1_memory_fixture_test_entry();
#define main halofpx_v1_memory_fixture_test_entry
#include "test-halofpx-context-store-v1-read-only.cpp"
#undef main

#include <linux/stat.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/statfs.h>
#include <sys/syscall.h>
#include <unistd.h>

#include <cerrno>
#include <filesystem>
#include <fstream>
#include <stdexcept>

namespace {

class temp_snapshot_root {
public:
    explicit temp_snapshot_root(signed_two_object_fixture & fixture) : fixture_(fixture) {
        std::array<char, 64> pattern {};
        const char value[] = "/tmp/halofpx-v1-linux-read-only-XXXXXX";
        std::copy(value, value + sizeof(value), pattern.begin());
        char * created = ::mkdtemp(pattern.data());
        assert(created != nullptr);
        path_ = created;
        assert(::chmod(path_.c_str(), 0700) == 0);
        assert(::mkdir((path_ / "manifests").c_str(), 0700) == 0);
        assert(::mkdir((path_ / "objects").c_str(), 0700) == 0);
        root_fd_ = ::open(path_.c_str(), O_RDONLY | O_DIRECTORY | O_CLOEXEC | O_NOFOLLOW);
        assert(root_fd_ >= 0);
        fixture_.refresh();
        write_manifest();
        write_object(0, fixture_.frames[0]);
        write_object(1, fixture_.frames[1]);
    }

    ~temp_snapshot_root() {
        if (root_fd_ >= 0) ::close(root_fd_);
        std::error_code ignored;
        std::filesystem::remove_all(path_, ignored);
    }

    temp_snapshot_root(const temp_snapshot_root &) = delete;
    temp_snapshot_root & operator=(const temp_snapshot_root &) = delete;

    halofpx::context_store_v1_linux_read_only_source source() const {
        halofpx::context_store_v1_linux_read_only_source value;
        value.root_fd = root_fd_;
        value.root_identity = identity();
        value.verification_policy = fixture_.policy;
        value.admission.manifest = fixture_.admission_metadata;
        value.admission.objects = fixture_.admission_objects.data();
        value.admission.object_count = fixture_.admission_objects.size();
        value.object_limits = { 4096, 1024 };
        value.max_total_frame_bytes = 8192;
        return value;
    }

    std::filesystem::path manifest_path() const {
        return path_ / "manifests" /
            ("m-" + hex(fixture_.policy.anchor.selected_manifest_digest) + ".cbor");
    }

    std::filesystem::path object_path(size_t index) const {
        return path_ / "objects" /
            ("o-" + hex(fixture_.admission_objects[index].object_id) + ".bin");
    }

    void remove_object(size_t index) const {
        assert(::unlink(object_path(index).c_str()) == 0);
    }

    void write_object(size_t index, const bytes & value) const {
        write_file(object_path(index), value);
    }

    void replace_manifest_with_symlink() const {
        const auto selected = manifest_path();
        const auto held = selected.string() + ".held";
        assert(::rename(selected.c_str(), held.c_str()) == 0);
        assert(::symlink(held.c_str(), selected.c_str()) == 0);
    }

private:
    static void write_file(const std::filesystem::path & path, const bytes & value) {
        const int fd = ::open(path.c_str(), O_WRONLY | O_CREAT | O_TRUNC | O_CLOEXEC, 0600);
        assert(fd >= 0);
        size_t offset = 0;
        while (offset < value.size()) {
            const ssize_t count = ::write(fd, value.data() + offset, value.size() - offset);
            if (count < 0 && errno == EINTR) continue;
            assert(count > 0);
            offset += static_cast<size_t>(count);
        }
        assert(::close(fd) == 0);
        assert(::chmod(path.c_str(), 0600) == 0);
    }

    void write_manifest() const { write_file(manifest_path(), fixture_.manifest); }

    halofpx::context_store_linux_root_identity_v1 identity() const {
        struct stat value {};
        struct statx extended {};
        struct statfs filesystem {};
        assert(::fstat(root_fd_, &value) == 0);
        assert(::syscall(SYS_statx, root_fd_, "", AT_EMPTY_PATH | AT_STATX_SYNC_AS_STAT,
                         STATX_BASIC_STATS | STATX_MNT_ID, &extended) == 0);
        assert((extended.stx_mask & STATX_MNT_ID) != 0);
        assert(::fstatfs(root_fd_, &filesystem) == 0);
        halofpx::context_store_linux_root_identity_v1 result;
        result.device = static_cast<uint64_t>(value.st_dev);
        result.inode = static_cast<uint64_t>(value.st_ino);
        result.mount_id = extended.stx_mnt_id;
        result.owner_uid = static_cast<uint64_t>(value.st_uid);
        result.mode = static_cast<uint32_t>(value.st_mode & 07777);
        result.filesystem_type = static_cast<uint64_t>(filesystem.f_type);
        return result;
    }

    signed_two_object_fixture & fixture_;
    std::filesystem::path path_;
    int root_fd_ = -1;
};

std::unique_ptr<halofpx::context_store_provider> make_linux_provider(
        const temp_snapshot_root & root) {
    return halofpx::make_context_store_v1_linux_read_only_provider(root.source());
}

void test_golden_hit_and_closed_surface() {
    auto fixture = make_fixture();
    temp_snapshot_root root(fixture);
    auto provider = make_linux_provider(root);
    assert(std::string(provider->name()) == "halofpx-v1-linux-read-only");
    const auto capabilities = provider->capabilities();
    assert(!capabilities.persistent_reads && !capabilities.persistent_writes);
    assert(!capabilities.enumeration && !capabilities.anonymous_scope && !capabilities.shared_scope);
    assert(capabilities.admitted_state_profiles == 0 && capabilities.admitted_codecs == 0);
    assert(provider->publish({}) == halofpx::context_store_publish_status::disabled);

    auto result = provider->lookup(request_for(fixture));
    assert(result.is_hit() && result.status() == halofpx::context_store_lookup_status::hit);
    const auto * candidate = dynamic_cast<const halofpx::context_store_v1_read_only_candidate *>(
        result.candidate());
    assert(candidate != nullptr && candidate->object_count() == 2);
    assert(candidate->manifest_digest() == fixture.policy.anchor.selected_manifest_digest);
    assert(candidate->payload(0).size == 5 && candidate->payload(1).size == 6);
}

void test_missing_or_corrupt_second_object_is_atomic_miss() {
    auto fixture = make_fixture();
    temp_snapshot_root root(fixture);
    auto provider = make_linux_provider(root);
    root.remove_object(1);
    auto missing = provider->lookup(request_for(fixture));
    assert(!missing.is_hit() && missing.candidate() == nullptr);
    assert(missing.status() == halofpx::context_store_lookup_status::miss_incomplete);

    bytes corrupt = fixture.frames[1];
    corrupt.back() ^= 1;
    root.write_object(1, corrupt);
    auto rejected = provider->lookup(request_for(fixture));
    assert(!rejected.is_hit() && rejected.candidate() == nullptr);
    assert(rejected.status() == halofpx::context_store_lookup_status::miss_corrupt);
}

void test_hostile_manifest_symlink_is_corrupt_miss() {
    auto fixture = make_fixture();
    temp_snapshot_root root(fixture);
    auto provider = make_linux_provider(root);
    root.replace_manifest_with_symlink();
    auto result = provider->lookup(request_for(fixture));
    assert(!result.is_hit() && result.candidate() == nullptr);
    assert(result.status() == halofpx::context_store_lookup_status::miss_corrupt);
}

void test_factory_rejects_root_identity_mismatch() {
    auto fixture = make_fixture();
    temp_snapshot_root root(fixture);
    auto source = root.source();
    ++source.root_identity.inode;
    bool rejected = false;
    try {
        (void) halofpx::make_context_store_v1_linux_read_only_provider(source);
    } catch (const std::invalid_argument &) {
        rejected = true;
    }
    assert(rejected);

    source = root.source();
    source.admission.object_count = halofpx::context_store_manifest_max_objects + 1;
    rejected = false;
    try {
        (void) halofpx::make_context_store_v1_linux_read_only_provider(source);
    } catch (const std::invalid_argument &) {
        rejected = true;
    }
    assert(rejected);
}

} // namespace

int main() {
    test_golden_hit_and_closed_surface();
    test_missing_or_corrupt_second_object_is_atomic_miss();
    test_hostile_manifest_symlink_is_corrupt_miss();
    test_factory_rejects_root_identity_mismatch();
    std::cout << "halofpx Linux full-v1 read-only adapter tests passed\n";
    return 0;
}
