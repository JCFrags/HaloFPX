# P06e MiniMax-M2 Q6 replicated shadow-compute canary independent review

Status: **ACCEPT after correction; no remaining blocking finding.**

The review checked admission, graph authority, routing reuse, masks, tensor
types, ordered readback, fail-closed numerical validation, default-off behavior,
provenance, rollback, and the retained Linux evidence. The implementation is
target-native and keeps the normal full MoE result authoritative. Its CPU
oracle copies that result only after both replicated local/RPC branches satisfy
finite-value, NMSE, and scaled-error limits.

The initial review found one P2 clarity defect: compute mode still emitted the
older placement-only message, and the corresponding model comment described
only P06d. The message now distinguishes P06e replicated Q6 shadow compute
from P06d placement-only admission and explicitly says that the authoritative
output remains the full local MoE result. The model comment now covers both
non-authoritative seams. These wording-only corrections were rebuilt and the
locked contracts passed; no graph behavior changed after the retained exact
model run.

The three failed disposable attempts are appropriately retained. They prove
that the oracle fails closed and isolate a real RPC nonzero expert-axis-view
defect. The accepted implementation avoids that unqualified boundary by using
full replicated expert tensors, global expert IDs, and complementary
contribution masks on both ranks. This proves real two-rank Q6 shadow execution
and exact continuation equivalence, but it does not prove physical sharding,
compact selected work, overlap, or a performance improvement.

The exact 159,873,097,824-byte artifact is pinned by revision and SHA-256. The
full 1,129-prompt-token/128-generation-token canary returned HTTP 200 and
matched the P06d control content byte-for-byte. Final feature-off/L02 controls,
strict malformed-gate rejection, manifests, service restoration, HTTP health,
and all five immutable reference-clone states are recorded. Promotion is
accepted only as a strict, default-off correctness seam; the single
CPU-synchronized timing is not performance evidence.
