# L90 identity and evidence receipt

- accepted base: `8c71aab90e8fdec124bbd593eb53c08ec54b2861`
- source root: `ad6c861ec615cb8f3e49e3165730e9874fee690404748f25993227557ac2ab60`
- build ID: `f5c159f5b4379038c3205d173bc0af25e271d7b02ab3c1f77fd6f7f8870d3585`
- controller SHA256:
  `89254efe4872a426f7b6193b1bd453f7b2b442446bd886d9a559beb9336ef742`
- source archive SHA256:
  `6d498c9a5de8fb24940a244f3aeb8213e18da8cdeab3349c178b52c8bc1d0164`
- worker SHA256:
  `b630271eca40f30a491a0d595e1d91a1ecf01e88123eb3dc44b4bd6a5c057ee6`
- canary SHA256:
  `eb0b9634b24cda5573c6ec57700afb6ee3c16b66b2b8e8162b2e42a05b64b133`
- pinned artifact SHA256:
  `96506ada918e60ca9a9cfde8a5437790e4453401a6a3e236e3f55e7bac3aaea6`

Focused prerequisite:

- 62 tests passed with 11 subtests;
- independent pre-runtime review: PASS, P1 none, P2 none;
- exact prerequisite status: absent before production mutation.

Runtime evidence:

- warmup status: authenticated success, one 512-token chunk;
- client response stream: 984 bytes, SHA256
  `493c4ba37132d41f13c1adbfd16b47feb14823deab4852a2869f059cf1c9d7b5`;
- server response stream: 3424 bytes, SHA256
  `1bdb995dca23433775cedba7b9ceb6fcad25211e40671d90f3a924bc9c08e3f3`;
- retained server authority: 4200 bytes, SHA256
  `39d70d4054c43d511ec099da76b7a8ca627354b3cf74f23c5650c0f59212a6bd`,
  terminal branch 1;
- terminal child refusal:
  `transient unit guard authority is outside the closed manifest`;
- workload/capture/restore/token: not run;
- retry: none.

Cleanup:

- controller recovery completed worker-first and coordinator-second;
- terminal prerequisite cleanup receipt:
  `../l90-terminal-cleanup/l90-disposable-prerequisite.json`;
- all closed disposable units and paths absent;
- production healthy, unique, expected arguments, NRestarts 0, HTTP 200.
