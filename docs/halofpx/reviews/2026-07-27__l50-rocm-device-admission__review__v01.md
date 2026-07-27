# L50 independent adversarial review

Status: **ACCEPT terminal NOT PROMOTED**

The reviewer accepted the recovered L49 cause and the minimal HIP/gfx1151
correction, frozen identities, real no-model ROCm0/HFXCAP2 admission, and
focused 54/54 tests as sufficient pre-runtime authority for the sole run.

The runtime evidence reaches only worker/device admission. The capture
coordinator exits during warmup with graph-compute/process-ubatch/decode
errors, before prompt processing, capture, composed authority, logits, or token
evidence. The reviewer therefore accepts only a warmup scheduler/compute
failure observation, not a composition, cache, or root-cause claim.

The reviewer independently identifies a separate material evidence defect:
`systemd-run --wait --pipe` did not emit the InvocationID required immediately
by the harness. Post-failure systemd authority recovered invocation
`a1b165d2401b4483946c0e4857da545b`, ExecMainPID `1673926`,
ExecMainCode `1`, ExecMainStatus `4`, and Result `exit-code`; the bounded SSH
record, not the empty invocation-filtered journal, is the complete failure
output authority. The initial fish cursor-quoting failure is retained and is
not the runtime cause.

The reviewer verified all five disposable units absent, ports 50248/50249
closed, keys and roots absent, and byte-identical production snapshots at
SHA-256
`511f05d2b638277e19ae7af44573eb9e244d829458f24cdefc46e3f88ffd6ded`.
Verdict: remove the rejected runtime candidate, retain all raw evidence, make
no repeat, and close L50 NOT PROMOTED.
