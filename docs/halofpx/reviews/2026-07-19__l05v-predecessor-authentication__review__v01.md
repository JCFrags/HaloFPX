# L05v predecessor authentication review v01

- Date: 2026-07-19
- Parent commit: `940af075425e6ea06ec41b81ba29e56e8fd2d662`
- Scope: M63-01b authentication of the sealed predecessor under the supplied
  locked credential plus exact launcher-pin matching; no root access or
  mutation
- Verdict: **ACCEPT**

## Outcome

The reviewed slice safely converts the sealed transport inputs into a narrow
aggregate authentication result. The no-copy facts verifier follows the
existing protected-registry parser, policy, HMAC, and continuity behavior but
does not expose the authenticated carrier or private authority material.

Independent review confirmed that the initializer authenticates the exact fd4
envelope under the locked credential and matches the registry-lab digest,
registry identity and epoch, authority scope, policy, high water, key tuple,
and continuity commitment. Secret, credential, predecessor, and retained facts
remain in the locked mapping and are wiped before unlock and unmap. Failure
cleanup clears the aggregate audit facts, closes the private descriptor copies,
restores the signal mask, and performs no root or fixture access.

## Review dimensions

- Correctness: facts-only and carrier verifiers have a focused behavioral-
  equivalence matrix covering successful and bounded hostile inputs. The initializer
  admits only a fully authenticated predecessor whose complete launcher pin
  set matches.
- Security: every fd4 bit is tested once against the original digest and once
  against a recomputed registry-lab digest; every fd3 structural, key-tuple,
  and secret bit is tested. Truncation, append, wrong-secret, incompatible-key,
  high-water, replay, and independently reauthenticated alternative cases are
  covered.
- Isolation: archive and contract audits retain two objects, two callable
  initializer definitions, exactly three HaloFPX imports, a narrow POSIX/libc
  surface, and no root, mutation, product, install, or export edge.
- Testing: each qualified Linux input invocation ran 3,303 fresh child cases
  plus one shared-thread rejection. Three invocations therefore covered 9,909
  fresh child cases plus three shared-thread rejection cases. The focused
  Release and sanitizer suites each passed 6/6, and the final Windows
  protected-registry controls passed 3/3 after the candidate froze.
- Provenance: target-native implementation; no donor or GPL code, no P3
  admission, and no direct cherry-pick.
- Rollback and performance: all gates remain default-off and inference graphs
  are unchanged. Removal is source-only; CPU contract timings are not an
  inference-performance claim.

The final independent reviewer returned **ACCEPT** with no actionable P0, P1,
or P2 finding. All ten changed-file hashes matched both Linux qualification
source trees.

This verdict does not prove credential issuer or origin, latestness,
revocation, monotonicity, rollback resistance, principal identity, or
production key custody. It does not admit root traversal, `writer.lock`,
initialization, mutation, persistence, publication, cache hits, restore,
inference, distributed behavior, or L14Q work.
