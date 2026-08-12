---
section_id: "32"
title: "llama.cpp lifecycle sources"
status: "verified"
last_verified: "2026-07-16"
applies_to:
  repositories: ["ggml-org/llama.cpp"]
  software_versions: ["788e07d"]
  hardware_revisions: []
related_sections: ["33", "61", "68"]
---

# Sources

| ID | Primary source | Supports | Limitation |
|---|---|---|---|
| S32-01 | [model load API path](https://github.com/ggml-org/llama.cpp/blob/788e07dc91d266ad3162a1ce9037665656269689/src/llama.cpp), [model implementation](https://github.com/ggml-org/llama.cpp/blob/788e07dc91d266ad3162a1ce9037665656269689/src/llama-model.cpp), commit `788e07d`, accessed 2026-07-16 | load, error cleanup, tensor placement | internal interfaces can change |
| S32-02 | [context/decode/scheduler path](https://github.com/ggml-org/llama.cpp/blob/788e07dc91d266ad3162a1ce9037665656269689/src/llama-context.cpp), accessed 2026-07-16 | context params, reserve, decode, async compute | source behavior not yet traced on HaloFPX |
| S32-03 | [graph infrastructure](https://github.com/ggml-org/llama.cpp/blob/788e07dc91d266ad3162a1ce9037665656269689/src/llama-graph.cpp), [model graph builders](https://github.com/ggml-org/llama.cpp/tree/788e07dc91d266ad3162a1ce9037665656269689/src/models), accessed 2026-07-16 | graph inputs/builders and architecture specialization | no stable external graph-cache ABI asserted |
| S32-04 | [context state implementation](https://github.com/ggml-org/llama.cpp/blob/788e07dc91d266ad3162a1ce9037665656269689/src/llama-context.cpp), [public API](https://github.com/ggml-org/llama.cpp/blob/788e07dc91d266ad3162a1ce9037665656269689/include/llama.h), accessed 2026-07-16 | serialization APIs | cross-version/topology stability not guaranteed here |
| S32-05 | [server slot lifecycle](https://github.com/ggml-org/llama.cpp/blob/788e07dc91d266ad3162a1ce9037665656269689/tools/server/server-context.cpp), accessed 2026-07-16 | slot prompt/decode/context-shift path | product layer evolves rapidly |
| S32-06 | [ggml backend scheduler](https://github.com/ggml-org/llama.cpp/blob/788e07dc91d266ad3162a1ce9037665656269689/ggml/src/ggml-backend.cpp), accessed 2026-07-16 | buffer/backend scheduler contracts | distributed transport requires new validation |
