# L63 independent adversarial review

**Verdict:** accept a precise terminal `NOT PROMOTED`; no implementation PASS
and no pre-runtime readiness.

The source audit confirms four material prerequisites:

1. Current L44 derives `session.connection_epoch` from the mutable CAPS server
   nonce. The server tracks allocation ordinals/maps but exposes no distinct
   negotiated live-connection and allocation-topology epochs.
2. A recorder created after successful mutable begin cannot authenticate a
   begin refusal. Attempt authority must predate begin and terminate or bind.
3. `ggml_backend_sched_authority_admission` is non-consuming but not
   handle-bound, and returns the pre-prepare chain root until preparation.
4. The retained mutable-authority fixture enables one scheduler authority,
   reads admission before preparation, then reuses it across unrelated graph
   UIDs and execution sequences. It does not qualify one real composed
   L42-to-L44-to-compute identity.

The global mutable-session lock is also held over network paths, while the
existing response recorder has a process-global event sequence and begins too
late. Layering more logging on those boundaries would not prove attempt
isolation or honest transport cardinality.

ADR-0049 is a coherent future prerequisite. The partial candidate was
correctly removed, L61 harvesting remains unchanged, and no stories run was
authorized. Review accepts the closeout wording and the read-only production
reconciliation. It does not accept L63 as implemented or primary
preflight-ready.
