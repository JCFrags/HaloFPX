# Build inclusion, runtime reachability, and trust boundaries

## Build inclusion

| Label | Component | Generic source default | ROCmFPX/predecessor release recipe | Result |
|---|---|---|---|---|
| [MEASURED] | RPC backend and `rpc-server` | `GGML_RPC=OFF` | Release workflow passes `-DGGML_RPC=ON` | RPC code and tool are distributed unless workflow changes. |
| [MEASURED] | HTTP `llama-server` | Built for a top-level standalone build | Release workflow passes `-DLLAMA_BUILD_SERVER=ON` | HTTP server is distributed. |
| [MEASURED] | OpenSSL support | ROCmFPX root option is on | Enabled in candidate configuration | cpp-httplib uses OpenSSL paths; system OpenSSL version remains local evidence. |
| [OPEN] | Compression feature macros | Not conclusively enumerated in the captured build recipe | No definitive loaded-feature proof | Version fixes make captured cpp-httplib advisories non-applicable, but exact feature inventory remains local. |

## Listener and authentication behavior

| Label | Surface | Source default | Authentication | Trust-boundary conclusion |
|---|---|---|---|---|
| [MEASURED] | RPC server | `127.0.0.1:50052` | None in protocol | Loopback reduces exposure but does not authenticate local/container/forwarded peers; never expose to an untrusted network. |
| [MEASURED] | HTTP server | `127.0.0.1:8080` | API-key middleware; empty key set returns success | Default is unauthenticated localhost. Non-loopback use must fail closed unless an approved auth boundary is configured. |
| [MEASURED] | HTTP public routes | Health and model-list routes are allowlisted | Public by design | Keep response content minimal and do not infer that other routes are protected when the key set is empty. |
| [MEASURED] | Slot/state routes | Fork defaults slots endpoint on | Protected only when API keys are nonempty | Administrative state mutation is too permissive as a default; turn off. |
| [MEASURED] | Project MTP launcher | Loopback host; passes `--no-context-shift` | No API key was established by source inspection | Removes the n_discard execution precondition for that launcher only; does not constrain all launch paths. |

## GGUF and model ingestion

[VERIFIED] GGUF/model files are untrusted file inputs even when downloaded by an operator rather than uploaded through a listener.

[MEASURED] The highest-risk allocation path for `GHSA-3p4r-fq3f-q74v` includes tools and auxiliary ingestion paths that request actual allocation (`no_alloc=false`), including quantization, imatrix, GGUF inspection, and control-vector loading.

[RECOMMENDATION] Admit only hashed/approved model and auxiliary files into production. Perform conversion/quantization in an isolated worker with resource limits and no secrets.

## Slot, state, and prompt-cache parsing

[MEASURED] Slot filenames are validated with subdirectories disallowed; the validator rejects path separators, `..`, invalid/noncanonical UTF-8, control characters, Windows-invalid characters, leading/trailing spaces, and trailing dots.

[MEASURED] Sequence-state loading validates magic/version, token count against caller capacity, exact reads, and catches exceptions at the public API wrapper.

[MEASURED] The automatic prompt cache uses a unique per-run owner directory, restrictive permissions, temporary files, fsync/fdatasync, atomic rename, exact state-size checks, and restored-token equality checks.

[OPEN] Same-UID attackers, a malicious pre-existing cache-base symlink, filesystem reparse behavior, nested memory-state parser coverage, and replacement races remain local test subjects.

## Reachability logic

| Label | Condition | Required for exploitation/reliability impact |
|---|---|---|
| [ASSUMPTION] | Code compiled | The affected translation unit and feature macro are present in the binary. |
| [ASSUMPTION] | Process launched | The RPC or HTTP executable/backend is active. |
| [ASSUMPTION] | Boundary reachable | A peer or untrusted file can cross the listener/filesystem trust boundary. |
| [ASSUMPTION] | Route/command reachable | Authentication, endpoint flags, and protocol state allow the vulnerable handler. |
| [ASSUMPTION] | Data preconditions satisfied | Context/fullness, type, size, pointer, parser mode, or backend conditions hold. |
| [OPEN] | Local proof | The remote source review cannot establish any deployed condition above. |