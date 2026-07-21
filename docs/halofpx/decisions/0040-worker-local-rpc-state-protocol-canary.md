# ADR-0040: worker-local RPC state protocol for the disposable two-rank canary

- Status: accepted design; implementation and qualification remain default-off
- Date: 2026-07-21
- Base: `78a102ac3212e4987486761983c336438cc3e7c0`
- Scope: Linux-only, two logical ranks, attention-KV sequence state, one
  disposable exact-key canary

## Decision

Add a target-native RPC command family and a llama sequence-storage adapter.
The coordinator continues to own `llama_context`, request/slot/token history,
the sampler, exact-key selection, and the coordinator-local portion of sequence
state. The RPC worker continues to own remote ggml buffers and graph execution;
it additionally owns one operator-configured local cache root, one configured
logical-rank identity, immutable local state objects, and the stage/apply state
for one bounded restore attempt per connection.

The worker does not reconstruct or own a `llama_context`. The adapter uses
`LLAMA_STATE_SEQ_FLAGS_ON_DEVICE` to copy selected live tensor ranges into
disposable device-local sequence-storage buffers. Remote-to-remote copies stay
on the worker. The new capture command persists those storage-buffer ranges on
the worker and returns only their descriptors, digests, byte counts, and
status. On restore, the adapter first recreates the same disposable sequence
storage from a clean context. The worker validates its immutable object and
loads it only into those staging buffers. No live sequence tensor is changed
while validation or all-rank agreement is pending.

After the coordinator-local object and every required worker return matching
`READY`, the coordinator sends `COMMIT_APPLY`. The worker reauthenticates the
attempt and copies its staged ranges into the described live ranges. Only after
worker apply succeeds does the coordinator copy its already-validated local
staging ranges live. Any apply failure destroys and recreates the disposable
context before cold recomputation. This first milestone therefore claims
validation-before-mutation and clean-context recovery, not crash-atomic live
mutation.

## Gates and configuration

The implementation is compiled only with
`GGML_RPC_HALOFPX_LOCAL_STATE=ON`. The option defaults to `OFF` and is rejected
outside Linux. Runtime remains off unless the worker is started with all of:

- an absolute operator-chosen `--halofpx-state-root`;
- `--halofpx-state-rank 1` for the first two-rank canary;
- `--halofpx-state-world 2`;
- a freshly generated 32-byte disposable control key supplied through a
  service-account-owned key file with
  no group/world permission bits;
- a 32-byte canary channel-binding value supplied through the same protected
  configuration; and
- the explicit `--halofpx-local-state` switch.

The coordinator uses separately configured rank `0`, world `2`, the same key
generation and channel binding, and a target-owned exact-key authority. The
wire never accepts a root, relative path, filename, object key, hostname,
device enumeration order, or caller-selected cache identity. The worker derives
the final object name as HMAC-SHA-256 over the authenticated stable checkpoint
identity and its configured logical rank. The stable identity contains every
closed-attempt field below except the per-attempt coordinator nonce; that nonce
still authenticates and anti-replays each wire attempt. Directory order and
filenames have no selection authority.

Single-node execution never relabels or concatenates the rank objects. It uses
a separately compatible single-node entry if one is later admitted; otherwise
it recomputes.

## Closed attempt identity

Every state-changing request binds all of the following fixed-width values:

- protocol major/minor and exact message type;
- key generation and 32-byte channel binding;
- exact model digest;
- compatibility root;
- plan digest and topology digest;
- world size, logical rank, and placement/ownership digest;
- checkpoint digest and generation;
- token-prefix digest, token count, and exact token boundary;
- component-manifest digest;
- fresh 32-byte coordinator attempt nonce; and
- fresh 32-byte worker commit nonce returned only after successful staging.

The component-manifest digest is computed over the actual canonical ordinal
descriptor sequence, not a caller label or directory order. Each
descriptor binds component kind, tensor type, four dimensions, four strides,
logical tensor label digest, byte offset, byte length, and content digest.
Pointer/buffer fields needed to address current process buffers are validated
against the worker's live RPC buffer registry but are excluded from persistent
identity.

For this milestone component kinds are closed to attention K and attention V.
Recurrent, hybrid, draft, speculative, MTP, grammar, sampler, and RNG state are
unsupported and force a miss. The sampler remains coordinator-local and is
reset/replayed from the exact saved token boundary; no continuation claim may
depend on unpersisted sampler state.

## Wire version and exact limits

The family appends commands after the existing RPC v4.0.1 command set without
renumbering an existing command:

| Command | Number | Request | Response |
|---|---:|---:|---:|
| `HALOFPX_STATE_CAPS` | 17 | 0 bytes | 64 bytes |
| `HALOFPX_STATE_CAPTURE` | 18 | at most 1,048,576 bytes | 256 bytes |
| `HALOFPX_STATE_STAGE` | 19 | at most 1,048,576 bytes | 256 bytes |
| `HALOFPX_STATE_COMMIT_APPLY` | 20 | at most 1,048,576 bytes | 256 bytes |
| `HALOFPX_STATE_ABORT` | 21 | 480 bytes | 256 bytes |

The canary protocol version is exactly `1.0`. Every integer is unsigned
little-endian and every structure is packed with reserved bytes required to be
zero. The maximum is 4,096 component descriptors, 1 GiB per component, 64 GiB
per rank object, one staged attempt per connection, a process-lifetime
fail-closed ledger of 4,096 accepted stage nonces, and five seconds from an
authenticated `READY` response to an accepted `COMMIT_APPLY`. The implementation
must also reject arithmetic overflow, duplicate ordinals, overlapping target
ranges, unknown kinds, zero-length ranges, trailing bytes, noncanonical order,
nonzero reserved bytes, and any request or response whose size is not exact.

All state-changing requests and all state-operation responses carry
`HMAC-SHA-256(K_control, "halofpx.rpc-local-state.v1\0" ||
message-with-zero-tag)`. The full tag covers the command number, exact encoded
length, closed attempt identity, all component descriptors, status, both
nonces, channel binding, and a SHA-256 digest of the complete authenticated
request transcript. Tags are compared in constant time. Capability discovery
is deliberately unauthenticated, discloses only version and limits, grants no
state authority, and can only cause fail-closed denial if altered.
The client also requires status-specific exact component/byte counts and
required or forbidden object/worker digests before accepting a response.

No command response may exceed 256 bytes after capability discovery. State
payload bytes are forbidden in every response and in `STAGE`, `COMMIT_APPLY`,
and `ABORT`. `CAPTURE` and `COMMIT_APPLY` requests contain only descriptors;
the addressed tensor bytes remain worker-local.

The coordinator canary stores its control bytes, coordinator-local staging,
tokens, and expected worker object digest under the checkpoint-derived root
with a fixed 504-byte HMAC-authenticated receipt. The receipt binds their
content digests to the same model, compatibility, plan, topology, placement,
checkpoint/generation, token boundary/prefix, component manifest, world, and
channel identity. Receipt or content failure is detected before worker stage
and cold-recomputes from freshly tokenized request input.

## Worker object and attempt lifecycle

For every state operation the worker revalidates and opens the configured root
using Linux descriptor-relative, no-follow primitives and refuses a symlink,
non-directory, wrong-owner, or group/world-writable root. It creates fixed
internal directories itself.

Capture reads only authenticated, registry-bounded worker-resident tensor
ranges. It writes a versioned header, canonical descriptors, and concatenated
range bytes to a unique staging inode, computes SHA-256 digests while reading,
flushes the file, atomically links it to the target-derived immutable name
without replacement, flushes the parent directory, and then returns `STORED`.
A pre-existing target name makes capture fail closed; later stage is the only
path that reopens and fully validates an existing immutable object.

Stage derives the object name from authenticated identity, opens it without
following links, validates type/owner/mode/size/header/identity/descriptor and
content digests completely, verifies the described destination staging ranges
against the RPC buffer registry, and only then loads bytes into staging ranges.
It records the attempt and returns authenticated `READY` with a fresh worker
commit nonce. Staging buffers are disposable and are not live context tensors.

Commit/apply requires the exact pending attempt, both nonces, unexpired timer,
identical component set, and authenticated live destination descriptors. It
copies staged ranges to live ranges on the worker and returns `APPLIED` only
after every copy succeeds. Abort discards the attempt. Connection loss,
timeout, duplicate/replayed stage or commit, any new attempt, or destruction of
timeout, duplicate/replayed stage or commit, any new `STAGE` attempt, or
destruction of a referenced buffer also discards it. Any intervening legacy RPC command that
can allocate, free, clear, set, copy, initialize, or compute against buffers
discards the pending attempt before that command is dispatched.

## Failure contract

Missing, corrupt, short, oversized, stale, mismatched, replayed, duplicated,
partial, unauthenticated, unauthorized, timed-out, or failed capture/stage/apply
state makes the whole distributed attempt a miss. The coordinator aborts every
rank it can reach, destroys/recreates the disposable context if live apply may
have begun, resets coordinator-local sequence and sampler state, and recomputes
from the original request tokens. No partial rank success, partial component
set, previous-generation readiness, or target-only continuation is accepted.

Worker storage errors are not RPC process-fatal. Malformed protocol framing and
oversized input close the connection. Diagnostics contain status class, logical
rank, generation, and truncated digests only; never paths, prompts, tokens,
keys, full object identities, or state bytes.

## Canary acceptance boundary

Before this ADR can be promoted beyond an accepted design, focused protocol and
adapter tests must prove exact-size parsing, HMAC coverage, replay/timeout and
range rejection, immutable object revalidation, validation-before-live-copy,
all-rank agreement, and compile/runtime-off preservation. One disposable
two-host small-model run must compare uninterrupted, cold, and process-restart
restore suffixes; record worker object creation/read; show zero GET/SET state
payload transfer on the rank-local path; and show one missing/corrupt worker
object plus one plan/topology mismatch both cold-recompute from a clean context.

The 160 GB model, production service roots/ports/units, eviction, online repair,
shared reuse, broad fault injection, cable faults, and performance claims remain
outside this decision.

## Alternatives rejected

- Wrapping the current monolithic sequence blob: it still transfers worker KV
  pages through GET/SET and violates ADR-0039.
- Reconstructing `llama_context` on the RPC worker: it duplicates model/runtime
  authority and is unnecessary for tensor-range ownership.
- Loading directly into live remote tensors during validation: it permits
  partial mutation before all-rank agreement.
- Caller-provided paths or object names: they create path and selection
  authority outside the worker and exact-key policy.
- Treating current RPC TCP as production-secure merely because messages use an
  HMAC: the first canary is a private, explicitly bound lab protocol; full
  mutually authenticated confidential transport remains a later enablement
  gate under ADR-0005.
