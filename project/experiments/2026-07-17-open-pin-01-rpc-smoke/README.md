# OPEN-PIN-01 candidate RPC smoke — 2026-07-17

Status: `PARTIAL PASS — CANDIDATE-ONLY DISTRIBUTED SMOKE COMPLETE`

## Purpose

Build the candidate with `GGML_RPC=ON` in a separate build directory on both nodes, then run a single small-model layer-split request over USB4 rail A using alternate worker port 50053 and loopback coordinator port 18081. This is a bounded reachability/load/request/teardown check, not tensor parallelism, dual-rail performance, security, fault tolerance, or large-model qualification.

## Stop rules

Stop on source dirtiness, less than 30 GB free on nimo-1, model mismatch, unexpected listener, deployed-service start, wrong device ordering, load/request failure, swap growth above 2 GiB, `MemAvailable` below 32 GB, GPU edge above 95 C, kernel/GPU/storage warning, or dirty teardown.

## Rollback

Terminate only the recorded experiment coordinator and worker PIDs, verify ports 18081 and 50053 are closed, preserve raw logs, and leave deployed services inactive/enabled.

## Outcome

The candidate built with RPC enabled on both nodes using identical artifact hashes. A rail-A-only, 1:1 layer-split request completed and both processes tore down cleanly. See [RESULTS.md](RESULTS.md). Security admission, fault behavior, dual-rail use, matched control, and meaningful performance remain open.
