#!/usr/bin/env python3
"""Generate independent private reference bodies for the implemented entries."""
import argparse
import inspect
import json
from pathlib import Path
import sqlite3

from compile_recomp import generate_page
from contract_screens_audit import CONTRACTS, CONTRACT_SCREENS_ENTRIES, require_contract_screens_index
from tactics_audit import routine_records


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("export", type=Path)
    parser.add_argument("output", type=Path)
    args = parser.parse_args()
    disabled = {name: False for name in inspect.signature(generate_page).parameters
                if name.endswith("_hooks") or name == "guest_context_transfers"}
    with sqlite3.connect((args.export / "instructions.sqlite").resolve().as_uri() + "?mode=ro", uri=True) as db:
        require_contract_screens_index(db)
        records = [record for entry, end, _, _ in CONTRACTS if entry in CONTRACT_SCREENS_ENTRIES
                   for record in routine_records(db, entry, end)]
    userops = json.loads((args.export / "program.json").read_text())["userops"]
    selected = {record["address"] for record in records}
    units, routes = [], []
    for page in sorted({address >> 12 for address in selected}):
        part = [record for record in records if record["address"] >> 12 == page]
        name = f"contract_screens_reference_{page:x}"
        code = generate_page(page, part, selected, userops=userops, **disabled)
        code = code.replace(f"page_{page:x}", name).replace(f"{name}_impl<true,", f"{name}_impl<false,")
        if "try_modern_" in code:
            raise ValueError("Original contract screen reference contains a replacement hook")
        units.append(code)
        routes.append(f"if ((s.pc >> 12) == 0x{page:x}) return {name}(s);")
    units.append("namespace cm::aot { bool contract_screens_reference(State& s) {\n" +
                 "\n".join(routes) + "\nreturn false; } }\n")
    args.output.write_text("\n".join(units))


if __name__ == "__main__":
    main()
