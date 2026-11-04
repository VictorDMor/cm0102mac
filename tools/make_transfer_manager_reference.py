#!/usr/bin/env python3
"""Emit the private, hook-free reference for the handwritten transfer-manager blocks."""
import argparse
from collections import defaultdict
import inspect
import json
from pathlib import Path
import sqlite3

from compile_recomp import generate_page
from tactics_audit import routine_records


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("export", type=Path)
    parser.add_argument("output", type=Path)
    parser.add_argument("--entry", type=lambda value: int(value, 0), action="append",
                        help="emit specific audited entries, including candidates before production takeover")
    args = parser.parse_args()

    from transfer_manager_audit import BOUNDS, registered, require_transfer_manager_index

    disabled = {name: False for name in inspect.signature(generate_page).parameters
                if name.endswith("_hooks") or name == "guest_context_transfers"}
    entries = sorted(set(args.entry or (entry for entry, _, _ in registered())))
    if not entries or set(entries) - BOUNDS.keys():
        parser.error("expected at least one audited transfer-manager entry")
    pages = defaultdict(list)
    with sqlite3.connect((args.export / "instructions.sqlite").resolve().as_uri() + "?mode=ro", uri=True) as db:
        require_transfer_manager_index(db)
        for entry in entries:
            for item in routine_records(db, entry, BOUNDS[entry]):
                pages[item["address"] >> 12].append(item)
    userops = json.loads((args.export / "program.json").read_text())["userops"]
    selected = {r["address"] for page in pages.values() for r in page}
    units, routes = [], []
    for page, records in sorted(pages.items()):
        code = generate_page(page, records, selected, userops=userops, **disabled)
        name = f"transfer_manager_reference_{page:x}"
        code = code.replace(f"page_{page:x}", name).replace(f"{name}_impl<true,", f"{name}_impl<false,")
        if "try_modern_" in code:
            raise ValueError("Original transfer-manager reference contains a replacement hook")
        units.append(code)
        routes.append(f"if ((s.pc >> 12) == 0x{page:x}) return {name}(s);")
    units.append("namespace cm::aot { bool transfer_manager_reference(State& s) {\n" +
                 "\n".join(routes) + "\nreturn false; } }\n")
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text("\n".join(units))


if __name__ == "__main__":
    main()
