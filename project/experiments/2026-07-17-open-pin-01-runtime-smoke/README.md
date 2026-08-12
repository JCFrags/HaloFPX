# OPEN-PIN-01 small-model runtime smoke — 2026-07-17

Status: `PARTIAL PASS — SMALL-MODEL ROCm0 SMOKE COMPLETE`

## Purpose

Advance only after both commits pass the matched build/reference lane. Run isolated alternate-port server smokes using the same local Qwen3 4B Q8 model, first on nimo-2 and then on nimo-1. Compare control versus candidate with identical command, model hash, prompt, seed, context, sampling, cache types, and backend.

This smoke does not qualify long-context bounded staging, MTP, distributed RPC, quality, performance, or production deployment.

## Intended scope

- Model: `/opt/llm-usb4-cluster/models/qwen-official/qwen--qwen3-4b-gguf__bc640142c66e1fdd12af0bd68f40445458f3869b/Qwen3-4B-Q8_0.gguf`; hash must be verified on both nodes before launch.
- Runtime: isolated control and candidate binaries under `/home/connorb/halofpx-lab/open-pin-01/`.
- Listener: loopback-only alternate port; never `8082`.
- Backend: explicit `ROCm0`; no RPC.
- Workload: fixed prompt, seed, greedy output, one slot, small context, then a TurboQuant-cache variant only if the produced binary's help and type enumeration prove the exact accepted option spelling.
- Evidence: complete server log, argv/environment, health/props/slot response, request/response, output tokens/text, memory/GPU telemetry, exit status, artifact hashes, and cleanup.

## Stop rules

Stop on model-hash mismatch, load failure, unsupported/fallback ambiguity, output divergence, non-finite/error result, listener collision, `MemAvailable < 32 GB`, swap growth above 2 GiB, GPU edge temperature above 95 C, new kernel/GPU/filesystem error, dirty teardown, or any change to deployed services/configuration.

## Rollback

Terminate only the exact experiment PID, verify its loopback port and `/dev/kfd` ownership are gone, preserve logs, and leave the deployed model services inactive/enabled until the broader local work finishes.

## Outcome

Eight isolated runs completed: control and candidate with F16 and Turbo4 K/V caches on both target nodes. See [RESULTS.md](RESULTS.md). The run proves only small-context load/request/teardown behavior; it exposed a reproducible F16-versus-Turbo4 output divergence that requires a real quality matrix before Turbo4 can be admitted.
