# io_uring and liburing contract

Semantic anchors: Linux 7.1.3 and liburing 2.15.

## Completion and durability

A CQE reports the result of one submitted operation. For write-like operations, `cqe->res` is bytes transferred or a negated errno. A successful write CQE is not a durability boundary. The candidate sequence requires exact write completion, then a successful fsync/fdatasync CQE, then successful namespace publication, then a successful directory-fsync CQE.

Linux 7.1.3 implements `IORING_OP_FSYNC` with `vfs_fsync_range`; `IORING_FSYNC_DATASYNC` selects datasync behavior. That establishes syscall equivalence, not deployed power-loss proof.

## Registered files

Registration holds kernel file references. Closing the ordinary descriptor does not cancel pending requests. Slot update or unregister can detach the table entry while an in-flight request retains the old resource. Use resource tags when lifecycle certainty matters.

## Registered buffers

Registration pins/maps application memory. Do not resize, unmap, free, repurpose, or return a buffer to another allocator while any request or old registered-resource generation can still reference it. A resource **update/replacement** may return while the old resource remains alive; a tag CQE can signal terminal release. Explicit `IORING_UNREGISTER_BUFFERS` is documented as synchronous, while ring-shutdown unpinning can finish asynchronously. Keep these cases distinct and test the selected lifecycle.

## Cancellation and late CQEs

Cancellation is inherently racy. Track two logical operations: target and cancel request. Their CQEs can arrive in either order. The cancel result can be success, `-ENOENT` or `-EALREADY`; the target can complete normally, fail, or be cancelled. Disk I/O already submitted to hardware is commonly not cancellable.

`user_data` should encode at least operation kind, cache-root ID, object generation and resource/slot generation. Never store a raw pointer that can be freed and reallocated before all terminal CQEs are consumed.

## Linked operations

Links sequence execution. Soft links cancel downstream requests only when a prior result is negative. A **positive short write is not negative**, so a blindly linked fsync can run after incomplete data. The safe default is:

1. submit write(s);
2. consume every CQE and prove exact total length;
3. submit file sync;
4. consume and check it;
5. publish;
6. submit/perform directory sync and check it.

Hard links deliberately continue after errors and are unsuitable for durability sequencing unless every result is separately interpreted.

## Teardown

Graceful shutdown: stop new SQEs; cancel outstanding operations; drain target, cancel, multishot-final and auxiliary/tag CQEs; unregister files/buffers; wait for release tags when used; then `io_uring_queue_exit`. Ring teardown cancels pending work but does not convert completed writes into synchronized writes or make application memory safe before terminal lifetime events.
