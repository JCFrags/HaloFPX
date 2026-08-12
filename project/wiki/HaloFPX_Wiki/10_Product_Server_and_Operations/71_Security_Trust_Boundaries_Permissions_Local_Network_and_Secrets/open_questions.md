---
section_id: "71"
title: "Security Open Questions"
status: "needs-machine-validation"
last_verified: "2026-07-16"
applies_to:
  repositories: ["Custom_Inference_Project"]
  software_versions: ["security design candidate 2026-07-16"]
  hardware_revisions: ["dual Strix Halo target; exact nodes open"]
related_sections: ["66", "69", "70", "72"]
---

# Open questions

1. **[OPEN]** Who are the security principals: one owner, household accounts, LAN services, or mutually hostile tenants?
2. **[OPEN]** Which CA/key mechanism authenticates clients and peers, and how are issuance, rotation, revocation, and recovery handled?
3. **[OPEN]** Is peer encryption mandatory on the direct link, and who may approve any exception?
4. **[OPEN]** Which admin roles/actions are required and which demand local presence or multi-step confirmation?
5. **[OPEN]** What model publisher/signing roots and provenance policy are accepted?
6. **[OPEN]** What secure-deletion guarantee is feasible for cache files, SSD media, RAM, and GPU memory?
7. **[OPEN]** Is SELinux, AppArmor, Landlock, a container sandbox, or a VM required for model staging?
8. **[OPEN]** What vulnerability disclosure, patch deadline, and emergency-disable process applies?
9. **[OPEN]** Can any cache sharing across principals be justified without content disclosure or timing side channels?
10. **[OPEN]** What exact executables/libraries are deployed on both RPC peers, what source/build identities produced them, and which interfaces and principals can reach their listeners?
