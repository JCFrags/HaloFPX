# L89 identity and custody receipt

- accepted base: `a546ca48c6d997f145ff42be3adcd34adf658d1d`
- source root: `ad6c861ec615cb8f3e49e3165730e9874fee690404748f25993227557ac2ab60`
- build ID: `f5c159f5b4379038c3205d173bc0af25e271d7b02ab3c1f77fd6f7f8870d3585`
- worker SHA256: `b630271eca40f30a491a0d595e1d91a1ecf01e88123eb3dc44b4bd6a5c057ee6`
- canary SHA256: `eb0b9634b24cda5573c6ec57700afb6ee3c16b66b2b8e8162b2e42a05b64b133`
- verifier SHA256: `782a6156bbedf985fe9f4b9377d75fc00065692a5aa1374af254b5030adafa28`
- verifier-test SHA256: `5fe1aa255ca6cd4dfa243ae7cd4d72e1f11b7b8a345a1e8a200d8bed5d8a2b6b`
- pinned artifact SHA256:
  `96506ada918e60ca9a9cfde8a5437790e4453401a6a3e236e3f55e7bac3aaea6`

Pre-runtime gates:

- focused tests: 24 passed, one platform-specific skip;
- feature-on exact-root Linux targets built;
- feature-off `rpc-server` compiled and remained inert;
- real two-host no-model paired success accepted;
- the same client stream without its server stream refused;
- independent pre-runtime review: PASS, no correctness/security P1/P2.

Runtime boundary:

- controller exit: 1;
- terminal boundary: prelaunch disposable-unit collision;
- model launched: no;
- warmup/capture/restore: none;
- response streams and server authority: absent, consistent with no request;
- retry: none.

Cleanup and production reconciliation:

- `halofpx-l48-canary-first-chunk.service`: unloaded and absent;
- exact remote `/var/tmp/halofpx-l89-source`,
  `/var/tmp/halofpx-l89-source.tar`, and control keys: absent on both hosts;
- all L89 build units: not found/inactive on both hosts;
- production coordinator and worker: active/running, unique listeners,
  NRestarts 0, expected executable arguments;
- coordinator health: HTTP 200, body SHA256
  `a29ee2b15c494311c52521766e44af56a3ad2248e7a8ab465e5206463c13d288`.
