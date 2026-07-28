# L87 result — NOT PROMOTED

L87 retained the reviewed default-off local-vs-RPC census correction, but the single authorized primary attempt failed during warmup. It produced no cache-correctness conclusion.

## Source and focused gates

- Accepted base: `4c77d7af13ae03b425ccf32377af7e8bc1024aa8`
- Pre-runtime patch identity: `387468b422a06ec105bcca58af93a3106868d47b`
- Runtime source root: `491584c0ff7745f4132879fb3940e30d369e6bb70544815668626d254fbbb41e`
- Runtime build ID: `821b6217f55f52d5f70a44c69d56add584f1d111894d07af39dd1d94e5867d7b`
- Exact manifest SHA256: `3a2b2654bc45976431c9302c0ca9db050f3242be3f0afe0032129c151000c48e`
- Exact source archive SHA256: `2f42e4d9421470ba88a0a12ea17cf4304bfc3bab6c44e2c65204f5417f17cd71`

The focused gates passed mixed local/RPC filtering, strict falsely-RPC refusal, immutable count/root/iteration agreement, two-session storage checks, feature-on/off behavior, and the real two-process no-model success/refusal fixture. The pre-runtime independent review found no correctness/security P1/P2.

## Single primary attempt

The one authorized attempt stopped at the warmup kill gate. The durable client authority was:

`branch=l44_mutable_exclude_refused`, backend `0`, census index `253`, disposition `EXCLUDE`, role `IMMUTABLE_MODEL_WEIGHT`, ordinal `579`, execution sequence `1`, pending `1`, ggml status `-1`, typed reason `9`.

For the L44 mutable-admit result enum, reason `9` is `GGML_RPC_HALOFPX_MUTABLE_ADMIT_RECORDER_FAILURE`. It is not the projection diagnostic enum. This places the next exact semantic boundary after the L87 projection/sealing path, at client L44 exclusion recording. `llama_decode` then returned `-3`.

The server authority for the same attempt was authenticated and retained:

- retained authority SHA256: `94f29a0f10c745214807e4c66c2d408277bf32b1d72a6bb194cf948d97864b2f`
- size: `1400`
- server terminal branch: `3`
- harvest status: `present`

The controller did not proceed to workload, capture, or restore. No retry or runtime semantic correction occurred.

## Recovery

All disposable units were absent after cleanup. Production recovered healthy and unique:

- coordinator: PID `2838185`, port `8081`, NRestarts `0`, HTTP `200`
- worker: PID `2035972`, port `50052`, NRestarts `0`

No production cache was enabled and no production configuration was changed.
