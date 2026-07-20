# P03 dual-rail bottleneck canary independent review

Date: 2026-07-20
Disposition: **ACCEPT**

## Scope

Independent review covered the P03 dual-rail canary report and receipt for arithmetic, matched-tuple claims, provenance, retained-evidence integrity, rollback, and claim calibration.

## Findings

No blocking finding remains. Both final manifests validate every retained file; bundle sizes and hashes match the receipt; exact transient commands are retained in the journald exports; both aborts and the coordinator core disposition are recorded; and the restored known-good services reconcile by binary hash, health, restart count, and two-subflow topology.

The rail deltas, request timings, content hash, and comparison to P02 were independently checked. The evidence supports deprioritizing a larger dual-rail-only matrix because the single canary showed no obvious generation signal. It does not establish a causal claim that a second rail has zero benefit.

One nonblocking wording correction was applied before promotion: the report now says that no **persistent** MPTCP, firewall, or system transport configuration changed, matching the receipt and the temporary listener change used by the canary.

## Promotion decision

P03 is accepted as a bounded bottleneck canary. It preserves the known-good rollback deployment and does not enable or promote a transport change.
