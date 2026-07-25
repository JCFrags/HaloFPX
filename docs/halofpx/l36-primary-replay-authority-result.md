# L36 exact-primary replay-authority discriminator result

Date: 2026-07-25

Base: `93c3ae313b86aa0bfddd2c5a1a8745223cb256ac`

Outcome: **NOT PROMOTED — RESULT AUTHORITY INVALID**

L36 authorized one ordinary two-residency exact-primary discriminator and,
after an admission-only failure, one narrowly corrected execution. Neither run
is an admissible replay-authority discriminator because the restore result
reads context metadata after freeing the context. L36 made no cache, model,
graph, scheduler, or computation root-cause classification and made no semantic
correction.

## Frozen execution

[VERIFIED] Both executions used the exact 159,873,097,824-byte artifact with
SHA-256
`96506ada918e60ca9a9cfde8a5437790e4453401a6a3e236e3f55e7bac3aaea6`,
the frozen 1,129-token prompt and 1,128-token boundary, one generated token,
Q8_0 K/V, flash attention on, context 4096, batch and ubatch 512, seed 1234,
temperature zero, explicit `RPC0,ROCm0`, layer split, tensor split `1,1`, two
fresh model residencies, and the accepted L25/L28/L30/L32/L34/L35 authority.
The controller manifest bound child SHA-256
`cfdd63650a73ffc0e5aa01de63dc98dc255a9087b20363270abd6e33c61e5130`
and the exact `--l36-primary` argv before mutation.

## Two rejected result-authority attempts

[MEASURED] The first immutable attempt,
`l36-run-20260725T003800Z`, completed capture and restore but reported restore
`n_batch=0`. The then-current admission incorrectly expected 512 when semantic,
replay, component, and live-recapture diagnostics were combined, so it rejected
the result before authenticating and comparing the retained replay records.
Its evidence-tree hash-list digest is
`c50c29d07e2ce2382a3d73113c3944e0e1b4f02b13f7842346e6699c21f89675`.
This attempt is classified only as rejected admission.

[VERIFIED] The narrow correction derived batch authority from lifecycle mode
instead of `SEMANTIC_DIAGNOSTICS_ONLY`: capture remained 512 and restore
required zero. Focused qualification passed 91 tests and 19 subtests, including
capture 512 / restore zero across the admitted diagnostic combinations and
refusal of capture zero / restore 512. Independent pre-mutation review accepted
the correction and exact manifest-to-`Popen` binding.

[MEASURED] The corrected immutable attempt,
`l36-corrected-run-20260725T005400Z`, again completed capture and restore, but
reported restore `n_batch=3386108400`; the corrected admission failed closed.
Capture remained authoritative at `n_batch=512`, three prompt chunks, and
maximum prompt chunk 512. Its evidence-tree hash-list digest is
`882bf85182c44c293de8c2155b287531d0229b5215a619b7f1ff38dc777b0ee6`.
No third execution was made.

## Source-backed result-authority defect

[VERIFIED] In
`tests/test-halofpx-distributed-state-canary.cpp`, `run_ctx` aliases the
fresh restore `disposable_ctx`. Line 978 frees `disposable_ctx`; the result
print then evaluates `llama_n_batch(run_ctx)` at line 985. The observed zero
and `3386108400` values are therefore post-free result reporting and are not
valid lifecycle measurements. This is a harness result-authority lifetime
defect. It was preserved but not fixed under L36's no-repeat closeout.

This finding does **not** show that capture, restore, or the model failed. It
shows that the acceptance record used to admit the discriminator is invalid.

## Retained pre-free observations

[MEASURED] In the corrected attempt, residency A used coordinator PID 1533918
and worker PID 2355906 / InvocationID
`ecd76efa673b4d88b7a23d3c65fa3108`. Residency B used coordinator PID 1534028
and distinct worker PID 2356039 / InvocationID
`6afe3f313236412787dec828032a377a`. The coordinator terminated A before
stopping worker A. Both workers passed exact HFXCAP2 admission.

[MEASURED] Capture produced object
`63d7e9ed11a22df289bcf71269218f5aae1b76b79f239df36b721af8613029a7`
and token 21549. Restore produced token 9283. Before the invalid result print,
the emitted HMAC-bearing semantic records each replayed token 32 exactly once
at positions 1127 to 1128 over 200,064 logits. Capture logits SHA-256 was
`8564aef91899f6d5cc61ad88a8df4c836600a1006f1bc03b6eb6150e8c27c754`;
restore was
`7a8807f402dfca9a309b3fb30f504c973439ec2fabe29a77fd1dd19a436d36cd`.
Argmax and sampled tokens were respectively 21549 and 9283.

[MEASURED] The two replay-authority records contain 274 segments and are equal
apart from phase and phase-bound HMAC: graph reuse false, scheduler reset true,
4,719 nodes, ordered RPC0/ROCm0/CPU backends, ROCm0 logits backend, flash
attention enabled, output mapping, KV prepare/apply cell 1128, heads
1128-to-1129, `n_kv=1280`, positions/sequence IDs, and every recorded K/V base
tensor and attention view. The live coordinator receipt reports boundary 1128,
64 worker components / 2,454,528 bytes, and control/local/manifest digests
`2d614e8634f7f9defc4ed59f59b900490e021e382045c566109588bd288a0cbb`,
`7117319f7dc2b848d3ce3b35469aee3c62bc93a8262f88cb53949e7bbc5ceaca`,
and `7ad364fb0a047ca8db745c439bf8ad3f6fe6b92ae56f7b9f632caa88e47c69b3`.

These are retained observations only. Because the result-authority gate failed
before the runner's authenticated comparison and state-window closeout,
L36 does not invoke its interpretation branch, does not claim zero legacy
GET/SET for the full attempt, and does not localize the token divergence.

## Recovery, cleanup, and evidence

[VERIFIED] The corrected controller retained 622 bounded transport records and
the child retained 230; neither contains a timeout. Cleanup removed every
admitted L36 transient unit, port 50236, key, state/evidence/coordinator/
rendezvous root, source archive, and remote build tree. Follow-up checks found
all transient units not found/inactive and all admitted paths absent.

[VERIFIED] Production recovered worker-first. Nimo-2's system unit
`minimax-m27-rpc-worker.service` is active/running in
`/system.slice/minimax-m27-rpc-worker.service`, PID 1535639, exact command and
listener 50052, `NRestarts=0`. Nimo-1's system unit
`minimax-m27-q6-server.service` is active/running in
`/system.slice/minimax-m27-q6-server.service`, PID 2356329, exact standard
UD-Q6 command and listener 8081, HTTP 200, `NRestarts=0`.

The immutable evidence root is
`C:\Users\britt\Documents\Custom_Inference_Project\sources\halofpx\l36-primary-20260725`.
Excluding its self-describing manifest, it contains 49 files / 2,567,151 bytes
with canonical relative-path-plus-NUL-plus-content SHA-256
`e61d4c006f908deb56d67dfbd7b5f6d225e8b37c4c9537bd72ded77427baf437`.

## Boundary

L36 is terminal NOT PROMOTED. It does not authorize a third attempt, the
result-authority fix, another primary run, cache promotion, production cache
enablement, tuning, a performance claim, or L37.
