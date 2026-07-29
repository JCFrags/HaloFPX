# L92 production reconciliation and cleanup

The controller cleanly stopped both accepted production services during the
authorized window:

- coordinator deactivated successfully at 18:16:12;
- worker deactivated successfully at 18:16:13.

Recovery created one fresh activation per unit:

- worker started at 18:21:39;
- coordinator started at 18:21:40.

There are no intervening failure, automatic-restart, or kernel OOM records.
Thus NRestarts=0 is the fresh systemd activation counter, not lost fault
evidence. The Lead-authorized new observed baseline is:

- coordinator: PID 2913255, InvocationID
  `01bdd7c2e7084036a472ec9cea4b3d62`, NRestarts 0, Result/ExecMain clean,
  exact production unit/launcher/argv, unique cgroup and listener 8081,
  health body `{"status":"ok"}`, HTTP 200;
- worker: PID 2099163, InvocationID
  `ac463c354550467bbebbe9c6253144b5`, NRestarts 0, Result/ExecMain clean,
  exact production unit/launcher/argv, unique cgroup and listener 50052.

Executable/config identities:

- coordinator executable:
  `d62ab220a4743a347461c958ce99a701e7ed21a938d9ab033334d9fb77fabbdb`
- coordinator launcher:
  `ad2bba0fb16595d29c9acf08ccc79d62015ef50e3b735b3995283c14e9249704`
- coordinator unit:
  `f57b99c7e5e17583d0cb671a675dc045577d32d855e897c3592ea58f2e3949f4`
- coordinator environment:
  `bf72418470a6dbc20f95d5434c39f829c6c68fb0b02c78357104a18a4eaaee61`
- worker executable:
  `cf0f39231fdab6b30254959edbb8de0c36cde2312cf4ee6761cfc27a3267bf63`
- worker launcher:
  `7385c9e572594ea82ac4baf879812e572768d26c80a229d27dd3344b21e4789e`
- worker unit:
  `15f71297a26b6690d2860e8acc9bcf1c86f8cea3a256e33d50242b42848d44e5`

All exact L48/L89/L90/L91/L92 disposable units were already not-found,
inactive/dead, MainPID 0, with no cgroup or fragment. Every exact closed
disposable key/source/build/archive/evidence/staging path was already absent on
both hosts. Cleanup was therefore a no-op: no unload or deletion was issued.
The final read-only service/listener/HTTP check exactly matched the baseline
above. Neither production service was touched and counters were not reset.

The local reproducible staging archive `l92-source.tar` was resolved inside the
HaloFPX workspace, verified as a regular 193,699,840-byte file with SHA256
`6f8768d31c625fa286a302aff174ca3806fe61567af17242890f56275990445a`,
checked for process-command references, deleted by exact literal path, and
re-proved absent. Evidence directories were preserved.
