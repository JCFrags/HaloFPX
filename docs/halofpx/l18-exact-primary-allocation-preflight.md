# L18 exact-primary allocation preflight

Date: 2026-07-21

Base: `730e96330ae0585719941a93b65c31a6217a7a54`

Outcome: **PASS — ALLOCATION SHAPES ADMISSIBLE; NO PRODUCTION MUTATION; NO PRIMARY LOAD**

## Result

[VERIFIED] The final L18 probe hashes the exact pinned 159,873,097,824-byte
artifact to
`96506ada918e60ca9a9cfde8a5437790e4453401a6a3e236e3f55e7bac3aaea6`,
then opens it with the real model loader using `no_alloc=true`, `use_mmap=false`,
explicit `RPC0,ROCm0`, layer split, and tensor split `1,1`. The loader accounted
for all 809 GGUF tensors: 809 created, 809 unique source names, zero unknown,
zero unaccounted, and no views or source slices. The exact source tensor bytes
sum to 159,864,809,984 bytes.

The real loader produced three planned, would-be material allocation groups:

| Device/backend | Layers | Tensors | Exact request bytes |
|---|---:|---:|---:|
| RPC0 / RPC | 0-31 plus `output.weight` | 416 | 80,950,550,528 |
| ROCm0 / ROCm device | 32-61 plus `output_norm.weight` | 392 | 78,280,456,704 |
| ROCm0 / ROCm host | input embedding (`token_embd.weight`) | 1 | 633,802,752 |

The three group requests sum exactly to the source tensor bytes. RPC0 owns 32
repeating layers plus `output.weight`; ROCm0 owns 30 repeating layers,
`output_norm.weight`, and the host input-embedding group. This is an important
divergence from L17's resolver prediction that placed the output on ROCm0. The
L18 architecture loader groups are higher authority for allocation. P01/P11
remain supporting evidence only because they used the same explicit device
order and loaded successfully.

## Capacity gate

The measured backend-reported total capacity was 133,143,986,176 bytes on each
device. Current free values (19,458,879,488 RPC0 and 13,903,089,664 ROCm0) were
recorded while production was live and were not used as a quiescent-run
admission basis.

The gate separately labels exact weight requests, runtime `no_alloc`
context/KV and compute simulations, and policy assumptions:

| Device | Exact weights | Context estimate | Compute estimate | 10% fragmentation assumption | 16 GiB reserve | Required | Margin |
|---|---:|---:|---:|---:|---:|---:|---:|
| RPC0 | 80,950,550,528 | 285,212,672 | 94,464,000 | 8,133,022,720 | 17,179,869,184 | 106,643,119,104 | 26,500,867,072 |
| ROCm0 | 78,914,259,456 | 267,386,880 | 416,022,528 | 7,959,766,887 | 17,179,869,184 | 104,737,304,935 | 28,406,681,241 |

Both total and maximum-single-request checks retain the required margin. These
are admissible planned shapes, not proof that a future allocator will succeed.

## Qualification and operations

The focused self-test passed exact identity, wrong-hash, wrong-order,
unaccounted-source, insufficient-margin, and overflow cases. Development also
corrected a stack-bound hash buffer before the retained diagnostic refusal for
a misplaced completion finalizer. Neither admitted a plan or allocated model
state. The final binary is SHA-256
`31686c793c11d93b68aa0e690ee06fd209bd2b264604f3284eb6225cd5275c07`;
its resolved loader library `libllama.so.0.0.0` is SHA-256
`accb278956f5e8108000df40530f218e1fcbf3a513443eb4c34c0f258ad4e310`;
the final raw plan is SHA-256
`b78fa995f8360673083d7b882189abf1a4d62fe903cfa45436d99908ea602d9b`.

The isolated nimo-1 RPC worker used port 50194. Its journal contains no
`alloc_buffer`, `GET_TENSOR`, or `SET_TENSOR` operation. Only zero-byte backend
sentinels and metadata/capability queries were permitted by the `no_alloc`
path. Production remained continuously active at nimo-1 coordinator PID
2144857/8081 and nimo-2 worker PID 1305879/50052, both with `NRestarts=0`, and
HTTP remained 200.

The protected raw archive is
`/var/tmp/halofpx-l18-primary-allocation-evidence-20260721-v4.tar.zst`, mode 0600,
SHA-256 `8c05b1c31644b919af8bd9a30953022b58ef4e0588ca694d0edf7fd3fc52b8a9`.
The superseded pre-closeout v1-v3 archives are retained rather than overwritten.
The transient service is unloaded, port 50194 is closed, and its binary plus
the nimo-2 disposable source/build roots are absent.

## Residual uncertainty and boundary

The GGML allocation-size query shares the real loader contexts and grouping,
but it does not perform an allocation. KV/context and compute values are
runtime simulations; fragmentation and reserve are explicit policy
assumptions. L18 does not prove allocator success, live fragmentation,
throughput, correctness, or a maintenance transition. The L17 output-device
prediction mismatch remains explicit and requires the real loader plan, rather
than the resolver alone, to govern any later admission. L18 did not load primary
weights, run inference, write cache state, stop or restart production, open
L19, or authorize a primary retry.
