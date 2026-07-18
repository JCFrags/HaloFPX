# Controlled model unload receipt

## Before

- nimo-1 worker: `minimax-m27-rocmfp4-rpc-worker.service`, active and enabled; RPC server on `10.44.0.1:50052` using `ROCm0`.
- nimo-2 coordinator: `minimax-m27-rocmfp4-dual-server.service`, active and enabled; HTTP server on `0.0.0.0:8082`, health OK, both slots idle.
- Active model SHA-256 was reverified as `9a3743422ffc073430038114872404189d7fe6d2a5a17f01fa3e9e01673b4836`; size `121861632736` bytes.
- Worker executable SHA-256: `7f7cb7f0b2217ed714e32d028c210059d78dc932caf2b1a78055d23b59b99d9a`.
- Coordinator executable SHA-256: `ab9c0275289857811154e17fdffd35bb857ce20a1b0fdcf00e3c85e82de5a479`.
- Preserved nimo-1 RPC cache size: `119688007680` bytes.

## Rollback authority

| Artifact | SHA-256 |
|---|---|
| nimo-1 worker unit | `75af31423fccc07b8c89d811aa7b2192c713322a7736dfb96d3a0bff6a9cd319` |
| nimo-1 worker wrapper | `1e1af079e054654b577b526e521efe8d8480f50feda5967972595b2978ee6809` |
| nimo-2 coordinator unit | `a03572d90304486b83bcb84eef84495e7864efd0bf258c0d1ca0269ae9fc31cb` |
| nimo-2 coordinator wrapper | `259ece0533824abdb27167a5e43d36ef2f3c61891e1bb15491778004c40dd4d8` |
| nimo-2 environment file | `04cc64601e406f1c1eec16363c078c5395375fd33212c5352b5985bf143f3c25` |

## Action

1. Stopped the nimo-2 coordinator first at `2026-07-17T18:01:43-07:00`.
2. Verified port `8082` closed, `/dev/kfd` released, and the unit remained enabled.
3. Stopped the nimo-1 worker at `2026-07-17T18:01:53-07:00`.
4. Verified port `50052` closed, `/dev/kfd` released, and the unit remained enabled.

## After

- Both model services: inactive and enabled.
- Both MPTCP and PM QoS services: active.
- Available memory: approximately `131.47 GB` on nimo-2 and `131.27 GB` on nimo-1.
- Active GGUF still exists at its exact path and size; nimo-1 cache remains exactly `119688007680` bytes.
- No model or cache file was deleted or modified.

## Exact restore order

```bash
ssh nimo-1 'sudo -n systemctl start minimax-m27-rocmfp4-rpc-worker.service'
# Verify service plus 10.44.0.1:50052 before continuing.
ssh nimo-2 'sudo -n systemctl start minimax-m27-rocmfp4-dual-server.service'
# Verify /health, /slots, device order, and an inference smoke test.
```

## Restore verification

- **[MEASURED]** nimo-1 worker started at `2026-07-17T18:46:57-07:00`; unit active/enabled; listener restored at `10.44.0.1:50052`.
- **[MEASURED]** nimo-2 coordinator started at `2026-07-17T18:47:07-07:00` and became healthy after the 121,861,632,736-byte model loaded at approximately `18:48:46-07:00`; unit active/enabled; listener restored at `0.0.0.0:8082`.
- **[MEASURED]** Coordinator startup enumerated local `ROCm0` followed by remote `RPC0` at `10.44.0.1:50052`. `/health` returned `{"status":"ok"}` and `/slots` returned two non-processing 4096-token slots.
- **[MEASURED]** A post-restore distributed completion evaluated 8 prompt tokens and generated 16 tokens at 22.76 tok/s, returned HTTP success with non-empty content, and left both slots non-processing. This is rollback-readiness evidence, not a quality or performance benchmark.
- No experiment process or alternate listener remained. The production model, cache, service files, and deployment paths were not replaced.
