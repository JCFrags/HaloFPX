# L38 replay-exec-v2 adversarial review

Verdict: `REJECT`.

The reviewer confirmed that the separate HMAC domain, strict field rejection,
leaf enumeration, and poison-test direction were useful foundations. The
candidate nevertheless failed the milestone because it summarized the graph
outside the actual execution authorities.

Material findings:

1. The scheduler copy loop did not expose split ranges, inserted copy mappings
   and slots, or synchronized source-before/destination-after hashes.
2. The RPC implementation had no canonical pointer/tensor-ID mapping, client
   serialization digest, server reconstruction digest, mutable `SET_TENSOR`
   record, or recompute-UID authority.
3. The collector followed only `GGML_TENSOR_FLAG_INPUT`, so active mutable K/V
   slices or other unflagged mutable inputs could be omitted.
4. Contiguous `ggml_nbytes` reads did not prove logical hashing for arbitrary
   non-contiguous strides, and the view chain was incomplete.
5. Canonical topology omitted some view/source edges and encoded an unknown
   source as `-1` instead of refusing.
6. The standalone poison test did not invoke or authenticate the proposed
   replay-exec-v2 contract.
7. The verifier could validate summary counts but not the missing per-copy/RPC
   records or their hard bounds.

The rejected source candidate was removed. Review accepts only a narrow
`NOT PROMOTED` closeout and the source-owned next-discriminator plan; it does
not accept a primary run.
