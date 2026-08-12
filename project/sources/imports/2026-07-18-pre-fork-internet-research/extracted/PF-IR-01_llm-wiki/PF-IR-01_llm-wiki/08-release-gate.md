# Security release gate

[RECOMMENDATION] The release is blocked until every P0 gate is marked pass with attached evidence. A source-only “fixed” result is insufficient.

| Label | Gate | Evidence required | Owner | Current disposition |
|---|---|---|---|---|
| [RECOMMENDATION] | GATE-01 | Advisory refresh | Current official llama.cpp, fork, Talos, and bundled-dependency advisory captures regenerated on release date. | External security reviewer | Open |
| [RECOMMENDATION] | GATE-02 | Exact source | All repository and submodule commits match the approved ledger; working trees clean. | Release engineering | Open/local |
| [RECOMMENDATION] | GATE-03 | RPC build denial | Standard artifact built with GGML_RPC off; no RPC executable/backend symbols. | Build/CI | Fail in reviewed workflow |
| [RECOMMENDATION] | GATE-04 | HTTP defaults | Loopback default, slots/admin endpoints off, non-loopback startup fails without approved auth exception. | Server owner | Fail until policy patch |
| [RECOMMENDATION] | GATE-05 | Source sentinels | All GGUF/RPC/vocab/server sentinels pass on exact source trees. | CI | Ready to implement |
| [RECOMMENDATION] | GATE-06 | Safe regression | Isolated ASan/UBSan negative suite passes. | Security lab | Open/local |
| [RECOMMENDATION] | GATE-07 | Binary provenance | Artifact hash, source SHA, compiler, flags, SBOM, linked libraries, signatures published. | Release engineering | Open/local |
| [RECOMMENDATION] | GATE-08 | Loaded-library provenance | Runtime loader output matches the approved system-dependency allowlist. | Platform owner | Open/local |
| [RECOMMENDATION] | GATE-09 | Listener proof | Only approved loopback/Unix sockets; no RPC; firewall/namespace evidence attached. | Platform owner | Open/local |
| [RECOMMENDATION] | GATE-10 | Authentication proof | Protected routes reject absent/invalid credentials; public routes expose only approved data. | Service owner | Open/local |
| [RECOMMENDATION] | GATE-11 | Model provenance | Every production GGUF/auxiliary file has approved origin, license, hash, and malware/supply-chain review. | Model owner | Open/local |
| [RECOMMENDATION] | GATE-12 | Exception approval | Any RPC or non-loopback server exception is time-bounded, isolated, owner-approved, and separately packaged. | Security owner | Not permitted by default |

## Decision rule

[RECOMMENDATION] **PASS** only when gates 01–11 are complete and gate 12 is either not applicable or has explicit security approval.

[RECOMMENDATION] **HOLD** when any source, build, binary, listener, authentication, library, or model-provenance item is open.

[RECOMMENDATION] **REJECT** a standard artifact that contains RPC, binds an unauthenticated server beyond loopback, or cannot be tied to an exact clean source/build manifest.

## Evidence separation

[VERIFIED] External evidence in this package can satisfy advisory enumeration, commit identification, source equivalence, and build-recipe observations.

[OPEN] Deployed-binary equivalence, loaded-library provenance, effective listener/firewall state, and negative reachability can only be satisfied in the target environment.
