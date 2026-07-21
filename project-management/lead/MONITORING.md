# Monitoring and Intervention Policy

## Cadence

- Prefer event-driven checks when the worker marks a milestone complete, rejects
  a candidate, reports a blocker, requests authority, or changes production.
- Major implementation milestones should end the worker's current task turn.
  Task completion/attention is the reliable native wake event; commentary alone
  is progress evidence but does not wake a thread listener.
- Use the 30-minute heartbeat only as a durable fallback when no event arrives.
  Take one snapshot per heartbeat rather than opening an internal polling loop.
- Predict the next useful inspection from the phase and observed pace: roughly
  30–45 minutes for source/build/focused-test work, 60–90 minutes for model load
  or bounded runtime qualification, and multi-hour sleep for a stable long run.
- During ordinary healthy work, check no more frequently than every 30–60
  minutes. During long stable work, prefer multi-hour sleeps.
- Never poll at 30-second intervals.

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
- the same blocker recurs across three management checks without a new approach;
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
