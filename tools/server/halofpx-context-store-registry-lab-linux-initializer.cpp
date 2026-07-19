#include "halofpx-context-store-registry-lab-linux-initializer-internal.h"

namespace halofpx {

bool context_store_registry_lab_linux_initializer_predecessor_digest_v1(
        const uint8_t * data, size_t size, context_store_format_digest & output) noexcept {
    return context_store_registry_lab_registry_envelope_digest_v1(data, size, output);
}

} // namespace halofpx
