# Safe test tools

[VERIFIED] `static_sentinels.py` reads source files and optional local Git metadata only. It does not compile or execute target code and does not open a network connection.

[VERIFIED] `verify_manifest.py` recomputes local SHA-256 hashes only.

[RECOMMENDATION] Example source check:

```text
python3 tests/static_sentinels.py /path/to/ROCmFPX \
  --expected-commit 61f2f2d7bc4955e9bca821095ef69125837133b5 \
  --standard-release
```

[RECOMMENDATION] The current reviewed release workflow is expected to fail `standard_release_rpc_denied` until RPC is removed from the standard artifact.

[OPEN] Regex sentinels are regression tripwires, not semantic proofs. A human review and safe dynamic tests remain required.
