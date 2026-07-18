### Q: `xrt-smi` enumerates the NPU, but commands abort (`ERT_CMD_STATE_ABORT`) or the mailbox times out right after load. What's wrong?

A: This is almost always a **firmware/driver version mismatch** — a stale NPU firmware
(`npu.sbin`) left over from a previous or distro install does not match the loaded `amdxdna.ko`.
The plugin package ships the matching firmware to `/usr/lib/firmware/amdnpu/<device>/`; make sure
that copy is the one in use (re-install the plugin package, or remove a stale firmware you placed
by hand), then reload the driver. Mixing a firmware version from one source with a driver from
another is the common cause of post-load command aborts.

### Q: The device enumerates, but telemetry/array queries fail with "Operation not supported" or `EINVAL` (e.g. `GET_ARRAY`, `QUERY_TELEMETRY`). Why?

A: Your XRT/plugin is newer than the loaded kernel driver. A distro **in-tree** `amdxdna` can
predate ioctls that the current XRT SHIM issues, so those calls return `EOPNOTSUPP`/`EINVAL` while
basic BO/exec paths still work. Use the driver built and shipped from this repo (the staging
`amdxdna.ko` — see [Default driver in the plugin package](#default-driver-in-the-plugin-package)),
which implements the matching ioctls, rather than an older in-tree module.
