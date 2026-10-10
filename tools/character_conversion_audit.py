"""Pinned Mac character-conversion contracts, including both classifier pages."""
import hashlib
import json

CONTRACTS = (
    (0x10777f60, 0x1077804c, '18e9c5e242d2a7bc609170c8b8b46a41a8f917b8c9b0d24504e3fe8bfb8478d4'),
    (0x10778050, 0x10778170, 'ee7bbc5ebd365ac62f88d0c618e9ee1fe816970d04ce4dc534235bec448475a6'),
    (0x10778170, 0x10778190, 'f81d79e971a21aa38f0ac88fcd49a741524566e51711a35d9b0ae5f02ae3caf0'),
    (0x10778190, 0x10778234, 'd8f444a8742586897868cba5be5309ced8e2c0eafea674c44c577133d5779db8'),
    (0x10778240, 0x10778260, 'd1bc2f230c1440eabdfae5512df65af3610f5e07148fd0f1b77d9f798ffbd795'),
    (0x10778260, 0x10778300, '7fe292fa32e6a1dfb0820710fdc91ad15d90a518237d5f791b989132d123c37e'),
)
CHARACTER_CONVERSION_ENTRIES = tuple(start for start, _, _ in CONTRACTS)
INSTRUCTION_COUNTS = (59, 72, 8, 41, 8, 40)
CLASSIFIER_PAGE_PINS = {
    0x10777: '55df94060c13da861c4ab8df7454ce2348e9f7bdf4c6b8b72bfa0eb4bce30581',
    0x10778: '0800ad96c12c9c9403049a0459c78e5511c64982035f9029bed3b8303591c82c',
}


def _require(records, expected, start):
    body = [{key: item.get(key) for key in ('address', 'assembly', 'ops', 'flows', 'fallthrough')}
            for item in records]
    actual = hashlib.sha256(json.dumps(body, sort_keys=True, separators=(',', ':')).encode()).hexdigest()
    if actual != expected:
        raise ValueError(f'Mac character conversion contract changed at {start:x}')


def require_character_conversion_contract(page, instructions):
    """Validate complete page fragments, allowing a legitimately split body."""
    for start, end, expected in CONTRACTS:
        if not start >> 12 <= page <= (end - 1) >> 12:
            continue
        records = [r for r in instructions if start <= r['address'] < end]
        if records:
            _require(records, CLASSIFIER_PAGE_PINS[page] if start == 0x10777f60 else expected, start)


def require_character_conversion_bodies(instructions):
    """Validate the full bodies across pages before a completion manifest."""
    for start, end, expected in CONTRACTS:
        records = [r for r in instructions if start <= r['address'] < end]
        if records:
            _require(records, expected, start)


def require_character_conversion_edges(edges):
    for source, target in edges:
        for start, end, _ in CONTRACTS:
            if start <= target < end and not start <= source < end and target != start:
                raise ValueError(f'External character conversion interior edge from {source:x} to {target:x}')


def require_character_conversion_index(db, selected=None):
    if selected is None:
        instructions = [json.loads(row[0]) for row in db.execute(
            'SELECT body FROM instructions WHERE address>=? AND address<? ORDER BY address',
            (CONTRACTS[0][0], max(end for _, end, _ in CONTRACTS)))]
        addresses = {item['address'] for item in instructions}
        for entry in CHARACTER_CONVERSION_ENTRIES:
            if entry not in addresses:
                raise ValueError(f'Missing complete Mac character conversion body at {entry:x}')
    else:
        instructions = [item for _, item in sorted(selected.items())]
    require_character_conversion_bodies(instructions)
    require_character_conversion_edges(db.execute("""SELECT i.address, edge.value
        FROM instructions AS i,
          json_each(json_insert(json_extract(i.body, '$.flows'),
                                '$[#]', json_extract(i.body, '$.fallthrough'))) AS edge
        WHERE edge.value>=? AND edge.value<?""", (CONTRACTS[0][0], max(end for _, end, _ in CONTRACTS))))


def require_character_conversion_hooks(page, instructions, code):
    addresses = {r['address'] for r in instructions}
    for entry in CHARACTER_CONVERSION_ENTRIES:
        if entry >> 12 != page or entry not in addresses:
            continue
        label = f'i_{entry:x}:'
        hook = f'if (try_modern_character_conversion(s, 0x{entry:x})) return true;'
        step = f's.step(0x{entry:x});'
        if code.count(hook) != 1 or label not in code:
            raise ValueError(f'Missing or duplicate character conversion hook at {entry:x}')
        fragment = code.split(label, 1)[1].split(f'i_{entry + 4:x}:', 1)[0]
        if hook not in fragment or step not in fragment or fragment.index(hook) > fragment.index(step):
            raise ValueError(f'Misplaced character conversion hook at {entry:x}')
