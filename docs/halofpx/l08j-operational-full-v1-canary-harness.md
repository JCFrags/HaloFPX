# L08j operational full-v1 canary harness repair

Status: **current lifecycle contract restored and process canary requalified**

L08j repairs the checked-in L08i process harness after L09 made quota,
filesystem reserve, and a single-entry limit mandatory for the generation-one
authority. The prior harness omitted those arguments, so current server startup
correctly rejected it before executing the canary. This is a test/operator
repair only; no server, provider, format, state, or inference implementation
changed.

The harness now supplies a positive 64 MiB logical quota, a 64 MiB filesystem
reserve, and `max_entries=1` by default. The quota and reserve remain
environment-overridable and their exact values are retained in `tuple.json`.
Direct script execution no longer imports pytest, while pytest collection keeps
the existing environment-dependent skip marker. The HTTP client dependency is
unchanged.

## Linux qualification

On nimo-1, the repaired harness used the already-qualified L09 gated server
binary and the Stories 15M Q4_0 fixture. It completed the real authenticated
HTTP sequence:

1. exact absent-handle restore missed;
2. cold deterministic completion was retained;
3. prompt state was published and returned an explicit manifest handle;
4. a new process authenticated and restored the handle as a hit;
5. restored continuation exactly matched cold continuation;
6. one anchor byte was corrupted and synchronized; and
7. a third process rejected the state and recomputed the same continuation.

The selected manifest was
`e1346d196d085386500b75aee04c3f636aca88ae793d198aeef1b8831494b6a1`.
Cold content SHA-256 was
`d4befa4c08b0bdd9023bfa965064be3c3a8eda8804174fc74c286b2a66710860`.
The disposable operator key was removed; evidence retains only its SHA-256.

The focused feature-off, L02, transformer state/codec, attempt-wire, and Linux
generation-one authority set passed 9/9. Pytest independently collected the
canary after the direct-run import correction. The known-good MiniMax service
remained active with zero restarts and returned HTTP 200.

## Boundary

L08j does not add automatic discovery, exact-prefix or system-prefix matching,
automatic writeback, multiple generations, online eviction, shared scope,
large-state admission, distributed restore, or production persistence. The
160 GB primary model remains a separately qualified performance workload, not
an admitted cache-state workload. Broader fault matrices remain deferred until
one of those product boundaries opens or a concrete defect requires them.

No donor code, GPL llama-ai implementation, CachyLLama code, dependency in the
engine, WebUI, remote, deployment replacement, model mutation, notice, license,
or SBOM change entered this milestone.
