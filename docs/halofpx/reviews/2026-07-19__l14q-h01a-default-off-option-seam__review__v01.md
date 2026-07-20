# L14Q-H01a default-off option seam independent review

Date: 2026-07-19

Verdict: **ACCEPT_FOR_H01A_INERT_SEAM**

The five-line diff adds one CMake option that defaults `OFF` and propagates its
definition only inside the HIP backend directory when explicitly enabled. It
does not change source lists, kernels, selectors, template instances, runtime
behavior, TurboQuant/ROCmFPX/Vulkan routing, CLI, server, or deployment state.
No donor or GPL implementation entered HaloFPX. The independently approved
`L14Q-H01` P3 at preparation commit
`4f0a2749c2b3c23dc3d45ea25a380ed2a274dfc2` authorizes this target-native seam.

On nimo-1, configure without the option and configure with explicit `OFF` both
resolved to `OFF`; explicit `ON` resolved to `ON`. Clean gfx1151 HIP builds
completed in both modes. The feature macro appeared in zero OFF compile
commands and 154 ON compile commands. The retained evidence bundle is 4357
bytes with SHA-256
`46f9905f991b6e32d976b42fef66ac758ceb4cd96f862850a925a9190101929b`.

The node evidence checkout is commit
`7e7c224947cc3844b40fef12cf5731aae24a1101`, tree
`2608ddbbecf157dd010431fb57e43a31ec103f83`. Its two qualifying parent blobs
are identical to those at the `91e9ba6` milestone parent; the intervening
commits are documentation-only. This distinction is now explicit in the
receipt.

H01a is not a kernel or runtime optimization admission. H01 implementation,
correctness, no-copy similarity, matched performance, and promotion gates all
remain open.
