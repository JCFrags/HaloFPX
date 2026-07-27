# L47 — foundation composition result

Result: **NOT PROMOTED — RUNNER/CONTROLLER EVIDENCE BINDING BLOCKER**

Base: `63eb8a415a40157a7a94a99ad30aa1ee1e2cbc25`

L47 froze ADR-0048 and implemented a candidate composition of the accepted
L40 RPC graph, L42 scheduler, and L44 mutable-session authorities. The
candidate qualified the difficult runtime mechanics on the disposable
stories15M path, but it did not meet the terminal acceptance boundary because
the closed primary runner/controller was not changed to enable, authenticate,
retain, and require the composed result. The unaccepted source candidate was
therefore removed before closeout.

## Qualified but rejected candidate evidence

The exact disposable model was
`stories15M-q4_0.gguf`, 19,077,344 bytes, SHA-256
`66967fbece6dbe97886593fdbb73589584927e29119ec31f08090732d1861739`.
The 1,129-token prompt SHA-256 was
`326cb4971a99fe8588fffc8635f57144173a16295a2375c3e1eb28240182f81d`.
The run used F16 K/V and flash attention off solely for the accepted
disposable lifetime fixture.

The final capture decoded the prompt in three chunks with a maximum chunk of
512, saved boundary 1,128, and produced token `4245`. After terminating the
first coordinator, restarting the disposable worker from PID `2392463` to
PID `2392519`, and loading a fresh second coordinator, restore produced the
same token `4245`.

The canary reported 578 worker components totaling 2,598,912 logical bytes
at capture and restore. Coordinator control, local, and manifest digests were
equal across capture and restore:

- control: `74994f73a5b5e972d9a57cf897a8869871fe83844b35909d51f57ae07de196e4`
- local: `9916ba346c6dd489ee7294dde4b742d479fcc93ba8aab9f437e24946716659c9`
- manifest: `bbbb48e17bd687679cf83c5d77dc6e72dd5895eb1040ea9dfe79d6934b7e9504`

Both logs also contain a failed diagnostic warmup graph compute before the
successful admitted execution. The candidate did not provide a clean
feature-enabled warmup contract, which is an additional reason this evidence
cannot establish primary-path readiness.

The authenticated composed record covered prompt execution sequences
`1,2,3`, capture replay sequence `4`, and fresh-residency restore replay
sequence `1`. Each execution reported a prepared scheduler root, final
scheduler root/tag, exact local/RPC census, and a committed L44 receipt.
The replay executions reported 53 local and 36 RPC roots, 7 ordered mutable
SET receipts, and a 29-entry RPC census. No legacy `GET_TENSOR` or
`SET_TENSOR` appeared in the worker state capture/restore windows.

Focused L44 tests passed 18 self-tests, six real-handler refusal cases,
concurrent-session isolation, omitted-leaf refusal without compute,
compute/recompute exactness, mutation sensitivity, and a real SET_HASH miss.
Focused scheduler tests passed 17 self-tests, feature-off inertness,
composition ordering/refusals, exact ordinary and expert transcripts,
tamper/order/duplicate/unknown/overlap/bounds/malformed refusals, one ordinary
copy, and two expert partial ranges.

One admission attempt omitted the required restore-authorized handoff and
timed out with result code 15. It is preserved separately as rejected harness
evidence and was not used as qualification evidence.

## Terminal blocker

The manual disposable runner emitted a valid composed result, but
`scripts/halofpx-l13-primary-retry.py` and its closed controller manifest
remained unchanged. They cannot activate the L42/L44 composition securely,
cannot require the per-execution prepared/final identities, cannot verify the
composed HMAC, and cannot retain its roots/counts/statuses as mandatory
controller-owned evidence. A manual log is not an executable pre-mutation
authority for the future primary path.

Adding an environment-only or ad hoc shell-key path would also violate the
closed manifest-to-Popen and secret-handling boundaries. Completing this
requires a separately reviewed runner/controller binding with an
argv-safe protected key-file authority and exact result verification. L47 did
not infer permission for that unreviewed expansion.

No primary artifact was accessed and production was never mutated. No cache,
correctness, or performance claim is promoted.

Raw evidence is retained under `docs/halofpx/evidence/l47-raw/`; the manifest
is `SHA256SUMS`, SHA-256
`84335955b6c4f282129ade2d6e6aa6cca40f1731dfbdbf8d6d6a70ab6e0f64e9`.
