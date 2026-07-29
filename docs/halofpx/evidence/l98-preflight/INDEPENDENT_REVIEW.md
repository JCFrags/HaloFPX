# L98 independent pre-runtime review

Result: **PASS; eligible for the single authorized runtime**.

The first focused review rejected two false-accept paths: ambiguous multiple
`mode` result lines and contradictory systemd terminal tuples. Both were
corrected before runtime. The final reviewer reran 61 focused tests and found no
P1 or P2.

The final source requires exactly one fully consumed canonical result line,
including empty values, and compares its exact map with independently verified
durable JSON. Restore terminal handling is a finite fail-closed state machine:
running identity is stable; success is exactly active/exited, MainPID 0,
ExecMainPID equal to the retained launch PID, code 1, status 0, and result
success; closed failure tuples are retained but cannot pass as success.
InvocationID, PID, cursor, terminal properties, and journal evidence are
cross-bound before explicit stop/unload. Ordinary canary collection retains the
same RemainAfterExit ordering.

