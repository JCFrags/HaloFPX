#include "ggml.h"

#include <cstdio>
#include <cstdint>
#include <limits>

#define REQUIRE(condition) do { \
    if (!(condition)) { \
        std::fprintf(stderr, "requirement failed at line %d: %s\n", __LINE__, #condition); \
        return 1; \
    } \
} while (false)

int main() {
    ggml_init_params params {
        16 * ggml_tensor_overhead(),
        nullptr,
        true,
    };
    ggml_context * ctx = ggml_init(params);
    REQUIRE(ctx != nullptr);

    ggml_tensor * q8 = ggml_new_tensor_2d(ctx, GGML_TYPE_Q8_0, 1024, 4096);
    int64_t elements = 0;
    constexpr size_t occupied = 1128 * 1088;
    REQUIRE(ggml_checked_1d_elements_for_bytes(q8, 0, occupied, &elements));
    REQUIRE(elements == 1128 * 1024);
    ggml_tensor * q8_view = ggml_view_1d(ctx, q8, elements, 0);
    REQUIRE(q8_view != nullptr);
    REQUIRE(ggml_nbytes(q8_view) == occupied);

    REQUIRE(ggml_checked_1d_elements_for_bytes(q8, 34, 34, &elements));
    REQUIRE(elements == 32);
    REQUIRE(!ggml_checked_1d_elements_for_bytes(q8, 1, 34, &elements));
    REQUIRE(!ggml_checked_1d_elements_for_bytes(q8, 0, 33, &elements));
    REQUIRE(!ggml_checked_1d_elements_for_bytes(
        q8, ggml_nbytes(q8) - 34, 68, &elements));
    REQUIRE(!ggml_checked_1d_elements_for_bytes(
        q8, 0, std::numeric_limits<size_t>::max(), &elements));
    const size_t q8_nb1 = q8->nb[1];
    q8->nb[1] += 34;
    REQUIRE(!ggml_checked_1d_elements_for_bytes(q8, 0, occupied, &elements));
    q8->nb[1] = q8_nb1;

    ggml_tensor * f16 = ggml_new_tensor_1d(ctx, GGML_TYPE_F16, 64);
    REQUIRE(ggml_checked_1d_elements_for_bytes(f16, 4, 64, &elements));
    REQUIRE(elements == 32);
    ggml_tensor * f16_view = ggml_view_1d(ctx, f16, elements, 4);
    REQUIRE(f16_view != nullptr);
    REQUIRE(ggml_nbytes(f16_view) == 64);
    REQUIRE(!ggml_checked_1d_elements_for_bytes(f16, 1, 64, &elements));

    ggml_tensor * f32 = ggml_new_tensor_1d(ctx, GGML_TYPE_F32, 64);
    REQUIRE(ggml_checked_1d_elements_for_bytes(f32, 8, 128, &elements));
    REQUIRE(elements == 32);
    ggml_tensor * f32_view = ggml_view_1d(ctx, f32, elements, 8);
    REQUIRE(f32_view != nullptr);
    REQUIRE(ggml_nbytes(f32_view) == 128);
    REQUIRE(!ggml_checked_1d_elements_for_bytes(f32, 8, 126, &elements));
    REQUIRE(!ggml_checked_1d_elements_for_bytes(f32, 0, 0, &elements));

    ggml_free(ctx);
    return 0;
}
