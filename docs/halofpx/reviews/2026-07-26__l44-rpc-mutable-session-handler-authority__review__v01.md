# L44 RPC mutable session/handler authority adversarial review

Verdict: **PASS**

The independent review examined the actual RPC handler paths, admitted-session
lifetime and concurrency, census and server-applied material authority,
feature-off behavior, qualification evidence, and cleanup.

## Findings and corrections

The first review rejected three details:

1. Semantic negative updates were changed after signing and therefore stopped
   at generic HMAC validation.
2. Failed sessions could not be explicitly aborted because ordinary lookup
   rejected their failed state.
3. Handler refusal receipts were synthesized without authentication.

The final source corrects each issue. Semantic malformed, duplicate-sequence,
out-of-bounds, and wrong-view requests are signed after their deliberate
semantic mutation; only the tamper case retains an invalid request tag.
Accepted evidence reaches distinct header, sequence, range, and view refusal
branches. The server constructs a bounded HMAC receipt bound to the admitted
attempt and server nonce, and the client verifies it. Abort performs exact
handle identity validation while permitting failed-session cleanup.

## Accepted evidence

The final disposable evidence shows:

- five authenticated SET-handler refusal receipts with exact status `0`;
- distinct server logs for malformed header, tampered header,
  duplicate/out-of-order sequence, out-of-bounds range, and wrong view;
- an authenticated graph-prepare status `3` for the omitted reconstructed
  leaf, with no execute and unchanged output sentinel;
- two simultaneous RPC sessions with foreign/cross-connection refusal;
- stale, closed, failed, and buffer-destroyed session refusal/cleanup;
- ordinary SET and synchronized applied-material receipt;
- real SET_HASH miss/fallback followed by hit;
- exact compute/recompute output and controlled mutation sensitivity;
- runtime-off inertness and a successful compile-off RPC server build;
- no disposable listener/process after cleanup and unchanged production.

No further material issue was found in the scoped corrected source. The
review accepts L44 as a reusable, runtime-default-off scheduler/RPC
mutable-update authority layer. It does not review or authorize primary-model
execution, production cache enablement, or subsequent milestones.
