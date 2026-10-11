#!/usr/bin/env python3
"""Emit an isolated complete original reference for compiler arithmetic checks."""
import argparse
import json
import sqlite3
from pathlib import Path
from compile_recomp import generate_page
from compiler_arithmetic_audit import CONTRACTS, require_compiler_arithmetic_bodies


def render_reference(records, userops):
    require_compiler_arithmetic_bodies(records, complete=True)
    code = generate_page(0x10773, records, {r['address'] for r in records}, userops=userops,
                         guest_context_transfers=False, exception_runtime_hooks=False, compiler_arithmetic_hooks=False)
    # All templates have private names. The reference always selects the
    # original body, independent of the current production Turbo preference.
    code = code.replace('page_10773_impl', 'compiler_arithmetic_reference_impl')
    code = code.replace('bool page_10773(State& s)', 'bool compiler_arithmetic_reference(State& s)')
    code = code.replace('compiler_arithmetic_reference_impl<true,', 'compiler_arithmetic_reference_impl<false,')
    if 'bool compiler_arithmetic_reference(State& s)' not in code or 'page_10773_impl' in code:
        raise ValueError('Missing isolated original compiler arithmetic reference')
    return code


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('input', type=Path)
    parser.add_argument('output', type=Path)
    args = parser.parse_args()
    with sqlite3.connect((args.input / 'instructions.sqlite').resolve().as_uri() + '?mode=ro', uri=True) as db:
        records = [json.loads(row[0]) for start, end, _, _ in CONTRACTS
                   for row in db.execute('SELECT body FROM instructions WHERE address>=? AND address<? ORDER BY address', (start, end))]
    metadata = json.loads((args.input / 'program.json').read_text())
    code = render_reference(records, metadata['userops'])
    args.output.parent.mkdir(parents=True, exist_ok=True)
    if not args.output.exists() or args.output.read_text() != code:
        args.output.write_text(code)
    else:
        args.output.touch()
    print(f'Emitted {len(records)} pinned original compiler arithmetic instructions')


if __name__ == '__main__':
    main()
