# L05p registry-lab read-only operations 1-4 review v02

- Date: 2026-07-18
- Scope: registry-lab credential-profile correction and exact-candidate
  requalification of the accepted operations 1-4 milestone
- Final verdict: **ACCEPT**

## Supersession boundary

This additive review supersedes v01 only where v01 described the credential
key-ID profile as the general non-NUL ASCII `0x01..0x7f` range. The
registry-lab-specific normative grammar at
`contracts/context-store-registry-lab-v1.cddl` fixes credential-package key IDs
to printable ASCII `0x21..0x7e`, and the independent golden parser enforces the
same range. That more specific contract controls this record family.

The implementation and hostile boundary tests now reject `0x01`, `0x20`,
`0x7f`, embedded NUL, and `0x80`, while accepting exact boundaries `0x21` and
`0x7e`. Independent rereview returned ACCEPT and found no contrary
higher-precedence registry-lab source.

## Exact-candidate requalification

All repeat evidence was recollected after the correction rather than reusing
the pre-correction processes:

| Gate | Result |
|---|---|
| Windows Release repeated focused executions | Pass, 1,000/1,000 in 81.48 s |
| Windows Debug repeated focused executions | Pass, 200/200 in 27.28 s |
| nimo-2 ASan/UBSan repeated focused executions | Pass, 1,000/1,000 in 60.96 s |
| nimo-2 ASan/UBSan registry-lab matrix | Pass, 6/6 |
| Independent specificity rereview | ACCEPT |

The raw pre-correction logs remain preserved under explicit
`pre-profile-correction` names. The machine-readable receipt now contains the
corrected source, executable, archive, and exact-candidate evidence hashes.

All other correctness, isolation, rollback, provenance, non-authority, and
promotion-boundary findings in v01 remain unchanged. Operation 5, decoding,
mutation, Linux I/O, persistence, cache reuse, and performance authority remain
closed.
