# L89 response-boundary ownership and focused gates

## Source-proven ownership

The graph-auth execute path has two mutually exclusive transport recorders.
When preexecute authority is enabled, `ggml-rpc.cpp:4214-4224` selects
`send_rpc_cmd_preexecute()` rather than `send_rpc_cmd_observed()`. The former
records opcode/header/body send and response header/body receive progress in
the L44 preexecute recorder (`ggml-rpc.cpp:2845`); it does not emit
`halofpx.rpc-response-boundary.v1` client transport records.

After that transport returns, the graph-auth client independently emits only
`client_decode` and `client_receipt_validation` at
`ggml-rpc.cpp:4235-4263`. The server owns handler validation, backend result,
receipt construction, handler exit, and the observed response publication at
`ggml-rpc.cpp:9000-9005`. L88 retained exactly that source-produced pairing:
two authenticated client semantic records and seven authenticated server
records through `response_body_publish`.

The client parent UID is client-owned and nonzero while the server's
server-local parent UID is canonically zero. It is therefore checked for
internal stability, not false cross-side equality. The legitimately shared
wire identities are cross-bound: opcode, attempt nonce, connection/server
nonce, split UID, execution sequence, and backend ordinal.

## Exact correction

`halofpx_rpc_response_boundary.py` now recognizes exactly one additional
client production:

1. `client_decode`, success, status 1;
2. `client_receipt_validation`, success, status 2.

Both records require opcode 25, zero expected/actual bytes, `rc=1`, zero
errno/EOF, authentication, consecutive event numbers, and internally stable
identities. This production is accepted only with a simultaneously supplied
complete seven-record authenticated server success whose exact phases,
statuses, publication byte counts, and shared identities match. It is not an
incomplete-prefix relaxation. Existing full client productions and retained
failure prefixes are unchanged.

## Focused qualification

- Python boundary, L59 custody, and L61 host-bound tests: 20 passed.
- Exact L88 payload replay: accepted after re-signing with the bounded test
  key; the destroyed L88 runtime key was not reconstructed.
- Missing server, incomplete server, reordered/missing/extra client events,
  authenticated client failure, wrong opcode, authentication tamper, and
  every shared-identity mismatch: refused.
- Real two-host no-model graph-auth execution: succeeded and produced the
  exact two-plus-seven pair. Pair verification passed; the same real client
  stream without its server stream refused.
- Feature-on Linux builds passed on both hosts. Feature-off `rpc-server`
  compiled successfully; no protocol/client/server feature semantics changed.
- Source root:
  `ad6c861ec615cb8f3e49e3165730e9874fee690404748f25993227557ac2ab60`
- Build ID:
  `f5c159f5b4379038c3205d173bc0af25e271d7b02ab3c1f77fd6f7f8870d3585`
- Worker:
  `b630271eca40f30a491a0d595e1d91a1ecf01e88123eb3dc44b4bd6a5c057ee6`
- Canary:
  `eb0b9634b24cda5573c6ec57700afb6ee3c16b66b2b8e8162b2e42a05b64b133`
- Verifier:
  `782a6156bbedf985fe9f4b9377d75fc00065692a5aa1374af254b5030adafa28`

Focused disposable units, keys, response paths, and authority files were
quiesced and removed. Named production services were not mutated.
