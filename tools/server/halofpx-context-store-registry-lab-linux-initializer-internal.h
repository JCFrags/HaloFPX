#pragma once

#include "halofpx-context-store-registry-lab-wire.h"

namespace halofpx {

// Internal L05t linkage seam only. It performs no filesystem access or mutation.
bool context_store_registry_lab_linux_initializer_predecessor_digest_v1(
    const uint8_t * data, size_t size, context_store_format_digest & output) noexcept;

} // namespace halofpx
