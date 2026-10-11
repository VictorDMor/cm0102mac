"""Pinned original contracts for the complete Mac exception-runtime block.

These pins describe the 21-entry reconstruction scope. Production enrollment
requires the complete semantic implementation and its separately checked hooks.
"""
import hashlib
import json

CONTRACTS = (
    (0x10771640, 0x107717f0, '590a84233c6b40288cfd04db93534c188846f25e9db537bfb67cc770762be7a3'),
    (0x107717f0, 0x10771b28, '9e5081cd1f26d9420d08ed666a79d2fe70c5194c413c309ff3cc033b09b3a54a'),
    (0x10771b30, 0x10772430, '9ac1dfc95b04e210e9e8ec964a168a19d99bd09128bb41bc3a2be272e5ba85a9'),
    (0x10772430, 0x107724bc, 'da2965615abb5a7f0984e0b0842aea8b3b833aeef4a96e17158df50e021fd586'),
    (0x107724c0, 0x1077259c, 'c45f442f6916255654005fee0111f889eb47a77c3e3cfd7c567e801c7bb29546'),
    (0x107725a0, 0x107725f4, '9b23f1e26bc76be900efbb636d9d0ba3db4ab95bc3a2e76c02c06be6d385668f'),
    (0x10772600, 0x10772644, '1116230d91edf6d16d8053436bdf2f8c9d1cee2df0c8028fa13ff884fd1e12eb'),
    (0x10772650, 0x10772778, '62eeb36c909ed5401d76b5c629fa8b9bd631da133be8cf711e9d20fd1d975434'),
    (0x10772780, 0x10772808, '2f67019b7351f06be8bdddff031f0af3895c67fc318d316074117916062d810f'),
    (0x10772810, 0x10772ac8, 'bae6135948a6bb7d5558d615da414e9f45283bdd1e8207f19e9ab4afda94f4ce'),
    (0x10772ad0, 0x10772b04, '2e77e0ae5326b6761a627e6fcfc340662a48e126a354e78eb99c00c731bd3eb7'),
    (0x10772b10, 0x10772c9c, '32bfa7e099472410e401127408917555a5cadc7beaadea4585e1e433db65aec8'),
    (0x10772ca0, 0x10772d7c, '69d00d5072489b9057337680f903810df8dbfc98289e8413e955f4d17d7dc959'),
    (0x10772d80, 0x10772dc4, '3ac4ecf21d4af9211e9175b034ec42a7307b6cd8af19367fd4bf6606eeba8b9b'),
    (0x10772dd0, 0x10772dd8, 'f07c4975eec47358ce5b9a203df17983f1b98a54afc29c5f1a872d7219bc69f4'),
    (0x10772de0, 0x10772de8, '3c1383ac8f60830d5c353514fd9cc8f990bf863ac6b387c660f8d332ff51da83'),
    (0x10772e70, 0x10772e90, '31e0b7062310b0e0185ec1540573431cac39ccfb255bc14bff159d66ec0ad13e'),
    (0x10772e90, 0x10772eb0, 'cacd604964a8f3076c3cb4605adc0fe3c6bc632cb6ce2415cfba029f7ee8cd07'),
    (0x10772eb0, 0x10772ed8, '017960f45d4a5866c6af2dec311444581d0b993668bc6d567f431b2820570db8'),
    (0x10772ee0, 0x10772f08, '93d9845268831957c1799fd09624aad27244a27db1638ff3132296c3c12efbb6'),
    (0x10772f10, 0x10773148, 'cefc9032efbb73a8448c43ba1c80620525b6270bb9991b527612bd3e97b60831'),
)
EXCEPTION_RUNTIME_ENTRIES = tuple(start for start, _, _ in CONTRACTS)
INSTRUCTION_COUNTS = (108,206,572,35,55,21,17,74,34,172,13,99,55,17,2,2,8,8,10,10,139,)
PAGE_FRAGMENTS = {
    (0x10771b30, 0x10771): (306, '5db10932a9513cfc79faa00a5603af51b0379644ac459d470e030ceec4691c3b'),
    (0x10771b30, 0x10772): (266, '62a3db43bae81833b9e791d871b050b4ff906ac58e77381c8f5698e571e9161a'),
    (0x10772f10, 0x10772): (59, '2566c01e5e5fc4bfc2b8363217e4e0cf871a2865790df3ba36f7cb73a5012d86'),
    (0x10772f10, 0x10773): (80, 'c6a2aaa52da3d5a0b9d1e29032bd1173a1d4097c493bb35315fa573ca3af405e'),
}


def _require(records, count, expected, start):
    body = [{key: item.get(key) for key in ('address', 'assembly', 'ops', 'flows', 'fallthrough')}
            for item in records]
    actual = hashlib.sha256(json.dumps(body, sort_keys=True, separators=(',', ':')).encode()).hexdigest()
    if len(records) != count or actual != expected:
        raise ValueError(f'Mac exception_runtime contract changed at {start:x}')


def require_exception_runtime_bodies(instructions, complete=False):
    for (start, end, expected), count in zip(CONTRACTS, INSTRUCTION_COUNTS):
        records = [r for r in instructions if start <= r['address'] < end]
        if complete and not records:
            raise ValueError(f'Missing complete Mac exception_runtime body at {start:x}')
        if records:
            _require(records, count, expected, start)


def require_exception_runtime_contract(page, instructions):
    for (start, end, expected), count in zip(CONTRACTS, INSTRUCTION_COUNTS):
        if not start >> 12 <= page <= (end - 1) >> 12:
            continue
        records = [r for r in instructions if start <= r['address'] < end]
        if records:
            fragment_count, digest = PAGE_FRAGMENTS.get((start, page), (count, expected))
            _require(records, fragment_count, digest, start)


def require_exception_runtime_edges(edges):
    for source, target in edges:
        for start, end, _ in CONTRACTS:
            if start < target < end and not start <= source < end:
                raise ValueError(f'External exception_runtime interior edge from {source:x} to {target:x}')


def require_exception_runtime_index(db, selected=None):
    instructions = ([json.loads(row[0]) for row in db.execute(
        'SELECT body FROM instructions WHERE address>=? AND address<? ORDER BY address',
        (CONTRACTS[0][0], CONTRACTS[-1][1]))] if selected is None
        else [item for _, item in sorted(selected.items())])
    require_exception_runtime_bodies(instructions, complete=selected is None)
    require_exception_runtime_edges(db.execute("""SELECT i.address, edge.value
        FROM instructions AS i,
          json_each(json_insert(json_extract(i.body, '$.flows'),
                                '$[#]', json_extract(i.body, '$.fallthrough'))) AS edge
        WHERE edge.value>=? AND edge.value<?""", (CONTRACTS[0][0], CONTRACTS[-1][1])))


def require_exception_runtime_hooks(page, instructions, code):
    addresses = {r['address'] for r in instructions}
    hooks = [(entry, f'if (try_modern_exception_runtime(s, 0x{entry:x})) return true;')
             for entry in EXCEPTION_RUNTIME_ENTRIES]
    hooks.append((0x107724f4, 'if (try_modern_exception_unexpected_landing(s)) return true;'))
    for entry, hook in hooks:
        if entry >> 12 != page or entry not in addresses:
            continue
        label = f'i_{entry:x}:'
        if code.count(hook) != 1 or label not in code:
            raise ValueError(f'Missing or duplicate exception_runtime hook at {entry:x}')
        fragment = code.split(label, 1)[1].split(f'i_{entry + 4:x}:', 1)[0]
        step = f'counter.step(0x{entry:x});' if f'counter.step(0x{entry:x});' in fragment else f's.step(0x{entry:x});'
        if hook not in fragment or step not in fragment or fragment.index(hook) > fragment.index(step):
            raise ValueError(f'Misplaced exception_runtime hook at {entry:x}')
