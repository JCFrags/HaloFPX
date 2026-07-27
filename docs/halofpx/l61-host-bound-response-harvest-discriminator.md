# L61 host-bound response harvesting and deferred discriminator

**Status:** `[MEASURED] NOT PROMOTED`

L61 closes the L60 host-path admission defect. The closed manifest now binds
each response harvester to its executing host, exact staged source path,
SHA-256, interpreter, input stream, and output staging path. Cross-host
substitution refuses. The controller harvests both streams in its finally path
before worker stop or root/key cleanup.

The final no-model two-host fixture used real transient units and exact final
binaries. The nimo-1 worker prefix was 483 bytes
(`51c168d15c271f680f2f9e7009342f732f8df1bf9e9ef5c1507bbcbc5ddd2f3e`);
the nimo-2 client prefix was 485 bytes
(`4a3214263291ca5587d6105289be86856eabf912201a534757e92e0e00d1ccb9`).
Both authenticated, their writers were bound to exact PID/InvocationID and
quiesced before collection, and all units, roots, keys, streams, and staging
files were removed. Focused one-side-missing and wrong-host/path cases refused.
The exact runtime-environment client probe also created, harvested,
authenticated, and removed a 485-byte prefix before model launch. Its receipt
hash is
`6bf835420c027d0b9dd95a376720b8237a2fe84914f47e49734629bf6a8609b8`.

The focused final suite passed 85 tests and 11 subtests, with one retained
POSIX-only test skipped on Windows. Independent pre-runtime review issued GO
for the single stories15M run.

## Sole runtime result

The single run loaded stories15M but did not reach the intended authenticated
first-chunk response seam. During `common_init_from_params` warmup, the worker
executed the ordinary RPC `graph_compute` path for 144 nodes/193 tensors and
served subsequent `get_tensor` reads. The coordinator's armed capture-chunk
status then reported scheduler status `-1` and `llama_decode` `-3`, without any
authenticated graph prepare/execute or mutable-session record. Its later line
2026 abort is in
`ggml_backend_rpc_buffer_free_buffer` and is teardown after the earlier
scheduler failure, not evidence that the worker crashed.

Both host-local response streams were authoritatively harvested as
`missing/source_absent`; the receipt hash is
`64e2cefe0049d080b502d397e6b4cff8fbf9de5e9902d779e8bc155003f2d0b9`.
This is consistent with the reviewed instrumentation boundary: it emits for
`RPC_CMD_HALOFPX_GRAPH_AUTH_EXECUTE`, whereas the retained worker chronology
contains only ordinary warmup `RPC_CMD_GRAPH_COMPUTE` and no
`[halofpx-rpc-graph-auth]` prepare/execute record. The L55 status line labels
the surrounding requested phase `capture-chunk`, but it cannot override the
timestamped common-warmup entry and absence of an authenticated execution.

Therefore L61 classifies the earliest boundary as **armed first-chunk
coordinator scheduler failure before the instrumented authenticated
GRAPH_AUTH_EXECUTE response seam**. The only retained worker execution belongs
chronologically to common warmup and is not attributed to that first chunk. L61 does not
classify handler crash, response publication, truncation, opcode/size mismatch,
socket EOF, or graph-receipt validation for the intended first chunk. No
semantic correction is proposed from this evidence. The smallest future
discriminator would instrument the preceding L40/L44 admission and
GRAPH_AUTH_COMPUTE/RECOMPUTE client decisions to establish why control returns
before the observed execute seam; it requires new authority.

The model runtime was not repeated. No primary artifact was accessed and
production was read-only. Production snapshots are byte-identical at SHA-256
`511f05d2b638277e19ae7af44573eb9e244d829458f24cdefc46e3f88ffd6ded`:
nimo-2 worker PID 1535639/50052/NRestarts0 and nimo-1 coordinator PID
2356329/8081/HTTP200/NRestarts0. All eight unit guards closed at exact absence
and all disposable resources were removed.

The 29-file runtime evidence hash-list digest is
`f63e03b7709e478d02d5eef6c4996fc1d8825e7da5fd4f5542d6ee469d231725`.
