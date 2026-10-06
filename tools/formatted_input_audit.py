"""Pinned Mac formatted-input contracts, including all three scanner pages."""
import hashlib
import json

CONTRACTS = (
    (0x1077b7a0, 0x1077bf20, '1f92666f58a9485bc5f015c13c3c3942fc756a123d6270a52717c6866e025c9a'),
    (0x1077bf20, 0x1077d8cc, 'a8d9ca1f82d1dae1139544ce0bf135f361b2f6eae0cbef423c3d6e7fc5cb5108'),
    (0x1077d960, 0x1077d9f0, '9d8bfe52bc79d144184931008bacb7767f6d95c36ebe2a3b92a70c23139e7967'),
    (0x1077da70, 0x1077dac8, 'd247cbccf4af1cd8df61149e1c50d024a4186777f0504193a0d9e7ee07980e12'),
    (0x1077dad0, 0x1077db14, '7db0c98c7415dc19f6e9a6df73e14b24517a032d2c9125b1aa833485934ea585'),
    (0x1077d8d0, 0x1077d954, '862b4f73bb03b4db4aae69453b16d81232966f15819433dc28b397eb5c5d5d19'),
    (0x1077d9f0, 0x1077da6c, 'b840d438996ce92f30b4dda4c874385828932d89549225139ed261692923c91b'),
)
FORMATTED_INPUT_ENTRIES = tuple(start for start, _, _ in CONTRACTS)
INSTRUCTION_COUNTS = (478, 1631, 36, 22, 17, 33, 31)
SCANNER_PAGE_PINS = {
    0x1077b: 'b4e3299368839bba2f7db7d215ad64a4606a977bfc372aedffa64604e835b824',
    0x1077c: '30f13561b7b1953663678bea999776d063c76b56117e61ec08fcbac44d49a28e',
    0x1077d: 'cecc5cb7384f46ac586e121cb142fb8af11b7e2a7c199ec01774227bbe7dce48',
}


def _require(records, expected, start):
    body = [{key: item.get(key) for key in ('address', 'assembly', 'ops', 'flows', 'fallthrough')}
            for item in records]
    actual = hashlib.sha256(json.dumps(body, sort_keys=True, separators=(',', ':')).encode()).hexdigest()
    if actual != expected:
        raise ValueError(f'Mac formatted input contract changed at {start:x}')


def require_formatted_input_contract(page, instructions):
    """Validate complete page fragments, allowing a legitimately split body."""
    for start, end, expected in CONTRACTS:
        if not start >> 12 <= page <= (end - 1) >> 12:
            continue
        records = [r for r in instructions if start <= r['address'] < end]
        if records:
            _require(records, SCANNER_PAGE_PINS[page] if start == 0x1077bf20 else expected, start)


def require_formatted_input_bodies(instructions):
    """Validate the full bodies across pages before a completion manifest."""
    for start, end, expected in CONTRACTS:
        records = [r for r in instructions if start <= r['address'] < end]
        if records:
            _require(records, expected, start)


def require_formatted_input_edges(edges):
    for source, target in edges:
        for start, end, _ in CONTRACTS:
            if start <= target < end and not start <= source < end and target != start:
                raise ValueError(f'External formatted input interior edge from {source:x} to {target:x}')


def require_formatted_input_index(db, selected=None):
    if selected is None:
        instructions = [json.loads(row[0]) for row in db.execute(
            'SELECT body FROM instructions WHERE address>=? AND address<? ORDER BY address',
            (CONTRACTS[0][0], max(end for _, end, _ in CONTRACTS)))]
        addresses = {item['address'] for item in instructions}
        for entry in FORMATTED_INPUT_ENTRIES:
            if entry not in addresses:
                raise ValueError(f'Missing complete Mac formatted input body at {entry:x}')
    else:
        instructions = [item for _, item in sorted(selected.items())]
    require_formatted_input_bodies(instructions)
    require_formatted_input_edges(db.execute("""SELECT i.address, edge.value
        FROM instructions AS i,
          json_each(json_insert(json_extract(i.body, '$.flows'),
                                '$[#]', json_extract(i.body, '$.fallthrough'))) AS edge
        WHERE edge.value>=? AND edge.value<?""", (CONTRACTS[0][0], max(end for _, end, _ in CONTRACTS))))


def require_formatted_input_hooks(page, instructions, code):
    addresses = {r['address'] for r in instructions}
    for entry in FORMATTED_INPUT_ENTRIES:
        if entry >> 12 != page or entry not in addresses:
            continue
        label = f'i_{entry:x}:'
        hook = f'if (try_modern_formatted_input(s, 0x{entry:x})) return true;'
        step = f's.step(0x{entry:x});'
        if code.count(hook) != 1 or label not in code:
            raise ValueError(f'Missing or duplicate formatted input hook at {entry:x}')
        fragment = code.split(label, 1)[1].split(f'i_{entry + 4:x}:', 1)[0]
        if hook not in fragment or step not in fragment or fragment.index(hook) > fragment.index(step):
            raise ValueError(f'Misplaced formatted input hook at {entry:x}')
