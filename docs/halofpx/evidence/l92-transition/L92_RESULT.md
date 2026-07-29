# L92 terminal result

Classification: **NOT PROMOTED**. The single authorized primary transition was
consumed. No retry or L92 correction occurred.

The offline rehearsal published an exact five-entry authority set with SHA256
`769b1e2b713c1f70ac44d91c0093d61df30895a0944e4717065849debcb15cc1`
and a 12-step planned path whose projected tuples all passed membership.
Residency-A then completed authenticated capture (`capture-suffix.txt` =
`alpha`) and four server terminal authorities were authenticated and retained.

The real transition failed closed during capture-worker cleanup. Durable request
sequence 5 proves the exact rejected tuple:

- host: `nimo-1`
- unit: `halofpx-l48-worker-capture`
- port: `50184`
- phase: `postcleanup`
- authority hash: `769b1e2b...5cc1`
- membership: `false`

`stop_worker(unit, port: int = PORT)` bound `PORT=50184` when Python defined the
function. Later `configure_l77_primary()` changed the global `PORT` to 50248,
but omitted cleanup arguments retained the old default. Launch and manifest
authority correctly used 50248. This is the exact source-proven defect.

Evidence hashes:

- request 5:
  `c2ba9d331fbd27457685f5db3511d8bc4d82698225435f6f0a76392728cae6ad`
- failure:
  `c0a381f91944c80fb920afec176d7bfc17c95a1840f763ed3017e3058fc005b5`
- capture log:
  `3018d50f640a04970b20e83546e950bcc4fc6a66dde526d4d1da638792cc3321`
- server harvest manifest:
  `ed267f10422875db171f61ed7c26ef8c642c53cd1c4f957e0c1cf64e742134db`

There was no residency-B launch, restored token, represented-state comparison,
or cache correctness conclusion.
