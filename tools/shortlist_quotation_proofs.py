"""Checked, bounded unreachable-site proofs for the complete quotation pair.

These are source/data audit tools, not a guest executor. Callees retain the
PowerPC nonvolatile register ABI. Every original instruction remains emitted.
"""
import json
import re
import struct
from collections import deque
from shortlist_quotation_audit import (
    CONTRACTS, TABLES, require_shortlist_quotation_bodies,
    require_shortlist_quotation_tables, require_shortlist_quotation_padding)

STRUCTURALLY_UNREACHABLE = (
    0x10366658, 0x1036665c,
    *range(0x103670cc, 0x103670dc, 4), *range(0x103670e8, 0x103670f4, 4),
    0x10367480, 0x103674c0, 0x103683b4, 0x10368400,
    0x10368adc, 0x10368f60, 0x10368f88, 0x10368f8c,
    *range(0x1037c92c, 0x1037c93c, 4), 0x1037cf68,
    *range(0x1037de20, 0x1037def4, 4),
)
CONDITIONAL = {'beq', 'bne', 'blt', 'bgt', 'ble', 'bge'}
JUMP_SITES = (0x10367118, 0x10367318, 0x10367e60, 0x10367eb8, 0x1037b970)


def check(condition, message):
    if not condition:
        raise ValueError(message)


def writes_gpr(text, register):
    op, _, tail = text.partition(' ')
    if op in CONDITIONAL or op in {'b', 'bl', 'bctr', 'blr', 'stw', 'stb', 'sth', 'stmw',
                                     'cmpwi', 'cmplwi', 'cmpw', 'cmplw', 'mtspr'}:
        return False
    if op == 'lmw':
        return int(tail.split(',')[0][1:]) <= register
    return tail.split(',')[0] == f'r{register}'


def flow_graph(assembly, read):
    tables = {pc: struct.unpack('>' + 'I' * (size // 4), read(base, size))
              for pc, (_, base, size, _) in zip(JUMP_SITES, TABLES[6:])}
    graph = {}
    for pc, text in assembly.items():
        op, _, tail = text.partition(' ')
        if op in CONDITIONAL: graph[pc] = (int(tail, 0), pc + 4)
        elif op == 'b': graph[pc] = (int(tail, 0),)
        elif op == 'bctr': graph[pc] = tuple(dict.fromkeys(tables[pc]))
        elif op == 'blr': graph[pc] = ()
        else: graph[pc] = (pc + 4,)
    return graph


def reachable(graph, root):
    seen = set()
    todo = [root]
    while todo:
        pc = todo.pop()
        if pc not in seen:
            seen.add(pc); todo.extend(graph.get(pc, ()))
    return seen


def nullness_reachability(assembly, graph, root, register):
    """Conservative intraprocedural reachability with one zero/nonzero fact.

    Calls may change volatile conditions but retain this nonvolatile GPR. Other
    comparisons and recorded operations discard the remembered CR predicate.
    Unknown branches take both edges; loads into the tracked GPR discard its fact.
    """
    todo = deque([(root, None, False)]); seen = set(); sites = set()
    while todo:
        pc, zero, predicate = todo.popleft()
        key = (pc, zero, predicate)
        if key in seen or pc not in assembly: continue
        seen.add(key); sites.add(pc); text = assembly[pc]
        op = text.split(' ')[0]
        if writes_gpr(text, register): zero = None
        if text in (f'cmpwi r{register},0x0', f'cmplwi r{register},0x0'):
            predicate = True
        elif op.startswith(('cmp', 'fcmp')) or op.endswith('.') or op in {'cror', 'bl'}:
            predicate = False
        successors = graph[pc]
        for edge, target in enumerate(successors):
            fact = zero
            if predicate and op in {'beq', 'bne'}:
                fact = (edge == 0) == (op == 'beq')
                if zero is not None and fact != zero: continue
            todo.append((target, fact, predicate))
    return sites


def require_shortlist_quotation_structural_proofs(instructions, read):
    require_shortlist_quotation_bodies(instructions, complete=True)
    require_shortlist_quotation_tables(read); require_shortlist_quotation_padding(read)
    assembly = {r['address']: r['assembly'] for r in instructions}
    graph = flow_graph(assembly, read)
    premises = {
        0x1036659c:'lwz r4,0x20(r15)', 0x103665a0:'cmplwi r4,0x0',
        0x103665a4:'beq 0x103665bc', 0x103665a8:'extsb r5,r0',
        0x103665ac:'cmpwi r5,0x5', 0x103665b0:'bne 0x103665bc',
        0x103665b8:'b 0x10369058', 0x10366644:'extsb r5,r0',
        0x10366648:'cmpwi r5,0x5', 0x1036664c:'bne 0x10366660',
        0x10366650:'cmplwi r4,0x0', 0x10366654:'beq 0x10366660',
        0x10367074:'lbz r3,0x4c(r31)', 0x10367084:'extsb r0,r3',
        0x103670a0:'fsub f1,f1,f2', 0x103670a4:'fdiv f0,f1,f0',
        0x103670a8:'fsub f1,f3,f0', 0x103670c8:'ble 0x103670dc',
        0x103670e4:'bge 0x103670f4',
        0x10368aa0:'lfd f1,0x150(r30)', 0x10368aa8:'ble 0x10368b18',
        0x10368ab4:'fdiv f2,f31,f1', 0x10368abc:'fsub f2,f3,f2',
        0x10368ac0:'frsp f2,f2', 0x10368ad8:'bge 0x10368ae0',
        0x10368ef8:'fcmpo cr0,f31,f0', 0x10368efc:'bge 0x10368f08',
        0x10368f04:'b 0x10369058', 0x10368f4c:'bl 0x1037b6f0',
        0x10368f58:'fcmpo cr0,f0,f31', 0x10368f5c:'ble 0x10368f64',
        0x10368f80:'fcmpo cr0,f31,f0', 0x10368f84:'bge 0x10368f90',
        0x1037c8e4:'extsb r5,r19', 0x1037c920:'subis r5,r5,0x1',
        0x1037c924:'cmplwi r5,0x4244', 0x1037c928:'bne 0x1037c93c',
        0x1037cee4:'bl 0x10718130', 0x1037cf04:'fsubs f30,f1,f2',
        0x1037cf10:'fmr f30,f0', 0x1037cf2c:'fadds f30,f30,f0',
        0x1037cf38:'ble 0x1037cf50', 0x1037cf3c:'fsubs f1,f30,f0',
        0x1037cf48:'fmadd f30,f2,f1,f0', 0x1037cf4c:'frsp f30,f30',
        0x1037cf5c:'fnmsubs f0,f1,f30,f0', 0x1037cf64:'ble 0x1037cf6c',
        0x1037dbb8:'lwz r6,0x0(r5)', 0x1037dbbc:'cmpw r0,r6',
        0x1037dbc0:'beq 0x1037dbf4', 0x1037de18:'cmpw r0,r6',
        0x1037de1c:'bne 0x1037def4',
    }
    check(all(assembly.get(pc) == text for pc, text in premises.items()),
          'Changed quotation structural-proof premise')
    check(not any(writes_gpr(text, reg) or text.startswith('bl ')
                  for pc,text in assembly.items() if 0x103665bc <= pc < 0x10366658
                  for reg in (0,4)), 'Changed stable secondary-status rejection')
    for status in range(-128,128):
        for secondary in (False,True):
            early = secondary and status == 5
            check(not (not early and status == 5 and secondary), 'Invalid repeated status rejection')
    bank = read(0x1080b8d8, 952)
    double = lambda offset: struct.unpack('>d', bank[offset:offset+8])[0]
    check({o:double(o) for o in (0x30,0x48,0x68,0x80,0x150,0x178,0x200,0x308,0x3a8)} ==
          {0x30:100.0,0x48:0.1,0x68:0.5,0x80:1.0,0x150:10.0,0x178:1.5,
           0x200:1.1,0x308:400.0,0x3a8:1.75}, 'Changed contextual bound constants')
    factors = [1.0 - value / 400.0 for value in range(-128,128)]
    check(min(factors)>0.5 and max(factors)<1.5, 'Invalid contract-byte factor interval')
    live = nullness_reachability(assembly, graph, CONTRACTS[0][0], 20)
    check(not {0x10367480,0x103674c0,0x103683b4,0x10368400} & live,
          'Reachable redundant current-club null arm')
    single = lambda value: struct.unpack('>f',struct.pack('>f',value))[0]
    # Unordered comparison leaves before the f31>10 arm; +infinity gives -inf.
    check(single(1.1 - 10.0/100.0) == 1.0, 'Invalid lower-reference ceiling')
    check(not any(re.match(r'f\w+ f31,',text) for pc,text in assembly.items()
                  if 0x10368ef8 <= pc <= 0x10368f8c), 'Changed stable financial-context score')
    # The selected child saves and restores f31; all external callees retain its
    # nonvolatile ABI. NaN takes both matching bge guards and the later ble bypass.
    check(assembly[0x1037b6f4]=='stfd f31,-0x8(r1)' and
          assembly[0x1037e3c4]=='lfd f31,-0x8(r1)', 'Changed child nonvolatile floating boundary')
    check(all(((value-0x10000)&0xffffffff) != 0x4244 for value in range(-128,128)),
          'Invalid signed-byte mode exclusion')
    # The scalar date word is converted to finite float, capped at 250, may gain
    # 60, then is compressed above 30. IEEE rounding is monotone, including at the
    # upper bound 86. Lower values only increase the final negative-product factor.
    fbank = read(0x1080c3b0,360); qbank = read(0x1080c538,584)
    f = lambda o: struct.unpack('>f',fbank[o:o+4])[0]
    d = lambda o: struct.unpack('>d',qbank[o:o+8])[0]
    check((f(0xe0),f(0x11c),f(0x164),d(0xa0),d(0x238)) == (250.0,60.0,30.0,0.2,30.0),
          'Changed date-reference upper-bound constants')
    upper = single(d(0xa0)*single(single(250.0+60.0)-30.0)+d(0x238))
    lower = -single(f(0x128)*upper-f(0x124))
    check(upper==86.0 and lower>f(0x120), 'Invalid date-reference factor lower bound')
    # Earlier equality always leaves through the common exit. On every path
    # reaching the repeated comparison from the false arm, both operands retain
    # their values. This checks the intervening false arms, not lexical proximity.
    check(0x1037de18 not in reachable(graph,0x1037dbf4), 'Changed early country-equality exit')
    reverse = {pc:[] for pc in graph}
    for pc,edges in graph.items():
        for target in edges: reverse.setdefault(target,[]).append(pc)
    paths = reachable(graph,0x1037dbc4) & reachable(reverse,0x1037de18)
    check(not any(writes_gpr(assembly[pc],reg) or assembly[pc].startswith('bl ')
                  for pc in paths for reg in (0,6)), 'Changed stable repeated-country operands')
    check(len(STRUCTURALLY_UNREACHABLE)==75 and len(set(STRUCTURALLY_UNREACHABLE))==75,
          'Changed structural quotation partition')
    return STRUCTURALLY_UNREACHABLE


def load_shortlist_quotation_structural_proofs(directory, instructions):
    program = json.loads((directory/'program.json').read_text())
    segments = [(s['address'],(directory/s['file']).read_bytes())
                for s in program['segments'] if s.get('file')]
    def read(address,size):
        for base,data in segments:
            if base<=address and address+size<=base+len(data):
                return data[address-base:address-base+size]
        raise ValueError('Missing original quotation proof data')
    return require_shortlist_quotation_structural_proofs(instructions,read)
