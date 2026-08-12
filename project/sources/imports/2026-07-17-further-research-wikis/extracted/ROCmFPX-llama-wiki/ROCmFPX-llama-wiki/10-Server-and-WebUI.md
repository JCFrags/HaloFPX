# Server and WebUI

> **Audit snapshot:** fork `a5605a72768c6562241b248e268e33dc92787394`; upstream `86d86ed4396b4130922f7b9af26e3d9fc11a591b`; inventory date `2026-07-17`.  
> **Decision vocabulary:** **RETAIN** = capability/ABI must survive; **REFRESH** = preserve capability but re-port onto current upstream; **RETIRE** = remove the fork patch and use upstream or archive it. [S-FORK-HEAD](https://github.com/charlie12345/ROCmFPX/tree/a5605a72768c6562241b248e268e33dc92787394) [S-UP-HEAD](https://github.com/ggml-org/llama.cpp/tree/86d86ed4396b4130922f7b9af26e3d9fc11a591b)

## Fork-specific server capabilities

| Capability | Primary paths | Decision | Migration treatment | Primary sources |
|---|---|---|---|---|
| Per-request speculative controls | `tools/server/server-task.cpp`, `server-context.cpp` | **RETAIN** | Re-port to current schema/task plumbing and preserve clamp/isolation tests. | [S-C-B56](https://github.com/charlie12345/ROCmFPX/commit/b56ad79d77bc1eb9fe6407640eec8b8edfc04900) [S-SERVER-TASK](https://github.com/charlie12345/ROCmFPX/blob/a5605a72768c6562241b248e268e33dc92787394/tools/server/server-task.cpp) [S-SERVER-CTX](https://github.com/charlie12345/ROCmFPX/blob/a5605a72768c6562241b248e268e33dc92787394/tools/server/server-context.cpp) [S-SERVER-SCHEMA-UP](https://github.com/ggml-org/llama.cpp/blob/86d86ed4396b4130922f7b9af26e3d9fc11a591b/tools/server/server-schema.cpp) |
| Strict HY3 verification | `common/speculative.*`, `tools/server/server-context.cpp` | **RETAIN** | Implement as a small policy layer above upstream draft-MTP. | [S-C-7D7](https://github.com/charlie12345/ROCmFPX/commit/7d7b06bc5e6fd4a128af8efa954caa7409376a79) [S-SPEC-UP](https://github.com/ggml-org/llama.cpp/blob/86d86ed4396b4130922f7b9af26e3d9fc11a591b/common/speculative.cpp) [S-SERVER-CTX](https://github.com/charlie12345/ROCmFPX/blob/a5605a72768c6562241b248e268e33dc92787394/tools/server/server-context.cpp) |
| Automatic strict-mode slot constraint | server startup/context | **REFRESH** | Make the constraint explicit and test multi-slot behavior. | [S-C-276](https://github.com/charlie12345/ROCmFPX/commit/2766f419526ea14ba1be8f31eca21263cfc52813) |
| SSD prompt cache | arguments, common speculative state, server context/task, unit tests | **REFRESH** | Preserve target+draft+spec state, but version and harden the cache format. | [S-C-C81](https://github.com/charlie12345/ROCmFPX/commit/c81c7c92233b6370b4eb7087398779a8dcb234a4) [S-C-BB7](https://github.com/charlie12345/ROCmFPX/commit/bb7d9cb5965e3be1ce2073134ba14787bf378113) [S-C-756](https://github.com/charlie12345/ROCmFPX/commit/756121a5e8e7da464aebd2ab344a2aefef6cecac) [S-C-0D7](https://github.com/charlie12345/ROCmFPX/commit/0d7ac512e5eb511843797c07a16bc7d97d3bc4f8) [S-SERVER-TEST](https://github.com/charlie12345/ROCmFPX/blob/a5605a72768c6562241b248e268e33dc92787394/tools/server/tests/unit/test_prompt_cache_disk.py) |
| ROCmFPX serving scripts/profiles | `scripts/rocmfpx-*`, `run-rocmfpx-mtp-server.sh`, serving docs | **RETAIN** | Update flags/endpoints after upstream server rebase. | [S-SERVER-DOC](https://github.com/charlie12345/ROCmFPX/blob/a5605a72768c6562241b248e268e33dc92787394/docs/ROCmFPX-SERVING.md) [S-PR27-FILES](https://github.com/charlie12345/ROCmFPX/pull/27/files) |

## Disk-cache behavior

The cache feature adds `--cache-disk` and a size limit, creates a private run directory, and integrates idle-slot persistence with speculative contexts. It also changes speculative state interfaces so implementation-private state can be serialized, restored, and position-shifted. [S-C-C81](https://github.com/charlie12345/ROCmFPX/commit/c81c7c92233b6370b4eb7087398779a8dcb234a4) [S-SERVER-ARGS](https://github.com/charlie12345/ROCmFPX/blob/a5605a72768c6562241b248e268e33dc92787394/common/arg.cpp) [S-SERVER-CTX](https://github.com/charlie12345/ROCmFPX/blob/a5605a72768c6562241b248e268e33dc92787394/tools/server/server-context.cpp) [S-SPEC-FORK](https://github.com/charlie12345/ROCmFPX/blob/a5605a72768c6562241b248e268e33dc92787394/common/speculative.cpp)

Subsequent commits address portable file synchronization, UTF-8 filesystem paths, and portable failure tests. These are evidence that the cache crosses platform and durability boundaries and should be treated as a versioned storage subsystem rather than a small server optimization. [S-C-BB7](https://github.com/charlie12345/ROCmFPX/commit/bb7d9cb5965e3be1ce2073134ba14787bf378113) [S-C-756](https://github.com/charlie12345/ROCmFPX/commit/756121a5e8e7da464aebd2ab344a2aefef6cecac) [S-C-0D7](https://github.com/charlie12345/ROCmFPX/commit/0d7ac512e5eb511843797c07a16bc7d97d3bc4f8) [S-SERVER-TEST](https://github.com/charlie12345/ROCmFPX/blob/a5605a72768c6562241b248e268e33dc92787394/tools/server/tests/unit/test_prompt_cache_disk.py)

## Server changes that should be dropped

The fork contains generic server/WebUI backports unrelated to ROCmFPX semantics:

- async conversion and base64 image-URI repairs in the WebUI;
- WebUI asset provisioning/CI isolation;
- `read_file` indentation preservation;
- generic Windows test-shutdown and parser/Jinja fixes.

These should be inherited from current upstream unless a fresh upstream reproducer remains. [S-C-FF8](https://github.com/charlie12345/ROCmFPX/commit/ff8e7b8cf9dab714951df49d71f5835a7322404a) [S-C-063](https://github.com/charlie12345/ROCmFPX/commit/0631515b0bca7859387e3467a6f6ac6379622a02) [S-C-A8C](https://github.com/charlie12345/ROCmFPX/commit/a8c41d31423e3e0b6f2dc7138af1d4fba8edb295) [S-C-D52](https://github.com/charlie12345/ROCmFPX/commit/d52c96a8339325417624351bebad194c3864cb26) [S-C-FE2](https://github.com/charlie12345/ROCmFPX/commit/fe2b7dc5e19a5e24c276593368a1bb41d0e27b1d) [S-UP-HEAD](https://github.com/ggml-org/llama.cpp/tree/86d86ed4396b4130922f7b9af26e3d9fc11a591b)

## API compatibility note

The fork’s request-level speculative fields are valuable API surface. They should be documented as an extension until upstream enables an equivalent schema, and the server should advertise support rather than silently accepting/ignoring unknown fields. [S-C-B56](https://github.com/charlie12345/ROCmFPX/commit/b56ad79d77bc1eb9fe6407640eec8b8edfc04900) [S-SERVER-SCHEMA-UP](https://github.com/ggml-org/llama.cpp/blob/86d86ed4396b4130922f7b9af26e3d9fc11a591b/tools/server/server-schema.cpp) [S-SERVER-DOC](https://github.com/charlie12345/ROCmFPX/blob/a5605a72768c6562241b248e268e33dc92787394/docs/ROCmFPX-SERVING.md)

## WebUI classification

All three files changed by the fork’s original WebUI repair are generic application code. The technical inventory classifies them `RETIRE`, and the build should consume the current upstream WebUI. ROCmFPX-specific controls, if needed, should be exposed through supported server metadata/config APIs rather than long-lived WebUI forks. [S-C-FF8](https://github.com/charlie12345/ROCmFPX/commit/ff8e7b8cf9dab714951df49d71f5835a7322404a) [S-C-A8C](https://github.com/charlie12345/ROCmFPX/commit/a8c41d31423e3e0b6f2dc7138af1d4fba8edb295) [S-UP-HEAD](https://github.com/ggml-org/llama.cpp/tree/86d86ed4396b4130922f7b9af26e3d9fc11a591b)

## Server acceptance criteria

| Area | Required test |
|---|---|
| Request overrides | Two parallel requests with different `n_max/n_min/p_min` retain independent settings. |
| Strict mode | Greedy output matches target-only output; incompatible parallelism is rejected or clearly constrained. |
| Cache restart | Prompt cache survives process restart and reproduces uninterrupted output. |
| Cache corruption | Truncated, wrong-version, wrong-model, and checksum-failed entries are rejected without partial state restore. |
| Filesystem | UTF-8 paths, permission failures, disk-full, rename/sync failures, and cleanup are covered. |
| Upstream API | OpenAI-compatible and native completion endpoints remain compatible after rebase. |

The existing fork commits and unit tests provide the starting evidence for these required tests. [S-C-B56](https://github.com/charlie12345/ROCmFPX/commit/b56ad79d77bc1eb9fe6407640eec8b8edfc04900) [S-C-7D7](https://github.com/charlie12345/ROCmFPX/commit/7d7b06bc5e6fd4a128af8efa954caa7409376a79) [S-C-C81](https://github.com/charlie12345/ROCmFPX/commit/c81c7c92233b6370b4eb7087398779a8dcb234a4) [S-C-756](https://github.com/charlie12345/ROCmFPX/commit/756121a5e8e7da464aebd2ab344a2aefef6cecac) [S-C-0D7](https://github.com/charlie12345/ROCmFPX/commit/0d7ac512e5eb511843797c07a16bc7d97d3bc4f8) [S-SERVER-TEST](https://github.com/charlie12345/ROCmFPX/blob/a5605a72768c6562241b248e268e33dc92787394/tools/server/tests/unit/test_prompt_cache_disk.py)
