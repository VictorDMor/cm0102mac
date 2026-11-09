#!/usr/bin/env python3
"""The leftover-stack contract as a table: who reads stale stack, and who left it.

  stack_contract.py build [--jobs N] [--captures DIR ...] [--reattribute]
  stack_contract.py module MODULE.CPP

The original reads stack bytes it never wrote, and a rewrite must leave the
bytes that a known reader reads (CLAUDE.md, per-reader contract). `build` runs
`cm-world-replay CAPTURE 1 --timing --stack-audit-all` on every capture (or the
ones named), in parallel, and writes .local/stack-contract/contract.json and
report.md. `module` then answers, without another audit: which of the module's
routines leave bytes that some reader reads, and which leftovers the module
itself reads. Rebuild after recording captures.

A capture whose replay fails or times out contributes nothing and is listed.
The table covers what the captures reach: a reader none of them reaches is
missing, and the match replays have no audit.

Attribution. The audit names a site by the PC the state held at the access.
In generated code that is the start of the straight-line run, so the access
belongs to the routine that contains the PC. An access made by a C++ owner
that the caller's page calls directly carries the caller's return address, so
at a PC right after a `bl` to a C++ owner the access may be the owner's. The
table keeps both, and `module` lists the PC under each: for the caller's
rewrite it means "leave these bytes, or call that routine at the original
stack pointer"; for the owner, "these are bytes you read or leave". The
listing settles which. Routine bounds and owners come from the newest
tools/game_inventory.py output, so its module attribution limits apply.
`build --reattribute` redoes this from the stored audits after a new inventory.

Effects. If .local/stack-contract/effects.json exists ({"0x...": "differs"}),
each reader shows that note. The file from 2026-11-09 records, per reader,
whether the day or week end state changed when that reader alone read zero.
"""
import argparse
import bisect
from concurrent.futures import ThreadPoolExecutor
import json
from pathlib import Path
import re
import sqlite3
import sys

sys.path.insert(0, str(Path(__file__).resolve().parent))
import replay_sweep  # noqa: E402

ROOT = replay_sweep.ROOT
OUTPUT = ROOT / ".local/stack-contract"
SITE = re.compile(r"^\s+(0x[0-9a-f]+) reads (\d+) nonzero (\d+) at sp\+(.*?) left by(.*?)(?: values.*)?$")
COUNTED = re.compile(r" (-?(?:0x)?[0-9a-f]+)\(x(\d+)\)")


def parse_audit(text):
    """The sites of one --stack-audit-all report."""
    sites = []
    for line in text.splitlines():
        found = SITE.match(line)
        if not found:
            continue
        sites.append({"pc": int(found[1], 16), "reads": int(found[2]), "nonzero": int(found[3]),
                      "offsets": [int(offset) for offset, _ in COUNTED.findall(found[4])],
                      "writers": {int(pc, 16): int(count) for pc, count in COUNTED.findall(found[5])}})
    return sites


class Routines:
    """Routine and module of a PC, from the newest inventory and the export."""

    def __init__(self):
        inventories = sorted((ROOT / ".local/inventory").glob("*/inventory.json"))
        if not inventories:
            sys.exit("No inventory: run tools/game_inventory.py first")
        self.source = inventories[-1]
        routines = json.loads(self.source.read_text())["routines"]
        self.routines = sorted(((int(r["entry"], 16), int(r["end"], 16), r) for r in routines), key=lambda r: r[0])
        self.starts = [start for start, _, _ in self.routines]
        self.by_entry = {start: routine for start, _, routine in self.routines}
        self.export = sqlite3.connect(f"file:{ROOT / '.local/current/export/instructions.sqlite'}?mode=ro", uri=True)

    def containing(self, pc):
        index = bisect.bisect_right(self.starts, pc) - 1
        while index >= 0 and pc - self.routines[index][0] < 0x10000:
            start, end, routine = self.routines[index]
            if start <= pc < end:
                return routine
            index -= 1
        return None

    def assembly(self, address):
        row = self.export.execute("select body from instructions where address=?", (address,)).fetchone()
        return json.loads(row[0])["assembly"] if row else ""

    def attribute(self, pc):
        """The routine containing pc and, after a `bl` to a C++ owner, that owner."""
        if pc == 0:
            return {"entry": None, "module": "(before the capture)", "callee": None, "callee_module": None}
        routine = self.containing(pc)
        result = {"entry": routine["entry"] if routine else None,
                  "module": (routine["module"] or "(boundary)") if routine else "(no routine)",
                  "callee": None, "callee_module": None}
        called = re.fullmatch(r"bl (0x[0-9a-f]+)", self.assembly(pc - 4))
        callee = self.by_entry.get(int(called[1], 16)) if called else None
        if callee and callee.get("owner"):
            result.update(callee=callee["entry"], callee_module=callee["module"] or "(boundary)")
        return result


def audit(capture, timeout):
    status, output = replay_sweep.replay(replay_sweep.REPLAY, capture, ("--timing", "--stack-audit-all"), timeout=timeout)
    if status is None:
        return "TIMEOUT", []
    if status != 0:
        return f"failed: {replay_sweep.divergence(output) or f'rc={status}'}", []
    return "complete", parse_audit(output)


def stored_audits():
    """(sites, used, unused) as build collects them, from the table on disk."""
    contract = json.loads((OUTPUT / "contract.json").read_text())
    sites = {int(site["pc"], 16): {"reads": site["reads"], "nonzero": site["nonzero"], "offsets": set(site["offsets"]),
                                   "captures": site["captures"],
                                   "writers": {int(writer["pc"], 16): writer["bytes"] for writer in site["writers"]}}
             for site in contract["sites"]}
    return sites, contract["captures"], contract["unused"]


def run_audits(arguments):
    captures = [Path(c).resolve() for c in arguments.captures] or replay_sweep.captures()
    with ThreadPoolExecutor(arguments.jobs) as pool:
        results = list(pool.map(lambda capture: audit(capture, arguments.timeout), captures))
    sites, used, unused = {}, [], []
    for capture, (status, found) in zip(captures, results):
        name = str(capture.relative_to(ROOT)) if capture.is_relative_to(ROOT) else str(capture)
        if status != "complete":
            unused.append({"capture": name, "status": status})
            continue
        used.append({"capture": name, "sites": len(found)})
        for site in found:
            merged = sites.setdefault(site["pc"], {"reads": 0, "nonzero": 0, "offsets": set(), "writers": {}, "captures": []})
            merged["reads"] += site["reads"]
            merged["nonzero"] += site["nonzero"]
            merged["offsets"].update(site["offsets"])
            merged["captures"].append(name)
            for pc, count in site["writers"].items():
                merged["writers"][pc] = merged["writers"].get(pc, 0) + count
    return sites, used, unused


def build(arguments):
    sites, used, unused = stored_audits() if arguments.reattribute else run_audits(arguments)
    routines = Routines()
    effects_file = OUTPUT / "effects.json"
    effects = json.loads(effects_file.read_text()) if effects_file.exists() else {}
    table = []
    for pc, site in sorted(sites.items(), key=lambda item: -item[1]["reads"]):
        table.append({"pc": f"0x{pc:08x}", **routines.attribute(pc), "reads": site["reads"], "nonzero": site["nonzero"],
                      "offsets": sorted(site["offsets"]), "captures": site["captures"],
                      "effect": effects.get(f"0x{pc:08x}"),
                      "writers": [{"pc": f"0x{writer:08x}", **routines.attribute(writer), "bytes": count}
                                  for writer, count in sorted(site["writers"].items())]})
    OUTPUT.mkdir(parents=True, exist_ok=True)
    contract = {"inventory": str(routines.source.relative_to(ROOT)), "captures": used, "unused": unused, "sites": table}
    (OUTPUT / "contract.json").write_text(json.dumps(contract, indent=1) + "\n")
    (OUTPUT / "report.md").write_text(report(contract))
    print(f"{len(table)} reader sites from {len(used)} captures; {len(unused)} captures gave nothing")
    for entry in unused:
        print(f"  {entry['status']}: {entry['capture']}")
    print(f"Wrote {(OUTPUT / 'report.md').relative_to(ROOT)}")


def roles(access):
    """(module, routine entry, role) for an attributed PC: its routine and any owner called there."""
    found = [(access["module"], access["entry"], "run")]
    if access["callee"]:
        found.append((access["callee_module"], access["callee"], "called"))
    return found


def by_module(contract):
    """module -> {"reads": [(site, role)], "leaves": {routine entry: {reader pc: {writer pc: role}}}}"""
    modules = {}
    for site in contract["sites"]:
        for module_name, _, role in roles(site):
            modules.setdefault(module_name, {"reads": [], "leaves": {}})["reads"].append((site, role))
        for writer in site["writers"]:
            for module_name, entry, role in roles(writer):
                routine = modules.setdefault(module_name, {"reads": [], "leaves": {}})["leaves"].setdefault(entry, {})
                routine.setdefault(site["pc"], {})[writer["pc"]] = role
    return modules


def report(contract):
    modules = by_module(contract)
    lines = ["# Leftover-stack contract", "",
             f"{len(contract['sites'])} reader sites from {len(contract['captures'])} captures "
             f"({len(contract['unused'])} captures gave nothing); routines from `{contract['inventory']}`.",
             "`tools/stack_contract.py module MODULE.CPP` prints one module's part.", "",
             "| Module | Reader sites | Routines that leave bytes | Readers they feed |", "| --- | ---: | ---: | ---: |"]
    for module, entry in sorted(modules.items(), key=lambda item: (-len(item[1]["leaves"]), str(item[0]))):
        fed = {reader for readers in entry["leaves"].values() for reader in readers}
        lines.append(f"| {module} | {len(entry['reads'])} | {len(entry['leaves'])} | {len(fed)} |")
    if contract["unused"]:
        lines += ["", "Captures that gave nothing:", ""]
        lines += [f"- `{entry['capture']}`: {entry['status']}" for entry in contract["unused"]]
    return "\n".join(lines) + "\n"


def module(arguments):
    path = OUTPUT / "contract.json"
    if not path.exists():
        sys.exit("No table yet: run tools/stack_contract.py build")
    contract = json.loads(path.read_text())
    wanted = arguments.name.casefold()
    found = {name: entry for name, entry in by_module(contract).items() if name.casefold() == wanted}
    if not found:
        print(f"{arguments.name}: no reader and no writer in the table ({len(contract['captures'])} captures)")
        return
    sites = {site["pc"]: site for site in contract["sites"]}
    for name, entry in found.items():
        print(f"{name}: {len(entry['leaves'])} routines leave bytes a reader reads; {len(entry['reads'])} reader sites."
              " (called): the write may be that routine's, called at the PC before; see the docstring.")
        for routine, readers in sorted(entry["leaves"].items(), key=lambda item: str(item[0])):
            print(f"  routine {routine} leaves bytes read by:")
            for reader, writers in sorted(readers.items()):
                site = sites[reader]
                where = " ".join(pc + ("" if role == "run" else "(called)") for pc, role in sorted(writers.items()))
                effect = f", zero test: {site['effect']}" if site["effect"] else ""
                place = f"{site['module']} routine {site['entry']}" if site["entry"] else site["module"]
                print(f"    {reader} in {place}{effect}; written at {where}")
        for site, role in entry["reads"]:
            origin = {}
            for writer in site["writers"]:
                origin[writer["module"]] = origin.get(writer["module"], 0) + writer["bytes"]
            offsets = " ".join(str(offset) for offset in site["offsets"][:8])
            if role == "called":
                where = f"as routine {site['callee']}, called from {site['module']} {site['entry']}"
            elif site["callee"]:
                where = f"in routine {site['entry']}, after its call to {site['callee_module']} {site['callee']}"
            else:
                where = f"in routine {site['entry']}"
            effect = f"; zero test: {site['effect']}" if site["effect"] else ""
            left = ", ".join(f"{module} {count}" for module, count in sorted(origin.items(), key=lambda item: -item[1])[:6])
            print(f"  reader {site['pc']} {where}: {site['reads']} stale reads at sp+ {offsets}{effect}")
            print(f"    left by: {left}")


def main():
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    commands = parser.add_subparsers(dest="command", required=True)
    builder = commands.add_parser("build")
    builder.add_argument("--jobs", type=int, default=6)
    builder.add_argument("--timeout", type=float, default=900)
    builder.add_argument("--captures", nargs="*", default=[])
    builder.add_argument("--reattribute", action="store_true", help="reuse the stored audits; only map PCs to routines again")
    commands.add_parser("module").add_argument("name")
    arguments = parser.parse_args()
    (build if arguments.command == "build" else module)(arguments)


if __name__ == "__main__":
    main()
