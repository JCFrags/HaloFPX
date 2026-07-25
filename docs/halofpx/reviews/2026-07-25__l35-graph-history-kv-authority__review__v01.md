# L35 independent adversarial review

Verdict: **PASS** after correction and final-source rerun.

The reviewer initially rejected the candidate because the controller did not
consume replay-authority records, verifier grammar was weak, some diagnostic
bookkeeping was not runtime-gated, the census grouped uncovered graph inputs too
broadly, retained runs predated explicit environment/hash binding, and closeout
claims lacked hashed evidence.

The accepted candidate:

- hash-binds and invokes the replay verifier through the bounded SSH evidence
  primitive using the protected key file, requires exactly one authenticated
  record per phase, and retains parsed records in `result.json`;
- validates ordered backend records, unique typed K/V tensor and attention-view
  identities, numeric geometry/ranges, slot/head/position vectors, phase, HMAC,
  and prepare/apply equality;
- gates graph/KV diagnostic bookkeeping and canonical reset behind
  `HALOFPX_REPLAY_AUTHORITY_DIAGNOSTICS=1`;
- explicitly enumerates covered and residual graph-input authority without
  claiming a primary root cause;
- reruns ordinary and canonical-reset combined lifecycles with explicit current
  argv and hash-bound verifier evidence; and
- retains a system-scope production/cgroup/command/listener/HTTP/NRestarts and
  disposable-cleanup closeout snapshot in the authenticated evidence tree.

Independent conclusion: the bounded L35 diagnostic contract is acceptable. The
stories15M result does not establish the primary-model root cause and does not
authorize a primary run or promotion.
