# L91 offline guard diagnosis

Status: source/evidence-backed diagnosis after the single authorized runtime; no retry or host mutation.

The full L77 runner reached authenticated residency-A capture. `capture.log`
(SHA256 `07177cdefcfc8ca62780ff58ec8cf9fa38c79239b5730442012ea7a2153d3241`)
terminalized capture, retained `capture-suffix.txt` as `alpha`, created the capture
epoch receipt, and the controller harvested four authenticated server authority
files. The capture worker was then stopped and its absence was durably recorded
by `transient-unit-guard-005.json`.

The next intended operation is `start_worker(True, restore_unit, root)` at
`scripts/halofpx-l13-primary-retry.py:3224`. No restore-worker prelaunch record,
restore-worker launch, or restore canary launch exists. The durable error is
`transient unit guard authority is outside the closed manifest`
(`failure.txt`, SHA256
`c0a381f91944c80fb920afec176d7bfc17c95a1840f763ed3017e3058fc005b5`).
The subsequent restore-worker/device-gate/restore-canary records are absence
checks from the `finally` cleanup path, not launches.

The exact rejection is therefore local and precedes the fresh-residency-B host
operation. The rejecting source predicate is the tuple membership check in
`ensure_transient_unit_absent()` at lines 1182-1186. The runtime did not retain
the rejected canonical `(host, unit, port)` tuple or the installed authority set,
so the evidence cannot distinguish a tuple-construction mismatch from an
installed-environment mismatch. Current source and manifest would nominally
pair `("nimo-1", "halofpx-l48-worker-restore", 50248)`, which is admitted.
Claiming a more specific field mismatch would therefore exceed the retained
evidence.

Smallest future action, if separately authorized: add a bounded nonsecret
refusal receipt containing the rejected canonical tuple and the hash of the
installed canonical authority set, then exercise the closed path offline before
any host transition. No L91 semantic correction or retry is authorized.
