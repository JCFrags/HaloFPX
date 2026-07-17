# Evidence model and support semantics

## Why the classification is strict

The Strix Halo stack spans independent authorities:

- AMD certifies selected ROCm hardware, operating systems, release lines, and applications.
- Linux, Mesa, CMake, Vulkan Loader, and llama.cpp publish upstream code and releases without jointly certifying a complete Strix Halo appliance.
- Community maintainers report combinations that run on real systems, but those reports vary in workload, firmware, BIOS, memory allocation, and regression coverage.

A row is therefore classified by the **narrowest defensible claim**, not by the strongest label found anywhere in the stack.

## Evidence grades

| Grade | Meaning | Typical use |
|---|---|---|
| `O1` | Vendor or upstream hardware/OS/application support matrix | “ROCm 7.14 Core SDK supports gfx1151 on these OS/kernel pairs.” |
| `O2` | Official upstream release, source, or build configuration | “llama.cpp b10064 exists and this option defaults OFF.” |
| `C1` | First-hand report with exact versions, reproducer, benchmark, or logs | Regression and workload-specific claims |
| `C2` | Maintained community image or repeated operational recipe | Known-good community baseline |
| `C3` | Single anecdotal report with limited reproduction detail | Leads for testing, not defaults |
| `I1` | Explicit synthesis or inference from compatible component scopes | Reproducible candidate, never relabeled official |

## Classification labels

- **official-supported** — the target and release/OS scope are explicitly supported by the responsible vendor.
- **official-upstream-current** — a current upstream release exists; target-specific validation may still be absent.
- **community-validated** — a maintainer or reproducible report exercised the combination on Strix Halo.
- **experimental-community** — working research or fork work with explicit stability limitations.
- **unsupported-known-bad** — documented failure or an official unsupported classification.
- **reproducible-candidate** — all components can be pinned coherently, but the combined profile is not published as a supported application row.
- **official-historical** — formerly documented vendor path that is obsolete under the current matrix.

## Non-transitivity rules

1. **Core SDK support is not application support.** ROCm 7.14’s gfx1151 row does not automatically validate PyTorch, llama.cpp, ROCmFPX, or a given container image.
2. **A general ROCm release matrix is not a target-specific row.** ROCm 7.2.4 is official generally; the captured RDNA 3.5 table names 7.2.1–7.2.3 specifically.
3. **An upstream release is not a host certification.** A llama.cpp ROCm 7.2 binary still depends on a correct host kernel, firmware, `/dev/kfd`, runtime libraries, and model path.
4. **A fixed kernel does not fix bad firmware.** Kernel and firmware are separate gates.
5. **A community benchmark is workload-specific.** Vulkan beating HIP in one MoE decode test does not establish a universal backend ranking.
6. **A workaround is not support.** Architecture overrides or disabled kernel features can help isolate faults but do not turn an unsupported release into a supported one.

## Updating the matrix

A status promotion requires a new source that states the relevant scope. For example, ROCm 7.2.4 moves from “general official + community-validated gfx1151” to “official gfx1151 stable” only when AMD’s target-specific RDNA 3.5 table or an equivalent application matrix names it.
