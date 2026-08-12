# Contributing

Changes should preserve the repository's separation between sourced facts, calculated identities, measured inputs, scenario assumptions, and decision rules.

## Required for technical changes

1. Cite a primary source for new hardware, operating-system, runtime, or model-architecture facts.
2. Put measured values in a measurement record; do not silently promote them to platform constants.
3. Give every numerical estimate a unit and evidence label.
4. Update `data/formula_catalog.csv` when changing a cost equation.
5. Update or add a test for arithmetic or schema changes.
6. Run `make all` before review.

## Measurement contributions

A measurement submission should identify system vendor/model, firmware, BIOS settings, OS and kernel, cable certification, port-to-controller topology, power profile, runtime commit, model/checkpoint, quantization, context and batch, transport, message-size distribution, and raw command output. Report medians and tails; retain raw samples.
