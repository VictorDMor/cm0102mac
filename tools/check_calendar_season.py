#!/usr/bin/env python3
"""Check immutable real fixtures in strict reference and semantic calendar modes.

The original executable records an additional private calendar-edge stream in a
new temporary directory. Modern runs only consume it; fixture hashes and the
original strict comparison remain intact. No private data is published.
"""
import argparse
import json
import gzip
import re
from pathlib import Path
import subprocess
import tempfile
from contextlib import nullcontext
from check_season_reference import check, digest


def first_edge(path, entry):
    # Exact v2 record lengths: raw arguments/date, result/effects and ABI.
    sizes = {0x10717890: 97, 0x10717B20: 97, 0x10718130: 214,
             0x10331860: 17244, 0x103317F0: 17244}
    with gzip.open(path, "rb") as stream:
        if stream.read(8) != b"CMCAL002":
            raise RuntimeError("Unrecognized private calendar v2 stream")
        index = 0
        while raw := stream.read(4):
            address = int.from_bytes(raw, "big")
            if len(raw) != 4 or address not in sizes:
                raise RuntimeError("Invalid private calendar edge")
            if address == entry:
                return index
            if len(stream.read(sizes[address] - 4)) != sizes[address] - 4:
                raise RuntimeError("Truncated private calendar edge")
            index += 1
    raise RuntimeError("Reported modern entry absent from original edge stream")


def check_calendar(executable, fixture, edges_directory=None):
    strict = check(executable, fixture)
    runs = []
    if edges_directory is not None:
        edges_directory.mkdir(parents=True, exist_ok=False)
    context = nullcontext(str(edges_directory)) if edges_directory else tempfile.TemporaryDirectory(prefix="cm-calendar-edges-")
    with context as temporary:
        edges = Path(temporary) / "original-calendar-v2.gz"
        month_edges = Path(temporary) / "original-month-v1.gz"
        options = [
            ["--calendar-record-v2", str(edges), "--month-record", str(month_edges)],
            ["--modern", "--calendar-check-v2", str(edges)],
            ["--modern", "--calendar-check-v2", str(edges)],
            ["--modern", "--calendar-check-v2", str(edges), "--perturb-rng"],
            ["--modern", "--calendar-check-v2", str(edges), "--perturb-calendar"],
            ["--calendar-baseline", "--calendar-check-v2", str(edges)],
            ["--modern", "--calendar-check-v2", str(edges), "--perturb-difference"],
            ["--date-ordering-baseline", "--calendar-check-v2", str(edges)],
            ["--modern", "--calendar-check-v2", str(edges), "--perturb-ordering"],
            ["--date-before-baseline", "--calendar-check-v2", str(edges)],
            ["--modern", "--calendar-check-v2", str(edges), "--perturb-before"],
        ]
        options.extend([
            ["--date-month-baseline", "--calendar-check-v2", str(edges)],
            ["--modern", "--calendar-check-v2", str(edges), "--perturb-month"],
        ])
        for arguments in options[1:]:
            arguments.extend(["--month-check", str(month_edges)])
        edge_hash = None
        month_hash = None
        month_calls = None
        ordering_calls = None
        before_calls = None
        for index, arguments in enumerate(options):
            if (index == 8 and not ordering_calls) or (index == 10 and not before_calls) or (index == 12 and not month_calls):
                continue  # An unexercised entry cannot provide a negative control.
            result = subprocess.run([str(executable), str(fixture), *arguments],
                                    capture_output=True, text=True, timeout=7200)
            runs.append({"mode": index, "exitCode": result.returncode,
                         "stdout": result.stdout, "stderr": result.stderr})
            if index in (0, 1, 2, 5, 7, 9, 11) and result.returncode:
                raise RuntimeError("Equivalent calendar replay failed: " + json.dumps(runs[-1]))
            if index == 0:
                edge_hash = digest(edges)
                month_hash = digest(month_edges)
            elif digest(edges) != edge_hash or digest(month_edges) != month_hash:
                raise RuntimeError("Original calendar edges changed")
            if index == 1:
                count = re.search(r"modern-ordering=(\d+)", result.stdout)
                if not count:
                    raise RuntimeError("Modern ordering coverage was not reported")
                ordering_calls = int(count.group(1))
                count = re.search(r"modern-before=(\d+)", result.stdout)
                if not count:
                    raise RuntimeError("Modern before coverage was not reported")
                before_calls = int(count.group(1))
                count = re.search(r"modern-month=(\d+)", result.stdout)
                if not count:
                    raise RuntimeError("Modern month coverage was not reported")
                month_calls = int(count.group(1))
        if runs[1]["stdout"] != runs[2]["stdout"]:
            raise RuntimeError("Equivalent modern reports disagree")
        by_mode = {r["mode"]: r for r in runs}
        negatives = [(3, "First divergence:"), (4, "First divergence: calendar edge"),
                     (6, "First divergence: calendar edge")]
        if ordering_calls:
            negatives.append((8, "First divergence: calendar edge"))
        if before_calls:
            negatives.append((10, "First divergence: calendar edge"))
        if month_calls:
            negatives.append((12, "First divergence: month edge 0"))
        for index, diagnostic in negatives:
            if by_mode[index]["exitCode"] != 1 or not by_mode[index]["stderr"].startswith(diagnostic):
                raise RuntimeError("Known divergence was not detected: " + json.dumps(by_mode[index]))
        if "field=difference return" not in by_mode[6]["stderr"]:
            raise RuntimeError("Difference control was not rejected at the return boundary")
        if ordering_calls and "field=ordering return" not in by_mode[8]["stderr"]:
            raise RuntimeError("Ordering control was not rejected at the return boundary")
        before_first_edge = first_edge(edges, 0x103317F0) if before_calls else None
        if before_calls:
            diagnostic = by_mode[10]["stderr"]
            if (f"calendar edge {before_first_edge} at 0x103317f0 field=before return byte=36" not in diagnostic):
                raise RuntimeError("Before control was not rejected at its first return boundary")
        if month_calls and "at 0x10718360 field=month return byte=48" not in by_mode[12]["stderr"]:
            raise RuntimeError("Month control was not rejected at its first return boundary")
    after = {name: digest(fixture / name) for name in strict["fixtureSHA256"]}
    if after != strict["fixtureSHA256"]:
        raise RuntimeError("Immutable season fixture changed")
    return {"status": "pass", "strictOriginal": strict, "modernEquivalentRuns": 2,
            "rngDivergenceDetected": True, "calendarDivergenceDetected": True,
            "differenceDivergenceDetected": True, "deliveredCalendarBaselineChecked": True,
            "orderingDivergenceDetected": bool(ordering_calls), "modernOrderingCalls": ordering_calls,
            "orderingCoverage": "exercised" if ordering_calls else "unexercised; separate real fixture required",
            "deliveredDateDifferenceBaselineChecked": True, "deliveredDateOrderingBaselineChecked": True,
            "modernBeforeCalls": before_calls, "beforeDivergenceDetected": bool(before_calls),
            "beforeCoverage": "exercised" if before_calls else "unexercised; separate real fixture required",
            "modernMonthCalls": month_calls, "monthDivergenceDetected": bool(month_calls),
            "deliveredDateBeforeBaselineChecked": True, "monthEdgeFormat": "CMMON001",
            "originalMonthEdgesSHA256": month_hash,
            "monthCoverage": "exercised" if month_calls else "unexercised; separate real fixture required",
            "calendarEdgeFormat": "CMCAL002", "firstBeforeEdge": before_first_edge,
            "originalCalendarEdgesSHA256": edge_hash, "fixtureUnchanged": True, "runs": runs,
            "comparison": "Only translated instruction accounting differs; PC, all registers, temporaries, all mapped bytes, allocation state, import/callback order and RNG remain checked"}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("fixture", type=Path)
    parser.add_argument("--executable", type=Path, default=Path(__file__).resolve().parent.parent / ".local/aot-build/cm-season-reference")
    parser.add_argument("--edges-directory", type=Path, help="New private directory retaining separate original CMCAL002 and CMMON001 streams")
    args = parser.parse_args()
    try:
        report = check_calendar(args.executable.resolve(strict=True), args.fixture.resolve(strict=True),
                                args.edges_directory.resolve() if args.edges_directory else None)
    except (OSError, ValueError, RuntimeError, subprocess.TimeoutExpired) as error:
        parser.exit(1, str(error) + "\n")
    print(json.dumps(report, indent=2))


if __name__ == "__main__":
    main()
