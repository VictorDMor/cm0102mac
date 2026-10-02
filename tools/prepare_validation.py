#!/usr/bin/env python3
"""Prepare an isolated real-game app for UI-driven performance validation.

This tool copies files only. Launch and operate the resulting app with the UI
tools. No game data is patched, no simulation is skipped, and the installed
runtime remains untouched. Each destination must be new.
"""
import argparse
import hashlib
import json
from pathlib import Path
import plistlib
import re
import shutil
import subprocess

ROOT = Path(__file__).resolve().parent.parent
REFERENCE_SHA = "82c504f46ce54a723137dbf42c005d458a4d7b18a9abab4c9f92627aa94f08fb"


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("name", help="Unique lowercase diagnostic run name")
    parser.add_argument("--app", type=Path, default=ROOT / ".local/aot-build/CM 01-02 Native Prototype.app")
    parser.add_argument("--runtime", type=Path, default=Path.home() / "Library/Application Support/CM 01-02 Native/Runtime")
    parser.add_argument("--season-reference-days", type=int, choices=range(1, 8),
                        help="Capture 1 through 7 complete original days with private native-service inputs")
    args = parser.parse_args()
    if not re.fullmatch(r"[a-z][a-z0-9-]{0,47}", args.name):
        parser.error("Use a short lowercase name with optional digits and hyphens")
    reference = ROOT / "saves/Mac-Teste-Validado-Pos-Jogo.sav"
    if hashlib.sha256(reference.read_bytes()).hexdigest() != REFERENCE_SHA:
        parser.error("The full immutable LFS reference save is required")
    source = args.app.resolve(strict=True)
    source_runtime = args.runtime.resolve(strict=True)
    destination = ROOT / ".local/performance" / args.name
    destination.mkdir(parents=True, exist_ok=False)
    app = destination / f"CM Validation {args.name}.app"
    runtime = destination / "Runtime"
    subprocess.run(["ditto", "--rsrc", "--extattr", str(source), str(app)], check=True)
    subprocess.run(["ditto", "--rsrc", "--extattr", str(source_runtime), str(runtime)], check=True)
    program = app / "Contents/Resources/OriginalProgram"
    program.mkdir(parents=True, exist_ok=True)
    for item in [ROOT / ".local/recomp-input/program.json", *sorted((ROOT / ".local/recomp-input").glob("segment-*.bin"))]:
        shutil.copy2(item, program / item.name)
    working_save = runtime / "Files/games/Perf-Reference.sav"
    working_save.parent.mkdir(parents=True, exist_ok=True)
    shutil.copy2(reference, working_save)
    plist = app / "Contents/Info.plist"
    info = plistlib.loads(plist.read_bytes())
    source_executable = source / "Contents/MacOS" / info["CFBundleExecutable"]
    source_hash = hashlib.sha256(source_executable.read_bytes()).hexdigest()
    copied_executable = app / "Contents/MacOS" / info["CFBundleExecutable"]
    if hashlib.sha256(copied_executable.read_bytes()).hexdigest() != source_hash:
        raise RuntimeError("Copied executable differs from the selected build")
    info.update(CFBundleIdentifier=f"local.cm0102.validation.{args.name}",
                CFBundleName=f"CM Validation {args.name}",
                CFBundleDisplayName=f"CM Validation {args.name}",
                CMValidationRuntime=str(runtime))
    fixture = None
    if args.season_reference_days:
        fixture = destination / "SeasonFixture"
        fixture.mkdir(mode=0o700)
        info.update(CMSeasonReferenceDirectory=str(fixture),
                    CMSeasonReferenceDays=args.season_reference_days)
    plist.write_bytes(plistlib.dumps(info))
    subprocess.run(["codesign", "--force", "--deep", "--sign", "-", str(app)], check=True)
    executable = app / "Contents/MacOS" / info["CFBundleExecutable"]
    architecture = subprocess.check_output(["lipo", "-archs", str(executable)], text=True).strip()
    if architecture != "arm64":
        raise RuntimeError("Validation requires the native ARM64 executable")
    manifest = {"app": str(app), "runtime": str(runtime), "architecture": architecture,
                "binarySHA256": hashlib.sha256(executable.read_bytes()).hexdigest(),
                "sourceBinarySHA256": source_hash,
                "referenceSHA256": REFERENCE_SHA, "sourceRuntime": str(source_runtime),
                "note": "Original simulation; isolated files; event-coordinate pointer; sparse performance trace"}
    if fixture:
        manifest.update(seasonFixture=str(fixture), fullDays=args.season_reference_days,
                        boundary="0x101195e8 morning to morning",
                        sourceRevision=subprocess.check_output(["git", "rev-parse", "HEAD"], cwd=ROOT, text=True).strip(),
                        sourceDiffSHA256=hashlib.sha256(subprocess.check_output(["git", "diff", "HEAD"], cwd=ROOT)).hexdigest(),
                        sourceFiles={str(p.relative_to(ROOT)): hashlib.sha256(p.read_bytes()).hexdigest()
                                     for folder in (ROOT / "native", ROOT / "tools") for p in sorted(folder.iterdir())
                                     if p.is_file() and p.suffix in (".cpp", ".hpp", ".mm", ".py", ".txt", ".in")},
                        originalProgramSHA256=hashlib.sha256((program / "program.json").read_bytes()).hexdigest(),
                        privateRuntimeFiles={str(p.relative_to(runtime)): hashlib.sha256(p.read_bytes()).hexdigest()
                                             for p in sorted(runtime.rglob("*")) if p.is_file()},
                        note="Bounded original simulation; recorded native effects; callbacks re-execute; no RNG override")
    (destination / "manifest.json").write_text(json.dumps(manifest, indent=2) + "\n")
    print(json.dumps(manifest, indent=2))


if __name__ == "__main__":
    main()
