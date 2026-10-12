#!/usr/bin/env python3
"""Emit complete independent original references for injury withdrawal and position entries."""
import argparse
import inspect
import json
from pathlib import Path
import sqlite3
from compile_recomp import generate_page
from match_actor_audit import CONTRACTS, require_match_actor_bodies


def render_reference(records, userops):
    require_match_actor_bodies(records, complete=True)
    disabled = {name: False for name in inspect.signature(generate_page).parameters
                if name.endswith('_hooks') or name == 'guest_context_transfers'}
    units = []
    for start, end, _, _ in CONTRACTS:
        body = [r for r in records if start <= r['address'] < end]
        for page in range(start >> 12, ((end - 1) >> 12) + 1):
            part = [r for r in body if r['address'] >> 12 == page]
            code = generate_page(page, part, {r['address'] for r in body}, userops=userops, **disabled)
            name = f'match_actor_reference_{start:x}_{page:x}'
            code = code.replace(f'page_{page:x}', name).replace(f'{name}_impl<true,', f'{name}_impl<false,')
            if f'bool {name}(State& s)' not in code or 'try_modern_' in code:
                raise ValueError('Missing isolated original match-actor reference')
            units.append(code)
    units.append('namespace cm::aot { bool match_actor_reference(State& s) {\n' + ''.join(
        f'if (s.pc >= 0x{max(start, page << 12):x} && s.pc < 0x{min(end, (page + 1) << 12):x}) return match_actor_reference_{start:x}_{page:x}(s);\n'
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
    print(f'Emitted {len(records)} original match-actor instructions')


if __name__ == '__main__': main()
