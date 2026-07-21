# P06h MiniMax-M2 peer half-load canary

Status: **exact-artifact physical peer half-load qualified; authoritative
expert sharding remains closed**

P06h adds a target-owned loader seam for a bounded contiguous slice of the
highest active GGUF tensor dimension. Under a third default-off gate, the
selected MiniMax-M2 layer now allocates and uploads only experts 96 through
191 on the RPC peer. The peer graph consumes that 96-expert tensor directly
with rank-local IDs 0 through 95.

The full local 192-expert tensor and its MoE result remain authoritative. The
P06g rank-local branches and synchronous CPU oracle remain active and abort
before HTTP output if their sum diverges from the full result. This milestone
therefore proves reduced peer storage and correct source-range loading, not an
authoritative distributed graph or a production performance path.

Nimo-2 is the coordinator and owns the authoritative layer-32 tensor/result;
nimo-1 owns only the experimental peer upper-half allocation. A missing or
failed peer makes model load or graph execution fail; no partial branch output
is accepted. This 160 GB artifact has no admitted single-node plan on a
128 GiB node, so single-node continuation is explicitly unavailable. The
fallback is a fresh restart of the known-good default-off dual-node plan (or a
separately admitted smaller model), never continuation from partial state.

## Admission and loader contract

The new gate is
`HALOFPX_MINIMAX_M2_EXPERT_PEER_HALF_LOAD=1`. It is accepted only when the
existing layer-placement and shadow-compute gates are also active. The P06d,
P06e, and P06g exact model, topology, device, split-mode, tensor-type, graph,
sequence, adapter, and output restrictions remain in force. An absent gate
leaves the P06g path unchanged; any present value other than exactly `1`
fails closed.

Source-slice tensors are implementation-only duplicates bound to the exact
created tensor pointer because names repeat across device contexts. Creation
requires a concrete GGUF source, strict target-device buffer ownership, a
positive bounded axis-2 range, matching dimensions 0, 1, and 3, checked
offset/length arithmetic, unchanged packed row geometry, and a source range
within the full tensor. The recorded packed source offset is applied to mmap
ranges, mmap-backed tensors, host reads, DirectIO/staged asynchronous reads,
ordinary fallback reads, validation, and `load_data_for()`.

For each layer-32 gate, down, and up expert tensor, the admitted source range
was experts `[96,192)` and packed bytes `[368050176,736100352)`. Each half is
368,050,176 packed bytes; all three peer halves are 1,104,150,528 packed bytes.
On this ROCmFPX build the peer half expands to approximately 1,377 MiB, saving
approximately 1,377 MiB versus P06g's full replicated peer copy. The retained
server reported an RPC model buffer of 78,253.46 MiB. These are storage
measurements and geometry calculations, not throughput claims.

## Focused qualification

Both nimo nodes completed fresh Release builds for `gfx1151` with HIP,
Vulkan, RPC, forced MMQ, no VMM, and WebUI disabled. Their `llama-server`,
`rpc-server`, and focused Q6-view executable hashes are byte-identical. The
feature-off and locked L02 controls passed 2/2 on both nodes.

The focused direct-HIP and RPC Q6-view oracles each reported nonzero reference
L2 `24.3547155`, NMSE 0, and maximum absolute error 0. The exact-model canary
then logged the three physical upper-half source ranges and admitted P06h on
layer 32. Its shadow oracle observed experts from both domains and passed; the
retained samples reached local/peer selections of 3/5 through 5/3 with finite,
nonzero contribution norms and NMSE on the order of `10^-15`.

The exact artifact remained revision
`dba517197f2854f3d362529e13abddcdcad6c10b`, file
`saricles-MiniMax-M2.7-REAP-172B-A10B-Q6_0_ROCMFPX_AGENT.gguf`, size
159,873,097,824 bytes, SHA-256
`96506ada918e60ca9a9cfde8a5437790e4453401a6a3e236e3f55e7bac3aaea6`.
The retained deterministic request contained 1,129 prompt tokens and generated
128 tokens with HTTP 200. Its newline-terminated content SHA-256 was
`a9c38c7f948adcfa8cfab5468ab84cc089b01a34c3f270f1c487a9a5fa74b555`,
byte-identical to P06g. The observed 202.92 prompt tokens/s and 15.91 generation
tokens/s are canary timings only: the run was not an interleaved matched
performance experiment and still executed the full MoE plus the shadow oracle.

## Boundaries and next seam

P06h does not split the authoritative local tensor, generalize routing to
prefill batches, remove the CPU oracle, change defaults, enable persistence,
deploy a new binary, or establish a speed improvement. The next P06 seam is a
single default-off authoritative-layer experiment only after routing is
factored without semantic drift; it must be rejected if matched measurements
show a reproducible regression. Broader mmap/fault permutations remain
deferred unless a concrete defect justifies them.

Raw node evidence is hash-manifested and stored in mode-0600 bundles. The
known-good nimo-2 worker and nimo-1 coordinator were restored in that order;
their original binary hashes, zero-restart state, listener, and HTTP health
were verified. All five immutable reference clones remain clean at their
locked commits and trees. No donor expression, GPL llama-ai implementation,
CachyLLama code, new dependency, model mutation, remote, WebUI, notice,
license, SBOM, or persistent-write change entered P06h.
