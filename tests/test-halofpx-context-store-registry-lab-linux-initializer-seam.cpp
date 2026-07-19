#ifdef NDEBUG
#undef NDEBUG
#endif

#include "halofpx-context-store-registry-lab-linux-initializer-internal.h"

#include <algorithm>
#include <array>
#include <cassert>
#include <cstdint>
#include <fstream>
#include <iterator>
#include <string>
#include <vector>

namespace {

uint8_t nibble(char c) {
    return uint8_t(c <= '9' ? c - '0' : c - 'a' + 10);
}

std::vector<uint8_t> hex_bytes(const std::string & text) {
    assert(text.size() % 2 == 0);
    std::vector<uint8_t> bytes(text.size() / 2);
    for (size_t i = 0; i < bytes.size(); ++i) {
        bytes[i] = uint8_t(nibble(text[2 * i]) << 4 | nibble(text[2 * i + 1]));
    }
    return bytes;
}

std::string value(const std::string & json, const std::string & key, size_t from = 0) {
    auto position = json.find("\"" + key + "\"", from);
    assert(position != std::string::npos);
    position = json.find(':', position) + 1;
    position = json.find('"', position) + 1;
    const auto end = json.find('"', position);
    return json.substr(position, end - position);
}

} // namespace

int main(int argc, char ** argv) {
    assert(argc == 2);
    std::ifstream input(argv[1], std::ios::binary);
    const std::string json((std::istreambuf_iterator<char>(input)), {});
    const auto predecessor = hex_bytes(value(json, "predecessor_registry_envelope_hex"));
    const auto expected_bytes = hex_bytes(value(json, "predecessor_registry_envelope_digest_hex"));
    assert(expected_bytes.size() == 32);

    halofpx::context_store_format_digest expected {};
    std::copy(expected_bytes.begin(), expected_bytes.end(), expected.begin());
    halofpx::context_store_format_digest actual {};
    assert(halofpx::context_store_registry_lab_linux_initializer_predecessor_digest_v1(
        predecessor.data(), predecessor.size(), actual));
    assert(actual == expected);

    actual.fill(0xa5);
    const auto unchanged = actual;
    assert(!halofpx::context_store_registry_lab_linux_initializer_predecessor_digest_v1(
        nullptr, predecessor.size(), actual));
    assert(actual == unchanged);
    assert(!halofpx::context_store_registry_lab_linux_initializer_predecessor_digest_v1(
        predecessor.data(), 0, actual));
    assert(actual == unchanged);
    std::array<uint8_t, 1025> oversized {};
    assert(!halofpx::context_store_registry_lab_linux_initializer_predecessor_digest_v1(
        oversized.data(), oversized.size(), actual));
    assert(actual == unchanged);
}
