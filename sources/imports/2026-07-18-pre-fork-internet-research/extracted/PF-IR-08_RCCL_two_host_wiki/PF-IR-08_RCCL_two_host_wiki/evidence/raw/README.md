# Byte-exact raw evidence

These files are complete upstream file bodies at the pinned refs. Each file was checked by recomputing the Git blob SHA-1 (`sha1("blob <size>\0" + bytes)`) and comparing it with the upstream blob identifier recorded during collection.

| Source ID | Repository path | Ref | Git blob | Bytes |
|---|---|---|---|---:|
| `RAW-ACTIVE-FAULT-DOC` | `projects/rccl/docs/how-to/fault-tolerance.rst` | `801a9ca2ad89…` | `4cdd5a668e693bc2ffc03056c00de13e61ae87a8` | 11750 |
| `RAW-ACTIVE-PLUGIN-HEADER` | `projects/rccl/src/include/plugin/nccl_net.h` | `801a9ca2ad89…` | `c7c8d995ef05c771f6be9ff7edf8434e026b3d98` | 2215 |
| `RAW-ACTIVE-PLUGIN-V12` | `projects/rccl/src/include/plugin/net/net_v12.h` | `801a9ca2ad89…` | `299fd32cbec1c9762ca4e458b3ee42420bd61040` | 10363 |
| `RAW-ACTIVE-VERSION` | `projects/rccl/makefiles/version.mk` | `801a9ca2ad89…` | `a623ed8aa0ff402b2a66c8d916407b93217cadd9` | 299 |
| `RAW-STABLE-VERSION` | `makefiles/version.mk` | `96a25b5fd6f7…` | `3b182d61bdab57ddbe66f4f1e8383095fd7917af` | 103 |
| `RAW-STABLE-PLUGIN-HEADER` | `src/include/plugin/nccl_net.h` | `96a25b5fd6f7…` | `de271219b010a5d201113605e6a2a407fb5fafd7` | 1864 |
| `RAW-ACTIVE-LICENSE` | `projects/rccl/LICENSE.txt` | `801a9ca2ad89…` | `0149df86b4c913dab4f490428c130c211eb0a056` | 2089 |
| `RAW-STABLE-LICENSE` | `LICENSE.txt` | `96a25b5fd6f7…` | `da5cc8feab0d002d79c261aba3507953ed966956` | 2089 |

The targeted raw set is not a full repository mirror. Larger implementation files are preserved as claim-scoped excerpts elsewhere under `evidence/source/`, with completeness explicitly recorded in `manifests/source_manifest.csv`.
