# L51 warmup lifecycle and unit-evidence result

Status: **NOT PROMOTED**
Base: `8fa511036ef9fc633b00fe1148ae0b032457f495`

## Source-backed diagnosis and candidate

L51 reconstructed only the previously reviewed L47/L50 disposable candidate.
The L50 warmup failure was localized to the RPC graph-compute branch: global
diagnostic environment enabled authenticated RPC compute before the canary
explicitly admitted an execution. The unarmed model-load warmup therefore
reached L44's missing-session refusal and propagated scheduler `-1`,
`process_ubatch -1`, and `llama_decode -3`.

The reviewed candidate added an explicit RPC execution arm/disarm identity
bound to attempt nonce and monotonic execution sequence. Unarmed compute took
the ordinary RPC path; L42/L44 traversal and graph-input authority were gated
by the admitted execution. Armed missing authority remained fail-closed.
Every examined error path aborted mutable sessions, scheduler authority, and
RPC execution authority before returning.

The unit-evidence repair replaced `systemd-run --wait --pipe` admission with
detached named units, a bounded InvocationID/PID poll, exact opaque journal
cursor handling, and TERM -> bounded wait -> invocation-filtered
journal/status collection -> stop/reset teardown. Fast exits retained the
InvocationID before PID availability.

Focused qualification passed:

- 59 Python lifecycle, manifest, evidence, refusal, and controller tests;
- real ROCm RPC feature-on compute/recompute, armed refusal, teardown, and
  subsequent unarmed compute;
- feature-off inertness;
- exact final-source ROCm builds and a closed-manifest dry run;
- independent pre-runtime review: GO.

L40 connection/allocation ordinals remained available before arm because they
are later graph identity, not L42/L44 execution activity.

## Sole stories15M session

The one authorized controller session ended before model load or warmup. The
real no-model gate succeeded and retained:

- unit `halofpx-l50-device-gate.service`;
- invocation `a197d74306944f3bb5f0dd970d8d064a`;
- PID `2419661`;
- exact worker binary SHA-256
  `7a8fb0496486cc12746ec31f7ed3eb32ba6d9b450948bb2fc39dd7014e194b10`;
- authenticated device tuple `ROCm` / `ROCm0` / `gfx1151`;
- device inventory SHA-256
  `27c70d2186271e068e42ea720b1f78d76d1ba805605676abd3e0df51a1243951`.

The runner then attempted to copy `device-admission.json` to
`nimo-2:/var/tmp/halofpx-l48-evidence/device-admission.json` before creating
that remote evidence directory. `scp` refused with “No such file or
directory.” Under the one-session/no-repeat boundary this is terminal.

No stories model was loaded. No warmup, prompt chunks, capture, restore,
token-4245 comparison, composed L40/L42/L44 result, or legacy state-window
count was produced. The source correction is therefore focused-qualified but
the complete primary preflight contract is not qualified.

## Safety and closeout

The controller completed its mandatory cleanup with no reported cleanup
failure. Disposable units are absent, ports 50248/50249 are closed, protected
keys and all manifest-owned source/build/state/evidence roots are absent.

Production was never mutated. Preflight and final snapshots are byte-identical
at SHA-256
`511f05d2b638277e19ae7af44573eb9e244d829458f24cdefc46e3f88ffd6ded`:

- nimo-1 coordinator: system unit active/running, PID `2356329`, port 8081,
  HTTP 200, NRestarts 0;
- nimo-2 worker: system unit active/running, PID `1535639`, port 50052,
  NRestarts 0.

The primary artifact was not accessed. The rejected runtime candidate was
removed from the terminal tree. Raw evidence is retained under
`docs/halofpx/evidence/l51-raw/`; its `SHA256SUMS` file hashes to
`3f9fec9499994e00c43f91847c2b0c1939a85509d34e6e0184fa62646d5711e3`.
