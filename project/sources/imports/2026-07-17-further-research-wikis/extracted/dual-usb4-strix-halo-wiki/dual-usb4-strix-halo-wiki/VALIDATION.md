# Validation report

**Validated:** 2026-07-17T19:25:02Z  
**Result:** `passed-with-environmental-skips`

Validation is generated for the packaged snapshot. See `scripts/validate-wiki.sh` for the executable checks and `data/validation-environment.json` for the machine-readable result.

## Passed checks

- Bash syntax for every script and shell example.
- Python compilation with bytecode redirected outside the artifact tree.
- JSON, JSONL, and CSV parsing through the offline self-test.
- Relative Markdown link validation across the full wiki.
- Graphviz regeneration of all included SVG and PNG diagrams.
- `gcc -Wall -Wextra -Werror` build of `tools/mptcp_smoke.c`.
- Parse and `git apply --check`/apply of the optional llama.cpp MPTCP patch against a fixture containing the exact old-line contexts reviewed in `transport.cpp` blob `a728152421f7dac44baefc582d713540398dabe4`.
- Two-listener localhost execution of `tools/dual_path_copy.py` with a 2 MiB random payload, complete range coverage, and final SHA-256 equality.
- `tools/tbstream_file.py` send/receive execution over a FIFO with a 2 MiB random payload and final SHA-256 equality.

## Environmental skips and boundaries

### MkDocs build

`mkdocs` and MkDocs Material were not installed in the build container, so `mkdocs build --strict` was not executed. The dependency-pinned build instructions are in `requirements-docs.txt`; Markdown links, data files, Mermaid/Graphviz sources, and pre-rendered SVG/PNG diagrams were validated independently.

### Native MPTCP runtime

The MPTCP C program compiled cleanly. A runtime socket attempt on the build container's Linux `4.4.0` kernel failed with `socket(IPPROTO_MPTCP): Address family not supported by protocol`. This is an environment limitation, not a successful MPTCP transport test. Use Linux 5.6+ with MPTCP enabled; this wiki's deployment baseline is Linux 7.1.

### Target hardware

No Strix Halo hardware or physical USB4 links were available in the build environment. The folder therefore does **not** claim measured throughput, controller independence, or successful MPTCP subflow aggregation on a particular product. It supplies the capture scripts, measurement commands, and falsifiable proof criteria required to establish those facts on the target pair of nodes.

## Re-run

```bash
cd dual-usb4-strix-halo-wiki
scripts/validate-wiki.sh
```

For a full web-theme check after installing documentation dependencies:

```bash
python3 -m venv .venv
. .venv/bin/activate
pip install -r requirements-docs.txt
mkdocs build --strict
```
