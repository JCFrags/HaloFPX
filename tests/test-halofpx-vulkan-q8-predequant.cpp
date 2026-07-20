#include "halofpx-vulkan-q8-predequant.h"

#include <cstdint>
#include <cstdlib>
#include <limits>

static void require(bool condition) {
    if (!condition) {
        std::abort();
    }
}

static halofpx_vk_q8_predequant_facts admitted_facts() {
    halofpx_vk_q8_predequant_facts facts;
    facts.feature_enabled = true;
    facts.standard_q8_kv = true;
    facts.original_coopmat1 = true;
    facts.effective_f16_coopmat1 = true;
    facts.dense_layout = true;
    facts.dim0_block_contiguous = true;
    facts.pipeline_available = true;
    facts.query_rows = 64;
    facts.k_elements = 4096;
    facts.v_elements = 4096;
    facts.k_source_bytes = 4352;
    facts.v_source_bytes = 4352;
    facts.mask_bytes = 1024;
    facts.split_scratch_upper_bytes = 4096;
    facts.min_storage_alignment = 256;
    facts.max_storage_range = 16384;
    return facts;
}

int main() {
    const auto baseline = halofpx_vk_select_q8_predequant(admitted_facts());
    require(baseline.admitted);
    require(baseline.k_f16_bytes == 8192);
    require(baseline.v_f16_bytes == 8192);
    require(baseline.mask_offset == 8192);
    require(baseline.y_allocation_bytes == 9216);

    auto facts = admitted_facts();
    facts.query_rows = 63;
    require(!halofpx_vk_select_q8_predequant(facts).admitted);
    facts.query_rows = 65;
    require(halofpx_vk_select_q8_predequant(facts).admitted);

    facts = admitted_facts();
    facts.original_coopmat1 = false;
    require(!halofpx_vk_select_q8_predequant(facts).admitted);
    facts = admitted_facts();
    facts.effective_f16_coopmat1 = false;
    require(!halofpx_vk_select_q8_predequant(facts).admitted);
    facts = admitted_facts();
    facts.dense_layout = false;
    require(!halofpx_vk_select_q8_predequant(facts).admitted);
    facts = admitted_facts();
    facts.standard_q8_kv = false;
    require(!halofpx_vk_select_q8_predequant(facts).admitted);

    facts = admitted_facts();
    facts.max_storage_range = baseline.y_allocation_bytes;
    require(halofpx_vk_select_q8_predequant(facts).admitted);
    facts.max_storage_range = baseline.y_allocation_bytes - 1;
    require(!halofpx_vk_select_q8_predequant(facts).admitted);

    facts = admitted_facts();
    facts.split_scratch_upper_bytes = facts.max_storage_range + 1;
    require(!halofpx_vk_select_q8_predequant(facts).admitted);

    facts = admitted_facts();
    facts.k_elements = (uint64_t)std::numeric_limits<uint32_t>::max() + 1;
    require(!halofpx_vk_select_q8_predequant(facts).admitted);

    uint64_t out = 0;
    require(!halofpx_vk_checked_mul_u64(std::numeric_limits<uint64_t>::max(), 2, out));
    require(!halofpx_vk_checked_add_u64(std::numeric_limits<uint64_t>::max(), 1, out));
    require(!halofpx_vk_checked_align_u64(1, 0, out));
    require(!halofpx_vk_checked_align_u64(std::numeric_limits<uint64_t>::max(), 256, out));
    require(halofpx_vk_checked_align_u64(8192, 256, out) && out == 8192);

    return 0;
}
