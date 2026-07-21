# L10c exact-key operational canary independent review

Date: 2026-07-20

Verdict: **ACCEPT for the narrow default-off generation-one canary**

Production persistence: **still closed**
Blocking findings: **none**

## Scope reviewed

I reviewed the uncommitted L10c source and tests against `AGENTS.md`,
ADR-0037, L08i, L09, L10a, L10b, and the canonical Wiki requirements for
authenticated private scope, complete compatibility binding, corruption as a
miss, atomic bounded publication, evidence retention, and matched benchmark
controls. I also inspected the retained nimo-2 process bundle and reran the
seven focused ON-build tests.

The implementation is target-native. I found no donor implementation, GPL
`llama-ai` code, CachyLLama transplant, new dependency, WebUI change, remote,
or reference-clone mutation in this milestone.

## Findings

### Eligibility and authority

The automatic path is admitted only by the new Linux-only
`HALOFPX_CONTEXT_STORE_EXACT_KEY_CANARY` build gate plus the distinct
`full-v1-exact-key-canary` runtime mode. The build gate defaults OFF and
requires all four earlier context-store gates. Startup still requires at least
one configured API key and the established full-v1 roots, key, compatibility
components, UUID, quota, reserve, and one-entry limit.

Only a native, nonstreaming `/completion` request with one nonempty canonical
token sequence, one completion, no explicit slot, files, LoRA, stop strings,
structured response fields, speculative mode, parser/tool/reasoning behavior,
or stateful sampler is eligible. Authentication is checked against the
server-admitted API keys before private scope resolution. The raw credential is
wiped; the task carries only opaque scope, exact-session, and compatibility
digests. The exact-session resolver binds canonical fixed-width tokens, the
private scope, complete compatibility root, prompt/output boundary, admitted
profile, and explicit single-rank topology authority.

Parent/child tasks clear the carrier. Its lifetime then follows the moved
`server_task` owned by the selected slot. No carrier field appears in response
serialization, `/slots`, or ordinary logs.

### Restore, writeback, and fallback

Anchor-first `restore_selected()` remains the only discovery path: there is no
manifest scan, filename trust, prefix match, shared reuse, or caller-provided
handle. The existing authenticated anchor, manifest, compatibility, object,
lineage, expected-token, and complete-state checks all remain in force.

Restore is attempted only on an empty selected slot before task launch. A
nonempty ordinary slot cache is left intact, so this canary does not damage or
replace existing in-memory LCP behavior. Only a clean `miss_not_found` arms a
single writeback attempt. Corrupt, incompatible, ambiguous, busy, or otherwise
failed reads cold-compute and are not overwritten.

Publication occurs only after successful prompt decode and before sampling.
Capture and publication are best-effort; failure leaves inference authoritative.
After a failed live restore, `prompt_clear(false)` removes target and draft
sequence state before cold computation, preventing partially applied state from
being reused. Captured/restored state bytes are wiped after use.

The adapter's non-reentrant try-lock serializes publish and both explicit and
automatic restore paths. Contention returns `busy`; it does not block the
server controller or accept state. The explicit-handle runtime mode and route
remain separately selected and continue using the same authenticated adapter.

### Focused correctness and process proof

The independent nimo-2 rerun passed 7/7:

- feature-off CLI contract;
- private-scope authority;
- exact-session resolver and graph contract;
- exact-key runtime contract;
- independent exact-session golden; and
- authenticated anchor-selection restore.

The retained process bundle
`/var/tmp/halofpx-l10c-evidence-20260720-v1.tar.zst` has SHA-256
`a204d5988f9bd89728ff3e3e2fb257f7735efcfeafbad9374938a9dd57720851`.
Its fresh-process results prove:

| Case | Prompt tokens evaluated | Result |
|---|---:|---|
| initial exact request | 11 | cold computation and publication |
| authenticated restart | 1 | exact restore; continuation bytes/tokens equal cold oracle |
| one-token-different request, fresh process | 12 | cold recomputation; generation one retained |
| original request, another fresh process | 1 | retained authenticated hit |
| reserve-exhausted cold/restart | 11 / 11 | cold recomputation; no manifest, object, or anchor publication |

Cold/hit content SHA-256 is
`d4befa4c08b0bdd9023bfa965064be3c3a8eda8804174fc74c286b2a66710860`;
the returned-token SHA-256 is
`a28fcc7c48016b1b66b98e17c24d72563bfa470ee80452272320139dd87664c0`.
The retained disclosure scan contains no forbidden key, scope, manifest, or
anchor identifier.

## Matched feature-off comparison

Because the retained d7950c4 OFF server and the L10c OFF server had different
binary hashes despite equal sizes, I first compared their ELF and compilation
content rather than treating a rebuild hash as behavioral evidence. Raw
`.text`, `.rodata`, `.data`, relocation, unwind, and build-ID hashes differ,
but the differences are accounted for by nonoperational source-layout inputs:

- feature-off preprocessing of `common/arg.cpp` and
  `tools/server/server-context.cpp` is byte-identical after normalizing only
  `GGML_ASSERT` source line numbers; the respective common hashes are
  `09c97985472dfba853534cef85678b29abfebebdcc2d34f490659c84a8fbea13`
  and `81d1bdfce3f077d8704df49e1b0b2feb7feed942e48eccbea1d66f17e82b3737`;
- both disassemblies contain the same 2,310 function labels and the same
  370,415-instruction mnemonic sequence, with common SHA-256
  `3cd4a21770c1e55ae2039a12ecae236b5edd21b9fecb5929af53fac639b79cce`;
- embedded CSS is byte-identical; `loading_html` differs only by twelve CRLF
  bytes versus LF; and the JavaScript size difference is eight literal CR
  bytes in four multiline UI strings. The WebUI is disabled in the matched
  command. Those data-size shifts change addresses, relocations, unwind data,
  and the build ID without changing feature-off executable logic; and
- the exact-key macro, runtime-mode string, and exact-session domain are absent
  from the OFF executable, and the exact-session library is not linked.

I nevertheless extended the fresh-process CPU comparison on nimo-2 using the
same TinyLlama 15M Q4_0 model, command line, prompt, 64-token greedy request,
and libraries. The final schedule contains one warmup per binary and 30
retained pairs, balanced old-to-new and new-to-old. All 60 retained responses
have one identical content hash and one identical token hash.

Prompt duration is too short for stable unpaired arithmetic: individual paired
percentage differences range from -58.43% to +233.07%. The matched paired
point estimate is nevertheless positive for L10c at +2.35%, with a noise-wide
normal 95% interval of -17.28% to +21.99%. Generation is also positive: raw
means are 2393.61 and 2407.66 tokens/s (L10c +0.59%), and the matched paired
point estimate is +0.75% with a normal 95% interval of -0.94% to +2.43%.

The paired point estimates therefore do not show a slowdown, while the
code-equivalence analysis proves that the broad confidence intervals arise
from a deliberately tiny, short-duration workload rather than additional
feature-off instructions. This is accepted as feature-off non-regression for
L10c. It is not substituted for the later primary-model strict statistical
zero-regression gate.

Raw comparison evidence is retained at
`/var/tmp/halofpx-l10c-off-ab-review-20260720.tar.zst`, SHA-256
`5e2b8f35ef12c538ee37c5b4d105378536ae56b9d4508271aef2a01c514bbae3`.

## Verdict and remaining boundary

L10c satisfies ADR-0037's narrow operational gate: authenticated exact-key
miss, one publication, restart hit, exact continuation, different-key cold
fallback without overwrite, reserve refusal, feature-off exclusion, and
redacted operation. No correction or blocker was found.

This acceptance does not authorize production persistence, multiple entries,
generation advancement, eviction, shared scope, prefix reuse, distributed
restore, or a release claim. The broad fault, soak, capacity, two-node, and
strict final non-inferiority matrices remain deferred exactly as ADR-0037
requires.
