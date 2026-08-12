# Strix Halo SHA-256 capability screen — 2026-08-12

Purpose: determine whether a hardware-dispatched SHA-256 provider is a
plausible optimization for the SSD prompt-cache integrity path. This is a
single-thread cryptographic-library capability screen, not an application or
cache performance result.

## Environment

Both CachyOS targets reported:

- AMD Ryzen AI MAX+ 395;
- CPU feature flag `sha_ni`;
- OpenSSL 3.6.3, package `openssl 3.6.3-1.1`, `linux-x86_64`.

Metadata was captured at 2026-08-12T20:48:58Z. The benchmark ran from
2026-08-12T20:49:09Z through 20:49:22Z, serially on nimo-1 and then nimo-2.
The selected metadata output is retained in `metadata.normalized.txt`.

The metadata command, run serially against the two saved SSH aliases, was:

```powershell
$cmd='printf "host=%s\n" "$(hostname)"; lscpu | sed -n "/Model name:/p;/Flags:/p"; openssl version -a 2>/dev/null | sed -n "1,4p"; pacman -Q openssl 2>/dev/null || true'
ssh -o BatchMode=yes -o ConnectTimeout=8 nimo-1 $cmd
ssh -o BatchMode=yes -o ConnectTimeout=8 nimo-2 $cmd
```

The repository record normalizes the host, model, `sha_ni` presence, and
OpenSSL fields needed for this claim. The full flag lines and the local Codex
controller transcript were not promoted as repository authority. All retained
benchmark summary output is reproduced below. `SHA256SUMS` binds the published
record.

## Exact command

From the repository root on the Windows control PC:

```powershell
$cmd='nice -n 19 taskset -c 15 openssl speed -seconds 1 -evp sha256 2>&1 | tail -n 4'
ssh -o BatchMode=yes -o ConnectTimeout=8 nimo-1 $cmd
ssh -o BatchMode=yes -o ConnectTimeout=8 nimo-2 $cmd
```

The process was low priority, pinned to logical CPU 15, and ran one second per
OpenSSL buffer-size cell. Only each host's final four summary lines were
retained. This does not measure the current bundled scalar helper, file I/O,
page-cache effects, cache save/load, or end-to-end TTFT.

## Retained output

nimo-1:

```text
CPUINFO: OPENSSL_ia32cap=0x7ed8320b078bffff:0x19405fdef1bf97ab:0x0000003010000110:0x0000000000000000:0x0000000000000000
The 'numbers' are in 1000s of bytes per second processed.
type             16 bytes     64 bytes    256 bytes   1024 bytes   8192 bytes  16384 bytes
sha256          221901.46k   654925.50k  1450667.01k  2085753.86k  2390351.87k  2416082.94k
```

nimo-2:

```text
CPUINFO: OPENSSL_ia32cap=0x7ed8320b078bffff:0x19405fdef1bf97ab:0x0000003010000110:0x0000000000000000:0x0000000000000000
The 'numbers' are in 1000s of bytes per second processed.
type             16 bytes     64 bytes    256 bytes   1024 bytes   8192 bytes  16384 bytes
sha256          220268.69k   655904.32k  1448084.48k  2088064.00k  2394537.98k  2421407.74k
```

At 16 KiB blocks, OpenSSL reported 2.416 GB/s on nimo-1 and 2.421 GB/s on
nimo-2 in decimal units.

## Interpretation

- **[MEASURED]** OpenSSL EVP SHA-256 can process about 2.42 GB/s on one pinned,
  low-priority logical CPU on each exact target under this short screen.
- **[INFERENCE]** A server-local optional EVP provider may materially reduce
  the CPU time of the current byte-at-a-time SHA helper.
- **[OPEN]** The speedup over the bundled helper and the resulting cache-hit,
  cache-save, or TTFT benefit have not been measured.

The candidate must preserve full-file hashing before rename and before state
apply, exact length and digest semantics, corruption-as-miss, and a portable
feature-off fallback. Kill the candidate on any digest/status mismatch; require
at least a 3x helper-level gain and a clear matched target cache benefit before
promotion.

## Post-screen service state

At 2026-08-12T20:51:00Z, nimo-1's coordinator was active at PID 3027112 with
`NRestarts=0` and HTTP 200; nimo-2's worker was active at PID 2148915 with
`NRestarts=0`. No service or host configuration was changed.
