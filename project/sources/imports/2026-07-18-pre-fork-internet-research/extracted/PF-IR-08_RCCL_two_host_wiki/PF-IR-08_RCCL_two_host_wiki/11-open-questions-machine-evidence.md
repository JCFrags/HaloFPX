# Open questions requiring machine evidence

1. Does the installed candidate RCCL binary contain usable gfx1151 code and initialize on each host?
2. Which Linux interface name, MTU, address family, route, and reported link speed correspond to Ethernet-over-USB4?
3. Does RCCL choose Socket without silently falling back or selecting another interface?
4. Does the two-rank communicator initialize repeatedly without race or stale-state failures?
5. Which collectives and message sizes are correct, and where are the latency/bandwidth knees?
6. What CPU and memory-bandwidth cost is imposed by the host-only Socket path on unified-memory APUs?
7. How do peer kill, link loss, delayed peer, and interface removal surface through async state and wall-clock behavior?
8. Can abort followed by a fresh unique ID and communicator reliably restore service?
9. Does stable shrink work meaningfully with only two ranks, where losing one rank collapses tensor parallelism to one survivor?
10. Do active revoke/grow semantics behave on this platform, and are they valuable for a two-rank inference application?
11. Does a framework tensor-parallel adapter deliver an end-to-end gain over ggml RPC or pipeline split after synchronization overhead?
12. Is there a quantified requirement that justifies a custom USB4STREAM Net provider?

Until these are measured, “supported,” “GPU-direct,” “suitable,” and “reconnects” remain prohibited conclusions for the target pair.
