# PR #31 — speculative override patch map

- **PR:** [#31](https://github.com/charlie12345/ROCmFPX/pull/31)
- **Base:** `6bf20cd688ba0af882d1f68ba50b292edf646ab4`
- **Head before merge:** `b56ad79d77bc1eb9fe6407640eec8b8edfc04900`
- **Merge:** `25c71fc6e12d73bb3804127e032d29fb8976ae40`
- **Authored commits:** 1
- **Changed paths:** 2
- **Canonical commits:** [PR commits](https://github.com/charlie12345/ROCmFPX/pull/31/commits)
- **Canonical diff:** [PR files](https://github.com/charlie12345/ROCmFPX/pull/31/files)

Primary source: [S-PR31](https://github.com/charlie12345/ROCmFPX/pull/31)

| Exact path | Subsystem | Decision | Inventory rationale |
| --- | --- | --- | --- |
| `tools/server/server-context.cpp` | Server / cache | **REFRESH** | Shared server implementation: use current upstream and re-port disk-cache, strict verification, and request-override deltas. |
| `tools/server/server-task.cpp` | Server / cache | **REFRESH** | Shared server implementation: use current upstream and re-port disk-cache, strict verification, and request-override deltas. |
