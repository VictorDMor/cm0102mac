#!/usr/bin/env python3
"""Emit private, unmodified FIX_MAN and host-news bodies for policy checks."""
import argparse
import inspect
import json
from pathlib import Path
import sqlite3
from compile_recomp import generate_page


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("input", type=Path)
    parser.add_argument("output", type=Path)
    args = parser.parse_args()
    disabled = {name: False for name in inspect.signature(generate_page).parameters
                if name.endswith("_hooks") or name == "guest_context_transfers"}
    with sqlite3.connect((args.input / "instructions.sqlite").resolve().as_uri() + "?mode=ro", uri=True) as db:
        records = [json.loads(row[0]) for row in db.execute(
            "SELECT body FROM instructions WHERE address>=? AND address<? ORDER BY address",
            (0x1048a930, 0x1048ad10))]
    userops = json.loads((args.input / "program.json").read_text())["userops"]
    code = generate_page(0x1048a, records, {r["address"] for r in records}, userops=userops, **disabled)
    code = code.replace("page_1048a", "retirement_fixture_reference")
    if "inter_amer_cup_cancel_pending" in code:
        raise ValueError("Retirement reference must retain every original fixture")
    with sqlite3.connect((args.input / "instructions.sqlite").resolve().as_uri() + "?mode=ro", uri=True) as db:
        host_records = [json.loads(row[0]) for row in db.execute(
            "SELECT body FROM instructions WHERE address>=? AND address<? ORDER BY address",
            (0x1049df70, 0x1049e1f0))]
    routes = []
    for page in sorted({r["address"] >> 12 for r in host_records}):
        part = [r for r in host_records if r["address"] >> 12 == page]
        name = f"retirement_host_page_{page:x}"
        unit = generate_page(page, part, {r["address"] for r in host_records}, userops=userops, **disabled)
        code += unit.replace(f"page_{page:x}", name)
        routes.append(f"if ((s.pc >> 12) == 0x{page:x}) return {name}(s);")
    code += "namespace cm::aot { bool retirement_host_reference(State& s) {\n" + "\n".join(routes) + "\nreturn false; } }\n"
    if "inter_amer_cup_skip_host_announcement" in code:
        raise ValueError("Retirement reference must retain original host announcements")
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(code)
    print(f"Emitted {len(records)} unmodified FIX_MAN reader and {len(host_records)} host-news instructions")


if __name__ == "__main__":
    main()
