# Failure injection

## Objective

Prove that malformed input, resource pressure, cancellation, transport loss, and persistent-state corruption produce bounded, diagnosable outcomes without cross-request contamination or false success.

## Mandatory harness controls

- explicit per-case watchdog;
- isolated temporary directory and process group;
- no production model/cache paths;
- loopback or isolated-network RPC only;
- bounded filesystem images for ENOSPC;
- raw stdout/stderr and process-tree capture;
- preservation of original and mutated SHA-256 values;
- sanitizer lanes for parser/state corruption;
- health and correctness probe after recoverable failures;
- timeout classified as failure, never skip.

## Fault classes

| Class | Examples | Required evidence |
|---|---|---|
| Input corruption | bad GGUF, truncated state, malformed JSON | normalized rejection class, no sanitizer finding |
| Compatibility mismatch | model/quant/context/cache/protocol version | rejection before unsafe use |
| Resource exhaustion | host/device OOM, disk full, queue saturation | explicit response or documented observable fallback |
| Filesystem | permission, EIO, rename/fsync failure, interrupted write | atomicity and last-good recovery |
| Transport | HTTP disconnect, RPC disconnect/refusal | bounded cancellation/transport error and reuse probe |
| Process | SIGINT/termination during prefill/save | exit contract and persistent-state policy |
| Backend | unsupported op, allocation fault, device loss | no unchecked output; recovery policy explicit |
| Authorization/isolation | user namespace crossing, path traversal | deny and retain tenant isolation |

## GGUF and state corruption

Prefer structure-aware generators. For each mutation:

1. record the original digest;
2. generate one narrowly described mutation;
3. record the mutated digest and exact operation;
4. run with sanitizers and a watchdog;
5. assert failure class and post-failure process condition;
6. never reuse a partially loaded model/state in the same context unless the API explicitly supports it.

## Persistent-cache atomicity

Checkpoint publication should be tested at write, flush, fsync, directory update, and rename boundaries. Kill the writer at each supported hook. On restart, either the previous complete checkpoint or the new complete checkpoint may be accepted according to the format contract; a partial checkpoint must not become active.

## Cancellation phases

Inject cancellation during:

- model loading;
- early/middle/late prompt prefill;
- token decode and streaming;
- draft/MTP generation before verification;
- RPC transfer and remote execution;
- cache restore and cache publication;
- multiple-slot execution;
- graceful process shutdown.

Every cancellation case includes a follow-up clean request or state verification.

## Resource tests

Use test cgroups, bounded containers, loopback filesystems, fault-injection allocators, or backend mocks. Do not create uncontrolled host pressure. Hardware device-loss tests belong only on dedicated self-hosted runners.

The machine-readable registry is `fixtures/failure/failure-recipes.yaml`.
