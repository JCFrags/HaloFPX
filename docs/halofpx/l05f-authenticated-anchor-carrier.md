# L05f authenticated-anchor carrier reconciliation

Status: accepted offline after independent adversarial review.

L05f removes the provisional publication-anchor structure and makes the
coordinator/simulator operate on the exact owned carrier produced by the L05e
codec. The carrier cannot be aggregate-initialized or populated by callers,
owns its canonical bytes across caller-buffer lifetime, and exposes body/key/
digest accessors only when authenticated.

Ordinary publication validates the closed transition before any backend call.
The backend read must return a valid carrier, and both the initial stale check
and final CAS compare the full canonical envelope. A separately authenticated
anchor-key tuple change is therefore stale/administrative even when the body is
unchanged. Manifest verification remains bound to the next selected-manifest
digest. Absent protected state is `bootstrap_required`, never an implicit
generation-one write.

The private carrier includes a domain-separated HMAC commitment to its derived
anchor key. Constant-time predecessor/next comparison rejects the same declared
key tuple signed with a different master. This is continuity metadata only;
the protected key registry remains a later gate.

The existing 23-step lifecycle, exact attempt binding, abandonment, ambiguity
fencing, durable close, live/durable simulator, and crash projections remain in
place. Tests additionally cover default/invalid carriers, owned-byte lifetime,
16-byte UUID and nullable predecessor mapping, every authority/domain field,
anchor-authentication key ID/generation, old/skipped/overflow transitions,
absent bootstrap, manifest mismatch, stale exact-envelope CAS, and distinct-
fence interleaving.

No concrete backend, filesystem, persistent key authority, administrative
bootstrap, server integration, or node operation is authorized by this slice.

The clean Windows CPU/WebUI-off Release build passed all 16 HaloFPX CTests and
the seven focused inherited regressions. The frozen anchor, coordinator, and
simulator executables each passed 100 independent process runs; the simulator
therefore repeated 147,200 core failpoint/crash scenarios. The first
independent review found a same-declared-tuple/different-master continuity gap;
the private authority commitment and pre-backend rejection test closed it, and
the re-review returned ACCEPT. Exact hashes and repetition counts are retained
in `evidence/l05f-authenticated-carrier-repeat-receipt.json`.
