# L49 — readiness transport requalification result

Result: **NOT PROMOTED — DISPOSABLE WORKER EXITED BEFORE CAPS**

Base: `591603ff0982fe684fd67c45f40898f4332fac88`

L49 reconstructed the independently reviewed L48 candidate and changed only
the transport authority needed for application readiness. The controller
added a closed `hfxcap2-readiness` operation with a 120-second inner
application budget, a 150-second outer local-process deadline, and a
30-second termination/evidence margin. It required the complete tokenized
readiness argv to match one of two exact L48 forms before `Popen`; endpoint,
option order, rank/world/key authority, feature-on/off shape, and every
deadline field were closed.

Focused qualification passed 54 affected tests. It covered a deterministic
responder after 30 seconds, unchanged generic-command 30-second behavior,
wrong class/budget/path/endpoint, missing/extra/reordered arguments, stuck
probe termination and reap, key and result-verifier binding, and controller
manifest refusal. The final candidate identities were:

- controller SHA-256:
  `57b612380da7f428883121671a791b5cd9b4c16ddbc8a97a63f7cbe44d2eb032`
- runner SHA-256:
  `f0e084faa7932561eb04cee8100ee2ab46ce24e516538dd41b589915a603c823`
- reconstructed source identity root:
  `1bd88144d65583da285ad3aa7ded2c57bc31037775e18e859501a491de034e7f`

The final pre-runtime adversarial review returned GO. The fresh dry-run
recorded 47 bounded operations, zero failures or timeouts, and exact unchanged
production authority.

## Sole disposable runtime

The one authorized stories15M session failed before model or composed
authority qualification. The worker launch command was accepted by
`systemd-run --user` as invocation
`50c47e1d94d34eafbc4997d96c05a9b6`, but the unit subsequently reported
`failed`, `MainPID=0`, and never opened port 50248. The real readiness record
shows:

- operation class: `hfxcap2-readiness`
- inner budget: 120 seconds
- outer deadline: 150 seconds
- observed duration: 120.198851 seconds
- local transport timeout: false
- return code: 1
- result: 123 bounded attempts, all `connect-failed`

Thus the L49 deadline correction behaved as specified, but no HELLO/HFXCAP2
response and no authenticated composed result existed. There is no token,
L40 graph, L42 scheduler, L44 mutable-session, state-transfer, or replay
correctness evidence to accept. No retry was performed.

The worker's exit cause is unknown. The session did not retain its journal,
`ExecMainStatus`/`ExecMainCode`, or unit stderr. This result does not attribute
the exit to the binary, key, ROCm, systemd, controller, or protocol.

## Cleanup and production

The capture unit was reset to `not-found/inactive/dead`; restore and canary
units were absent; port 50248 was closed. The protected key and every admitted
source, build, worker-state, coordinator-state, rendezvous, and remote
evidence path were removed and verified absent. The later read-only preflight
correctly refused because those staged sources had already been cleaned; it
adds no runtime diagnosis.

Production-before and production-final records are byte-identical, SHA-256
`511f05d2b638277e19ae7af44573eb9e244d829458f24cdefc46e3f88ffd6ded`:

- nimo-2 system worker: PID 1535639, exact command, port 50052, NRestarts 0.
- nimo-1 system coordinator: PID 2356329, exact command, port 8081,
  HTTP 200, NRestarts 0.

No primary artifact was accessed and production was not mutated. The rejected
runtime candidate was removed before closeout. Raw evidence is retained under
`docs/halofpx/evidence/l49-raw/`; its `SHA256SUMS` digest is
`3651eeeb10be27c4203519d8bbace25232256e02e7c65ab924d2c67b057af56f`.
