# L05z response-loss qualification independent review

**Result: ACCEPT_FOR_NARROW_RESPONSE_LOSS_MILESTONE_NOT_L05Z_PROMOTION.**

This milestone admits exactly one target-owned test case,
`L05Z-RSP-LOSS-FULL-001`, outside the frozen 8,612-case returned-fault
compatibility authority. The separate response manifest freezes one semantic
identity and dedup key, an extended total of 8,613, combined ID and manifest
commitments, distinctness from every compatibility case, and the exact
17-shard distribution. Physical syscall kind, fragment count, and response
length remain profile metadata rather than semantic identity.

The implemented selector is narrow and exact. Before fork it pins the private
audit pipe's FIFO device, inode, and mount. In response mode it accepts only the
launcher's sole direct `PTRACE_EVENT_FORK` child after launcher exec, exact
child executable and argv, and exact fd-1 pipe identity at both exec and write.
`VFORK`, `CLONE`, a second child, grandchild substitution, wrong PID, wrong fd,
rebound pipe, second fragment, and a live `writev` profile reject. Payload and
`iovec` table arithmetic is bounded against count, size, aggregate, and address
wrap before tracee memory access.

Six prior fresh discovery runs established the admitted physical profile:
nimo-1 emitted 1,117 bytes in each of three runs and nimo-2 emitted 1,119 bytes
in each of three runs; all six used exactly one complete `SYS_write`, no
`SYS_writev`, and returned child and launcher status zero. The implementation
does not pin either observed byte length. It accepts only a nonempty transcript
up to 65,536 bytes and then applies the complete semantic audit oracle.

On the final nimo-2 fresh fully allocated 1 GiB nodiscard Btrfs canary, the
controller captured the 1,119-byte attempted response, substituted `ENOSYS`
before kernel delivery, required the real `-ENOSYS` exit, and synthesized the
exact full return. The independently captured transcript SHA-256 was
`64db76f5...`; the full qualified phase-13 audit passed; the consumer observed
zero bytes followed by EOF; and controller, child, and launcher all returned
zero. Exact publication counts, final tree, object bytes, authorities, writer
and fixture lock release, and sticky whole-root discard also passed. The raw
attempted response is not retained in the project receipt. The controller
overwrites its owned captured-transcript buffer after validation; ordinary
`exact_audit` parser temporaries are destroyed normally and are not claimed to
be securely erased.

A separate fresh-media `close-step4/pre/EIO/occurrence49` run proves focused
legacy compatibility. It emitted no response-loss event, matched the previously
accepted three-row receipt after timestamp removal, retained the identical
final tree, and released both locks. The equivalence proof SHA-256 is
`d0294ff8...`. Both canary images remained exactly 1,073,741,824 bytes and
2,097,152 allocated 512-byte blocks at every recorded stage, then were
identity-checked and removed. Final mount, loop, and process residue was zero.

Strict C++17 compilation with `-Wall -Wextra -Wpedantic -Werror`, response
decoder and authority self-tests, one-case selection, five parser negatives,
the corrected static seam contract, and the focused inherited feature-off,
initializer-anchor, and Linux build-gate controls passed. The first focused
feature-off invocation was a preserved setup failure because that old build
directory had not built `llama-server`; after building the declared target, the
fresh rerun passed 3/3. It is not classified as a regression.

The retained nimo-2 bundle is
`/var/tmp/halofpx-qualification/l05z-response-loss-canary-d7a9150-20260719-nimo2-evidence.tar.zst`,
SHA-256 `f6302ad6...`, 154,276 bytes, with 70 safe members and 68 artifact
manifest records. Independent extraction and `sha256sum -c` reconstruction
passed. The RPC service remained PID 3562775 with zero reported restarts and
the listener unchanged. nimo-1's response suppression canary was not started
because its available `/var/tmp` space remained below the fixed 64 GiB reserve;
the three-run discovery evidence remains valid, but two-node suppression
qualification remains open.

The profile boundary is important: the bounded `writev` decoder is not live
admitted. `sendfile`, `splice`, `vmsplice`, `tee`, `copy_file_range`,
`io_uring`, and other fd-1 mechanisms are outside this case and may be detected
only after bytes reach the private controller pipe. This milestone therefore
proves pre-kernel suppression only for the discovered one-`SYS_write` profile,
not universal output suppression.

All four immutable references remain clean at their locked commit and tree.
The changed implementation is excluded test infrastructure and a static
contract: it adds no product link, runtime flag, persistence path, storage
window, dependency, install/export edge, remote, donor implementation, GPL
llama-ai code, or documentation copied from llama-ai. Feature-off and server
defaults are unchanged. Rollback removes the response manifest/controller
case and its documentation while preserving every existing root as
discard-only.

This acceptance does not admit a second transport profile, close the nimo-1
reserve skip, admit the 247 pending roles, qualify full returned-fault or
sanitizer scale, promote L05z, enable persistent writes or cache hits, or make
an inference-performance or zero-regression claim.
