# Processed source catalog

Research date: **2026-07-17**. Sources are linked rather than redistributed. Notes capture only the design-relevant point and limitations. The search did not identify an authoritative public system named “HaloKV”; the name is used as a working label for this proposal.

## Wiki pattern and repository conventions

### WIKI-01 — Karpathy, “LLM Wiki”

- URL: https://gist.github.com/karpathy/442a6bf555914893e9891c11519de94f
- Type: primary concept note.
- Relevance: defines the raw-source layer, LLM-maintained interlinked Markdown wiki, agent schema, index, and append-only log pattern used by this package.
- Caveat: intentionally abstract; it does not prescribe a fixed directory layout or protocol-design method.

### WIKI-02 — GitHub, “About READMEs”

- URL: https://docs.github.com/en/repositories/managing-your-repositorys-settings-and-features/customizing-your-repository/about-readmes
- Type: official documentation.
- Relevance: repository-level navigation and standard project files.

## LLM KV-cache systems and transfer patterns

### KV-01 — vLLM, “Disaggregated Prefilling”

- URL: https://docs.vllm.ai/en/latest/features/disagg_prefill/
- Type: official documentation.
- Relevance: separates prefill and decode instances and transfers KV cache through a connector; useful precedent for control/data-plane separation.
- Caveat: does not define the two-rank persistent global-commit protocol proposed here.

### KV-02 — vLLM, “NixlConnector Usage”

- URL: https://docs.vllm.ai/en/stable/features/nixl_connector_usage/
- Type: official documentation.
- Relevance: asynchronous cross-process KV transfer and transport-oriented connector behavior.

### KV-03 — vLLM, “NIXL KV Cache Lease Renewal”

- URL: https://docs.vllm.ai/en/latest/design/nixl_kv_cache_lease/
- Type: official design documentation.
- Relevance: lease/retention mechanism that keeps producer-side blocks available until consumers retrieve them.
- Caveat: a transfer lease is not an epoch or globally committed checkpoint.

### KV-04 — vLLM, “Automatic Prefix Caching”

- URL: https://docs.vllm.ai/en/stable/design/prefix_caching/
- Type: official design documentation.
- Relevance: hash-based block identity over prefix context; motivates complete semantic cache-key inputs and collision handling.

### KV-05 — vLLM, KV transfer configuration

- URL: https://docs.vllm.ai/en/stable/api/vllm/config/kv_transfer/
- Type: official API documentation.
- Relevance: explicit KV roles/ranks and connector configuration.

### KV-06 — LMCache, “Architecture”

- URL: https://docs.lmcache.ai/developer_guide/architecture.html
- Type: official documentation.
- Relevance: persistent KV storage across engine lifecycles, multi-tier movement, and reusable cache objects.

### KV-07 — LMCache documentation

- URL: https://docs.lmcache.ai/
- Type: official documentation.
- Relevance: persistent and reusable KV-cache system context.

### KV-08 — LMCache, “Secondary KV Storage”

- URL: https://docs.lmcache.ai/mp/l2_storage/index.html
- Type: official documentation.
- Relevance: fast L1 versus persistent L2, asynchronous push/prefetch, and multi-tier recovery patterns.

### KV-09 — Mooncake repository

- URL: https://github.com/kvcache-ai/Mooncake
- Type: primary project repository.
- Relevance: distributed KV-cache transfer/store, replication, eviction, and high-bandwidth object movement.

### KV-10 — Mooncake Store design

- URL: https://github.com/kvcache-ai/Mooncake/blob/main/docs/source/design/mooncake-store.md
- Type: primary project design document.
- Relevance: immutable-object and distributed-store concepts applicable to content-addressed cache pages.

### KV-11 — NVIDIA Dynamo, overall architecture

- URL: https://docs.nvidia.com/dynamo/v-0-9-1/design-docs/overall-architecture
- Type: official design documentation.
- Relevance: disaggregated serving, routing, cache management, and transfer components.

### KV-12 — NVIDIA Dynamo, KV-cache offloading

- URL: https://docs.nvidia.com/dynamo/backends/v-llm/kv-cache-offloading
- Type: official documentation.
- Relevance: host/storage offload and cache lifecycle context.

### KV-13 — vLLM, Mooncake Store connector

- URL: https://docs.vllm.ai/en/stable/features/mooncake_store_connector_usage/
- Type: official documentation.
- Relevance: combines peer transfer with a shared storage pool.

### KV-14 — “KVDirect: High-Performance LLM Serving via Direct KV Cache Transfer”

- URL: https://arxiv.org/abs/2501.14743
- Type: primary research paper abstract page.
- Relevance: tensor-centric, pull-oriented KV transfer and transfer-overhead analysis.
- Caveat: performance architecture, not a persistent checkpoint safety protocol.

## Coordination, quorum, and fencing

### COORD-01 — etcd FAQ, quorum and failure tolerance

- URL: https://etcd.io/docs/v3.7/faq/
- Type: official documentation.
- Relevance: explicitly shows a two-member majority is two and failure tolerance is zero; a three-member cluster tolerates one failure, and explains why odd-sized clusters are preferred.

### COORD-02 — Ongaro and Ousterhout, “In Search of an Understandable Consensus Algorithm (Raft)”

- URL: https://www.usenix.org/conference/atc14/technical-sessions/presentation/ongaro
- Type: primary research publication page.
- Relevance: replicated log, terms, leader election, and safety foundation for a small external metadata authority.

### COORD-03 — etcd, “Why etcd?”

- URL: https://etcd.io/docs/v3.5/learning/why/
- Type: official documentation.
- Relevance: versioned key-value operations, compare-and-swap/transactions, and leases suitable for small control records.

## RPC, serialization, and service reliability

### RPC-01 — gRPC authentication

- URL: https://grpc.io/docs/guides/auth/
- Type: official documentation.
- Relevance: TLS and client-certificate authentication patterns.

### RPC-02 — gRPC cancellation

- URL: https://grpc.io/docs/guides/cancellation/
- Type: official documentation.
- Relevance: cancellation propagation and the application’s responsibility to stop ongoing work.

### RPC-03 — gRPC deadlines

- URL: https://grpc.io/docs/guides/deadlines/
- Type: official documentation.
- Relevance: explicit deadlines, server cancellation, and propagation of remaining time budget.

### RPC-04 — gRPC retry

- URL: https://grpc.io/docs/guides/retry/
- Type: official documentation.
- Relevance: retry policy and backoff; HaloKV adds application-level idempotency and ambiguity resolution.

### RPC-05 — gRPC status codes and error handling

- URLs: https://grpc.io/docs/guides/status-codes/ and https://grpc.io/docs/guides/error/
- Type: official documentation.
- Relevance: distinguishes transient availability errors, precondition failures, aborts, resource exhaustion, and data loss.

### RPC-06 — gRPC flow control

- URL: https://grpc.io/docs/guides/flow-control/
- Type: official documentation.
- Relevance: receiver protection for streaming RPCs and deadlock caveat; HaloKV adds resource-specific credits.

### RPC-07 — Protocol Buffers CodedInputStream

- URL: https://protobuf.dev/reference/cpp/api-docs/google.protobuf.io.coded_stream/
- Type: official API documentation.
- Relevance: total-byte and recursion limits for hostile serialized input.

## Storage durability and integrity

### STORAGE-01 — Linux `fsync(2)` manual page

- URL: https://man7.org/linux/man-pages/man2/fsync.2.html
- Type: Linux man-pages project documentation.
- Relevance: flushing file data/metadata and the separate need to synchronize the containing directory entry.

### STORAGE-02 — Linux `rename(2)` manual page

- URL: https://man7.org/linux/man-pages/man2/rename.2.html
- Type: Linux man-pages project documentation.
- Relevance: atomic replacement/no-missing-window semantics and retry ambiguity caveats on network filesystems.

### STORAGE-03 — NIST FIPS 180-4, Secure Hash Standard

- URL: https://csrc.nist.gov/pubs/fips/180-4/upd1/final
- Type: official standard publication page.
- Relevance: SHA-2 family, including SHA-256, for cryptographic object identity.

### STORAGE-04 — CrashMonkey repository

- URL: https://github.com/utsaslab/crashmonkey
- Type: primary project repository.
- Relevance: record/replay testing for persisted file data and metadata consistency.

### STORAGE-05 — CrashMonkey HotStorage publication page

- URL: https://www.usenix.org/conference/hotstorage17/program/presentation/martinez
- Type: primary research publication page.
- Relevance: systematic crash-consistency test methodology.

## Formal modeling

### FORMAL-01 — TLA+ tools and TLC

- URL: https://lamport.azurewebsites.net/tla/tools.html
- Type: official TLA+ resource.
- Relevance: TLC explicit-state model checking of safety and liveness properties.

### FORMAL-02 — Apalache documentation

- URLs: https://apalache-mc.org/docs/apalache/index.html and https://apalache-mc.org/docs/apalache/running.html
- Type: official documentation.
- Relevance: typed, bounded symbolic checking and simulation with SMT; useful as a secondary checker.

### FORMAL-03 — P programming framework

- URL: https://p-org.github.io/P/whatisP/
- Type: official documentation.
- Relevance: communicating state machines, model checking, specification monitors, and implementation-near distributed-protocol modeling.

### FORMAL-04 — Alloy online tutorial

- URL: https://alloytools.org/tutorials/online/
- Type: official project tutorial.
- Relevance: finite relational checking for static topology, shard-completeness, and fingerprint compatibility constraints.

## Fuzzing and fault injection

### FUZZ-01 — LLVM libFuzzer

- URL: https://llvm.org/docs/LibFuzzer.html
- Type: official documentation.
- Relevance: in-process coverage-guided fuzzing for parsers, validators, and state-machine handlers.

### FUZZ-02 — libprotobuf-mutator

- URL: https://github.com/google/libprotobuf-mutator/blob/master/README.md
- Type: primary project documentation.
- Relevance: structure-aware Protocol Buffer mutation integrated with coverage-guided fuzzers.

### FUZZ-03 — Go fuzzing

- URL: https://go.dev/doc/security/fuzz/
- Type: official documentation.
- Relevance: native coverage-guided fuzzing for Go implementations.

### FUZZ-04 — AFL++ best practices

- URL: https://aflplus.plus/docs/best_practices/
- Type: official project documentation.
- Relevance: corpus, persistent-mode, instrumentation, and campaign operations.

### FUZZ-05 — CrashMonkey / crash-consistency testing

- URLs: https://github.com/utsaslab/crashmonkey and https://www.usenix.org/conference/hotstorage17/program/presentation/martinez
- Type: primary project/publication.
- Relevance: systematic power-loss/crash-point exploration for local object publication.

### FUZZ-06 — P model checking and reproducible traces

- URL: https://p-org.github.io/P/whatisP/
- Type: official documentation.
- Relevance: state-machine exploration and reproducible counterexample traces that can seed implementation fuzzers.

## Security

### SEC-01 — gRPC authentication and Protocol Buffers parser limits

- URLs: https://grpc.io/docs/guides/auth/ and https://protobuf.dev/reference/cpp/api-docs/google.protobuf.io.coded_stream/
- Type: official documentation.
- Relevance: authenticated transport plus explicit parser byte/recursion limits. HaloKV adds authorization, anti-replay, request binding, and resource accounting.
