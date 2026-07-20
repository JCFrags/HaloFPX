#include "halofpx-context-store-compatibility-v1.h"

#include <algorithm>
#include <array>
#undef NDEBUG
#include <cassert>
#include <cstring>

namespace {

using component = halofpx::context_store_compatibility_component_digest_v1;
using digest = halofpx::context_store_format_digest;

std::array<component, halofpx::context_store_compatibility_v1_component_count> fixture() {
    std::array<component, halofpx::context_store_compatibility_v1_component_count> result {};
    for (size_t index = 0; index < result.size(); ++index) {
        result[index].label = halofpx::context_store_compatibility_component_label_v1(index);
        result[index].label_size = std::strlen(result[index].label);
        for (size_t byte = 0; byte < result[index].digest.size(); ++byte) {
            result[index].digest[byte] = static_cast<uint8_t>(index + 1);
        }
    }
    return result;
}

bool zero(const digest & value) {
    return std::all_of(value.begin(), value.end(), [](uint8_t byte) { return byte == 0; });
}

} // namespace

int main() {
    using status = halofpx::context_store_compatibility_build_status_v1;

    auto components = fixture();
    const auto built = halofpx::context_store_build_compatibility_expectation_v1(
        components.data(), components.size());
    assert(built.status == status::built);
    assert(!zero(built.expectation.root));
    const digest golden_root {{
        0x25, 0xd5, 0x72, 0xe3, 0x3e, 0x6a, 0x11, 0x8a,
        0x2f, 0xd1, 0x78, 0x5c, 0xce, 0xa2, 0xa6, 0x41,
        0x64, 0xb1, 0xd3, 0x71, 0x22, 0x3b, 0x62, 0xe7,
        0xc7, 0xfa, 0x87, 0x08, 0x8e, 0xe3, 0x58, 0x51,
    }};
    assert(built.expectation.root == golden_root);
    for (size_t index = 0; index < components.size(); ++index) {
        assert(built.expectation.components[index] == components[index].digest);
    }

    auto reversed = components;
    std::reverse(reversed.begin(), reversed.end());
    const auto reordered = halofpx::context_store_build_compatibility_expectation_v1(
        reversed.data(), reversed.size());
    assert(reordered.status == status::misordered_component);
    assert(zero(reordered.expectation.root));

    for (size_t index = 0; index < components.size(); ++index) {
        auto mutated = components;
        mutated[index].digest[index] ^= 0x80;
        const auto changed = halofpx::context_store_build_compatibility_expectation_v1(
            mutated.data(), mutated.size());
        assert(changed.status == status::built);
        assert(changed.expectation.root != built.expectation.root);
    }

    assert(halofpx::context_store_build_compatibility_expectation_v1(
        nullptr, components.size()).status == status::invalid_input);
    assert(halofpx::context_store_build_compatibility_expectation_v1(
        components.data(), components.size() - 1).status == status::wrong_component_count);

    auto duplicate = components;
    duplicate.back().label = duplicate.front().label;
    duplicate.back().label_size = duplicate.front().label_size;
    auto rejected = halofpx::context_store_build_compatibility_expectation_v1(
        duplicate.data(), duplicate.size());
    assert(rejected.status == status::duplicate_component);
    assert(zero(rejected.expectation.root));

    auto unknown = components;
    unknown[3].label = "model_metadata_extra";
    unknown[3].label_size = std::strlen(unknown[3].label);
    rejected = halofpx::context_store_build_compatibility_expectation_v1(
        unknown.data(), unknown.size());
    assert(rejected.status == status::unknown_component);
    assert(zero(rejected.expectation.root));

    auto empty = components;
    empty[9].digest.fill(0);
    rejected = halofpx::context_store_build_compatibility_expectation_v1(
        empty.data(), empty.size());
    assert(rejected.status == status::zero_component_digest);
    assert(zero(rejected.expectation.root));

    assert(halofpx::context_store_compatibility_component_label_v1(components.size()) == nullptr);
    assert(std::strcmp(halofpx::context_store_compatibility_build_status_name_v1(status::built), "built") == 0);
    return 0;
}
