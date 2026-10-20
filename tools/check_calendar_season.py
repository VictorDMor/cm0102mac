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
from date_audit_stream import boundaries
from date_oracle_storage import DateOracle


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


def check_calendar(executable, fixture, edges_directory=None, historical_edges_directory=None, date_subsystem=False, archive_date=False, date_timeout=7200):
    if archive_date and not date_subsystem:
        raise ValueError("Date archival storage requires the complete subsystem campaign")
    if not isinstance(date_timeout, int) or date_timeout < 1:
        raise ValueError("Date oracle timeout must be positive whole seconds")
    strict = check(executable, fixture)
    runs = []
    if edges_directory is not None:
        edges_directory.mkdir(parents=True, exist_ok=False)
    context = nullcontext(str(edges_directory)) if edges_directory else tempfile.TemporaryDirectory(prefix="cm-calendar-edges-")
    with context as temporary:
        edges = Path(temporary) / "original-calendar-v2.gz"
        month_edges = Path(temporary) / "original-month-v1.gz"
        day_edges = Path(temporary) / "original-day-of-month-v1.gz"
        weekday_edges = Path(temporary) / "original-weekday-v1.gz"
        date_edges = Path(temporary) / "original-date-v1.gz"
        date_oracle = DateOracle(date_edges, archive_date)
        date_hash = None
        date_archive_hash = None
        date_matrix = {}
        historical_names = []
        options = [
            ["--calendar-record-v2", str(edges), "--month-record", str(month_edges), "--day-of-month-record", str(day_edges), "--weekday-record", str(weekday_edges)],
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
            ["--date-day-of-month-baseline", "--calendar-check-v2", str(edges)],
            ["--modern", "--calendar-check-v2", str(edges), "--perturb-day-of-month"],
        ])
        options.extend([
            ["--date-weekday-baseline", "--calendar-check-v2", str(edges)],
            ["--modern", "--calendar-check-v2", str(edges), "--perturb-weekday"],
        ])
        if date_subsystem:
            options[0].extend(["--date-record", str(date_edges)])
            options.append(["--date-subsystem-baseline", "--calendar-check-v2", str(edges)])
            for arguments in options[1:]:
                arguments.extend(["--date-check", str(date_edges)])
        for arguments in options[1:]:
            arguments.extend(["--month-check", str(month_edges), "--day-of-month-check", str(day_edges), "--weekday-check", str(weekday_edges)])
        edge_hash = None
        month_hash = None
        day_hash = None
        weekday_hash = None
        weekday_calls = None
        day_calls = None
        month_calls = None
        ordering_calls = None
        before_calls = None
        for index, arguments in enumerate(options):
            if (index == 8 and not ordering_calls) or (index == 10 and not before_calls) or (index == 12 and not month_calls) or (index == 14 and not day_calls) or (index == 16 and not weekday_calls):
                continue  # An unexercised entry cannot provide a negative control.
            command = [str(executable), str(fixture), *arguments]
            result = date_oracle.run(command, timeout=date_timeout) if date_subsystem else subprocess.run(
                command, capture_output=True, text=True, timeout=7200)
            runs.append({"mode": index, "exitCode": result.returncode,
                         "stdout": result.stdout, "stderr": result.stderr})
            if index in (0, 1, 2, 5, 7, 9, 11, 13, 15, 17) and result.returncode:
                raise RuntimeError("Equivalent calendar replay failed: " + json.dumps(runs[-1]))
            if index == 0:
                edge_hash = digest(edges)
                month_hash = digest(month_edges)
                day_hash = digest(day_edges)
                weekday_hash = digest(weekday_edges)
                if date_subsystem:
                    date_hash = date_oracle.digest()
                    date_archive_hash = digest(date_oracle.stored_path) if archive_date else None
                    date_matrix = boundaries(date_oracle.stored_path)
                if historical_edges_directory:
                    current_streams = [edges, month_edges, day_edges]
                    if date_subsystem or (historical_edges_directory / weekday_edges.name).exists():
                        current_streams.append(weekday_edges)
                    for current in current_streams:
                        historical_names.append(current.name)
                        previous = historical_edges_directory / current.name
                        if digest(previous) != digest(current):
                            raise RuntimeError("Recaptured original historical stream differs: " + current.name)
            elif digest(edges) != edge_hash or digest(month_edges) != month_hash or digest(day_edges) != day_hash or digest(weekday_edges) != weekday_hash:
                raise RuntimeError("Original calendar edges changed")
            if date_subsystem and date_oracle.digest() != date_hash:
                raise RuntimeError("Original Date stream changed")
            if archive_date and digest(date_oracle.stored_path) != date_archive_hash:
                raise RuntimeError("Original Date lossless archive changed")
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
                count = re.search(r"modern-day-of-month=(\d+)", result.stdout)
                if not count:
                    raise RuntimeError("Modern day-of-month coverage was not reported")
                day_calls = int(count.group(1))
                count = re.search(r"modern-weekday=(\d+)", result.stdout)
                if not count:
                    raise RuntimeError("Modern weekday coverage was not reported")
                weekday_calls = int(count.group(1))
        if date_subsystem:
            observed = {entry: (mode, int(modern), int(original)) for entry, mode, modern, original in
                        re.findall(r"date.entry=(0x[0-9a-f]+) mode=(original|modern) modern=(\d+) original=(\d+)", runs[1]["stdout"])}
            for entry, evidence in date_matrix.items():
                if entry not in observed or observed[entry] != ("modern", evidence["calls"], 0):
                    raise RuntimeError("Date own-call matrix differs from original boundaries: " + entry)
                evidence["mode"] = "modern"
                evidence["coverage"] = "exercised" if evidence["calls"] else "unexercised; additional real flow required"
                evidence["negativeDetected"] = False
                if not evidence["calls"]:
                    continue
                arguments = ["--modern", "--date-check", str(date_edges), "--perturb-date", entry]
                result = date_oracle.run([str(executable), str(fixture), *arguments], timeout=date_timeout)
                run = {"mode": "date-negative-" + entry, "exitCode": result.returncode, "stdout": result.stdout, "stderr": result.stderr}
                runs.append(run)
                expected = f"First divergence: Date edge {evidence['firstIndex']} at {entry} field=result/ABI/effects"
                if result.returncode != 1 or not result.stderr.startswith(expected):
                    raise RuntimeError("Date perturbation escaped its first own result boundary: " + json.dumps(run))
                if date_oracle.digest() != date_hash:
                    raise RuntimeError("Original Date negative control modified its oracle")
                if archive_date and digest(date_oracle.stored_path) != date_archive_hash:
                    raise RuntimeError("Original Date negative control modified its lossless archive")
                evidence["negativeDetected"] = True
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
        if day_calls:
            negatives.append((14, "First divergence: day-of-month edge 0"))
        if weekday_calls:
            negatives.append((16, "First divergence: weekday edge 0"))
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
        if day_calls and "at 0x107182f0 field=day-of-month return byte=48" not in by_mode[14]["stderr"]:
            raise RuntimeError("Day-of-month control was not rejected at its first return boundary")
        if weekday_calls and "at 0x10715b20 field=weekday return byte=20" not in by_mode[16]["stderr"]:
            raise RuntimeError("Weekday control was not rejected at its first return boundary")
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
            "deliveredDateMonthBaselineChecked": True,
            "modernDayOfMonthCalls": day_calls, "dayOfMonthDivergenceDetected": bool(day_calls),
            "modernWeekdayCalls": weekday_calls, "weekdayDivergenceDetected": bool(weekday_calls),
            "weekdayEdgeFormat": "CMWKD001", "originalWeekdayEdgesSHA256": weekday_hash,
            "deliveredDateDayOfMonthBaselineChecked": True,
            "weekdayCoverage": "exercised" if weekday_calls else "unexercised; separate real fixture required",
            "dayOfMonthEdgeFormat": "CMDOM001", "originalDayOfMonthEdgesSHA256": day_hash,
            "dayOfMonthCoverage": "exercised" if day_calls else "unexercised; separate real fixture required",
            "historicalStreamsByteIdentical": bool(historical_edges_directory),
            "monthCoverage": "exercised" if month_calls else "unexercised; separate real fixture required",
            "calendarEdgeFormat": "CMCAL002", "firstBeforeEdge": before_first_edge,
            "originalCalendarEdgesSHA256": edge_hash, "fixtureUnchanged": True, "runs": runs,
            "historicalStreamsCompared": historical_names,
            "dateSubsystemChecked": date_subsystem, "dateSubsystemEdgeFormat": "CMDAT001" if date_subsystem else None,
            "originalDateEdgesSHA256": date_hash, "dateOwnEntryMatrix": date_matrix,
            "dateOracleStorage": "lossless xz of identical gzip bytes" if archive_date else "gzip",
            "dateOracleTimeoutSeconds": date_timeout if date_subsystem else None,
            "originalDateArchiveSHA256": date_archive_hash,
            "comparison": "Only translated instruction accounting differs; PC, all registers, temporaries, all mapped bytes, allocation state, import/callback order and RNG remain checked"}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("fixture", type=Path)
    parser.add_argument("--executable", type=Path, default=Path(__file__).resolve().parent.parent / ".local/current/build/cm-season-reference")
    parser.add_argument("--edges-directory", type=Path, help="New private directory retaining separate original CMCAL002, CMMON001, CMDOM001 and CMWKD001 streams")
    parser.add_argument("--historical-edges-directory", type=Path, help="Preserved original CMCAL002/CMMON001/CMDOM001/CMWKD001 directory; require byte-identical original recapture")
    parser.add_argument("--date-subsystem", action="store_true", help="Validate all 34 entries, seven baselines, CMDAT001 and each exercised pending-entry negative")
    parser.add_argument("--archive-date-oracle", action="store_true", help="Store identical Date gzip bytes losslessly in xz; feed every complete native replay through a fresh FIFO")
    parser.add_argument("--date-oracle-timeout", type=int, help="Positive host time limit in seconds for each complete Date capture/replay (default: 7200); preserves all native comparison and EOF checks")
    args = parser.parse_args()
    if args.date_oracle_timeout is not None and (not args.date_subsystem or args.date_oracle_timeout < 1):
        parser.error("--date-oracle-timeout requires --date-subsystem and positive whole seconds")
    try:
        report = check_calendar(args.executable.resolve(strict=True), args.fixture.resolve(strict=True),
                                args.edges_directory.resolve() if args.edges_directory else None,
                                args.historical_edges_directory.resolve(strict=True) if args.historical_edges_directory else None, args.date_subsystem, args.archive_date_oracle,
                                args.date_oracle_timeout if args.date_oracle_timeout is not None else 7200)
    except (OSError, ValueError, RuntimeError, subprocess.TimeoutExpired) as error:
        parser.exit(1, str(error) + "\n")
    print(json.dumps(report, indent=2))


if __name__ == "__main__":
    main()
