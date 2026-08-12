# ROCmFPX local model fixture generation scripts

capture_method: GitHub/public-web connector capture
byte_fidelity: line-normalized authoritative connector capture
claim_label: PRIMARY-GENERATION-RECIPE
source_url: https://github.com/charlie12345/ROCmFPX/tree/61f2f2d7bc4955e9bca821095ef69125837133b5/scripts
repository: charlie12345/ROCmFPX
commit_or_revision: 61f2f2d7bc4955e9bca821095ef69125837133b5
repository_path: scripts/build-rocmfpx-agent-fixtures.sh and scripts/run-deepseek-v4-rocmfp4-fixture.sh
git_blob_sha1: n/a
access_date: 2026-07-18

---

- `scripts/build-rocmfpx-agent-fixtures.sh`, blob `ef695709e160abedc61d552703d16338fae6123d`, defaults to a maintainer-local Qwen3-0.6B Q4_K_M path and produces derived GGUF test assets.
- `scripts/run-deepseek-v4-rocmfp4-fixture.sh`, blob `3b7329a757c8150499448eb19e02e1285455d086`, defaults to a maintainer-local `DeepSeek-V4-Flash-180B` source directory and generates/quantizes GGUF outputs.

Neither script identifies a publisher revision, source license file, source hash, quantizer identity, or permission record for the local inputs. The scripts may be considered under the repository code assertion; their generated model outputs are separate and blocked pending exact input provenance.
