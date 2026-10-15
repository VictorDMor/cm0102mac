#!/usr/bin/env python3
"""Emit isolated original ball-action components after pinning the whole body.

Partial component checks do not authorize a production replacement. The complete
8,142-instruction boundary includes the tail omitted by inventory body_bytes.
"""
import argparse
import inspect
import json
import re
import sqlite3
from pathlib import Path

from ball_action_audit import ENTRY, load_ball_action, load_ball_action_tables, require_ball_action_body
from compile_recomp import generate_page

COMPONENTS = {'preparation': (ENTRY, 0x1023d184), 'orientation': (0x1023d184, 0x1023d344),
              'facing': (0x1023d344, 0x1023d5e4), 'area': (0x1023d5e4, 0x1023d8f8),
              'elevated': (0x1023d8f8, 0x1023de18), 'low': (0x1023de18, 0x1023e404),
              'selected': (0x1023e404, 0x1023ea44), 'contact': (0x1023ea44, 0x1023ecf4),
              'contact_resolution': (0x1023ecf4, 0x1023f1e8),
              'contact_outcome': (0x1023f1e8, 0x1023fa5c),
              'execution': (0x1023fa5c, 0x1023ff48),
              'execution_message': (0x1023ff48, 0x10240178),
              'receiver': (0x10240178, 0x1024060c),
              'receiver_outcome': (0x1024060c, 0x10240ff8),
              'result': (0x10240ff8, 0x10241454),
              'result_message': (0x10241454, 0x10241680),
              'result_resolution': (0x10241680, 0x10241b24),
              'result_outcome': (0x10241b24, 0x102424ac),
              'unopposed': (0x102424ac, 0x102425f4),
              'final_preparation': (0x102425f4, 0x102426c8),
              'special_effects': (0x102426c8, 0x10242cc0),
              'regular_effects': (0x10242cc0, 0x10242f3c),
              'success_reaction': (0x10242f3c, 0x102433d4),
              'failed_preparation': (0x102433d4, 0x1024366c),
              'failed_message': (0x1024366c, 0x1024389c),
              'failed_comment': (0x1024389c, 0x102439d0),
              'failed_fallback': (0x102439d0, 0x10243ba4)}


def render_reference(records, userops, component):
    require_ball_action_body(records)
    start, end = COMPONENTS[component]
    body = [row for row in records if start <= row['address'] < end]
    disabled = {name: False for name in inspect.signature(generate_page).parameters
                if name.endswith('_hooks') or name == 'guest_context_transfers'}
    units = []
    for page in range(start >> 12, ((end - 1) >> 12) + 1):
        part = [row for row in body if row['address'] >> 12 == page]
        name = f'ball_action_{component}_reference_{page:x}'
        code = generate_page(page, part, {row['address'] for row in body}, userops=userops, **disabled)
        code = code.replace(f'page_{page:x}', name).replace(f'{name}_impl<true,', f'{name}_impl<false,')
        if f'bool {name}(State& s)' not in code or 'try_modern_' in code:
            raise ValueError('Missing isolated original ball-action reference')
        units.append(code)
    units.append(f'namespace cm::aot {{ bool ball_action_{component}_reference(State& s) {{\n' + ''.join(
        f'if (s.pc >= 0x{max(start, page << 12):x} && s.pc < 0x{min(end, (page + 1) << 12):x}) return ball_action_{component}_reference_{page:x}(s);\n'
        for page in range(start >> 12, ((end - 1) >> 12) + 1)) + 'return false; } }\n')
    source = '\n'.join(units).replace('#include "aot_runtime.hpp"', '#include "aot_runtime.hpp"\nnamespace cm::aot { void ball_action_step_seen(State&, std::uint32_t); }', 1)
    return re.sub(r's.step\((0x[0-9a-f]+)\);', r'ball_action_step_seen(s, \1); s.step(\1);', source)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('input', type=Path)
    parser.add_argument('output', type=Path)
    parser.add_argument('--component', choices=COMPONENTS, required=True)
    args = parser.parse_args()
    with sqlite3.connect((args.input / 'instructions.sqlite').resolve().as_uri() + '?mode=ro', uri=True) as db:
        records = load_ball_action(db)
    tables = load_ball_action_tables(args.input)
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(render_reference(records, json.loads((args.input / 'program.json').read_text())['userops'], args.component))
    if args.component == 'execution_message':
        (args.output.parent / 'execution-message-table.hpp').write_text(
            '#pragma once\n#include <array>\n#include <cstdint>\n'
            'inline constexpr std::array<std::uint32_t,47> original_execution_message_table{{' +
            ','.join(f'0x{word:08x}' for word in tables[0x1023ff68]) + '}};\n')
    if args.component == 'result_message':
        (args.output.parent / 'result-message-table.hpp').write_text(
            '#pragma once\n#include <array>\n#include <cstdint>\n'
            'inline constexpr std::array<std::uint32_t,57> original_result_message_table{{' +
            ','.join(f'0x{word:08x}' for word in tables[0x10241470]) + '}};\n')
    if args.component == 'failed_message':
        source='#pragma once\n#include <array>\n#include <cstdint>\n'
        for name,site in [('action',0x1024368c),('comment',0x10243918),('fallback',0x102439f0)]:
            words=tables[site]
            source+=f'inline constexpr std::array<std::uint32_t,{len(words)}> original_failed_{name}_table{{{{'+','.join(f'0x{word:08x}' for word in words)+'}};\n'
        (args.output.parent/'failed-message-tables.hpp').write_text(source)
    start, end = COMPONENTS[args.component]
    print(f'Emitted {(end-start)//4} original {args.component} instructions; complete ball-action body and five tables pinned')


if __name__ == '__main__':
    main()
