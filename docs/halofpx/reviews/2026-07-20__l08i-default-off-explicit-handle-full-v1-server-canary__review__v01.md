# L08i default-off explicit-handle full-v1 server canary independent review v01

Verdict: **ACCEPT** for the Linux-only, default-off L08i laboratory canary. No
P1/P2 blocker remains.

The four compile gates and explicit runtime mode confine persistence. The
caller-supplied handle is parsed as one exact digest and used only to derive a
fixed contained manifest name. The file is type-, ownership-, mode-, mount-,
and size-checked before its closed compatibility domain and authentication are
verified. Only then is its bounded admission roster copied into L08h-b, which
authenticates and reconciles the exact protected anchor and material before a
candidate can reach the transformer decoder.

Decode finishes before the live llama restore boundary. The server inserts
prompt tokens only after the state restore succeeds; failure wipes the snapshot
state and clears the empty destination slot so cold recomputation remains
authoritative. Authentication supplies the private principal, while request
data supplies neither filesystem paths nor authority identity. Restore requires
exact session, selected manifest, and bounded integer tokens.

The current edge is processed by the single server-queue controller loop and
therefore meets L08h-b's one-owner condition. This is an external serialization
contract, not an adapter-local guard. Before exposing the adapter through any
other path, add internal serialization or a controller-thread assertion.
Similarly, add complete transient frame wiping before broader sensitive or
large-state use, and reject nested roots at startup for clearer diagnostics;
L08h-b already rejects them before lookup or publication, so the current path
fails closed.

The focused process canary was tightened after review to require exact
`miss-not-found` and `miss-corrupt` statuses. It passed again with a new raw
evidence root. The successful run retained only the disposable operator key's
SHA-256 and removed the key file. The earlier disposable run's key was also
removed after its exact path and hash were verified.

Review against canonical Wiki Sections 61, 63, and 71 found alignment with
complete admitted state, private authenticated scope, bounded authenticated
format, explicit selected generation, and corruption-as-miss. Production
durability, quota/reserve, retention, multi-node ownership, broader fault
matrices, and soak remain correctly deferred. No donor, GPL llama-ai, WebUI,
dependency, remote, deployment, or reference-clone boundary was crossed.
