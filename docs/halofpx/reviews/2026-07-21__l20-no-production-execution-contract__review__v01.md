# Independent review: L20 no-production execution contract

Date: 2026-07-21

Verdict: **REJECT — TERMINAL NOT PROMOTED**

## Accepted evidence

The small two-host fixture demonstrates three genuine model residencies with a
real worker restart between capture and restore. Each residency reports one
load. The long-prefix path chunks at 512/512/104, all continuation hashes
agree, corrupt-object and plan-mismatch paths cold-recompute, and the bounded
state windows contain no legacy GET_TENSOR/SET_TENSOR operation.

## Blocking findings

1. **P1:** no early allocation-refusal child/unit case was exercised and
   retained through the candidate evidence collector.
2. **P1:** the manifest's evidence root was not enforced and source, build, and
   state roots were outside manifest-owned cleanup.
3. **P1:** `systemd-run --collect` could unload a canary unit before its
   InvocationID was captured; the saved pre-child cursor was not used as a
   fail-closed lower bound.
4. **P1:** evidence commands allowed failure and persisted error text without
   necessarily failing the maintenance result.
5. **P2:** the successful v6 evidence lacked its own production-before runtime
   snapshot.

The reviewer rejected committing the candidate implementation. The terminal
docs-only closeout preserves the scoped three-residency result, removes the
unaccepted source, records full cleanup and final production health, and does
not weaken the canary or authorize continuation.
