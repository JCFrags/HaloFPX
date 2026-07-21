#pragma once

#include "llama.h"

#include <vector>

// Resolves the repeating layers followed by the output layer to selected-device
// indices. A value of -1 means CPU. split_points must be cumulative, normalized,
// nondecreasing, and end at 1.0.
LLAMA_API std::vector<int> llama_model_resolve_layer_devices(
        int                        n_layer,
        int                        n_gpu_layers,
        const std::vector<float> & split_points);
