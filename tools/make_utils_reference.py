#!/usr/bin/env python3
"""Emit private UTILS reference pages with every replacement hook disabled."""
import argparse
import inspect
import json
from pathlib import Path
import sqlite3

from compile_recomp import generate_page
from utils_audit import MODULE, require_utils_index


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("input", type=Path)
    parser.add_argument("output", type=Path)
    args = parser.parse_args()
    disabled = {name: False for name in inspect.signature(generate_page).parameters
                if name.endswith("_hooks") or name == "guest_context_transfers"}
    with sqlite3.connect((args.input / "instructions.sqlite").resolve().as_uri() + "?mode=ro", uri=True) as db:
        require_utils_index(db)
        records = [json.loads(row[0]) for row in db.execute(
            "SELECT body FROM instructions WHERE address>=? AND address<? ORDER BY address", MODULE)]
    userops = json.loads((args.input / "program.json").read_text())["userops"]
    selected = {r["address"] for r in records}
    units, routes = [], []
    for page in sorted({r["address"] >> 12 for r in records}):
        part = [r for r in records if r["address"] >> 12 == page]
        code = generate_page(page, part, selected, userops=userops, **disabled)
        name = f"utils_reference_{page:x}"
        code = code.replace(f"page_{page:x}", name).replace(f"{name}_impl<true,", f"{name}_impl<false,")
        if "try_modern_" in code:
            raise ValueError("Original UTILS reference contains a replacement hook")
        units.append(code)
        routes.append(f"if ((s.pc >> 12) == 0x{page:x}) return {name}(s);")
    units.append("namespace cm::aot { bool utils_reference(State& s) {\n" +
                 "\n".join(routes) + "\nreturn false; } }\n")
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text("\n".join(units))
    print(f"Emitted {len(records)} original UTILS instructions")


if __name__ == "__main__":
    main()
