#ifdef NDEBUG
#undef NDEBUG
#endif
#include <cassert>

#include "halofpx-context-store-object.h"
#include "halofpx-context-store-signed-fixture.h"

#include <algorithm>
#include <atomic>
#include <cstdint>
#include <string>
#include <thread>
#include <vector>

namespace {

using bytes = std::vector<uint8_t>;

void append_u16(bytes & output, uint16_t value) {
    output.push_back(static_cast<uint8_t>(value >> 8));
    output.push_back(static_cast<uint8_t>(value));
}

void append_u64(bytes & output, uint64_t value) {
    for (int shift = 56; shift >= 0; shift -= 8) output.push_back(static_cast<uint8_t>(value >> shift));
}

void set_u16(bytes & output, size_t offset, uint16_t value) {
    output[offset] = static_cast<uint8_t>(value >> 8);
    output[offset + 1] = static_cast<uint8_t>(value);
}

void set_u64(bytes & output, size_t offset, uint64_t value) {
    for (size_t index = 0; index < 8; ++index) {
        output[offset + index] = static_cast<uint8_t>(value >> (56 - index * 8));
    }
}

bytes make_frame(const bytes & type = { 'x' }, const bytes & payload = { 0 }) {
    bytes frame = { 0x48,0x41,0x4c,0x4f,0x4f,0x42,0x4a,0x01 };
    const std::string domain = "halofpx.object.v1";
    append_u16(frame, static_cast<uint16_t>(domain.size()));
    frame.insert(frame.end(), domain.begin(), domain.end());
    append_u16(frame, static_cast<uint16_t>(type.size()));
    frame.insert(frame.end(), type.begin(), type.end());
    append_u64(frame, payload.size());
    frame.insert(frame.end(), payload.begin(), payload.end());
    return frame;
}

halofpx::context_store_format_digest hash_frame(const bytes & frame) {
    halofpx::context_store_format_digest digest {};
    assert(halofpx::context_store_sha256_bounded(frame.data(), frame.size(), 4096, digest));
    return digest;
}

std::string hex(const halofpx::context_store_format_digest & value) {
    static constexpr char alphabet[] = "0123456789abcdef";
    std::string output(value.size() * 2, '0');
    for (size_t index = 0; index < value.size(); ++index) {
        output[index * 2] = alphabet[value[index] >> 4];
        output[index * 2 + 1] = alphabet[value[index] & 0x0f];
    }
    return output;
}

halofpx::context_store_manifest_verify_result authenticated_manifest(
        const bytes & frame,
        const bytes & type = { 'x' }) {
    auto signed_manifest = halofpx_test_fixture::make_signed_object_manifest(
        hash_frame(frame), type, frame.size());
    auto result = signed_manifest.verify();
    assert(result.status == halofpx::context_store_manifest_verify_status::authenticated_unadmitted);
    return result;
}

halofpx::context_store_manifest_verify_result authenticated_manifest_with_reference(
        const halofpx::context_store_format_digest & object_id,
        const bytes & type,
        uint64_t frame_bytes) {
    auto signed_manifest = halofpx_test_fixture::make_signed_object_manifest(
        object_id, type, frame_bytes);
    auto result = signed_manifest.verify();
    assert(result.status == halofpx::context_store_manifest_verify_status::authenticated_unadmitted);
    return result;
}

halofpx::context_store_object_verify_result verify(
        const bytes & frame,
        const halofpx::context_store_manifest_verify_result & manifest,
        halofpx::context_store_object_limits limits = { 4096, 1024 }) {
    return halofpx::context_store_verify_object_frame_v1(
        frame.data(), frame.size(), limits, manifest, 0);
}

void expect_failure_has_no_payload(const halofpx::context_store_object_verify_result & result) {
    assert(result.status != halofpx::context_store_object_verify_status::object_verified_unadmitted);
    assert(result.payload_offset == 0);
    assert(result.payload_size == 0);
}

void test_golden_and_boundaries() {
    auto frame = make_frame();
    auto manifest = authenticated_manifest(frame);
    auto result = verify(frame, manifest);
    assert(result.status == halofpx::context_store_object_verify_status::object_verified_unadmitted);
    assert(frame.size() == 39);
    assert(result.payload_offset == 38 && result.payload_size == 1);
    assert(hex(result.computed_object_id) == "d3ace5f0a24e7078cfb0a11987d4d0b31567317dd52125734b0500f8b5fb3f45");

    bytes max_type(128, 0x7f);
    bytes opaque_payload = { 0x00, 0x7f, 0x80, 0xff };
    auto maximum = make_frame(max_type, opaque_payload);
    auto maximum_manifest = authenticated_manifest(maximum, max_type);
    assert(verify(maximum, maximum_manifest, { maximum.size(), opaque_payload.size() }).status ==
        halofpx::context_store_object_verify_status::object_verified_unadmitted);

    auto empty_frame = make_frame({ 'x' }, {});
    auto empty_result = verify(empty_frame, authenticated_manifest(empty_frame));
    assert(empty_frame.size() == 38);
    assert(empty_result.status == halofpx::context_store_object_verify_status::object_verified_unadmitted);
    assert(empty_result.payload_offset == 38 && empty_result.payload_size == 0);
    assert(hex(empty_result.computed_object_id) == "b6915e1a81c18913dbd854bc548fbfd984b487dc2b6610dc5322033e4a464a11");

    for (uint8_t control : { uint8_t(0x01), uint8_t(0x7f) }) {
        bytes type = { control };
        auto control_frame = make_frame(type, { 1 });
        auto control_manifest = authenticated_manifest(control_frame, type);
        assert(verify(control_frame, control_manifest).status ==
            halofpx::context_store_object_verify_status::object_verified_unadmitted);
    }
}

void test_manifest_and_limit_rejection() {
    auto frame = make_frame();
    auto manifest = authenticated_manifest(frame);

    auto unadmitted = manifest;
    unadmitted.status = halofpx::context_store_manifest_verify_status::authentication_failed;
    expect_failure_has_no_payload(verify(frame, unadmitted));
    halofpx::context_store_manifest_verify_result fabricated;
    fabricated.status = halofpx::context_store_manifest_verify_status::authenticated_unadmitted;
    expect_failure_has_no_payload(verify(frame, fabricated));
    expect_failure_has_no_payload(halofpx::context_store_verify_object_frame_v1(
        frame.data(), frame.size(), { 4096, 1024 }, manifest, 1));

    expect_failure_has_no_payload(verify(frame, manifest, { 0, 1 }));
    expect_failure_has_no_payload(verify(frame, manifest, { 39, 0 }));
    expect_failure_has_no_payload(verify(frame, manifest, { 38, 1 }));
    expect_failure_has_no_payload(verify(frame, manifest, { UINT64_MAX, 1 }));

    auto too_large = frame;
    too_large.push_back(0);
    expect_failure_has_no_payload(verify(too_large, authenticated_manifest(too_large), { 39, 2 }));
    auto payload_over_cap = frame;
    expect_failure_has_no_payload(verify(payload_over_cap, manifest, { 4096, 0 }));

    const auto null_result = halofpx::context_store_verify_object_frame_v1(
        nullptr, frame.size(), { 4096, 1024 }, manifest, 0);
    expect_failure_has_no_payload(null_result);
}

void test_header_and_type_rejection() {
    const auto valid = make_frame();
    for (size_t index = 0; index < 8; ++index) {
        auto frame = valid;
        frame[index] ^= 0x01;
        expect_failure_has_no_payload(verify(frame, authenticated_manifest(frame)));
    }
    for (uint16_t length : { uint16_t(0), uint16_t(16), uint16_t(18), uint16_t(0x1100), uint16_t(0xffff) }) {
        auto frame = valid;
        set_u16(frame, 8, length);
        expect_failure_has_no_payload(verify(frame, authenticated_manifest(frame)));
    }
    for (size_t index = 10; index < 27; ++index) {
        auto frame = valid;
        frame[index] ^= 0x01;
        expect_failure_has_no_payload(verify(frame, authenticated_manifest(frame)));
    }
    for (const bytes & type : { bytes{}, bytes(129, 'a'), bytes{ 0 }, bytes{ 0x80 }, bytes{ 0xff }, bytes{ 0xc3, 0xb8 } }) {
        auto frame = make_frame(type, { 1 });
        auto manifest = authenticated_manifest_with_reference(
            hash_frame(frame), { 'x' }, frame.size());
        expect_failure_has_no_payload(verify(frame, manifest));
    }
    auto wrong_type = authenticated_manifest_with_reference(
        hash_frame(valid), { 'X' }, valid.size());
    expect_failure_has_no_payload(verify(valid, wrong_type));
}

void test_payload_framing_and_digest_rejection() {
    const auto valid = make_frame({ 'x' }, { 1, 2, 3 });
    const size_t payload_length_offset = 30;

    auto zero_with_trailing = valid;
    set_u64(zero_with_trailing, payload_length_offset, 0);
    expect_failure_has_no_payload(verify(zero_with_trailing, authenticated_manifest(zero_with_trailing)));
    auto too_long = valid;
    set_u64(too_long, payload_length_offset, 4);
    expect_failure_has_no_payload(verify(too_long, authenticated_manifest(too_long)));
    auto too_short = valid;
    set_u64(too_short, payload_length_offset, 2);
    expect_failure_has_no_payload(verify(too_short, authenticated_manifest(too_short)));
    auto huge = valid;
    set_u64(huge, payload_length_offset, UINT64_MAX);
    expect_failure_has_no_payload(verify(huge, authenticated_manifest(huge)));
    auto over_cap = valid;
    expect_failure_has_no_payload(verify(over_cap, authenticated_manifest(over_cap), { 4096, 2 }));

    for (size_t length = 0; length < valid.size(); ++length) {
        bytes truncated(valid.begin(), valid.begin() + length);
        expect_failure_has_no_payload(verify(truncated, authenticated_manifest(valid)));
    }
    auto appended = valid;
    appended.push_back(4);
    expect_failure_has_no_payload(verify(appended, authenticated_manifest(valid)));

    auto length_mismatch = authenticated_manifest_with_reference(
        hash_frame(valid), { 'x' }, valid.size() + 1);
    expect_failure_has_no_payload(verify(valid, length_mismatch));
    for (size_t index : { size_t(0), size_t(16), size_t(31) }) {
        auto wrong_digest = hash_frame(valid);
        wrong_digest[index] ^= 0x01;
        auto digest_mismatch = authenticated_manifest_with_reference(
            wrong_digest, { 'x' }, valid.size());
        expect_failure_has_no_payload(verify(valid, digest_mismatch));
    }
    auto changed = valid;
    changed.back() ^= 0x01;
    expect_failure_has_no_payload(verify(changed, authenticated_manifest(valid)));
    assert(verify(changed, authenticated_manifest(changed)).status ==
        halofpx::context_store_object_verify_status::object_verified_unadmitted);
}

void test_immutable_concurrent_input() {
    const auto frame = make_frame({ 't','o','k','e','n','s' }, bytes(256, 0xa5));
    const auto original = frame;
    const auto manifest = authenticated_manifest(frame, { 't','o','k','e','n','s' });
    std::atomic<bool> failed { false };
    std::vector<std::thread> threads;
    for (size_t thread_index = 0; thread_index < 8; ++thread_index) {
        threads.emplace_back([&]() {
            for (size_t iteration = 0; iteration < 100; ++iteration) {
                if (verify(frame, manifest).status !=
                    halofpx::context_store_object_verify_status::object_verified_unadmitted) {
                    failed.store(true);
                }
            }
        });
    }
    for (auto & thread : threads) thread.join();
    assert(!failed.load());
    assert(frame == original);
}

} // namespace

int main() {
    test_golden_and_boundaries();
    test_manifest_and_limit_rejection();
    test_header_and_type_rejection();
    test_payload_framing_and_digest_rejection();
    test_immutable_concurrent_input();
    return 0;
}
