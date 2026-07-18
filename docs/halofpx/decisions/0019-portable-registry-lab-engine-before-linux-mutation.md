# ADR-0019: portable registry-lab engine before Linux mutation

- Status: accepted for portable fake-engine and encoder implementation only; Linux mutation remains closed
- Date: 2026-07-18

## Decision

The next L05o implementation lane is a portable, no-system-call transaction
and recovery engine over an internal compile-time fake-operations type. It is
`STATIC EXCLUDE_FROM_ALL`, has no product/server/provider link, and cannot
produce any L05k/L05l/L05m/L05n value. It adds the canonical target-owned
encoders required by ADR-0018 before any Linux initializer or compare-and-
advance writer exists.

Only after the portable engine passes every-boundary fault qualification and
independent review may a separate Linux primitive adapter be added. Its first
lane admits sealed-fd credentials and a still-empty pre-initialization
disposable root, and separately qualifies `openat2`, mount-identity, and OFD-
lock primitives on non-authoritative fixtures. It cannot call any root
initialized or authenticate a marker/HEAD as admitted state. No external
fixture writer may bypass ADR-0018 initialization. Initialization and compare-
and-advance remain compile-time unavailable until their complete fake engines
and boundary matrices are separately accepted.

No runtime option, environment variable, default target, server argument, or
automatic root discovery is introduced. This sequencing changes no ADR-0018
wire, path, credential, synchronization, recovery, or threat-model decision.

## Target and authority boundaries

The portable target owns:

- deterministic encoders for authenticated root, HEAD, PREPARE, CLOSE, ABORT,
  and QUARANTINE objects that reproduce the accepted CDDL/golden vector;
- a bounded transaction/recovery state machine templated on an internal final
  operations type;
- a fake operations implementation with a fixed state image and fault point
  before and after every modeled operation; and
- a closed result type with only a fake-specific, non-authoritative
  `modeled_registry_lab_terminal_disposition` and bounded operation trace.

The engine never owns a path, fd, syscall number, mount, real clock, thread,
process, callback, environment lookup, or logging sink. It never invokes user
code while its modeled lock is held. The fake target contains no concrete-
observation type, factory, or symbol. Fake state, traces, and dispositions are
not convertible to concrete Linux, synthetic bootstrap, material, anchor,
cache, restore, or inference authority. Only the future Linux mutation target
may define and construct `concrete_registry_lab_observation`, after real CLOSE
readback, file synchronization, and attempt-directory synchronization.

The future Linux target is separately named, built only when
`CMAKE_SYSTEM_NAME` is exactly `Linux`, remains `EXCLUDE_FROM_ALL`, and is
reachable only through a default-off laboratory build option. A second
default-off compile gate is required before mutation code can even be built.

## Closed statuses

The engine exposes only:

- `invalid_request_no_mutation`;
- `unsupported_no_mutation`;
- `busy_no_mutation`;
- `capacity_exhausted_no_mutation`;
- `reserve_exhausted_no_mutation`;
- `attempt_replayed_no_mutation`;
- `slot_occupied_no_mutation`;
- `invalid_transition_no_mutation`;
- `preexisting_material_no_authority`;
- `aborted_predecessor_mismatch_no_authority`;
- `uncertain_requires_recovery`;
- `quarantined_or_unavailable`;
- `recovered_not_applied_no_authority`; and
- fake-only terminal dispositions named `modeled_advanced_closed` or
  `modeled_recovered_successor_closed`.

No result carries a secret, raw path, reusable absence result, or response-
supplied phase. A modeled terminal disposition is test evidence only and
cannot exist until terminal CLOSE readback, file synchronization, and modeled
directory synchronization are confirmed. It is never a positive authority
observation.

## Canonical encoder contract

Each encoder accepts a kind-specific bounded value object, the admitted
move-only credential by const reference, and a caller-owned output span. It
must:

1. reject invalid enums, zero-required identities, invalid optionals, overflow,
   output overlap, and undersized/oversized spans before output publication;
2. emit the exact deterministic CBOR map order and canonical integer/length
   encoding frozen by ADR-0018;
3. derive only the purpose-separated L05o key and wipe derived key, tag, and
   scratch storage on every path;
4. accept a separately obtained, already validated lifecycle witness that the
   encoder cannot derive from the body it is encoding; HEAD witnesses include
   the exact independently authenticated selected envelope, and PREPARE or
   terminal witnesses include independently authenticated predecessor,
   successor, HEAD, PREPARE, and chain context as applicable;
5. encode and self-verify entirely in bounded private scratch, including exact
   inner-envelope authentication, then copy to the caller span only on success;
   input and output half-open address ranges must be nonoverlapping without
   integer wrap, and failure leaves the entire caller span unchanged; and
6. return encoded length and content digest only after exact verification of
   private scratch. Temporary key/tag copies are wiped, while the authenticated
   tag intentionally published inside the successful output remains.

Golden equality is necessary but not sufficient. Tests must generate arbitrary
high-water values, selector generations, optional quarantine shapes, terminal
classes, and boundary lengths. The accepted Python oracle remains independent;
the C++ test encoder is not an authority oracle.

## Credential and lock lifetime

The engine consumes a move-only scoped credential owner. Its secret and every
derived key remain in one non-relocating bounded owner for the complete modeled
operation. On every success, error, exception, quarantine, and simulated-death
return, the engine wipes the credential and derived state with a no-throw
operation before releasing the modeled writer lock, then invalidates the moved-
from request. No disposition escapes while the modeled lock remains held.
Tests inspect wipe-before-release ordering for moved-from, overwrite,
exception, terminal, quarantine, and every fault boundary. The future Linux
owner adds `mlock`/`munlock`; the portable engine makes no locked-memory claim.

## Internal operations and fault model

The engine calls a closed internal operations vocabulary. Each line below is
one indivisible modeled call with a stable numeric operation ID; the fault seam
injects immediately before or immediately after that call:

- 1 guard acquire; 2 candidate lock acquire; 3 under-lock preflight; 4 fixed-
  layout/snapshot load; 5 recovery validation;
- 10 PREPARE create; 11 PREPARE write; 12 PREPARE readback; 13 PREPARE file
  sync; 14 attempts-directory sync;
- 20 current HEAD read; 21 resolved predecessor read;
- 30 successor-staging create; 31 successor write; 32 successor readback;
  33 successor file sync; 34 successor no-replace rename; 35 envelopes-
  directory sync;
- 40 selector-staging create; 41 selector write; 42 selector readback;
  43 selector file sync; 44 HEAD replacement; 45 root-directory sync;
- 50 post-replacement HEAD read; 51 selected successor read;
- 60 terminal create; 61 terminal write; 62 terminal readback; 63 terminal
  file sync; 64 attempts-directory sync;
- 70 quarantine-staging create; 71 quarantine write; 72 quarantine readback;
  73 quarantine file sync; 74 quarantine no-replace rename; 75 root-directory
  sync; and
- 90 credential/derived-state wipe; 91 writer-lock release; 92 guard release.

Operation 10 is the first possible mutation of a new CAS attempt, but recovery
or quarantine may mutate earlier in the same lock acquisition. Initialization
has no operation IDs in this milestone and cannot be modeled or claimed.

Every call returns an orthogonal product. Storage-effect extent is one of
`none`, `bounded_partial_bytes`, `complete_live`,
`bounded_partial_durability_projection`, or
`complete_durability_projection`. Completion is one of `response_confirmed`,
`response_lost`, or `process_death`. The contract enumerates valid combinations
per operation; for example a confirmed successful synchronization requires
`complete_durability_projection + response_confirmed`, while process death can
combine with partial, complete-live, or complete-durable effect where the
operation permits it.

The following table is normative. `C`, `L`, and `D` abbreviate confirmed
response, lost response, and modeled process death. A row permits only the
listed products; every other product is invalid and must be rejected by the
fake before engine execution.

| operation IDs | primitive | allowed storage effects | allowed completion | restart projection |
|---|---|---|---|---|
| 1 | guard acquire | `none` | C or D | guard is clear after D |
| 2 | writer-lock acquire | `none` | C or D | lock is clear after D |
| 3-5 | preflight/snapshot/recovery validation | `none` | C, L, or D | storage unchanged |
| 10, 30, 40, 60, 70 | unique file create | `none` or `complete_live` empty atomic name | C, L, or D | unsynchronized name is either absent or wholly present; never partial |
| 11, 31, 41, 61, 71 | bounded write | `none`, `bounded_partial_bytes`, or `complete_live` | C, L, or D | any retained prefix no longer than the live effect, including none; never bytes not issued |
| 12, 20-21, 32, 42, 50-51, 62, 72 | read/readback | `none` | C, L, or D | storage unchanged |
| 13, 33, 43, 63, 73 | file synchronization | `none`, `bounded_partial_durability_projection`, or `complete_durability_projection` | C, L, or D | restart retains exactly the advanced file projection; confirmed success requires complete durability |
| 14, 35, 45, 64, 75 | directory synchronization | `none`, `bounded_partial_durability_projection`, or `complete_durability_projection` | C, L, or D | restart retains exactly the advanced namespace projection; confirmed success requires complete durability |
| 34, 74 | no-replace rename | `none` or atomic `complete_live` namespace replacement | C, L, or D | before directory sync, either whole old or whole new namespace state may remain; partial names are forbidden |
| 44 | atomic HEAD replacement | `none` or atomic `complete_live` namespace replacement | C, L, or D | before root sync, either whole old or whole new HEAD may remain; mixed or partial HEAD names are forbidden |
| 90 | credential/derived-state wipe | `none` storage effect | C only | non-faultable, no-throw; memory owner is provably zeroed |
| 91 | writer-lock release | `none` | C only | non-faultable, no-throw; requires completed operation 90 |
| 92 | guard release | `none` | C only | non-faultable, no-throw; requires completed operations 90 and 91 |

Create is namespace-atomic and produces an empty live file; content appears only
through its separate write operation. A no-replace rename or HEAD replacement
is likewise namespace-atomic. File sync can advance only bytes of that file
already present in the live projection. Directory sync can advance only live
namespace changes in that directory. Neither sync invents bytes or names. A
confirmed successful sync is invalid unless its corresponding projection is
complete. Operations 90-92 are cleanup steps, not injectable fault points.

The fake retains separate live bytes/names and synchronized byte/directory
projections. For every effect not proven in the durability projection, restart
branches across every retained or discarded outcome permitted by the modeled
operation's table row, write/rename atomicity, and ordering constraints. It never assumes
that unsynchronized means absent. Thus partial PREPARE and HEAD replacement may
survive or disappear when the contract permits either. The fake can also
return short I/O, `EINTR`, space/quota/read-only/I/O errors, resource exhaustion,
and late completion without a callback.

Each recovery action, quarantine action, and subsequent CAS attempt owns a
dynamic `before_first_mutation` transition. Immediately before whichever
create/write/sync/rename is first for that action, the engine recomputes the
16 MiB worst-case logical-byte bound, rechecks reserve, and marks uncertainty
before invoking the call. Recovery or quarantine mutation failure remains
uncertain/quarantined. Only a wholly read-only recovery followed by a new CAS
reaches operation 10 as the lock acquisition's first mutation. Exceptions and
allocation failure at or after any action's transition cannot become definite
no-mutation.

## Transaction and recovery invariants

All 512 fixed slots are examined under the modeled lock. An attempt ID present
in any slot is replay; an occupied requested slot never wraps, evicts, or
reuses. Pre-lock inspection may locate a candidate marker/lock only; complete
identity/layout/current-state validation repeats under lock before authority.

CAS follows ADR-0018 steps 1 through 9 exactly. A pre-existing successor,
selector staging object, or exact successor HEAD cannot attribute work to the
current attempt. The only definite operation-time ABORT predicate is a byte-
exact predecessor mismatch proved before selector replacement. The only
recovery ABORT predicate is one valid unresolved PREPARE with authenticated
HEAD and resolved envelope byte-exactly equal to its predecessor. Every other
post-PREPARE non-success path is uncertain or quarantine; pre-existing equal
material never broadens ABORT or grants attribution. Exact byte comparison is
required wherever ADR-0018 requires it.

Every lock acquisition runs recovery first. A lone valid unresolved PREPARE
with HEAD exactly at successor may synchronize and CLOSE for a fake-only
terminal disposition; HEAD exactly at predecessor receives ABORT and no authority.
Multiple unresolved attempts, contradictions, missing referents, unexpected
objects, ambiguous staging, or an unprovable boundary become sticky quarantine.
Malformed QUARANTINE or retained quarantine staging blocks forever. No time,
directory order, or maximum generation selects authority.

## Required promotion tests

Before the Linux read-only adapter opens, independent review must accept:

- exact equality with every accepted golden fixture and independent digest;
- general high-water/generation and all quarantine optional shapes;
- wrong modelled record, digest, length, domain, key, phase, terminal class,
  predecessor, successor, receipt, operation, and continuity rejection;
- before/after faults at every operation boundary, partial I/O, late completion,
  allocation/exception paths, and no terminal disposition before CLOSE
  directory synchronization;
- replay, all 512 slots, capacity plus one, slot conflict, same-process
  recursion, lock timeout, and two-engine exactly-one outcomes;
- sticky quarantine and recovery idempotence across serialized fake-state
  restart;
- initializing or incomplete markers; full fixed-layout and unexpected-entry
  rejection; validation of every referenced immutable envelope; 16 MiB worst-
  case logical-byte accounting; and dynamic reserve/uncertainty admission before
  the first mutation of every recovery, quarantine, and CAS action;
- restart branching for every valid retained/discarded non-durable projection,
  including partial PREPARE and HEAD replacement, plus all valid combinations
  of effect extent with confirmed response, lost response, and process death;
- explicit rejection tests for every effect/completion product forbidden by
  the normative operation table, including partial-name create/rename/HEAD,
  sync that invents state, confirmed incomplete sync, and faulted cleanup;
- malformed or partial PREPARE, terminal, selector, successor staging, HEAD,
  quarantine, and quarantine-staging objects; terminal publication/readback/
  file-sync/directory-sync failures; and every quarantine publication failure;
- ASan/UBSan on Linux plus clean Release/Debug builds;
- feature-off, full HaloFPX, and focused inherited regressions; and
- source/include forbidden-surface scans with synthetic detector self-tests;
  dependency-closure and link-graph assertions; Windows and Linux object/archive
  undefined-symbol/import audits; and proof that engine/fake cannot link path,
  syscall, Linux-adapter, product, synthetic, donor, material, anchor, cache,
  restore, or inference code and product targets cannot link back to it;
- a compile/link negative test proving no fake target contains or can construct
  a `concrete_registry_lab_observation` symbol.

## Non-claims and rollback

This lane proves only deterministic encoding and behavior of a bounded fake
state machine. It provides no Linux syscall evidence, credential custody,
filesystem state, cross-process fencing, process-crash evidence, persistence,
durability, rollback resistance, material/anchor operation, cache hit, restore,
runtime behavior, or performance claim. Rollback is removal of the excluded
targets and their tests; no node or deployment state exists to recover.
