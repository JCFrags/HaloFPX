# L46 — real replay authority integration

Result: **NOT PROMOTED — FOUNDATION INTEGRATION BLOCKER**

Base: `fd1abb9ac79c0ef94308d874e24c758ecce0f0d4`

L46 stopped without a primary artifact access or production mutation. The
source audit found that the accepted L42 and L44 public contracts cannot
represent the real llama scheduler replay lifecycle without changing those
accepted foundation boundaries.

The MiniMax replay creates mutable inputs structurally, but the scheduler may
materialize backend-local copies only during graph splitting. L44 accepts only
an already RPC-resident tensor on the admitted socket and requires every graph
leaf to have an explicit role or exclusion before filtering out non-RPC
storage. Both role registration and exclusion reject non-RPC tensors, so the
real mixed RPC0/ROCm0 graph is not admissible through the accepted API. It also
has no source-to-scheduler-copy registration bridge, and dynamically created
views cannot all be classified at model-load call sites. Registering by names,
sizes, or `INPUT` traversal would violate L46.

Independently, L42 is enabled once per scheduler and fails after more than one
post-enable graph split. The real runner performs multiple prompt chunks before
the saved-boundary replay, and the llama execution API exposes no admitted
per-execution arm/finalize lifecycle around the scheduler-owned split,
materialization, and compute sequence. Enabling at scheduler creation therefore
cannot cover the frozen path; enabling late cannot reliably cover a reused
graph.

There is a second ordering mismatch. L44 snapshots the L42 admission root at
session begin and must commit before RPC compute. L42's authenticated final
split/copy transcript exists only after execution. The current public APIs
cannot bind that final scheduler result into the pre-compute L44 session.

Closing either gap requires a separately reviewed foundation extension:

1. revised L44 census semantics for mixed local/RPC leaves and structurally
   created nested views;
2. a pre-compute RPC split or source-to-copy admission bridge using L42
   canonical copy identity; and
3. a bounded re-arm/finalize/abort scheduler authority lifecycle whose final
   result can be bound to the same execution.

Those are changes to accepted L42/L44 authority, not runner binding. L46 did
not weaken the contracts, retain an integration candidate, run the disposable
lifecycle, or update the frozen primary runner/controller binding.

No correctness, performance, or cache-promotion claim is established.

Read-only closeout reconfirmed the unchanged system units: nimo-2 worker
PID `1535639`, port `50052`, `NRestarts=0`; nimo-1 coordinator PID `2356329`,
port `8081`, HTTP `200`, `NRestarts=0`.
