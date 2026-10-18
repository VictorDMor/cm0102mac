#!/usr/bin/env python3
"""Emit complete independent original references for the daily injury processing."""
import argparse
import inspect
import json
import re
from pathlib import Path
import sqlite3
from compile_recomp import generate_page
from injury_daily_audit import CONTRACTS, require_injury_daily_bodies, load_injury_daily_tables


def render_reference(records, userops):
    require_injury_daily_bodies(records, complete=True)
    disabled = {name: False for name in inspect.signature(generate_page).parameters
                if name.endswith('_hooks') or name == 'guest_context_transfers'}
    units = []
    for start, end, _, _ in CONTRACTS:
        body = [r for r in records if start <= r['address'] < end]
        for page in range(start >> 12, ((end - 1) >> 12) + 1):
            part = [r for r in body if r['address'] >> 12 == page]
            # Keep calls to other roots through the explicit dependency boundary.
            # The root switch and every original instruction remain present.
            code = generate_page(page, part, {r['address'] for r in body if r['address'] != start}, userops=userops, **disabled)
            name = f'injury_daily_reference_{start:x}_{page:x}'
            code = code.replace(f'page_{page:x}', name).replace(f'{name}_impl<true,', f'{name}_impl<false,')
            if f'bool {name}(State& s)' not in code or 'try_modern_' in code:
                raise ValueError('Missing isolated original injury-daily reference')
            units.append(code)
    units.append('namespace cm::aot { bool injury_daily_reference(State& s) {\n' + ''.join(
        f'if (s.pc >= 0x{max(start, page << 12):x} && s.pc < 0x{min(end, (page + 1) << 12):x}) return injury_daily_reference_{start:x}_{page:x}(s);\n'
        for start, end, _, _ in CONTRACTS for page in range(start >> 12, ((end - 1) >> 12) + 1)) + 'return false; } }\n')
    source = '\n'.join(units).replace('#include \"aot_runtime.hpp\"', '#include \"aot_runtime.hpp\"\nnamespace cm::aot { void injury_daily_step_seen(State&, std::uint32_t); }', 1)
    return re.sub(r's.step\((0x[0-9a-f]+)\);', r'injury_daily_step_seen(s, \1); s.step(\1);', source)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('input', type=Path); parser.add_argument('output', type=Path)
    args = parser.parse_args()
    with sqlite3.connect((args.input / 'instructions.sqlite').resolve().as_uri() + '?mode=ro', uri=True) as db:
        records = [json.loads(row[0]) for start, end, _, _ in CONTRACTS for row in db.execute(
            'SELECT body FROM instructions WHERE address>=? AND address<? ORDER BY address', (start, end))]
    args.output.parent.mkdir(parents=True, exist_ok=True)
    source = render_reference(records, json.loads((args.input / 'program.json').read_text())['userops'])
    tables=load_injury_daily_tables(args.input)
    source+='\nnamespace cm::aot { void injury_daily_fixture_tables(State& s, std::uint32_t toc, std::uint32_t base) {\n'
    for offset,words in zip((-0x64d0,-0x543c,-0x5440),tables):
        source+=f's.memory.write<4>(toc + ({offset}), base);\n'
        source+=''.join(f's.memory.write<4>(base + {4*n}, 0x{word:x}u);\n' for n,word in enumerate(words))
        source+='base += 0x1000;\n'
    source+='} }\n'
    args.output.write_text(source)
    print(f'Emitted {len(records)} original injury-daily instructions')


if __name__ == '__main__': main()
