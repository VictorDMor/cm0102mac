#!/usr/bin/env python3
"""Emit the whole original-only movement body for private characterization."""
from contextlib import closing
import argparse
import inspect
import json
from pathlib import Path
import re
import sqlite3
from compile_recomp import generate_page
from player_movement_audit import ENTRY, END, FRAGMENTS, TABLES, load_player_movement, load_player_movement_tables, require_player_movement_body


def render_reference(records, userops):
    require_player_movement_body(records)
    disabled = {name: False for name in inspect.signature(generate_page).parameters
                if name.endswith('_hooks') or name == 'guest_context_transfers'}
    units = []
    for page, _, _ in FRAGMENTS:
        part = [row for row in records if row['address'] >> 12 == page]
        code = generate_page(page, part, {row['address'] for row in records}, userops=userops, **disabled)
        name = f'player_movement_reference_{page:x}'
        code = code.replace(f'page_{page:x}', name).replace(f'{name}_impl<true,', f'{name}_impl<false,')
        if f'bool {name}(State& s)' not in code or 'try_modern_' in code:
            raise ValueError('Missing isolated original movement reference')
        units.append(code)
    units.append('namespace cm::aot { bool player_movement_reference(State& s) {\n' + ''.join(
        f'if (s.pc >= 0x{max(ENTRY, page << 12):x} && s.pc < 0x{min(END, (page + 1) << 12):x}) return player_movement_reference_{page:x}(s);\n'
        for page, _, _ in FRAGMENTS) + 'return false; } }\n')
    code = '\n'.join(units)
    code = code.replace('#include "aot_runtime.hpp"', '#include "aot_runtime.hpp"\nnamespace cm::aot { void player_movement_reference_step_seen(State&, std::uint32_t); void player_movement_reference_call_seen(State&, std::uint32_t, std::uint32_t); void player_movement_reference_jump_seen(State&, std::uint32_t, std::uint32_t); }', 1)
    code = re.sub(r's.step\((0x[0-9a-f]+)\);', r'player_movement_reference_step_seen(s, \1); s.step(\1);', code)
    for row in records:
        checkpoint = f's.step(0x{row["address"]:x});'
        if row['assembly'].startswith('bl '):
            target = int(row['assembly'].split()[1], 16)
            code = code.replace(checkpoint, f'player_movement_reference_call_seen(s, 0x{row["address"]:x}, 0x{target:x}); {checkpoint}')
    for site, *_ in TABLES:
        checkpoint = f's.step(0x{site:x});'
        code = code.replace(checkpoint, f'player_movement_reference_jump_seen(s, 0x{site:x}, std::uint32_t(s.r<4>(4132))); {checkpoint}')
    return code


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('input', type=Path)
    parser.add_argument('output', type=Path)
    args = parser.parse_args()
    with closing(sqlite3.connect((args.input / 'instructions.sqlite').resolve().as_uri() + '?mode=ro', uri=True)) as db:
        records = load_player_movement(db)
    load_player_movement_tables(args.input)
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(render_reference(records, json.loads((args.input / 'program.json').read_text())['userops']))
    print(f'Emitted {len(records)} original movement instructions and three live computed-branch probes; no semantic replacement')


if __name__ == '__main__':
    main()
