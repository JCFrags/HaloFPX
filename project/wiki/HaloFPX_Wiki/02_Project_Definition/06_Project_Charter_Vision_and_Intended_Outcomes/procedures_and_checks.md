---
section_id: "06"
title: "Charter Validation Procedures"
status: "needs-machine-validation"
last_verified: "2026-07-16"
applies_to:
  repositories: ["HaloFPX project"]
  software_versions: []
  hardware_revisions: ["actual node A and node B"]
related_sections: ["09", "11", "18", "38", "49", "78", "80"]
---

# Charter validation procedures

## Internet/source follow-up

1. Freeze each dependency by commit and record default branch, license, submodules, and nearest release/tag.
2. Diff ROCmFPX and CachyLLama against their recorded llama.cpp ancestors before selecting code to combine.
3. Map every claimed feature to source files and tests; treat README-only behavior as unverified until reproduced.
4. Recheck heads and server API documentation at each milestone because all four repositories are active.

## On-machine baseline campaign

Prerequisites: exact model files and SHA-256 hashes, pinned binaries, isolated test network, sufficient disk, and monitoring. Root is not required for inference; platform/transport tuning may require root and must be recorded separately.

1. Inventory BIOS, firmware, kernel, amdgpu, ROCm, Vulkan, memory allocation, NVMe, USB4 topology, and thermals on both nodes.
2. Run identical single-node correctness and performance workloads on A and B.
3. Characterize each link alone, both links together, and link-loss behavior before model distribution.
4. Test each candidate execution mode against the best matched single-node and replication baselines.
5. Inject server restart, cache corruption, node loss, and one-link loss; retain raw logs and recovery timing.
6. Have the sponsor ratify numeric gates only after reviewing distributions, not peak results.

## Charter review checklist

- [ ] Sponsor/owner, budget, schedule, and deployment audience recorded.
- [ ] Supported workload and model matrix ratified.
- [ ] Quality, speed, capacity, resilience, privacy, and maintainability thresholds numbered in Section 09.
- [ ] Every benchmark is matched and reproducible.
- [ ] Stop/narrow criteria and fallback product defined.

