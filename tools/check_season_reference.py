#!/usr/bin/env python3
"""Repeat a completed private day fixture and require a known RNG divergence.

No fixture is written or uploaded. The JSON report can be redirected to a
private local evidence file. Requires the exact compiled reference executable.
"""
import argparse
import hashlib
import json
from pathlib import Path
import subprocess


def digest(path):
    value = hashlib.sha256()
    with path.open("rb") as source:
        for block in iter(lambda: source.read(1024 * 1024), b""):
            value.update(block)
    return value.hexdigest()


def check(executable, fixture):
    names = ("capture.txt", "initial.snapshot", "expected.snapshot", "imports.gz")
    before = {name: digest(fixture / name) for name in names}
    metadata = (fixture / "capture.txt").read_text()
    if "status=complete\n" not in metadata:
        raise RuntimeError("The private capture has no completed full-day boundary")
    results = []
    for options in ([], [], ["--perturb-rng"]):
        result = subprocess.run([str(executable), str(fixture), *options],
                                capture_output=True, text=True, timeout=7200)
        results.append({"options": options, "exitCode": result.returncode,
                        "stdout": result.stdout, "stderr": result.stderr})
    for result in results[:2]:
        if result["exitCode"] != 0 or not result["stdout"].startswith(
                "PASS: exact bounded simulation agreement\n"):
            raise RuntimeError("Equivalent bounded replay failed: " + json.dumps(result))
    if results[0]["stdout"] != results[1]["stdout"]:
        raise RuntimeError("Equivalent replay reports disagree")
    if results[2]["exitCode"] != 1 or not results[2]["stderr"].startswith("First divergence:"):
        raise RuntimeError("Known RNG divergence was not detected: " + json.dumps(results[2]))
    after = {name: digest(fixture / name) for name in names}
    if before != after:
        raise RuntimeError("The immutable private fixture changed during validation")
    return {"status": "pass", "equivalentRuns": 2, "knownDivergenceDetected": True,
            "fixtureUnchanged": True, "fixtureSHA256": before,
            "executableSHA256": digest(executable), "capture": metadata, "runs": results}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("fixture", type=Path)
    parser.add_argument("--executable", type=Path,
                        default=Path(__file__).resolve().parent.parent / ".local/aot-build/cm-season-reference")
    args = parser.parse_args()
    try:
        report = check(args.executable.resolve(strict=True), args.fixture.resolve(strict=True))
    except (OSError, ValueError, RuntimeError, subprocess.TimeoutExpired) as error:
        parser.exit(1, str(error) + "\n")
    print(json.dumps(report, indent=2))


if __name__ == "__main__":
    main()
