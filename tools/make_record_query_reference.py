#!/usr/bin/env python3
"""Emit the 253 original record-query instructions privately, without hooks."""
import argparse
import json
import sqlite3
from pathlib import Path
from compile_recomp import generate_page
from record_query_audit import CONTRACTS, require_record_query_bodies


def render_reference(records, userops):
    require_record_query_bodies(records, complete=True)
    units = []
    # Separate functions expose cross-object calls to the component dependency
    # observer, even when the original objects share an instruction page.
    for start, end, _, _ in CONTRACTS:
        body = [r for r in records if start <= r['address'] < end]
        for page in range(start >> 12, ((end - 1) >> 12) + 1):
            part = [r for r in body if r['address'] >> 12 == page]
            code = generate_page(page, part, {r['address'] for r in body}, userops=userops,
                guest_context_transfers=False, exception_runtime_hooks=False,
                compiler_arithmetic_hooks=False, register_context_hooks=False,
                stdlib_core_hooks=False, time_runtime_hooks=False, import_descriptor_hooks=False,
                native_service_hooks=False, engine_initialization_hooks=False, utility_object_hooks=False,
                tree_mutation_hooks=False, aggregate_object_hooks=False, record_group_hooks=False, record_query_hooks=False)
            name = f'record_query_reference_{start:x}_{page:x}'
            code = code.replace(f'page_{page:x}', name)
            if f'bool {name}(State& s)' not in code or 'try_modern_' in code:
                raise ValueError('Missing isolated original record-query reference')
            units.append(code)
    units.append('namespace cm::aot { bool record_query_reference(State& s) {\n' + ''.join(
        f'if (s.pc >= 0x{max(start, page << 12):x} && s.pc < 0x{min(end, (page + 1) << 12):x}) return record_query_reference_{start:x}_{page:x}(s);\n'
        for start, end, _, _ in CONTRACTS for page in range(start >> 12, ((end - 1) >> 12) + 1)) + 'return false; } }\n')
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
    print(f'Emitted {len(records)} original record-query instructions')


if __name__ == '__main__': main()
