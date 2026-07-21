# P10 device-resident fused expert-partial rejection

Status: **rejected; candidate source removed**

P10 tested whether layer 32 of the pinned MiniMax-M2.7 Q6 ROCmFPX workload
could overlap the remote and local expert halves through a target-owned,
fully device-resident HIP partial operation. The default-off candidate used no
host ID readback or stream synchronization and computed only owned experts.

Fresh gfx1151 builds passed the feature-off and L02 contracts. Direct ROCm and
RPC synthetic tests matched the inherited expert-chain oracle exactly for both
ownership bases, including poisoned-output all-unowned zero cases. Invalid
metadata and expert IDs failed closed. An independent review required and then
accepted overflow-safe server-owned RPC view bounds before exact-model use.

The first exact request exposed a scheduler-placement defect: a local weight
view did not carry direct weight-buffer authority, so both marked partials were
placed in the RPC split. The assertion stopped execution before output. The
repair bound base 0 directly to the full local 192-expert allocation and left
base 96 on the physical peer half. The corrected exact request produced 1,129
prompt tokens and 128 generated tokens with decoded-content SHA-256
`3c9bcc5624f4a1d6558c51be89e2443ffba4b2206d10b632c89b7ef64fdd026f`,
identical to the locked control.

Performance nevertheless failed the zero-regression gate. Three retained
candidate requests averaged 195.113559 prompt tokens/s and 16.244181 generation
tokens/s. Three retained feature-off requests from the same binaries, model,
topology, request, and runtime tuple averaged 203.802007 and 16.667243 tokens/s.
The candidate deltas were -4.263181% prompt and -2.538287% generation. The
separation was large and consistent enough to reject without a redundant final
candidate block.

The fused op, wire commands, model routing, test executable, and ADR were
removed. Only the generic overflow-safe RPC view-bounds repair remains: it is
independent of P10, strengthens the already-retained P09 server-side view
authority, and changes no feature-off graph or protocol. No donor code,
dependency, persistent write, WebUI surface, model file, deployment binary, Git
remote, or immutable reference was changed.

Raw evidence is retained separately:

- nimo-1 bundle: `/var/tmp/halofpx-p10-fused-rejection-7486700-nimo1-20260720.tar.zst`
  (`0acab9138ca745b9d797de08daabfebaf49346c13f0ed19f33b6e0cde4d8fb11`)
- nimo-2 bundle: `/var/tmp/halofpx-p10-fused-rejection-7486700-nimo2-20260720.tar.zst`
  (`1bfd00040dd6ef2e7f9db5e9e9179196f62df497657d28e80fc132faa503bd1f`)

The known-good `minimax-m27-rpc-worker` then `minimax-m27-q6-server` rollback
was restored; afterward both services reported `NRestarts=0`. P10 is closed; another expert-overlap
attempt requires a materially different kernel/scheduling design.
