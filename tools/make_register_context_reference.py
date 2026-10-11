#!/usr/bin/env python3
"""Emit all original suffix/capture/restore instructions under private names."""
import argparse
import json
import sqlite3
from pathlib import Path
from compile_recomp import generate_page
from register_context_audit import ROOT_CONTRACTS, require_register_context_bodies


def render_reference(records, userops):
    require_register_context_bodies(records, complete=True)
    code = generate_page(0x10773, records, {r['address'] for r in records}, userops=userops,
                         guest_context_transfers=False, exception_runtime_hooks=False, compiler_arithmetic_hooks=False,
                         register_context_hooks=False)
    code = code.replace('page_10773_impl', 'register_context_reference_impl')
    code = code.replace('bool page_10773(State& s)', 'bool register_context_reference(State& s)')
    code = code.replace('register_context_reference_impl<true,', 'register_context_reference_impl<false,')
    if 'page_10773_impl' in code or 'bool register_context_reference(State& s)' not in code:
        raise ValueError('Missing isolated original register-context reference')
    return code


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('input', type=Path)
    parser.add_argument('output', type=Path)
    args = parser.parse_args()
    with sqlite3.connect((args.input / 'instructions.sqlite').resolve().as_uri() + '?mode=ro', uri=True) as db:
        records = [json.loads(row[0]) for start, end, _, _ in ROOT_CONTRACTS for row in db.execute(
            'SELECT body FROM instructions WHERE address>=? AND address<? ORDER BY address', (start, end))]
    code = render_reference(records, json.loads((args.input / 'program.json').read_text())['userops'])
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(code)
    print(f'Emitted {len(records)} original register-context instructions')


if __name__ == '__main__':
    main()
