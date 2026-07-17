# Operating Runbook

> **Wiki status:** Proposed · **Evidence state:** D0 — design only · **Last reviewed:** 2026-07-17  
> **Machine-validation status:** Not run. Missing measurements are `INSUFFICIENT_EVIDENCE`, never an implicit pass.


## Before a run block

1. Confirm upstream freshness and no untriaged P0/P1 event.
2. Verify model, binary, container, config, dataset, and collector hashes.
3. Record BIOS/firmware/kernel/driver, boot ID, power profile, ambient, link speed/lanes, MTU/offloads, time offset, free memory/disk.
4. Stop unrelated workloads and automatic update/index/backup jobs.
5. Run health and correctness canaries on each node separately.
6. Prepare the declared cache state and prove it.

## During

- Start collectors before the service and retain a post-run tail.
- Abort safely on thermal throttle, GPU reset, kernel oops, uncontrolled memory pressure, data corruption, or loss of required telemetry.
- Do not restart and continue under the same run ID after a process/node discontinuity unless the experiment is explicitly a recovery test.

## After

1. Capture final topology, counters, logs, service status, and time offset.
2. Stop collectors cleanly; checksum raw files.
3. Validate schemas and manifest completeness.
4. Aggregate without editing raw files.
5. Evaluate gates and record `PASS`, `FAIL`, `WARN_RETEST`, or `INSUFFICIENT_EVIDENCE`.
6. File anomalies against the exact upstream/build identity.

## Emergency stop

Stop workload, then service, then collectors; preserve the journal and kernel log before reboot. Do not reboot first unless the host is unresponsive or unsafe.
