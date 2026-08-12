# CI assets

- `.github/workflows/validate-suite.yml` validates this wiki/harness itself.
- `ci/examples/` contains templates for source-fork CI; they are not automatically active.
- `run-lane.sh` demonstrates explicit CPU sanitizer, ROCm, and Vulkan build lanes.
- `path-map.json` maps changed implementation paths to conformance areas.
- `lane-inventory.template.json` defines hardware-lane evidence.
- `reference-workflow.sh` deliberately fails until an organization implements protected reference collection and approval.

The example workflows use current major GitHub-owned action tags for readability. Production workflows should pin action commits under the organization's supply-chain policy and verify runner-version compatibility.
