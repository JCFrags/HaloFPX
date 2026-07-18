# PF-IR-03 — Official gfx1151 Compute-Stack and Signed-Artifact Compatibility Tuple

**Priority:** P0  
**Evidence date:** 2026-07-18  
**Local comparison date:** 2026-07-17

This offline dossier separates the legacy ROCm 7.2.4 lane, the Core SDK/TheRock 7.14.0 preview lane, the local two-node comparison tuple, and a Linux 7.2/USB4STREAM kernel overlay candidate. It preserves source identifiers, exact known commits and hashes, machine-readable claims, dependency/package mappings, license status, acquisition scripts and rollback guidance.

## Immediate findings

- The current official gfx1151 Core SDK matrix supports Ubuntu 26.04/GA 7.0 or Ubuntu 24.04.4/HWE 6.17 with the inbox kernel driver.
- Core SDK 7.14.0 is explicitly preview; TheRock gfx1151 is build/sanity tested but not marked Release Ready.
- The exact stable gfx1151 tarball URL is pinned, but no vendor checksum, detached signature or SBOM was located. The raw tarball lane is therefore non-promotable.
- Stable native package repositories document GPG-verified metadata and are the preferred signed-artifact candidate lane.
- The 7.2.4 control uses exact superrepository commits; `default.xml` alone is not a 7.2.4 lock because it defaults projects to 7.2.0.
- The installed ROCm 7.2.4/Mesa 26.1.4/kernel 7.1.3 tuple is comparison-only.
- Linux 7.2/USB4STREAM is a plausible separate candidate overlay, not an official replacement for the compute baseline.

## Start here

Open `index.html`, or read [Executive decision](docs/00-executive-decision.md).

## Folder map

- `wiki/` and `index.html`: offline LLM-Wiki rendering.
- `docs/`: canonical Markdown dossier.
- `manifests/`: claims, sources, component/source/dependency locks and tuple definitions.
- `raw/`: structured primary-source capture records and provenance-gap records.
- `scripts/`: fail-closed acquisition, source checkout, inventory, SBOM, tuple capture and preflight tooling.
- `SHA256SUMS`: checksums for dossier files.

## Integrity note

`SHA256SUMS` authenticates this generated dossier only when its digest is received through a trusted channel. It does not authenticate external AMD/kernel/Mesa artifacts.
