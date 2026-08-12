# Open questions and release gates

## Deployment facts

- What are the exact HaloKV and tensor-cache paths in production?
- What do `findmnt`, `/proc/self/mountinfo`, filesystem feature tools and block topology report for each?
- Is either root inside a container overlay, bind mount or managed volume?
- What are the controller/device volatile-cache and power-loss-protection characteristics?

## OPEN-FMT-01 details

- Final header schema, endianness, checksum/hash and collision policy.
- Object key binding and separation between HaloKV and tensor-cache object classes.
- Generation, manifest and garbage-collection model.
- Compatibility/upgrade policy and maximum object size.
- Whether metadata such as mode/ownership/xattrs is part of the durable contract, which determines `fdatasync` versus `fsync`.

## OPEN-STORAGE-01 details

- Create-only versus replace semantics per object class.
- Whether a manifest indirection is required for multi-object transactions.
- Whether directory batching is allowed and the maximum acknowledgement window.
- ext4 versus XFS deployment decision; Btrfs conditional acceptance.
- DIO admission criteria and measured benefit.
- Required behavior after directory-sync failure or EIO: process exit, root read-only mode, or operator fence.

## io_uring details

- Minimum supported kernel and liburing versions for deployment, not merely the research anchor.
- Ring ownership model and whether SQPOLL/IOPOLL is permitted.
- CQ overflow policy, multishot use and `IOSQE_CQE_SKIP_SUCCESS` prohibition/allowance.
- Resource-tag strategy and maximum registered memory.
- Shutdown deadlines and behavior for noncancelable storage I/O.

## Hard release gate

No persistent writes until `T001–T040` applicable to the selected profile pass and the evidence bundle is reviewed. Documentation narrows the design space; it does not certify the implementation.
