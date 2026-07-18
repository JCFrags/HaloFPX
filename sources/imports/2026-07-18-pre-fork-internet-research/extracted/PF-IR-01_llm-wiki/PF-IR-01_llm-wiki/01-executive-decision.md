# Executive decision

## Disposition

[RECOMMENDATION] **HOLD the standard security release.**

[MEASURED] The candidate source contains the material guards for all 13 captured current llama.cpp repository advisories, including the corrected post-disclosure RPC pointer handling.

[MEASURED] The candidate release workflow still passes `-DGGML_RPC=ON` and builds `llama-server`. This converts dormant source into distributable attack surface.

[VERIFIED] Upstream's current security policy explicitly says not to use the RPC backend, `ggml-rpc-server`, or `llama-server` on untrusted networks.

[OPEN] The declared deployed-source commit is not proof of the deployed binary, its compile definitions, loaded libraries, effective listener addresses, endpoint authentication, or network reachability.

## P0 release-unblocking controls

| Label | Control | Pass criterion |
|---|---|---|
| [RECOMMENDATION] | Deny RPC in the standard release | Standard workflow does not set `GGML_RPC=ON`; no RPC executable or RPC backend symbols are shipped. A separately named lab artifact may exist only under an explicit exception. |
| [RECOMMENDATION] | Fail closed for HTTP exposure | Default bind is loopback; non-loopback start requires an explicit exception and nonempty API key or an authenticated reverse proxy/mTLS boundary. |
| [RECOMMENDATION] | Disable administrative/state endpoints by default | `/slots`, properties, metrics, and similar operational routes are opt-in; the fork default `endpoint_slots=true` is changed to false. |
| [RECOMMENDATION] | Prove candidate artifact provenance | Local manifest records source commit, dirty state, submodule commit, compiler, definitions, linked libraries, artifact hashes, and reproducible build recipe. |
| [RECOMMENDATION] | Prove negative reachability | Local inspection shows only approved listener addresses; no RPC listener; protected routes reject missing/invalid credentials; firewall/namespace controls match policy. |
| [RECOMMENDATION] | Pass safe regression suite | Static sentinels and isolated ASan/UBSan negative tests pass with no exploit corpus and no live-service probing. |

## Source-backport disposition

[MEASURED] No missing published llama.cpp memory-safety fix was identified in `ROCmFPX@61f2f2d7bc4955e9bca821095ef69125837133b5`.

[RECOMMENDATION] The required backports are therefore primarily **build and policy changes**: remove standard-release RPC, restore upstream's slots-off default, require explicit authentication for any non-loopback HTTP use, and add release sentinels.

[RECOMMENDATION] If any maintained branch lacks the corrected RPC change, use `ba38f3becce7d1283585c73d796eb47d72bbbd30` semantics. Do not cherry-pick only the emergency `39bf0d3c...` behavior because it rejected legitimate null-buffer CPU graph nodes.

[OPEN] The official RPC advisory still lists no patched version. Treat commit-level mitigation as source evidence, not an upstream declaration that RPC is safe to expose.
