# Fault Injection Program

> **Wiki status:** Proposed · **Evidence state:** D0 — design only · **Last reviewed:** 2026-07-17  
> **Machine-validation status:** Not run. Missing measurements are `INSUFFICIENT_EVIDENCE`, never an implicit pass.


> **Safety boundary:** Prefer reversible software faults (`tc netem`, service stop, cgroup limits, interface down) before manual cable removal or node reboot. Never corrupt the only model copy or alter firmware/voltage outside vendor-supported controls.

## Fault families

| Family | Injection examples | Required observation |
|---|---|---|
| Worker process | `SIGTERM`, `SIGKILL`, crash loop, startup failure | Detection, explicit request error, no corrupted continuation, cleanup, restart |
| Coordinator process | graceful stop, `SIGKILL`, restart during load/serve | Client error semantics, state loss boundaries, model reload/recovery |
| USB4 network | link down, 100% loss, 0.1–5% loss, latency/jitter, bandwidth cap, reorder | TTFT/ITL impact, timeout behavior, retransmits, recovery |
| Physical link | cable disconnect/reconnect, port swap where supported | Domain events, renegotiated speed/lanes, service readiness |
| Node | worker reboot, coordinator reboot, clock offset, suspend/resume if supported | Health state, bounded recovery, provenance discontinuity |
| Memory | oversized context, cgroup-limited coordinator, cache pressure | Safe reject/OOM handling, no host lockup, no stale ready state |
| Storage/model | unreadable shard, checksum mismatch, full artifact volume | Fail before ready, explicit diagnosis, no partial serving |
| Client/API | cancel midstream, slow reader, malformed request, burst overload | Resource release, backpressure, protocol correctness |

## Timing boundaries

Default stable targets, subject to stricter SUT SLOs:

- Fault detection: ≤10 seconds.
- Affected request terminates with explicit error: ≤30 seconds.
- Service returns healthy after reversible worker/link restoration: ≤120 seconds, or a documented automated restart occurs inside that bound.
- Three of three injections pass for each mandatory fault/severity.

## Fault acceptance

A fault trial passes only when raw telemetry proves the injection happened at the intended time, the expected state transition occurred, no incorrect output escaped, the service did not claim ready while required resources were absent, and post-recovery canaries match the pre-fault reference.

## RPC security control

Because upstream `llama.cpp` labels RPC proof-of-concept and insecure, bind the listener only to the dedicated point-to-point address, block all other ingress/forwarding, and treat any untrusted exposure as a stable-release blocker unless an independently validated authenticated and encrypted tunnel is used. [[SRC-007]](../references/Sources.md#src-007)
