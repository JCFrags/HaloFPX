# Socket transport and interface selection

## Stock Socket data path

The inspected active and 2.27.7 Socket implementations are explicit:

* `ptrSupport = NCCL_PTR_HOST`.
* `netDeviceType = NCCL_NET_DEVICE_HOST`.
* `regMr` rejects any pointer type other than `NCCL_PTR_HOST`.
* `iflush` says device pointers are unsupported.
* `regMrDmaBuf` is a null callback.

That source boundary supports a host-staged interpretation. It does not establish which copies, pinning strategy, or CPU load will be observed on the target APU; those require profiling.

## Interface controls

Use exact, logged selection rather than auto-discovery:

```bash
export NCCL_NET=Socket
export NCCL_SOCKET_IFNAME='=usb4eth0'   # replace with the exact Linux name
export NCCL_SOCKET_FAMILY=AF_INET
export NCCL_DEBUG=INFO
export NCCL_DEBUG_SUBSYS=INIT,NET,ENV,GRAPH
```

`NCCL_SOCKET_IFNAME` supports prefix lists, exclusion with `^`, and exact matching with `=`. Without a setting, source tries IB first, then a `NCCL_COMM_ID` subnet match, then ordinary interfaces excluding docker/loopback/virbr, followed by docker, loopback, and virbr.

## Retry, close, and reconnect

The source exposes connection/progress parameters including retry count, retry sleep, and poll timeout. A remote close is converted to `ncclRemoteError`. These knobs are not a documented universal deadline for every collective. No audited path transparently reconnects a failed established peer and restores the same communicator. The experiment harness must own a wall-clock watchdog, async polling, abort, and fresh initialization.
