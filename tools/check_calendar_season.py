#!/usr/bin/env python3
"""Check immutable real fixtures in strict reference and semantic calendar modes.

The original executable records an additional private calendar-edge stream in a
new temporary directory. Modern runs only consume it; fixture hashes and the
original strict comparison remain intact. No private data is published.
"""
import argparse
import json
from pathlib import Path
import subprocess
import tempfile
from check_season_reference import check, digest


def check_calendar(executable, fixture):
    strict = check(executable, fixture)
    runs = []
    with tempfile.TemporaryDirectory(prefix="cm-calendar-edges-") as temporary:
        edges = Path(temporary) / "original-calendar.gz"
        options = [
            ["--calendar-record", str(edges)],
            ["--modern", "--calendar-check", str(edges)],
            ["--modern", "--calendar-check", str(edges)],
            ["--modern", "--calendar-check", str(edges), "--perturb-rng"],
            ["--modern", "--calendar-check", str(edges), "--perturb-calendar"],
            ["--calendar-baseline", "--calendar-check", str(edges)],
            ["--modern", "--calendar-check", str(edges), "--perturb-difference"],
        ]
        edge_hash = None
        for index, arguments in enumerate(options):
            result = subprocess.run([str(executable), str(fixture), *arguments],
                                    capture_output=True, text=True, timeout=7200)
            runs.append({"mode": index, "exitCode": result.returncode,
                         "stdout": result.stdout, "stderr": result.stderr})
            if index in (0, 1, 2, 5) and result.returncode:
                raise RuntimeError("Equivalent calendar replay failed: " + json.dumps(runs[-1]))
            if index == 0:
                edge_hash = digest(edges)
            elif digest(edges) != edge_hash:
                raise RuntimeError("Original calendar edges changed")
        if runs[1]["stdout"] != runs[2]["stdout"]:
            raise RuntimeError("Equivalent modern reports disagree")
        for index, diagnostic in ((3, "First divergence:"), (4, "First divergence: calendar edge"),
                                  (6, "First divergence: calendar edge")):
            if runs[index]["exitCode"] != 1 or not runs[index]["stderr"].startswith(diagnostic):
                raise RuntimeError("Known divergence was not detected: " + json.dumps(runs[index]))
        if "field=difference return" not in runs[6]["stderr"]:
            raise RuntimeError("Difference control was not rejected at the return boundary")
    after = {name: digest(fixture / name) for name in strict["fixtureSHA256"]}
    if after != strict["fixtureSHA256"]:
        raise RuntimeError("Immutable season fixture changed")
    return {"status": "pass", "strictOriginal": strict, "modernEquivalentRuns": 2,
            "rngDivergenceDetected": True, "calendarDivergenceDetected": True,
            "differenceDivergenceDetected": True, "deliveredCalendarBaselineChecked": True,
            "originalCalendarEdgesSHA256": edge_hash, "fixtureUnchanged": True, "runs": runs,
            "comparison": "Only translated instruction accounting differs; PC, all registers, temporaries, all mapped bytes, allocation state, import/callback order and RNG remain checked"}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("fixture", type=Path)
    parser.add_argument("--executable", type=Path, default=Path(__file__).resolve().parent.parent / ".local/aot-build/cm-season-reference")
    args = parser.parse_args()
    try:
        report = check_calendar(args.executable.resolve(strict=True), args.fixture.resolve(strict=True))
    except (OSError, ValueError, RuntimeError, subprocess.TimeoutExpired) as error:
        parser.exit(1, str(error) + "\n")
    print(json.dumps(report, indent=2))


if __name__ == "__main__":
    main()
