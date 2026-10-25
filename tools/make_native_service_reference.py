#!/usr/bin/env python3
"""Emit the 27 original facade instructions privately, without semantic hooks."""
import argparse
import json
import sqlite3
from pathlib import Path
from compile_recomp import generate_page
from native_service_audit import CONTRACTS, require_native_service_bodies


def render_reference(records, userops):
    require_native_service_bodies(records, complete=True)
    selected = {r['address'] for r in records}
    units = []
    for page in (0x10770, 0x10778):
        code = generate_page(page, [r for r in records if r['address'] >> 12 == page], selected, userops=userops,
                             guest_context_transfers=False, exception_runtime_hooks=False,
                             compiler_arithmetic_hooks=False, register_context_hooks=False,
                             stdlib_core_hooks=False, time_runtime_hooks=False, import_descriptor_hooks=False,
                             native_service_hooks=False, main_program_hooks=False)
        code = code.replace(f'bool page_{page:x}(State& s)', f'bool native_service_reference_{page:x}(State& s)')
        if f'bool native_service_reference_{page:x}(State& s)' not in code or 'try_modern_' in code:
            raise ValueError('Missing isolated original native-service reference')
        units.append(code)
    units.append('namespace cm::aot { bool native_service_reference(State& s) {\n'
                 'switch (s.pc >> 12) {\n' + ''.join(
                     f'case 0x{page:x}: return native_service_reference_{page:x}(s);\n'
                     for page in (0x10770, 0x10778)) + 'default: return false; } } }\n')
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
    print(f'Emitted {len(records)} original native-service instructions')


if __name__ == '__main__': main()
