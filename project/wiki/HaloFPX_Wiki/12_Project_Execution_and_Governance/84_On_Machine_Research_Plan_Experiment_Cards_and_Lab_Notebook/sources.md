---
section_id: "84"
title: "On-Machine Research Sources"
status: "needs-machine-validation"
last_verified: "2026-07-17"
applies_to:
  repositories: ["Custom_Inference_Project", "mlcommons/inference_policies@c547732b539cb3a14cc5680597714c8c1df4cad0", "richardcochran/linuxptp@e312b959a7dd3e8db0c6c8e282917c89909f8f76", "torvalds/linux@fce2dfa773ced15f27dd27cd0b482a7473cdcf2a"]
  software_versions: ["JCGM 100:2008", "RFC 3339", "RFC 8633", "W3C PROV-O 2013-04-30"]
  hardware_revisions: []
related_sections: ["02", "03", "04", "05", "73", "75", "81", "85"]
---

# On-machine research sources

Access date for all Internet sources: 2026-07-17. Current remote heads were resolved on that date; future runs must re-pin volatile repositories.

| ID | Primary authority and revision | Claims supported | Limitations/conflicts |
|---|---|---|---|
| `S84-01` | Local [`AGENTS.md`](../../../../AGENTS.md), observed 2026-07-17 | evidence route, claim labels, raw-source preservation, closeout review | project governance, not measurement proof |
| `S84-02` | Canonical Agent Harness `C:\Users\britt\Documents\Agent_Harness\AGENTS.md` and `guide/architecture.md`, observed 2026-07-17; routed by [`references/agent-harness.md`](../../../../references/agent-harness.md) | evidence promotion, reversibility, raw preservation, review loop | conceptual governance authority; project rules take precedence |
| `S84-03` | [Section 03 stable identifiers](../../01_Wiki_Governance/03_Glossary_Naming_and_Stable_Identifiers/design_implications.md), verified 2026-07-16 | experiment/run/ADR identifier namespaces and host/rank/link naming | local recommendation; not yet machine exercised |
| `S84-04` | [Section 73 benchmark contract](../../11_Verification_and_Performance/73_Benchmark_Methodology_Terminology_Experimental_Controls_and_Data_Schema/README.md), verified 2026-07-16 | metric scope, controls, raw records, uncertainty, no measurements present | proposed schema/methodology; requires implementation |
| `S84-05` | [MLCommons MLPerf Inference rules](https://github.com/mlcommons/inference_policies/blob/c547732b539cb3a14cc5680597714c8c1df4cad0/inference_rules.adoc), commit `c547732b539cb3a14cc5680597714c8c1df4cad0`, 2026-07-07 | replicability, consistent SUT/framework, fixed randomness, shared implementation | MLPerf rules are not HaloFPX conformance or sample thresholds |
| `S84-06` | [JCGM 100:2008, Evaluation of measurement data - Guide to the expression of uncertainty in measurement](https://www.bipm.org/documents/20126/2071204/JCGM_100_2008_E.pdf), version 1.10 file served by BIPM; document 2008 with 2010 corrections | measurement-result uncertainty and evaluation categories | general metrology guide; HaloFPX must choose fit-for-purpose statistical methods |
| `S84-07` | [RFC 3339, Date and Time on the Internet: Timestamps](https://www.rfc-editor.org/rfc/rfc3339.html), July 2002 | UTC timestamp representation and offset syntax | wall-time format, not monotonic clock or sync-error bound |
| `S84-08` | [Linux timekeeping documentation source](https://github.com/torvalds/linux/blob/fce2dfa773ced15f27dd27cd0b482a7473cdcf2a/Documentation/core-api/timekeeping.rst), Linux commit `fce2dfa773ced15f27dd27cd0b482a7473cdcf2a`, 2026-07-16 | monotonic versus wall-clock timing boundary | kernel API guidance; actual available clocks and behavior must be probed |
| `S84-09` | [RFC 8633, Network Time Protocol Best Current Practices](https://www.rfc-editor.org/rfc/rfc8633.html), BCP 223, July 2019 | synchronization monitoring, diverse sources, leap/security operational constraints | NTP synchronization status alone does not quantify experiment error |
| `S84-10` | [linuxptp `ptp4l`, `phc2sys`, and `pmc`](https://github.com/richardcochran/linuxptp/tree/e312b959a7dd3e8db0c6c8e282917c89909f8f76), commit `e312b959a7dd3e8db0c6c8e282917c89909f8f76`, 2026-06-15 | candidate PTP/PHC synchronization and observation tools | USB4 interface timestamp support and achieved uncertainty are unverified |
| `S84-11` | [W3C PROV-O](https://www.w3.org/TR/2013/REC-prov-o-20130430/), W3C Recommendation, 2013-04-30 | entity/activity/agent and derivation provenance relationships | HaloFPX uses the concepts; full PROV-O implementation is not required |

## Internal input inventory

The machine-experiment registries and procedures in completed Sections 18-81 were inspected and consolidated. Those pages own technical detail and acceptance rules; Section 84 owns cross-section ordering, card completeness, synchronized notebook structure, and promotion gates. An upstream list remains traceable through the source section IDs on E01-E10 rather than being copied as a second conflicting registry.

## Source conflicts and freshness

- **[VERIFIED]** RFC 3339 timestamps, monotonic clocks, NTP synchronization, and PTP hardware clocks solve different parts of timing. They must not be treated as interchangeable [S84-07][S84-08][S84-09][S84-10].
- **[INFERENCE]** MLCommons replicability rules and JCGM uncertainty principles align with HaloFPX's evidence needs, but neither supplies project-specific pass thresholds [S84-05][S84-06].
- **[OPEN]** Repository heads, kernel docs, drivers, and tool behavior are volatile. Section 85 must trigger re-review when pinned revisions or machine software change.
