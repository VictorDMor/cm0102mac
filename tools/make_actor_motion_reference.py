#!/usr/bin/env python3
"""Emit five independent whole original-only actor-motion references."""
from contextlib import closing
import argparse
import inspect
import json
from pathlib import Path
import re
import sqlite3
from compile_recomp import generate_page
from actor_motion_audit import BODIES, load_actor_motion, require_actor_motion_contract


def render_reference(records, userops):
    selected = {row['address'] for row in records}
    if any(not any(entry <= address < end for entry, end, *_ in BODIES) for address in selected):
        raise ValueError('Unrelated original actor-motion reference body')
    disabled = {name: False for name in inspect.signature(generate_page).parameters
                if name.endswith('_hooks') or name == 'guest_context_transfers'}
    units = []
    for page in sorted({entry >> 12 for entry, *_ in BODIES}):
        require_actor_motion_contract(page, records)
        part = [row for row in records if row['address'] >> 12 == page]
        name = f'actor_motion_reference_{page:x}'
        code = generate_page(page, part, selected, userops=userops, **disabled).replace(f'page_{page:x}', name)
        code = code.replace(f'{name}_impl<true,', f'{name}_impl<false,')
        if 'try_modern_' in code or f'bool {name}(State& s)' not in code:
            raise ValueError('Missing independent actor-motion reference')
        units.append(code)
    units.append('namespace cm::aot { bool actor_motion_reference(State& s) {\n' + ''.join(
        f'if (s.pc >= 0x{entry:x} && s.pc < 0x{end:x}) return actor_motion_reference_{entry >> 12:x}(s);\n'
        for entry, end, *_ in BODIES) + 'return false; } }\n')
    code = '\n'.join(units)
    code = code.replace('#include "aot_runtime.hpp"', '#include "aot_runtime.hpp"\nnamespace cm::aot { void actor_motion_reference_step_seen(State&, std::uint32_t); void actor_motion_reference_call_seen(State&, std::uint32_t, std::uint32_t); }', 1)
    code = re.sub(r's.step\((0x[0-9a-f]+)\);', r'actor_motion_reference_step_seen(s, \1); s.step(\1);', code)
    for row in records:
        if row['assembly'].startswith('bl '):
            target = int(row['assembly'].split()[1], 16)
            checkpoint = f's.step(0x{row["address"]:x});'
            code = code.replace(checkpoint, f'actor_motion_reference_call_seen(s, 0x{row["address"]:x}, 0x{target:x}); {checkpoint}')
    return code


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('input', type=Path); parser.add_argument('output', type=Path)
    args = parser.parse_args()
    with closing(sqlite3.connect((args.input / 'instructions.sqlite').resolve().as_uri() + '?mode=ro', uri=True)) as db:
        records = load_actor_motion(db)
    args.output.write_text(render_reference(records, json.loads((args.input / 'program.json').read_text())['userops']))
    print('Emitted five complete original motion bodies; no semantic hooks')

if __name__ == '__main__':
    main()
