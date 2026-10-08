#!/usr/bin/env python3
"""Normalize Elgato CLI packages to classic ZIP32 for Stream Deck Windows installs.

The CLI has been observed writing ZIP64 headers for tiny (<1 MB) plugins.
Some Stream Deck installers respond with the misleading "requires 0.0 or later"
message when they cannot read the package metadata. Do not change the SDK
minimum requirement to bypass this error: preserve the manifest and payload.
"""
from __future__ import annotations

import argparse
import hashlib
import json
import os
import tempfile
import zipfile
from pathlib import Path

PLUGIN_FOLDER = "com.himothee.studio.sdPlugin"
MANIFEST = f"{PLUGIN_FOLDER}/manifest.json"


def assert_classic_package(path: Path) -> None:
    with zipfile.ZipFile(path) as archive:
        names = archive.namelist()
        if not names or len(names) != len(set(names)):
            raise RuntimeError(f"{path.name}: archive is empty or contains duplicate names")
        if MANIFEST not in names or f"{PLUGIN_FOLDER}/bin/plugin.js" not in names:
            raise RuntimeError(f"{path.name}: required manifest or entrypoint missing")
        if archive.testzip() is not None:
            raise RuntimeError(f"{path.name}: CRC verification failed")
        for info in archive.infolist():
            if info.filename.startswith("/") or ".." in Path(info.filename).parts:
                raise RuntimeError(f"{path.name}: unsafe archive path: {info.filename}")
            if not info.filename.startswith(f"{PLUGIN_FOLDER}/"):
                raise RuntimeError(f"{path.name}: unexpected root: {info.filename}")
            if info.extract_version >= 45 or has_zip64_extra(info.extra):
                raise RuntimeError(f"{path.name}: ZIP64 entry: {info.filename}")
        manifest = json.loads(archive.read(MANIFEST))
        required_version = manifest.get("Software", {}).get("MinimumVersion")
        if required_version != "7.1":
            raise RuntimeError(f"{path.name}: unexpected Stream Deck minimum: {required_version}")


def has_zip64_extra(extra: bytes) -> bool:
    offset = 0
    while offset + 4 <= len(extra):
        header_id = int.from_bytes(extra[offset : offset + 2], "little")
        length = int.from_bytes(extra[offset + 2 : offset + 4], "little")
        if header_id == 0x0001:
            return True
        offset += 4 + length
    return False


def repack(path: Path) -> None:
    fd, temp_name = tempfile.mkstemp(prefix="himo-zip32-", suffix=".streamDeckPlugin", dir=path.parent)
    os.close(fd)
    temp_path = Path(temp_name)
    try:
        with zipfile.ZipFile(path) as src, zipfile.ZipFile(
            temp_path, "w", compression=zipfile.ZIP_DEFLATED,
            compresslevel=7, allowZip64=False,
        ) as dst:
            for info in src.infolist():
                if info.is_dir():
                    continue
                dst.writestr(info.filename, src.read(info.filename),
                             compress_type=zipfile.ZIP_DEFLATED, compresslevel=7)
        # Verify every byte of every extracted file is unchanged.
        with zipfile.ZipFile(path) as original, zipfile.ZipFile(temp_path) as fixed:
            if original.namelist() != fixed.namelist():
                raise RuntimeError("Repacked filenames differ")
            for name in original.namelist():
                a = hashlib.sha256(original.read(name)).digest()
                b = hashlib.sha256(fixed.read(name)).digest()
                if a != b:
                    raise RuntimeError(f"Repacked payload differs: {name}")
        assert_classic_package(temp_path)
        temp_path.replace(path)
        print(f"ZIP32 package verified: {path}")
    finally:
        temp_path.unlink(missing_ok=True)


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("--verify", action="store_true", help="Check packages without rewriting")
    parser.add_argument("--directory", type=Path,
                        default=Path(__file__).resolve().parents[1] / "dist")
    args = parser.parse_args()
    packages = sorted(args.directory.glob("*.streamDeckPlugin"))
    if not packages:
        parser.error(f"No .streamDeckPlugin packages found in {args.directory}")
    for path in packages:
        if not args.verify:
            repack(path)
        else:
            assert_classic_package(path)
            print(f"ZIP32 package verified: {path}")


if __name__ == "__main__":
    main()
