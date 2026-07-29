# L97 production and cleanup receipt

The Lead accepted the following healthy production authority as the new
observed baseline after exact journal attribution:

- coordinator: PID `2989515`, InvocationID
  `49d23af81c5d495b80e3c9c906f72c7a`, `NRestarts=1`,
  active/running/result success, unique listener `8081`, HTTP 200, unchanged
  argv/config, executable SHA256 beginning `d62ab220`
- worker: PID `2135516`, InvocationID
  `7a3c97b846854036acd33421bb45ab73`, `NRestarts=1`,
  active/running/result success, unique listener `50052`, unchanged argv/config,
  executable SHA256 beginning `cf0f3923`

The worker was kernel-OOM-killed at 21:08:42 and restarted once by its existing
policy at 21:08:45. The coordinator aborted after RPC loss at 21:08:47 and
restarted once by its existing policy at 21:08:52. Journals showed no continuing
fault. Neither production service was restarted, reset, or reconfigured during
terminal closeout.

Exact disposable remote cleanup had completed before this receipt. No remote
cleanup remained pending.

