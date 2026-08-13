# Normative amendment 01 — feature-OFF generator

This file amends only the feature-OFF toolchain description in
`2026-08-12__halofpx-same-pc-full-recovery__receipt__open-v01`.

The exact current CI command used CMake's default **Unix Makefiles** generator,
`CMAKE_MAKE_PROGRAM=/usr/bin/gmake`, and GNU Make 4.4.1. Ninja 1.13.2 was
installed and recorded in the host environment, but it was **not** the generator
used for this feature-OFF lane. The build log and CMake cache already recorded
the correct Makefile output.

No build result, same-PC rehearsal status, or issue #2 status changes. The
feature-OFF result remains PASS compile-only, and issue #2 remains OPEN.

The authoritative continuation integrity index is
`2026-08-12__halofpx-same-pc-full-recovery__sha256-ledger__v02.txt`. The v01
ledger remains retained and valid for its original pre-amendment scope.
