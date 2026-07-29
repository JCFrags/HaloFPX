# L110 atomic rank-partition gate

Status: **NOT PROMOTED — mandatory source-gate review failed**

Base: `e5b4a9a0d1e92c44785fee8cc58bf56ef29bd4d2`

No L110 candidate source is retained. No host, accelerator, model, production,
RPC, graph, or runtime action occurred.

## Gate attempted

The first L110 gate attempted to replace the earlier caller-asserted
single-slice API with an atomic two-rank source-partition constructor. The
candidate validated exact axis-2 coverage and strict distinct device buffer
ownership, hid the secondary tensor from public lookup, and introduced a
context checkpoint intended to unwind a failed second allocation.

The local environment contained no CMake, Ninja, MSVC, Clang, GCC, or WSL
compiler, so no compile or tiny-GGUF result was available. The exact
uncommitted diff was therefore submitted to fresh independent source review
before any retention or wider integration.

## Review failure

The independent reviewer found two P1 defects:

1. The success path subtracted both physical slice sizes after the existing
   slice loader added them. This could omit both rank-owned physical
   allocations from model buffer accounting and under-allocate buffers.
2. The transaction's catch boundary ended before later throwing mutations,
   including insertion into the implementation-only lookup set. An allocation
   failure there could leave tensors, offsets, contexts, and counters committed
   without returning an authority.

The reviewer also found P2 defects:

- rank identity accepted arbitrary unequal numbers instead of the closed
  `{0,1}` set;
- the public checkpoint exposed a forgeable raw arena address and lacked
  context/generation binding, allowing stale checkpoint reuse;
- invalid non-axis dimensions could reach GGML assertions rather than typed
  refusal;
- the mandatory tiny-GGUF rollback/accounting/lookup evidence was absent.

Verdict: **FAIL / REMOVE candidate**.

## Result

All L110 source edits, including the original four-file WIP and the checkpoint
experiment, were removed. The accepted base source is unchanged. Pre-L110
untracked evidence and archives remain preserved.

A future attempt must begin with a loader-internal, generation-bound
transaction whose no-throw commit accounts one logical source tensor and both
physical device allocations exactly. It must pass the complete tiny-GGUF gate
before touching MiniMax graph or asynchronous RPC work.
