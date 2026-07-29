# L93 production reconciliation and cleanup

The accepted L92 baseline was exact before the transition. The L93 controller
then performed coordinator-first shutdown, worker shutdown, and after the
terminal child refusal recovered worker first and coordinator second.

Final production authority:

- coordinator: PID `2932494`, InvocationID
  `d3c9d328bdb940dba62ea3ad1e93be40`, NRestarts `0`,
  Result/ExecMain clean, unique listener 8081, HTTP 200 body
  `{"status":"ok"}`;
- worker: PID `2115484`, InvocationID
  `bcf04aa0d57e40f59f1f56531b0aaa99`, NRestarts `0`,
  Result/ExecMain clean, unique listener 50052.

Installed production identities remained unchanged:

- coordinator executable:
  `d62ab220a4743a347461c958ce99a701e7ed21a938d9ab033334d9fb77fabbdb`
- coordinator launcher:
  `ad2bba0fb16595d29c9acf08ccc79d62015ef50e3b735b3995283c14e9249704`
- coordinator unit:
  `f57b99c7e5e17583d0cb671a675dc045577d32d855e897c3592ea58f2e3949f4`
- worker executable:
  `cf0f39231fdab6b30254959edbb8de0c36cde2312cf4ee6761cfc27a3267bf63`
- worker launcher:
  `7385c9e572594ea82ac4baf879812e572768d26c80a229d27dd3344b21e4789e`
- worker unit:
  `15f71297a26b6690d2860e8acc9bcf1c86f8cea3a256e33d50242b42848d44e5`

The recovery journal contains no kernel OOM, failed activation, automatic
restart, or continuing fault. The temporary HTTP 503 response was exactly the
bounded model-loading interval and terminalized at HTTP 200.

All exact L48/L93 disposable user units are not-found, inactive/dead, MainPID
0, with no fragment or listener. All exact keys, source archives/directories,
builds, worker/coordinator/rendezvous roots, device-gate paths, and remote
evidence staging paths are absent. No production service was reconfigured and
no counter was reset.
