# Local evidence collection

Run on the measured target from a trusted administrative shell:

```bash
sudo ./collect-local-evidence.sh /secure/path/PF-IR-09-local-evidence
```

The script is read-only with respect to firmware and device controls. It does not refresh LVFS metadata, install updates, write NVM, authenticate images, reset devices, inject errors, mount debugfs, or run stress tests. Root access improves DMI, logs, NVMe and debugfs visibility.

The output contains sensitive inventory such as serials, UUIDs, topology, Secure Boot state and firmware versions. Review before sharing. The script creates `files.sha256` for integrity verification.
