#include "halofpx-context-store-v1-linux-publish.h"

// Reuse the exact target-owned authenticated two-object fixture from L08d.
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

namespace {

class empty_publication_root {
public:
    empty_publication_root() {
        std::array<char, 64> pattern {};
        const char value[] = "/tmp/halofpx-v1-linux-publish-XXXXXX";
        std::copy(value, value + sizeof(value), pattern.begin());
        char * created = ::mkdtemp(pattern.data());
        assert(created != nullptr);
        path_ = created;
        assert(::chmod(path_.c_str(), 0700) == 0);
        assert(::mkdir((path_ / "staging").c_str(), 0700) == 0);
        assert(::mkdir((path_ / "manifests").c_str(), 0700) == 0);
        assert(::mkdir((path_ / "objects").c_str(), 0700) == 0);
        const int writer_lock = ::open((path_ / "writer.lock").c_str(),
            O_RDWR | O_CREAT | O_EXCL | O_CLOEXEC | O_NOFOLLOW, 0600);
        assert(writer_lock >= 0);
        assert(::fsync(writer_lock) == 0);
        assert(::close(writer_lock) == 0);
        root_fd_ = ::open(path_.c_str(), O_RDONLY | O_DIRECTORY | O_CLOEXEC | O_NOFOLLOW);
        assert(root_fd_ >= 0);
        identity_ = inspect_root();
    }

    ~empty_publication_root() {
        if (root_fd_ >= 0) ::close(root_fd_);
        std::error_code ignored;
        std::filesystem::remove_all(path_, ignored);
    }

    empty_publication_root(const empty_publication_root &) = delete;
    empty_publication_root & operator=(const empty_publication_root &) = delete;

    halofpx::context_store_v1_linux_publish_root publisher_root() const {
        return { root_fd_, identity_ };
    }

    halofpx::context_store_v1_linux_read_only_source reader_source(
            signed_two_object_fixture & fixture) const {
        fixture.refresh();
        halofpx::context_store_v1_linux_read_only_source source;
        source.root_fd = root_fd_;
        source.root_identity = identity_;
        source.verification_policy = fixture.policy;
        source.admission.manifest = fixture.admission_metadata;
        source.admission.objects = fixture.admission_objects.data();
        source.admission.object_count = fixture.admission_objects.size();
        source.object_limits = { 4096, 1024 };
        source.max_total_frame_bytes = 8192;
        return source;
    }

    std::filesystem::path manifest_path(const signed_two_object_fixture & fixture) const {
        return path_ / "manifests" /
            ("m-" + hex(fixture.policy.anchor.selected_manifest_digest) + ".cbor");
    }

    std::filesystem::path object_path(
            const signed_two_object_fixture & fixture, size_t index) const {
        return path_ / "objects" /
            ("o-" + hex(fixture.admission_objects[index].object_id) + ".bin");
    }

    std::filesystem::path writer_lock_path() const { return path_ / "writer.lock"; }

    bool published_files_absent(const signed_two_object_fixture & fixture) const {
        return !std::filesystem::exists(manifest_path(fixture)) &&
            !std::filesystem::exists(object_path(fixture, 0)) &&
            !std::filesystem::exists(object_path(fixture, 1));
    }

    static void assert_private_immutable_file(const std::filesystem::path & path) {
        struct stat value {};
        assert(::lstat(path.c_str(), &value) == 0);
        assert(S_ISREG(value.st_mode));
        assert((value.st_mode & 07777) == 0600);
        assert(value.st_nlink == 1);
    }

private:
    halofpx::context_store_linux_root_identity_v1 inspect_root() const {
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

    std::filesystem::path path_;
    int root_fd_ = -1;
    halofpx::context_store_linux_root_identity_v1 identity_;
};

halofpx::context_store_v1_publish_attempt_id attempt(uint8_t value) {
    halofpx::context_store_v1_publish_attempt_id result {};
    result.fill(value);
    return result;
}

void test_miss_publish_restart_hit() {
    auto fixture = make_fixture();
    empty_publication_root root;

    {
        auto reader = halofpx::make_context_store_v1_linux_read_only_provider(
            root.reader_source(fixture));
        const auto result = reader->lookup(request_for(fixture));
        assert(!result.is_hit() && result.candidate() == nullptr);
        assert(result.status() == halofpx::context_store_lookup_status::miss_incomplete);
    }

    {
        auto publisher = halofpx::make_context_store_v1_linux_snapshot_materializer(
            root.publisher_root());
        assert(publisher->publish(attempt(0x41), fixture.source()) ==
            halofpx::context_store_v1_linux_publish_status::materialized_non_authoritative);
    }

    empty_publication_root::assert_private_immutable_file(root.manifest_path(fixture));
    empty_publication_root::assert_private_immutable_file(root.object_path(fixture, 0));
    empty_publication_root::assert_private_immutable_file(root.object_path(fixture, 1));

    auto restarted = halofpx::make_context_store_v1_linux_read_only_provider(
        root.reader_source(fixture));
    const auto result = restarted->lookup(request_for(fixture));
    assert(result.is_hit());
    const auto * candidate = dynamic_cast<const halofpx::context_store_v1_read_only_candidate *>(
        result.candidate());
    assert(candidate != nullptr && candidate->object_count() == 2);
    const std::array<uint8_t, 5> tokens = { 0x01,0x02,0x03,0x04,0x05 };
    const std::array<uint8_t, 6> kv = { 0xa0,0xb1,0xc2,0xd3,0xe4,0xf5 };
    assert(candidate->payload(0).size == tokens.size());
    assert(candidate->payload(1).size == kv.size());
    assert(std::equal(tokens.begin(), tokens.end(), candidate->payload(0).data));
    assert(std::equal(kv.begin(), kv.end(), candidate->payload(1).data));
}

void test_preverification_failure_publishes_nothing() {
    auto fixture = make_fixture();
    empty_publication_root root;
    fixture.frames[1].back() ^= 1;
    auto publisher = halofpx::make_context_store_v1_linux_snapshot_materializer(
        root.publisher_root());
    assert(publisher->publish(attempt(0x52), fixture.source()) ==
        halofpx::context_store_v1_linux_publish_status::verification);
    assert(root.published_files_absent(fixture));
}

void test_collision_preserves_original_snapshot() {
    auto fixture = make_fixture();
    empty_publication_root root;
    auto publisher = halofpx::make_context_store_v1_linux_snapshot_materializer(
        root.publisher_root());
    assert(publisher->publish(attempt(0x63), fixture.source()) ==
        halofpx::context_store_v1_linux_publish_status::materialized_non_authoritative);
    assert(publisher->publish(attempt(0x64), fixture.source()) ==
        halofpx::context_store_v1_linux_publish_status::already_equal_non_authoritative);

    auto reader = halofpx::make_context_store_v1_linux_read_only_provider(
        root.reader_source(fixture));
    const auto result = reader->lookup(request_for(fixture));
    assert(result.is_hit());
    const auto * candidate = dynamic_cast<const halofpx::context_store_v1_read_only_candidate *>(
        result.candidate());
    assert(candidate != nullptr && candidate->object_count() == 2);
    assert(candidate->payload(0).size == 5 && candidate->payload(1).size == 6);
}

void test_root_scoped_ofd_fence_is_busy() {
    auto fixture = make_fixture();
    empty_publication_root root;
    const int lock_fd = ::open(root.writer_lock_path().c_str(), O_RDWR | O_CLOEXEC | O_NOFOLLOW);
    assert(lock_fd >= 0);
    struct flock lock {};
    lock.l_type = F_WRLCK;
    lock.l_whence = SEEK_SET;
    assert(::fcntl(lock_fd, F_OFD_SETLK, &lock) == 0);

    auto materializer = halofpx::make_context_store_v1_linux_snapshot_materializer(
        root.publisher_root());
    assert(materializer->publish(attempt(0x75), fixture.source()) ==
        halofpx::context_store_v1_linux_publish_status::busy);
    assert(root.published_files_absent(fixture));
    assert(::close(lock_fd) == 0);
}

} // namespace

int main() {
    test_miss_publish_restart_hit();
    test_preverification_failure_publishes_nothing();
    test_collision_preserves_original_snapshot();
    test_root_scoped_ofd_fence_is_busy();
    std::cout << "halofpx Linux full-v1 publisher tests passed\n";
    return 0;
}
