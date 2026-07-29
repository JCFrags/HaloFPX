# L96 production and cleanup

The package gate refused before `controller.preflight()` and before any
production mutation. The accepted baseline remained continuously unchanged:

- coordinator PID `2962825`, InvocationID
  `c781d4778c4b4d5489187a9e6658afc0`, active/running/result success,
  NRestarts `0`, unique port 8081 listener, HTTP 200;
- worker PID `2128643`, InvocationID
  `e7e16a1a4b884ffeb22d54f89cad398e`, active/running/result success,
  NRestarts `0`, unique port 50052 listener.

No disposable unit, key, model process, or 50248/50249 listener was created.
After the terminal refusal, the exact nimo-1/nimo-2 staged source trees and
archives and the possible L96 receipt path were checked for mount/live-file
references, removed, and proven absent. Local reproducible source/build and
stdout/stderr debris was removed only after its exact hashes and sizes were
retained.
