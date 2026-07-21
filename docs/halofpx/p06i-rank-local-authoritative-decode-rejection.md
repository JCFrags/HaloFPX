# P06i rank-local authoritative decode rejection

Status: **rejected and reverted; P06h remains the accepted boundary**

P06i tested the smallest graph-authority step after P06h. A fourth default-off
gate made the sum of the lower- and upper-96-expert branches authoritative for
one-token decode at layer 32. The existing router produced the exact logical
top-8 IDs and weights, while a routing-only return avoided the full local MoE
calculation for that decode step. Prompt prefill and P06h's full-local plus
physical-peer-half storage layout remained unchanged.

The candidate was target-native, isolated, and never deployed. It introduced
no persistent write, donor code, dependency, public option, or default change.
After the matched screen recorded materially adverse candidate-labeled
samples, all candidate runtime source was reverted. This commit retains only
the rejection record.

## Correctness and focused controls

Both nimo nodes built byte-identical Release binaries for `gfx1151` with HIP,
Vulkan, RPC, forced MMQ, no VMM, and WebUI disabled. Feature-off and locked L02
contracts passed 2/2 on each node. Direct-HIP and RPC Q6-view oracles retained
nonzero reference L2 `24.3547155`, NMSE 0, and maximum absolute error 0.

The exact 160 GB primary artifact loaded through the P06h physical upper-half
ranges. The first authoritative request returned HTTP 200 with 1,129 prompt
tokens, 128 generated tokens, and newline-terminated content SHA-256
`a9c38c7f948adcfa8cfab5468ab84cc089b01a34c3f270f1c487a9a5fa74b555`,
identical to the retained P06g/P06h control.

## Matched screen and rejection

The screen used one binary, exact model and request, identical runtime tuple,
and order control–candidate–control. Each block used one excluded warmup and
three retained requests. The six retained control samples bracketed the three
candidate samples. Every retained request returned HTTP 200, exact token
counts, and byte-identical decoded content.

| Metric | Control mean +/- sample SD | Candidate mean +/- sample SD | Candidate delta | Approx. normal 95% interval |
| --- | ---: | ---: | ---: | ---: |
| Prompt processing | 203.8873 +/- 0.0934 tok/s | 195.2435 +/- 0.4353 tok/s | -4.2395% | -9.1420 to -8.1455 tok/s |
| Generation | 16.66172 +/- 0.01738 tok/s | 16.17293 +/- 0.07198 tok/s | -2.9336% | -0.57143 to -0.40616 tok/s |
| End-to-end curl wall time | 13,245.46 +/- 37.92 ms | 13,700.79 +/- 47.14 ms | +3.4376% | +393.96 to +516.69 ms |

The point estimates and intervals are materially adverse, so the candidate was
conservatively rejected under the project's no-regression rule. The planned
duplicate candidate block was cut: the first candidate block plus the recovered
closing control already answered the retention question, and rescue trials
would only spend time explaining a candidate that cannot ship. The archived
journals preserve identical commands and the canary log proves the fourth gate
was functional, but transient unit metadata did not preserve an explicit
per-block environment snapshot. The screen therefore supports rejection and
revert, not a causal performance claim about the fourth gate. The unexpected
prompt regression remains uninvestigated.

## Boundary and reusable result

The P06i screen rules out promoting this candidate as recorded. Future MoE work
must retain explicit per-block environment evidence and should reduce remote
dispatch/combine cost and avoid computing inactive ownership slots before any
new authority candidate is opened.

The known-good nimo-2 worker and nimo-1 coordinator were restored worker-first
with their original binary hashes, zero restarts, the expected listener, and
HTTP 200. Raw source, builds, tests, requests, responses, timings, and journals
are preserved in verified mode-0600 node bundles. All immutable references
remain unchanged. Generation above 30 t/s remains a stretch objective, not a
claim or gate result.
