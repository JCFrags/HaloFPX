# L95 result

Disposition: **NOT PROMOTED; the single authorized attempt was consumed and
must not be retried.**

## Focused source result

- Accepted base: `4e4245f447928bd7ae8d63a1d0ef330ec2c0cc64`.
- Restore-canary launch authority is now obtained from
  `capture_disposable_unit_authority()` and its cursor, PID, and InvocationID
  are revalidated against later systemd state. Launch stdout/stderr remain raw
  evidence only.
- Alternate shared-listener ownership now requires one exact cgroup-v2
  `/proc/<pid>/cgroup` entry in `0::/absolute/path` form and cross-binds the
  normalized path to systemd ControlGroup, unit suffix, PID, InvocationID,
  host, port, and the closed manifest tuple.
- Focused qualification: 60 tests passed; `py_compile` and `git diff --check`
  passed.
- Independent pre-runtime review: PASS, no P1/P2, safe for the one authorized
  attempt.

Identity:

- child SHA256:
  `4f2ccfae4479d26039be5941b02b65017b7b4a72a5743993b06aedf412be4dd9`;
- source root:
  `236eec6e1117827541b2063a4dca6b064721540a7e6002e6244abf013dec8d20`;
- build ID:
  `3d8b9e11af8a51bef7fd61afcea178223c210ccbb7d651d21c2fb8de6e37e0ce`;
- source archive: 167352320 bytes, SHA256
  `6c27f49f23c83bc29d1a145c1f1a9f9b237d204b0604eed48a71b5a7c04ab7da`;
- build archive: 237393920 bytes, SHA256
  `c961f78ee4f0923f8cf1df06f7aa52d4981dd77154410334ff099aa9a5aaad0b`;
- worker binary SHA256:
  `439f156ccdb1de1a07fc786b50329572fa3485e3c3e3275749e3f165b258c2e0`;
- canary binary SHA256:
  `ced280877cc4b874b8cbc264dd40f1e4d04c0a453b1ba48aa0ec22bd47b3031c`.

## Runtime terminal boundary

The transition passed its exact source/build/model/capacity/production
preflight, prepared the closed evidence/key environment, shut production down
in the authorized order, and then stopped before model execution. The nimo-2
canary provenance command returned 127:

`libllama-common.so.0: cannot open shared object file: No such file or directory`

The build archive was produced under the nimo-1 source root and extracted
under the differently named nimo-2 source root. Offline binary inspection
proves the canary contains this exact RUNPATH:

`/var/tmp/halofpx-l48-source-nimo1/build-l48/bin:/opt/rocm/lib:`

The copied library and its relative symlinks were present in the archive, but
the nimo-1-only RUNPATH could not resolve them from the nimo-2 path. This is a
source-proven build/staging identity defect, not a model, protocol, cache, or
authority result. The controller also reported
`server-authority:publication_journal_missing` because no real handler attempt
had begun.

No residency-A capture, residency-B restore, token, represented-state
comparison, state-page transport conclusion, or cache correctness conclusion
is credited to L95.

Controller stdout was empty (SHA256
`e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855`);
stderr was 447 bytes (SHA256
`d9dbb54ada2140129139955aaa3522551506a2402f45c5c27ea4196ef498af9a`).
