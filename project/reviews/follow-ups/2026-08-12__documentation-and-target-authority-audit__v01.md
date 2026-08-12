# Documentation and Target-Authority Audit — 2026-08-12

Disposition: current authority requires correction; historical/raw evidence is
preserved unchanged.

## Scope

- 10,224 tracked files were inventoried.
- 5,606 tracked documentation-like files were classified.
- The Wiki contains 86/86 structurally complete section sets.
- The manifest check, Wiki validator, four validator unit tests, and core
  documentation validator passed before correction.

Structural validation does not prove semantic freshness. Before this
reconciliation, all 86 Wiki sections were `needs-machine-validation` with
section-level verification dates from 2026-07-16 through 2026-07-19. Sections
13, 23, and 30 now carry a bounded 2026-08-12 source/machine reconciliation;
their remaining machine-validation status is intentional.

Counts were taken at `4a156395db62604cf37e27e6459e3ee0e3949c48`
from the named commit with:

```powershell
$files = git ls-tree -r --name-only 4a156395db62604cf37e27e6459e3ee0e3949c48
$docs = $files | Where-Object { $_ -match '\.(md|mdx|rst|txt|ya?ml|json|toml)$' }
Write-Output "TRACKED_TOTAL=$($files.Count) DOC_LIKE=$($docs.Count)"
```

The case-insensitive filter counts pathnames ending in Markdown, reStructured
Text, text, YAML, JSON, or TOML. It excludes untracked files and other formats.

## Confirmed current authority

A bounded live read-only audit on both targets measured CachyOS rolling,
kernel `7.1.3-1-cachyos`, ROCm 7.2.4-family packages, Mesa 26.1.4, and
`gfx1151`. nimo-1 is the current coordinator/API host and nimo-2 is the current
RPC worker. The active conventional UD-Q6 units were active/running with zero
restarts, the coordinator returned HTTP 200, and no service was changed;
HaloFPX/ROCmFPX is the development target, not the always-on deployment.

The normalized routing receipt is
[`../../../docs/halofpx/evidence/2026-08-12-strix-halo-live-authority/README.md`](../../../docs/halofpx/evidence/2026-08-12-strix-halo-live-authority/README.md).

## Material contradictions found

1. `docs/STRIX-HALO-QUICKSTART.md` described Framework hardware and Ubuntu
   `apt-get` as the project target path.
2. Section 23 used a July 17 tuple and ROCmFP4 service as current state.
3. Compact knowledge and candidate-skill pages treated the historical
   nimo-1-worker/nimo-2-coordinator assignment as current.
4. current Lead status and cache navigation still called closed issue #5 the
   active slice after PR #20 merged; issue #14 is now active.
5. `OBJECTIVES.md` called MiniMax the primary workload despite the owner's
   model-general clarification.
6. current navigation did not make the ROCmFPX GGUF product boundary explicit.
7. current handoff stopped at the publication/L111 boundary and did not route
   post-publication work.
8. ROCmFPX documentation omitted the source tree's Q2 family member or implied
   uniform Vulkan coverage. Q2 has a CPU implementation plus partial CUDA/HIP
   wiring and no Vulkan path in current source; Q3/Q4/Q4_FAST/Q6/Q8 have CPU,
   CUDA/HIP, and Vulkan paths.

## Preservation and authority policy

- Do not rewrite dated experiment roles, raw captures, imported sites, or
  commit-scoped implementation decisions.
- Add current authority at the routing layer and time-scope older observations.
- Treat Ubuntu and Framework material as portability/donor evidence, not the
  installed target tuple.
- Treat ROCmFPX as a GGUF weight-format family, not as the HIP backend and not
  as a K/V-cache-only feature.
- Treat MiniMax as a stress/capacity fixture. Primary performance evidence must
  use ROCmFPX/ROCmFP4 GGUF artifacts, with ordinary model fixtures added for
  model-general qualification.

## Remaining documentation work

- Pin a reproducible CachyOS bootstrap/package manifest without mutating the
  production machines merely to discover dependencies.
- Create and qualify an ordinary ROCmFPX fixture registry.
- Extend documentation validation to root/inherited current guides and add
  semantic freshness checks.
- Recover missing raw P01-P14 target bundles where possible.
- Perform a fresh-clone recovery drill and retain its receipt.
