#include "halofpx-context-store-v1-prefix-selector.h"

#ifdef NDEBUG
#undef NDEBUG
#endif
#include <cassert>

#include <algorithm>
#include <array>
#include <cerrno>
#include <cstdio>
#include <filesystem>
#include <fcntl.h>
#include <fstream>
#include <iterator>
#include <map>
#include <string>
#include <sys/stat.h>
#include <unistd.h>
#include <vector>

namespace {

constexpr size_t slot_count = 4;

void create_lock(const std::filesystem::path & path) {
    const int fd = ::open(path.c_str(), O_RDWR | O_CREAT | O_EXCL | O_CLOEXEC | O_NOFOLLOW, 0600);
    assert(fd >= 0);
    assert(::fsync(fd) == 0);
    assert(::close(fd) == 0);
}

void create_child_layout(const std::filesystem::path & data,
                         const std::filesystem::path & anchor) {
    assert(::mkdir(data.c_str(), 0700) == 0);
    assert(::mkdir(anchor.c_str(), 0700) == 0);
    assert(::mkdir((data / "staging").c_str(), 0700) == 0);
    assert(::mkdir((data / "objects").c_str(), 0700) == 0);
    assert(::mkdir((data / "manifests").c_str(), 0700) == 0);
    create_lock(data / "writer.lock");
    create_lock(anchor / "writer.lock");
}

class catalog_roots {
public:
    catalog_roots() {
        std::array<char, 72> pattern {};
        const char value[] = "/tmp/halofpx-prefix-selector-XXXXXX";
        std::copy(value, value + sizeof(value), pattern.begin());
        const char * created = ::mkdtemp(pattern.data());
        assert(created != nullptr);
        base = created;
        catalog = base / "catalog";
        assert(::chmod(base.c_str(), 0700) == 0);
        assert(::mkdir(catalog.c_str(), 0700) == 0);
        create_lock(catalog / "writer.lock");
        for (size_t index = 0; index < slot_count; ++index) {
            data[index] = base / ("data-" + std::to_string(index));
            anchor[index] = base / ("anchor-" + std::to_string(index));
            create_child_layout(data[index], anchor[index]);
            data_text[index] = data[index].string();
            anchor_text[index] = anchor[index].string();
            slots[index] = { data_text[index].c_str(), anchor_text[index].c_str() };
        }
        catalog_text = catalog.string();
    }

    ~catalog_roots() {
        std::error_code ignored;
        std::filesystem::remove_all(base, ignored);
    }

    catalog_roots(const catalog_roots &) = delete;
    catalog_roots & operator=(const catalog_roots &) = delete;

    std::filesystem::path base;
    std::filesystem::path catalog;
    std::array<std::filesystem::path, slot_count> data;
    std::array<std::filesystem::path, slot_count> anchor;
    std::string catalog_text;
    std::array<std::string, slot_count> data_text;
    std::array<std::string, slot_count> anchor_text;
    std::array<halofpx::context_store_v1_catalog_slot_config, slot_count> slots {};
};

void cbor_head(std::vector<uint8_t> & out, uint8_t major, uint64_t value) {
    assert(value <= 0xff);
    if (value < 24) {
        out.push_back(static_cast<uint8_t>((major << 5) | value));
    } else {
        out.push_back(static_cast<uint8_t>((major << 5) | 24));
        out.push_back(static_cast<uint8_t>(value));
    }
}

void bind_compatibility(halofpx::context_store_v1_server_canary_config & config) {
    std::vector<uint8_t> encoded;
    cbor_head(encoded, 5, config.compatibility.components.size());
    for (size_t index = 0; index < config.compatibility.components.size(); ++index) {
        cbor_head(encoded, 0, index);
        cbor_head(encoded, 2, config.compatibility.components[index].size());
        encoded.insert(encoded.end(), config.compatibility.components[index].begin(),
                       config.compatibility.components[index].end());
    }
    static constexpr char domain[] = "halofpx.compat.v1";
    std::vector<uint8_t> preimage(reinterpret_cast<const uint8_t *>(domain),
                                  reinterpret_cast<const uint8_t *>(domain) + sizeof(domain));
    preimage.insert(preimage.end(), encoded.begin(), encoded.end());
    assert(halofpx::context_store_sha256(preimage.data(), preimage.size(),
                                        config.compatibility.root));
}

halofpx::context_store_v1_catalog_config make_config(
        catalog_roots & roots, const std::array<uint8_t, 32> & operator_key) {
    halofpx::context_store_v1_catalog_config config;
    config.catalog_root_path = roots.catalog_text.c_str();
    config.slots = roots.slots.data();
    config.slot_count = roots.slots.size();
    config.child.operator_key = { operator_key.data(), operator_key.size() };
    config.child.store_uuid.fill(0x11);
    for (size_t index = 0; index < config.child.compatibility.components.size(); ++index) {
        config.child.compatibility.components[index].fill(static_cast<uint8_t>(0x20 + index));
    }
    bind_compatibility(config.child);
    config.child.producer_identity.fill(0x31);
    config.child.global_plan_digest.fill(0x32);
    config.child.rank_ownership_digest.fill(0x33);
    config.child.rank_placement_digest.fill(0x34);
    config.child.topology_epoch = 1;
    config.child.quota_bytes = 1024 * 1024;
    config.child.max_entries = 1;
    config.child.limits = { { 64, 16 }, 4096, 65536 };
    return config;
}

halofpx::context_store_transformer_profile_v1 admitted_profile() {
    halofpx::context_store_transformer_profile_v1 profile;
    profile.target_only = true;
    profile.world_size = 1;
    profile.rank = 0;
    profile.architecture = halofpx::context_store_transformer_architecture_v1::transformer;
    profile.greedy_memoryless_sampling = true;
    return profile;
}

halofpx::context_store_exact_session_inputs_v1 exact_inputs(
        const halofpx::context_store_v1_catalog_config & config,
        const std::array<uint8_t, 32> & session_key,
        const std::vector<llama_token> & tokens) {
    halofpx::context_store_exact_session_inputs_v1 exact;
    exact.derivation_key = { session_key.data(), session_key.size() };
    exact.scope_namespace.fill(0x51);
    exact.compatibility_root = config.child.compatibility.root;
    exact.tokens = tokens.data();
    exact.token_count = tokens.size();
    exact.logical_boundary = tokens.size();
    exact.output_boundary = tokens.size();
    exact.profile = halofpx::context_store_exact_session_profile_v1::
        target_only_greedy_memoryless;
    exact.global_plan_digest = config.child.global_plan_digest;
    exact.rank_ownership_digest = config.child.rank_ownership_digest;
    exact.rank_placement_digest = config.child.rank_placement_digest;
    exact.topology_epoch = config.child.topology_epoch;
    exact.world_size = 1;
    exact.rank = 0;
    return exact;
}

halofpx::context_store_transformer_snapshot_v1 snapshot_for(
        const halofpx::context_store_v1_catalog_config & config,
        const std::array<uint8_t, 32> & session_key,
        const std::vector<llama_token> & tokens,
        uint64_t policy_epoch,
        std::initializer_list<uint8_t> state) {
    auto exact = exact_inputs(config, session_key, tokens);
    const auto resolved = halofpx::context_store_resolve_exact_session_v1(exact);
    assert(resolved.resolved());

    halofpx::context_store_transformer_snapshot_v1 snapshot;
    snapshot.compatibility_identity.compatibility_root = exact.compatibility_root;
    snapshot.compatibility_identity.scope_namespace = exact.scope_namespace;
    snapshot.compatibility_identity.checkpoint_lineage_id = resolved.session_id;
    snapshot.compatibility_identity.policy_epoch = policy_epoch;
    snapshot.profile = admitted_profile();
    snapshot.tokens = tokens;
    snapshot.state.assign(state);
    return snapshot;
}

halofpx::context_store_v1_prefix_selector_request request_for(
        const halofpx::context_store_v1_catalog_config & config,
        const std::array<uint8_t, 32> & session_key,
        const std::vector<llama_token> & tokens,
        uint64_t policy_epoch,
        const size_t * boundaries,
        size_t boundary_count) {
    halofpx::context_store_v1_prefix_selector_request request;
    request.exact_session = exact_inputs(config, session_key, tokens);
    request.candidate_boundaries = boundaries;
    request.candidate_boundary_count = boundary_count;
    request.policy_epoch = policy_epoch;
    request.profile = admitted_profile();
    return request;
}

std::vector<llama_token> prefix(const std::vector<llama_token> & tokens, size_t size) {
    assert(size <= tokens.size());
    return { tokens.begin(), tokens.begin() + static_cast<std::ptrdiff_t>(size) };
}

void publish(halofpx::context_store_v1_catalog & catalog,
             const halofpx::context_store_transformer_snapshot_v1 & snapshot) {
    const auto result = catalog.publish(snapshot);
    if (result.status != halofpx::context_store_v1_catalog_status::published) {
        std::fprintf(stderr, "catalog publish failed: %s tokens=%zu state=%zu policy=%llu\n",
                     halofpx::context_store_v1_catalog_status_name(result.status),
                     snapshot.tokens.size(), snapshot.state.size(),
                     static_cast<unsigned long long>(snapshot.compatibility_identity.policy_epoch));
    }
    assert(result.status == halofpx::context_store_v1_catalog_status::published);
}

using tree_state = std::map<std::string, std::vector<char>>;

tree_state inspect_tree(const std::filesystem::path & base) {
    tree_state result;
    for (const auto & entry : std::filesystem::recursive_directory_iterator(base)) {
        std::string relative = std::filesystem::relative(entry.path(), base).generic_string();
        if (entry.is_directory()) {
            result.emplace(relative + '/', std::vector<char> {});
            continue;
        }
        std::ifstream input(entry.path(), std::ios::binary);
        assert(input.good());
        result.emplace(std::move(relative),
                       std::vector<char>(std::istreambuf_iterator<char>(input), {}));
    }
    return result;
}

void flip_first_byte(const std::filesystem::path & path) {
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

void truncate_last_byte(const std::filesystem::path & path) {
    const int fd = ::open(path.c_str(), O_RDWR | O_CLOEXEC | O_NOFOLLOW);
    assert(fd >= 0);
    struct stat metadata {};
    assert(::fstat(fd, &metadata) == 0);
    assert(metadata.st_size > 1);
    assert(::ftruncate(fd, metadata.st_size - 1) == 0);
    assert(::fsync(fd) == 0);
    assert(::close(fd) == 0);
}

std::filesystem::path first_object(const std::filesystem::path & data_root) {
    for (const auto & entry : std::filesystem::directory_iterator(data_root / "objects")) {
        if (entry.is_regular_file()) return entry.path();
    }
    return {};
}

void test_longest_multiple_prefixes_and_suffix_boundary() {
    catalog_roots roots;
    std::array<uint8_t, 32> operator_key {}, session_key {};
    operator_key.fill(0x41);
    session_key.fill(0x71);
    auto config = make_config(roots, operator_key);
    auto opened = halofpx::make_context_store_v1_catalog(config);
    assert(opened.catalog != nullptr);

    const std::vector<llama_token> tokens { 2, 3, 5, 7, 11, 13, 17, 19 };
    publish(*opened.catalog, snapshot_for(config, session_key, prefix(tokens, 2), 1, { 0x22 }));
    publish(*opened.catalog, snapshot_for(config, session_key, prefix(tokens, 4), 1, { 0x44 }));
    publish(*opened.catalog, snapshot_for(config, session_key, prefix(tokens, 6), 1, { 0x66 }));

    const std::array<size_t, 4> boundaries { 2, 4, 6, 8 };
    const auto result = halofpx::context_store_v1_restore_longest_prefix(
        *opened.catalog,
        request_for(config, session_key, tokens, 1, boundaries.data(), boundaries.size()));
    assert(result.hit());
    assert(result.matched_token_count == 6);
    assert(result.restored_token_count == 6);
    assert(result.residual_token_offset == 6);
    assert(result.residual_token_count == 2);
    assert(result.candidates_examined == 2);
    assert(result.validation_time_ns > 0);
    assert(result.snapshot.tokens == prefix(tokens, 6));
    assert(result.snapshot.state == std::vector<uint8_t>({ 0x66 }));
    assert(result.fallback_reason == halofpx::context_store_v1_prefix_fallback_reason::none);
}

void test_full_empty_and_exact_token_behavior() {
    catalog_roots roots;
    std::array<uint8_t, 32> operator_key {}, session_key {};
    operator_key.fill(0x42);
    session_key.fill(0x72);
    auto config = make_config(roots, operator_key);
    auto opened = halofpx::make_context_store_v1_catalog(config);
    assert(opened.catalog != nullptr);

    const std::vector<llama_token> tokens { 23, 29, 31, 37 };
    publish(*opened.catalog, snapshot_for(config, session_key, tokens, 1, { 0xa4 }));
    const std::array<size_t, 1> full_boundary { tokens.size() };
    auto result = halofpx::context_store_v1_restore_longest_prefix(
        *opened.catalog,
        request_for(config, session_key, tokens, 1, full_boundary.data(), full_boundary.size()));
    assert(result.hit());
    assert(result.matched_token_count == tokens.size());
    assert(result.residual_token_offset == tokens.size());
    assert(result.residual_token_count == 0);

    result = halofpx::context_store_v1_restore_longest_prefix(
        *opened.catalog, request_for(config, session_key, tokens, 1, nullptr, 0));
    assert(result.status == halofpx::context_store_v1_prefix_selector_status::miss_not_found);
    assert(result.fallback_reason ==
           halofpx::context_store_v1_prefix_fallback_reason::no_candidate_boundaries);
    assert(result.residual_token_count == tokens.size());
    assert(result.candidates_examined == 0);

    std::vector<llama_token> changed = tokens;
    changed.back() += 1;
    result = halofpx::context_store_v1_restore_longest_prefix(
        *opened.catalog,
        request_for(config, session_key, changed, 1,
                    full_boundary.data(), full_boundary.size()));
    assert(!result.hit());
    assert(result.snapshot.tokens.empty() && result.snapshot.state.empty());
    assert(result.residual_token_count == changed.size());

    const std::vector<llama_token> no_tokens;
    result = halofpx::context_store_v1_restore_longest_prefix(
        *opened.catalog, request_for(config, session_key, no_tokens, 1, nullptr, 0));
    assert(result.status == halofpx::context_store_v1_prefix_selector_status::source_rejected);
    assert(result.fallback_reason ==
           halofpx::context_store_v1_prefix_fallback_reason::invalid_request);
}

void test_exact_tie_and_boundary_rejection() {
    catalog_roots roots;
    std::array<uint8_t, 32> operator_key {}, session_key {}, alternate_session_key {};
    operator_key.fill(0x43);
    session_key.fill(0x73);
    alternate_session_key.fill(0x7b);
    auto config = make_config(roots, operator_key);
    auto opened = halofpx::make_context_store_v1_catalog(config);
    assert(opened.catalog != nullptr);

    const std::vector<llama_token> tokens { 41, 43, 47, 53, 59 };
    const auto checkpoint = prefix(tokens, 4);
    publish(*opened.catalog, snapshot_for(config, session_key, checkpoint, 1, { 0xb1 }));
    publish(*opened.catalog,
            snapshot_for(config, alternate_session_key, checkpoint, 1, { 0xb2 }));

    const std::array<size_t, 1> one_boundary { 4 };
    auto result = halofpx::context_store_v1_restore_longest_prefix(
        *opened.catalog,
        request_for(config, alternate_session_key, tokens, 1,
                    one_boundary.data(), one_boundary.size()));
    assert(result.hit());
    assert(result.snapshot.state == std::vector<uint8_t>({ 0xb2 }));

    // Query the older same-length entry after the alternate authority was
    // published, so scan order or newest-wins behavior cannot satisfy the test.
    result = halofpx::context_store_v1_restore_longest_prefix(
        *opened.catalog,
        request_for(config, session_key, tokens, 1,
                    one_boundary.data(), one_boundary.size()));
    assert(result.hit());
    assert(result.snapshot.state == std::vector<uint8_t>({ 0xb1 }));

    // Make any accidental catalog access terminal. Every invalid-boundary case
    // below must reject before restore_exact observes this corrupted record.
    flip_first_byte(roots.catalog / "slot-00.final.v1");
    const auto expect_pre_catalog_rejection = [&](const size_t * boundaries,
                                                   size_t boundary_count) {
        const auto rejected = halofpx::context_store_v1_restore_longest_prefix(
            *opened.catalog,
            request_for(config, alternate_session_key, tokens, 1,
                        boundaries, boundary_count));
        assert(rejected.status ==
               halofpx::context_store_v1_prefix_selector_status::source_rejected);
        assert(rejected.fallback_reason ==
               halofpx::context_store_v1_prefix_fallback_reason::invalid_boundaries);
        assert(rejected.candidates_examined == 0);
        assert(rejected.matched_token_count == 0);
        assert(rejected.restored_token_count == 0);
        assert(rejected.residual_token_offset == 0);
        assert(rejected.residual_token_count == tokens.size());
        assert(rejected.snapshot.tokens.empty() && rejected.snapshot.state.empty());
    };

    expect_pre_catalog_rejection(nullptr, 1);
    const std::array<size_t, 1> zero { 0 };
    expect_pre_catalog_rejection(zero.data(), zero.size());
    const std::array<size_t, 2> descending { 4, 2 };
    expect_pre_catalog_rejection(descending.data(), descending.size());
    const std::array<size_t, 2> duplicate { 4, 4 };
    expect_pre_catalog_rejection(duplicate.data(), duplicate.size());
    const std::array<size_t, 2> out_of_range { 2, tokens.size() + 1 };
    expect_pre_catalog_rejection(out_of_range.data(), out_of_range.size());
    const std::array<size_t,
                     halofpx::context_store_v1_prefix_selector_max_boundaries + 1>
        too_many { 1, 2, 3, 4, 5, 5, 5, 5, 5 };
    expect_pre_catalog_rejection(too_many.data(), too_many.size());
}

void test_different_longer_authority_and_incompatible_authority_cannot_win() {
    catalog_roots roots;
    std::array<uint8_t, 32> operator_key {}, session_key {};
    operator_key.fill(0x44);
    session_key.fill(0x74);
    auto config = make_config(roots, operator_key);
    auto opened = halofpx::make_context_store_v1_catalog(config);
    assert(opened.catalog != nullptr);

    const std::vector<llama_token> tokens { 61, 67, 71, 73, 79, 83, 89 };
    publish(*opened.catalog, snapshot_for(config, session_key, prefix(tokens, 2), 1, { 0xc2 }));
    auto different_longer = snapshot_for(
        config, session_key, prefix(tokens, 6), 1, { 0xc6 });
    auto different_exact = exact_inputs(config, session_key, different_longer.tokens);
    different_exact.global_plan_digest[0] ^= 1;
    const auto different_authority =
        halofpx::context_store_resolve_exact_session_v1(different_exact);
    assert(different_authority.resolved());
    different_longer.compatibility_identity.checkpoint_lineage_id =
        different_authority.session_id;
    publish(*opened.catalog, different_longer);
    const std::array<size_t, 2> boundaries { 2, 6 };

    auto result = halofpx::context_store_v1_restore_longest_prefix(
        *opened.catalog,
        request_for(config, session_key, tokens, 1, boundaries.data(), boundaries.size()));
    assert(result.hit());
    assert(result.matched_token_count == 2);
    assert(result.snapshot.state == std::vector<uint8_t>({ 0xc2 }));
    assert(result.candidates_examined == 2);

    auto incompatible = request_for(
        config, session_key, tokens, 1, boundaries.data(), boundaries.size());
    incompatible.exact_session.compatibility_root[0] ^= 1;
    result = halofpx::context_store_v1_restore_longest_prefix(*opened.catalog, incompatible);
    assert(result.status == halofpx::context_store_v1_prefix_selector_status::miss_not_found);
    assert(result.matched_token_count == 0);
    assert(result.snapshot.tokens.empty() && result.snapshot.state.empty());

    auto wrong_topology = request_for(
        config, session_key, tokens, 1, boundaries.data(), boundaries.size());
    wrong_topology.exact_session.world_size = 2;
    result = halofpx::context_store_v1_restore_longest_prefix(*opened.catalog, wrong_topology);
    assert(result.status == halofpx::context_store_v1_prefix_selector_status::source_rejected);
    assert(result.fallback_reason ==
           halofpx::context_store_v1_prefix_fallback_reason::incompatible_topology);
    assert(result.candidates_examined == 0);
    assert(result.residual_token_offset == 0);
    assert(result.residual_token_count == tokens.size());

    auto unsupported = request_for(
        config, session_key, tokens, 1, boundaries.data(), boundaries.size());
    unsupported.profile.greedy_memoryless_sampling = false;
    result = halofpx::context_store_v1_restore_longest_prefix(*opened.catalog, unsupported);
    assert(result.status == halofpx::context_store_v1_prefix_selector_status::source_rejected);
    assert(result.fallback_reason ==
           halofpx::context_store_v1_prefix_fallback_reason::unsupported_profile);
    assert(result.residual_token_offset == 0);
    assert(result.residual_token_count == tokens.size());
}

void test_policy_epoch_is_exact_authority() {
    catalog_roots roots;
    std::array<uint8_t, 32> operator_key {}, session_key {};
    operator_key.fill(0x47);
    session_key.fill(0x77);
    auto config = make_config(roots, operator_key);
    auto opened = halofpx::make_context_store_v1_catalog(config);
    assert(opened.catalog != nullptr);

    const std::vector<llama_token> tokens { 167, 173, 179, 181 };
    publish(*opened.catalog, snapshot_for(config, session_key, tokens, 1, { 0xa1 }));
    const std::array<size_t, 1> boundary { tokens.size() };

    const auto result = halofpx::context_store_v1_restore_longest_prefix(
        *opened.catalog,
        request_for(config, session_key, tokens, 2, boundary.data(), boundary.size()));
    assert(result.status ==
           halofpx::context_store_v1_prefix_selector_status::miss_not_found);
    assert(result.fallback_reason ==
           halofpx::context_store_v1_prefix_fallback_reason::no_eligible_prefix);
    assert(result.candidates_examined == 1);
    assert(result.residual_token_count == tokens.size());
    assert(result.snapshot.tokens.empty() && result.snapshot.state.empty());
}

void test_selected_incompatible_longer_is_terminal() {
    catalog_roots roots;
    std::array<uint8_t, 32> operator_key {}, session_key {};
    operator_key.fill(0x46);
    session_key.fill(0x76);
    auto config = make_config(roots, operator_key);
    auto opened = halofpx::make_context_store_v1_catalog(config);
    assert(opened.catalog != nullptr);

    const std::vector<llama_token> tokens { 131, 137, 139, 149, 151, 157, 163 };
    publish(*opened.catalog, snapshot_for(config, session_key, prefix(tokens, 2), 1, { 0xe2 }));

    const auto exact_longer = snapshot_for(
        config, session_key, prefix(tokens, 6), 1, { 0xe6 });
    auto wrong_tokens = prefix(tokens, 6);
    wrong_tokens.back() += 1;
    auto incompatible = snapshot_for(config, session_key, wrong_tokens, 1, { 0xef });
    // Model an authenticated catalog record whose exact opaque authority is
    // selected but whose child token payload cannot satisfy that authority.
    incompatible.compatibility_identity = exact_longer.compatibility_identity;
    publish(*opened.catalog, incompatible);

    const std::array<size_t, 2> boundaries { 2, 6 };
    const auto result = halofpx::context_store_v1_restore_longest_prefix(
        *opened.catalog,
        request_for(config, session_key, tokens, 1, boundaries.data(), boundaries.size()));
    assert(result.status ==
           halofpx::context_store_v1_prefix_selector_status::miss_incompatible);
    assert(result.fallback_reason ==
           halofpx::context_store_v1_prefix_fallback_reason::authenticated_state_incompatible);
    assert(result.candidates_examined == 1);
    assert(result.matched_token_count == 0);
    assert(result.restored_token_count == 0);
    assert(result.residual_token_offset == 0);
    assert(result.residual_token_count == tokens.size());
    assert(result.snapshot.tokens.empty() && result.snapshot.state.empty());
}

void test_corruption_is_global_cold_miss_without_rewrite() {
    catalog_roots roots;
    std::array<uint8_t, 32> operator_key {}, session_key {};
    operator_key.fill(0x45);
    session_key.fill(0x75);
    auto config = make_config(roots, operator_key);
    auto opened = halofpx::make_context_store_v1_catalog(config);
    assert(opened.catalog != nullptr);

    const std::vector<llama_token> tokens { 97, 101, 103, 107, 109, 113, 127 };
    publish(*opened.catalog, snapshot_for(config, session_key, prefix(tokens, 2), 1, { 0xd2 }));
    publish(*opened.catalog, snapshot_for(config, session_key, prefix(tokens, 6), 1, { 0xd6 }));
    const auto object = first_object(roots.data[1]);
    assert(!object.empty());
    flip_first_byte(object);
    const auto before = inspect_tree(roots.base);

    const std::array<size_t, 2> boundaries { 2, 6 };
    const auto result = halofpx::context_store_v1_restore_longest_prefix(
        *opened.catalog,
        request_for(config, session_key, tokens, 1, boundaries.data(), boundaries.size()));
    assert(result.status == halofpx::context_store_v1_prefix_selector_status::miss_corrupt);
    assert(result.fallback_reason ==
           halofpx::context_store_v1_prefix_fallback_reason::authenticated_state_corrupt);
    assert(result.candidates_examined == 1);
    assert(result.matched_token_count == 0);
    assert(result.snapshot.tokens.empty() && result.snapshot.state.empty());
    assert(result.residual_token_offset == 0);
    assert(result.residual_token_count == tokens.size());
    assert(inspect_tree(roots.base) == before);
}

void test_truncated_child_is_global_cold_miss_without_rewrite() {
    catalog_roots roots;
    std::array<uint8_t, 32> operator_key {}, session_key {};
    operator_key.fill(0x47);
    session_key.fill(0x77);
    auto config = make_config(roots, operator_key);
    auto opened = halofpx::make_context_store_v1_catalog(config);
    assert(opened.catalog != nullptr);

    const std::vector<llama_token> tokens { 167, 173, 179, 181, 191, 193, 197 };
    publish(*opened.catalog, snapshot_for(config, session_key, prefix(tokens, 2), 1, { 0xf2 }));
    publish(*opened.catalog, snapshot_for(config, session_key, prefix(tokens, 6), 1, { 0xf6 }));
    const auto object = first_object(roots.data[1]);
    assert(!object.empty());
    truncate_last_byte(object);
    const auto before = inspect_tree(roots.base);

    const std::array<size_t, 2> boundaries { 2, 6 };
    const auto result = halofpx::context_store_v1_restore_longest_prefix(
        *opened.catalog,
        request_for(config, session_key, tokens, 1, boundaries.data(), boundaries.size()));
    assert(result.status == halofpx::context_store_v1_prefix_selector_status::miss_corrupt);
    assert(result.fallback_reason ==
           halofpx::context_store_v1_prefix_fallback_reason::authenticated_state_corrupt);
    assert(result.candidates_examined == 1);
    assert(result.matched_token_count == 0);
    assert(result.residual_token_offset == 0);
    assert(result.residual_token_count == tokens.size());
    assert(result.snapshot.tokens.empty() && result.snapshot.state.empty());
    assert(inspect_tree(roots.base) == before);
}

} // namespace

int main() {
    test_longest_multiple_prefixes_and_suffix_boundary();
    test_full_empty_and_exact_token_behavior();
    test_exact_tie_and_boundary_rejection();
    test_different_longer_authority_and_incompatible_authority_cannot_win();
    test_policy_epoch_is_exact_authority();
    test_selected_incompatible_longer_is_terminal();
    test_corruption_is_global_cold_miss_without_rewrite();
    test_truncated_child_is_global_cold_miss_without_rewrite();
    assert(std::string(halofpx::context_store_v1_prefix_selector_status_name(
               halofpx::context_store_v1_prefix_selector_status::hit)) == "hit");
    assert(std::string(halofpx::context_store_v1_prefix_fallback_reason_name(
               halofpx::context_store_v1_prefix_fallback_reason::no_eligible_prefix)) ==
           "no-eligible-prefix");
    return 0;
}
