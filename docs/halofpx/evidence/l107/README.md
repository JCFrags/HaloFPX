# L107 dedicated shadow-context product mode

Status: **NOT PROMOTED — pre-runtime review failure**

Base and inspected HEAD: `e15d6da0de55c0f1a604614db62b5d50957b40e3`

No candidate source is retained. No host, model, production, protocol, cache, or
runtime action occurred.

## Confirmed feasible lifecycle

Source review established that a dedicated shadow-context mode is feasible
without a new wire protocol only when two authorities remain distinct:

- persisted stable checkpoint identity: logical plan/topology/placement,
  component manifest, key generation, and channel binding;
- live attempt freshness: connection identity and allocation-topology epoch
  negotiated after all shadow allocations.

RPC buffer allocation and free advance `allocation_topology_epoch`
(`ggml/src/ggml-rpc/ggml-rpc.cpp:6121-6162`, `:6207-6228`). Consequently the
required ordering is:

1. quiesce and retain the old single-slot context;
2. construct the shadow completely, including all RPC/KV/compute allocations;
3. authenticate/reconcile the candidate using stable logical authority;
4. freeze the exact shadow ubatch/plan;
5. run preflight only after allocations and bind the resulting live epochs;
6. stage/commit state, atomically transfer server ownership, execute and
   terminalize with the same plan;
7. retain the old context until terminal; destroy it only afterward, accepting
   that its frees invalidate future attempts and require a new preflight.

Persisting an old socket or allocation epoch in the cache key, allocating after
admission, or destroying the old context before terminal is a P1 defect.

## Review failure

The initial mechanical seam demonstrated that server context ownership could be
made movable and that a new mode could refuse multi-slot/speculative/recurrent/
hybrid/ISWA configurations. It was removed before retention because it did not
yet include the indispensable frozen-plan transaction, live topology producer,
distributed profile/codec, or staged ownership swap.

Retaining a selectable mode at that point would be misleading or unsafe:

- leaving it outside task eligibility would be unreachable scaffolding rather
  than a product slice;
- adding it to current exact-key eligibility would reuse hard-coded world1/rank0
  key/profile authority and the live in-place restore path, contradicting L107.

The exact pre-runtime review therefore cannot pass. No two-host or Stories15M
gate was eligible to run.

## Smallest continuation

Continue as one implementation only when the source includes, before exposing
the mode:

- a frozen single-use shadow request plan and live topology authority;
- distributed manifest-v1 profile/codec with stable candidate identity;
- staged remote/local transaction and quiescent server ownership transfer;
- retained old-context lifetime through terminal and post-commit cold-recovery;
- exact resource-headroom refusal and focused lifecycle tests.

