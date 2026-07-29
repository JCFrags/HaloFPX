# L98 production and cleanup reconciliation

The authorized controller stopped production cleanly at 21:44 and recovered it
worker-first/coordinator-second at 21:53. Journals show deactivation success,
then one clean new service activation per unit, with no failure, OOM, or
automatic restart. The service-local restart counters therefore reset from the
prior activation's accepted value 1 to 0.

Observed healthy post-attempt authority:

- coordinator: PID `3011904`, InvocationID
  `6e0b94e5c1ac46949e1b00946e3bb64e`, NRestarts `0`,
  active/running/result success, exact production argv, executable
  `d62ab220a4743a347461c958ce99a701e7ed21a938d9ab033334d9fb77fabbdb`,
  unique listener 8081, HTTP 200 body `{"status":"ok"}`
- worker: PID `2142476`, InvocationID
  `eb9bc3c6165346aeb871fb09d26753ef`, NRestarts `0`,
  active/running/result success, exact production argv, executable
  `cf0f39231fdab6b30254959edbb8de0c36cde2312cf4ee6761cfc27a3267bf63`,
  unique listener 50052

All L98/L48 disposable units, source roots, source archives, keys, controller
staging paths, and evidence staging paths are absent. The two exact temporary
build archives and the exact L98 build logs created for staging were separately
removed after reference/mount/process checks. Production units were not
restarted, reset, or reconfigured during reconciliation.

