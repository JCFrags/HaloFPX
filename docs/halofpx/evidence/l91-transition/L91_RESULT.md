# L91 terminal result

Classification: **NOT PROMOTED**.

The reviewed controller integration source was retained, and the single
authorized transition was consumed. Residency-A primary capture completed and
retained authenticated server custody, but the controller's consolidated
transient-unit guard refused locally before launching residency B. Consequently
there is no restore token, represented-state equality result, or cache
correctness conclusion.

Exact identities:

- base: `6e8d2b2b834af3a0e739efd11dbef271a362ddc6`
- source-root receipt: `672a1a4ac7d34507a1ece48c0e753efcf19f9b03b23072f49ddcb1e59ee74f1f`
- build ID: `f28e919b610b01a5d747eaff58be162e65c1d10663627064aeedca5a94e0a3f8`
- worker binary: `e12cb2f470be9338f4757e0f1a11fcd81b7e123fad7becccd112e5e3c5083b8b`
- canary binary: `701a10a6b8180b8a421665fc5da25f0dd26c79888676209a82d99ade0517580c`
- child: `5e374d5448d65f042ba80a1b4f28a3196351ba87b59ca1ab8f1ffd71f32d2d02`
- controller: `b11ef3e918b3720499c253ff11c0d854ea4fba7ce0447925fe6154ab454bf67b`
- source archive before terminal cleanup: 190,963,200 bytes, SHA256
  `2dc9bf0cf63facddc2a9ef7c785639d344bd4a7f22e35292c1d929bcb639a089`

Runtime evidence:

- exact capture log SHA256:
  `07177cdefcfc8ca62780ff58ec8cf9fa38c79239b5730442012ea7a2153d3241`
- retained capture suffix: `alpha`
- failure SHA256:
  `c0a381f91944c80fb920afec176d7bfc17c95a1840f763ed3017e3058fc005b5`
- server harvest manifest SHA256:
  `2c4b75fd444fbb06d50e2026f22da2aab1e5a1d7d1807a9b21fd47bf1cfb652d`
- four authenticated server authority attempts were retained.

Production recovery incurred one kernel OOM kill of the recovery-started worker,
followed by coordinator abort after RPC loss. Existing `on-failure` policies
performed exactly one restart each. The Lead accepted the resulting healthy,
unique authority as the new observed baseline. No production configuration,
unit definition, listener, or model was changed.
