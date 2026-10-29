#!/usr/bin/env python3
"""Install the user's Mac game data and the existing native development build."""
import argparse
import ctypes
import errno
import hashlib
import json
import os
from pathlib import Path
import plistlib
import re
import shutil
import struct
import subprocess
import tempfile
from language_compat import install_index_compatibility

PROJECT = Path(__file__).resolve().parent.parent
APP_NAME = "reCM 01-02.app"
LEGACY_APP_NAME = "CM 01-02 Native Prototype.app"
APP_ID = "local.cm0102.native-prototype"
GAME_SHA256 = "91692f95df17cf851e7c1d6dbdecd0bd0a5809ce1937133e209aee859a6b516b"
RESOURCE_SHA256 = "1f4ae4f499d43f163b9153026939b19a2ad022a8b4181690ba7b1b49cf5e551c"


def digest(path):
    hasher = hashlib.sha256()
    with path.open("rb") as stream:
        for block in iter(lambda: stream.read(1024 * 1024), b""):
            hasher.update(block)
    return hasher.hexdigest()


def finder_info(path):
    # Python's os xattr helpers are Linux-only; use Darwin's native API.
    getxattr = ctypes.CDLL(None, use_errno=True).getxattr
    getxattr.argtypes = [ctypes.c_char_p, ctypes.c_char_p, ctypes.c_void_p,
                        ctypes.c_size_t, ctypes.c_uint32, ctypes.c_int]
    getxattr.restype = ctypes.c_ssize_t
    buffer = ctypes.create_string_buffer(32)
    count = getxattr(os.fsencode(path), b"com.apple.FinderInfo", buffer, len(buffer), 0, 0)
    if count < 0:
        code = ctypes.get_errno()
        if code == errno.ENOATTR:
            return None
        raise OSError(code, os.strerror(code), path)
    if count != 32:
        raise RuntimeError(f"Unexpected Finder metadata length: {path}")
    return buffer.raw.hex()


def inventory(root):
    result = {}
    for path in sorted(root.rglob("*")):
        if path.is_symlink():
            raise RuntimeError(f"Installation source contains a symbolic link: {path}")
        if not path.is_file():
            continue
        # Finder's transient folder-view preferences are not game data.
        if path.name == ".DS_Store":
            continue
        forks = path / "..namedfork/rsrc"
        result[path.relative_to(root).as_posix()] = {
            "bytes": path.stat().st_size,
            "sha256": digest(path),
            "resourceSHA256": digest(forks) if forks.exists() and forks.stat().st_size else None,
            "finderInfo": finder_info(path),
        }
    return result


def check_game(root):
    executable = root / "CM 01-02"
    if not executable.is_file() or digest(executable) != GAME_SHA256:
        raise RuntimeError("The game executable does not match this native build's original Mac release.")
    fork = executable / "..namedfork/rsrc"
    if not fork.exists() or digest(fork) != RESOURCE_SHA256:
        raise RuntimeError("The original game resource fork is missing or does not match this build.")
    if not (root / "Data").is_dir():
        raise RuntimeError("The original Data directory is missing.")


def copy_mac(source, destination):
    subprocess.run(["/usr/bin/ditto", "--rsrc", "--extattr", str(source), str(destination)], check=True)


def extra_files(source):
    extras = source.parent / "Extras"
    if not (extras / "qpr for the cup").is_file():
        raise RuntimeError("The original CD Extras are needed for installation. Mount CM-01-02.cdr and try again.")
    return extras, {"Extras/" + name: data for name, data in inventory(extras).items()}


def check_app(path):
    with (path / "Contents/Info.plist").open("rb") as stream:
        info = plistlib.load(stream)
    if info.get("CFBundleIdentifier") != APP_ID:
        raise RuntimeError(f"Refusing to replace an unrelated application: {path}")
    executable = path / "Contents/MacOS" / info["CFBundleExecutable"]
    architectures = subprocess.check_output(["/usr/bin/lipo", "-archs", str(executable)], text=True).split()
    if architectures != ["arm64"]:
        raise RuntimeError("The application must contain the native ARM64 build.")


def install_icon(game, resources, stage):
    # The original Mac application stores its icon in a classic resource fork.
    listing = subprocess.check_output(
        ["/usr/bin/DeRez", "-only", "ICN#", "-only", "icl8", str(game / "CM 01-02")],
        text=True, errors="replace")
    chunks = []
    for kind, expected in (("ICN#", 256), ("icl8", 1024)):
        match = re.search(r"data '" + re.escape(kind) + r"' \(128(?:,.*?)?\) \{(.*?)\n\};", listing, re.S)
        if not match:
            raise RuntimeError(f"Original application icon resource {kind} is missing.")
        data = bytes.fromhex("".join(re.findall(r'\$"([0-9A-Fa-f ]+)"', match.group(1))))
        if len(data) != expected:
            raise RuntimeError(f"Original application icon resource {kind} has an unexpected size.")
        chunks.append(kind.encode("ascii") + struct.pack(">I", len(data) + 8) + data)
    classic = stage / "original.icns"
    classic.write_bytes(b"icns" + struct.pack(">I", 8 + sum(map(len, chunks))) + b"".join(chunks))
    png = stage / "original.png"
    subprocess.run(["/usr/bin/sips", "-s", "format", "png", str(classic), "--out", str(png)],
                   check=True, capture_output=True)
    iconset = stage / "AppIcon.iconset"
    iconset.mkdir()
    for size in (16, 32, 128, 256, 512):
        for scale in (1, 2):
            filename = f"icon_{size}x{size}" + ("@2x" if scale == 2 else "") + ".png"
            subprocess.run(["/usr/bin/sips", "-z", str(size * scale), str(size * scale),
                            str(png), "--out", str(iconset / filename)],
                           check=True, capture_output=True)
    subprocess.run(["/usr/bin/iconutil", "-c", "icns", str(iconset), "-o", str(resources / "AppIcon.icns")],
                   check=True, capture_output=True)


def install(source, support, applications, built_app=None):
    destination = applications / APP_NAME
    legacy = applications / LEGACY_APP_NAME
    for app in (destination, legacy):
        running = subprocess.run(["/usr/bin/pgrep", "-f", str(app / "Contents/MacOS")],
                                 capture_output=True, text=True)
        if running.returncode not in (0, 1):
            raise RuntimeError("Cannot check whether the game is running.")
        if running.returncode == 0:
            raise RuntimeError("Quit the game before installing a new build.")
    built_app = built_app or PROJECT / ".local/current/build" / APP_NAME
    check_app(built_app)
    program = PROJECT / ".local/current/export"
    program_files = [program / "program.json", *sorted(program.glob("segment-*.bin"))]
    if not program_files[0].is_file() or len(program_files) < 2:
        raise RuntimeError("The local AOT program data is missing. Build the native prototype first.")
    if support.is_symlink() or applications.is_symlink():
        raise RuntimeError("Installation directories must not be symbolic links.")
    support.mkdir(parents=True, exist_ok=True)
    applications.mkdir(parents=True, exist_ok=True)
    game = support / "Game"
    manifest_file = support / "installation.json"
    if game.exists():
        check_game(game)
        manifest = json.loads(manifest_file.read_text())
        if inventory(game) != manifest["files"]:
            raise RuntimeError("Installed game files have changed. They were preserved; installation was stopped.")
        if manifest.get("format") == 1:
            check_game(source)
            extras, expected_extras = extra_files(source)
            with tempfile.TemporaryDirectory(prefix=".extras-", dir=support) as stage:
                staged_extras = Path(stage) / "Extras"
                copy_mac(extras, staged_extras)
                if {"Extras/" + name: data for name, data in inventory(staged_extras).items()} != expected_extras:
                    raise RuntimeError("Copied CD Extras did not match the source.")
                staged_extras.rename(game / "Extras")
                manifest["files"].update(expected_extras)
                manifest["format"] = 2
                staged_manifest = Path(stage) / "installation.json"
                staged_manifest.write_text(json.dumps(manifest, indent=2) + "\n")
                staged_manifest.replace(manifest_file)
            print(f"Installed and verified {len(expected_extras)} original CD Extras.", flush=True)
        elif manifest.get("format") != 2:
            raise RuntimeError("Unsupported installation manifest version.")
        print("Verified the existing local game installation.", flush=True)
    else:
        check_game(source)
        expected = inventory(source)
        extras, expected_extras = extra_files(source)
        expected.update(expected_extras)
        with tempfile.TemporaryDirectory(prefix=".install-", dir=support) as stage:
            staged_game = Path(stage) / "Game"
            print("Copying original game data and Mac resource forks...", flush=True)
            copy_mac(source, staged_game)
            copy_mac(extras, staged_game / "Extras")
            if inventory(staged_game) != expected:
                raise RuntimeError("Copied game files did not match the source. Installation was stopped.")
            manifest = {"format": 2, "originalExecutableSHA256": GAME_SHA256, "files": expected}
            staged_manifest = Path(stage) / "installation.json"
            staged_manifest.write_text(json.dumps(manifest, indent=2) + "\n")
            staged_game.rename(game)
            staged_manifest.replace(manifest_file)
        print(f"Verified {len(expected)} installed files, including their resource forks.", flush=True)
    runtime = support / "Runtime"
    if not runtime.exists() and (PROJECT / ".local/runtime").is_dir():
        with tempfile.TemporaryDirectory(prefix=".preferences-", dir=support) as stage:
            copied = Path(stage) / "Runtime"
            copy_mac(PROJECT / ".local/runtime", copied)
            copied.rename(runtime)
        print("Preserved existing prototype preferences and private files.", flush=True)
    install_index_compatibility(game, runtime)
    print("Verified the private English club-index compatibility file.", flush=True)
    if destination.exists():
        check_app(destination)
    elif legacy.exists():
        check_app(legacy)
    with tempfile.TemporaryDirectory(prefix=".cm-native-", dir=applications) as stage:
        staged_app = Path(stage) / APP_NAME
        copy_mac(built_app, staged_app)
        bundled_program = staged_app / "Contents/Resources/OriginalProgram"
        bundled_program.mkdir(parents=True, exist_ok=True)
        for item in program_files:
            shutil.copy2(item, bundled_program / item.name)
            if digest(item) != digest(bundled_program / item.name):
                raise RuntimeError("The installed native program data failed verification.")
        info_path = staged_app / "Contents/Info.plist"
        with info_path.open("rb") as stream:
            info = plistlib.load(stream)
        info.update(CFBundleName="reCM 01-02", CFBundleDisplayName="reCM 01-02", CFBundleIconFile="AppIcon")
        with info_path.open("wb") as stream:
            plistlib.dump(info, stream)
        install_icon(game, staged_app / "Contents/Resources", Path(stage))
        # Seal the added program data with an ad-hoc signature.
        subprocess.run(["/usr/bin/codesign", "--force", "--deep", "--sign", "-", str(staged_app)],
                       check=True, capture_output=True)
        subprocess.run(["/usr/bin/codesign", "--verify", "--deep", "--strict", str(staged_app)],
                       check=True, capture_output=True)
        check_app(staged_app)
        previous = Path(stage) / "previous.app"
        replaced = destination if destination.exists() else legacy if legacy.exists() else None
        if replaced:
            replaced.rename(previous)
        try:
            staged_app.rename(destination)
        except BaseException:
            if previous.exists():
                previous.rename(replaced)
            raise
    print(f"Installed native application: {destination}", flush=True)
    print(f"Installed game data: {game}", flush=True)
    print("You can eject the CD image. This development build still has unfinished game services.", flush=True)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--source", type=Path, default=Path("/Volumes/CM 01-02/Championship Manager"))
    parser.add_argument("--app", type=Path, help="Use an existing native app as the installation source")
    args = parser.parse_args()
    try:
        install(args.source, Path.home() / "Library/Application Support/CM 01-02 Native", Path.home() / "Applications", args.app)
    except (OSError, ValueError, KeyError, RuntimeError, subprocess.CalledProcessError) as error:
        parser.exit(1, f"Installation stopped: {error}\n")


if __name__ == "__main__":
    main()
