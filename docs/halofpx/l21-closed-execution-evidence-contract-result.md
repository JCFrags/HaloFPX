# L21 closed execution/evidence contract result

Date: 2026-07-23

Base: `e2edc4b3277f5385118e759ed9f89c1ea0a7445a`

Outcome: **PASS — TARGETED CONTRACT REPAIR ACCEPTED**

## Result

[VERIFIED] L21 replaces the rejected L20 candidate with an exact-schema
manifest owning every disposable host, port, unit, executable/hash, child argv,
source archive/root, build/state/key/evidence path, archive intermediate,
retained archive, and cleanup target. Unknown, missing, duplicate, swapped,
relative, nonprivate, inconsistent, or out-of-allowlist identities refuse
before unit or child mutation.

[VERIFIED] Remote argv is transported as one `shlex.join`-quoted command for
fish. Journal output must contain exactly one opaque cursor token. Transient
units start without `--collect`; PID, InvocationID, remote-host wall/monotonic
lower bounds, cursor, and start time are captured before waiting. Every
journal, allocation/refusal, disk, exit, evidence, archive/hash, and cleanup
step is mandatory.

[MEASURED] One disposable nimo-1 worker on port 50198 and nimo-2 child exercised
the real collector. The child requested 133,143,990,272 bytes against reported
total 133,143,986,176 bytes, received the worker allocator refusal, and exited
23. The final mode-0600 archive is
`/var/tmp/halofpx-l21-small-evidence-v9.tar.zst`, SHA-256
`b807e10a24813b9cc5178962aa4dfefbc513737482e86843e67d4b0273ab7f13`.
It embeds verified runtime cleanup and packaging-finalization records.

Thirteen focused tests passed, covering manifest closure, shell-safe argv, exact
and ambiguous cursor parsing, PID/InvocationID mismatch, abnormal exit,
timeout, mandatory evidence/path-probe failure, archive-failure retention, and
cleanup failure. The accepted L20 three-residency lifecycle was not repeated or
broadened.

## Preserved fail-closed evidence

No archive was overwritten. The retained sequence is:

- `failed-v1` `42714399ca9659111aafa482e301a2ae02fa85da22c8e9e724fc4df69e11f17f`
- `failed-v2` `f45137a1fda53ba4f764adcba035b62984d3e2f5c69ccdb082f5f30a0fce6690`
- `failed-v3` `dde37577bcd22f312d859a36e4379efa315cfcc1af69471aa1e0365240f1c436`
- `failed-v4` `7b4b3d2253481fda4ee82319ff8702f7b591838cf078eec62866bb803adcc351`
- `failed-v5` `a9347cd693aea057250b9e867e2f66718db26a565a8baba7bc0f02f7665845cf`
- review-rejected `v6`
  `f704f131c7ba6a25848b040a0c641a362b289806f002e4811f66606c0d7799b8`
- `failed-v7` `31f2d0e2436a2115559486532c58cd3879824cbd77024eaf67c9937f6a654d24`
- review-rejected `v8`
  `c4824657411045351648d4857efb6ca078b8e58b638872471a4142da243ea867`
- accepted `v9` `b807e10a24813b9cc5178962aa4dfefbc513737482e86843e67d4b0273ab7f13`

Each pre-v9 result remained non-promotable and entered the same bounded
cleanup path.

## Boundary and closeout

[MEASURED] Production remained continuously active and closed unchanged:
nimo-1 coordinator PID 2144857, port 8081, HTTP 200; nimo-2 worker PID
1305879, port 50052; both `NRestarts=0` with original start times. All L21 units
are inactive, port 50198 is closed, and every mutable manifest path is absent.
Only the nine manifest-owned mode-0600 archives remain.

L21 did not access the primary artifact, provision production keys, stop or
restart production, run performance tests, promote cache behavior, repeat the
lifecycle matrix, or open L22.
