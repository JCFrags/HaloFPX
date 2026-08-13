from __future__ import annotations

import copy
import importlib.util
import json
import shutil
import sys
import tempfile
import unittest
from pathlib import Path
from types import SimpleNamespace
from unittest import mock


REPO_ROOT = Path(__file__).resolve().parents[1]
SCRIPT_PATH = REPO_ROOT / "scripts" / "halofpx-fresh-pc-recovery.py"
REGISTRY_PATH = REPO_ROOT / "docs" / "publication" / "continuation-releases.json"
SPEC = importlib.util.spec_from_file_location("halofpx_fresh_pc_recovery", SCRIPT_PATH)
assert SPEC is not None and SPEC.loader is not None
recovery = importlib.util.module_from_spec(SPEC)
sys.modules[SPEC.name] = recovery
SPEC.loader.exec_module(recovery)


def write_json(path: Path, value: object) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(json.dumps(value, indent=2) + "\n", encoding="utf-8")


def release_api_payload(release: dict) -> dict:
    return {
        "id": release["release_id"],
        "tag_name": release["tag"],
        "target_commitish": release["peeled_commit"],
        "draft": release["draft"],
        "prerelease": release["prerelease"],
        "immutable": release["immutable"],
        "published_at": release["published_at_utc"],
        "assets": [
            {
                "name": asset["name"],
                "size": asset["size_bytes"],
                "state": "uploaded",
                "digest": f"sha256:{asset['sha256']}",
            }
            for asset in release["_assets"]
        ],
    }


def attestation_payload(release: dict) -> dict:
    repository = release["_repository"]
    purl = f"pkg:github/{repository['slug']}@{release['tag']}"
    subjects = [
        {
            "uri": purl,
            "digest": {"sha1": release["tag_object"]},
        }
    ]
    subjects.extend(
        {
            "name": asset["name"],
            "digest": {"sha256": asset["sha256"]},
        }
        for asset in release["_assets"]
    )
    return {
        "attestation": {"bundle": "opaque-to-the-runner"},
        "verificationResult": {
            "mediaType": (
                "application/vnd.dev.sigstore.verificationresult+json;version=0.1"
            ),
            "signature": {
                "certificate": {
                    "certificateIssuer": "CN=Fulcio Intermediate l1,O=GitHub\\, Inc.",
                    "subjectAlternativeName": "https://dotcom.releases.github.com",
                }
            },
            "verifiedTimestamps": [
                {
                    "type": "TimestampAuthority",
                    "uri": "timestamp.githubapp.com",
                    "timestamp": "2026-08-13T03:45:11Z",
                }
            ],
            "verifiedIdentity": {
                "subjectAlternativeName": {
                    "subjectAlternativeName": "",
                    "regexp": "^https://dotcom\\.releases\\.github\\.com$",
                },
                "issuer": {"issuer": "", "regexp": ".*"},
            },
            "statement": {
                "_type": "https://in-toto.io/Statement/v1",
                "subject": subjects,
                "predicateType": "https://in-toto.io/attestation/release/v0.2",
                "predicate": {
                    "databaseId": str(release["release_id"]),
                    "ownerId": str(repository["owner_id"]),
                    "packageId": str(repository["id"]),
                    "purl": purl,
                    "repository": repository["slug"],
                    "repositoryId": str(repository["id"]),
                    "tag": release["tag"],
                },
            },
        },
    }


class FakeCommandRunner:
    """Deterministic offline command boundary for Recovery orchestration tests."""

    def __init__(self, registry: dict, expected_commit: str) -> None:
        self.registry = registry
        self.expected_commit = expected_commit
        self.calls: list[tuple[tuple[str, ...], Path | None]] = []

    def run(
        self, argv: list[str], *, cwd: Path | None = None
    ) -> recovery.CommandResult:
        command = tuple(argv)
        self.calls.append((command, cwd))
        stdout = ""

        if command == ("gh", "--version"):
            stdout = "gh version 2.96.0 (test)\n"
        elif command in {
            ("git", "--version"),
            ("cmake", "--version"),
            ("ninja", "--version"),
            ("cc", "--version"),
            ("c++", "--version"),
            ("sha256sum", "--version"),
            ("tar", "--version"),
        }:
            stdout = f"{command[0]} test-version\n"
        elif command[:3] == ("pwsh", "-NoLogo", "-NoProfile") and command[-1] == "$PSVersionTable.PSVersion.ToString()":
            stdout = "7.5.2\n"
        elif command == ("git", "rev-parse", "--show-toplevel"):
            assert cwd is not None
            stdout = f"{cwd}\n"
        elif command == ("git", "remote", "get-url", "origin"):
            stdout = "https://github.com/JCFrags/HaloFPX.git\n"
        elif command == ("git", "rev-parse", "--is-shallow-repository"):
            stdout = "false\n"
        elif command in (
            ("git", "rev-parse", "HEAD"),
            ("git", "rev-parse", "origin/main"),
        ):
            stdout = f"{self.expected_commit}\n"
        elif command[:2] == ("git", "rev-parse") and len(command) == 3:
            ref = command[2]
            peeled = ref.endswith("^{}")
            tag = ref[:-3] if peeled else ref
            release = next(item for item in self.registry["releases"] if item["tag"] == tag)
            stdout = f"{release['peeled_commit'] if peeled else release['tag_object']}\n"
        elif command == (
            "gh",
            "api",
            f"repos/{self.registry['repository']['slug']}",
        ):
            repository = self.registry["repository"]
            stdout = json.dumps(
                {
                    "id": repository["id"],
                    "full_name": repository["slug"],
                    "visibility": repository["visibility"],
                    "owner": {"id": repository["owner_id"]},
                    "generation": len(self.calls),
                }
            )
        elif command[:2] == ("gh", "api") and "/releases/tags/" in command[2]:
            tag = command[2].rsplit("/", maxsplit=1)[1]
            release = next(item for item in self.registry["releases"] if item["tag"] == tag)
            stdout = json.dumps(release_api_payload(release))
        elif command[:3] == ("gh", "release", "verify") and command[-2:] == ("--format", "json"):
            tag = command[3]
            release = next(item for item in self.registry["releases"] if item["tag"] == tag)
            stdout = json.dumps(attestation_payload(release))

        return recovery.CommandResult(command, 0, stdout, "")


class Python312(tuple):
    major = 3
    minor = 12

    def __new__(cls) -> "Python312":
        return super().__new__(cls, (3, 12, 0, "final", 0))


class RegistryTests(unittest.TestCase):
    def setUp(self) -> None:
        self.raw = json.loads(REGISTRY_PATH.read_text(encoding="utf-8"))

    def load_changed(self, changed: dict) -> dict:
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / "registry.json"
            write_json(path, changed)
            return recovery.load_registry(path, REPO_ROOT)

    def test_tracked_registry_resolves_four_exact_releases_and_all_assets(self) -> None:
        registry = recovery.load_registry(REGISTRY_PATH, REPO_ROOT)
        releases = registry["releases"]

        self.assertEqual(len(releases), 4)
        self.assertEqual(
            [len(item["_assets"]) for item in releases],
            [41, 1, 1, 9],
        )
        self.assertEqual(sum(len(item["_assets"]) for item in releases), 52)
        self.assertEqual(
            sum(
                asset["size_bytes"]
                for item in releases
                for asset in item["_assets"]
            ),
            24_696_192_820,
        )
        self.assertEqual(
            registry["storage"]["all_release_assets_bytes"], 24_696_192_820
        )
        self.assertEqual(
            registry["storage"]["minimum_free_bytes"], 53_687_091_200
        )

    def test_duplicate_release_tag_or_asset_name_is_rejected(self) -> None:
        duplicate_release = copy.deepcopy(self.raw)
        duplicate_release["releases"].append(
            copy.deepcopy(duplicate_release["releases"][1])
        )

        duplicate_asset = copy.deepcopy(self.raw)
        direct = duplicate_asset["releases"][1]
        direct["asset_authority"]["assets"].append(
            copy.deepcopy(direct["asset_authority"]["assets"][0])
        )
        direct["asset_count"] += 1
        direct["asset_bytes"] *= 2

        for label, candidate in (
            ("release", duplicate_release),
            ("asset", duplicate_asset),
        ):
            with self.subTest(label=label):
                with self.assertRaises(recovery.RecoveryError):
                    self.load_changed(candidate)

    def test_unsafe_source_path_and_bad_digest_are_rejected(self) -> None:
        unsafe = copy.deepcopy(self.raw)
        unsafe["releases"][0]["asset_authority"]["path"] = "../outside.json"

        bad_digest = copy.deepcopy(self.raw)
        bad_digest["releases"][1]["asset_authority"]["assets"][0][
            "sha256"
        ] = "not-a-sha256"

        for label, candidate in (("unsafe", unsafe), ("digest", bad_digest)):
            with self.subTest(label=label):
                with self.assertRaises(recovery.RecoveryError):
                    self.load_changed(candidate)

    def test_mutable_draft_and_noninteger_release_totals_are_rejected(self) -> None:
        cases: list[tuple[str, dict]] = []
        for field, value in (
            ("draft", True),
            ("immutable", False),
            ("asset_count", True),
            ("asset_count", "1"),
            ("asset_bytes", False),
            ("asset_bytes", "40697"),
        ):
            candidate = copy.deepcopy(self.raw)
            candidate["releases"][1][field] = value
            cases.append((f"{field}={value!r}", candidate))

        for label, candidate in cases:
            with self.subTest(label=label):
                with self.assertRaises(recovery.RecoveryError):
                    self.load_changed(candidate)

    def test_source_receipt_with_duplicate_asset_is_rejected(self) -> None:
        with tempfile.TemporaryDirectory() as directory:
            repo = Path(directory)
            registry = copy.deepcopy(self.raw)
            fixture = json.loads(
                (
                    REPO_ROOT
                    / "docs"
                    / "halofpx"
                    / "evidence"
                    / "2026-08-12-qwen3-0.6b-rocmfpx-fixture"
                    / "publication-receipt.json"
                ).read_text(encoding="utf-8")
            )
            fixture["assets"].append(copy.deepcopy(fixture["assets"][0]))
            fixture["release"]["asset_count"] += 1
            target = (
                repo
                / "docs"
                / "halofpx"
                / "evidence"
                / "2026-08-12-qwen3-0.6b-rocmfpx-fixture"
                / "publication-receipt.json"
            )
            write_json(target, fixture)
            # Only the fixture source is reached before this deliberately malformed
            # receipt; the other source-backed release is made direct for isolation.
            original = registry["releases"][0]
            original["asset_authority"] = {
                "kind": "direct",
                "assets": [
                    {
                        "name": f"original-{index}",
                        "size_bytes": 1,
                        "sha256": f"{index + 1:064x}",
                    }
                    for index in range(original["asset_count"])
                ],
            }
            original["asset_bytes"] = original["asset_count"]
            registry["storage"]["all_release_assets_bytes"] = (
                original["asset_bytes"]
                + sum(item["asset_bytes"] for item in registry["releases"][1:])
            )
            registry_path = repo / "registry.json"
            write_json(registry_path, registry)

            with self.assertRaises(recovery.RecoveryError):
                recovery.load_registry(registry_path, repo)

    def test_source_authorities_must_bind_their_enclosing_release(self) -> None:
        original_source = json.loads(
            (REPO_ROOT / "docs" / "publication" / "release-manifest.json").read_text(
                encoding="utf-8"
            )
        )
        fixture_source = json.loads(
            (
                REPO_ROOT
                / "docs"
                / "halofpx"
                / "evidence"
                / "2026-08-12-qwen3-0.6b-rocmfpx-fixture"
                / "publication-receipt.json"
            ).read_text(encoding="utf-8")
        )
        cases: list[tuple[str, dict, dict]] = []
        for field, value in (
            ("repository", "SomeoneElse/HaloFPX"),
            ("release_tag", "wrong-tag"),
            ("visibility", "public"),
        ):
            changed = copy.deepcopy(original_source)
            changed[field] = value
            cases.append((f"original-{field}", changed, fixture_source))
        for field, value in (
            ("id", 1),
            ("tag", "wrong-tag"),
            ("target_commit", "f" * 40),
            ("draft", True),
            ("prerelease", False),
            ("immutable", False),
            ("asset_count", 8),
        ):
            changed = copy.deepcopy(fixture_source)
            changed["release"][field] = value
            cases.append((f"fixture-{field}", original_source, changed))

        for label, original, fixture in cases:
            with self.subTest(label=label), tempfile.TemporaryDirectory() as directory:
                repo = Path(directory)
                registry = copy.deepcopy(self.raw)
                original_path = repo / registry["releases"][0]["asset_authority"]["path"]
                fixture_path = repo / registry["releases"][3]["asset_authority"]["path"]
                write_json(original_path, original)
                write_json(fixture_path, fixture)
                registry_path = repo / "registry.json"
                write_json(registry_path, registry)
                with self.assertRaises(recovery.RecoveryError):
                    recovery.load_registry(registry_path, repo)

    def test_split_payloads_and_reconstructed_total_are_strict(self) -> None:
        original_source = json.loads(
            (REPO_ROOT / "docs" / "publication" / "release-manifest.json").read_text(
                encoding="utf-8"
            )
        )
        fixture_source = json.loads(
            (
                REPO_ROOT
                / "docs"
                / "halofpx"
                / "evidence"
                / "2026-08-12-qwen3-0.6b-rocmfpx-fixture"
                / "publication-receipt.json"
            ).read_text(encoding="utf-8")
        )
        mutations: list[tuple[str, dict]] = []
        wrong_size = copy.deepcopy(original_source)
        wrong_size["split_payloads"][0]["original_size_bytes"] += 1
        mutations.append(("wrong-size", wrong_size))
        duplicate_part = copy.deepcopy(original_source)
        duplicate_part["split_payloads"][0]["reassembly_order"][1] = duplicate_part[
            "split_payloads"
        ][0]["reassembly_order"][0]
        mutations.append(("duplicate-part", duplicate_part))
        missing_part = copy.deepcopy(original_source)
        missing_part["split_payloads"][0]["reassembly_order"][0] = "missing.part"
        mutations.append(("missing-part", missing_part))
        unsafe_name = copy.deepcopy(original_source)
        unsafe_name["split_payloads"][0]["logical_name"] = "../escape.tar"
        mutations.append(("unsafe-name", unsafe_name))
        bad_digest = copy.deepcopy(original_source)
        bad_digest["split_payloads"][0]["original_sha256"] = "bad"
        mutations.append(("bad-digest", bad_digest))

        for label, original in mutations:
            with self.subTest(label=label), tempfile.TemporaryDirectory() as directory:
                repo = Path(directory)
                registry = copy.deepcopy(self.raw)
                write_json(
                    repo / registry["releases"][0]["asset_authority"]["path"], original
                )
                write_json(
                    repo / registry["releases"][3]["asset_authority"]["path"],
                    fixture_source,
                )
                registry_path = repo / "registry.json"
                write_json(registry_path, registry)
                with self.assertRaises(recovery.RecoveryError):
                    recovery.load_registry(registry_path, repo)

        wrong_total = copy.deepcopy(self.raw)
        wrong_total["storage"]["all_release_assets_plus_reconstructed_bytes"] += 1
        with self.assertRaises(recovery.RecoveryError):
            self.load_changed(wrong_total)


class WorkRootSafetyTests(unittest.TestCase):
    def setUp(self) -> None:
        self.temporary = tempfile.TemporaryDirectory()
        self.base = Path(self.temporary.name).resolve()
        self.checkout = self.base / "checkout"
        self.checkout.mkdir()

    def tearDown(self) -> None:
        self.temporary.cleanup()

    def test_new_absolute_sibling_work_root_is_accepted(self) -> None:
        work_root = self.base / "recovery work"
        self.assertEqual(
            recovery.validate_work_root(work_root, self.checkout), work_root
        )

    def test_relative_checkout_ancestor_and_descendant_are_rejected(self) -> None:
        candidates = {
            "relative": Path("relative-recovery-root"),
            "checkout": self.checkout,
            "ancestor": self.base,
            "descendant": self.checkout / "recovery",
        }
        for label, candidate in candidates.items():
            with self.subTest(label=label):
                with self.assertRaises(recovery.UsageSafetyError):
                    recovery.validate_work_root(candidate, self.checkout)

    def test_existing_unmarked_directory_is_rejected(self) -> None:
        work_root = self.base / "existing"
        work_root.mkdir()
        (work_root / "unrelated.txt").write_text("preserve me\n", encoding="utf-8")

        with self.assertRaises(recovery.UsageSafetyError):
            recovery.validate_work_root(work_root, self.checkout)

    def test_symlink_work_root_is_rejected_when_supported(self) -> None:
        target = self.base / "target"
        target.mkdir()
        link = self.base / "linked-work"
        try:
            link.symlink_to(target, target_is_directory=True)
        except OSError as error:
            self.skipTest(f"directory symlink unavailable: {error}")

        with self.assertRaises(recovery.UsageSafetyError):
            recovery.validate_work_root(link, self.checkout)

    def test_linked_descendant_and_linked_marker_are_rejected(self) -> None:
        target = self.base / "outside-target"
        target.mkdir()
        for label, link_name in (("nested", "metadata"), ("marker", recovery.MARKER_NAME)):
            with self.subTest(label=label):
                work_root = self.base / f"marked-{label}"
                work_root.mkdir()
                marker = work_root / recovery.MARKER_NAME
                if label == "nested":
                    write_json(
                        marker,
                        {
                            "schema_version": recovery.SCHEMA_VERSION,
                            "purpose": "halofpx-fresh-pc-recovery",
                        },
                    )
                link_target = target / f"{label}-target"
                if label == "nested":
                    link_target.mkdir()
                else:
                    write_json(
                        link_target,
                        {
                            "schema_version": recovery.SCHEMA_VERSION,
                            "purpose": "halofpx-fresh-pc-recovery",
                        },
                    )
                try:
                    (work_root / link_name).symlink_to(link_target, target_is_directory=label == "nested")
                except OSError as error:
                    self.skipTest(f"directory symlink unavailable: {error}")
                with self.assertRaises(recovery.UsageSafetyError):
                    recovery.validate_work_root(work_root, self.checkout)

    def test_broken_work_root_symlink_is_rejected_when_supported(self) -> None:
        link = self.base / "broken-recovery-root"
        try:
            link.symlink_to(self.base / "does-not-exist", target_is_directory=True)
        except OSError as error:
            self.skipTest(f"directory symlink unavailable: {error}")
        with self.assertRaises(recovery.UsageSafetyError):
            recovery.validate_work_root(link, self.checkout)


class ReleaseApiTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls) -> None:
        cls.registry = recovery.load_registry(REGISTRY_PATH, REPO_ROOT)
        cls.release = cls.registry["releases"][0]

    def verify(self, payload: dict) -> dict:
        return recovery.verify_release_api(
            self.release, self.release["_assets"], payload
        )

    def test_exact_api_payload_passes(self) -> None:
        result = self.verify(release_api_payload(self.release))
        self.assertEqual(result["tag"], self.release["tag"])
        self.assertEqual(result["asset_count"], 41)

    def test_wrong_identity_or_release_state_fails_closed(self) -> None:
        cases = {
            "id": ("id", self.release["release_id"] + 1),
            "tag": ("tag_name", "wrong-tag"),
            "target": ("target_commitish", "f" * 40),
            "immutable": ("immutable", False),
            "draft": ("draft", True),
            "prerelease": ("prerelease", True),
        }
        for label, (field, value) in cases.items():
            with self.subTest(label=label):
                payload = release_api_payload(self.release)
                payload[field] = value
                with self.assertRaises(recovery.RecoveryError):
                    self.verify(payload)

    def test_api_identity_and_state_types_are_strict(self) -> None:
        for field, value in (
            ("id", str(self.release["release_id"])),
            ("draft", 0),
            ("prerelease", 0),
            ("immutable", 1),
        ):
            with self.subTest(field=field):
                payload = release_api_payload(self.release)
                payload[field] = value
                with self.assertRaises(recovery.RecoveryError):
                    self.verify(payload)

    def test_missing_extra_duplicate_wrong_state_and_wrong_digest_assets_fail(self) -> None:
        cases: dict[str, dict] = {}

        missing = release_api_payload(self.release)
        missing["assets"].pop()
        cases["missing"] = missing

        extra = release_api_payload(self.release)
        extra["assets"].append(
            {
                "name": "unexpected.bin",
                "size": 1,
                "state": "uploaded",
                "digest": f"sha256:{'a' * 64}",
            }
        )
        cases["extra"] = extra

        duplicate = release_api_payload(self.release)
        duplicate["assets"].append(copy.deepcopy(duplicate["assets"][0]))
        cases["duplicate"] = duplicate

        state = release_api_payload(self.release)
        state["assets"][0]["state"] = "new"
        cases["state"] = state

        digest = release_api_payload(self.release)
        digest["assets"][0]["digest"] = f"sha256:{'f' * 64}"
        cases["digest"] = digest

        size = release_api_payload(self.release)
        size["assets"][0]["size"] += 1
        cases["size"] = size

        for label, payload in cases.items():
            with self.subTest(label=label):
                with self.assertRaises(recovery.RecoveryError):
                    self.verify(payload)


class AttestationTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls) -> None:
        cls.registry = recovery.load_registry(REGISTRY_PATH, REPO_ROOT)
        cls.release = cls.registry["releases"][3]

    def verify(self, payload: dict) -> dict:
        return recovery.verify_attestation(
            self.release, self.release["_assets"], payload
        )

    def test_exact_attestation_passes(self) -> None:
        result = self.verify(attestation_payload(self.release))
        self.assertEqual(result["tag"], self.release["tag"])
        self.assertEqual(result["asset_count"], 9)

    def test_wrong_statement_and_predicate_identity_fail_closed(self) -> None:
        mutations = {
            "statement_type": ("_type", "https://in-toto.io/Statement/v0.1"),
            "predicate_type": (
                "predicateType",
                "https://in-toto.io/attestation/release/v0.1",
            ),
        }
        for label, (field, value) in mutations.items():
            with self.subTest(label=label):
                payload = attestation_payload(self.release)
                payload["verificationResult"]["statement"][field] = value
                with self.assertRaises(recovery.RecoveryError):
                    self.verify(payload)

        predicate_mutations = {
            "repository": "SomeoneElse/HaloFPX",
            "tag": "wrong-tag",
            "databaseId": "1",
            "ownerId": "1",
            "packageId": "1",
            "repositoryId": "1",
            "purl": "pkg:github/SomeoneElse/HaloFPX@wrong",
        }
        for field, value in predicate_mutations.items():
            with self.subTest(predicate_field=field):
                payload = attestation_payload(self.release)
                payload["verificationResult"]["statement"]["predicate"][
                    field
                ] = value
                with self.assertRaises(recovery.RecoveryError):
                    self.verify(payload)

    def test_wrong_package_subject_or_asset_subject_fails_closed(self) -> None:
        package_sha = attestation_payload(self.release)
        package_sha["verificationResult"]["statement"]["subject"][0]["digest"][
            "sha1"
        ] = "f" * 40

        package_purl = attestation_payload(self.release)
        package_purl["verificationResult"]["statement"]["subject"][0][
            "uri"
        ] = "pkg:github/JCFrags/HaloFPX@wrong"

        asset_digest = attestation_payload(self.release)
        asset_digest["verificationResult"]["statement"]["subject"][1]["digest"][
            "sha256"
        ] = "f" * 64

        missing_asset = attestation_payload(self.release)
        missing_asset["verificationResult"]["statement"]["subject"].pop()

        duplicate_asset = attestation_payload(self.release)
        duplicate_asset["verificationResult"]["statement"]["subject"].append(
            copy.deepcopy(
                duplicate_asset["verificationResult"]["statement"]["subject"][1]
            )
        )

        for label, payload in (
            ("package_sha", package_sha),
            ("package_purl", package_purl),
            ("asset_digest", asset_digest),
            ("missing_asset", missing_asset),
            ("duplicate_asset", duplicate_asset),
        ):
            with self.subTest(label=label):
                with self.assertRaises(recovery.RecoveryError):
                    self.verify(payload)

    def test_malformed_attestation_or_verification_result_fails_closed(self) -> None:
        malformed = [
            {},
            {"attestation": [], "verificationResult": {}},
            {"attestation": {}, "verificationResult": []},
            {"attestation": {}, "verificationResult": {"statement": "bad"}},
        ]
        for index, payload in enumerate(malformed):
            with self.subTest(index=index):
                with self.assertRaises(recovery.RecoveryError):
                    self.verify(payload)

    def test_attestation_verification_envelope_is_required(self) -> None:
        cases: list[tuple[str, dict]] = []
        for field, value in (
            ("mediaType", "application/json"),
            ("signature", {}),
            ("verifiedIdentity", {}),
            ("verifiedTimestamps", []),
        ):
            payload = attestation_payload(self.release)
            payload["verificationResult"][field] = value
            cases.append((field, payload))
        no_attestation = attestation_payload(self.release)
        no_attestation["attestation"] = {}
        cases.append(("attestation", no_attestation))

        for label, payload in cases:
            with self.subTest(label=label):
                with self.assertRaises(recovery.RecoveryError):
                    self.verify(payload)

        nested_cases: list[tuple[str, tuple[str, ...], object]] = [
            (
                "certificate-san",
                ("signature", "certificate", "subjectAlternativeName"),
                "https://attacker.invalid",
            ),
            (
                "identity-san-regexp",
                ("verifiedIdentity", "subjectAlternativeName", "regexp"),
                ".*",
            ),
            (
                "identity-issuer-regexp",
                ("verifiedIdentity", "issuer", "regexp"),
                "^attacker$",
            ),
            ("timestamp-type", ("verifiedTimestamps", "0", "type"), "Unknown"),
            ("timestamp-uri", ("verifiedTimestamps", "0", "uri"), "evil.invalid"),
        ]
        for label, path, value in nested_cases:
            with self.subTest(label=label):
                payload = attestation_payload(self.release)
                cursor: object = payload["verificationResult"]
                for component in path[:-1]:
                    cursor = cursor[int(component)] if component.isdigit() else cursor[component]
                cursor[path[-1]] = value
                with self.assertRaises(recovery.RecoveryError):
                    self.verify(payload)


class AtomicJsonTests(unittest.TestCase):
    def test_atomic_write_creates_and_replaces_json_without_temp_residue(self) -> None:
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / "nested" / "state.json"
            recovery.atomic_write_json(path, {"step": 1})
            self.assertEqual(json.loads(path.read_text(encoding="utf-8")), {"step": 1})

            recovery.atomic_write_json(path, {"step": 2, "status": "OPEN"})
            self.assertEqual(
                json.loads(path.read_text(encoding="utf-8")),
                {"step": 2, "status": "OPEN"},
            )
            self.assertEqual([item.name for item in path.parent.iterdir()], [path.name])

    def test_failed_serialization_preserves_existing_state(self) -> None:
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / "state.json"
            recovery.atomic_write_json(path, {"stable": True})

            with self.assertRaises((TypeError, recovery.RecoveryError)):
                recovery.atomic_write_json(path, {"not_json": {1, 2, 3}})

            self.assertEqual(
                json.loads(path.read_text(encoding="utf-8")), {"stable": True}
            )


class RecoveryOrchestrationTests(unittest.TestCase):
    EXPECTED_COMMIT = "3758febacfc07fdc6e84b63637131b02d413de59"

    def setUp(self) -> None:
        self.temporary = tempfile.TemporaryDirectory()
        self.base = Path(self.temporary.name).resolve()
        self.work_root = self.base / "recovery work"
        self.registry = recovery.load_registry(REGISTRY_PATH, REPO_ROOT)
        self.runner = FakeCommandRunner(self.registry, self.EXPECTED_COMMIT)
        self.instance = recovery.Recovery(
            REGISTRY_PATH,
            self.work_root,
            self.EXPECTED_COMMIT,
            repo_root=REPO_ROOT,
            runner=self.runner,
        )

    def tearDown(self) -> None:
        self.temporary.cleanup()

    def run_mocked_preflight(self) -> dict:
        found = lambda name: f"/test-tools/{name}"
        disk = SimpleNamespace(
            total=100 * 1024**3,
            used=10 * 1024**3,
            free=90 * 1024**3,
        )
        with (
            mock.patch.object(recovery.sys, "version_info", Python312()),
            mock.patch.object(recovery.sys, "version", "3.12.0 (test)"),
            mock.patch.object(recovery.shutil, "which", side_effect=found),
            mock.patch.object(recovery.shutil, "disk_usage", return_value=disk),
        ):
            return self.instance.run_preflight()

    def test_mocked_preflight_uses_only_pinned_read_only_commands(self) -> None:
        details = self.run_mocked_preflight()
        commands = [argv for argv, _ in self.runner.calls]

        self.assertEqual(details["head"], self.EXPECTED_COMMIT)
        self.assertEqual(
            set(details["tool_versions"]),
            {"git", "cmake", "ninja", "c_compiler", "cxx_compiler", "sha256sum", "tar"},
        )
        self.assertTrue(all(details["tool_versions"].values()))
        self.assertIn(("git", "fsck", "--full"), commands)
        self.assertIn(("git", "remote", "get-url", "origin"), commands)
        self.assertIn(
            ("gh", "auth", "status", "--hostname", "github.com"), commands
        )
        self.assertIn(("gh", "release", "verify", "--help"), commands)
        for release in self.registry["releases"]:
            self.assertIn(("git", "rev-parse", release["tag"]), commands)
            self.assertIn(("git", "rev-parse", f"{release['tag']}^{{}}"), commands)
        forbidden = ("download", "upload", "create", "delete", "latest", "fetch")
        for argv, cwd in self.runner.calls:
            self.assertEqual(cwd, REPO_ROOT)
            self.assertFalse(any(word in forbidden for word in argv), argv)
            self.assertTrue(all(isinstance(item, str) for item in argv))

        state = json.loads(self.instance.state_path.read_text(encoding="utf-8"))
        self.assertEqual(state["overall"], "OPEN")
        self.assertEqual(state["steps"]["preflight"]["status"], "PASS")

    def test_preflight_refresh_invalidates_prior_metadata_before_fallible_work(self) -> None:
        found = lambda name: f"/test-tools/{name}"
        disk = SimpleNamespace(
            total=100 * 1024**3,
            used=10 * 1024**3,
            free=90 * 1024**3,
        )
        with (
            mock.patch.object(recovery.sys, "version_info", Python312()),
            mock.patch.object(recovery.sys, "version", "3.12.0 (test)"),
            mock.patch.object(recovery.shutil, "which", side_effect=found),
            mock.patch.object(recovery.shutil, "disk_usage", return_value=disk),
        ):
            self.instance.run_metadata()

        prior = self.instance._load_state()
        self.assertEqual(prior["steps"]["metadata"]["status"], "PASS")

        with (
            mock.patch.object(recovery.sys, "version_info", Python312()),
            mock.patch.object(recovery.sys, "version", "3.12.0 (test)"),
            mock.patch.object(recovery.shutil, "which", side_effect=found),
            mock.patch.object(recovery.shutil, "disk_usage", return_value=disk),
            mock.patch.object(
                self.instance,
                "_command",
                side_effect=KeyboardInterrupt("simulated interruption"),
            ),
        ):
            with self.assertRaises(KeyboardInterrupt):
                self.instance.run_preflight()

        interrupted = json.loads(self.instance.state_path.read_text(encoding="utf-8"))
        self.assertEqual(interrupted["steps"]["preflight"]["status"], "RUNNING")
        self.assertNotIn("metadata", interrupted["steps"])
        self.assertEqual(self.instance._load_state(), interrupted)

        self.run_mocked_preflight()
        resumed = self.instance._load_state()
        self.assertEqual(resumed["steps"]["preflight"]["status"], "PASS")
        self.assertNotIn("metadata", resumed["steps"])

    def test_metadata_cli_dispatch_does_not_call_preflight_twice(self) -> None:
        fake = mock.Mock()
        fake.run_metadata.return_value = {"release_count": 4}
        fake._load_state.return_value = {
            "steps": {"preflight": {"details": {"head": self.EXPECTED_COMMIT}}}
        }
        argv = [
            "--registry",
            str(REGISTRY_PATH),
            "--work-root",
            str(self.base / "cli recovery"),
            "--expected-commit",
            self.EXPECTED_COMMIT,
            "run",
            "--through",
            "metadata",
        ]
        with (
            mock.patch.object(recovery, "Recovery", return_value=fake),
            mock.patch("builtins.print"),
        ):
            self.assertEqual(recovery.main(argv), 0)

        fake.run_metadata.assert_called_once_with()
        fake.run_preflight.assert_not_called()
        fake._load_state.assert_called_once_with()

    def test_metadata_rechecks_all_four_releases_and_receipt_remains_open(self) -> None:
        found = lambda name: f"/test-tools/{name}"
        disk = SimpleNamespace(
            total=100 * 1024**3,
            used=10 * 1024**3,
            free=90 * 1024**3,
        )
        with (
            mock.patch.object(recovery.sys, "version_info", Python312()),
            mock.patch.object(recovery.sys, "version", "3.12.0 (test)"),
            mock.patch.object(recovery.shutil, "which", side_effect=found),
            mock.patch.object(recovery.shutil, "disk_usage", return_value=disk),
        ):
            first = self.instance.run_metadata()
            first_repository = json.loads(
                (self.work_root / "metadata" / "repository.json").read_text(
                    encoding="utf-8"
                )
            )
            second = self.instance.run_metadata()
            second_repository = json.loads(
                (self.work_root / "metadata" / "repository.json").read_text(
                    encoding="utf-8"
                )
            )
            shutil.rmtree(self.work_root / "metadata")
            receipt_path = self.instance.write_receipt()
        receipt = json.loads(receipt_path.read_text(encoding="utf-8"))

        self.assertEqual(first["release_count"], 4)
        self.assertEqual(first["asset_count"], 52)
        self.assertEqual(first["asset_bytes"], 24_696_192_820)
        self.assertEqual(second["fresh_pc_acceptance"], "OPEN")
        self.assertGreater(
            second_repository["generation"], first_repository["generation"]
        )
        self.assertEqual(receipt["overall_fresh_pc_recovery"], "OPEN")
        self.assertEqual(receipt["metadata_only_status"], "PASS")
        self.assertTrue((self.work_root / "metadata" / "repository.json").is_file())
        self.assertFalse(receipt["bulk_payload_downloaded"])
        self.assertGreater(len(receipt["not_run"]), 0)

        metadata_calls = [
            argv
            for argv, _ in self.runner.calls
            if argv[:2] == ("gh", "api")
            or (
                argv[:3] == ("gh", "release", "verify")
                and argv[-2:] == ("--format", "json")
            )
        ]
        # Two explicit checks plus receipt issuance; every receipt refreshes the
        # live metadata/attestation proof instead of trusting stale state.
        self.assertEqual(len(metadata_calls), 27)
        for argv in metadata_calls:
            self.assertNotIn("download", argv)
            self.assertNotIn("latest", argv)

    def test_existing_state_with_wrong_registry_or_commit_is_refused(self) -> None:
        self.instance._initialize_root()
        clean = self.instance._load_state()
        cases = {
            "registry_sha256": "f" * 64,
            "expected_commit": "e" * 40,
        }
        for field, value in cases.items():
            with self.subTest(field=field):
                changed = copy.deepcopy(clean)
                changed[field] = value
                recovery.atomic_write_json(self.instance.state_path, changed)
                with self.assertRaises(recovery.UsageSafetyError):
                    self.instance._load_state()

    def test_recovery_refuses_an_external_registry_even_with_valid_contents(self) -> None:
        external_registry = self.base / "copied-registry.json"
        external_registry.write_bytes(REGISTRY_PATH.read_bytes())
        with self.assertRaises(recovery.UsageSafetyError):
            recovery.Recovery(
                external_registry,
                self.base / "other-work-root",
                self.EXPECTED_COMMIT,
                repo_root=REPO_ROOT,
                runner=self.runner,
            )

    def test_original_asset_verifier_uses_exact_argv_with_path_spaces(self) -> None:
        asset_directory = self.base / "downloaded release assets"
        asset_directory.mkdir()

        with (
            mock.patch.object(recovery.sys, "version_info", Python312()),
            mock.patch.object(recovery.sys, "version", "3.12.0 (test)"),
            mock.patch.object(
                recovery.shutil, "which", side_effect=lambda name: f"/test-tools/{name}"
            ),
            mock.patch.object(
                recovery.shutil,
                "disk_usage",
                return_value=SimpleNamespace(
                    total=100 * 1024**3,
                    used=10 * 1024**3,
                    free=90 * 1024**3,
                ),
            ),
        ):
            result = self.instance.verify_original_assets(asset_directory)

        expected_argv = (
            "pwsh",
            "-NoLogo",
            "-NoProfile",
            "-NonInteractive",
            "-File",
            str(REPO_ROOT / "scripts" / "verify-publication-assets.ps1"),
            "-AssetDirectory",
            str(asset_directory),
            "-ManifestPath",
            str(REPO_ROOT / "docs" / "publication" / "release-manifest.json"),
        )
        self.assertEqual(self.runner.calls[-1], (expected_argv, REPO_ROOT))
        self.assertIn(
            (("git", "status", "--porcelain=v1", "--untracked-files=all"), REPO_ROOT),
            self.runner.calls,
        )
        self.assertIn((("git", "fsck", "--full"), REPO_ROOT), self.runner.calls)
        self.assertEqual(result["status"], "PASS")
        self.assertNotIn("download", expected_argv)
        self.assertNotIn("latest", expected_argv)

    def test_forged_passing_state_is_rejected(self) -> None:
        self.instance._initialize_root()
        state = self.instance._load_state()
        state["steps"] = {
            "preflight": {
                "status": "PASS",
                "recorded_at_utc": "2026-08-13T00:00:00Z",
                "details": {},
            },
            "metadata": {
                "status": "PASS",
                "recorded_at_utc": "2026-08-13T00:00:01Z",
                "details": {
                    "repository": "JCFrags/HaloFPX",
                    "release_count": 4,
                    "asset_count": 52,
                    "asset_bytes": 24_696_192_820,
                    "bulk_payload_downloaded": False,
                    "fresh_pc_acceptance": "OPEN",
                    "releases": [],
                },
            },
        }
        recovery.atomic_write_json(self.instance.state_path, state)
        with self.assertRaises(recovery.UsageSafetyError):
            self.instance._load_state()

    def test_changed_verifier_is_refused_before_execution(self) -> None:
        asset_directory = self.base / "exact release files"
        asset_directory.mkdir()
        found = lambda name: f"/test-tools/{name}"
        disk = SimpleNamespace(
            total=100 * 1024**3,
            used=10 * 1024**3,
            free=90 * 1024**3,
        )
        with (
            mock.patch.object(recovery.sys, "version_info", Python312()),
            mock.patch.object(recovery.sys, "version", "3.12.0 (test)"),
            mock.patch.object(recovery.shutil, "which", side_effect=found),
            mock.patch.object(recovery.shutil, "disk_usage", return_value=disk),
            mock.patch.object(recovery, "_sha256_file", return_value="f" * 64),
        ):
            with self.assertRaises(recovery.RecoveryError):
                self.instance.verify_original_assets(asset_directory)

        verifier_commands = [
            argv
            for argv, _ in self.runner.calls
            if "-File" in argv
            and str(REPO_ROOT / "scripts" / "verify-publication-assets.ps1") in argv
        ]
        self.assertEqual(verifier_commands, [])


if __name__ == "__main__":
    unittest.main()
