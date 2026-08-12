# Safe negative-regression plan

## Safe-lab prerequisites

| Label | Requirement |
|---|---|
| [RECOMMENDATION] | Disposable VM or container plus a separate network namespace; no route to production or the public internet. |
| [RECOMMENDATION] | Loopback-only listeners; no port forwarding; host firewall denies ingress/egress for the lab namespace. |
| [RECOMMENDATION] | Unprivileged dedicated UID, empty environment, no cloud credentials, no SSH agent, no host secrets, read-only source/model mount. |
| [RECOMMENDATION] | ASan/UBSan build for CPU paths; strict CPU, memory, file-size, process, and wall-clock limits. |
| [RECOMMENDATION] | Tiny trusted or locally generated non-exploit fixtures; never download or run public PoCs. |
| [RECOMMENDATION] | Snapshot/rollback before each parser test; capture exit code, sanitizer log, peak RSS, opened files, and sockets. |
| [RECOMMENDATION] | Tests target unit seams where possible. Socket tests use only a fresh local process in the isolated namespace. |

## Test catalog

| Label | Test ID | Class | Scope | Safe stimulus | Pass criterion |
|---|---|---|---|---|---|
| [RECOMMENDATION] | PF-STATIC-001 | Source sentinel | All snapshots | Scan for corrected RPC null-buffer/data-pointer semantics. | Required patterns present; no vulnerable fallback pattern selected. |
| [RECOMMENDATION] | PF-STATIC-002 | Source sentinel | All snapshots | Scan GGUF for checked cumulative arithmetic and tensor representability. | Required guards present. |
| [RECOMMENDATION] | PF-STATIC-003 | Source sentinel | All snapshots | Scan vocab for checked special-token access and both INT32_MAX guards. | Required guards present. |
| [RECOMMENDATION] | PF-BUILD-001 | Build policy | Standard release | Inspect CMake cache, build logs, and binary list for RPC. | RPC option off; no RPC executable/backend symbols. |
| [RECOMMENDATION] | PF-BUILD-002 | Build policy | Standard release | Inspect server defaults and route flags. | Slots/properties/metrics administrative endpoints default off. |
| [RECOMMENDATION] | PF-LISTEN-001 | Negative reachability | Disposable host/network namespace | Start approved HTTP profile and inspect local sockets only. | Only approved loopback/Unix socket is listening; no RPC port. |
| [RECOMMENDATION] | PF-AUTH-001 | HTTP auth | Loopback lab | Request a protected route with no credential and an invalid credential. | 401/403; no state mutation. |
| [RECOMMENDATION] | PF-AUTH-002 | HTTP public route | Loopback lab | Request health/model-list public routes. | Only documented minimal data returned. |
| [RECOMMENDATION] | PF-HTTP-001 | n_discard | Tiny trusted model; ASan/UBSan | Reach context-full state with negative n_discard. | 4xx or clamped behavior; no crash or sanitizer finding. |
| [RECOMMENDATION] | PF-HTTP-002 | Header limits | Local parser/unit seam | Cross configured header-count and aggregate-size limits with bounded data. | Connection/request rejected and memory released. |
| [RECOMMENDATION] | PF-HTTP-003 | Body limits | Local parser/unit seam | Cross configured chunked/body aggregate limit with bounded data. | Read stops at limit; no unbounded growth. |
| [RECOMMENDATION] | PF-RPC-001 | GRAPH_COMPUTE validation | RPC unit seam only | Compare legitimate CPU-buffer node with inconsistent null-buffer/nonzero-data descriptor. | Legitimate case accepted; inconsistent descriptor rejected before graph construction. |
| [RECOMMENDATION] | PF-RPC-002 | GET/SET/COPY bounds | RPC unit seam only | Exercise exact-end and one-byte-outside ranges. | In-range succeeds; out-of-range rejects; guard bytes unchanged. |
| [RECOMMENDATION] | PF-RPC-003 | Type validation | RPC unit seam only | Enumerate invalid/deprecated tensor types. | All invalid/zero-block-size types reject without division or allocation. |
| [RECOMMENDATION] | PF-GGUF-001 | Arithmetic boundaries | Local parser unit seam; ASan/UBSan | Bounded counts/dimensions around every multiplication/addition limit. | Early deterministic error; no allocation based on wrapped value. |
| [RECOMMENDATION] | PF-GGUF-002 | Truncation matrix | Local parser; generated bounded fixtures | Truncate after each header/string/array/tensor field. | Clean error; no leak, invalid free, or OOB. |
| [RECOMMENDATION] | PF-VOCAB-001 | Special token bounds | Tiny synthetic vocab | Set each special-token ID just outside vocab size. | Clean error; no unchecked access. |
| [RECOMMENDATION] | PF-VOCAB-002 | Signed size boundaries | Mocked size provider | Return INT32_MAX and INT32_MAX+1 token-piece/result sizes. | Documented error path; no huge allocation. |
| [RECOMMENDATION] | PF-STATE-001 | State header/truncation | Local state parser; trusted tiny model | Wrong magic/version and truncation at each field. | Load returns failure/zero; process state remains usable. |
| [RECOMMENDATION] | PF-STATE-002 | Token capacity | Local state parser | Declared token count above caller capacity. | Rejected before token read. |
| [RECOMMENDATION] | PF-STATE-003 | Model/state mismatch | Two tiny trusted models | Load state from a different architecture/model context. | Rejected; current context remains intact. |
| [RECOMMENDATION] | PF-CACHE-001 | Cache replacement | Owner-only temporary filesystem | Replace/truncate a cache file between index and load under controlled same-UID test. | Entry rejected; no partial state mutation. |
| [RECOMMENDATION] | PF-CACHE-002 | Symlink/reparse | Disposable filesystem namespace | Pre-create namespace/base symlink and exercise initialization. | Implementation refuses or confines path to approved root. |
| [RECOMMENDATION] | PF-DEPS-001 | cpp-httplib TLS | Local TLS fixtures only | Invalid certificate/IP and cross-origin redirect cases. | Verification fails; credentials do not cross origin. |
| [RECOMMENDATION] | PF-PROV-001 | Artifact provenance | Release artifact | Recompute source/build/library/artifact manifest. | All hashes and expected flags match signed provenance. |

## Prohibitions

[VERIFIED] The plan does not require exploit payloads, shellcode, arbitrary-address primitives, public malicious models, or probing an existing service.

[RECOMMENDATION] Abort a test immediately on sanitizer error, unexpected child process, unexpected file access, non-loopback socket, limit breach, or persistence outside the disposable workspace.
