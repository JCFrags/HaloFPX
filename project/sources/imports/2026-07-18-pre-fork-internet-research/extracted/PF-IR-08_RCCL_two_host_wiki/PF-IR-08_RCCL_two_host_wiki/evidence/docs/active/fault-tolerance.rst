---
upstream_repository: "ROCm/rocm-systems"
upstream_ref: "801a9ca2ad8940ac7cd7d571163e003f3a3d6cab"
accessed_at: "2026-07-18"
source_id: "DOC-ACTIVE-FAULT"
source_class: "documentation"
claim_label: "[NORMATIVE_DOC]"
upstream_path: "projects/rccl/docs/how-to/fault-tolerance.rst"
upstream_blob_sha: "4cdd5a668e693bc2ffc03056c00de13e61ae87a8"
upstream_locator: "selected substantive sections"
snapshot_kind: "structured relevant documentation excerpt preserving literal API names"
completeness: "selected substantive sections; not the full upstream file"
license: "RCCL repository/documentation terms; file SPDX not captured"
---

```rst
.. meta::
   :description: How RCCL handles errors and supports fault tolerance for multi-GPU and multi-node collective communication on AMD GPUs
   :keywords: RCCL, ROCm, AMD, fault tolerance, error handling, communicator abort, shrink, grow, revoke

***************************
Fault tolerance in RCCL
***************************

Large-scale jobs running across many AMD GPUs and nodes must survive failures
such as a network link going down, an ECC error, a node crash, or a process that
exits unexpectedly. RCCL provides APIs to detect these conditions, release the
affected resources, and keep running without restarting the whole job.

Fault tolerance relies on communicators created in non-blocking mode. With
``config.blocking = 0``, every RCCL call (except ``ncclCommDestroy`` and
``ncclCommAbort``) returns immediately, so the application can react to a hang
or a failure instead of being stuck inside a collective.

Error handling and communicator abort
=====================================

``ncclSystemError``: system call, for example a network failure; abort and re-create.
``ncclRemoteError``: remote process exited or a network error occurred; abort, then shrink or re-create.
``ncclInProgress``: poll with ``ncclCommGetAsyncError``.
``ncclTimeout``: operation exceeded its time limit; abort, then shrink or re-create.

A fatal error applies to all communicators in the same group. To recover, call
``ncclCommAbort`` on every affected communicator and re-create it.

Asynchronous errors
===================

Network failures surface later through ``ncclCommGetAsyncError``. In
non-blocking mode, poll it until the communicator leaves ``ncclInProgress``.
When waiting for a collective, query the stream and poll asynchronous errors at
the same time instead of blocking in ``hipStreamSynchronize``.

Recovering from a failure
=========================

* Abort and re-create the whole communicator.
* Shrink to drop failed ranks.
* Grow to bring replacement ranks back in.
* Revoke to abort in-flight work without destroying the communicator.

Abort requires non-blocking communicators, and no thread may be inside an RCCL
call when ``ncclCommAbort`` is invoked.

``ncclCommShrink`` creates a communicator without excluded ranks and renumbers
survivors contiguously. ``NCCL_SHRINK_ABORT`` aborts in-flight work first;
``NCCL_SHRINK_DEFAULT`` assumes no in-flight work or a preceding revoke.

``ncclCommGrow`` uses a single-use unique ID distributed out of band. Existing
ranks retain their rank numbers; new ranks are appended. The parent must have no
outstanding operations. Non-blocking grow is polled with
``ncclCommGetAsyncError``.

``ncclCommRevoke`` aborts in-flight collectives without destroying the parent.
Output buffers of an aborted collective contain undefined data. New collectives
on the revoked parent return ``ncclInvalidUsage``; the parent remains usable for
split/shrink and teardown.

For clean shutdown, finalize, poll until globally quiescent, and destroy. Use
abort instead when outstanding work cannot be drained.
```
