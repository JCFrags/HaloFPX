#pragma once

#include <cstdint>
#include <limits>

struct halofpx_vk_q8_predequant_facts {
    bool feature_enabled = false;
    bool standard_q8_kv = false;
    bool original_coopmat1 = false;
    bool effective_f16_coopmat1 = false;
    bool dense_layout = false;
    bool dim0_block_contiguous = false;
    bool pipeline_available = false;
    uint64_t query_rows = 0;
    uint64_t k_elements = 0;
    uint64_t v_elements = 0;
    uint64_t k_source_bytes = 0;
    uint64_t v_source_bytes = 0;
    uint64_t mask_bytes = 0;
    uint64_t split_scratch_upper_bytes = 0;
    uint64_t min_storage_alignment = 1;
    uint64_t max_storage_range = 0;
};

struct halofpx_vk_q8_predequant_plan {
    bool admitted = false;
    uint64_t k_f16_bytes = 0;
    uint64_t v_f16_bytes = 0;
    uint64_t mask_offset = 0;
    uint64_t y_allocation_bytes = 0;
};

inline bool halofpx_vk_checked_mul_u64(uint64_t a, uint64_t b, uint64_t &out) {
    if (a != 0 && b > std::numeric_limits<uint64_t>::max() / a) {
        return false;
    }
    out = a * b;
    return true;
}

inline bool halofpx_vk_checked_add_u64(uint64_t a, uint64_t b, uint64_t &out) {
    if (b > std::numeric_limits<uint64_t>::max() - a) {
        return false;
    }
    out = a + b;
    return true;
}

inline bool halofpx_vk_checked_align_u64(uint64_t value, uint64_t alignment, uint64_t &out) {
    if (alignment == 0) {
        return false;
    }
    const uint64_t remainder = value % alignment;
    if (remainder == 0) {
        out = value;
        return true;
    }
    return halofpx_vk_checked_add_u64(value, alignment - remainder, out);
}

inline halofpx_vk_q8_predequant_plan halofpx_vk_select_q8_predequant(
        const halofpx_vk_q8_predequant_facts &facts) {
    halofpx_vk_q8_predequant_plan plan;
    if (!facts.feature_enabled || !facts.standard_q8_kv || !facts.original_coopmat1 ||
        !facts.effective_f16_coopmat1 || !facts.dense_layout || !facts.dim0_block_contiguous ||
        !facts.pipeline_available || facts.query_rows < 64 || facts.max_storage_range == 0 ||
        facts.k_elements == 0 || facts.v_elements == 0 ||
        facts.k_elements > std::numeric_limits<uint32_t>::max() ||
        facts.v_elements > std::numeric_limits<uint32_t>::max()) {
        return plan;
    }

    if (!halofpx_vk_checked_mul_u64(facts.k_elements, sizeof(uint16_t), plan.k_f16_bytes) ||
        !halofpx_vk_checked_mul_u64(facts.v_elements, sizeof(uint16_t), plan.v_f16_bytes) ||
        !halofpx_vk_checked_align_u64(plan.v_f16_bytes, facts.min_storage_alignment, plan.mask_offset)) {
        return {};
    }

    plan.y_allocation_bytes = plan.v_f16_bytes;
    if (facts.mask_bytes != 0) {
        if (!halofpx_vk_checked_add_u64(plan.mask_offset, facts.mask_bytes, plan.y_allocation_bytes)) {
            return {};
        }
    }

    if (facts.k_source_bytes > facts.max_storage_range ||
        facts.v_source_bytes > facts.max_storage_range ||
        plan.k_f16_bytes > facts.max_storage_range ||
        plan.v_f16_bytes > facts.max_storage_range ||
        facts.mask_bytes > facts.max_storage_range ||
        facts.split_scratch_upper_bytes > facts.max_storage_range ||
        plan.y_allocation_bytes > facts.max_storage_range) {
        return {};
    }

    plan.admitted = true;
    return plan;
}
