#include <hip/hip_runtime.h>

#include <cmath>
#include <cstdlib>
#include <iostream>
#include <vector>

#define HIP_CHECK(call) do { \
    hipError_t e = (call); \
    if (e != hipSuccess) { \
        std::cerr << "HIP error at " << __FILE__ << ':' << __LINE__ << ": " \
                  << hipGetErrorString(e) << " (" << static_cast<int>(e) << ")\n"; \
        std::exit(1); \
    } \
} while (0)

__global__ void vector_add(const float* a, const float* b, float* c, std::size_t n) {
    const std::size_t i = static_cast<std::size_t>(blockIdx.x) * blockDim.x + threadIdx.x;
    if (i < n) c[i] = a[i] + b[i];
}

int main() {
    int count = 0;
    HIP_CHECK(hipGetDeviceCount(&count));
    if (count < 1) {
        std::cerr << "No HIP devices detected\n";
        return 2;
    }

    HIP_CHECK(hipSetDevice(0));
    hipDeviceProp_t prop{};
    HIP_CHECK(hipGetDeviceProperties(&prop, 0));
    std::cout << "device.name=" << prop.name << "\n";
    std::cout << "device.gcnArchName=" << prop.gcnArchName << "\n";
    std::cout << "device.totalGlobalMem=" << prop.totalGlobalMem << "\n";

    constexpr std::size_t n = 1u << 20;
    const std::size_t bytes = n * sizeof(float);
    std::vector<float> h_a(n, 1.25f), h_b(n, 2.75f), h_c(n, 0.0f);
    float *d_a = nullptr, *d_b = nullptr, *d_c = nullptr;
    HIP_CHECK(hipMalloc(&d_a, bytes));
    HIP_CHECK(hipMalloc(&d_b, bytes));
    HIP_CHECK(hipMalloc(&d_c, bytes));
    HIP_CHECK(hipMemcpy(d_a, h_a.data(), bytes, hipMemcpyHostToDevice));
    HIP_CHECK(hipMemcpy(d_b, h_b.data(), bytes, hipMemcpyHostToDevice));

    constexpr unsigned block = 256;
    const unsigned grid = static_cast<unsigned>((n + block - 1) / block);
    hipLaunchKernelGGL(vector_add, dim3(grid), dim3(block), 0, 0, d_a, d_b, d_c, n);
    HIP_CHECK(hipGetLastError());
    HIP_CHECK(hipDeviceSynchronize());
    HIP_CHECK(hipMemcpy(h_c.data(), d_c, bytes, hipMemcpyDeviceToHost));

    std::size_t bad = 0;
    for (std::size_t i = 0; i < n; ++i) {
        if (std::fabs(h_c[i] - 4.0f) > 1e-6f) {
            if (++bad < 5) std::cerr << "Mismatch at " << i << ": " << h_c[i] << "\n";
        }
    }
    HIP_CHECK(hipFree(d_a));
    HIP_CHECK(hipFree(d_b));
    HIP_CHECK(hipFree(d_c));
    HIP_CHECK(hipDeviceReset());

    if (bad != 0) {
        std::cerr << "FAIL mismatches=" << bad << "\n";
        return 3;
    }
    std::cout << "PASS elements=" << n << " expected=4\n";
    return 0;
}
