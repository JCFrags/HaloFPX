# Communicator creation and failure semantics

## Creation

One rank calls `ncclGetUniqueId`; the ID is distributed out of band; every rank calls `ncclCommInitRank` or the configured variant with a unique rank in `[0, N-1]`. Initialization synchronizes the clique. The control/rendezvous channel is an application responsibility.

## Nonblocking and asynchronous errors

For failure experiments, create communicators with `config.blocking = 0` and poll `ncclCommGetAsyncError`. Avoid relying exclusively on `hipStreamSynchronize` after peer loss because the active fault-tolerance documentation warns that it can hang indefinitely.

## Version matrix

| Mechanism | 2.27.7 / ROCm 7.2.x | active 2.30.4 | Boundary |
|---|---:|---:|---|
| `ncclRemoteError` | Yes | Yes | Remote process exit/network error result. |
| `ncclInProgress` + async poll | Yes | Yes | Core nonblocking state. |
| `ncclCommAbort` | Yes | Yes | Destroys communicator; application recreates. |
| `ncclCommShrink` | Yes | Yes | Surviving ranks create a smaller communicator. |
| `ncclTimeout` | No | Yes | Active result; tested paths are not a universal Socket deadline. |
| `ncclCommRevoke` | No | Yes | Aborts in-flight work while preserving parent for recovery. |
| `ncclCommGrow` | No | Yes | Adds ranks using a single-use out-of-band grow ID. |

## Recovery interpretation

No audited mechanism transparently reconnects the same failed transport and resumes the old collective. Stable recovery is abort/recreate, with shrink where applicable. Active recovery can also revoke/shrink/grow, but membership decisions and replacement process coordination remain application-level. Buffers from aborted or revoked in-flight collectives must be treated as invalid/undefined.
