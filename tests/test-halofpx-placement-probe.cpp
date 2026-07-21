#include "arg.h"
#include "common.h"
#include "ggml-backend.h"
#include "llama-model-placement.h"

#include <algorithm>
#include <array>
#include <cctype>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

struct device_record {
    std::string name;
    std::string backend;
    std::string description;
    size_t free = 0;
    size_t total = 0;
};

struct contract_input {
    bool device_option_present = false;
    llama_split_mode split_mode = LLAMA_SPLIT_MODE_NONE;
    std::array<float, 2> tensor_split { 0.0f, 0.0f };
    std::vector<device_record> devices;
    std::vector<int> ownership;
    std::string expected_endpoint;
    int n_layer = 0;
};

struct contract_result {
    int repeating_rpc = 0;
    int repeating_rocm = 0;
    int total_rpc = 0;
    int total_rocm = 0;
};

bool contract_validate(const contract_input & input, contract_result & result, std::string & reason) {
    if (!input.device_option_present) { reason = "device-option-omitted"; return false; }
    if (input.devices.size() != 2) { reason = "selected-device-count"; return false; }
    if (input.devices[0].name != "RPC0" || input.devices[1].name != "ROCm0") {
        reason = "selected-device-order"; return false;
    }
    if (input.devices[0].backend != "RPC" || input.devices[1].backend != "ROCm") {
        reason = "selected-device-backend"; return false;
    }
    if (input.devices[0].description != input.expected_endpoint) {
        reason = "rpc-endpoint-identity"; return false;
    }
    for (const auto & device : input.devices) {
        if (device.free == 0 || device.total == 0 || device.free > device.total) {
            reason = "device-memory"; return false;
        }
    }
    if (input.split_mode != LLAMA_SPLIT_MODE_LAYER) { reason = "split-mode"; return false; }
    if (input.tensor_split[0] != 1.0f || input.tensor_split[1] != 1.0f) {
        reason = "tensor-split"; return false;
    }
    if (input.n_layer != 62 || input.ownership.size() != 63) {
        reason = "layer-count"; return false;
    }
    for (int il = 0; il < input.n_layer; ++il) {
        if (input.ownership[il] == 0) ++result.repeating_rpc;
        else if (input.ownership[il] == 1) ++result.repeating_rocm;
        else { reason = "one-device-resolution"; return false; }
    }
    if (input.ownership.back() == 0) ++result.total_rpc;
    else if (input.ownership.back() == 1) ++result.total_rocm;
    else { reason = "output-resolution"; return false; }
    result.total_rpc += result.repeating_rpc;
    result.total_rocm += result.repeating_rocm;
    if (result.repeating_rpc == 0 || result.repeating_rocm == 0 ||
        result.total_rpc == 0 || result.total_rocm == 0) {
        reason = "monolithic-ownership"; return false;
    }
    if (result.repeating_rpc != 32 || result.repeating_rocm != 30 ||
        input.ownership.back() != 1 || result.total_rpc != 32 || result.total_rocm != 31) {
        reason = "unexpected-ownership"; return false;
    }
    return true;
}

contract_input valid_fixture() {
    contract_input input;
    input.device_option_present = true;
    input.split_mode = LLAMA_SPLIT_MODE_LAYER;
    input.tensor_split = { 1.0f, 1.0f };
    input.devices = {
        { "RPC0", "RPC", "10.44.0.1:50190", 1, 2 },
        { "ROCm0", "ROCm", "fixture", 1, 2 },
    };
    input.expected_endpoint = "10.44.0.1:50190";
    input.n_layer = 62;
    input.ownership = llama_model_resolve_layer_devices(62, 999, { 0.5f, 1.0f });
    return input;
}

bool expect_result(const contract_input & input, bool expected, const char * label) {
    contract_result result;
    std::string reason;
    const bool actual = contract_validate(input, result, reason);
    if (actual != expected) {
        std::fprintf(stderr, "self-test %s: expected %s, got %s (%s)\n", label,
            expected ? "pass" : "refusal", actual ? "pass" : "refusal", reason.c_str());
        return false;
    }
    return true;
}

int self_test() {
    bool ok = expect_result(valid_fixture(), true, "correct");
    auto omitted = valid_fixture(); omitted.device_option_present = false;
    ok = expect_result(omitted, false, "omitted") && ok;
    auto reversed = valid_fixture(); std::swap(reversed.devices[0], reversed.devices[1]);
    ok = expect_result(reversed, false, "wrong-order") && ok;
    auto one = valid_fixture(); one.devices.pop_back();
    ok = expect_result(one, false, "one-device") && ok;
    auto all_rpc = valid_fixture(); std::fill(all_rpc.ownership.begin(), all_rpc.ownership.end(), 0);
    ok = expect_result(all_rpc, false, "all-rpc") && ok;
    if (ok) std::puts("halofpx-placement-self-test: PASS (correct omitted wrong-order one-device all-rpc)");
    return ok ? 0 : 1;
}

bool safe_endpoint(const std::string & value) {
    return !value.empty() && value.size() <= 128 && std::all_of(value.begin(), value.end(), [](unsigned char c) {
        return std::isalnum(c) || c == '.' || c == ':' || c == '-';
    });
}

} // namespace

int main(int argc, char ** argv) {
    if (argc == 2 && std::strcmp(argv[1], "--self-test") == 0) return self_test();

    std::string expected_endpoint;
    bool device_option_present = false;
    std::vector<std::string> filtered { argv[0] };
    for (int i = 1; i < argc; ++i) {
        const std::string arg = argv[i];
        if (arg == "--hfx-expected-rpc-endpoint") {
            if (++i >= argc) { std::fprintf(stderr, "placement refusal: expected-endpoint-argument\n"); return 2; }
            expected_endpoint = argv[i];
            continue;
        }
        if (arg == "--device" || arg == "-dev" || arg.rfind("--device=", 0) == 0) {
            device_option_present = true;
        }
        filtered.push_back(arg);
    }
    if (!safe_endpoint(expected_endpoint)) {
        std::fprintf(stderr, "placement refusal: expected-endpoint\n"); return 2;
    }
    std::vector<char *> cargs;
    for (auto & arg : filtered) cargs.push_back(arg.data());
    common_params params;
    // The common parser requires a model path for LLAMA_EXAMPLE_COMMON. The
    // placement probe never opens it; this sentinel keeps the probe strictly
    // pre-allocation while retaining the production argument parser.
    params.model.path = "/dev/null";
    common_init();
    if (!common_params_parse(static_cast<int>(cargs.size()), cargs.data(), params, LLAMA_EXAMPLE_COMMON)) return 2;

    contract_input input;
    input.device_option_present = device_option_present;
    input.split_mode = params.split_mode;
    input.tensor_split = { params.tensor_split[0], params.tensor_split[1] };
    input.expected_endpoint = expected_endpoint;
    input.n_layer = 62;

    if (params.devices.size() == 3 && params.devices[0] != nullptr &&
        params.devices[1] != nullptr && params.devices[2] == nullptr) {
        for (size_t i = 0; i < 2; ++i) {
            auto * dev = params.devices[i];
            size_t free = 0;
            size_t total = 0;
            ggml_backend_dev_memory(dev, &free, &total);
            auto * reg = ggml_backend_dev_backend_reg(dev);
            input.devices.push_back({
                ggml_backend_dev_name(dev), reg ? ggml_backend_reg_name(reg) : "",
                ggml_backend_dev_description(dev), free, total,
            });
        }
    }
    const bool exact_split = params.tensor_split[0] == 1.0f && params.tensor_split[1] == 1.0f &&
        std::all_of(params.tensor_split + 2, params.tensor_split + llama_max_devices(), [](float value) {
            return value == 0.0f;
        });
    if (exact_split) {
        input.ownership = llama_model_resolve_layer_devices(62, params.n_gpu_layers, { 0.5f, 1.0f });
    }

    contract_result result;
    std::string reason;
    if (!contract_validate(input, result, reason)) {
        std::fprintf(stderr, "placement refusal: %s\n", reason.c_str());
        return 3;
    }
    std::printf(
        "{\"admitted\":true,\"endpoint\":\"%s\",\"split_mode\":\"layer\","
        "\"tensor_split\":\"1,1\",\"repeating_layers\":62,\"output_device\":\"ROCm0\","
        "\"devices\":[{\"name\":\"RPC0\",\"backend\":\"RPC\",\"free\":%zu,\"total\":%zu},"
        "{\"name\":\"ROCm0\",\"backend\":\"ROCm\",\"free\":%zu,\"total\":%zu}],"
        "\"ownership\":{\"repeating_rpc\":%d,\"repeating_rocm\":%d,\"total_rpc\":%d,\"total_rocm\":%d}}\n",
        expected_endpoint.c_str(), input.devices[0].free, input.devices[0].total,
        input.devices[1].free, input.devices[1].total, result.repeating_rpc,
        result.repeating_rocm, result.total_rpc, result.total_rocm);
    return 0;
}
