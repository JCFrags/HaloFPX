---
title: Glossary
status: normative terminology
---

# Glossary

**Activation boundary.** The hidden-state tensor transferred when a contiguous model partition ends on one rank and the next partition begins on another.

**Coordinator.** The process that admits requests, assigns session IDs, maintains externally visible ordering, and resolves final control decisions. The coordinator need not own all model computation.

**Decode.** Autoregressive generation after prefill. A decode step processes one new input token per active sequence, attends to its existing KV cache, produces logits, samples/selects a token, and feeds that token back for the next step.

**Expert owner.** The rank that stores and executes a particular MoE expert's parameters. Experts are stateless with respect to attention KV, but execution uses transient routed-token buffers.

**KV owner.** The rank responsible for the authoritative key/value cache for a defined set of layers, heads, and sessions. KV ownership must be single and explicit unless a replication protocol is defined.

**Model owner.** The rank that stores and executes a defined model component: full model, contiguous layers, tensor shards, embeddings, final norm, LM head, routers, or experts.

**Nominal line rate.** The interface rate in a hardware specification. It is not application payload throughput. In this wiki, 40 Gb/s is converted to 5 GB/s only to construct payload-only lower bounds.

**Pipeline microbatch.** An independent unit of work passed through stage A, the link, and stage B. During decode, microbatches must contain independent sequences because one sequence's next token depends on the previous sampler result.

**Prefill.** Processing the prompt tokens to produce initial KV state and the first next-token logits. The forward token count \(N\) can be one prompt, a batch sum, or a chunk.

**Rank.** A separately addressable distributed process participating in model execution. This wiki uses rank 0 on node A and rank 1 on node B unless an external frontend is specified.

**Remote fraction \(\rho_l\).** At MoE layer \(l\), the number of expert assignments sent across the node cut divided by all routed assignments. It is workload-, routing-, placement-, and batch-dependent.

**Sampler.** The authoritative component that applies logits processing and selects the next token under greedy, top-k, top-p, temperature, or another policy. A sharded-vocabulary sampler may be a protocol distributed across ranks, but one rank owns final RNG/counter and emission authority.

**Session owner.** The rank or frontend holding authoritative request lifecycle, emitted-token history, cancellation, and error state. Session ownership is distinct from layer-local KV storage.

**Synchronization point.** A dependency that prevents downstream model work from proceeding until communication or another rank's result is complete. Transport message phases are lower-level operations within a synchronization point.

**Tokenizer owner.** The component that maps user text to canonical token IDs and token IDs back to text. Replicas may each own a full tokenizer; split modes normally use rank 0 as canonical owner and require exact vocabulary compatibility on every model component.

**Transport phase count \(m_{AR}\).** Number of fixed-latency message phases used by the actual p=2 all-reduce implementation. A direct exchange and a reduce-scatter/all-gather ring have the same per-rank byte total but different phase counts; the runtime trace decides the value.
