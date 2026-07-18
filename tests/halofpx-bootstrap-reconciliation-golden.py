"""Independent stdlib-only golden for ADR-0015 reconciliation commitment."""

import hashlib
import json
import pathlib
import sys


DOMAIN = b"halofpx.bootstrap-consumption-reconciliation.v1\0"
ROOT_IDENTITY = bytes.fromhex("33" * 32)
RECONCILIATION_ATTEMPT_ID = bytes.fromhex("55" * 32)
ORIGINAL_ATTEMPT_ID = bytes.fromhex("44" * 32)
ORIGINAL_OPERATION_COMMITMENT = bytes.fromhex("66" * 32)
PREDECESSOR = bytes.fromhex(
    "a200a400a8000101000200036b72656769737472792d76310409055820"
    + "aa" * 32
    + "065820"
    + "bb" * 32
    + "071828017072656769737472792d617574682d76310201030d015820"
    + "8f563d0a255e6b09a44b87be0867e84b4b8368d043e5f885fa86348662c509d4"
)
SUCCESSOR = bytes.fromhex(
    "a200a400aa000201000200036b72656769737472792d76310409055820"
    + "aa" * 32
    + "065820"
    + "bb" * 32
    + "071829085820a7b731bccfdea83a4595d5257ffa34ef9248bb61499b40a37874895cff6bc1ec"
    + "09a6001829015820"
    + "cc" * 32
    + "025820"
    + "dd" * 32
    + "035820"
    + "ee" * 32
    + "045820"
    + "11" * 32
    + "055820"
    + "22" * 32
    + "017072656769737472792d617574682d76310201030d015820"
    + "1f3f79689d0b2dd37988aac8c92bf90be6689b0079bd5fc4a7066973fdebd202"
)
ANCHOR = bytes.fromhex(
    "a200a400ab000101000250"
    + "11" * 16
    + "035820"
    + "22" * 32
    + "0403055820"
    + "44" * 32
    + "060507060802095820"
    + "77" * 32
    + "0a5820"
    + "88" * 32
    + "016d616e63686f722d6b65792d763102010309015820"
    + "41b4d7a3821784aa8776ac4dad38db57ffea381e892d597e0efca1b9717274a3"
)


def u64be(value):
    if not 0 <= value <= 0xFFFFFFFFFFFFFFFF:
        raise ValueError("outside uint64")
    return value.to_bytes(8, "big")


def preimage(
    domain=DOMAIN,
    root=ROOT_IDENTITY,
    reconciliation_attempt=RECONCILIATION_ATTEMPT_ID,
    original_attempt=ORIGINAL_ATTEMPT_ID,
    original_operation=ORIGINAL_OPERATION_COMMITMENT,
    predecessor=PREDECESSOR,
    successor=SUCCESSOR,
    anchor=ANCHOR,
):
    return b"".join(
        (
            domain,
            root,
            reconciliation_attempt,
            original_attempt,
            original_operation,
            u64be(len(predecessor)),
            predecessor,
            u64be(len(successor)),
            successor,
            u64be(len(anchor)),
            anchor,
        )
    )


def changed(value):
    out = bytearray(value)
    out[0] ^= 1
    return bytes(out)


golden_preimage = preimage()
golden_commitment = hashlib.sha256(golden_preimage).digest()

# These checks exercise every fixed field, the exact one-NUL domain, and order.
mutations = {
    "domain": preimage(domain=DOMAIN[:-1] + b".mutated\0"),
    "root_identity": preimage(root=changed(ROOT_IDENTITY)),
    "reconciliation_attempt_id": preimage(reconciliation_attempt=changed(RECONCILIATION_ATTEMPT_ID)),
    "original_attempt_id": preimage(original_attempt=changed(ORIGINAL_ATTEMPT_ID)),
    "original_operation_commitment": preimage(original_operation=changed(ORIGINAL_OPERATION_COMMITMENT)),
    "predecessor_envelope": preimage(predecessor=changed(PREDECESSOR)),
    "successor_envelope": preimage(successor=changed(SUCCESSOR)),
    "anchor_envelope": preimage(anchor=changed(ANCHOR)),
    "field_order": preimage(root=RECONCILIATION_ATTEMPT_ID, reconciliation_attempt=ROOT_IDENTITY),
}
if DOMAIN.count(b"\0") != 1 or not DOMAIN.endswith(b"\0"):
    raise SystemExit("domain must contain exactly one trailing NUL")
for name, candidate in mutations.items():
    if hashlib.sha256(candidate).digest() == golden_commitment:
        raise SystemExit(f"mutation was not detected: {name}")

result = {
    "anchor_envelope_bytes": len(ANCHOR),
    "anchor_envelope_sha256_hex": hashlib.sha256(ANCHOR).hexdigest(),
    "domain_ascii_with_nul_hex": DOMAIN.hex(),
    "mutation_checks": len(mutations),
    "original_attempt_id_hex": ORIGINAL_ATTEMPT_ID.hex(),
    "original_operation_commitment_hex": ORIGINAL_OPERATION_COMMITMENT.hex(),
    "predecessor_envelope_bytes": len(PREDECESSOR),
    "predecessor_envelope_sha256_hex": hashlib.sha256(PREDECESSOR).hexdigest(),
    "preimage_bytes": len(golden_preimage),
    "reconciliation_attempt_id_hex": RECONCILIATION_ATTEMPT_ID.hex(),
    "reconciliation_commitment_sha256_hex": golden_commitment.hex(),
    "root_identity_hex": ROOT_IDENTITY.hex(),
    "successor_envelope_bytes": len(SUCCESSOR),
    "successor_envelope_sha256_hex": hashlib.sha256(SUCCESSOR).hexdigest(),
}

if len(sys.argv) == 3 and sys.argv[1] == "--check":
    expected = json.loads(pathlib.Path(sys.argv[2]).read_text(encoding="utf-8"))
    for key, value in result.items():
        if expected.get(key) != value:
            raise SystemExit(f"golden mismatch for {key}")
elif len(sys.argv) == 1:
    print(json.dumps(result, indent=2, sort_keys=True))
else:
    raise SystemExit(f"usage: {pathlib.Path(sys.argv[0]).name} [--check VECTOR.json]")
