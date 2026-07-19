# L05t initializer no-I/O seam review v01

- Date: 2026-07-19
- Parent commit: `9da5b32efd1fe5ab726228761063bcca2cab9e35`
- Scope: public registry-envelope digest API plus the default-off Linux
  initializer linkage seam; no filesystem implementation
- Verdict: **ACCEPT**

## Outcome

The reviewed slice exposes the existing registry-lab envelope identity as a
bounded no-I/O API. Failure leaves caller output unchanged. The Linux-only
initializer archive delegates through that API and contains no filesystem,
mutation, environment, process, model, server, provider, HIP, Vulkan, RPC, or
WebUI behavior.

Both initializer options remain default-off. The initializer requires Linux
and the independently qualified preinit option. The general mutation option
remains a fatal configuration error. Feature-off target and CTest inventories
contain no initializer target or test.

## Findings and repairs

Independent review initially rejected promotion because the contract searched
for diagnostic strings without executing the gate matrix and did not prove
install/export/archive isolation. The implementation added:

- an executable Linux/non-Linux/preinit/initializer/mutation gate matrix;
- a nested feature-off configure proving target and CTest absence;
- whole-file checks for public/interface, install, export, product, and helper
  target edges; and
- an archive audit requiring one object, one exact global seam definition, and
  one exact HaloFPX digest import.

A second review found that a literal one-import rule would reject sanitizer
runtime instrumentation. The audit now admits only `__asan_` and `__ubsan_`
compiler-runtime imports in a detected sanitizer build while still requiring
exactly one HaloFPX dependency. Unsanitized archives must have exactly one
total import. Detection covers the canonical `LLAMA_SANITIZE_ADDRESS` and
`LLAMA_SANITIZE_UNDEFINED` options plus base and configuration-specific C,
C++, and linker flags.

A final review found no remaining actionable defect and returned **ACCEPT**.
The reviewer made no source edits.

## Review dimensions

- Correctness: independent golden bytes identify the predecessor; null, empty,
  oversized, and changed inputs are rejected or distinguished; failed calls do
  not modify output.
- Security: the slice has no I/O or mutation primitive and grants no reusable
  authority.
- Build isolation: Linux/preinit gates, feature-off absence, excluded static
  target shape, exact archive membership, imports, and product-tail isolation
  are tested.
- Provenance: the implementation is target-native; no donor or GPL code was
  copied and no P3 admission is required.
- Rollback/performance: defaults and product graphs are unchanged, so removal is
  a source-only rollback and no inference-performance claim is made.

This verdict authorizes the next initializer implementation submilestone only.
It does not admit persistent writes, repair, reopen, quarantine, cache hits,
restore, inference, or production key custody.
