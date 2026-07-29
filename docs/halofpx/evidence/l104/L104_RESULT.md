# L104 terminal result

Status: **SEMANTIC BLOCKER — NO CANDIDATE SOURCE**.

L104 was authorized to add a live distributed topology authority, distributed
profile/codec, and typed cache transaction. Exact source inspection exposed a
different prerequisite: llama-server performs exact-key lookup before the
request lifecycle has produced any live scheduler/RPC authority.

No source edit, build, host access, model run, or production action occurred.

## Ordering proof

- `tools/server/server-context.cpp:2780-2785` invokes
  `halofpx_exact_key_restore_before_launch()` after selecting a slot but before
  `launch_slot_with_task()` at line 2801.
- The lookup helper at `server-context.cpp:1406` constructs the cache identity
  from task/static fields and calls the transformer restore path at line 1460.
  No request graph, storage projection, split plan, or live RPC preflight
  exists at this point.
- `src/llama-context.cpp:1571` is the internal `process_ubatch` seam. The exact
  graph is built at line 1674, allocated at line 1734, and scheduler authority
  is prepared at lines 1740-1741.
- Canonical storage/RPC census resolution occurs at lines 1751-1788. Split
  enumeration/binding occurs at lines 1839-1876.
- Authenticated non-mutating RPC preflight first produces key generation,
  client/server connection epochs, and allocation-topology epoch at lines
  1884-1906. Expected/prepared admission follows at lines 1913-1929.

The public llama API exposes decode/encode, not a request plan-only operation.
Although the low-level scheduler can allocate a caller-supplied graph without
executing it, it cannot construct llama's exact ubatch graph, roots/copies,
resolved census, split mapping, or authenticated preflight authority.

## Component-identity cycle

`llama_state_seq_storage` is populated only by state capture or restore
preparation. RPC component enumeration then derives the live/copy tensors and
component manifest. Capture, stage, and commit refuse unless that manifest
matches the supplied identity.

Therefore:

1. distributed exact-key lookup needs live topology and component authority;
2. that authority currently exists only after request graph/storage
   preparation; but
3. lookup currently precedes both; and
4. a cached candidate cannot safely supply the authority used to select and
   authenticate itself.

Using compatibility component 14, copied endpoint configuration, invented
ranks, or a stored candidate as authority is explicitly prohibited and would
repeat L103's defect. Moving lookup after an independently rebuilt graph would
also permit plan drift between lookup and execution.

## Smallest safe product correction

Add one non-executing request lifecycle seam before the three already
authorized L104 components:

1. `prepare_distributed_checkpoint_plan(request ubatch)` must run the same
   graph build, allocation, canonical census resolution, split binding, and
   authenticated non-mutating RPC preflight used by `process_ubatch`.
2. It returns an immutable typed handle owning the graph allocation and
   binding topology, epochs, key generation/channel identity, endpoints,
   split/census roots, and descriptor-level component manifest.
3. Exact-key derivation and lookup consume only that handle's digest.
4. A hit independently reconciles prepared restore storage before stage/apply.
5. A miss passes the same handle into decode and later capture; an independent
   graph rebuild is forbidden.
6. The handle cannot consume admission, set inputs, mutate tensor/model state,
   compute, or publish authority before the selected hit/miss transaction.

This is a request lifecycle/plan-ownership seam, not a harness. It was not one
of L104's three authorized implementation components. Lead authority is
required before changing server scheduling and graph-lifetime ownership.
