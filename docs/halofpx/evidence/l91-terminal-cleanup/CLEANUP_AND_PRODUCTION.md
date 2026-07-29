# L91 terminal cleanup and production reconciliation

Lead-authorized cleanup was bounded to the exact L48/L89/L90/L91 disposable
unit and path inventory produced by the closed manifest/controller.

Before cleanup, every listed disposable unit on both hosts resolved as
`LoadState=not-found`, `ActiveState=inactive`, `SubState=dead`, `MainPID=0`.
Every exact disposable key/source/build/archive/evidence-staging path resolved
absent. There was therefore no unit unload or filesystem deletion to perform.
No glob, parent-directory cleanup, or production mutation was issued. Repeated
checks proved the same exact absence.

Final read-only production authority:

- coordinator `minimax-m27-q6-server.service`: loaded, active/running,
  Result=success, PID `2896932`, InvocationID
  `d33e57248a4e4eb98f81cc1a44cf1ff6`, NRestarts `1`, exact production
  launcher `/opt/llm-usb4-cluster/bin/run-minimax-m27-q6-server.sh`, unique
  PID-owned listener on `0.0.0.0:8081`, `/health` body `{"status":"ok"}`,
  HTTP 200.
- worker `minimax-m27-rpc-worker.service`: loaded, active/running,
  Result=success, PID `2084398`, InvocationID
  `0137204322234e5e9ddde8a4173ef177`, NRestarts `1`, exact production
  launcher `/opt/llm-usb4-cluster/bin/run-minimax-m27-rpc-worker.sh`, exact
  process `/opt/llm-usb4-cluster/llama/ggml-rpc-server --host 0.0.0.0
  --port 50052`, and unique PID-owned listener on `0.0.0.0:50052`.

This exactly matches the Lead-accepted post-recovery baseline. Counters were not
reset and neither production service was restarted or reconfigured.

The reproducible local `l91-source.tar` was separately re-resolved inside the
HaloFPX workspace, verified as a regular 190,963,200-byte file with SHA256
`2dc9bf0cf63facddc2a9ef7c785639d344bd4a7f22e35292c1d929bcb639a089`,
checked for process-command references, removed by exact literal path, and
re-proved absent. Evidence directories were preserved.
