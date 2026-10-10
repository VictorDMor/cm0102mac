"""Pinned Mac floating-input contracts, including both parser pages."""
import hashlib
import json

CONTRACTS = ((0x1077de10, 0x1077ee14, 'd8a7ca0ccef635d1d52ac5f2964656fddcd485635321da6552d2668ff9e8c873'),)
FLOATING_INPUT_ENTRIES = (0x1077de10,)
INSTRUCTION_COUNTS = (1025,)
PARSER_PAGE_PINS = {
    0x1077d: '67684ac1e0471dffaa6e72b56d7e2a49480f2853c28e2e45b7b00e942e5793f5',
    0x1077e: '13e429241415e3f20b280993cbe63ceb56b5b17b11ad9e2a77c699f11a997e44',
}


def _require(records, expected, start):
    body = [{key: item.get(key) for key in ('address', 'assembly', 'ops', 'flows', 'fallthrough')}
            for item in records]
    actual = hashlib.sha256(json.dumps(body, sort_keys=True, separators=(',', ':')).encode()).hexdigest()
    if actual != expected:
        raise ValueError(f'Mac floating input contract changed at {start:x}')


def require_floating_input_contract(page, instructions):
    """Validate complete page fragments, allowing a legitimately split body."""
    for start, end, expected in CONTRACTS:
        if not start >> 12 <= page <= (end - 1) >> 12:
            continue
        records = [r for r in instructions if start <= r['address'] < end]
        if records:
            _require(records, PARSER_PAGE_PINS[page] if start == 0x1077de10 else expected, start)


def require_floating_input_bodies(instructions):
    """Validate the full bodies across pages before a completion manifest."""
    for start, end, expected in CONTRACTS:
        records = [r for r in instructions if start <= r['address'] < end]
        if records:
            _require(records, expected, start)


def require_floating_input_edges(edges):
    for source, target in edges:
        for start, end, _ in CONTRACTS:
            if start <= target < end and not start <= source < end and target != start:
                raise ValueError(f'External floating input interior edge from {source:x} to {target:x}')


def require_floating_input_index(db, selected=None):
    if selected is None:
        instructions = [json.loads(row[0]) for row in db.execute(
            'SELECT body FROM instructions WHERE address>=? AND address<? ORDER BY address',
            (CONTRACTS[0][0], max(end for _, end, _ in CONTRACTS)))]
        addresses = {item['address'] for item in instructions}
        for entry in FLOATING_INPUT_ENTRIES:
            if entry not in addresses:
                raise ValueError(f'Missing complete Mac floating input body at {entry:x}')
    else:
        instructions = [item for _, item in sorted(selected.items())]
    require_floating_input_bodies(instructions)
    require_floating_input_edges(db.execute("""SELECT i.address, edge.value
        FROM instructions AS i,
          json_each(json_insert(json_extract(i.body, '$.flows'),
                                '$[#]', json_extract(i.body, '$.fallthrough'))) AS edge
        WHERE edge.value>=? AND edge.value<?""", (CONTRACTS[0][0], max(end for _, end, _ in CONTRACTS))))


def require_floating_input_hooks(page, instructions, code):
    addresses = {r['address'] for r in instructions}
    for entry in FLOATING_INPUT_ENTRIES:
        if entry >> 12 != page or entry not in addresses:
            continue
        label = f'i_{entry:x}:'
        hook = f'if (try_modern_floating_input(s, 0x{entry:x})) return true;'
        step = f's.step(0x{entry:x});'
        if code.count(hook) != 1 or label not in code:
            raise ValueError(f'Missing or duplicate floating input hook at {entry:x}')
        fragment = code.split(label, 1)[1].split(f'i_{entry + 4:x}:', 1)[0]
        if hook not in fragment or step not in fragment or fragment.index(hook) > fragment.index(step):
            raise ValueError(f'Misplaced floating input hook at {entry:x}')
