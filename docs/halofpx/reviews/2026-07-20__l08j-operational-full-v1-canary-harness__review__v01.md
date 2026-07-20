# L08j operational full-v1 canary harness independent review

Date: 2026-07-20

Verdict: **accept**

No P1 or P2 blocking finding remains. The change is correctly limited to the
operator/test harness: direct execution no longer requires pytest, pytest
collection retains the environment skip marker, and startup now supplies the
positive quota, nonnegative reserve, and exact single-entry limit required by
L09. The exact values are retained in the runtime tuple. No server, provider,
inference, or feature default changed.

The reviewed source hash matches the candidate. Retained logs show `ready`,
`published`, `recovered-success`, and corruption-driven `quarantined` states.
The result records an authenticated restart hit, corruption miss, and equal
cold recomputation. The operator key is absent. The focused controls passed
9/9; the 18-entry manifest and compressed bundle verify. The known-good
service remains active with zero restarts and HTTP 200, and no canary listener
remains.

The evidence bundle was initially created mode 0644. Although it contains only
a public tiny fixture, fixed prompt, and no key, future user-derived state must
not use that mode. L08j corrected the admitted v02 bundle to mode 0600 before
closeout.
