#!/usr/bin/env python3
"""Emit independent contact components only after pinning all six complete bodies."""
import argparse
import inspect
import json
from pathlib import Path
import re
import sqlite3
from ball_contact_audit import CONTRACTS, load_ball_contact, load_ball_contact_tables, require_ball_contact_bodies
from compile_recomp import generate_page


def render_reference(records, userops, component):
    require_ball_contact_bodies(records)
    chosen = CONTRACTS[:4] if component == 'facades' else (CONTRACTS[4 if component == 'description' else 5],)
    body = [row for row in records if any(start <= row['address'] < end for start, end, *_ in chosen)]
    disabled = {name: False for name in inspect.signature(generate_page).parameters if name.endswith('_hooks') or name == 'guest_context_transfers'}
    units = []
    for page in sorted({row['address'] >> 12 for row in body}):
        part = [row for row in body if row['address'] >> 12 == page]
        name = f'ball_contact_{component}_reference_{page:x}'
        code = generate_page(page, part, {row['address'] for row in body}, userops=userops, **disabled)
        code = code.replace(f'page_{page:x}', name).replace(f'{name}_impl<true,', f'{name}_impl<false,')
        if f'bool {name}(State& s)' not in code or 'try_modern_' in code:
            raise ValueError('Missing independent contact reference')
        units.append(code)
    units.append(f'namespace cm::aot {{ bool ball_contact_{component}_reference(State& s) {{\n' + ''.join(
        f'if (s.pc >> 12 == 0x{page:x}) return ball_contact_{component}_reference_{page:x}(s);\n'
        for page in sorted({row['address'] >> 12 for row in body})) + 'return false; } }\n')
    code = '\n'.join(units).replace('#include "aot_runtime.hpp"', '#include "aot_runtime.hpp"\nnamespace cm::aot {void ball_contact_step_seen(State&,std::uint32_t);}', 1)
    return re.sub(r's.step\((0x[0-9a-f]+)\);', r'ball_contact_step_seen(s, \1);s.step(\1);', code)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('input', type=Path)
    parser.add_argument('output', type=Path)
    parser.add_argument('--component', choices=('facades', 'description', 'failure'), required=True)
    args = parser.parse_args()
    with sqlite3.connect((args.input / 'instructions.sqlite').resolve().as_uri() + '?mode=ro', uri=True) as db:
        records = load_ball_contact(db)
    load_ball_contact_tables(args.input)
    userops = json.loads((args.input / 'program.json').read_text())['userops']
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(render_reference(records, userops, args.component))
    print(f'Emitted independent {args.component}; all six contact bodies and three tables pinned')


if __name__ == '__main__':
    main()
