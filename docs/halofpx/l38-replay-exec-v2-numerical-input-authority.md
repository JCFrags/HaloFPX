# L38 replay-exec-v2 numerical-input authority

Status: `[MEASURED] NOT PROMOTED`. L38 remained no-production and did not read
or load the primary artifact. It does not authorize a primary run, cache
promotion, tuning, or another milestone.

## Result

The bounded candidate produced a separately versioned, default-off
`halofpx.replay-exec-v2` HMAC contract. Version 3 retains the L37 lifetime-safe
result path and adds actual graph leaves to the collector, stable view-chain and
base-relative-range authority, canonical leaf/node operation metadata and
source edges, plus scheduler split/copy counts. Unknown active mutable input
roles, malformed ranges, oversized content, absent storage/backend authority,
and authentication or count mismatches fail closed.

Independent adversarial review rejected that candidate because it is not the
complete L38 contract. The real scheduler and RPC
implementations do not currently expose enough diagnostic authority to the
llama-context collector to authenticate individual inserted copies or normalize
the RPC wire graph. The candidate therefore deliberately does not claim:

- pre-copy source versus synchronized post-copy destination content equality;
- exact scheduler split ranges or source-to-copy slot mappings;
- RPC pointer/tensor ID normalization to graph indices;
- equality of client serialization and server reconstruction;
- ordered mutable `SET_TENSOR` role/range/content records; or
- graph recompute UID lineage.

Those omissions are material to the requested numerical-input discriminator, so
the milestone is terminal `NOT PROMOTED` rather than a partial PASS. The
rejected source candidate and standalone test were removed before closeout; the
accepted L37 source remains unchanged.

## Focused qualification

The Release Linux build and exact C++ mutable-input sentinel executable passed.
The Python verifier suite passed 54 focused contract/controller tests, including
version/domain separation, HMAC tamper refusal, unknown roles, malformed range
metadata, and split/copy count sentinels.

One synthetic graph used an isolated nimo-1 RPC worker on port 50238 and nimo-2
ROCm0. It executed a single flash-attention operation with Q8_0 K/V
(`d=64`, `n_kv=8`). Two fresh graph residencies used identical declared
Q/K/V/mask content and different poison bytes only after the authenticated live
K/V spans. The active-span digests were equal, backing digests differed, and
all 64 output floats were byte-identical. This bounded result demotes the
hypothesis that this FA graph reads beyond the selected K/V views; it does not
generalize to the primary graph or prove RPC serialization authority.

The stories15M lifecycle was not repeated because it was unnecessary to prove
the new synthetic result and L37 already established the lifetime-safe
two-residency path.

## Smallest next discriminator

A separately authorized milestone must instrument the scheduler copy loop and
RPC client/server source paths directly, behind the same default-off gate:

1. assign stable graph indices before split and retain exact split ranges,
   inserted copy slots, and source/destination logical hashes around a
   synchronized copy;
2. normalize RPC tensor IDs to those indices and hash the full client wire graph
   and reconstructed server graph;
3. admit and hash only named mutable `SET_TENSOR` inputs with ordered
   role/offset/size records, and bind recompute UID lineage; and
4. feed those authenticated records back into one phase-neutral comparison
   before any primary discriminator is considered.

No primary experiment should be authorized until that source-owned equality is
executable and independently reviewed.

## Production and cleanup

Production stayed continuously active: the nimo-1 system coordinator returned
HTTP 200 on port 8081 and the nimo-2 system RPC worker remained listening on
50052. The disposable user unit was stopped, port 50238 closed, and both L38
remote build roots were removed.
