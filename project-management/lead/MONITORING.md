# Monitoring and Intervention Policy

## Event-driven cadence

- Do not run a periodic project-lead heartbeat or poll active workers.
- A worker must end its current task turn and report when its assigned milestone
  completes, whether the result is promoted or rejected.
- A worker must also report immediately when it needs authority or a material
  decision; production health or rollback is uncertain; a correctness, safety,
  or performance regression appears; scope must expand; or authoritative state
  becomes uncertain after context loss.
- Treat two materially different failed approaches to the same blocker as a
  stuck event. Treat 60 minutes without material progress in source/design/build
  work, or 120 minutes in bounded model/runtime work, as a no-progress event.
- Ordinary healthy progress needs no management check-in. The project lead acts
  only on worker completion/attention events or an explicit user request.
- Every event report must state exact HEAD and worktree status, result evidence,
  production health if touched, and the precise next decision or action needed.

## State check before steering

Before any steering, inspect:

1. current worker status and latest completed/in-progress milestone;
2. HaloFPX HEAD, dirty state, recent commits, and current diff;
3. whether the known-good services are healthy when runtime work occurred;
4. the exact evidence for any claimed correctness or performance result;
5. whether existing steering already covers the situation.

## Steering thresholds

Intervene only when one or more are evidenced:

- unsafe persistence, accepted corruption, correctness divergence, or loss of
  the known-good rollback path;
- a performance candidate is being retained despite a matched slowdown;
- the worker expands repetitive tests or archaeology after the decision is
  already supported and no concrete defect justifies expansion;
- the same blocker survives two materially different attempted approaches;
- work materially leaves the ordered objectives or duplicates a completed lane;
- a required human authority/decision is missing;
- an active task becomes context-polluted enough to contradict current evidence,
  repeat closed work, or ignore steering.

Do not steer merely because a build, model load, benchmark, review, or careful
correctness repair takes time.

## Worker replacement policy

Keep the current worker while it remains evidence-aware, accepts negative
results, restores production, and advances coherent milestones. A multiday
context alone is not sufficient reason to replace it. Replace or split work
only after verified drift/repetition, repeated unresolved blockage, or a clean
bounded handoff point that benefits from a fresh context.
