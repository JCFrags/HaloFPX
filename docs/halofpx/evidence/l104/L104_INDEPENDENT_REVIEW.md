# L104 independent source review

Verdict: **BLOCKER CONFIRMED**.

The reviewer independently verified:

- exact-key restore lookup occurs before slot launch;
- live scheduler/RPC topology facts exist only inside `process_ubatch` after
  graph build/allocation, census resolution, split binding, and authenticated
  preflight;
- public llama APIs have no non-executing request-plan entry point;
- low-level scheduler allocation alone cannot reproduce llama's exact graph
  and authority construction;
- the component manifest exists only after capture/restore storage
  materialization; and
- selecting a candidate using its own stored topology or copied configuration
  would be circular and unauthoritative.

The smallest safe correction is an immutable non-executing request-plan handle
which owns the exact graph allocation and live authority, selects the cache,
and is later consumed by the same decode/capture path. The reviewer classified
this as a new request-lifecycle seam beyond L104's three authorized
components. Stopping before source edits was required.
