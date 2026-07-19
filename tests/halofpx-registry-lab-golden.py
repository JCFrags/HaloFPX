"""Independent stdlib-only L05o registry-lab wire oracle and mutation checks."""

import hashlib
import hmac
import json
import pathlib
import sys


def head(major, value):
    if not 0 <= value <= 0xFFFFFFFFFFFFFFFF:
        raise ValueError("outside uint64 profile")
    if value < 24:
        return bytes([(major << 5) | value])
    for maximum, ai, width in ((0xFF, 24, 1), (0xFFFF, 25, 2),
                               (0xFFFFFFFF, 26, 4),
                               (0xFFFFFFFFFFFFFFFF, 27, 8)):
        if value <= maximum:
            return bytes([(major << 5) | ai]) + value.to_bytes(width, "big")
    raise AssertionError


def uint(value):
    return head(0, value)


def bstr(value):
    return head(2, len(value)) + value


def tstr(value):
    raw = value.encode("ascii")
    return head(3, len(raw)) + raw


def mapping(entries):
    prior = -1
    out = bytearray(head(5, len(entries)))
    for key, value in entries:
        if key <= prior:
            raise ValueError("map keys must be strictly increasing")
        prior = key
        out += uint(key) + value
    return bytes(out)


NULL = b"\xf6"


def rep(value, count=32):
    return bytes([value]) * count


KEY_DOMAIN = b"halofpx.registry-lab-key.v1\0"
REGISTRY_DOMAIN = b"halofpx.registry-lab-registry-envelope.v1\0"
OPERATION_DOMAIN = b"halofpx.registry-lab-operation.v1\0"
PATH_POLICY_DOMAIN = b"halofpx.registry-lab-path-policy.v1\0"
INNER_DOMAINS = [
    b"halofpx.registry-snapshot-key.v1\0",
    b"halofpx.registry-snapshot-auth.v1\0",
    b"halofpx.registry-snapshot.v1\0",
    b"halofpx.registry-successor-key.v1\0",
    b"halofpx.registry-successor-auth.v1\0",
    b"halofpx.registry-successor.v1\0",
    b"halofpx.registry-key-continuity.v1\0",
]
DOMAINS = {
    "root": (b"halofpx.registry-lab-root-auth.v1\0", b"halofpx.registry-lab-root.v1\0"),
    "head": (b"halofpx.registry-lab-head-auth.v1\0", b"halofpx.registry-lab-head.v1\0"),
    "prepare": (b"halofpx.registry-lab-prepare-auth.v1\0", b"halofpx.registry-lab-prepare.v1\0"),
    "close": (b"halofpx.registry-lab-close-auth.v1\0", b"halofpx.registry-lab-close.v1\0"),
    "abort": (b"halofpx.registry-lab-abort-auth.v1\0", b"halofpx.registry-lab-abort.v1\0"),
    "quarantine": (b"halofpx.registry-lab-quarantine-auth.v1\0", b"halofpx.registry-lab-quarantine.v1\0"),
}

KEY_ID = "registry-auth-v1"
KEY_GENERATION = 13
MASTER_KEY = rep(0x44)
CREDENTIAL_PACKAGE = (
    b"HaloFPXRegKey01\0"
    + len(KEY_ID).to_bytes(2, "big") + KEY_ID.encode("ascii")
    + KEY_GENERATION.to_bytes(8, "big")
    + (32).to_bytes(2, "big") + MASTER_KEY
)


def parse_credential_package(package):
    if len(package) < 16 + 2 + 1 + 8 + 2 + 32 or package[:16] != b"HaloFPXRegKey01\0":
        raise ValueError("credential header or minimum length")
    offset = 16
    key_id_length = int.from_bytes(package[offset:offset + 2], "big")
    offset += 2
    if not 1 <= key_id_length <= 128 or offset + key_id_length + 42 != len(package):
        raise ValueError("credential key ID length or trailing bytes")
    key_id = package[offset:offset + key_id_length].decode("ascii")
    if any(not 0x21 <= byte <= 0x7E for byte in package[offset:offset + key_id_length]):
        raise ValueError("credential key ID profile")
    offset += key_id_length
    generation = int.from_bytes(package[offset:offset + 8], "big")
    offset += 8
    if generation == 0 or int.from_bytes(package[offset:offset + 2], "big") != 32:
        raise ValueError("credential generation or secret length")
    offset += 2
    return key_id, generation, package[offset:offset + 32]
DERIVED_KEY = hmac.new(
    MASTER_KEY, KEY_DOMAIN + tstr(KEY_ID) + uint(KEY_GENERATION), hashlib.sha256
).digest()


def digest(domain, payload):
    return hashlib.sha256(domain + payload).digest()


def authenticated(kind, body):
    auth_input = mapping([(0, body), (1, tstr(KEY_ID)), (2, uint(1)),
                          (3, uint(KEY_GENERATION))])
    auth_domain, content_domain = DOMAINS[kind]
    tag = hmac.new(DERIVED_KEY, auth_domain + auth_input, hashlib.sha256).digest()
    envelope = mapping([(0, auth_input), (1, bstr(tag))])
    return envelope, tag, digest(content_domain, envelope)


ROOT_ID = rep(0x11)
STORE_UUID = rep(0x22, 16)
REGISTRY_ID = "registry-v1"
FS_UUID = rep(0x44, 16)
SUBVOLUME_UUID = rep(0x45, 16)
PARENT_PATH = b"/mnt/halofpx-l05o-loop"
ROOT_PATH = b"/mnt/halofpx-l05o-loop/run-001"
MOUNT_ID = 731
ST_DEV = 2049
OWNER_UID = 1000
ROOT_MODE = 0o700
AUTHORITY_MODE = 0o600
ATTEMPT_CAPACITY = 512
MAX_LOGICAL_AUTHORITY_BYTES = 16777216
LOOP_IMAGE_BYTES = 1073741824
REQUIRED_HOST_RESERVE_BYTES = 68719476736
REQUIRED_LOOP_FREE_BYTES = 268435456
PATH_POLICY_PREIMAGE = (
    len(PARENT_PATH).to_bytes(8, "big") + PARENT_PATH
    + len(ROOT_PATH).to_bytes(8, "big") + ROOT_PATH
    + FS_UUID + SUBVOLUME_UUID
    + MOUNT_ID.to_bytes(8, "big") + ST_DEV.to_bytes(8, "big")
    + OWNER_UID.to_bytes(8, "big")
    + ROOT_MODE.to_bytes(4, "big") + AUTHORITY_MODE.to_bytes(4, "big")
    + ATTEMPT_CAPACITY.to_bytes(8, "big")
    + MAX_LOGICAL_AUTHORITY_BYTES.to_bytes(8, "big")
    + LOOP_IMAGE_BYTES.to_bytes(8, "big")
    + REQUIRED_HOST_RESERVE_BYTES.to_bytes(8, "big")
    + REQUIRED_LOOP_FREE_BYTES.to_bytes(8, "big")
)
PATH_POLICY = digest(PATH_POLICY_DOMAIN, PATH_POLICY_PREIMAGE)
ATTEMPT_ID = rep(0x66)
QUARANTINE_ID = rep(0x77)

# Exact admitted L05j H and L05k H+1 envelopes; copied as fixed input bytes, not
# imported code. Their independent registry-lab identities are recomputed here.
PREDECESSOR = bytes.fromhex(
    "a200a400a8000101000200036b72656769737472792d76310409055820" + "aa" * 32 +
    "065820" + "bb" * 32 + "071828017072656769737472792d617574682d76310201030d" +
    "0158208f563d0a255e6b09a44b87be0867e84b4b8368d043e5f885fa86348662c509d4"
)
SUCCESSOR = bytes.fromhex(
    "a200a400aa000201000200036b72656769737472792d76310409055820" + "aa" * 32 +
    "065820" + "bb" * 32 + "071829085820a7b731bccfdea83a4595d5257ffa34ef9248bb61499b40a37874895cff6bc1ec" +
    "09a6001829015820" + "cc" * 32 + "025820" + "dd" * 32 + "035820" + "ee" * 32 +
    "045820" + "11" * 32 + "055820" + "22" * 32 +
    "017072656769737472792d617574682d76310201030d" +
    "0158201f3f79689d0b2dd37988aac8c92bf90be6689b0079bd5fc4a7066973fdebd202"
)
PRED_DIGEST = digest(REGISTRY_DOMAIN, PREDECESSOR)
SUCC_DIGEST = digest(REGISTRY_DOMAIN, SUCCESSOR)

operation_input = mapping([
    (0, bstr(ROOT_ID)), (1, bstr(PATH_POLICY)), (2, bstr(ATTEMPT_ID)),
    (3, uint(17)), (4, bstr(PRED_DIGEST)), (5, bstr(SUCC_DIGEST)),
    (6, uint(len(PREDECESSOR))), (7, uint(len(SUCCESSOR))),
])
OPERATION = digest(OPERATION_DOMAIN, operation_input)

head_initial_body = mapping([
    (0, uint(1)), (1, uint(0)), (2, uint(1)), (3, bstr(ROOT_ID)),
    (4, bstr(PATH_POLICY)), (5, bstr(PRED_DIGEST)), (6, uint(len(PREDECESSOR))),
    (7, tstr(REGISTRY_ID)), (8, uint(9)), (9, uint(40)), (10, uint(1)),
    (11, tstr(KEY_ID)), (12, uint(KEY_GENERATION)),
])
HEAD_INITIAL, _, HEAD_INITIAL_DIGEST = authenticated("head", head_initial_body)

root_initializing_body = mapping([
    (0, uint(1)), (1, uint(0)), (2, uint(0)), (3, bstr(ROOT_ID)),
    (4, bstr(STORE_UUID)), (5, tstr(REGISTRY_ID)), (6, uint(9)),
    (7, bstr(FS_UUID)), (8, uint(MOUNT_ID)), (9, uint(OWNER_UID)), (10, tstr(KEY_ID)),
    (11, uint(KEY_GENERATION)), (12, uint(512)), (13, bstr(PATH_POLICY)),
    (14, uint(0)), (15, NULL), (16, uint(ST_DEV)), (17, uint(987654)),
])
ROOT_INITIALIZING, _, _ = authenticated("root", root_initializing_body)

root_initialized_body = mapping([
    (0, uint(1)), (1, uint(0)), (2, uint(0)), (3, bstr(ROOT_ID)),
    (4, bstr(STORE_UUID)), (5, tstr(REGISTRY_ID)), (6, uint(9)),
    (7, bstr(FS_UUID)), (8, uint(MOUNT_ID)), (9, uint(OWNER_UID)), (10, tstr(KEY_ID)),
    (11, uint(KEY_GENERATION)), (12, uint(512)), (13, bstr(PATH_POLICY)),
    (14, uint(1)), (15, bstr(HEAD_INITIAL_DIGEST)),
    (16, uint(ST_DEV)), (17, uint(987654)),
])
ROOT_INITIALIZED, _, ROOT_INITIALIZED_DIGEST = authenticated("root", root_initialized_body)

prepare_body = mapping([
    (0, uint(1)), (1, uint(0)), (2, uint(2)), (3, bstr(ROOT_ID)),
    (4, bstr(PATH_POLICY)), (5, bstr(ATTEMPT_ID)), (6, uint(17)),
    (7, bstr(OPERATION)), (8, uint(len(PREDECESSOR))), (9, bstr(PREDECESSOR)),
    (10, bstr(PRED_DIGEST)), (11, uint(len(SUCCESSOR))), (12, bstr(SUCCESSOR)),
    (13, bstr(SUCC_DIGEST)), (14, uint(1)), (15, bstr(HEAD_INITIAL_DIGEST)),
    (16, bstr(HEAD_INITIAL_DIGEST)),
])
PREPARE, _, PREPARE_DIGEST = authenticated("prepare", prepare_body)

head_successor_body = mapping([
    (0, uint(1)), (1, uint(0)), (2, uint(1)), (3, bstr(ROOT_ID)),
    (4, bstr(PATH_POLICY)), (5, bstr(SUCC_DIGEST)), (6, uint(len(SUCCESSOR))),
    (7, tstr(REGISTRY_ID)), (8, uint(9)), (9, uint(41)), (10, uint(2)),
    (11, tstr(KEY_ID)), (12, uint(KEY_GENERATION)),
])
HEAD_SUCCESSOR, _, HEAD_SUCCESSOR_DIGEST = authenticated("head", head_successor_body)


def terminal(kind, record_kind, classification, selected_head_digest):
    body = mapping([
        (0, uint(1)), (1, uint(0)), (2, uint(record_kind)), (3, bstr(ROOT_ID)),
        (4, bstr(PATH_POLICY)), (5, bstr(ATTEMPT_ID)), (6, uint(17)),
        (7, bstr(OPERATION)), (8, bstr(PRED_DIGEST)), (9, bstr(SUCC_DIGEST)),
        (10, bstr(PREPARE_DIGEST)), (11, uint(2 if kind == "close" else 1)),
        (12, uint(classification)), (13, bstr(selected_head_digest)),
    ])
    return (*authenticated(kind, body), body)


CLOSE, _, CLOSE_DIGEST, close_body = terminal("close", 3, 0, HEAD_SUCCESSOR_DIGEST)
# HEAD still resolves byte-exactly to PREDECESSOR, so this is the admitted
# recovery branch (class 1), never the operation-time predecessor-mismatch
# branch (class 0).
ABORT, _, ABORT_DIGEST, abort_body = terminal("abort", 4, 1, HEAD_INITIAL_DIGEST)

quarantine_body = mapping([
    (0, uint(1)), (1, uint(0)), (2, uint(5)), (3, bstr(ROOT_ID)),
    (4, bstr(PATH_POLICY)), (5, bstr(QUARANTINE_ID)), (6, bstr(ATTEMPT_ID)),
    (7, uint(17)), (8, uint(6)), (9, uint(2)), (10, bstr(PREPARE_DIGEST)),
    (11, bstr(HEAD_SUCCESSOR_DIGEST)), (12, bstr(OPERATION)),
])
QUARANTINE, _, QUARANTINE_DIGEST = authenticated("quarantine", quarantine_body)

FIXTURES = {
    "root_initializing": ("root", ROOT_INITIALIZING, root_initializing_body, 1024),
    "root_initialized": ("root", ROOT_INITIALIZED, root_initialized_body, 1024),
    "head_initial": ("head", HEAD_INITIAL, head_initial_body, 1024),
    "prepare": ("prepare", PREPARE, prepare_body, 4096),
    "head_successor": ("head", HEAD_SUCCESSOR, head_successor_body, 1024),
    "close": ("close", CLOSE, close_body, 1024),
    "abort": ("abort", ABORT, abort_body, 1024),
    "quarantine": ("quarantine", QUARANTINE, quarantine_body, 1024),
}


def read_argument(data, offset, ai):
    if ai < 24:
        return ai, offset
    widths = {24: 1, 25: 2, 26: 4, 27: 8}
    if ai not in widths or offset + widths[ai] > len(data):
        raise ValueError("invalid or truncated argument")
    width = widths[ai]
    value = int.from_bytes(data[offset:offset + width], "big")
    minima = {1: 24, 2: 256, 4: 65536, 8: 4294967296}
    if value < minima[width]:
        raise ValueError("non-shortest argument")
    return value, offset + width


def parse(data, offset=0):
    if offset >= len(data):
        raise ValueError("truncated item")
    first = data[offset]
    major, ai = first >> 5, first & 31
    offset += 1
    if first == 0xF6:
        return None, offset
    value, offset = read_argument(data, offset, ai)
    if major == 0:
        return value, offset
    if major in (2, 3):
        end = offset + value
        if end > len(data):
            raise ValueError("truncated string")
        raw = data[offset:end]
        return (raw if major == 2 else raw.decode("ascii")), end
    if major == 5:
        result = []
        prior = -1
        for _ in range(value):
            key, offset = parse(data, offset)
            if not isinstance(key, int) or key <= prior:
                raise ValueError("map order, type, or duplicate")
            prior = key
            item, offset = parse(data, offset)
            result.append((key, item))
        return result, offset
    raise ValueError("type outside closed profile")


def encode(value):
    if value is None:
        return NULL
    if isinstance(value, int):
        return uint(value)
    if isinstance(value, bytes):
        return bstr(value)
    if isinstance(value, str):
        return tstr(value)
    return mapping([(key, encode(item)) for key, item in value])


def closed_map(value, keys):
    if not isinstance(value, list) or [entry[0] for entry in value] != list(keys):
        raise ValueError("wrong closed-map keys")
    return dict(value)


def bytes_n(value, size, nonzero=False):
    if not isinstance(value, bytes) or len(value) != size or (nonzero and not any(value)):
        raise ValueError("wrong byte-string size or zero identity")


def validate_inner_transition(predecessor, successor):
    def inner(encoded, version):
        outer, end = parse(encoded)
        outer = closed_map(outer, range(2))
        if end != len(encoded):
            raise ValueError("inner trailing bytes")
        auth = closed_map(outer[0], range(4))
        if auth[1] != KEY_ID or auth[2] != 1 or auth[3] != KEY_GENERATION:
            raise ValueError("inner key tuple")
        bytes_n(outer[1], 32)
        tuple_wire = tstr(KEY_ID) + uint(KEY_GENERATION)
        key_domain = (b"halofpx.registry-snapshot-key.v1\0" if version == 1
                      else b"halofpx.registry-successor-key.v1\0")
        auth_domain = (b"halofpx.registry-snapshot-auth.v1\0" if version == 1
                       else b"halofpx.registry-successor-auth.v1\0")
        derived = hmac.new(MASTER_KEY, key_domain + tuple_wire, hashlib.sha256).digest()
        expected = hmac.new(derived, auth_domain + encode(list(auth.items())), hashlib.sha256).digest()
        if not hmac.compare_digest(outer[1], expected):
            raise ValueError("inner HMAC")
        return closed_map(auth[0], range(8 if version == 1 else 10))

    pred = inner(predecessor, 1)
    succ = inner(successor, 2)
    if [pred[i] for i in range(7)] != [1, 0, 0, REGISTRY_ID, 9, rep(0xAA), rep(0xBB)]:
        raise ValueError("predecessor schema or scope")
    if not isinstance(pred[7], int) or pred[7] == 0xFFFFFFFFFFFFFFFF:
        raise ValueError("predecessor high-water")
    if [succ[i] for i in range(7)] != [2, 0, 0, pred[3], pred[4], pred[5], pred[6]]:
        raise ValueError("successor continuity")
    receipt = closed_map(succ[9], range(6))
    if succ[7] != pred[7] + 1 or receipt[0] != succ[7]:
        raise ValueError("successor is not exact H+1")
    for index in range(1, 6):
        bytes_n(receipt[index], 32, True)
    predecessor_identity = digest(b"halofpx.registry-snapshot.v1\0", predecessor)
    if succ[8] != predecessor_identity:
        raise ValueError("successor predecessor identity")
    continuity = hmac.new(
        MASTER_KEY,
        b"halofpx.registry-key-continuity.v1\0" + tstr(KEY_ID) + uint(KEY_GENERATION),
        hashlib.sha256,
    ).digest()
    return pred, succ, receipt, continuity


INNER_PRED, INNER_SUCC, INNER_RECEIPT, INNER_CONTINUITY = validate_inner_transition(
    PREDECESSOR, SUCCESSOR
)


def validate_body(kind, value):
    expected_kind = {"root": 0, "head": 1, "prepare": 2, "close": 3,
                     "abort": 4, "quarantine": 5}[kind]
    key_count = {"root": 18, "head": 13, "prepare": 17, "close": 14,
                 "abort": 14, "quarantine": 13}[kind]
    body = closed_map(value, range(key_count))
    if body[0] != 1 or body[1] != 0 or body[2] != expected_kind:
        raise ValueError("wrong body version or kind")
    bytes_n(body[3], 32, True)
    if body[3] != ROOT_ID:
        raise ValueError("wrong root")
    if kind == "root":
        bytes_n(body[4], 16); bytes_n(body[7], 16)
        if (body[4] != STORE_UUID or body[5] != REGISTRY_ID or body[6] != 9 or
                body[7] != FS_UUID or body[8] != MOUNT_ID or body[9] != OWNER_UID or
                body[10] != KEY_ID or body[11] != KEY_GENERATION or body[12] != 512 or
                body[13] != PATH_POLICY or body[16] != ST_DEV or body[17] != 987654):
            raise ValueError("root marker cross-field")
        if not ((body[14] == 0 and body[15] is None) or
                (body[14] == 1 and body[15] == HEAD_INITIAL_DIGEST)):
            raise ValueError("root marker state")
        return
    bytes_n(body[4], 32)
    if body[4] != PATH_POLICY:
        raise ValueError("path policy")
    if kind == "head":
        bytes_n(body[5], 32)
        candidates = {
            PRED_DIGEST: (PREDECESSOR, 40, 1),
            SUCC_DIGEST: (SUCCESSOR, 41, 2),
        }
        if body[5] not in candidates:
            raise ValueError("HEAD unknown envelope")
        resolved, high_water, selector_generation = candidates[body[5]]
        validate_inner_transition(PREDECESSOR, resolved if resolved == SUCCESSOR else SUCCESSOR)
        if (body[6] != len(resolved) or body[7] != REGISTRY_ID or body[8] != 9 or
                body[9] != high_water or body[10] != selector_generation or
                body[11] != KEY_ID or body[12] != KEY_GENERATION or
                digest(REGISTRY_DOMAIN, resolved) != body[5]):
            raise ValueError("HEAD resolution mismatch")
        return
    if kind == "prepare":
        bytes_n(body[5], 32, True)
        if not isinstance(body[6], int) or not 0 <= body[6] <= 511:
            raise ValueError("slot")
        if body[8] != len(body[9]) or body[11] != len(body[12]) or not (1 <= body[8] <= 1024 and 1 <= body[11] <= 1024):
            raise ValueError("embedded lengths")
        validate_inner_transition(body[9], body[12])
        if body[10] != digest(REGISTRY_DOMAIN, body[9]) or body[13] != digest(REGISTRY_DOMAIN, body[12]):
            raise ValueError("embedded digest")
        operation = digest(OPERATION_DOMAIN, mapping([
            (0, bstr(body[3])), (1, bstr(body[4])), (2, bstr(body[5])),
            (3, uint(body[6])), (4, bstr(body[10])), (5, bstr(body[13])),
            (6, uint(body[8])), (7, uint(body[11])),
        ]))
        if (body[7] != operation or body[14] != 1 or body[15] != HEAD_INITIAL_DIGEST or
                body[16] != HEAD_INITIAL_DIGEST):
            raise ValueError("PREPARE binding or phase")
        return
    if kind in ("close", "abort"):
        bytes_n(body[5], 32, True)
        if (body[6] != 17 or body[7] != OPERATION or body[8] != PRED_DIGEST or
                body[9] != SUCC_DIGEST or body[10] != PREPARE_DIGEST or
                body[12] not in (0, 1)):
            raise ValueError("terminal binding")
        if kind == "close" and (body[11] != 2 or body[13] != HEAD_SUCCESSOR_DIGEST):
            raise ValueError("CLOSE phase or selector")
        if kind == "abort" and (body[11] != 1 or body[13] != HEAD_INITIAL_DIGEST):
            raise ValueError("ABORT phase or selector")
        return
    bytes_n(body[5], 32, True)
    if body[6] is not None: bytes_n(body[6], 32, True)
    if body[7] is not None and not 0 <= body[7] <= 511: raise ValueError("quarantine slot")
    if not 0 <= body[8] <= 15 or not 0 <= body[9] <= 2: raise ValueError("quarantine enum")
    for index in (10, 11, 12):
        if body[index] is not None: bytes_n(body[index], 32)


def verify(kind, encoded, maximum):
    if not 1 <= len(encoded) <= maximum:
        return False
    try:
        outer, end = parse(encoded)
        if end != len(encoded) or [x[0] for x in outer] != [0, 1]:
            return False
        auth_input, supplied_tag = outer[0][1], outer[1][1]
        if [x[0] for x in auth_input] != [0, 1, 2, 3] or len(supplied_tag) != 32:
            return False
        if auth_input[1][1] != KEY_ID or auth_input[2][1] != 1 or auth_input[3][1] != KEY_GENERATION:
            return False
        exact_auth = encode(auth_input)
        expected = hmac.new(DERIVED_KEY, DOMAINS[kind][0] + exact_auth, hashlib.sha256).digest()
        if not hmac.compare_digest(supplied_tag, expected):
            return False
        validate_body(kind, auth_input[0][1])
        return True
    except (ValueError, UnicodeError, TypeError):
        return False


def item(kind, encoded):
    outer, _ = parse(encoded)
    tag = outer[1][1]
    return {
        "encoded_bytes": len(encoded),
        "envelope_hex": encoded.hex(),
        "tag_hmac_sha256_hex": tag.hex(),
        "content_digest_sha256_hex": digest(DOMAINS[kind][1], encoded).hex(),
    }


def body_pairs(body):
    value, end = parse(body)
    if end != len(body):
        raise AssertionError("fixture body trailing bytes")
    return value


def replace_field(body, key, value):
    return encode([(entry_key, value if entry_key == key else entry_value)
                   for entry_key, entry_value in body_pairs(body)])


def expect_schema_reject(label, kind, body, maximum):
    global mutation_checks
    envelope = authenticated(kind, body)[0]
    if verify(kind, envelope, maximum):
        raise AssertionError(f"accepted recomputed-tag hostile body: {label}")
    mutation_checks += 1


mutation_checks = 0
for name, (kind, encoded, _body, maximum) in FIXTURES.items():
    if not verify(kind, encoded, maximum):
        raise AssertionError(f"fixture did not verify: {name}")
    for index in range(len(encoded)):
        changed = bytearray(encoded)
        changed[index] ^= 1
        if verify(kind, bytes(changed), maximum):
            raise AssertionError(f"accepted byte mutation: {name}:{index}")
        mutation_checks += 1
    for cut in (0, 1, len(encoded) - 1):
        if verify(kind, encoded[:cut], maximum):
            raise AssertionError(f"accepted truncation: {name}:{cut}")
        mutation_checks += 1
    if verify(kind, encoded + b"\x00", maximum):
        raise AssertionError(f"accepted trailing byte: {name}")
    mutation_checks += 1

# Recomputed valid outer tags ensure these exercise closed schemas and semantic
# bindings rather than stopping at authentication failure.
for name, (kind, _encoded, body, maximum) in FIXTURES.items():
    pairs = body_pairs(body)
    unknown = encode(pairs + [(pairs[-1][0] + 1, 0)])
    omitted = encode(pairs[:-1])
    reordered = (head(5, len(pairs)) + uint(pairs[1][0]) + encode(pairs[1][1])
                 + uint(pairs[0][0]) + encode(pairs[0][1])
                 + b"".join(uint(key) + encode(value) for key, value in pairs[2:]))
    noncanonical = body[:1] + b"\x18\x00" + body[2:]
    for label, hostile in (("unknown", unknown), ("omitted", omitted),
                           ("reordered", reordered), ("noncanonical", noncanonical),
                           ("wrong-kind", replace_field(body, 2, 99))):
        expect_schema_reject(f"{name}:{label}", kind, hostile, maximum)

expect_schema_reject("root:key-cross-field", "root", replace_field(root_initialized_body, 10, "other"), 1024)
expect_schema_reject("head:length", "head", replace_field(head_initial_body, 6, len(PREDECESSOR) + 1), 1024)
expect_schema_reject("head:high-water", "head", replace_field(head_initial_body, 9, 41), 1024)
expect_schema_reject("prepare:slot", "prepare", replace_field(prepare_body, 6, 512), 4096)
expect_schema_reject("prepare:length", "prepare", replace_field(prepare_body, 8, len(PREDECESSOR) + 1), 4096)
expect_schema_reject("prepare:phase", "prepare", replace_field(prepare_body, 14, 2), 4096)
expect_schema_reject("close:phase", "close", replace_field(close_body, 11, 1), 1024)
expect_schema_reject("abort:phase", "abort", replace_field(abort_body, 11, 2), 1024)
expect_schema_reject("quarantine:slot", "quarantine", replace_field(quarantine_body, 7, 512), 1024)
expect_schema_reject("quarantine:phase", "quarantine", replace_field(quarantine_body, 9, 3), 1024)

# Construct a fully authenticated inner successor that advances H by two, then
# repair every outer length/digest/operation binding. Only ADR-0013/0014
# transition validation can reject it.
successor_outer, _ = parse(SUCCESSOR)
successor_outer = dict(successor_outer)
successor_auth = dict(successor_outer[0])
successor_body_pairs = successor_auth[0]
successor_body = dict(successor_body_pairs)
successor_receipt = dict(successor_body[9])
successor_receipt[0] = 42
successor_body[7] = 42
successor_body[9] = list(successor_receipt.items())
successor_auth[0] = list(successor_body.items())
successor_auth_wire = encode(list(successor_auth.items()))
successor_inner_key = hmac.new(
    MASTER_KEY,
    b"halofpx.registry-successor-key.v1\0" + tstr(KEY_ID) + uint(KEY_GENERATION),
    hashlib.sha256,
).digest()
successor_inner_tag = hmac.new(
    successor_inner_key,
    b"halofpx.registry-successor-auth.v1\0" + successor_auth_wire,
    hashlib.sha256,
).digest()
hostile_successor = mapping([(0, successor_auth_wire), (1, bstr(successor_inner_tag))])
hostile_successor_digest = digest(REGISTRY_DOMAIN, hostile_successor)
hostile_operation = digest(OPERATION_DOMAIN, mapping([
    (0, bstr(ROOT_ID)), (1, bstr(PATH_POLICY)), (2, bstr(ATTEMPT_ID)),
    (3, uint(17)), (4, bstr(PRED_DIGEST)), (5, bstr(hostile_successor_digest)),
    (6, uint(len(PREDECESSOR))), (7, uint(len(hostile_successor))),
]))
hostile_prepare = prepare_body
for key, value in ((7, hostile_operation), (11, len(hostile_successor)),
                   (12, hostile_successor), (13, hostile_successor_digest)):
    hostile_prepare = replace_field(hostile_prepare, key, value)
expect_schema_reject("prepare:inner-H-plus-two", "prepare", hostile_prepare, 4096)

for domain in ([KEY_DOMAIN, REGISTRY_DOMAIN, OPERATION_DOMAIN, PATH_POLICY_DOMAIN]
               + INNER_DOMAINS + [d for pair in DOMAINS.values() for d in pair]):
    if domain.count(b"\0") != 1 or not domain.endswith(b"\0"):
        raise AssertionError("domain does not contain exactly one trailing NUL")
    for changed in (domain[:-1], domain + b"\0", bytes([domain[0] ^ 1]) + domain[1:]):
        if hashlib.sha256(changed + b"probe").digest() == hashlib.sha256(domain + b"probe").digest():
            raise AssertionError("domain mutation collision")
        mutation_checks += 1

for index in range(len(PATH_POLICY_PREIMAGE)):
    changed = bytearray(PATH_POLICY_PREIMAGE)
    changed[index] ^= 1
    if digest(PATH_POLICY_DOMAIN, bytes(changed)) == PATH_POLICY:
        raise AssertionError(f"path-policy preimage mutation collision: {index}")
    mutation_checks += 1

if parse_credential_package(CREDENTIAL_PACKAGE) != (KEY_ID, KEY_GENERATION, MASTER_KEY):
    raise AssertionError("credential package did not round-trip")
for index in range(len(CREDENTIAL_PACKAGE)):
    changed = bytearray(CREDENTIAL_PACKAGE)
    changed[index] ^= 1
    try:
        changed_id, changed_generation, changed_secret = parse_credential_package(bytes(changed))
        changed_derived = hmac.new(
            changed_secret,
            KEY_DOMAIN + tstr(changed_id) + uint(changed_generation),
            hashlib.sha256,
        ).digest()
        if changed_derived == DERIVED_KEY:
            raise AssertionError(f"credential mutation retained effective key: {index}")
    except (ValueError, UnicodeError):
        pass
    mutation_checks += 1

for malformed in (b"\xa2\x01\x00\x00\x00", b"\xa2\x00\x00\x00\x01", b"\xbf\xff", b"\x18\x01"):
    try:
        _, end = parse(malformed)
        if end == len(malformed):
            raise AssertionError("accepted structural mutation")
    except (ValueError, UnicodeError):
        pass
    mutation_checks += 1

result = {
    "format": "halofpx-registry-lab-golden-v1",
    "domains_ascii_escaped": {
        "key": "halofpx.registry-lab-key.v1\\0",
        "registry_envelope": "halofpx.registry-lab-registry-envelope.v1\\0",
        "operation": "halofpx.registry-lab-operation.v1\\0",
        "path_policy": "halofpx.registry-lab-path-policy.v1\\0",
        **{f"{kind}_{which}": domain[:-1].decode("ascii") + "\\0"
           for kind, pair in DOMAINS.items()
           for which, domain in zip(("auth", "content"), pair)},
    },
    "derived_key_hmac_sha256_hex": DERIVED_KEY.hex(),
    "credential_package_bytes": len(CREDENTIAL_PACKAGE),
    "credential_package_hex": CREDENTIAL_PACKAGE.hex(),
    "path_policy": {
        "parent_ascii": PARENT_PATH.decode("ascii"),
        "root_ascii": ROOT_PATH.decode("ascii"),
        "filesystem_uuid_hex": FS_UUID.hex(),
        "subvolume_uuid_hex": SUBVOLUME_UUID.hex(),
        "mount_id": MOUNT_ID,
        "st_dev": ST_DEV,
        "owner_uid": OWNER_UID,
        "root_mode_octal": "0700",
        "authority_file_mode_octal": "0600",
        "attempt_capacity": ATTEMPT_CAPACITY,
        "maximum_logical_authority_bytes": MAX_LOGICAL_AUTHORITY_BYTES,
        "loop_image_bytes": LOOP_IMAGE_BYTES,
        "required_host_reserve_bytes": REQUIRED_HOST_RESERVE_BYTES,
        "required_loop_free_bytes": REQUIRED_LOOP_FREE_BYTES,
        "preimage_without_domain_hex": PATH_POLICY_PREIMAGE.hex(),
        "commitment_sha256_hex": PATH_POLICY.hex(),
    },
    "inner_transition": {
        "registry_key_id": KEY_ID,
        "registry_key_generation": KEY_GENERATION,
        "predecessor_high_water": INNER_PRED[7],
        "successor_high_water": INNER_SUCC[7],
        "receipt_sequence": INNER_RECEIPT[0],
        "predecessor_snapshot_digest_hex": digest(b"halofpx.registry-snapshot.v1\0", PREDECESSOR).hex(),
        "successor_digest_hex": digest(b"halofpx.registry-successor.v1\0", SUCCESSOR).hex(),
        "key_continuity_hmac_sha256_hex": INNER_CONTINUITY.hex(),
    },
    "predecessor_registry_envelope_digest_hex": PRED_DIGEST.hex(),
    "predecessor_registry_envelope_hex": PREDECESSOR.hex(),
    "successor_registry_envelope_digest_hex": SUCC_DIGEST.hex(),
    "operation_commitment_sha256_hex": OPERATION.hex(),
    "mutation_checks": mutation_checks,
    "fixtures": {name: item(kind, encoded) for name, (kind, encoded, _, _) in FIXTURES.items()},
}

if len(sys.argv) == 1:
    print(json.dumps(result, indent=2, sort_keys=True))
elif len(sys.argv) == 3 and sys.argv[1] == "--check":
    expected = json.loads(pathlib.Path(sys.argv[2]).read_text(encoding="utf-8"))
    if expected != result:
        raise SystemExit("golden vector mismatch")
    print(f"PASS: {len(FIXTURES)} fixtures; {mutation_checks} mutation checks")
else:
    raise SystemExit(f"usage: {pathlib.Path(sys.argv[0]).name} [--check VECTOR.json]")
