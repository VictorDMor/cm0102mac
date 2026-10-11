#!/usr/bin/env python3
"""Emit the 239 original utility-object instructions privately, without hooks."""
import argparse
import json
import sqlite3
from pathlib import Path
from compile_recomp import generate_page
from utility_object_audit import CONTRACTS, require_utility_object_bodies


def render_reference(records, userops):
    require_utility_object_bodies(records, complete=True)
    units = []
    # Separate functions expose cross-object calls to the component dependency
    # observer, even when the original objects share an instruction page.
    for start, end, _, _ in CONTRACTS:
        body = [r for r in records if start <= r['address'] < end]
        page = start >> 12
        code = generate_page(page, body, {r['address'] for r in body}, userops=userops,
            guest_context_transfers=False, exception_runtime_hooks=False,
            compiler_arithmetic_hooks=False, register_context_hooks=False,
            stdlib_core_hooks=False, time_runtime_hooks=False, import_descriptor_hooks=False,
            native_service_hooks=False, engine_initialization_hooks=False, utility_object_hooks=False)
        code = code.replace(f'bool page_{page:x}(State& s)', f'bool utility_object_reference_{start:x}(State& s)')
        if f'bool utility_object_reference_{start:x}(State& s)' not in code or 'try_modern_' in code:
            raise ValueError('Missing isolated original utility-object reference')
        units.append(code)
    units.append('namespace cm::aot { bool utility_object_reference(State& s) {\n' + ''.join(
        f'if (s.pc >= 0x{start:x} && s.pc < 0x{end:x}) return utility_object_reference_{start:x}(s);\n'
        for start, end, _, _ in CONTRACTS) + 'return false; } }\n')
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
    print(f'Emitted {len(records)} original utility-object instructions')


if __name__ == '__main__': main()
