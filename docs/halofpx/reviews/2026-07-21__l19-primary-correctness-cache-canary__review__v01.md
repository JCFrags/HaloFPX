# Independent review: L19 guarded primary correctness/cache canary

Date: 2026-07-21

Verdict: **REJECT BEFORE PRODUCTION MUTATION**

## Findings

1. **P1 — controller/child identity mismatch.** The committed controller is
   still bound to L16 channel-key paths, disposable unit names, and child
   binary. An L19-only child update would fail protected-key revalidation after
   shutdown, while rollback cleanup would not have complete authority over the
   new disposable units.
2. **P1 — the sequence is not one material load.** Every canary mode starts a
   fresh process. The C++ canary initializes the model before branching on
   capture, cold, or restore, so the requested capture/cold/restore/fallback/
   runtime-off sequence would load the primary model six times.
3. **P2 — failure evidence is late.** The inherited runner collects the
   PID-bound worker journal and closing disk statistics only after successful
   capture. An allocation refusal or OOM can therefore lose material evidence.

The retained focused tests pass, but they do not change these source-level
facts. The reviewer explicitly rejected invoking the real transition and
recommended closing L19 under the literal one-load authorization rather than
weakening the gate. Production remained untouched and healthy.
