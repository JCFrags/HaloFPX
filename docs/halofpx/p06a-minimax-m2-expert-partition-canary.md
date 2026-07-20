# P06a MiniMax-M2 expert-partition canary

Status: **expert-axis contract qualified; runtime placement remains closed**

P06a creates the smallest honest implementation seam for concurrent MiniMax-M2
expert work. It is a target-native, test-only component behind
`HALOFPX_MINIMAX_M2_EXPERT_PARTITION_CANARY`, which defaults `OFF`. It does not
link into `llama`, enable generic tensor split, alter attention or Q8_0 KV-cache
ownership, change RPC, or affect a normal build.

The exact primary model has 192 experts and selects eight per token. P06a binds
world size two, rank 0 ownership of global experts 0 through 95, rank 1
ownership of 96 through 191, hidden width 3072, and a bounded 1 through 512
token reduction surface. The authoritative global selected-expert IDs remain
unchanged. Each rank receives local IDs 0 through 95 for owned slots; non-owned
slots map to dummy local ID zero with an explicit inactive mask. Invalid world
size, global ID, duplicate selection, null partial, zero tokens, and oversized
reduction fail closed.

## Why generic tensor mode was not opened

An initial allow-list proposal was rejected before configuration or build.
Existing generic tensor placement splits FFN intermediate dimensions rather
than the expert axis, partitions attention and KV-cache tensors, and has no
expert-axis-2 propagation for `MUL_MAT_ID`. Calling that route expert parallel
would be incorrect and would violate the requirement that Q8_0 KV remain
layer-owned. Unbuilt exploratory source snapshots are retained inert under
`/var/tmp/halofpx-p06-minimax-tensor` on both nodes; they were never integrated,
configured, run, deployed, or used for a result.

The existing RPC graph command is already client-asynchronous in practice: it
has no response, while a later response-producing command is ordered behind it
on the server connection. Its public synchronize hook remains a no-op, so a
future runtime lane needs an explicit completion contract for correctness, but
an RPC wire rewrite is not assumed to be a speedup.

## Focused qualification

Nimo-1 built the isolated target from parent
`0d9e2fb323a2537e6d4261775464ace69aaba0b6` with GCC 16.1.1, CMake 4.3.4,
Ninja, and Release CPU settings. The focused executable passed:

- exact 192/8/3072 and two-rank/96-per-rank geometry;
- an all-rank-0 selected set;
- a four/four cross-rank selected set including boundary IDs 0, 95, 96, 191;
- exact owner masks and global-to-local remapping;
- bounded two-rank hidden-state partial reduction;
- wrong-world, out-of-range, duplicate, null, zero-token, and oversized-input
  rejection.

CTest passed 1/1. The executable SHA-256 is
`8333968b2499650ff764d872f8c4bf0f4c5ddbcf2c8e2c57154a1026aa911099`.
A separate feature-off configuration recorded the option as `OFF` and exposed
no P06a test target. The ON and OFF `CMakeCache.txt` hashes are respectively
`aece87d1c03aa10b8fc96829e490f8e5fa304554648d6eb8249608420a54f632`
and `896d200a8768e42188ac0e98e2ee0e488254906ec84a5fca7cfe845240b3d79f`.

The selected evidence manifest SHA-256 is
`adc0136dae24592684ba56365691dcb5a012391b1f2b2890f3351619cf61787a`.
The 10,481-byte evidence bundle SHA-256 is
`c50908ed8e040b5e8a5af83d3f23eb7efc77e05b9b16b0494045feb63a2ef045`.
All manifest entries verify.

## Boundary and next step

P06a proves only ownership, remapping, masking, fail-closed admission, and
two-rank partial addition. It does not yet execute ROCmFPX expert kernels,
prove local/remote overlap, load the 160 GB model through expert placement, or
claim a speedup. P06b and P06c subsequently exercised a synthetic
Q8_0_ROCMFPX variant at the model's 3072 x 1536 x 192 geometry. P06d exact
artifact intake established that the primary model's expert tensors are
Q6_0_ROCMFPX, so the Q8 milestones are backend and partition-mechanics
evidence rather than primary-artifact type qualification.

No donor code, GPL llama-ai code, CachyLLama code, dependency, model mutation,
WebUI, persistent write, remote, deployment, or reference-clone change entered
this milestone. Generation above 30 tok/s remains a stretch objective.
