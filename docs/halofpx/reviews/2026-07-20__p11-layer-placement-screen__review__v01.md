# P11 layer-placement screen independent review

Status: **accepted; no blockers**

The review independently checked the P11 synthesis, receipt, raw-evidence
authority, and measurement harness against P07 through P10 and the project
Wiki. It verified the stored sample means and exact percentage deltas, JSON
validity, qualified request/output hashes, bounded nonclaims, rollback wording,
and the absence of engine, persistence, donor, license, remote, or default
behavior changes.

Two publication defects were found and corrected before acceptance:

1. The receipt's exact prompt and generation percentage deltas were corrected
   from the retained samples.
2. The optional split validator now rejects `0,0`. The receipt separately
   records SHA-256 `322ffa5cdb68bb872eb6a41153134c0b814181f469fa383ca39da81f362c87bf`
   for the harness that executed the measurements and SHA-256
   `6dc9d989398499d46a397108ce0adf88b012d98e3221fb9bc8a1fca0df469c5c`
   for the promoted post-review harness. The validation-only correction does
   not retroactively relabel the executed artifact.

Focused reruns passed Bash syntax, invalid arity, invalid string, `0,0`
fail-closed exit status, JSON parsing, and whitespace checks. Static review
confirmed that the six-argument default remains `1,1`. Windows line-ending
notices are warnings rather than diff defects.

The reviewer accepts the bounded remote-heavy placement rejection and the
decision to leave the time-boxed MMVQ expert-overlap candidate unopened until
its device-event kill gate can be implemented without broadening scope.
