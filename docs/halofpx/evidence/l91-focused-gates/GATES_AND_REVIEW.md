# L91 controller/child closure gates

The controller now derives one canonical disposable unit authority from the
already validated manifest and supplies it to the child. Service suffix
normalization occurs exactly once. L77 requires this manifest authority and
cannot fall back to the legacy mode-local set.

The exact L77 set covers device gate, capture/restore workers, and
capture/restore/first-chunk canaries. Every remote `systemd-run` flows through
the same prelaunch guard, and every worker/canary cleanup uses the identical
canonical tuple. Near names, wrong hosts, wrong ports, absent entries,
duplicate/malformed authority, and active ownership refuse.

The L77 manifest selects the full correctness runner and no longer passes
`--l55-first-chunk`. Response harvester identities remain required for the
full capture/restore path.

Focused qualification: 103 passed with 11 subtests. Independent review:
**PASS**, P1 none, P2 none. The accepted L79 closed replay traverses current
manifest structure and stops only at its intentionally older staged-source
hash; current source/binary validation is required after exact L91 staging.

Exact staging and read-only preflight subsequently passed:

- source root:
  `672a1a4ac7d34507a1ece48c0e753efcf19f9b03b23072f49ddcb1e59ee74f1f`;
- build ID:
  `f28e919b610b01a5d747eaff58be162e65c1d10663627064aeedca5a94e0a3f8`;
- worker:
  `e12cb2f470be9338f4757e0f1a11fcd81b7e123fad7becccd112e5e3c5083b8b`;
- canary:
  `701a10a6b8180b8a421665fc5da25f0dd26c79888676209a82d99ade0517580c`;
- controller:
  `b11ef3e918b3720499c253ff11c0d854ea4fba7ce0447925fe6154ab454bf67b`.
