"""Independent stdlib-only golden serializer for accepted ADR-0016 commitments."""

import hashlib
import json
import pathlib
import sys


DOMAINS = {
    "source_set": b"halofpx.bootstrap-material-source-set.v1\0",
    "root_policy": b"halofpx.bootstrap-material-root-policy.v1\0",
    "authority_source": b"halofpx.bootstrap-material-authority-source.v1\0",
    "material_set": b"halofpx.bootstrap-material-set.v1\0",
    "operation": b"halofpx.bootstrap-material-preparation.v1\0",
    "durable_close": b"halofpx.bootstrap-material-durable-close.v1\0",
}
TAGS = {
    "direct_advanced": b"direct-advanced-v1",
    "direct_already_same": b"direct-already-same-v1",
    "reconciled_successor": b"reconciled-successor-v1",
}


def u64be(value):
    if not 0 <= value <= 0xFFFFFFFFFFFFFFFF:
        raise ValueError("outside uint64")
    return value.to_bytes(8, "big")


def changed(value):
    out = bytearray(value)
    out[0] ^= 1
    return bytes(out)


def sha(value):
    return hashlib.sha256(value).digest()


MATERIAL_ROOT = bytes.fromhex("10" * 32)
REGISTRY_ROOT = bytes.fromhex("20" * 32)
STORE_UUID = bytes.fromhex("30" * 16)
NAMESPACE_ID = bytes.fromhex("40" * 32)
LINEAGE_ID = bytes.fromhex("50" * 32)
POLICY_EPOCH = 0x0102030405060708
KEY_GENERATION = 0x1112131415161718
WRITER_EPOCH = 0x2122232425262728
POLICY_ID = b"object-sync-manifest-sync-v1"
DURABILITY_MODE = 3
MAX_OBJECTS = 128
MAX_FRAME = 16_777_216
MAX_AGGREGATE = 67_108_864

SOURCES = (
    (0, b"HALOFPX-OBJECT-FRAME-ZERO"),
    (1, b"HALOFPX-OBJECT-FRAME-ONE-WITH-DIFFERENT-LENGTH"),
)
MANIFEST = bytes.fromhex("a30001016b6d616e69666573742d763102820102")
DESCRIPTORS = (
    (0, sha(SOURCES[0][1]), b"kv-state-v1", len(SOURCES[0][1])),
    (1, sha(SOURCES[1][1]), b"sampler-state-v1", len(SOURCES[1][1])),
)
SUCCESSOR = b"exact-authenticated-registry-successor-envelope\x00with-bytes"
ANCHOR = b"exact-authenticated-proposed-anchor-envelope\xffwith-bytes"
OBSERVED_SUCCESSOR = SUCCESSOR
ORIGINAL_ATTEMPT = bytes.fromhex("60" * 32)
ORIGINAL_OPERATION = bytes.fromhex("61" * 32)
AUTHORIZATION_SEQUENCE = 0x3132333435363738
COMMAND_ID = bytes.fromhex("62" * 32)
TOKEN_DIGEST = bytes.fromhex("63" * 32)
PLAN_COMMITMENT = bytes.fromhex("64" * 32)
SNAPSHOT_COMMITMENT = bytes.fromhex("65" * 32)
SELECTED_MANIFEST_DIGEST = sha(MANIFEST)
RECONCILIATION_ATTEMPT = bytes.fromhex("66" * 32)
RECONCILIATION_COMMITMENT = bytes.fromhex("67" * 32)
MATERIAL_ATTEMPT = bytes.fromhex("70" * 32)


def source_set_preimage(*, domain=DOMAINS["source_set"], sources=SOURCES, count=None,
                        index_overrides=None, length_overrides=None):
    count = len(sources) if count is None else count
    index_overrides = index_overrides or {}
    length_overrides = length_overrides or {}
    parts = [domain, u64be(count)]
    for position, (index, frame) in enumerate(sources):
        parts.extend((u64be(index_overrides.get(position, index)),
                      u64be(length_overrides.get(position, len(frame))), frame))
    return b"".join(parts)


SOURCE_SET_COMMITMENT = sha(source_set_preimage())


def root_policy_preimage(*, domain=DOMAINS["root_policy"], material_root=MATERIAL_ROOT,
                         registry_root=REGISTRY_ROOT, store_uuid=STORE_UUID,
                         namespace=NAMESPACE_ID, lineage=LINEAGE_ID,
                         policy_epoch=POLICY_EPOCH, key_generation=KEY_GENERATION,
                         writer_epoch=WRITER_EPOCH, policy_id=POLICY_ID,
                         policy_length=None, durability_mode=DURABILITY_MODE,
                         max_objects=MAX_OBJECTS, max_frame=MAX_FRAME,
                         max_aggregate=MAX_AGGREGATE,
                         source_set=SOURCE_SET_COMMITMENT):
    policy_length = len(policy_id) if policy_length is None else policy_length
    return b"".join((domain, material_root, registry_root, store_uuid, namespace, lineage,
                     u64be(policy_epoch), u64be(key_generation), u64be(writer_epoch),
                     u64be(policy_length), policy_id, bytes((durability_mode,)),
                     u64be(max_objects), u64be(max_frame), u64be(max_aggregate), source_set))


ROOT_POLICY_COMMITMENT = sha(root_policy_preimage())


def authority_source_preimage(*, domain=DOMAINS["authority_source"],
                              tag=TAGS["direct_advanced"], tag_length=None,
                              registry_root=REGISTRY_ROOT,
                              original_attempt=ORIGINAL_ATTEMPT,
                              original_operation=ORIGINAL_OPERATION,
                              successor=SUCCESSOR, successor_length=None,
                              anchor=ANCHOR, anchor_length=None,
                              sequence=AUTHORIZATION_SEQUENCE, command=COMMAND_ID,
                              token=TOKEN_DIGEST, plan=PLAN_COMMITMENT,
                              snapshot=SNAPSHOT_COMMITMENT,
                              selected=SELECTED_MANIFEST_DIGEST,
                              reconciliation_attempt=None,
                              reconciliation_commitment=None,
                              observed=None, observed_length=None):
    tag_length = len(tag) if tag_length is None else tag_length
    successor_length = len(successor) if successor_length is None else successor_length
    anchor_length = len(anchor) if anchor_length is None else anchor_length
    parts = [domain, u64be(tag_length), tag, registry_root, original_attempt,
             original_operation, u64be(successor_length), successor,
             u64be(anchor_length), anchor, u64be(sequence), command, token, plan,
             snapshot, selected]
    if (tag == TAGS["reconciled_successor"] or reconciliation_attempt is not None
            or reconciliation_commitment is not None or observed is not None
            or observed_length is not None):
        if reconciliation_attempt is None:
            reconciliation_attempt = RECONCILIATION_ATTEMPT
        if reconciliation_commitment is None:
            reconciliation_commitment = RECONCILIATION_COMMITMENT
        if observed is None:
            observed = OBSERVED_SUCCESSOR
        observed_length = len(observed) if observed_length is None else observed_length
        parts.extend((reconciliation_attempt, reconciliation_commitment,
                      u64be(observed_length), observed))
    return b"".join(parts)


SOURCE_COMMITMENTS = {
    name: sha(authority_source_preimage(tag=tag)) for name, tag in TAGS.items()
}


def material_set_preimage(*, domain=DOMAINS["material_set"], manifest=MANIFEST,
                          manifest_length=None, descriptors=DESCRIPTORS, count=None,
                          index_overrides=None, type_length_overrides=None):
    manifest_length = len(manifest) if manifest_length is None else manifest_length
    count = len(descriptors) if count is None else count
    index_overrides = index_overrides or {}
    type_length_overrides = type_length_overrides or {}
    parts = [domain, u64be(manifest_length), manifest, u64be(count)]
    for position, (index, object_id, stream_type, frame_length) in enumerate(descriptors):
        parts.extend((u64be(index_overrides.get(position, index)), object_id,
                      u64be(type_length_overrides.get(position, len(stream_type))),
                      stream_type, u64be(frame_length)))
    return b"".join(parts)


MATERIAL_SET_COMMITMENT = sha(material_set_preimage())


def operation_preimage(*, domain=DOMAINS["operation"], material_root=MATERIAL_ROOT,
                       registry_root=REGISTRY_ROOT, material_attempt=MATERIAL_ATTEMPT,
                       root_policy=ROOT_POLICY_COMMITMENT,
                       source=SOURCE_COMMITMENTS["direct_advanced"],
                       source_set=SOURCE_SET_COMMITMENT,
                       material_set=MATERIAL_SET_COMMITMENT,
                       selected=SELECTED_MANIFEST_DIGEST, anchor_digest=sha(ANCHOR)):
    return b"".join((domain, material_root, registry_root, material_attempt, root_policy,
                     source, source_set, material_set, selected, anchor_digest))


OPERATION_COMMITMENTS = {
    name: sha(operation_preimage(source=commitment))
    for name, commitment in SOURCE_COMMITMENTS.items()
}


def durable_close_preimage(*, domain=DOMAINS["durable_close"],
                           material_root=MATERIAL_ROOT, registry_root=REGISTRY_ROOT,
                           material_attempt=MATERIAL_ATTEMPT,
                           operation=OPERATION_COMMITMENTS["direct_advanced"],
                           source_set=SOURCE_SET_COMMITMENT,
                           material_set=MATERIAL_SET_COMMITMENT,
                           selected=SELECTED_MANIFEST_DIGEST, close_code=1):
    return b"".join((domain, material_root, registry_root, material_attempt, operation,
                     source_set, material_set, selected, bytes((close_code,))))


def mutation_summary(name, base, candidates):
    base_digest = sha(base)
    digests = []
    for mutation_name, candidate in candidates:
        digest = sha(candidate)
        if digest == base_digest:
            raise SystemExit(f"mutation was not detected: {name}.{mutation_name}")
        digests.append((mutation_name, digest))
    aggregate = hashlib.sha256()
    for mutation_name, digest in sorted(digests):
        aggregate.update(mutation_name.encode("ascii"))
        aggregate.update(b"\0")
        aggregate.update(digest)
    return {
        "count": len(digests),
        "names": sorted(mutation_name for mutation_name, _ in digests),
        "sha256_hex": aggregate.hexdigest(),
    }


for domain_name, domain in DOMAINS.items():
    if not domain.endswith(b"\0") or domain.count(b"\0") != 1:
        raise SystemExit(f"{domain_name} domain must contain exactly one trailing NUL")
for tag_name, tag in TAGS.items():
    if b"\0" in tag:
        raise SystemExit(f"{tag_name} provenance tag must not contain a NUL")

source_set_mutations = [
    ("domain_missing_nul", source_set_preimage(domain=DOMAINS["source_set"][:-1])),
    ("domain_extra_nul", source_set_preimage(domain=DOMAINS["source_set"] + b"\0")),
    ("object_count", source_set_preimage(count=3)),
    ("source_0_index", source_set_preimage(index_overrides={0: 9})),
    ("source_0_length", source_set_preimage(length_overrides={0: len(SOURCES[0][1]) + 1})),
    ("source_0_frame", source_set_preimage(sources=((0, changed(SOURCES[0][1])), SOURCES[1]))),
    ("source_1_index", source_set_preimage(index_overrides={1: 9})),
    ("source_1_length", source_set_preimage(length_overrides={1: len(SOURCES[1][1]) + 1})),
    ("source_1_frame", source_set_preimage(sources=(SOURCES[0], (1, changed(SOURCES[1][1]))))),
    ("source_order", source_set_preimage(sources=(SOURCES[1], SOURCES[0]))),
]

root_policy_mutations = [
    ("domain_missing_nul", root_policy_preimage(domain=DOMAINS["root_policy"][:-1])),
    ("domain_extra_nul", root_policy_preimage(domain=DOMAINS["root_policy"] + b"\0")),
    ("material_root", root_policy_preimage(material_root=changed(MATERIAL_ROOT))),
    ("registry_root", root_policy_preimage(registry_root=changed(REGISTRY_ROOT))),
    ("store_uuid", root_policy_preimage(store_uuid=changed(STORE_UUID))),
    ("namespace_id", root_policy_preimage(namespace=changed(NAMESPACE_ID))),
    ("checkpoint_lineage_id", root_policy_preimage(lineage=changed(LINEAGE_ID))),
    ("policy_epoch", root_policy_preimage(policy_epoch=POLICY_EPOCH + 1)),
    ("manifest_key_generation", root_policy_preimage(key_generation=KEY_GENERATION + 1)),
    ("writer_authority_epoch", root_policy_preimage(writer_epoch=WRITER_EPOCH + 1)),
    ("durability_policy_length", root_policy_preimage(policy_length=len(POLICY_ID) + 1)),
    ("durability_policy_identifier", root_policy_preimage(policy_id=changed(POLICY_ID))),
    ("manifest_durability_mode", root_policy_preimage(durability_mode=4)),
    ("maximum_source_object_count", root_policy_preimage(max_objects=127)),
    ("maximum_frame_bytes", root_policy_preimage(max_frame=MAX_FRAME - 1)),
    ("maximum_aggregate_frame_bytes", root_policy_preimage(max_aggregate=MAX_AGGREGATE - 1)),
    ("source_set_commitment", root_policy_preimage(source_set=changed(SOURCE_SET_COMMITMENT))),
    ("field_order", root_policy_preimage(material_root=REGISTRY_ROOT, registry_root=MATERIAL_ROOT)),
]


def authority_mutations(tag):
    items = [
        ("domain_missing_nul", authority_source_preimage(domain=DOMAINS["authority_source"][:-1], tag=tag)),
        ("domain_extra_nul", authority_source_preimage(domain=DOMAINS["authority_source"] + b"\0", tag=tag)),
        ("provenance_tag_length", authority_source_preimage(tag=tag, tag_length=len(tag) + 1)),
        ("provenance_tag", authority_source_preimage(
            tag=changed(tag),
            reconciliation_attempt=(RECONCILIATION_ATTEMPT
                                    if tag == TAGS["reconciled_successor"] else None))),
        ("registry_root", authority_source_preimage(tag=tag, registry_root=changed(REGISTRY_ROOT))),
        ("original_attempt", authority_source_preimage(tag=tag, original_attempt=changed(ORIGINAL_ATTEMPT))),
        ("original_operation", authority_source_preimage(tag=tag, original_operation=changed(ORIGINAL_OPERATION))),
        ("successor_length", authority_source_preimage(tag=tag, successor_length=len(SUCCESSOR) + 1)),
        ("successor_envelope", authority_source_preimage(tag=tag, successor=changed(SUCCESSOR))),
        ("anchor_length", authority_source_preimage(tag=tag, anchor_length=len(ANCHOR) + 1)),
        ("proposed_anchor_envelope", authority_source_preimage(tag=tag, anchor=changed(ANCHOR))),
        ("authorization_sequence", authority_source_preimage(tag=tag, sequence=AUTHORIZATION_SEQUENCE + 1)),
        ("command_id", authority_source_preimage(tag=tag, command=changed(COMMAND_ID))),
        ("token_digest", authority_source_preimage(tag=tag, token=changed(TOKEN_DIGEST))),
        ("plan_commitment", authority_source_preimage(tag=tag, plan=changed(PLAN_COMMITMENT))),
        ("authority_snapshot_commitment", authority_source_preimage(tag=tag, snapshot=changed(SNAPSHOT_COMMITMENT))),
        ("selected_manifest_digest", authority_source_preimage(tag=tag, selected=changed(SELECTED_MANIFEST_DIGEST))),
        ("field_order", authority_source_preimage(tag=tag, command=TOKEN_DIGEST, token=COMMAND_ID)),
    ]
    if tag == TAGS["reconciled_successor"]:
        items.extend((
            ("reconciliation_attempt", authority_source_preimage(tag=tag, reconciliation_attempt=changed(RECONCILIATION_ATTEMPT))),
            ("reconciliation_commitment", authority_source_preimage(tag=tag, reconciliation_commitment=changed(RECONCILIATION_COMMITMENT))),
            ("observed_successor_length", authority_source_preimage(tag=tag, observed_length=len(OBSERVED_SUCCESSOR) + 1)),
            ("observed_successor", authority_source_preimage(tag=tag, observed=changed(OBSERVED_SUCCESSOR))),
            ("reconciled_field_order", authority_source_preimage(tag=tag, reconciliation_attempt=RECONCILIATION_COMMITMENT, reconciliation_commitment=RECONCILIATION_ATTEMPT)),
        ))
    return items


material_set_mutations = [
    ("domain_missing_nul", material_set_preimage(domain=DOMAINS["material_set"][:-1])),
    ("domain_extra_nul", material_set_preimage(domain=DOMAINS["material_set"] + b"\0")),
    ("manifest_envelope_length", material_set_preimage(manifest_length=len(MANIFEST) + 1)),
    ("manifest_envelope", material_set_preimage(manifest=changed(MANIFEST))),
    ("object_count", material_set_preimage(count=3)),
    ("descriptor_0_index", material_set_preimage(index_overrides={0: 9})),
    ("descriptor_0_object_id", material_set_preimage(descriptors=((0, changed(DESCRIPTORS[0][1]), DESCRIPTORS[0][2], DESCRIPTORS[0][3]), DESCRIPTORS[1]))),
    ("descriptor_0_type_length", material_set_preimage(type_length_overrides={0: len(DESCRIPTORS[0][2]) + 1})),
    ("descriptor_0_stream_type", material_set_preimage(descriptors=((0, DESCRIPTORS[0][1], changed(DESCRIPTORS[0][2]), DESCRIPTORS[0][3]), DESCRIPTORS[1]))),
    ("descriptor_0_frame_length", material_set_preimage(descriptors=((0, DESCRIPTORS[0][1], DESCRIPTORS[0][2], DESCRIPTORS[0][3] + 1), DESCRIPTORS[1]))),
    ("descriptor_1_index", material_set_preimage(index_overrides={1: 9})),
    ("descriptor_1_object_id", material_set_preimage(descriptors=(DESCRIPTORS[0], (1, changed(DESCRIPTORS[1][1]), DESCRIPTORS[1][2], DESCRIPTORS[1][3])))),
    ("descriptor_1_type_length", material_set_preimage(type_length_overrides={1: len(DESCRIPTORS[1][2]) + 1})),
    ("descriptor_1_stream_type", material_set_preimage(descriptors=(DESCRIPTORS[0], (1, DESCRIPTORS[1][1], changed(DESCRIPTORS[1][2]), DESCRIPTORS[1][3])))),
    ("descriptor_1_frame_length", material_set_preimage(descriptors=(DESCRIPTORS[0], (1, DESCRIPTORS[1][1], DESCRIPTORS[1][2], DESCRIPTORS[1][3] + 1)))),
    ("descriptor_order", material_set_preimage(descriptors=(DESCRIPTORS[1], DESCRIPTORS[0]))),
]

operation_mutations = [
    ("domain_missing_nul", operation_preimage(domain=DOMAINS["operation"][:-1])),
    ("domain_extra_nul", operation_preimage(domain=DOMAINS["operation"] + b"\0")),
    ("material_root", operation_preimage(material_root=changed(MATERIAL_ROOT))),
    ("registry_root", operation_preimage(registry_root=changed(REGISTRY_ROOT))),
    ("material_attempt", operation_preimage(material_attempt=changed(MATERIAL_ATTEMPT))),
    ("root_policy_commitment", operation_preimage(root_policy=changed(ROOT_POLICY_COMMITMENT))),
    ("source_commitment", operation_preimage(source=changed(SOURCE_COMMITMENTS["direct_advanced"]))),
    ("source_set_commitment", operation_preimage(source_set=changed(SOURCE_SET_COMMITMENT))),
    ("material_set_commitment", operation_preimage(material_set=changed(MATERIAL_SET_COMMITMENT))),
    ("selected_manifest_digest", operation_preimage(selected=changed(SELECTED_MANIFEST_DIGEST))),
    ("proposed_anchor_envelope_digest", operation_preimage(anchor_digest=changed(sha(ANCHOR)))),
    ("field_order", operation_preimage(material_root=REGISTRY_ROOT, registry_root=MATERIAL_ROOT)),
]

durable_close_mutations = [
    ("domain_missing_nul", durable_close_preimage(domain=DOMAINS["durable_close"][:-1])),
    ("domain_extra_nul", durable_close_preimage(domain=DOMAINS["durable_close"] + b"\0")),
    ("material_root", durable_close_preimage(material_root=changed(MATERIAL_ROOT))),
    ("registry_root", durable_close_preimage(registry_root=changed(REGISTRY_ROOT))),
    ("material_attempt", durable_close_preimage(material_attempt=changed(MATERIAL_ATTEMPT))),
    ("operation_commitment", durable_close_preimage(operation=changed(OPERATION_COMMITMENTS["direct_advanced"]))),
    ("source_set_commitment", durable_close_preimage(source_set=changed(SOURCE_SET_COMMITMENT))),
    ("material_set_commitment", durable_close_preimage(material_set=changed(MATERIAL_SET_COMMITMENT))),
    ("selected_manifest_digest", durable_close_preimage(selected=changed(SELECTED_MANIFEST_DIGEST))),
    ("terminal_code", durable_close_preimage(close_code=2)),
    ("field_order", durable_close_preimage(material_root=REGISTRY_ROOT, registry_root=MATERIAL_ROOT)),
]

mutation_groups = {
    "source_set": mutation_summary("source_set", source_set_preimage(), source_set_mutations),
    "root_policy": mutation_summary("root_policy", root_policy_preimage(), root_policy_mutations),
    "material_set": mutation_summary("material_set", material_set_preimage(), material_set_mutations),
    "operation": mutation_summary("operation", operation_preimage(), operation_mutations),
    "durable_close": mutation_summary("durable_close", durable_close_preimage(), durable_close_mutations),
}
for case_name, tag in TAGS.items():
    mutation_groups[f"authority_source_{case_name}"] = mutation_summary(
        f"authority_source_{case_name}", authority_source_preimage(tag=tag),
        authority_mutations(tag))

result = {
    "authority_source_commitment_sha256_hex": {name: value.hex() for name, value in SOURCE_COMMITMENTS.items()},
    "domain_ascii_with_nul_hex": {name: value.hex() for name, value in DOMAINS.items()},
    "durable_close_confirmation_sha256_hex": sha(durable_close_preimage()).hex(),
    "fixture": {
        "anchor_envelope_hex": ANCHOR.hex(),
        "authorization_sequence": AUTHORIZATION_SEQUENCE,
        "checkpoint_lineage_id_hex": LINEAGE_ID.hex(),
        "command_id_hex": COMMAND_ID.hex(),
        "descriptors": [
            {"frame_length": frame_length, "index": index,
             "object_id_hex": object_id.hex(), "stream_type_hex": stream_type.hex()}
            for index, object_id, stream_type, frame_length in DESCRIPTORS
        ],
        "durability_policy_identifier_ascii": POLICY_ID.decode("ascii"),
        "manifest_durability_mode": DURABILITY_MODE,
        "manifest_envelope_hex": MANIFEST.hex(),
        "manifest_key_generation": KEY_GENERATION,
        "material_attempt_id_hex": MATERIAL_ATTEMPT.hex(),
        "material_root_identity_hex": MATERIAL_ROOT.hex(),
        "maximum_aggregate_frame_bytes": MAX_AGGREGATE,
        "maximum_frame_bytes": MAX_FRAME,
        "maximum_source_object_count": MAX_OBJECTS,
        "namespace_id_hex": NAMESPACE_ID.hex(),
        "observed_successor_hex": OBSERVED_SUCCESSOR.hex(),
        "original_attempt_id_hex": ORIGINAL_ATTEMPT.hex(),
        "original_operation_commitment_hex": ORIGINAL_OPERATION.hex(),
        "plan_commitment_hex": PLAN_COMMITMENT.hex(),
        "policy_epoch": POLICY_EPOCH,
        "reconciliation_attempt_id_hex": RECONCILIATION_ATTEMPT.hex(),
        "reconciliation_commitment_hex": RECONCILIATION_COMMITMENT.hex(),
        "registry_root_identity_hex": REGISTRY_ROOT.hex(),
        "selected_manifest_digest_hex": SELECTED_MANIFEST_DIGEST.hex(),
        "source_frames": [
            {"frame_hex": frame.hex(), "index": index} for index, frame in SOURCES
        ],
        "store_uuid_hex": STORE_UUID.hex(),
        "successor_envelope_hex": SUCCESSOR.hex(),
        "token_digest_hex": TOKEN_DIGEST.hex(),
        "authority_snapshot_commitment_hex": SNAPSHOT_COMMITMENT.hex(),
        "writer_authority_epoch": WRITER_EPOCH,
    },
    "material_set_commitment_sha256_hex": MATERIAL_SET_COMMITMENT.hex(),
    "mutation_checks_total": sum(group["count"] for group in mutation_groups.values()),
    "mutation_groups": mutation_groups,
    "operation_commitment_sha256_hex": {name: value.hex() for name, value in OPERATION_COMMITMENTS.items()},
    "preimage_bytes": {
        "authority_source": {name: len(authority_source_preimage(tag=tag)) for name, tag in TAGS.items()},
        "durable_close": len(durable_close_preimage()),
        "material_set": len(material_set_preimage()),
        "operation": len(operation_preimage()),
        "root_policy": len(root_policy_preimage()),
        "source_set": len(source_set_preimage()),
    },
    "provenance_tag_ascii": {name: value.decode("ascii") for name, value in TAGS.items()},
    "root_policy_commitment_sha256_hex": ROOT_POLICY_COMMITMENT.hex(),
    "source_set_commitment_sha256_hex": SOURCE_SET_COMMITMENT.hex(),
}

if len(sys.argv) == 3 and sys.argv[1] == "--check":
    expected = json.loads(pathlib.Path(sys.argv[2]).read_text(encoding="utf-8"))
    if expected != result:
        differing = sorted(set(expected) | set(result))
        differing = [key for key in differing if expected.get(key) != result.get(key)]
        raise SystemExit("golden mismatch: " + ", ".join(differing))
elif len(sys.argv) == 1:
    print(json.dumps(result, indent=2, sort_keys=True))
else:
    raise SystemExit(f"usage: {pathlib.Path(sys.argv[0]).name} [--check VECTOR.json]")
