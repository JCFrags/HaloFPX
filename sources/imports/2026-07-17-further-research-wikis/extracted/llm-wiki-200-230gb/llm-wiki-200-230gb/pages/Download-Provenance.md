# Download Provenance and Integrity

| Candidate | Quant repository | Revision | Selected path | Shards | Integrity record |
|---|---|---|---|---|---|
| Qwen3-235B-A22B-Instruct-2507 | unsloth/Qwen3-235B-A22B-Instruct-2507-GGUF | 160ed54 | UD-Q6_K_XL | 5 | Qwen3-235B-A22B-Instruct-2507-UD-Q6_K_XL-00001-of-00005.gguf: a28eea726d74f2f22c1fa18acd38a2e310185a4c605ee55ecccb551c68d83910 (49846266560 bytes) |
| Step-3.7-Flash | stepfun-ai/Step-3.7-Flash-GGUF | 713961b | Q8_0 | 5 | Run refresh script; revision/path manifest only |
| MiMo-V2-Flash | bartowski/XiaomiMiMo_MiMo-V2-Flash-GGUF | 6b8a0ba | XiaomiMiMo_MiMo-V2-Flash-Q5_K_M | 6 | XiaomiMiMo_MiMo-V2-Flash-Q5_K_M-00002-of-00006.gguf: 1014b460405b8f1db647ce974ebc5ea7c5466ecfd2a486b40eb3c8a749abd8e2 (39809416864 bytes) |
| GLM-4.7 | bartowski/zai-org_GLM-4.7-GGUF | 75abf8a | zai-org_GLM-4.7-Q4_K_M | 6 | Run refresh script; revision/path manifest only |
| Llama-3.1-Nemotron-Ultra-253B-v1 | bartowski/nvidia_Llama-3_1-Nemotron-Ultra-253B-v1-GGUF | 9195f67 | nvidia_Llama-3_1-Nemotron-Ultra-253B-v1-Q6_K | 5 | Run refresh script; revision/path manifest only |
| DeepSeek-R1-0528 | unsloth/DeepSeek-R1-0528-GGUF | main-observed-916bb7c | UD-IQ2_M | 5 | Run refresh script; revision/path manifest only |
| Llama-3.1-Tulu-3-405B | bartowski/Llama-3.1-Tulu-3-405B-GGUF | main | Llama-3.1-Tulu-3-405B-IQ4_XS | discover | Run refresh script; revision/path manifest only |
| MiniMax-M3 | unsloth/MiniMax-M3-GGUF | 41b3ee5f52f642949301cb1fc34cf8379ba22416 | UD-IQ4_XS | 6 | Run refresh script; revision/path manifest only |
| Kimi-K2-Thinking | mradermacher/Kimi-K2-Thinking-i1-GGUF | main |  | 5 | Run refresh script; revision/path manifest only |

## Provenance chain

For each candidate, preserve:

1. upstream model repository and license;
2. quantizer/converter repository;
3. immutable revision where available;
4. selected quant folder/file pattern;
5. exact file byte size and LFS SHA-256/OID;
6. local SHA-256 after download;
7. runtime commit used for the load test.

The `manifests/` directory records the observed repository/revision/path and any exact LFS pointer captured during research. The known hashes are partial by design; missing hashes are not guessed.

## Refresh exact Hugging Face manifests

```bash
python scripts/refresh_hf_manifests.py --all
```

The script queries Hugging Face's tree API with `expand=true`, writes exact byte sizes and LFS OIDs, and resolves the repository commit when the service exposes it. Review diffs before accepting a moved `main` revision.

## Verify downloaded files

```bash
python scripts/verify_downloads.py --download-root /models --manifest-dir manifests/refreshed
```

A model is not approved until every shard matches its manifest. Do not combine shards from different revisions.

## Authoritative versus community artifacts

Official publisher GGUFs are preferred where they exist (Step). Bartowski and Unsloth are community quantizers; their repositories are widely used but remain separate provenance domains from the model publisher. Kimi's community IQ1_M is screened out partly because both quant severity and conversion provenance are weak.
