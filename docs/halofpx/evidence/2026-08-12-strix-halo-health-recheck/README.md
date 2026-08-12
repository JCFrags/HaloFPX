# Strix Halo service health recheck — 2026-08-12

Claim scope: a bounded read-only service-health observation collected through
the saved OpenSSH aliases `nimo-1` and `nimo-2` at
`2026-08-12T23:06:08Z`. No service, process, file, package, model, or host
configuration was changed.

## Result

- **[MEASURED]** `nimo-1` reported
  `minimax-m27-q6-server.service` as `active/running`, PID `3027112`,
  InvocationID `e6da1fe637144cb394119959c0e88736`, and `NRestarts=0`.
  PID `3027112` owned the listener on `0.0.0.0:8081`, and
  `GET http://127.0.0.1:8081/health` returned `{"status":"ok"}`.
- **[MEASURED]** `nimo-2` reported
  `minimax-m27-rpc-worker.service` as `active/running`, PID `2148915`,
  InvocationID `3480c89086e04d5d80060366c5c7ab7f`, and `NRestarts=0`.
  PID `2148915` owned the listener on `0.0.0.0:50052`.
- **[MEASURED]** Both hosts reported kernel `7.1.3-1-cachyos` during this
  health observation.

These values match the service identities in the earlier
[`2026-08-12-strix-halo-live-authority`](../2026-08-12-strix-halo-live-authority/README.md)
receipt. This recheck does not replace that broader inventory.

## Deliberate limitations

This is a health-only receipt. It does not re-audit the operating-system
release, packages, firmware, hardware, boot configuration, systemd unit-file
contents, launcher contents, executable or library hashes, model bytes, cache
contents, network policy, or performance. `ExecStart` and `FragmentPath` are
retained only as systemd-reported routing fields. The RPC worker has no HTTP
health route in this check; its evidence is the exact unit state and listener
ownership.

The active deployment remains a comparison and rollback service. This receipt
does not show that HaloFPX source, a ROCmFPX GGUF, PR #30, or any performance
candidate is deployed.

## Retained records

- [`commands.md`](commands.md) records the read-only command shape and SSH
  options.
- [`nimo-1.txt`](nimo-1.txt) and [`nimo-2.txt`](nimo-2.txt) retain the selected
  command output without reinterpretation.
- [`SHA256SUMS`](SHA256SUMS) binds this README and the three records above.

No runtime or performance claim follows from this receipt.
