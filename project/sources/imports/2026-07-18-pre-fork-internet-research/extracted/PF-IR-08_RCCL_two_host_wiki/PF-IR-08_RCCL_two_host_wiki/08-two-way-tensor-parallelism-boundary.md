# Two-way tensor parallelism and adjacent architectures

## Standard RCCL collectives

For two-way tensor parallelism, relevant primitives include AllReduce, AllGather, ReduceScatter, Broadcast, and direct Send/Recv. They can implement synchronization and partition exchanges once a framework has selected tensor shards and insertion points.

RCCL does **not** decide layer/tensor partitioning, distribute model files, manage KV cache ownership, schedule kernels, or recover model state after a failed collective. Those are adapter/runtime responsibilities.

## Custom ggml RPC

A ggml/llama.cpp-style RPC path is an application protocol for remote tensors and operations. It may serialize commands, maintain remote objects, and choose where graph nodes execute. It is not a collective library and should be measured as a distinct architecture, not described as an RCCL transport.

## Pipeline or layer split

Pipeline/layer split assigns contiguous or logical model stages to hosts and moves activations between stages. RCCL Send/Recv could carry activation messages, but RCCL does not provide stage scheduling, microbatching, backpressure, or layer placement. Compare it separately from tensor parallelism.

## Hypothetical USB4STREAM

A custom USB4STREAM provider would sit below RCCL’s collective engine at the versioned Net ABI. It is relevant only when the desired path is not ordinary IP sockets, when direct/zero-copy semantics are required, or when the measured Socket baseline misses a defined target. It should not be conflated with ggml RPC or pipeline scheduling.

## Comparison frame

| Mechanism | Layer | Main unit | Owns model scheduling? | Uses RCCL collective algorithms? |
|---|---|---|---:|---:|
| RCCL Socket | collective + transport | tensors/collective buffers | No | Yes |
| ggml RPC | application/runtime | remote graph ops/tensors | Yes or partially | No |
| pipeline/layer split | model execution strategy | activations/stages | Yes | Optional primitive only |
| USB4STREAM Net plugin | transport provider | byte/message requests | No | Yes, above it |
