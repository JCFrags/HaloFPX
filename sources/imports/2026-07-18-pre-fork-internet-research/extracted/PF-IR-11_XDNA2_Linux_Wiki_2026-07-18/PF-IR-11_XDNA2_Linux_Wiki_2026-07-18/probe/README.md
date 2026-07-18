# XDNA2 read-only probe

The probe inventories the target Linux substrate without changing it.

## Default guarantees

It does not use `sudo`, install packages, load or unload modules, bind or unbind PCI devices, write sysfs/debugfs, change limits, reset the NPU, suspend the system, compile a model, or run inference.

## Run

```bash
./xdna2_readonly_probe.sh
```

Use a named output directory:

```bash
./xdna2_readonly_probe.sh --output ./pf-ir-11-probe
```

Opt in to a pre-existing XRT read-only query:

```bash
./xdna2_readonly_probe.sh --output ./pf-ir-11-probe --xrt-query
```

`--xrt-query` runs `xrt-smi examine` only when the command already exists. It does not install XRT.

## Output

The directory contains raw text captures, a compact `summary.json`, and `SHA256SUMS`.

The probe is a substrate check. It cannot establish graph placement, correctness, performance, energy efficiency, or production recovery.
