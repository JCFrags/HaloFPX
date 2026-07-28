# L88 focused-gates receipt

- Accepted base: `eb92a66da1a21ef230597f25dd5002c9890a7af3`
- Corrected reviewed diff identity: `eb648fd0d27b90e0d59372508ad69733ec2aada8`
- Derived grammar-v1 fixed maximum: `27`
- Mutable per-class maximum: `4096`
- Derived recorder maximum: `2 * 4096 + 27 = 8219`

The capacity fixture uses the real recorder and HMAC path. It reaches events
254, 255, and 256 while emitting the exact 253 REGISTER records after
BEGIN/L42/L44_BEGIN, then admits the first EXCLUDE as event 257. It verifies
exact counts, pending-record cardinality, and changing nonzero HMAC chains. A
separate recorder admits event 8219 and refuses event 8220 without changing
sequence, records, or chain.

The same fixture materializes all 14 exact grammar-v1 productions at maximum
admitted register/exclude cardinality, covers maximum register-only,
exclude-only, and combined streams, and refuses one-over admissions.

Focused outputs:

- `capacity.stdout`: `immutable_publication=1 recorder_capacity=1`
- `composed.stdout`: `real_composed=1 recompute=1 concurrent=1 exact=1`
- runtime-off: `feature_off_inert=1`
- cleanup: `units_absent=1`

Required feature-on targets and the compile-off RPC server built successfully.
The first HIP canary build was OOM-killed under `-j4`; production remained
byte-authoritative and healthy. The identical source completed serially under
`-j1` before preflight. The independent corrected pre-runtime review reported
PASS with no P1/P2.
