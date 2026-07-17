# Recipe: byte-aware ROCm 7.14 gfx1151 tarball install

**Classification:** official versioned distribution artifact; checksum must be recorded separately  
**Source:** [AMD-INSTALL-714](../sources.md#amd-install-714)

## Pinned inputs

```bash
ROCM_VERSION=7.14.0
ROCM_TARBALL=therock-dist-linux-gfx1151-7.14.0.tar.gz
ROCM_URL=https://repo.amd.com/rocm/tarball-multi-arch/$ROCM_TARBALL
ROCM_PREFIX=/opt/rocm-7.14.0
```

## Secure install behavior

The bundled installer refuses to call an unverified download reproducible unless one of these is true:

1. `ROCM_TARBALL_SHA256` is supplied and matches; or
2. `ALLOW_UNVERIFIED=1` is supplied, in which case the computed hash is printed and saved for provenance.

```bash
sudo install -d -m 0755 /opt/rocm-7.14.0
sudo chown "$USER":"$USER" /opt/rocm-7.14.0

ROCM_TARBALL_SHA256='...' \
ROCM_PREFIX=/opt/rocm-7.14.0 \
  ../../scripts/install-rocm-714-tarball.sh
```

## Verify layout

```bash
find /opt/rocm-7.14.0 -maxdepth 2 -type f \( -name hipconfig -o -name rocminfo -o -name clang \) -print
/opt/rocm-7.14.0/bin/hipconfig --full
/opt/rocm-7.14.0/llvm/bin/clang --version
```

The installer uses a staging directory and recognizes both a single top-level archive directory and a flat archive. It writes `INSTALL-PROVENANCE.txt` containing URL, computed hash, install time, and tool versions.

## Uninstall

The tarball install is isolated. Remove only after checking that no shell, service, or CMake cache references it:

```bash
grep -R '/opt/rocm-7.14.0' "$HOME/.config" /etc/systemd/system 2>/dev/null || true
sudo rm -rf /opt/rocm-7.14.0
```

Do not delete `/opt/rocm` if it belongs to a package-managed installation.
