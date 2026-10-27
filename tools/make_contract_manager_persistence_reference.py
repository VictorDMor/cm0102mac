#!/usr/bin/env python3
"""Emit isolated, complete original contract-manager load/save references."""
import argparse
import inspect
import json
from pathlib import Path
import sqlite3

from compile_recomp import generate_page
from contract_manager_audit import ROUTINES, require_contract_manager_index

ENTRIES = (0x10091bf0, 0x10093030)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('input', type=Path)
    parser.add_argument('output', type=Path)
    args = parser.parse_args()
    disabled = {name: False for name in inspect.signature(generate_page).parameters
                if name.endswith('_hooks') or name == 'guest_context_transfers'}
    userops = json.loads((args.input / 'program.json').read_text())['userops']
    units, routes, calls, total = [], [], [], 0
    with sqlite3.connect((args.input / 'instructions.sqlite').resolve().as_uri() + '?mode=ro', uri=True) as db:
        require_contract_manager_index(db)
        for start, end in ROUTINES:
            if start not in ENTRIES:
                continue
            records = [json.loads(row[0]) for row in db.execute(
                'SELECT body FROM instructions WHERE address>=? AND address<? ORDER BY address', (start, end))]
            total += len(records)
            calls.extend(r['address'] + 4 for r in records if r['assembly'].startswith('bl '))
            for page in range(start >> 12, ((end - 1) >> 12) + 1):
                part = [r for r in records if r['address'] >> 12 == page]
                code = generate_page(page, part, {r['address'] for r in records if r['address'] != start},
                                     userops=userops, **disabled)
                name = f'contract_persistence_reference_{start:x}_{page:x}'
                code = code.replace(f'page_{page:x}', name).replace(f'{name}_impl<true,', f'{name}_impl<false,')
                if f'bool {name}(State& s)' not in code or 'try_modern_' in code:
                    raise ValueError('Original persistence reference contains a replacement hook')
                units.append(code)
                routes.append(f'if (s.pc >= 0x{max(start, page << 12):x} && s.pc < 0x{min(end, (page + 1) << 12):x}) return {name}(s);')
    units.append('namespace cm::aot { bool contract_manager_persistence_reference(State& s) {\n' +
                 '\n'.join(routes) + '\nreturn false; } }\n')
    units.append('namespace cm::aot { std::vector<std::uint32_t> contract_manager_persistence_calls() { return {' +
                 ','.join(hex(back) for back in calls) + '}; } }\n')
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text('\n'.join(units))
    print(f'Emitted {total} original contract-manager persistence instructions')


if __name__ == '__main__':
    main()
