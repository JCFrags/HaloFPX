# L36 independent adversarial review

Verdict: **PASS for terminal NOT PROMOTED closeout**.

The independent reviewer accepted the focused admission correction before the
corrected transition:

- restore batch authority was separated from
  `SEMANTIC_DIAGNOSTICS_ONLY`, while capture remained 512;
- all remaining uses of `SEMANTIC_DIAGNOSTICS_ONLY` were limited to
  configuration, semantic-only equality policy, and component-diagnostic
  selection;
- 91 focused runner/controller tests and 19 subtests passed;
- the manifest matched child SHA-256
  `cfdd63650a73ffc0e5aa01de63dc98dc255a9087b20363270abd6e33c61e5130`;
  and
- manifest/hash/argv validation and exact `Popen` binding remained before key
  preparation and production shutdown.

The final evidence review found the corrected attempt non-admissible for a
different, source-backed reason. The restore result frees
`disposable_ctx` at `tests/test-halofpx-distributed-state-canary.cpp:978` and
then calls `llama_n_batch(run_ctx)` through its stale alias at line 985.
Attempt one happened to emit zero; attempt two emitted `3386108400`. Both are
invalid post-free result authority. The reviewer therefore rejected any cache,
model, replay, graph, scheduler, or computation root-cause conclusion.

The reviewer confirmed that the pre-free HMAC-bearing replay records were
structurally complete and equal except phase/HMAC, while synchronized logits
and tokens differed. Those are retained observations only because the runner
failed before its authenticated comparison and full state-window closeout.

Cleanup evidence removed every admitted L36 resource. Production-final evidence
proves worker-first system-unit recovery, exact cgroups/commands/listeners,
HTTP 200, and `NRestarts=0`. No transport timeout occurred. The terminal
closeout is accepted only with no third attempt, no result-authority fix under
L36, and no promotion or root-cause overclaim.
