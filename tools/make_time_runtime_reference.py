#!/usr/bin/env python3
"""Emit all original clock and calendar instructions privately."""
import argparse
import json
import sqlite3
from pathlib import Path
from compile_recomp import generate_page
from time_runtime_audit import CONTRACTS, require_time_runtime_bodies


def render_reference(records, userops):
    require_time_runtime_bodies(records, complete=True)
    code = generate_page(0x1077f, records, {r['address'] for r in records}, userops=userops,
                         guest_context_transfers=False, exception_runtime_hooks=False,
                         compiler_arithmetic_hooks=False, register_context_hooks=False, stdlib_core_hooks=False, time_runtime_hooks=False)
    code = code.replace('bool page_1077f(State& s)', 'bool time_runtime_reference(State& s)')
    if 'bool time_runtime_reference(State& s)' not in code or 'try_modern_' in code:
        raise ValueError('Missing isolated original time-runtime reference')
    return code


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('input', type=Path)
    parser.add_argument('output', type=Path)
    args = parser.parse_args()
    with sqlite3.connect((args.input / 'instructions.sqlite').resolve().as_uri() + '?mode=ro', uri=True) as db:
        records = [json.loads(row[0]) for start, end, _, _ in CONTRACTS for row in db.execute(
            'SELECT body FROM instructions WHERE address>=? AND address<? ORDER BY address', (start, end))]
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(render_reference(records, json.loads((args.input / 'program.json').read_text())['userops']))
    print(f'Emitted {len(records)} original time-runtime instructions')


if __name__ == '__main__':
    main()
