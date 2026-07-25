# L40 RPC graph-authority adversarial review

Final verdict: `PASS` for the frozen RPC-only foundation.

The first candidate was rejected for four material reasons:

1. CAPS, header, and receipt records used packed native structures instead of
   explicit canonical little-endian codecs.
2. Allocation identity was graph encounter order rather than the allocation
   ordinal assigned by the connection.
3. Server comparison inherited client wire tensor/node order.
4. The server returned its reconstruction receipt only after backend compute.

The corrected source resolves those issues:

- exact bounded LE codecs are the wire and HMAC input;
- allocation ordinals are assigned by the real successful `ALLOC_BUFFER`
  sequence on both sides and reset with socket lifetime;
- the server independently walks reconstructed graph nodes and recursive leaf
  postorder, rejects extras/reordering, and derives its own storage authority;
- graph preparation returns an authenticated receipt before a separately
  authenticated EXECUTE command can invoke the backend; and
- nonzero UID/sequence, attempt and server nonces, digest/transcript lineage,
  CAPS reset, legacy mutation invalidation, and consume-before-execute prevent
  stale or replayed preparation.

Feature-off compilation and runtime preserve the ordinary graph command path.
The real isolated compute/recompute evidence and focused negative tests support
the accepted foundation.

The verdict expressly excludes scheduler copies, mutable-input and tensor-role
authority, expert/Q8/flash-attention coverage, primary-model correctness,
performance, and cache promotion.

