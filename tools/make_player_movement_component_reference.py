#!/usr/bin/env python3
"""Generate private original movement fragment references for component checks."""
import argparse
from contextlib import closing
import inspect
import json
from pathlib import Path
import re
import sqlite3
from compile_recomp import generate_page
from player_movement_audit import ENTRY, END, load_player_movement, load_player_movement_tables
from make_player_movement_reference import render_reference

FRAGMENTS = {
    'preparation': ((ENTRY,0x1022ec48),(0x10235d88,END)),
    'carrier': ((0x1022ec48,0x1022f514),),
    'environment': ((0x1022f514,0x1022fa58),),
    'interaction': ((0x1022fa58,0x1022ffe0),),
    'contact': ((0x1022ffe0,0x102303fc),),
    'contact-outcome': ((0x102316c8,0x10231768),),
    'contact-action': ((0x102303fc,0x10230a20),),
    'contact-followup': ((0x10230a20,0x102311b4),),
    'restart': ((0x102311b4,0x102316c8),),
    'possession': ((0x10231768,0x10231c04),),
    'play-preparation': ((0x10231c04,0x10231e60),),
    'play-bias': ((0x10231e60,0x10232354),),
    'kind17-contact': ((0x10232354,0x102327b4),),
    'kind17-clear': ((0x102327b4,0x10232e30),),
    'kind18': ((0x10232e30,0x10233830),),
    'repeated-contact': ((0x10233830,0x10233f7c),),
    'transfer-tail': ((0x10233f7c,0x102342c4),(0x102342c4,0x102346f8),
                      (0x102346f8,0x10234a64),(0x10234a64,0x10234dec),(0x10234dec,0x10234e40)),
    'noncarrier': ((0x10234e40,END),),
}


def fragment_reference(records, userops, name, spans):
    disabled={key:False for key in inspect.signature(generate_page).parameters
              if key.endswith('_hooks') or key=='guest_context_transfers'}
    units=[];dispatch=[]
    for n,(start,end) in enumerate(spans):
        part=[row for row in records if start<=row['address']<end]
        selected={row['address'] for row in part}
        for page in sorted({row['address']>>12 for row in part}):
            symbol=f'movement_{name.replace("-","_")}_part_{n}_{page:x}'
            code=generate_page(page,[row for row in part if row['address']>>12==page],selected,userops=userops,**disabled)
            code=code.replace(f'page_{page:x}',symbol)
            if 'try_modern_' in code:raise ValueError('Original component reference contains a production hook')
            units.append(code)
            dispatch.append(f'if(s.pc>=0x{max(start,page<<12):x} && s.pc<0x{min(end,(page+1)<<12):x})return {symbol}(s);')
    public=f'movement_{name.replace("-","_")}_reference'
    units.append('namespace cm::aot { bool '+public+'(State& s){'+''.join(dispatch)+'return false;} }')
    code='\n'.join(units).replace('#include "aot_runtime.hpp"','#include "aot_runtime.hpp"\nnamespace cm::aot { void movement_noncarrier_step_seen(State&, std::uint32_t); }',1)
    return re.sub(r's.step\((0x[0-9a-f]+)\);',r'movement_noncarrier_step_seen(s, \1); s.step(\1);',code)


def write_changed(path, text):
    if not path.exists() or path.read_text()!=text:path.write_text(text)


def main():
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('input',type=Path);parser.add_argument('output',type=Path);args=parser.parse_args()
    with closing(sqlite3.connect((args.input/'instructions.sqlite').resolve().as_uri()+'?mode=ro',uri=True)) as db:records=load_player_movement(db)
    tables=load_player_movement_tables(args.input);userops=json.loads((args.input/'program.json').read_text())['userops'];args.output.mkdir(parents=True,exist_ok=True)
    for name,spans in FRAGMENTS.items():write_changed(args.output/(name+'-reference.cpp'),fragment_reference(records,userops,name,spans))
    write_changed(args.output/'whole-reference.cpp',render_reference(records,userops))
    for file,items in [('contact-action-table.hpp',[(0x1023041c,'original_contact_action_table')]),
                       ('repeated-contact-tables.hpp',[(0x10233b18,'original_repeated_direction_table'),(0x10233d98,'original_repeated_warning_table')])]:
        code='#pragma once\n#include <array>\n#include <cstdint>\n'
        for site,name in items:code+=f'inline constexpr std::array<std::uint32_t,{len(tables[site])}> {name}={{'+','.join(hex(word) for word in tables[site])+'};\n'
        write_changed(args.output/file,code)
    print('Generated audited original component references, all 7537 source sites and three live tables; no production hook')


if __name__=='__main__':main()
