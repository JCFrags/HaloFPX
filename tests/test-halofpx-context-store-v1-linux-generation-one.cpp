#include "halofpx-context-store-v1-linux-generation-one.h"

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

class generation_one_roots {
public:
    generation_one_roots() {
        std::array<char, 72> pattern {};
        const char value[] = "/tmp/halofpx-v1-generation-one-XXXXXX";
        std::copy(value, value + sizeof(value), pattern.begin());
        char * created = ::mkdtemp(pattern.data());
        assert(created != nullptr);
        base_ = created;
        data_path_ = base_ / "data";
        anchor_path_ = base_ / "anchor";
        assert(::chmod(base_.c_str(), 0700) == 0);
        assert(::mkdir(data_path_.c_str(), 0700) == 0);
        assert(::mkdir(anchor_path_.c_str(), 0700) == 0);
        assert(::mkdir((data_path_ / "staging").c_str(), 0700) == 0);
        assert(::mkdir((data_path_ / "manifests").c_str(), 0700) == 0);
        assert(::mkdir((data_path_ / "objects").c_str(), 0700) == 0);
        create_lock(data_path_ / "writer.lock");
        create_lock(anchor_path_ / "writer.lock");
        data_fd_ = open_root(data_path_);
        anchor_fd_ = open_root(anchor_path_);
        data_identity_ = inspect_root(data_fd_);
        anchor_identity_ = inspect_root(anchor_fd_);
    }

    ~generation_one_roots() {
        if (data_fd_ >= 0) ::close(data_fd_);
        if (anchor_fd_ >= 0) ::close(anchor_fd_);
        std::error_code ignored;
        std::filesystem::remove_all(base_, ignored);
    }

    generation_one_roots(const generation_one_roots &) = delete;
    generation_one_roots & operator=(const generation_one_roots &) = delete;

    halofpx::context_store_v1_linux_publish_root data_root() const {
        return { data_fd_, data_identity_ };
    }

    halofpx::context_store_v1_linux_publish_root anchor_root() const {
        return { anchor_fd_, anchor_identity_ };
    }

    void corrupt_anchor() const {
        const auto path = anchor_path_ / "anchor.v1";
        const int fd = ::open(path.c_str(), O_RDWR | O_CLOEXEC | O_NOFOLLOW);
        assert(fd >= 0);
        uint8_t byte = 0;
        ssize_t count;
        do count = ::pread(fd, &byte, 1, 0); while (count < 0 && errno == EINTR);
        assert(count == 1);
        byte ^= 1;
        do count = ::pwrite(fd, &byte, 1, 0); while (count < 0 && errno == EINTR);
        assert(count == 1);
        assert(::fsync(fd) == 0);
        assert(::close(fd) == 0);
    }

private:
    static void create_lock(const std::filesystem::path & path) {
        const int fd = ::open(path.c_str(),
            O_RDWR | O_CREAT | O_EXCL | O_CLOEXEC | O_NOFOLLOW, 0600);
        assert(fd >= 0);
        assert(::fsync(fd) == 0);
        assert(::close(fd) == 0);
    }

    static int open_root(const std::filesystem::path & path) {
        const int fd = ::open(path.c_str(), O_RDONLY | O_DIRECTORY | O_CLOEXEC | O_NOFOLLOW);
        assert(fd >= 0);
        return fd;
    }

    static halofpx::context_store_linux_root_identity_v1 inspect_root(int fd) {
        struct stat value {};
        struct statx extended {};
        struct statfs filesystem {};
        assert(::fstat(fd, &value) == 0);
        assert(::syscall(SYS_statx, fd, "", AT_EMPTY_PATH | AT_STATX_SYNC_AS_STAT,
                         STATX_BASIC_STATS | STATX_MNT_ID, &extended) == 0);
        assert((extended.stx_mask & STATX_MNT_ID) != 0);
        assert(::fstatfs(fd, &filesystem) == 0);
        halofpx::context_store_linux_root_identity_v1 result;
        result.device = static_cast<uint64_t>(value.st_dev);
        result.inode = static_cast<uint64_t>(value.st_ino);
        result.mount_id = extended.stx_mnt_id;
        result.owner_uid = static_cast<uint64_t>(value.st_uid);
        result.mode = static_cast<uint32_t>(value.st_mode & 07777);
        result.filesystem_type = static_cast<uint64_t>(filesystem.f_type);
        return result;
    }

    std::filesystem::path base_;
    std::filesystem::path data_path_;
    std::filesystem::path anchor_path_;
    int data_fd_ = -1;
    int anchor_fd_ = -1;
    halofpx::context_store_linux_root_identity_v1 data_identity_;
    halofpx::context_store_linux_root_identity_v1 anchor_identity_;
};

struct generation_one_keys {
    std::array<uint8_t, halofpx::context_store_protected_canary_anchor_master_key_bytes> anchor {};
    std::array<uint8_t, halofpx::context_store_v1_attempt_master_key_bytes> attempt {};

    generation_one_keys() {
        anchor.fill(0x41);
        attempt.fill(0x51);
    }
};

halofpx::context_store_v1_linux_generation_one_config make_config(
        signed_two_object_fixture & fixture,
        const generation_one_roots & roots,
        generation_one_keys & keys,
        halofpx::context_store_v1_linux_generation_one_failpoint failpoint =
            halofpx::context_store_v1_linux_generation_one_failpoint::none) {
    fixture.refresh();
    halofpx::context_store_v1_linux_generation_one_config config;
    config.data_root = roots.data_root();
    config.anchor_root = roots.anchor_root();
    config.verification_policy = fixture.policy;
    config.admission.manifest = fixture.admission_metadata;
    config.admission.objects = fixture.admission_objects.data();
    config.admission.object_count = fixture.admission_objects.size();
    config.object_limits = { 4096, 1024 };
    config.max_total_frame_bytes = 8192;
    config.anchor_body.store_uuid = fixture.policy.anchor.store_uuid;
    config.anchor_body.namespace_id = fixture.admission_metadata.scope_namespace;
    config.anchor_body.policy_epoch = 1;
    config.anchor_body.checkpoint_lineage_id = fixture.admission_metadata.checkpoint_lineage_id;
    config.anchor_body.manifest_key_generation = 1;
    config.anchor_body.authority_epoch = 1;
    config.anchor_body.generation = 1;
    config.anchor_body.selected_manifest_digest = fixture.policy.anchor.selected_manifest_digest;
    config.anchor_body.has_predecessor = false;
    config.anchor_key.key_id = registered_id("halofpx-protected-anchor-v1");
    config.anchor_key.generation = 1;
    config.anchor_key.master_key = { keys.anchor.data(), keys.anchor.size() };
    config.attempt_key.master_key = { keys.attempt.data(), keys.attempt.size() };
    config.test_failpoint = failpoint;
    return config;
}

signed_two_object_fixture make_generation_one_fixture() {
    fixture_options options;
    options.policy_epoch = 1;
    return make_fixture(options);
}

void assert_exact_hit(
        const halofpx::context_store_v1_linux_generation_one & authority,
        const signed_two_object_fixture & fixture) {
    const auto result = authority.lookup(request_for(fixture));
    assert(result.status() == halofpx::context_store_lookup_status::hit);
    assert(result.is_hit() && result.candidate() != nullptr);
    const auto * candidate = dynamic_cast<const halofpx::context_store_v1_read_only_candidate *>(
        result.candidate());
    assert(candidate != nullptr && candidate->object_count() == 2);
    assert(candidate->manifest_digest() == fixture.policy.anchor.selected_manifest_digest);
    assert(candidate->payload(0).size == 5 && candidate->payload(1).size == 6);
}

void test_miss_publish_exact_hit() {
    auto fixture = make_generation_one_fixture();
    generation_one_roots roots;
    generation_one_keys keys;
    auto opened = halofpx::make_context_store_v1_linux_generation_one(
        make_config(fixture, roots, keys));
    assert(opened.status == halofpx::context_store_v1_linux_generation_one_status::ready);
    assert(opened.authority != nullptr);
    const auto miss = opened.authority->lookup(request_for(fixture));
    assert(!miss.is_hit() && miss.candidate() == nullptr);
    assert(miss.status() == halofpx::context_store_lookup_status::miss_incomplete);
    assert(opened.authority->publish(fixture.source()) ==
        halofpx::context_store_v1_linux_generation_one_status::published);
    assert_exact_hit(*opened.authority, fixture);
}

void test_restart_after_anchor_recovers_success() {
    auto fixture = make_generation_one_fixture();
    generation_one_roots roots;
    generation_one_keys keys;
    {
        auto opened = halofpx::make_context_store_v1_linux_generation_one(make_config(
            fixture, roots, keys,
            halofpx::context_store_v1_linux_generation_one_failpoint::after_anchor));
        assert(opened.status == halofpx::context_store_v1_linux_generation_one_status::ready);
        assert(opened.authority != nullptr);
        assert(opened.authority->publish(fixture.source()) ==
            halofpx::context_store_v1_linux_generation_one_status::interrupted);
    }
    auto restarted = halofpx::make_context_store_v1_linux_generation_one(
        make_config(fixture, roots, keys));
    assert(restarted.status ==
        halofpx::context_store_v1_linux_generation_one_status::recovered_success);
    assert(restarted.authority != nullptr);
    assert_exact_hit(*restarted.authority, fixture);
}

void test_corrupt_anchor_quarantines_and_misses() {
    auto fixture = make_generation_one_fixture();
    generation_one_roots roots;
    generation_one_keys keys;
    {
        auto opened = halofpx::make_context_store_v1_linux_generation_one(
            make_config(fixture, roots, keys));
        assert(opened.status == halofpx::context_store_v1_linux_generation_one_status::ready);
        assert(opened.authority != nullptr);
        assert(opened.authority->publish(fixture.source()) ==
            halofpx::context_store_v1_linux_generation_one_status::published);
    }
    roots.corrupt_anchor();
    auto restarted = halofpx::make_context_store_v1_linux_generation_one(
        make_config(fixture, roots, keys));
    assert(restarted.status == halofpx::context_store_v1_linux_generation_one_status::quarantined);
    assert(restarted.authority != nullptr && restarted.authority->quarantined());
    const auto miss = restarted.authority->lookup(request_for(fixture));
    assert(!miss.is_hit() && miss.candidate() == nullptr);
    assert(miss.status() == halofpx::context_store_lookup_status::miss_corrupt);
}

void test_mismatched_authority_domain_is_rejected() {
    auto fixture = make_generation_one_fixture();
    generation_one_roots roots;
    generation_one_keys keys;
    auto config = make_config(fixture, roots, keys);
    config.anchor_body.namespace_id[0] ^= 1;
    auto opened = halofpx::make_context_store_v1_linux_generation_one(config);
    assert(opened.status == halofpx::context_store_v1_linux_generation_one_status::invalid);
    assert(opened.authority == nullptr);
}

} // namespace

int main() {
    test_miss_publish_exact_hit();
    test_restart_after_anchor_recovers_success();
    test_corrupt_anchor_quarantines_and_misses();
    test_mismatched_authority_domain_is_rejected();
    std::cout << "halofpx Linux generation-one authority tests passed\n";
    return 0;
}
