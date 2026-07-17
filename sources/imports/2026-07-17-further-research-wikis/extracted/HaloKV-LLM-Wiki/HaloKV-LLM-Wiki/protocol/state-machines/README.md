# HaloKV protocol state machines

The `.mmd` files contain Mermaid source suitable for GitHub, Obsidian, Mermaid CLI, or documentation tooling.

- `coordinator.mmd` — authority-backed operation lifecycle.
- `rank.mmd` — rank connection, snapshot, prepare, attach, fence, and quarantine states.
- `reconnect.mmd` — authoritative reconnect decision tree.
- `cancellation.mmd` — commit/cancel race and terminal outcomes.
- `commit-sequence.mmd` — normal coordinated checkpoint sequence.
- `degraded-mode.mmd` — service degradation and recovery transitions.

All state machines are subordinate to the invariants in `wiki/Executive-Summary.md` and should be checked against `formal/tla/HaloKV.tla`.
