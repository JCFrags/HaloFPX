# L10a anchor-first generation-one selection

Status: **implemented and target-qualified; server automatic writeback remains closed**

L10a opens the first implementation seam authorized by ADR-0037. It lets the
default-off full-v1 adapter discover the sole generation-one selected manifest
from the authenticated fixed anchor, then delegates to the existing exact
restore path. It adds no normal completion integration, automatic publication,
runtime mode, production persistence, or feature-on default.

## Implementation

The protected-canary anchor codec now has a bounded production decoder and an
opaque authenticated carrier. The decoder consumes the complete canonical
CBOR envelope, rejects noncanonical lengths and integers, verifies every map
key and fixed generation-one/null-predecessor policy field, checks trusted
store/namespace/lineage and protected-key metadata, and then invokes the
existing byte-exact MAC verifier. The selected-manifest digest is exposed only
after all authentication succeeds.

The server adapter's `restore_selected()` opens only contained fixed
`anchor.v1`, enforces the existing owner/device/mount/type/mode/size root
policy, authenticates the anchor, obtains its selected digest, and calls the
existing `restore()`. That path still authenticates the selected manifest,
closed compatibility domain, object roster, generation-one authority,
checkpoint lineage, exact expected tokens, profile, topology, and full state
before returning a snapshot. A missing anchor is `miss-not-found`; malformed,
truncated, corrupt, wrong-key, or wrong-authority material is `miss-corrupt`.
There is no directory enumeration or filename-derived authority.

## Focused Linux qualification

The nimo-2 Release CPU build passed 4/4 focused and inherited tests:

- protected-anchor authenticated decode and rejection coverage;
- server publish, close/reopen, anchor-selected exact restore, exact token/state
  equality, wrong-scope miss, and corrupted-anchor miss;
- inherited generation-one authority; and
- inherited generation-one static contract.

The first strengthened fixture reported `storage` because it omitted the
authority's required `staging`, `objects`, and `manifests` directories. Review
corrected the disposable fixture to create the exact 0700 layout; no product
code was weakened. The fresh corrected run passed 4/4.

Raw final test output, source hashes, and binary hashes remain on nimo-2 under
`/var/tmp/halofpx-l10a-evidence-20260720`. The compressed bundle is
`/var/tmp/halofpx-l10a-evidence-20260720.tar.zst`, SHA-256
`f7b2fd617e4b2f1ff8c8a10ac0b6dd1143313752f46ee82ab3e5325812fbdbeb`.

## Boundary and next milestone

L10a is target-native and remains reachable only through the already excluded
full-v1 canary library. Normal builds and requests gain no root access, read,
write, lookup, option, help text, or response change. Existing explicit-handle
behavior remains unchanged.

The next milestone derives the private authenticated exact request key and
wires one normal greedy-memoryless completion to automatic miss, cold prompt
processing, prompt-boundary publication, process restart, and exact hit under
a distinct compile-and-runtime canary opt-in. It must preserve cold
recomputation for every mismatch and may not add prefix, shared, multi-entry,
multi-generation, or overwrite behavior.

No donor implementation, GPL llama-ai code, CachyLLama transplant, dependency,
WebUI, remote, model, deployment, reference clone, or persistent user root was
changed.
