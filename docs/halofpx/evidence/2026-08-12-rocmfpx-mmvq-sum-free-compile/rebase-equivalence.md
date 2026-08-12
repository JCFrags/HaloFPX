# Target-build to rebased-source equivalence

PRs #31, #34, and #35 merged while PR #30 was under review. Rebasing PR #30
changed commit identities but did not change its target-built HIP/runtime
source. The current base is
`167df62ffc8970bc408d72e97ab71a57de4b69d2`.

The following command returned success for both commit pairs and the listed
paths:

```text
git diff --exit-code <target-built> <rebased> -- <paths below>
```

Commit mapping:

| Qualification | Target-built commit | Rebased commit |
|---|---|---|
| candidate ON | `3c9ba0b7c842a02b47c591ad47448cfb94b6dc72` | `b98dbf0` |
| candidate OFF and hardened guard | `a3fd09e0d522e20d0153bb2a07ddd09916249c8d` | `81a9b26` |

Exact shared Git blob identities:

| Path | Git blob |
|---|---|
| `ggml/CMakeLists.txt` | `5c97fe84deca2d083647e749a67dd09a1329a9ad` |
| `ggml/src/ggml-cuda/quantize.cu` | `83831ecbde79e76146b7b981ebb4579dfee8606d` |
| `ggml/src/ggml-cuda/rocmfpx-mmvq-sum-free.h` | `a05e2e5aa68f8050a8a6d9732dd0b3c64144263a` |
| `ggml/src/ggml-hip/CMakeLists.txt` | `c657c5aab18c27e30acb0cdb1c20a346e9d777ee` |
| `scripts/build-strix-rocmfp4-mtp.sh` | `047384449a38970530c8cb41485a2e487993e61f` |
| `tests/CMakeLists.txt` | `9178cd9594d38482c4858784ea716b13ea7fcaef` |
| `tests/test-halofpx-rocmfpx-mmvq-sum-free.cpp` | `cbb4dff617fdcf296677947e82583d744402bcb7` |

Candidate-ON qualifier blob shared by `3c9ba0b` and `b98dbf0`:

```text
scripts/qualify-rocmfpx-mmvq-sum-free.sh
015726861fa83cb28599de504c08acbe352971af
```

Hardened qualifier/guard blobs shared by `a3fd09e` and `81a9b26`:

```text
scripts/qualify-rocmfpx-mmvq-sum-free.sh
8e3f1fa14c5fc90393dcf6b48b6a8405980b3114
scripts/halofpx-rocmfpx-mmvq-process-guard.sh
0bfdb2efae20bc1e4610544046a8eeb050278c0e
tests/test-halofpx-rocmfpx-mmvq-process-guard.sh
e1769ab965e2ae2ab26c835bff6f157173441456
```

Both exact path-limited comparisons returned exit code zero after the latest
rebase. Other repository paths differ because PRs #31, #34, and #35 added the
A/B evidence core, current project-state records, and sampled/raw logits row
fix. This receipt supports source equivalence for the compile-level claim only.
It does not replace GPU correctness, model parity, or performance
qualification.
