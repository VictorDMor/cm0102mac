"""Pinned Mac integer-parser contracts, including the word32 page crossing."""
import hashlib
import json

CONTRACTS = (
    (0x1077ee20, 0x1077f180, '0e6f94e1876cc0214da11fe6867d125dc06d073581ecc399217f3bd399394098'),
    (0x1077f180, 0x1077f564, 'b99ed550fab592d2395d29a82f8631d3e08c0c3fa71574c256661d41ba1e8e63'),
    (0x1077f570, 0x1077f664, '0221610ecf6b5d56509dc7fae6948e5b117b2bd895c9bd3d8e413459597f32a7'),
    (0x1077f670, 0x1077f698, '2e45e76b5215652dce79a0ecc6023e610fef5053cb06f9800fd9424152783eb1'),
    (0x1077f6a0, 0x1077f6c8, '6a14b3d0e2dd4d19b0c5a4af9fd370a2e6b9436c4534fe0bff445a3168e609e9'),
)
INTEGER_PARSING_ENTRIES = tuple(start for start, _, _ in CONTRACTS)
INSTRUCTION_COUNTS = (215, 249, 61, 10, 10)
# Independently pinned complete fragments, never compared to the whole-body hash.
WORD32_PAGE_PINS = {
    0x1077e: '11881c7e1f44a621216da27d935b2042ea62428a813fad0fe0454e2ab9b32611',
    0x1077f: 'bd7db53412fb261887b57315819b920244237336db58104aa87b997112dce52d',
}


def _require(records, expected, start):
    body = [{key: item.get(key) for key in ('address', 'assembly', 'ops', 'flows', 'fallthrough')}
            for item in records]
    actual = hashlib.sha256(json.dumps(body, sort_keys=True, separators=(',', ':')).encode()).hexdigest()
    if actual != expected:
        raise ValueError(f'Mac integer parsing contract changed at {start:x}')


def require_integer_parsing_contract(page, instructions):
    """Validate complete page fragments, allowing a legitimately split body."""
    for start, end, expected in CONTRACTS:
        if not start >> 12 <= page <= (end - 1) >> 12:
            continue
        records = [r for r in instructions if start <= r['address'] < end]
        if records:
            _require(records, WORD32_PAGE_PINS[page] if start == 0x1077ee20 else expected, start)


def require_integer_parsing_bodies(instructions):
    """Validate the full bodies across pages before a completion manifest."""
    for start, end, expected in CONTRACTS:
        records = [r for r in instructions if start <= r['address'] < end]
        if records:
            _require(records, expected, start)


def require_integer_parsing_edges(edges):
    for source, target in edges:
        for start, end, _ in CONTRACTS:
            if start <= target < end and not start <= source < end and target != start:
                raise ValueError(f'External integer parsing interior edge from {source:x} to {target:x}')


def require_integer_parsing_index(db, selected=None):
    if selected is None:
        instructions = [json.loads(row[0]) for row in db.execute(
            'SELECT body FROM instructions WHERE address>=? AND address<? ORDER BY address',
            (CONTRACTS[0][0], CONTRACTS[-1][1]))]
    else:
        instructions = [item for _, item in sorted(selected.items())]
    require_integer_parsing_bodies(instructions)
    require_integer_parsing_edges(db.execute("""SELECT i.address, edge.value
        FROM instructions AS i,
          json_each(json_insert(json_extract(i.body, '$.flows'),
                                '$[#]', json_extract(i.body, '$.fallthrough'))) AS edge
        WHERE edge.value>=? AND edge.value<?""", (CONTRACTS[0][0], CONTRACTS[-1][1])))


def require_integer_parsing_hooks(page, instructions, code):
    addresses = {r['address'] for r in instructions}
    for entry in INTEGER_PARSING_ENTRIES:
        if entry >> 12 != page or entry not in addresses:
            continue
        label = f'i_{entry:x}:'
        hook = f'if (try_modern_integer_parsing(s, 0x{entry:x})) return true;'
        step = f's.step(0x{entry:x});'
        if code.count(hook) != 1 or label not in code:
            raise ValueError(f'Missing or duplicate integer parsing hook at {entry:x}')
        fragment = code.split(label, 1)[1].split(f'i_{entry + 4:x}:', 1)[0]
        if hook not in fragment or step not in fragment or fragment.index(hook) > fragment.index(step):
            raise ValueError(f'Misplaced integer parsing hook at {entry:x}')
