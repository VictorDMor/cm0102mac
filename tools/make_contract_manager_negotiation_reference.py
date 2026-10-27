#!/usr/bin/env python3
"""Emit isolated, complete original contract-manager negotiation processing references."""
import argparse
import inspect
import json
from pathlib import Path
import sqlite3

from compile_recomp import generate_page
from contract_manager_audit import ROUTINES, require_contract_manager_index
from tactics_audit import digest

ENTRIES = (0x10085860,)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('input', type=Path)
    parser.add_argument('output', type=Path)
    parser.add_argument('--stop-at', type=lambda value: int(value, 0), action='append', default=[],
                        help='Explicit partial boundary before this instruction; may be repeated')
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
            if digest(records) != 'b2c9569b93bea6d7':
                raise ValueError('Original negotiation body changed')
            if not set(args.stop_at) <= {r['address'] for r in records}:
                raise ValueError('Partial-check boundary outside original negotiation instructions')
            total += len(records)
            calls.extend(r['address'] + 4 for r in records if r['assembly'].startswith('bl '))
            for page in range(start >> 12, ((end - 1) >> 12) + 1):
                part = [r for r in records if r['address'] >> 12 == page]
                code = generate_page(page, part, {r['address'] for r in records if r['address'] != start},
                                     userops=userops, **disabled)
                name = f'contract_negotiation_reference_{start:x}_{page:x}'
                code = code.replace(f'page_{page:x}', name).replace(f'{name}_impl<true,', f'{name}_impl<false,')
                for stop in sorted(set(args.stop_at) & {r['address'] for r in part}):
                    label = f'i_{stop:x}:'
                    if code.count(label) != 1:
                        raise ValueError('Original partial-check boundary label unavailable')
                    code = code.replace(label, label + f'\n s.pc = 0x{stop:x}; return true;\n')
                if f'bool {name}(State& s)' not in code or 'try_modern_' in code:
                    raise ValueError('Original negotiation reference contains a replacement hook')
                units.append(code)
                routes.append(f'if (s.pc >= 0x{max(start, page << 12):x} && s.pc < 0x{min(end, (page + 1) << 12):x}) return {name}(s);')
    units.append('namespace cm::aot { bool contract_manager_negotiation_reference(State& s) {\n' +
                 '\n'.join(routes) + '\nreturn false; } }\n')
    units.append('namespace cm::aot { std::vector<std::uint32_t> contract_manager_negotiation_calls() { return {' +
                 ','.join(hex(back) for back in calls) + '}; } }\n')
    units.append('namespace cm::aot { std::vector<std::uint32_t> contract_manager_negotiation_stops() { return {' +
                 ','.join(hex(stop) for stop in sorted(set(args.stop_at))) + '}; } }\n')
    args.output.parent.mkdir(parents=True, exist_ok=True)
    segments = [(part['address'], (args.input / part['file']).read_bytes())
                for part in json.loads((args.input / 'program.json').read_text())['segments'] if part.get('file')]
    def read(address, size):
        for base, data in segments:
            if base <= address and address + size <= base + len(data):
                return data[address-base:address-base+size]
        raise ValueError('Missing original offer policy constants')
    body = []
    for slot, size, parameter in [(-0x571c, 0x410, 'doubles'), (-0x5734, 0x34, 'singles'), (-0x5744, 32, 'squad_role_table'), (-0x5748, 68, 'age_limits')]:
        address = int.from_bytes(read(0x108bba90 + slot, 4), 'big')
        table = read(address, size)
        body.append(f's.memory.write<4>(toc + std::uint32_t({slot}), {parameter});')
        body.append(f's.memory.copy_in({parameter}, std::vector<std::uint8_t>{{' + ','.join(str(b) for b in table) + '});')
    # Preserve the neighbouring original bytes too: the signed reputation
    # quotient can index on either side of the nominal reputation domain.
    financial_address = int.from_bytes(read(0x108bba90 - 0x5740, 4), 'big')
    financial_bytes = read(financial_address - 0x400, 0x810)
    body.append('s.memory.write<4>(toc - 0x5740, finances + 0x400);')
    body.append('s.memory.copy_in(finances, std::vector<std::uint8_t>{' + ','.join(str(b) for b in financial_bytes) + '});')
    units.append('namespace cm::aot { void contract_manager_negotiation_constants(State& s, std::uint32_t toc, std::uint32_t doubles, std::uint32_t singles, std::uint32_t squad_role_table, std::uint32_t age_limits, std::uint32_t finances) {\n' + '\n'.join(body) + '\n} }\n')
    args.output.write_text('\n'.join(units))
    print(f'Emitted {total} original negotiation instructions; explicit partial stops: {args.stop_at!r}')


if __name__ == '__main__':
    main()
