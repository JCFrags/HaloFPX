# L17 device discovery and layer-placement authority

Date: 2026-07-21

Base: `20f19a2d2460bff76f381a43ef8010c0e5f7ff08`

Outcome: **PASS — NO PRODUCTION MUTATION; NO PRIMARY MODEL ACCESS**

## Result

[VERIFIED] The eventual primary command now explicitly selects
`--device RPC0,ROCm0`. Its worker admission path performs the pinned HFXCAP2
readiness exchange and then runs a pinned pre-allocation placement probe before
the canary model command can start. Both the pinned L17 placement probe and the
pinned L17 distributed-state canary are built from the same source tree. The
probe uses the actual common parser and the same layer resolver as that
canary's model loader.

For the target contract it admitted only:

- endpoint `10.44.0.1:50190` in the disposable exercise;
- selected devices `RPC0`, `ROCm0` and backends `RPC`, `ROCm`;
- exact `layer` / `1,1` configuration;
- nonzero sane memory on both devices; and
- 62 repeating layers resolved 32/30, with output on ROCm0, for 32/31 total.

The self-test matrix passed correct, omitted-device, wrong-order, one-device,
and all-RPC predicted ownership cases. Fifteen runner tests passed, including
explicit command freezing, placement refusal before PID/listener admission,
and malformed evidence refusal. Both isolated ROCm/RPC builds completed.

## Real-host exercise

[MEASURED] Production remained continuously active:

| Authority | Before and after L17 |
|---|---|
| nimo-1 coordinator | PID 2144857, port 8081, HTTP 200, `NRestarts=0` |
| nimo-2 worker | PID 1305879, port 50052, `NRestarts=0` |

One nimo-1 disposable worker used port 50190 and a fresh protected key/root.
HFXCAP2 admitted RPC 4.0.1, state 1.0, rank/world 1/2, generation 17, command
mask 31, and the frozen request/response/component/object limits. The placement
probe then returned the exact device/backend/endpoint/split/ownership tuple.

The small-model exercise used the existing 19,077,344-byte
`stories15M-q4_0.gguf`, SHA-256
`66967fbece6dbe97886593fdbb73589584927e29119ec31f08090732d1861739`,
with the same explicit `RPC0,ROCm0`, layer, `1,1` ordering. Two noninteractive
24-token runs at seed 1234 and temperature zero produced byte-identical 96-byte
output, SHA-256
`d8eacd573d450a766f51b504452260e3363a0a1c3d9abd8f8123765b4afc231d`.

The loader offloaded 7/7 layers and reported nonzero allocations on both
selected devices:

| Buffer class | RPC0 | ROCm0 |
|---|---:|---:|
| model | 2.15 MiB | 10.41 MiB |
| KV | 2.25 MiB | 1.12 MiB |
| compute | 2.60 MiB | 15.91 MiB |

The worker recorded three bounded RPC allocations per exercised load:
2,253,824, 2,359,296, and 2,722,304 bytes, totaling 7,335,424 bytes. No file
was created in the worker state root and no state operation was invoked.

One diagnostic warning assigned layer 4 to ROCm0 while an unsupported automatic
Flash Attention tensor fell back to RPC0; the runtime disabled automatic Flash
Attention for this tiny fixture. This is preserved as residual evidence and is
not generalized to the primary configuration.

## Corrections and cleanup

The first disposable setup was refused before readiness because a local
test-key generator produced literal escape characters and failed its own exact
130-byte assertion. No key was installed. A later `llama-cli` exercise was
terminated because that binary remained interactive; the noninteractive
`llama-completion` target was then used for the admitted exact-output runs.
Neither correction touched production.

Both disposable units are `not-found`/inactive with PID 0; port 50190 is
closed; both keys, the empty worker root, source trees, builds, and staging
evidence directory are absent. The protected reviewed raw archive remains on
nimo-2 at
`/var/tmp/halofpx-l17-device-placement-evidence-20260721-v2.tar.zst`, mode
0600, 18,681 bytes, SHA-256
`e229ee0df08d368087dfee11c19ae24cb3e5249f4ac398f2dfd01b4a6d42ceeb`.
Its 34-file checksum manifest and zstd integrity check passed. It adds exact
one-argument-per-line placement/small-model invocation records, retained
negative self-test output, and common-source L17 canary/probe identities. The
original v1 archive is preserved as superseded evidence rather than
overwritten.

## Residual uncertainty

L17 proves executable device discovery, target layer-resolution authority, and
a real small-model two-device load. It does not allocate or inspect the 159.9
GB artifact and therefore cannot prove its eventual allocation shapes or
capacity fit. P01 and P11 are consistent supporting evidence because their
successful exact-model runs explicitly used `RPC0,ROCm0`; L16's missing device
argument remains the leading explanation for its monolithic RPC0 request, not
a proven sole cause. A primary retry requires a new Project Lead decision.
