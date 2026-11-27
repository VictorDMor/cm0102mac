"""Emit the six original Croatian Cup bodies exclusively for the database checker."""
import argparse
import inspect
import json
from pathlib import Path
import sqlite3

from compile_recomp import generate_page
from croatian_cup_audit import ROUTINES, require_croatian_cup_index


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("export", type=Path)
    parser.add_argument("output", type=Path)
    args = parser.parse_args()
    disabled = {name: False for name in inspect.signature(generate_page).parameters
                if name.endswith("_hooks") or name == "guest_context_transfers"}
    userops = json.loads((args.export / "program.json").read_text())["userops"]
    units = ["#include <cstdlib>"]
    cases = []
    with sqlite3.connect((args.export / "instructions.sqlite").resolve().as_uri() + "?mode=ro", uri=True) as db:
        require_croatian_cup_index(db)
        for entry, end, _ in ROUTINES:
            records = [json.loads(row[0]) for row in db.execute(
                "SELECT body FROM instructions WHERE address>=? AND address<? ORDER BY address", (entry, end))]
            code = generate_page(entry >> 12, records, {r["address"] for r in records}, userops=userops, **disabled)
            name = f"croatian_reference_{entry:x}"
            if "croatia_cup::dispatch" in code:
                raise ValueError("the reference must contain the original routine body")
            units.append(code.replace(f"page_{entry >> 12:x}", name))
            enabled = "all || lifecycle" if entry in (0x106bf0f0, 0x106bf3d0, 0x106bfbe0) else "all"
            cases.append(f"if (s.pc >= 0x{entry:x} && s.pc < 0x{end:x} && ({enabled})) return {name}(s);")
    units.append("namespace cm::aot { bool croatian_cup_reference(State& s) {\n"
                 "if (s.pc < 0x106bf0f0 || s.pc >= 0x106bfe8c) return false;\n"
                 'const bool all = std::getenv("CM_CRO_CUP_ORIGINAL") != nullptr;\n'
                 'const bool lifecycle = std::getenv("CM_CRO_CUP_LIFECYCLE_ORIGINAL") != nullptr;\n' +
                 "\n".join(cases) + "\nreturn false; } }")
    args.output.write_text("\n".join(units) + "\n")


if __name__ == "__main__":
    main()
