#!/usr/bin/env python3
"""Emit the 327 original initializer instructions privately, without semantic hooks."""
import argparse
import inspect
import json
import sqlite3
from pathlib import Path
from compile_recomp import generate_page
from engine_initialization_audit import CONTRACTS, require_engine_initialization_bodies


def render_reference(records, userops):
    require_engine_initialization_bodies(records, complete=True)
    selected = {r['address'] for r in records}
    units = []
    disabled = {name: False for name in inspect.signature(generate_page).parameters
                if name.endswith("_hooks") or name == "guest_context_transfers"}
    for page in (65536, 65636, 65683, 65758, 65855, 65859, 65939, 66218, 66317, 66325, 66390, 66433, 66702, 67254, 67259, 67365, 67385, 67396, 67405, 67438):
        code = generate_page(page, [r for r in records if r['address'] >> 12 == page], selected,
                             userops=userops, **disabled)
        code = code.replace(f'bool page_{page:x}(State& s)', f'bool engine_initialization_reference_{page:x}(State& s)')
        if f'bool engine_initialization_reference_{page:x}(State& s)' not in code or 'try_modern_' in code:
            raise ValueError('Missing isolated original engine-initialization reference')
        units.append(code)
    units.append('namespace cm::aot { bool engine_initialization_reference(State& s) {\n'
                 'switch (s.pc >> 12) {\n' + ''.join(
                     f'case 0x{page:x}: return engine_initialization_reference_{page:x}(s);\n'
                     for page in (65536, 65636, 65683, 65758, 65855, 65859, 65939, 66218, 66317, 66325, 66390, 66433, 66702, 67254, 67259, 67365, 67385, 67396, 67405, 67438)) + 'default: return false; } } }\n')
    return '\n'.join(units)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('input', type=Path); parser.add_argument('output', type=Path)
    args = parser.parse_args()
    with sqlite3.connect((args.input / 'instructions.sqlite').resolve().as_uri() + '?mode=ro', uri=True) as db:
        records = [json.loads(row[0]) for start, end, _, _ in CONTRACTS for row in db.execute(
            'SELECT body FROM instructions WHERE address>=? AND address<? ORDER BY address', (start, end))]
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(render_reference(records, json.loads((args.input / 'program.json').read_text())['userops']))
    print(f'Emitted {len(records)} original engine-initialization instructions')


if __name__ == '__main__': main()
