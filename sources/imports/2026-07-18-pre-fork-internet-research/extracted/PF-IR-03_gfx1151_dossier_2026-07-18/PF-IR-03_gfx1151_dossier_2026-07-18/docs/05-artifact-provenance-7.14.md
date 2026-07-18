# ROCm 7.14.0 gfx1151 artifact provenance

## Exact artifact

```text
https://repo.amd.com/rocm/tarball-multi-arch/therock-dist-linux-gfx1151-7.14.0.tar.gz
```

## Integrity status

| Field | Captured value |
|---|---|
| Vendor-published SHA-256 | **not located** |
| Vendor-published SHA-512 | **not located** |
| Detached signature | **not located** |
| Vendor SBOM | **not located** |
| HTTPS URL | yes |
| Promotion status | `false` until expected bytes are authenticated |

[PROVENANCE_GAP] A hash computed after download proves repeatability of the acquired bytes, not that AMD intended those bytes. The verification script therefore fails closed unless an expected digest is supplied from an independently trusted source, or an explicit inventory-only override is used.

## Stable native package alternative

AMD documents stable multi-architecture APT repositories for both supported candidate OS lanes (`ubuntu2604` and `ubuntu2404`) with a ROCm release GPG key and `signed-by` configuration. The package lane provides a stronger reproducible-integrity path because APT verifies signed repository metadata and package hashes. Freeze:

- key bytes and fingerprint;
- InRelease/Release/Release.gpg;
- Packages metadata;
- every selected package version, filename and SHA-256;
- dependency closure and package-file list;
- repository access date.

Do not confuse this stable repository model with TheRock nightly native-package instructions, which explicitly disable signature checks.

## Expected tarball contents

TheRock documents a flattened SDK layout containing `.kpack`, `bin`, `clients`, `include`, `lib`, `libexec` and `share`. `share/therock/therock_manifest.json` is expected to carry TheRock/source git pins and patch provenance. The tarball is not an installer and does not establish environment variables.

## Acquisition procedure

```bash
EXPECTED_SHA256='<authenticated value>'   scripts/verify-7.14-artifact.sh /var/tmp/pf-ir-03-acquire
```

The script records HTTP headers, SHA-256/SHA-512, a sorted tar listing, path-safety checks, extracted source manifest, file hashes and license candidates. It refuses promotion when `EXPECTED_SHA256` is absent or mismatched.

## Artifact/package compatibility tuple

For a signed-metadata candidate, prefer:

```text
Ubuntu 24.04.4 + HWE 6.17 inbox driver
amdrocm-core-sdk7.14-gfx1151 from the stable multi-arch APT repository
TheRock source root 418cd5f63abb7a604bad5874cd7b2e29334e640f
```

The Ubuntu 26.04/GA 7.0 option is equally documented; choose one and freeze it. Do not combine both node images under one tuple identifier.
