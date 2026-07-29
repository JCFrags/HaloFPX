# L95 production and cleanup

The controller performed an authorized clean coordinator-first shutdown and
worker-first recovery. Journals show only the clean stop/start boundary, with
no continuing OOM, automatic restart, or service fault.

Recovered production authority:

- coordinator: `minimax-m27-q6-server.service`, active/running/result success,
  PID `2962825`, InvocationID `c781d4778c4b4d5489187a9e6658afc0`,
  NRestarts `0`, unique port 8081 listener, executable SHA256
  `d62ab220a4743a347461c958ce99a701e7ed21a938d9ab033334d9fb77fabbdb`,
  HTTP 200 `{"status":"ok"}`;
- worker: `minimax-m27-rpc-worker.service`, active/running/result success,
  PID `2128643`, InvocationID `e7e16a1a4b884ffeb22d54f89cad398e`,
  NRestarts `0`, unique port 50052 listener, executable SHA256
  `cf0f39231fdab6b30254959edbb8de0c36cde2312cf4ee6761cfc27a3267bf63`.

Unit fragments, ExecStart, process argv, configuration, executable identities,
and listener ownership match the accepted production contract.

All five closed disposable units are not-found/inactive/dead/MainPID 0. Ports
50248 and 50249 have no listener. Every manifest-owned remote key, source
archive/tree, build/worker/coordinator/evidence/rendezvous/device-gate path is
absent. The separately staged L95 build archives/log were removed before the
transition.
