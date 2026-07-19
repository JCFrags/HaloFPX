# L05r fake sticky-quarantine publication review v01

- Date: 2026-07-18
- Scope: fake-only operations 69 through 76 under ADR-0024
- Final independent verdict: **ACCEPT**

## Delivered boundary

The internal `STATIC EXCLUDE_FROM_ALL` registry-lab now models an exact
quarantine-publication witness, independently revalidates the operation-6
authority before consuming it, and executes a typed, self-verifying immutable
publication sequence through operations 69 through 76. A successful sequence
commits data before visibility, preserves no-replace behavior, and synchronizes
the directory. Failure, process death, retained staging/final names, capacity or
reserve loss, malformed state, and authenticated readback contradiction remain
miss-like, uncertain, or sticky-quarantined according to the frozen contract.

The publication path is allocation-free after admission. Sequence identifiers
are nonzero, globally unique within the fake authority, nonwrapping, and consumed
once. State-only restart projections do not consult the script, private witness,
private action commitment, or private scratch. Operation 74 cannot replace an
externally retained staging or final sentinel.

No public header, product link edge, filesystem syscall, provider, cache hit,
restore path, inference path, persistent write, HIP path, Vulkan path, WebUI
path, or distributed protocol entered this milestone.

## Independent adversarial review

The reviewer recomputed the operation/product matrices and inspected the exact
witness, operation-6 consumption, byte derivation, self-verification, effect
guards, restart projections, hostile readback attacks, writer isolation,
capacity/reserve boundaries, failure/death cases, secret wiping, and static
feature-off contract. The final review found no actionable issue.

The accepted evidence covers:

- 155 admitted and 805 rejected operation products;
- 497 restart projections, including 18 state-only failure/death frontiers;
- 22 independent state axes and 61 malformed-script rejections;
- six private fault points plus sequence-capacity exhaustion;
- exact capacity and reserve boundaries, late reserve loss, and writer death;
- four hostile readback attacks, including correctly retagged wrong semantics;
- retained staging/final names that prevent sequence issuance; and
- allocation failure, wipe, death, and secret-exclusion checks.

## Qualification

Windows passed the full 40-test HaloFPX Release label, the static contract, the
focused Release product and diagnosis modes, and the expanded Debug diagnosis
mode. The Windows context-store-authority CRT setup still routes assertion text
to stderr and suppresses ReportFault UI without converting a forced assertion
to success; the earlier forced-assertion receipt retains exit code 3.

nimo-1 completed a clean 574-target optimized Linux build and 39/39 HaloFPX
tests. It then completed 25/25 exhaustive product processes. nimo-2 completed a
clean 571-target ASan/UBSan build and 39/39 HaloFPX tests, followed by 18/18
exhaustive product processes. Eight nimo-2 pairs overlapped after a controller
reconnect; their unique timestamps and zero exits are retained as extra
concurrency stress rather than misreported as sequential trials. Sanitizers
halted on first error with leak detection enabled and reported none.

The known-good nimo-1 MiniMax server and nimo-2 RPC service retained their
original PIDs and remained healthy. Neither service was restarted or redeployed.

## Wiki, provenance, rollback, and performance

The slice agrees with canonical Wiki Section 63 and ADR-0024: invalid or
ambiguous state cannot become a hit, immutable publication separates file and
directory durability, and restart authority is derived only from retained
state. Concrete Linux filesystem/process-death qualification M63-01 remains
open and is not pre-claimed.

The implementation is target-native. No GPL llama-ai implementation or
separately licensed documentation entered the MIT engine, no CachyLlama unit was
copied, the direct-cherry-pick roster remains empty, and no P3 record is needed.

Rollback is a source-only revert of this fake target and its tests. Because the
target remains excluded and no persistent object or deployment exists, rollback
requires no cache migration, model change, or service action. Feature-off
behavior is still the compatibility and zero-regression control. The retained
CPU timings are qualification telemetry, not an inference-speed claim.

Exact source/executable hashes, commands, counts, raw timestamps, reference
identities, service state, nonclaims, and the overlapping-run disclosure are in
`../evidence/l05r-quarantine-publication-receipt.json`.

## Reusable follow-up

- After this commit exists, record its exact hash in canonical Wiki Section 63.
- Open the concrete Linux provider only after the M63-01 syscall, mount,
  process-death, and rollback matrix is explicit and independently reviewed.
- Preserve the optional quantized-KV FlashAttention lane for L14Q; it does not
  modify this persistence boundary.
