# P model research plan

`HaloKV.p` is an implementation-near starter sketch: coordinator and rank machines exchange begin/prepared/commit/cancel messages, while a monitor checks that commit contains both ranks and the current epoch. Adapt and compile it against the selected P release before use; this package does not claim that the sketch has been compiled.

Recommended extensions:

- nondeterministic network machine for drop, duplicate, delay, and reorder;
- authority machine with conditional terminal update;
- crash/restart and durable high-epoch state;
- credit and cancellation propagation;
- reconnect inventory and attach barrier;
- monitor for corrupt materialization and single-node decision preconditions;
- event adapter from production traces.
