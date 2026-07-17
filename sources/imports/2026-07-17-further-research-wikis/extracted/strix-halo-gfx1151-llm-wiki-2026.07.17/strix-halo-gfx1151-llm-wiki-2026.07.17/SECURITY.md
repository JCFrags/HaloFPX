# Security

## Reporting issues

Do not include credentials, model-access tokens, private network addresses, serial numbers, or unreviewed diagnostics archives in a public report. The diagnostics collector warns that its output can contain hostnames, user names, paths, and network data.

## High-risk configuration in this wiki

- `thunderbolt-ibverbs` is explicitly research-only, buggy, and insecure.
- `amd_iommu=off` reduces DMA isolation and is not a default.
- `seccomp=unconfined` in container examples is compatibility-oriented and should be replaced with a narrowed profile.
- GPU firmware and kernel-module changes can make a host unbootable or unstable; retain a known-good boot entry.
- Environment variables such as `LD_LIBRARY_PATH` can load unintended libraries.

This package contains no firmware, kernel modules, ROCm binaries, model weights, or credentials.
