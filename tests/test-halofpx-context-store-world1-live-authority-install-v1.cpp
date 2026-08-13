#include "halofpx-context-store-world1-live-authority-install-v1.h"
#include "halofpx-context-store-compatibility-v1.h"

#include <array>
#include <cstdlib>
#include <cstring>

namespace {

using halofpx::context_store_world1_live_authority_install_status_v1;
using halofpx::context_store_world1_live_authority_snapshot_v1;
using halofpx::context_store_world1_live_authority_source_kind_v1;

void require(bool condition) {
    if (!condition) std::abort();
}

halofpx::context_store_format_digest digest(uint8_t seed) {
    halofpx::context_store_format_digest value {};
    for (size_t index = 0; index < value.size(); ++index) {
        value[index] = static_cast<uint8_t>(seed + index);
    }
    return value;
}

halofpx::context_store_world1_cache_authority_v1 valid_authority(
        uint64_t generation = 17) {
    halofpx::context_store_world1_cache_authority_v1 authority;
    std::array<halofpx::context_store_compatibility_component_digest_v1,
               halofpx::context_store_compatibility_v1_component_count> components {};
    for (size_t index = 0; index < components.size(); ++index) {
        components[index].label =
            halofpx::context_store_compatibility_component_label_v1(index);
        components[index].label_size = std::strlen(components[index].label);
        components[index].digest = digest(static_cast<uint8_t>(index + 1));
    }
    const auto built = halofpx::context_store_build_compatibility_expectation_v1(
        components.data(), components.size());
    require(built.status ==
        halofpx::context_store_compatibility_build_status_v1::built);
    authority.compatibility = built.expectation;
    authority.producer_identity = digest(40);
    authority.global_plan_digest = digest(80);
    authority.rank_ownership_digest = digest(120);
    authority.rank_placement_digest = digest(160);
    authority.topology_epoch = 9;
    authority.model_generation = generation;
    authority.world_size = 1;
    authority.rank = 0;
    require(halofpx::context_store_world1_cache_authority_v1_is_valid(authority));
    return authority;
}

class fake_source final :
        public halofpx::context_store_world1_live_authority_source_v1 {
public:
    context_store_world1_live_authority_snapshot_v1 snapshot;
    mutable size_t calls = 0;

    context_store_world1_live_authority_snapshot_v1 capture() const noexcept override {
        ++calls;
        return snapshot;
    }
};

fake_source complete_source(uint64_t generation = 17) {
    fake_source source;
    source.snapshot.source_kind = context_store_world1_live_authority_source_kind_v1::
        trusted_live_loader_context_lifecycle;
    source.snapshot.captured_facts =
        halofpx::context_store_world1_live_authority_required_facts_v1;
    source.snapshot.authority = valid_authority(generation);
    return source;
}

halofpx::context_store_world1_live_authority_install_result_v1 install(
        bool enabled,
        const halofpx::context_store_world1_live_authority_source_v1 * source,
        uint64_t generation) {
    return halofpx::context_store_install_world1_live_authority_v1({
        enabled, source, generation,
    });
}

void expect_refused(
        const halofpx::context_store_world1_live_authority_install_result_v1 & result,
        context_store_world1_live_authority_install_status_v1 status) {
    require(result.status == status);
    require(!result.installed());
    require(!result.authority);
}

void test_feature_off_and_missing_source_are_cold() {
    auto source = complete_source();
    expect_refused(install(false, &source, 17),
        context_store_world1_live_authority_install_status_v1::feature_off);
    require(source.calls == 0);

    expect_refused(install(true, nullptr, 17),
        context_store_world1_live_authority_install_status_v1::source_unavailable);
}

void test_generation_must_be_nonzero_and_match() {
    auto source = complete_source();
    expect_refused(install(true, &source, 0),
        context_store_world1_live_authority_install_status_v1::
            model_generation_unavailable);
    require(source.calls == 0);

    expect_refused(install(true, &source, 18),
        context_store_world1_live_authority_install_status_v1::
            model_generation_changed);
    require(source.calls == 1);
}

void test_partial_facts_never_install() {
    for (uint8_t fact = 0;
         fact < static_cast<uint8_t>(
             halofpx::context_store_world1_live_authority_fact_v1::count);
         ++fact) {
        auto source = complete_source();
        source.snapshot.captured_facts &= ~(uint64_t {1} << fact);
        expect_refused(install(true, &source, 17),
            context_store_world1_live_authority_install_status_v1::
                incomplete_fact_custody);
        require(source.calls == 1);
    }

    auto unknown_fact = complete_source();
    unknown_fact.snapshot.captured_facts |= uint64_t {1} << 63;
    expect_refused(install(true, &unknown_fact, 17),
        context_store_world1_live_authority_install_status_v1::
            incomplete_fact_custody);
}

void test_operator_and_world2_sources_never_install() {
    auto operator_source = complete_source();
    operator_source.snapshot.source_kind =
        context_store_world1_live_authority_source_kind_v1::operator_components;
    expect_refused(install(true, &operator_source, 17),
        context_store_world1_live_authority_install_status_v1::untrusted_source);

    auto standalone_world2 = complete_source();
    standalone_world2.snapshot.source_kind =
        context_store_world1_live_authority_source_kind_v1::
            standalone_world2_authority;
    standalone_world2.snapshot.authority.world_size = 2;
    expect_refused(install(true, &standalone_world2, 17),
        context_store_world1_live_authority_install_status_v1::untrusted_source);

    auto disguised_world2 = complete_source();
    disguised_world2.snapshot.authority.world_size = 2;
    expect_refused(install(true, &disguised_world2, 17),
        context_store_world1_live_authority_install_status_v1::invalid_authority);
}

void test_structurally_invalid_capability_never_installs() {
    auto source = complete_source();
    source.snapshot.authority.compatibility.components[3].fill(0);
    expect_refused(install(true, &source, 17),
        context_store_world1_live_authority_install_status_v1::invalid_authority);

    auto mismatched_root = complete_source();
    mismatched_root.snapshot.authority.compatibility.root[0] ^= 0x80;
    expect_refused(install(true, &mismatched_root, 17),
        context_store_world1_live_authority_install_status_v1::invalid_authority);

    for (size_t left = 0; left < 4; ++left) {
        for (size_t right = left + 1; right < 4; ++right) {
            auto duplicate = complete_source();
            std::array<halofpx::context_store_format_digest *, 4> identities = {
                &duplicate.snapshot.authority.producer_identity,
                &duplicate.snapshot.authority.global_plan_digest,
                &duplicate.snapshot.authority.rank_ownership_digest,
                &duplicate.snapshot.authority.rank_placement_digest,
            };
            *identities[right] = *identities[left];
            expect_refused(install(true, &duplicate, 17),
                context_store_world1_live_authority_install_status_v1::
                    invalid_authority);
        }
    }
}

void test_complete_trusted_fixture_is_owned() {
    auto source = complete_source();
    const auto expected = source.snapshot.authority;
    auto result = install(true, &source, 17);
    require(result.installed());
    require(static_cast<bool>(result.authority));
    require(halofpx::context_store_world1_cache_authority_v1_matches(
        *result.authority, expected));
    source.snapshot.authority = {};
    require(halofpx::context_store_world1_cache_authority_v1_matches(
        *result.authority, expected));
}

} // namespace

int main() {
    test_feature_off_and_missing_source_are_cold();
    test_generation_must_be_nonzero_and_match();
    test_partial_facts_never_install();
    test_operator_and_world2_sources_never_install();
    test_structurally_invalid_capability_never_installs();
    test_complete_trusted_fixture_is_owned();
    require(std::strcmp(
        halofpx::context_store_world1_live_authority_install_status_name_v1(
            context_store_world1_live_authority_install_status_v1::source_unavailable),
        "source-unavailable") == 0);
    return 0;
}
