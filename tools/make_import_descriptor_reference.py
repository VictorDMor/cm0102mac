#!/usr/bin/env python3
"""Emit the complete original import/CFM descriptor tails privately."""
import argparse
import json
import sqlite3
from pathlib import Path
from compile_recomp import generate_page
from import_descriptor_audit import CONTRACTS, require_import_descriptor_bodies


def render_reference(records, userops):
    require_import_descriptor_bodies(records, complete=True)
    selected = {r['address'] for r in records}
    units = []
    for page in (0x1077f, 0x10780, 0x10781):
        fragment = [r for r in records if r['address'] >> 12 == page]
        code = generate_page(page, fragment, selected, userops=userops,
                             guest_context_transfers=False, exception_runtime_hooks=False,
                             compiler_arithmetic_hooks=False, register_context_hooks=False,
                             stdlib_core_hooks=False, time_runtime_hooks=False, import_descriptor_hooks=False)
        code = code.replace(f'bool page_{page:x}(State& s)', f'bool import_descriptor_reference_{page:x}(State& s)')
        if f'bool import_descriptor_reference_{page:x}(State& s)' not in code or 'try_modern_' in code:
            raise ValueError('Missing isolated original import-descriptor reference')
        units.append(code)
    units.append('namespace cm::aot { bool import_descriptor_reference(State& s) {\n'
                 'switch (s.pc >> 12) {\n' + ''.join(
                     f'case 0x{page:x}: return import_descriptor_reference_{page:x}(s);\n'
                     for page in (0x1077f, 0x10780, 0x10781)) + 'default: return false; } } }\n')
    return '\n'.join(units)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('input', type=Path)
    parser.add_argument('output', type=Path)
    args = parser.parse_args()
    with sqlite3.connect((args.input / 'instructions.sqlite').resolve().as_uri() + '?mode=ro', uri=True) as db:
        records = [json.loads(row[0]) for row in db.execute(
            'SELECT body FROM instructions WHERE address>=? AND address<? ORDER BY address',
            (CONTRACTS[0][0], CONTRACTS[-1][1]))]
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(render_reference(records, json.loads((args.input / 'program.json').read_text())['userops']))
    print(f'Emitted {len(records)} original import-descriptor instructions')


if __name__ == '__main__':
    main()
