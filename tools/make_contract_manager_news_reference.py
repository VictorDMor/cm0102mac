#!/usr/bin/env python3
"""Emit isolated, complete original contract-manager news processing references."""
import argparse
import inspect
import json
from pathlib import Path
import sqlite3

from compile_recomp import generate_page
from contract_manager_audit import ROUTINES, require_contract_manager_index
from tactics_audit import digest

ENTRIES = (0x1008c090, 0x1008c270, 0x1008c4c0, 0x1008ca50, 0x1008f7b0, 0x1008fa40, 0x1008fcf0, 0x10090310, 0x10090560, 0x10090a00, 0x10090f20, 0x100913f0, 0x10091630, 0x100918b0)
ENTRIES += (0x1008a350,)
ENTRIES += (0x1008ab40,)
ENTRIES += (0x1008cd20,)
ENTRIES += (0x1008d450,)
ENTRIES += (0x1008e350,)
ENTRIES += (0x1008ec00,)
FINGERPRINTS = {269009040: '9593216f996d52c7', 269009520: '6271a1636daccd8a', 269010112: '1fd91c32492c0a90', 269011536: 'c8fc3c2b4ea8701d', 269023152: '899e634469b375a3', 269023808: '5d3a743be1f7aae9', 269024496: '885534cb6f8170de', 269026064: '533d2857f613f656', 269026656: '60c2c608ed100e64', 269027840: 'f999c117c1e9850b', 269029152: '34fc68b914ee757a', 269030384: 'b8ecb1fe4bf6774c', 269030960: 'b24543b02e19f936', 269031600: '00fd5eca204c0e7a'}
FINGERPRINTS[0x1008a350] = '0cdf7cf1066355f4'
FINGERPRINTS[0x1008ab40] = 'd6450a0cd038d12d'
FINGERPRINTS[0x1008cd20] = '7011e83db096c5f6'
FINGERPRINTS[0x1008d450] = '2e1bd399ab55337e'
FINGERPRINTS[0x1008e350] = 'abee8b89c70a9de8'
FINGERPRINTS[0x1008ec00] = '8d83e235af88f728'


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
            if digest(records) != FINGERPRINTS[start]:
                raise ValueError("Original news body changed")
            total += len(records)
            calls.extend(r['address'] + 4 for r in records if r['assembly'].startswith('bl '))
            for page in range(start >> 12, ((end - 1) >> 12) + 1):
                part = [r for r in records if r['address'] >> 12 == page]
                code = generate_page(page, part, {r['address'] for r in records if r['address'] != start},
                                     userops=userops, **disabled)
                name = f'contract_news_reference_{start:x}_{page:x}'
                code = code.replace(f'page_{page:x}', name).replace(f'{name}_impl<true,', f'{name}_impl<false,')
                if f'bool {name}(State& s)' not in code or 'try_modern_' in code:
                    raise ValueError('Original news reference contains a replacement hook')
                units.append(code)
                routes.append(f'if (s.pc >= 0x{max(start, page << 12):x} && s.pc < 0x{min(end, (page + 1) << 12):x}) return {name}(s);')
    units.append('namespace cm::aot { bool contract_manager_news_reference(State& s) {\n' +
                 '\n'.join(routes) + '\nreturn false; } }\n')
    units.append('namespace cm::aot { std::vector<std::uint32_t> contract_manager_news_calls() { return {' +
                 ','.join(hex(back) for back in calls) + '}; } }\n')
    # The original period selector jumps through this immutable table.
    segments = [(part['address'], (args.input / part['file']).read_bytes())
                for part in json.loads((args.input / 'program.json').read_text())['segments'] if part.get('file')]
    def read(address, size):
        for base, data in segments:
            if base <= address and address + size <= base + len(data):
                return data[address-base:address-base+size]
        raise ValueError('Missing original news table')
    body = []
    for index, slot in enumerate([-0x574c, -0x5750, -0x5754, -0x5758, -0x575c, -0x5760, -0x5764]):
        address = int.from_bytes(read(0x108bba90 + slot, 4), 'big')
        original = read(address, 32)
        offset = index * 0x40
        body.append(f's.memory.write<4>(toc + std::uint32_t({slot}), table + {offset});')
        body.append(f's.memory.copy_in(table + {offset}, std::vector<std::uint8_t>{{' + ','.join(str(b) for b in original) + '});')
    units.append('namespace cm::aot { void contract_manager_news_constants(State& s, std::uint32_t toc, std::uint32_t table) { ' + ''.join(body) + ' } }')
    constants = int.from_bytes(read(0x108bba90 - 0x571c, 4), 'big')
    original = read(constants, 0x230)
    units.append('namespace cm::aot { void contract_manager_offer_constants(State& s, std::uint32_t toc, std::uint32_t table) { s.memory.write<4>(toc - 0x571c, table); s.memory.copy_in(table, std::vector<std::uint8_t>{' + ','.join(str(b) for b in original) + '}); } }')
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text('\n'.join(units))
    print(f'Emitted {total} original contract-manager news instructions')


if __name__ == '__main__':
    main()
