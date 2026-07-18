# L05o concrete protected-registry contract review

- Date: 2026-07-18
- Scope: ADR-0018, normative CDDL, independent golden checker/vector, and
  read-only nimo-2 host-selection evidence
- Final verdict: ACCEPT

## Review history

The first independent adversarial review returned `REVISE` for nine blocking
issues: no frozen wire/layout, arbitrary authenticated transitions, inconsistent
`HEAD` meaning, a too-late uncertainty boundary, no persistent quarantine
object, underdefined credential and root authentication, incomplete OFD-lock
ownership, incomplete initialization/reserve accounting, and an overclaimed
`durable` result name.

After those corrections, the second review returned `REVISE` for three exact
remaining mismatches: the sealed credential did not authenticate the embedded
ADR-0013/0014 envelopes, initialization referenced a lock inode before creating
it, and the golden checker proved outer-MAC sensitivity without enforcing the
closed semantic schema under recomputed hostile tags.

The final revision uses the admitted `registry-auth-v1` generation-13 test
tuple for both inner transition authentication and a purpose-separated L05o
KDF; creates, validates, and synchronizes directories and the permanent lock
before the marker; requires exact selector/resolved-envelope agreement; and
adds closed schema, cross-field, inner-HMAC, continuity, H-to-H-plus-one, and
recomputed-tag hostile validation.

## Independent verification

The reviewer reran the standalone checker and observed:

```text
PASS: 8 fixtures; 3260 mutation checks
```

The accepted artifacts freeze:

- the fixed disposable-root layout and 512 one-shot slots;
- the sealed memfd credential package and purpose-separated KDF;
- authenticated initializing/final root markers, `HEAD`, `PREPARE`, `CLOSE`,
  `ABORT`, and sticky `QUARANTINE`;
- exact ADR-0013 predecessor and ADR-0014 H-to-H-plus-one successor admission;
- first-mutating-syscall uncertainty, restart recovery, and quarantine;
- Linux OFD locking, process-local non-reentrancy, and no stale-lock break;
- preallocated loopback/reserve bounds and protected-path exclusion; and
- process-crash-only result naming with no filesystem/device/power-loss or
  whole-domain rollback claim.

## Promotion boundary

ACCEPT authorizes implementation of the standalone, Linux-only,
`EXCLUDE_FROM_ALL` L05o laboratory backend. It does not authorize a server or
provider link, cache admission, material or anchor write, production key,
persistent feature enablement, nimo-1 mutation, or durability-mode label.
