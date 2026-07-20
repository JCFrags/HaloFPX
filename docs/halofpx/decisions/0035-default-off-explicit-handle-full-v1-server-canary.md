# ADR-0035: default-off explicit-handle full-v1 server canary

Status: accepted only for the Linux-only L08i laboratory canary. This decision
does not authorize production persistence, automatic cache discovery, shared
scope, multi-writer publication, generation advancement, distributed restore,
retention, or unattended use with user data.

L08i may link the L08g transformer codec and L08h-b generation-one authority
into `llama-server` only when four compile-time gates are explicitly enabled:
`HALOFPX_CONTEXT_STORE_CANARY`, `HALOFPX_CONTEXT_STORE_PROTECTED_CANARY`,
`HALOFPX_CONTEXT_STORE_COMPONENT_AUTHORITY`, and
`HALOFPX_CONTEXT_STORE_FULL_V1_CANARY`. The runtime remains off unless the
operator also selects `full-v1-rw-canary` and supplies authenticated private
scope, exact compatibility components, distinct pre-created owner-only roots,
a stable store UUID, and an owner-only operator-key file.

Publication captures one admitted idle-slot snapshot, encodes it into the
target-owned v1 format, publishes through the generation-one authority, and
returns the selected manifest digest as an opaque explicit handle. Restore
accepts no discovery or prefix match: the caller must supply that exact handle
and the exact expected token sequence. The server safely opens only the
digest-derived manifest path, authenticates its closed compatibility domain and
object roster, then lets L08h-b authenticate and reconcile the exact anchor
before lookup and decode. Only a complete decoded snapshot reaches the live
L07 restore boundary. Any failure remains a miss; a failed live restore clears
the empty destination slot so cold recomputation stays authoritative.

One server controller thread owns the adapter and its writer authority. The
canary uses generation one, world size one, rank zero, topology epoch one, and
the closed target-only transformer profile. This laboratory mapping deliberately
reuses the closed topology component for plan, ownership, and placement facts;
semantic distributed admission remains closed.

Serialization is currently supplied by the server queue, not an adapter-local
mutex. L08h-b still fail-closes nested or incoherent roots at authority-open
and operation boundaries, but startup diagnostics are not a separate root-
layout oracle. Encoded and read-only frame containers are bounded and freed but
are not all securely wiped. These constraints block promotion beyond the
single-controller disposable canary; they are not implicit production policy.

The qualifying experiment is one process-level sequence on nimo-1: absent
handle miss, cold generation, publish, process stop, new-process authenticated
hit, exact continuation equality, anchor corruption, new-process safe miss,
and exact cold recomputation equality. Broader corruption, storage-fault,
concurrency, restart-loop, retention, quota, reserve, multi-node, large-state,
and soak matrices remain explicit later gates. The filesystems demonstrated
synchronized publication and restart recovery in this experiment; L08i makes
no power-loss durability claim.

Rollback is to omit the compile gate or leave the runtime mode `off`, then
remove only the disposable canary roots. No donor code, GPL llama-ai code,
CachyLLama transplant, WebUI, dependency, remote, or deployment change is
admitted. The disposable operator key must be removed after qualification; raw
evidence retains only its SHA-256, never the key bytes.
