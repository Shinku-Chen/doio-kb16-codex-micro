import argparse
from datetime import datetime, timezone
import hashlib
import json
import os
from pathlib import Path
import re
import shutil
import struct
import subprocess
import sys
import zlib


QMK_COMMIT = "f5db962110615fd263c3ebf9f0402e7c2b08fbe4"
KEYMAP = Path("firmware/keymaps/codex_micro_via_v1")
APP_START = 0x08002000
FLASH_END = 0x08020000


def checked(condition, message):
    if not condition:
        raise ValueError(message)


def version_from_source(source, tag):
    header = (source / KEYMAP / "codex_micro_protocol.h").read_text()
    matched = re.search(r'^#define CODEX_MICRO_VERSION "(\d+\.\d+\.\d+-qmk)"$', header, re.M)
    checked(matched is not None, "无法读取固件版本")
    version = matched.group(1)
    checked(not tag or tag == f"v{version}", "发布标签必须与固件版本完全一致")
    return version


def validate_binary(data, version):
    checked(24 <= len(data) <= FLASH_END - APP_START + 16, "BIN 大小超出应用区")
    signature, length, checksum = struct.unpack_from("<3sBI", data, len(data) - 8)
    checked(signature == b"UFD" and length == 16, "缺少正确的 DFU suffix")
    checked(checksum == zlib.crc32(data[:-4]) ^ 0xFFFFFFFF, "DFU CRC 不匹配")
    stack, reset = struct.unpack_from("<II", data)
    checked(0x20000000 <= stack <= 0x20005000 and stack % 8 == 0, "初始栈地址无效")
    checked(reset & 1 and APP_START <= (reset & ~1) < APP_START + len(data) - 16, "复位向量不在应用区")
    for expected in (version.encode(), b"VIA WAITTING", bytes.fromhex("3a306083"), bytes.fromhex("0600ff0901a1018506"), bytes.fromhex("0660ff0961a1010962")):
        checked(expected in data, f"固件缺少版本、启动文字或 USB 描述符：{expected!r}")


def validate_hex(text, binary):
    memory = {}
    base = 0
    finished = False
    for line in text.splitlines():
        checked(line.startswith(":"), "HEX 记录格式无效")
        record = bytes.fromhex(line[1:])
        checked(not finished and len(record) >= 5 and len(record) == record[0] + 5 and sum(record) % 256 == 0, "HEX 长度、校验或 EOF 无效")
        length = record[0]
        address = int.from_bytes(record[1:3], "big")
        kind = record[3]
        payload = record[4:-1]
        if kind == 0:
            for offset, value in enumerate(payload):
                absolute = base + address + offset
                checked(APP_START <= absolute < FLASH_END and absolute not in memory, "HEX 越界或重叠，禁止覆盖 Bootloader")
                memory[absolute] = value
        elif kind == 4:
            checked(length == 2 and address == 0, "HEX 线性地址无效")
            base = int.from_bytes(payload, "big") << 16
        elif kind == 5:
            checked(length == 4 and address == 0, "HEX 入口记录无效")
        elif kind == 1:
            checked(length == 0 and address == 0, "HEX EOF 无效")
            finished = True
        else:
            raise ValueError("不支持的 HEX 记录类型")
    checked(finished and bool(memory), "HEX 缺少数据或 EOF")
    checked(min(memory) == APP_START, "HEX 应用镜像起址错误")
    image = bytes(memory.get(address, 0) for address in range(APP_START, max(memory) + 1))
    payload = binary[:-16]
    checked(payload.startswith(image) and len(payload) - len(image) < 4, "HEX 与 BIN 应用镜像不一致")


def digest_file(filename):
    data = filename.read_bytes()
    return {"name": filename.name, "bytes": len(data), "sha256": hashlib.sha256(data).hexdigest()}


def command(*arguments):
    return subprocess.check_output(arguments, text=True).strip()


def package(source, qmk, output, tag):
    version = version_from_source(source, tag)
    checked(command("git", "-C", str(qmk), "rev-parse", "HEAD") == QMK_COMMIT, "QMK 基线不匹配")
    binary = qmk / ".build/doio_kb16_rev2_codex_micro_via_v1.bin"
    hexadecimal = binary.with_suffix(".hex")
    validate_binary(binary.read_bytes(), version)
    validate_hex(hexadecimal.read_text(), binary.read_bytes())
    output.mkdir(parents=True, exist_ok=True)
    checked(not any(output.iterdir()), "打包目录必须为空，防止混入旧构建")
    prefix = f"doio_kb16_rev2_codex_micro_v{version}"
    shutil.copyfile(binary, output / f"{prefix}.bin")
    shutil.copyfile(hexadecimal, output / f"{prefix}.hex")
    shutil.copyfile(source / "firmware/via/kb16_codex_via.json", output / "kb16_codex_via.json")
    definition = json.loads((output / "kb16_codex_via.json").read_text())
    checked(definition["vendorId"] == "0xD010" and definition["productId"] == "0x1601" and definition["matrix"] == {"rows": 4, "cols": 5}, "VIA 定义不匹配")
    assets = [digest_file(filename) for filename in sorted(output.iterdir())]
    manifest = {
        "version": version,
        "release_tag": tag,
        "source_commit": command("git", "-C", str(source), "rev-parse", "HEAD"),
        "qmk_commit": QMK_COMMIT,
        "automation_commit": os.environ.get("GITHUB_SHA", ""),
        "build_system": "github-actions" if os.environ.get("GITHUB_ACTIONS") == "true" else "local-validation",
        "repository": os.environ.get("GITHUB_REPOSITORY", ""),
        "run_id": os.environ.get("GITHUB_RUN_ID", ""),
        "run_attempt": os.environ.get("GITHUB_RUN_ATTEMPT", ""),
        "run_url": f"https://github.com/{os.environ.get('GITHUB_REPOSITORY', '')}/actions/runs/{os.environ.get('GITHUB_RUN_ID', '')}",
        "built_at": datetime.now(timezone.utc).isoformat(),
        "compiler": command("arm-none-eabi-gcc", "--version").splitlines()[0],
        "qmk_cli": command("qmk", "--version"),
        "python": sys.version,
        "python_packages": command(sys.executable, "-m", "pip", "freeze").splitlines(),
        "runner_image": os.environ.get("ImageVersion", ""),
        "toolchain_archive_sha256": os.environ.get("ARM_ARCHIVE_SHA256", ""),
        "assets": assets,
    }
    (output / "BUILD-INFO.json").write_text(json.dumps(manifest, ensure_ascii=False, indent=2) + "\n")
    sums = [digest_file(filename) for filename in sorted(output.iterdir())]
    (output / "SHA256SUMS").write_text("".join(f"{entry['sha256']}  {entry['name']}\n" for entry in sums))
    if os.environ.get("GITHUB_OUTPUT"):
        with Path(os.environ["GITHUB_OUTPUT"]).open("a") as stream:
            stream.write(f"release_tag={tag}\nsource_commit={manifest['source_commit']}\nartifact_name=firmware-{version}-{manifest['source_commit'][:12]}\n")
    print(json.dumps(manifest, ensure_ascii=False, indent=2))


def verify(bundle, tag, source_commit):
    manifest = json.loads((bundle / "BUILD-INFO.json").read_text())
    checked(tag == f"v{manifest['version']}" and manifest["release_tag"] == tag, "附件版本与标签不匹配")
    checked(manifest["source_commit"] == source_commit and manifest["qmk_commit"] == QMK_COMMIT, "附件源码或 QMK 提交不匹配")
    checked(manifest["build_system"] == "github-actions", "禁止发布本地生成的附件")
    for environment, field in (("GITHUB_REPOSITORY", "repository"), ("GITHUB_RUN_ID", "run_id"), ("GITHUB_SHA", "automation_commit")):
        checked(bool(os.environ.get(environment)) and manifest[field] == os.environ[environment], "附件不是当前仓库、当前流水线生成")
    prefix = f"doio_kb16_rev2_codex_micro_{tag}"
    expected = {f"{prefix}.bin", f"{prefix}.hex", "kb16_codex_via.json", "BUILD-INFO.json"}
    checked({filename.name for filename in bundle.iterdir()} == expected | {"SHA256SUMS"}, "附件不完整或混入意外文件")
    records = (bundle / "SHA256SUMS").read_text().splitlines()
    checked(len(records) == len(expected), "校验表数量不匹配")
    found = set()
    for line in records:
        matched = re.fullmatch(r"([a-f0-9]{64})  ([A-Za-z0-9_.-]+)", line)
        checked(matched is not None and matched[2] in expected and matched[2] not in found, "校验表文件名无效或重复")
        found.add(matched[2])
        checked(digest_file(bundle / matched[2])["sha256"] == matched[1], "附件 SHA-256 校验失败")
    checked(manifest["assets"] == [digest_file(bundle / name) for name in sorted(expected - {"BUILD-INFO.json"})], "构建记录与实际附件不一致")
    validate_binary((bundle / f"{prefix}.bin").read_bytes(), manifest["version"])
    validate_hex((bundle / f"{prefix}.hex").read_text(), (bundle / f"{prefix}.bin").read_bytes())
    return manifest


def release_notes(manifest, base):
    heading = (
        "## GitHub Actions 构建 / CI build\n\n"
        f"- 构建记录 / Workflow: {manifest['run_url']}\n"
        f"- 固件源码 / Firmware commit: `{manifest['source_commit']}`\n"
        f"- 构建脚本 / Automation commit: `{manifest['automation_commit']}`\n"
        f"- QMK: `{manifest['qmk_commit']}`\n"
        f"- Compiler: `{manifest['compiler']}`\n\n"
        "本 Release 附件由该流水线从标签源码生成，不是本地编译上传。校验和以附件 SHA256SUMS 为准；BUILD-INFO.json 包含构建来源。本次未刷机。\n\n"
        "Assets are built from the tagged firmware source by this workflow, not uploaded from a local build. See SHA256SUMS and BUILD-INFO.json. No hardware flashing was performed.\n\n"
        "| Asset | Bytes | SHA-256 |\n|---|---|---|\n"
    )
    for entry in manifest["assets"]:
        heading += f"| `{entry['name']}` | {entry['bytes']} | `{entry['sha256']}` |\n"
    return heading + "\n---\n\n" + base


def main():
    parser = argparse.ArgumentParser()
    commands = parser.add_subparsers(dest="command", required=True)
    pack = commands.add_parser("package")
    for option in ("source", "qmk", "output"):
        pack.add_argument(f"--{option}", required=True, type=Path)
    pack.add_argument("--tag", default="")
    check = commands.add_parser("verify")
    for option in ("bundle", "notes", "output"):
        check.add_argument(f"--{option}", required=True, type=Path)
    check.add_argument("--tag", required=True)
    check.add_argument("--source-commit", required=True)
    arguments = parser.parse_args()
    if arguments.command == "package":
        package(arguments.source, arguments.qmk, arguments.output, arguments.tag)
    else:
        manifest = verify(arguments.bundle, arguments.tag, arguments.source_commit)
        base = arguments.notes.read_text() if arguments.notes.is_file() else f"# {arguments.tag}\n"
        arguments.output.write_text(release_notes(manifest, base))


if __name__ == "__main__":
    main()
