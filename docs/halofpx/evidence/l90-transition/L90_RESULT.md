# L90 terminal result

Status: **NOT PROMOTED**

The reviewed controller prerequisite passed before production mutation. Its
closed L48/L89/L90 unit and path set was exactly absent, with
`production_mutation_started=false`.

Exactly one primary transition was consumed. The primary canary loaded the
pinned artifact and completed the first authenticated 512-token warmup decode.
The response custody contains the exact paired two-record client success and
seven-record server success, and the server authority was authenticated and
retained. The attempt then stopped before workload, capture, restore, or token
generation because a subsequent child-owned cleanup call refused:
`transient unit guard authority is outside the closed manifest`.

No retry or runtime correction was made. L90 therefore provides no reference
token, restored token, represented-state equality, or cache correctness
conclusion.

The controller recovered worker first and coordinator second. A final bounded
invocation of the reviewed prerequisite unloaded the remaining exact
active/exited, MainPID 0 first-chunk transient unit and proved the full closed
disposable set absent.

Final production authority:

- coordinator PID 2882360, InvocationID
  `e7977bbefee74bf3b33787390f390811`, NRestarts 0, unique port 8081,
  HTTP 200 with `{"status":"ok"}`;
- worker PID 2074808, InvocationID
  `1f45a635e7154eeca44fe081b24ac6b9`, NRestarts 0, unique port 50052.

Named unit definitions, executable arguments, model configuration, and cache
configuration remain unchanged.
