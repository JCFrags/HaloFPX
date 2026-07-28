# L76 real server-authority harvest qualification

L76 is **PASS** at source commit
`52cda98fd3e6f871096089db623ddcc2c2f10705`.

The final bounded Linux qualification used one isolated CPU RPC server on
`nimo-1`; it loaded no model and did not use a production listener. The exact
feature-on build produced:

- `rpc-server` SHA-256
  `6cc1aba525ad38144b0c6c8eb7ec8c279b6a63383c8e9ba43a5c21512b9037f4`;
- focused client SHA-256
  `980efd445de7be90986895f08d37479f4a6ab6af10be3287244c0c4d57ff1c89`;
- harvest helper SHA-256
  `fae2dcbc0a01fb85d505971e05cc7c25c8568480b6f7d38a2f67d42d5d7b9e49`.

The final success request (InvocationID
`1ca90547338a47c9aefb1fa897cc7c5c`) completed authenticated prepare,
execution, response, and server publication. After unit quiescence the
controller derived the journal-bound path, validated source mode `0400`,
authenticated the six-record server grammar and its cross-bindings, atomically
retained the 4,200-byte file, fsynced and reopened it, and verified SHA-256
`04ea9584d338d3772fa7a031daa20b12818ad7c93c074d1197d1a942e2cd9c8f`
before remote removal.

The injected publication-failure request (InvocationID
`9dd2aa65376e494faef372622edb8e0c`) reached the real response/publication
seam. Its server journal recorded the bound publication result as
`status=error`, `errno=5`; no `*-server.authority` existed. The controller
durably classified the harvest as non-promotable for that attempt and cleanup
continued.

The qualification preserved both earlier successful handler publications whose
custody failed during source-proven controller/helper corrections. They are
evidence of the corrected refusal behavior, not promoted success attempts.

Focused local gates passed 58 tests plus 11 controller subtests. Independent
source review passed after one exact-size/changed-key correction; final terminal
review passed with no correctness/security P1/P2.

All disposable units are `not-found`/inactive and all disposable paths and keys
are absent. `production-terminal.json` is byte-equivalent to
`production-preflight.json`. No production unit, listener, configuration, or
model was mutated.
