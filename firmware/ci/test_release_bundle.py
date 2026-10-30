import hashlib
import json
import os
from pathlib import Path
import struct
import tempfile
import unittest
from unittest.mock import patch
import zlib

from release_bundle import APP_START, QMK_COMMIT, KEYMAP, digest_file, release_notes, validate_binary, validate_hex, verify, version_from_source


VERSION = "0.4.8-qmk"
TAG = f"v{VERSION}"
SOURCE_COMMIT = "a" * 40
ENVIRONMENT = {"GITHUB_REPOSITORY": "owner/keyboard", "GITHUB_RUN_ID": "123", "GITHUB_SHA": "b" * 40}


def dfu_image(payload):
    body = payload + struct.pack("<HHHH3sB", 0xFFFF, 0xFFFF, 0xFFFF, 0x0100, b"UFD", 16)
    return body + struct.pack("<I", zlib.crc32(body) ^ 0xFFFFFFFF)


def binary_fixture():
    payload = bytearray(256)
    struct.pack_into("<II", payload, 0, 0x20000400, APP_START + 9)
    expected = VERSION.encode() + b"VIA WAITTING" + bytes.fromhex("3a3060830600ff0901a10185060660ff0961a1010962")
    payload[16:16 + len(expected)] = expected
    return dfu_image(payload)


def hex_record(address, kind, payload):
    record = bytes([len(payload)]) + address.to_bytes(2, "big") + bytes([kind]) + payload
    return ":" + (record + bytes([-sum(record) % 256])).hex().upper()


def hex_fixture(binary):
    payload = binary[:-16]
    records = [hex_record(0, 4, b"\x08\x00")]
    for offset in range(0, len(payload), 16):
        records.append(hex_record(0x2000 + offset, 0, payload[offset:offset + 16]))
    records.append(hex_record(0, 5, (APP_START + 9).to_bytes(4, "big")))
    records.append(hex_record(0, 1, b""))
    return "\n".join(records) + "\n"


class ReleaseBundleTests(unittest.TestCase):
    def setUp(self):
        self.scratch = tempfile.TemporaryDirectory(prefix="kb16-bundle-tests-")
        self.addCleanup(self.scratch.cleanup)
        self.root = Path(self.scratch.name)
        self.bundle = self.root / "dist"
        self.bundle.mkdir()
        prefix = f"doio_kb16_rev2_codex_micro_{TAG}"
        binary = binary_fixture()
        (self.bundle / f"{prefix}.bin").write_bytes(binary)
        (self.bundle / f"{prefix}.hex").write_text(hex_fixture(binary))
        (self.bundle / "kb16_codex_via.json").write_text("{}")
        self.manifest = {
            "version": VERSION, "release_tag": TAG, "source_commit": SOURCE_COMMIT,
            "qmk_commit": QMK_COMMIT, "automation_commit": ENVIRONMENT["GITHUB_SHA"],
            "repository": ENVIRONMENT["GITHUB_REPOSITORY"], "run_id": ENVIRONMENT["GITHUB_RUN_ID"],
            "build_system": "github-actions", "run_url": "https://github.com/owner/keyboard/actions/runs/123",
            "compiler": "Arm GNU Toolchain 15.3.Rel2",
            "assets": [digest_file(filename) for filename in sorted(self.bundle.iterdir())],
        }
        self.refresh_manifest()

    def refresh_manifest(self):
        (self.bundle / "BUILD-INFO.json").write_text(json.dumps(self.manifest))
        assets = [digest_file(filename) for filename in sorted(self.bundle.iterdir()) if filename.name != "SHA256SUMS"]
        (self.bundle / "SHA256SUMS").write_text("".join(f"{entry['sha256']}  {entry['name']}\n" for entry in assets))

    def verify_bundle(self):
        with patch.dict(os.environ, ENVIRONMENT):
            return verify(self.bundle, TAG, SOURCE_COMMIT)

    def test_matching_version_and_rejected_tag(self):
        directory = self.root / KEYMAP
        directory.mkdir(parents=True)
        (directory / "codex_micro_protocol.h").write_text(f'#define CODEX_MICRO_VERSION "{VERSION}"\n')
        self.assertEqual(version_from_source(self.root, TAG), VERSION)
        with self.assertRaises(ValueError):
            version_from_source(self.root, "v9.9.9-qmk")
        with self.assertRaises(ValueError):
            version_from_source(self.root, "../../other")

    def test_valid_binary_and_hex(self):
        data = binary_fixture()
        validate_binary(data, VERSION)
        validate_hex(hex_fixture(data), data)

    def test_reject_corrupt_crc(self):
        data = bytearray(binary_fixture())
        data[100] ^= 1
        with self.assertRaises(ValueError):
            validate_binary(data, VERSION)

    def test_reject_bootloader_vector_and_missing_descriptor(self):
        for replacement in (0x08000001, APP_START + 0x10001):
            data = bytearray(binary_fixture()[:-16])
            struct.pack_into("<I", data, 4, replacement)
            with self.assertRaises(ValueError):
                validate_binary(dfu_image(data), VERSION)
        data = binary_fixture()[:-16].replace(b"VIA WAITTING", b"not awaiting")
        with self.assertRaises(ValueError):
            validate_binary(dfu_image(data), VERSION)

    def test_reject_bad_hex_checksum_and_bootloader_write(self):
        data = binary_fixture()
        with self.assertRaises(ValueError):
            validate_hex(hex_fixture(data).replace(":020000040800F2", ":02000004080000"), data)
        text = hex_record(0, 4, b"\x08\x00") + "\n" + hex_record(0, 0, b"\x00") + "\n" + hex_record(0, 1, b"")
        with self.assertRaises(ValueError):
            validate_hex(text, data)

    def test_reject_different_hex_payload(self):
        data = bytearray(binary_fixture()[:-16])
        data[100] ^= 1
        with self.assertRaises(ValueError):
            validate_hex(hex_fixture(dfu_image(data)), binary_fixture())

    def test_verify_exact_pipeline_bundle(self):
        self.assertEqual(self.verify_bundle()["source_commit"], SOURCE_COMMIT)

    def test_reject_local_build(self):
        self.manifest["build_system"] = "local-validation"
        self.refresh_manifest()
        with self.assertRaises(ValueError):
            self.verify_bundle()

    def test_reject_other_run_and_source(self):
        for field in ("source_commit", "qmk_commit", "repository", "run_id", "automation_commit"):
            with self.subTest(field=field):
                original = self.manifest[field]
                self.manifest[field] = "other"
                self.refresh_manifest()
                with self.assertRaises(ValueError):
                    self.verify_bundle()
                self.manifest[field] = original

    def test_reject_tampered_asset(self):
        (self.bundle / "kb16_codex_via.json").write_text("tampered")
        with self.assertRaises(ValueError):
            self.verify_bundle()

    def test_reject_extra_file(self):
        (self.bundle / "extra.txt").write_text("unexpected")
        with self.assertRaises(ValueError):
            self.verify_bundle()

    def test_reject_path_traversal(self):
        records = (self.bundle / "SHA256SUMS").read_text().splitlines()
        records[0] = f"{hashlib.sha256(b'').hexdigest()}  ../private"
        (self.bundle / "SHA256SUMS").write_text("\n".join(records) + "\n")
        with self.assertRaises(ValueError):
            self.verify_bundle()

    def test_release_notes_reference_actual_build(self):
        notes = release_notes(self.manifest, "# Release\n\n10 秒 / 1 秒\n")
        self.assertIn(self.manifest["run_url"], notes)
        self.assertIn(SOURCE_COMMIT, notes)
        self.assertIn(self.manifest["assets"][0]["sha256"], notes)
        self.assertTrue(notes.endswith("10 秒 / 1 秒\n"))


if __name__ == "__main__":
    unittest.main()
