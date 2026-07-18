"""Independent stdlib-only golden serializer for accepted ADR-0017 commitments."""

import hashlib
import json
import pathlib
import sys


DOMAINS = {
    "root_policy": b"halofpx.bootstrap-anchor-root-policy.v1\0",
    "material_source": b"halofpx.bootstrap-anchor-material-source.v1\0",
    "create": b"halofpx.bootstrap-anchor-create.v1\0",
    "anchor_witness": b"halofpx.protected-anchor-head-witness.v1\0",
    "durable_close": b"halofpx.bootstrap-anchor-durable-close.v1\0",
    "reconciliation": b"halofpx.bootstrap-anchor-reconciliation.v1\0",
    "reconciliation_fence": b"halofpx.bootstrap-anchor-reconciliation-fence.v1\0",
    "reconciliation_close": b"halofpx.bootstrap-anchor-reconciliation-durable-close.v1\0",
}
OUTCOME_TAGS = {
    "prepared": b"material-prepared-v1",
    "already_same": b"material-already-same-v1",
}
PHASES = {"pre_create": 1, "create_linearized": 2, "unknown": 3}
CLASSIFICATIONS = {"exact_proposed": 1, "absent": 2, "other_present": 3}


def u64be(value):
    if not 0 <= value <= 0xFFFFFFFFFFFFFFFF:
        raise ValueError("outside uint64")
    return value.to_bytes(8, "big")


def changed(value):
    if not value:
        return b"\x01"
    out = bytearray(value)
    out[0] ^= 1
    return bytes(out)


def sha(value):
    return hashlib.sha256(value).digest()


ANCHOR_ROOT = bytes.fromhex("80" * 32)
MATERIAL_ROOT = bytes.fromhex("10" * 32)
REGISTRY_ROOT = bytes.fromhex("20" * 32)
STORE_UUID = bytes.fromhex("30" * 16)
NAMESPACE_ID = bytes.fromhex("40" * 32)
LINEAGE_ID = bytes.fromhex("50" * 32)
POLICY_EPOCH = 0x0102030405060708
MANIFEST_KEY_GENERATION = 0x1112131415161718
WRITER_AUTHORITY_EPOCH = 0x2122232425262728
ANCHOR_KEY_ID = b"halofpx-test-anchor-key-v1"
ANCHOR_KEY_GENERATION = 0x4142434445464748
DURABILITY_POLICY_ID = b"anchor-sync-namespace-sync-v1"
MAX_ANCHOR_ENVELOPE = 1024

MATERIAL_ATTEMPT = bytes.fromhex("70" * 32)
MATERIAL_ROOT_POLICY = bytes.fromhex("de2f4e7344e2f639f360222a5e722b1d21a87e1e70a5da40d1145275bfd5208b")
AUTHORITY_SOURCE = bytes.fromhex("095eadb65a1a872b3a164808801025ea7115b154d629a8b0304366e2db01021e")
SOURCE_SET = bytes.fromhex("14274b7838119b668ea651083758ba0f5708b9e8c2480cb046e92a80c801c3c2")
MATERIAL_SET = bytes.fromhex("4bd6dd76f2c7ece17621b61f60d693f4370b47e7e83d46e41e797055bae1928e")
MATERIAL_OPERATION = bytes.fromhex("2a592a981520be6f2688451e70e475f52409815cc6d2965079469ac1438c9a78")
SELECTED_MANIFEST_DIGEST = bytes.fromhex("b08ece24efc5b25e7ac53b8c41d9981b40e7d9113a2ac60f75da9b2a675e713f")
MANIFEST = bytes.fromhex("a30001016b6d616e69666573742d763102820102")
FRAMES = (
    (0, b"HALOFPX-OBJECT-FRAME-ZERO"),
    (1, b"HALOFPX-OBJECT-FRAME-ONE-WITH-DIFFERENT-LENGTH"),
)
PROPOSED_ANCHOR = b"exact-authenticated-proposed-anchor-envelope\xffwith-bytes"
MATERIAL_CLOSE = bytes.fromhex("f65ea35143253641988c9d9ce77ce20acdb07e32446c0fddd146c12a35923171")
ANCHOR_ATTEMPT = bytes.fromhex("90" * 32)
RECONCILIATION_ATTEMPT = bytes.fromhex("91" * 32)
OTHER_ANCHOR = b"different-authoritative-anchor-envelope\x00with-bytes"
OBSERVED = {
    "exact_proposed": PROPOSED_ANCHOR,
    "absent": b"",
    "other_present": OTHER_ANCHOR,
}


def root_policy_preimage(*, domain=DOMAINS["root_policy"], anchor_root=ANCHOR_ROOT,
                         material_root=MATERIAL_ROOT, registry_root=REGISTRY_ROOT,
                         store_uuid=STORE_UUID, namespace=NAMESPACE_ID,
                         lineage=LINEAGE_ID, policy_epoch=POLICY_EPOCH,
                         manifest_key_generation=MANIFEST_KEY_GENERATION,
                         writer_epoch=WRITER_AUTHORITY_EPOCH,
                         anchor_key_id=ANCHOR_KEY_ID, anchor_key_length=None,
                         anchor_key_generation=ANCHOR_KEY_GENERATION,
                         durability_id=DURABILITY_POLICY_ID,
                         durability_length=None, maximum=MAX_ANCHOR_ENVELOPE):
    anchor_key_length = len(anchor_key_id) if anchor_key_length is None else anchor_key_length
    durability_length = len(durability_id) if durability_length is None else durability_length
    return b"".join((domain, anchor_root, material_root, registry_root, store_uuid,
                     namespace, lineage, u64be(policy_epoch),
                     u64be(manifest_key_generation), u64be(writer_epoch),
                     u64be(anchor_key_length), anchor_key_id,
                     u64be(anchor_key_generation), u64be(durability_length),
                     durability_id, u64be(maximum)))


ROOT_POLICY = sha(root_policy_preimage())


def material_source_preimage(*, domain=DOMAINS["material_source"],
                             tag=OUTCOME_TAGS["prepared"], tag_length=None,
                             material_root=MATERIAL_ROOT, registry_root=REGISTRY_ROOT,
                             material_attempt=MATERIAL_ATTEMPT,
                             material_root_policy=MATERIAL_ROOT_POLICY,
                             authority_source=AUTHORITY_SOURCE, source_set=SOURCE_SET,
                             material_set=MATERIAL_SET,
                             material_operation=MATERIAL_OPERATION,
                             selected=SELECTED_MANIFEST_DIGEST, manifest=MANIFEST,
                             manifest_length=None, frames=FRAMES, count=None,
                             index_overrides=None, length_overrides=None,
                             proposed=PROPOSED_ANCHOR, proposed_length=None,
                             material_close=MATERIAL_CLOSE):
    tag_length = len(tag) if tag_length is None else tag_length
    manifest_length = len(manifest) if manifest_length is None else manifest_length
    count = len(frames) if count is None else count
    proposed_length = len(proposed) if proposed_length is None else proposed_length
    index_overrides = index_overrides or {}
    length_overrides = length_overrides or {}
    parts = [domain, u64be(tag_length), tag, material_root, registry_root,
             material_attempt, material_root_policy, authority_source, source_set,
             material_set, material_operation, selected, u64be(manifest_length),
             manifest, u64be(count)]
    for position, (index, frame) in enumerate(frames):
        parts.extend((u64be(index_overrides.get(position, index)),
                      u64be(length_overrides.get(position, len(frame))), frame))
    parts.extend((u64be(proposed_length), proposed, material_close))
    return b"".join(parts)


MATERIAL_SOURCES = {
    name: sha(material_source_preimage(tag=tag)) for name, tag in OUTCOME_TAGS.items()
}


def create_preimage(*, domain=DOMAINS["create"], anchor_root=ANCHOR_ROOT,
                    material_root=MATERIAL_ROOT, registry_root=REGISTRY_ROOT,
                    attempt=ANCHOR_ATTEMPT, root_policy=ROOT_POLICY,
                    material_source=MATERIAL_SOURCES["prepared"],
                    selected=SELECTED_MANIFEST_DIGEST, proposed=PROPOSED_ANCHOR,
                    proposed_length=None):
    proposed_length = len(proposed) if proposed_length is None else proposed_length
    return b"".join((domain, anchor_root, material_root, registry_root, attempt,
                     root_policy, material_source, selected, u64be(proposed_length),
                     proposed))


CREATE_OPERATIONS = {
    name: sha(create_preimage(material_source=source))
    for name, source in MATERIAL_SOURCES.items()
}


def witness_preimage(*, domain=DOMAINS["anchor_witness"], anchor=PROPOSED_ANCHOR,
                     anchor_length=None):
    anchor_length = len(anchor) if anchor_length is None else anchor_length
    return b"".join((domain, u64be(anchor_length), anchor))


WITNESSES = {name: sha(witness_preimage(anchor=value)) for name, value in OBSERVED.items()}
PROPOSED_DIGEST = sha(PROPOSED_ANCHOR)


def durable_close_preimage(*, domain=DOMAINS["durable_close"],
                           anchor_root=ANCHOR_ROOT, material_root=MATERIAL_ROOT,
                           registry_root=REGISTRY_ROOT, attempt=ANCHOR_ATTEMPT,
                           operation=CREATE_OPERATIONS["prepared"],
                           material_source=MATERIAL_SOURCES["prepared"],
                           proposed_digest=PROPOSED_DIGEST,
                           witness=WITNESSES["exact_proposed"], terminal=1):
    return b"".join((domain, anchor_root, material_root, registry_root, attempt,
                     operation, material_source, proposed_digest, witness,
                     bytes((terminal,))))


DIRECT_CLOSES = {
    name: sha(durable_close_preimage(operation=CREATE_OPERATIONS[name],
                                    material_source=MATERIAL_SOURCES[name]))
    for name in OUTCOME_TAGS
}


def reconciliation_preimage(*, domain=DOMAINS["reconciliation"],
                            anchor_root=ANCHOR_ROOT, material_root=MATERIAL_ROOT,
                            registry_root=REGISTRY_ROOT,
                            reconciliation_attempt=RECONCILIATION_ATTEMPT,
                            original_attempt=ANCHOR_ATTEMPT,
                            original_operation=CREATE_OPERATIONS["prepared"],
                            phase=PHASES["create_linearized"],
                            root_policy=ROOT_POLICY,
                            material_source=MATERIAL_SOURCES["prepared"],
                            proposed=PROPOSED_ANCHOR, proposed_length=None):
    proposed_length = len(proposed) if proposed_length is None else proposed_length
    return b"".join((domain, anchor_root, material_root, registry_root,
                     reconciliation_attempt, original_attempt, original_operation,
                     bytes((phase,)), root_policy, material_source,
                     u64be(proposed_length), proposed))


RECONCILIATIONS = {
    phase_name: sha(reconciliation_preimage(phase=phase))
    for phase_name, phase in PHASES.items()
}


def fence_preimage(*, domain=DOMAINS["reconciliation_fence"],
                   anchor_root=ANCHOR_ROOT,
                   reconciliation_attempt=RECONCILIATION_ATTEMPT,
                   reconciliation=RECONCILIATIONS["create_linearized"],
                   original_attempt=ANCHOR_ATTEMPT,
                   original_operation=CREATE_OPERATIONS["prepared"],
                   phase=PHASES["create_linearized"],
                   witness=WITNESSES["exact_proposed"],
                   classification=CLASSIFICATIONS["exact_proposed"], terminal=1):
    return b"".join((domain, anchor_root, reconciliation_attempt, reconciliation,
                     original_attempt, original_operation, bytes((phase,)), witness,
                     bytes((classification,)), bytes((terminal,))))


FENCES = {}
for phase_name, phase in PHASES.items():
    for classification_name, classification in CLASSIFICATIONS.items():
        key = f"{phase_name}.{classification_name}"
        FENCES[key] = sha(fence_preimage(
            reconciliation=RECONCILIATIONS[phase_name], phase=phase,
            witness=WITNESSES[classification_name], classification=classification))


def reconciliation_close_preimage(*, domain=DOMAINS["reconciliation_close"],
                                  anchor_root=ANCHOR_ROOT,
                                  reconciliation_attempt=RECONCILIATION_ATTEMPT,
                                  reconciliation=RECONCILIATIONS["create_linearized"],
                                  original_attempt=ANCHOR_ATTEMPT,
                                  original_operation=CREATE_OPERATIONS["prepared"],
                                  phase=PHASES["create_linearized"],
                                  material_source=MATERIAL_SOURCES["prepared"],
                                  proposed_digest=PROPOSED_DIGEST,
                                  witness=WITNESSES["exact_proposed"],
                                  fence=FENCES["create_linearized.exact_proposed"],
                                  terminal=1):
    return b"".join((domain, anchor_root, reconciliation_attempt, reconciliation,
                     original_attempt, original_operation, bytes((phase,)),
                     material_source, proposed_digest, witness, fence,
                     bytes((terminal,))))


RECONCILIATION_CLOSES = {}
for phase_name, phase in PHASES.items():
    for classification_name in CLASSIFICATIONS:
        key = f"{phase_name}.{classification_name}"
        RECONCILIATION_CLOSES[key] = sha(reconciliation_close_preimage(
            reconciliation=RECONCILIATIONS[phase_name], phase=phase,
            witness=WITNESSES[classification_name], fence=FENCES[key]))


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
    return {"count": len(digests),
            "names": sorted(mutation_name for mutation_name, _ in digests),
            "sha256_hex": aggregate.hexdigest()}


for name, domain in DOMAINS.items():
    if not domain.endswith(b"\0") or domain.count(b"\0") != 1:
        raise SystemExit(f"{name} domain must contain exactly one trailing NUL")
for name, tag in OUTCOME_TAGS.items():
    if b"\0" in tag:
        raise SystemExit(f"{name} outcome tag must not contain a NUL")

root_policy_mutations = [
    ("domain_missing_nul", root_policy_preimage(domain=DOMAINS["root_policy"][:-1])),
    ("domain_extra_nul", root_policy_preimage(domain=DOMAINS["root_policy"] + b"\0")),
    ("anchor_root", root_policy_preimage(anchor_root=changed(ANCHOR_ROOT))),
    ("material_root", root_policy_preimage(material_root=changed(MATERIAL_ROOT))),
    ("registry_root", root_policy_preimage(registry_root=changed(REGISTRY_ROOT))),
    ("store_uuid", root_policy_preimage(store_uuid=changed(STORE_UUID))),
    ("namespace_id", root_policy_preimage(namespace=changed(NAMESPACE_ID))),
    ("lineage_id", root_policy_preimage(lineage=changed(LINEAGE_ID))),
    ("policy_epoch", root_policy_preimage(policy_epoch=POLICY_EPOCH + 1)),
    ("manifest_key_generation", root_policy_preimage(manifest_key_generation=MANIFEST_KEY_GENERATION + 1)),
    ("writer_authority_epoch", root_policy_preimage(writer_epoch=WRITER_AUTHORITY_EPOCH + 1)),
    ("anchor_key_id_length", root_policy_preimage(anchor_key_length=len(ANCHOR_KEY_ID) + 1)),
    ("anchor_key_id", root_policy_preimage(anchor_key_id=changed(ANCHOR_KEY_ID))),
    ("anchor_key_generation", root_policy_preimage(anchor_key_generation=ANCHOR_KEY_GENERATION + 1)),
    ("durability_policy_length", root_policy_preimage(durability_length=len(DURABILITY_POLICY_ID) + 1)),
    ("durability_policy_id", root_policy_preimage(durability_id=changed(DURABILITY_POLICY_ID))),
    ("maximum_anchor_envelope", root_policy_preimage(maximum=MAX_ANCHOR_ENVELOPE - 1)),
    ("field_order", root_policy_preimage(anchor_root=MATERIAL_ROOT, material_root=ANCHOR_ROOT)),
]


def material_source_mutations(tag):
    return [
        ("domain_missing_nul", material_source_preimage(domain=DOMAINS["material_source"][:-1], tag=tag)),
        ("domain_extra_nul", material_source_preimage(domain=DOMAINS["material_source"] + b"\0", tag=tag)),
        ("outcome_tag_length", material_source_preimage(tag=tag, tag_length=len(tag) + 1)),
        ("outcome_tag", material_source_preimage(tag=changed(tag))),
        ("material_root", material_source_preimage(tag=tag, material_root=changed(MATERIAL_ROOT))),
        ("registry_root", material_source_preimage(tag=tag, registry_root=changed(REGISTRY_ROOT))),
        ("material_attempt", material_source_preimage(tag=tag, material_attempt=changed(MATERIAL_ATTEMPT))),
        ("material_root_policy", material_source_preimage(tag=tag, material_root_policy=changed(MATERIAL_ROOT_POLICY))),
        ("authority_source", material_source_preimage(tag=tag, authority_source=changed(AUTHORITY_SOURCE))),
        ("source_set", material_source_preimage(tag=tag, source_set=changed(SOURCE_SET))),
        ("material_set", material_source_preimage(tag=tag, material_set=changed(MATERIAL_SET))),
        ("material_operation", material_source_preimage(tag=tag, material_operation=changed(MATERIAL_OPERATION))),
        ("selected_manifest", material_source_preimage(tag=tag, selected=changed(SELECTED_MANIFEST_DIGEST))),
        ("manifest_length", material_source_preimage(tag=tag, manifest_length=len(MANIFEST) + 1)),
        ("manifest_bytes", material_source_preimage(tag=tag, manifest=changed(MANIFEST))),
        ("frame_count", material_source_preimage(tag=tag, count=3)),
        ("frame_0_index", material_source_preimage(tag=tag, index_overrides={0: 9})),
        ("frame_0_length", material_source_preimage(tag=tag, length_overrides={0: len(FRAMES[0][1]) + 1})),
        ("frame_0_bytes", material_source_preimage(tag=tag, frames=((0, changed(FRAMES[0][1])), FRAMES[1]))),
        ("frame_1_index", material_source_preimage(tag=tag, index_overrides={1: 9})),
        ("frame_1_length", material_source_preimage(tag=tag, length_overrides={1: len(FRAMES[1][1]) + 1})),
        ("frame_1_bytes", material_source_preimage(tag=tag, frames=(FRAMES[0], (1, changed(FRAMES[1][1]))))),
        ("frame_order", material_source_preimage(tag=tag, frames=(FRAMES[1], FRAMES[0]))),
        ("proposed_length", material_source_preimage(tag=tag, proposed_length=len(PROPOSED_ANCHOR) + 1)),
        ("proposed_bytes", material_source_preimage(tag=tag, proposed=changed(PROPOSED_ANCHOR))),
        ("material_close", material_source_preimage(tag=tag, material_close=changed(MATERIAL_CLOSE))),
        ("field_order", material_source_preimage(tag=tag, material_root=REGISTRY_ROOT, registry_root=MATERIAL_ROOT)),
    ]


create_mutations = [
    ("domain_missing_nul", create_preimage(domain=DOMAINS["create"][:-1])),
    ("domain_extra_nul", create_preimage(domain=DOMAINS["create"] + b"\0")),
    ("anchor_root", create_preimage(anchor_root=changed(ANCHOR_ROOT))),
    ("material_root", create_preimage(material_root=changed(MATERIAL_ROOT))),
    ("registry_root", create_preimage(registry_root=changed(REGISTRY_ROOT))),
    ("anchor_attempt", create_preimage(attempt=changed(ANCHOR_ATTEMPT))),
    ("root_policy", create_preimage(root_policy=changed(ROOT_POLICY))),
    ("material_source", create_preimage(material_source=changed(MATERIAL_SOURCES["prepared"]))),
    ("selected_manifest", create_preimage(selected=changed(SELECTED_MANIFEST_DIGEST))),
    ("proposed_length", create_preimage(proposed_length=len(PROPOSED_ANCHOR) + 1)),
    ("proposed_bytes", create_preimage(proposed=changed(PROPOSED_ANCHOR))),
    ("field_order", create_preimage(anchor_root=MATERIAL_ROOT, material_root=ANCHOR_ROOT)),
]

witness_mutations = [
    ("domain_missing_nul", witness_preimage(domain=DOMAINS["anchor_witness"][:-1])),
    ("domain_extra_nul", witness_preimage(domain=DOMAINS["anchor_witness"] + b"\0")),
    ("anchor_length", witness_preimage(anchor_length=len(PROPOSED_ANCHOR) + 1)),
    ("anchor_bytes", witness_preimage(anchor=changed(PROPOSED_ANCHOR))),
]

durable_close_mutations = [
    ("domain_missing_nul", durable_close_preimage(domain=DOMAINS["durable_close"][:-1])),
    ("domain_extra_nul", durable_close_preimage(domain=DOMAINS["durable_close"] + b"\0")),
    ("anchor_root", durable_close_preimage(anchor_root=changed(ANCHOR_ROOT))),
    ("material_root", durable_close_preimage(material_root=changed(MATERIAL_ROOT))),
    ("registry_root", durable_close_preimage(registry_root=changed(REGISTRY_ROOT))),
    ("anchor_attempt", durable_close_preimage(attempt=changed(ANCHOR_ATTEMPT))),
    ("create_operation", durable_close_preimage(operation=changed(CREATE_OPERATIONS["prepared"]))),
    ("material_source", durable_close_preimage(material_source=changed(MATERIAL_SOURCES["prepared"]))),
    ("proposed_digest", durable_close_preimage(proposed_digest=changed(PROPOSED_DIGEST))),
    ("anchor_witness", durable_close_preimage(witness=changed(WITNESSES["exact_proposed"]))),
    ("terminal_code_zero", durable_close_preimage(terminal=0)),
    ("terminal_code_two", durable_close_preimage(terminal=2)),
    ("field_order", durable_close_preimage(anchor_root=MATERIAL_ROOT, material_root=ANCHOR_ROOT)),
]

reconciliation_mutations = [
    ("domain_missing_nul", reconciliation_preimage(domain=DOMAINS["reconciliation"][:-1])),
    ("domain_extra_nul", reconciliation_preimage(domain=DOMAINS["reconciliation"] + b"\0")),
    ("anchor_root", reconciliation_preimage(anchor_root=changed(ANCHOR_ROOT))),
    ("material_root", reconciliation_preimage(material_root=changed(MATERIAL_ROOT))),
    ("registry_root", reconciliation_preimage(registry_root=changed(REGISTRY_ROOT))),
    ("reconciliation_attempt", reconciliation_preimage(reconciliation_attempt=changed(RECONCILIATION_ATTEMPT))),
    ("original_attempt", reconciliation_preimage(original_attempt=changed(ANCHOR_ATTEMPT))),
    ("original_operation", reconciliation_preimage(original_operation=changed(CREATE_OPERATIONS["prepared"]))),
    ("original_phase_pre_create", reconciliation_preimage(phase=PHASES["pre_create"])),
    ("original_phase_unknown", reconciliation_preimage(phase=PHASES["unknown"])),
    ("root_policy", reconciliation_preimage(root_policy=changed(ROOT_POLICY))),
    ("material_source", reconciliation_preimage(material_source=changed(MATERIAL_SOURCES["prepared"]))),
    ("proposed_length", reconciliation_preimage(proposed_length=len(PROPOSED_ANCHOR) + 1)),
    ("proposed_bytes", reconciliation_preimage(proposed=changed(PROPOSED_ANCHOR))),
    ("field_order", reconciliation_preimage(anchor_root=MATERIAL_ROOT, material_root=ANCHOR_ROOT)),
]

fence_mutations = [
    ("domain_missing_nul", fence_preimage(domain=DOMAINS["reconciliation_fence"][:-1])),
    ("domain_extra_nul", fence_preimage(domain=DOMAINS["reconciliation_fence"] + b"\0")),
    ("anchor_root", fence_preimage(anchor_root=changed(ANCHOR_ROOT))),
    ("reconciliation_attempt", fence_preimage(reconciliation_attempt=changed(RECONCILIATION_ATTEMPT))),
    ("reconciliation_commitment", fence_preimage(reconciliation=changed(RECONCILIATIONS["create_linearized"]))),
    ("original_attempt", fence_preimage(original_attempt=changed(ANCHOR_ATTEMPT))),
    ("original_operation", fence_preimage(original_operation=changed(CREATE_OPERATIONS["prepared"]))),
    ("original_phase_pre_create", fence_preimage(phase=PHASES["pre_create"])),
    ("original_phase_unknown", fence_preimage(phase=PHASES["unknown"])),
    ("anchor_witness", fence_preimage(witness=changed(WITNESSES["exact_proposed"]))),
    ("classification_absent", fence_preimage(classification=CLASSIFICATIONS["absent"])),
    ("classification_other_present", fence_preimage(classification=CLASSIFICATIONS["other_present"])),
    ("terminal_code_zero", fence_preimage(terminal=0)),
    ("terminal_code_two", fence_preimage(terminal=2)),
]

reconciliation_close_mutations = [
    ("domain_missing_nul", reconciliation_close_preimage(domain=DOMAINS["reconciliation_close"][:-1])),
    ("domain_extra_nul", reconciliation_close_preimage(domain=DOMAINS["reconciliation_close"] + b"\0")),
    ("anchor_root", reconciliation_close_preimage(anchor_root=changed(ANCHOR_ROOT))),
    ("reconciliation_attempt", reconciliation_close_preimage(reconciliation_attempt=changed(RECONCILIATION_ATTEMPT))),
    ("reconciliation_commitment", reconciliation_close_preimage(reconciliation=changed(RECONCILIATIONS["create_linearized"]))),
    ("original_attempt", reconciliation_close_preimage(original_attempt=changed(ANCHOR_ATTEMPT))),
    ("original_operation", reconciliation_close_preimage(original_operation=changed(CREATE_OPERATIONS["prepared"]))),
    ("original_phase_pre_create", reconciliation_close_preimage(phase=PHASES["pre_create"])),
    ("original_phase_unknown", reconciliation_close_preimage(phase=PHASES["unknown"])),
    ("material_source", reconciliation_close_preimage(material_source=changed(MATERIAL_SOURCES["prepared"]))),
    ("proposed_digest", reconciliation_close_preimage(proposed_digest=changed(PROPOSED_DIGEST))),
    ("anchor_witness", reconciliation_close_preimage(witness=changed(WITNESSES["exact_proposed"]))),
    ("fence_confirmation", reconciliation_close_preimage(fence=changed(FENCES["create_linearized.exact_proposed"]))),
    ("terminal_code_zero", reconciliation_close_preimage(terminal=0)),
    ("terminal_code_two", reconciliation_close_preimage(terminal=2)),
]

mutation_groups = {
    "root_policy": mutation_summary("root_policy", root_policy_preimage(), root_policy_mutations),
    "create": mutation_summary("create", create_preimage(), create_mutations),
    "anchor_witness": mutation_summary("anchor_witness", witness_preimage(), witness_mutations),
    "durable_close": mutation_summary("durable_close", durable_close_preimage(), durable_close_mutations),
    "reconciliation": mutation_summary("reconciliation", reconciliation_preimage(), reconciliation_mutations),
    "reconciliation_fence": mutation_summary("reconciliation_fence", fence_preimage(), fence_mutations),
    "reconciliation_close": mutation_summary("reconciliation_close", reconciliation_close_preimage(), reconciliation_close_mutations),
}
for name, tag in OUTCOME_TAGS.items():
    mutation_groups[f"material_source_{name}"] = mutation_summary(
        f"material_source_{name}", material_source_preimage(tag=tag),
        material_source_mutations(tag))

result = {
    "anchor_root_policy_commitment_sha256_hex": ROOT_POLICY.hex(),
    "anchor_witness_digest_sha256_hex": {name: value.hex() for name, value in WITNESSES.items()},
    "classification_codes": CLASSIFICATIONS,
    "create_close_confirmation_sha256_hex": {name: value.hex() for name, value in DIRECT_CLOSES.items()},
    "create_operation_commitment_sha256_hex": {name: value.hex() for name, value in CREATE_OPERATIONS.items()},
    "domain_ascii_with_nul_hex": {name: value.hex() for name, value in DOMAINS.items()},
    "fixture": {
        "anchor_attempt_id_hex": ANCHOR_ATTEMPT.hex(),
        "anchor_authentication_key_generation": ANCHOR_KEY_GENERATION,
        "anchor_authentication_key_id_ascii": ANCHOR_KEY_ID.decode("ascii"),
        "anchor_durability_policy_identifier_ascii": DURABILITY_POLICY_ID.decode("ascii"),
        "anchor_root_identity_hex": ANCHOR_ROOT.hex(),
        "checkpoint_lineage_id_hex": LINEAGE_ID.hex(),
        "manifest_envelope_hex": MANIFEST.hex(),
        "manifest_key_generation": MANIFEST_KEY_GENERATION,
        "material_attempt_id_hex": MATERIAL_ATTEMPT.hex(),
        "material_close_confirmation_hex": MATERIAL_CLOSE.hex(),
        "material_operation_commitment_hex": MATERIAL_OPERATION.hex(),
        "material_root_identity_hex": MATERIAL_ROOT.hex(),
        "material_root_policy_commitment_hex": MATERIAL_ROOT_POLICY.hex(),
        "maximum_anchor_envelope_bytes": MAX_ANCHOR_ENVELOPE,
        "namespace_id_hex": NAMESPACE_ID.hex(),
        "observed_anchor_hex": {name: value.hex() for name, value in OBSERVED.items()},
        "policy_epoch": POLICY_EPOCH,
        "proposed_anchor_digest_hex": PROPOSED_DIGEST.hex(),
        "proposed_anchor_envelope_hex": PROPOSED_ANCHOR.hex(),
        "reconciliation_attempt_id_hex": RECONCILIATION_ATTEMPT.hex(),
        "registry_root_identity_hex": REGISTRY_ROOT.hex(),
        "selected_manifest_digest_hex": SELECTED_MANIFEST_DIGEST.hex(),
        "source_frames": [{"frame_hex": frame.hex(), "index": index} for index, frame in FRAMES],
        "source_set_commitment_hex": SOURCE_SET.hex(),
        "material_set_commitment_hex": MATERIAL_SET.hex(),
        "authority_source_commitment_hex": AUTHORITY_SOURCE.hex(),
        "store_uuid_hex": STORE_UUID.hex(),
        "writer_authority_epoch": WRITER_AUTHORITY_EPOCH,
    },
    "material_source_commitment_sha256_hex": {name: value.hex() for name, value in MATERIAL_SOURCES.items()},
    "mutation_checks_total": sum(group["count"] for group in mutation_groups.values()),
    "mutation_groups": mutation_groups,
    "outcome_tag_ascii": {name: value.decode("ascii") for name, value in OUTCOME_TAGS.items()},
    "phase_codes": PHASES,
    "preimage_bytes": {
        "anchor_witness": {name: len(witness_preimage(anchor=value)) for name, value in OBSERVED.items()},
        "create": len(create_preimage()),
        "durable_close": len(durable_close_preimage()),
        "material_source": {name: len(material_source_preimage(tag=tag)) for name, tag in OUTCOME_TAGS.items()},
        "reconciliation": len(reconciliation_preimage()),
        "reconciliation_close": len(reconciliation_close_preimage()),
        "reconciliation_fence": len(fence_preimage()),
        "root_policy": len(root_policy_preimage()),
    },
    "reconciliation_close_confirmation_sha256_hex": {name: value.hex() for name, value in RECONCILIATION_CLOSES.items()},
    "reconciliation_commitment_sha256_hex": {name: value.hex() for name, value in RECONCILIATIONS.items()},
    "reconciliation_fence_confirmation_sha256_hex": {name: value.hex() for name, value in FENCES.items()},
    "terminal_codes": {
        "create_durable_close": 1,
        "reconciliation_fence": 1,
        "reconciliation_durable_close": 1,
    },
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
