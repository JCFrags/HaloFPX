# L39 direct scheduler/RPC execution authority

Status: `[MEASURED] NOT PROMOTED`. L39 remained no-production and did not
access or load the primary artifact. It does not authorize a primary run,
cache promotion, tuning, or another milestone.

## Result

The bounded candidate placed default-off hooks in the real scheduler split and
copy paths and in RPC graph serialization, reconstruction, tensor update, and
recompute paths. A Linux ROCm build succeeded. The isolated combined graph
forced two scheduler splits, four copy mappings, and eight authenticated
source/destination copy observations across compute and recompute.

The first run exposed a fixture-authority error: recompute did not replay the
declared mutable inputs and returned different output (`0.711914062` versus
`0.87890625`, maximum delta `0.166992188`). Replaying the identical mutable
input transcript before recompute produced exact output equality, while the
server reported equal graph authority for compute and recompute. This
correction qualified only the narrow fixture behavior; it did not cure the
contract omissions below.

Independent adversarial review rejected the implementation. In particular:

- the client did not receive a negotiated authenticated receipt independently
  generated from the server reconstruction;
- the mutable-input census still treated unknown unflagged tensors as
  immutable and used tensor names rather than closed canonical roles;
- the server-side allocation comparison reused client wire allocation fields
  after reconstruction and was therefore partly circular;
- scheduler records were callback-local rather than bounded, versioned HMAC
  events and omitted complete op/source/view/backend authority;
- logical nested/non-contiguous view hashing, expert partial copies,
  `SET_TENSOR_HASH`, Q8_0 flash attention, and required tamper/refusal cases
  were not qualified; and
- recompute identity and canonical encoding did not meet the frozen opaque-UID
  and canonical little-endian contract.

Those are material acceptance failures. The rejected runtime hooks and test
were removed. Accepted source remains identical to L38 commit `169d81ad`.

## Bounded evidence

Candidate worktree diff SHA-256:
`998b629e267f708c5ca5ecb379d2315cd0e5f1bb66c6dd132b9edac6c4e5db6d`.

Disposable test binary SHA-256:
`a8e6661d8ba6a0129ecd8f10f7d546029ae675885e681686cf7f631ccab84926`.

Corrected combined result:

```text
splits=2 leaves=6 nodes=6 copy_maps=4 verified_copies=8
output_equal=1 finite=1 max_delta=0
first0=0.711914062 second0=0.711914062
trace_sha256=014bab491b1212d8be15d209cc2c3f04987b93667c363ed8db7ba4d077ad7909
```

Server evidence recorded compute sequence 1 and recompute sequence 2 with
`server_graph_equal=1` and graph SHA-256
`85016b8ed87d2549179723d41c0e27c9e2496ebfab058600559e2cc4595cecdc`.
This is directional candidate evidence only because the server receipt and
allocation authority were not independently authenticated.

## Smallest future prerequisite

A separately authorized milestone would need to design the protocol receipt
first: negotiate an exact capability and return an authenticated server-owned
reconstruction receipt to the client. It must then close the mutable-role
census independently of the input flag, derive server allocation ordinals and
ranges without copying client authority, and qualify canonical LE HMAC records
for nested logical ranges, expert partial copies, `SET_TENSOR_HASH`, Q8_0
flash attention, recompute UID, and the focused tamper/refusal cases. L39 does
not authorize that work.

## Production and cleanup

Production remained continuously active and was not mutated. At closeout:

- nimo-2 system worker was active/running, PID `1535639`, port `50052`
  listening, `NRestarts=0`;
- nimo-1 system coordinator was active/running, PID `2356329`, port `8081`
  listening, HTTP health returned `200`, `NRestarts=0`; and
- disposable user unit `halofpx-l39-rpc.service`, port `50239`, nimo-1 binary
  root, and nimo-2 source/build root were absent.

