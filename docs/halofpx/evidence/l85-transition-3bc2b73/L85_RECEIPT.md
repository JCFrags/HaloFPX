# L85 evidence receipt

- Accepted base: `f33b8e4ce24aa876bbf75c2729421dc1fdb0b55e`
- Runtime source identity root:
  `6052c8de1a47b35fd9467f2533d3fee80a83a9d7bb2ff16dec497ebacddc6ecb`
- Build identity:
  `3bc2b73aaee8c4431b2e3e05af96b06d180410b3c2a03a80efde8e3e7240fd14`
- Manifest SHA-256:
  `01a92b3294e25bd519db0b8b91fc29cc228e11706b44650d2c28fd3cbf634cfd`
- Worker binary SHA-256:
  `2fcdd7d9f5aee58f77111bf831952ecd267ea2fb6ea42b9c8afc453a7c5bf548`
- Canary binary SHA-256:
  `4a4eca5b6ce5a5c3986924eb29522d4b09ae4b68ab6f0e8899f29217c9e275c9`
- Primary artifact: 159,873,097,824 bytes; SHA-256
  `96506ada918e60ca9a9cfde8a5437790e4453401a6a3e236e3f55e7bac3aaea6`
- Canary invocation:
  `903d58c046a04cb899388838b7c4b972`
- Canary journal SHA-256:
  `331975ec38eaa94082bffa81efbe147afbac16bfcd591e2992474cf935d7a277`
- Worker journal SHA-256:
  `187ba872b453a29de711c95bf4f3a0745b56f6e6d334f728094741bc9e6eb71e`
- Controller SSH operations: 689; SHA-256
  `03d17b31bf8e63d861d8350fc3dc6ab99bc7c4f7aae88c57ec325e77ea0bd50a`
- Child SSH operations: 328; SHA-256
  `f17013dca44d749a8e7b070810a716b9b1845dbd0b0074023ec44a9fa159d94d`
- Production preflight SHA-256:
  `29ade05da0b185e90094a68b776846d42863b19b1003f9a9267c273338fb6597`
- Terminal branch: `l42_resolved_census_refused`
- Typed reason: `0`
- Admission / execute / cache operations: `0 / 0 / 0`
- Runtime attempts: `1`
- Retry count: `0`
- Terminal classification: **NOT PROMOTED**

Production closeout:

- coordinator: PID `2808706`, InvocationID
  `4e67ea30ddaf4037a70435a9ff2ff022`, `NRestarts=0`, port 8081,
  HTTP 200;
- worker: PID `2000412`, InvocationID
  `bf3ec63660b5451892cd346124fec158`, `NRestarts=0`, port 50052;
- no `halofpx-l48*` disposable user unit or top-level
  `/var/tmp/halofpx-l48-*` path remained on either host.

The separately retained refused preflight directories record mechanical
source/helper byte-staging corrections and consumed no runtime attempt.
