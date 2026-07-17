---
section_id: "61"
title: "Continuation state sources"
status: "draft"
last_verified: "2026-07-17"
applies_to:
  repositories: ["ggml-org/llama.cpp@788e07dc91d266ad3162a1ce9037665656269689", "fewtarius/CachyLLama@6be745998f568e379ea197fcf827baec73ff9940", "fewtarius/llama-ai@1017f3dfdce3ca2b06aa9007b23295db3bb35722"]
  software_versions: []
  hardware_revisions: []
related_sections: ["35", "36"]
---

# Sources

Access date: 2026-07-17. Repository sources are pinned to full commits; commit dates identify revisions, not access dates or HaloFPX runtime validation.

| ID | Primary source and revision | Supports | Limitations |
|---|---|---|---|
| S61-01 | [llama.cpp source](https://github.com/ggml-org/llama.cpp/tree/788e07dc91d266ad3162a1ce9037665656269689), `788e07dc`, committed 2026-07-17 +02:00, accessed 2026-07-16 PDT | memory and sequence state APIs | moving implementation; runtime tests needed |
| S61-02 | [CachyLLama cache/server source](https://github.com/fewtarius/CachyLLama/tree/6be745998f568e379ea197fcf827baec73ff9940), `6be74599`, committed 2026-07-08 -04:00 | v3 target/draft/spec blobs and server restore | no general atomic multi-stream contract |
| S61-03 | [CachyLLama speculative implementation](https://github.com/fewtarius/CachyLLama/blob/6be745998f568e379ea197fcf827baec73ff9940/common/speculative.cpp), same commit | implementation-specific speculative state | explicit multi-state TODO |
| S61-04 | [CachyLLama sampling implementation](https://github.com/fewtarius/CachyLLama/blob/6be745998f568e379ea197fcf827baec73ff9940/common/sampling.cpp), same commit | stateful sampler/grammar surface | no persistence guarantee |
| S61-05 | [llama-ai integration repository](https://github.com/fewtarius/llama-ai/tree/1017f3dfdce3ca2b06aa9007b23295db3bb35722), `1017f3df`, committed 2026-07-08 -04:00 | wrapper/submodule baseline | delegates cache implementation to CachyLLama |

Source count is 5.
