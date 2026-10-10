"""Pinned Mac formatted-output bodies, page fragments and shared reset edge."""
import hashlib
import json

CONTRACTS = (
    (0x10778db0, 0x1077961c, '9d77e7c9292f155e8d5f82dbae9a95eac10fa25b31809bf25a8335d2b715d018'),
    (0x10779620, 0x10779874, 'd04782e3a2562e22408ade74e133079fe657227d1e56b841db2f4f00e3403716'),
    (0x10779880, 0x10779b70, '73cee661ead3f7dc13187e6ed3b7a1592571e86e073e79b7d378eaee87d6f0b3'),
    (0x10779b70, 0x10779ec4, '05dd8372702a1f5eaf71461f600caaf06beaa3f1c5f8c4a8d9ab77fbbec019bf'),
    (0x10779ed0, 0x10779ff0, 'd5d8a676a3785276a70f5b4528ad1d674dbe9b73398d0ea665b0415eaf449ba7'),
    (0x10779ff0, 0x1077a000, 'a7b6b9884fc4e62cda472cea5e0e690bc527320cf6962c8486a60bbe8f490b3d'),
    (0x1077a000, 0x1077a688, 'd2efad11f7bacef895ef80e09dc1551740f6f194658cca520d40cbfbee95a1a3'),
    (0x1077a690, 0x1077b4bc, 'f2f03b913302837f0e3cdf83c7443b6b33e03cf54a117784c64c8405a07213ce'),
    (0x1077b4c0, 0x1077b518, '151c7ee9e4e169a7948965366ff9ef7d5d7f77bdd15b3d53211006f2ff1e285a'),
    (0x1077b520, 0x1077b58c, '04f3344b694f63cebd1ef4015c0f84e627edfbcc1ab30442bc9120cbc9a73895'),
    (0x1077b590, 0x1077b60c, 'b4299df76c0e97fbff2af11d9bccd17652b2668fc00659e58c5e6c085beba492'),
    (0x1077b610, 0x1077b680, '63859c1a7c3e18edff44de9076e0ad885a53a22f6126e14e906faa12cf20816d'),
    (0x1077b680, 0x1077b6f0, 'ca9ddb3c231f9890952b7781f60951318408bc84aa32fa8f5b679f682e0d7478'),
    (0x1077b6f0, 0x1077b71c, 'd09d94e55aa60fcb3079d02a4f31baf6e9a576301b62bea953f6b53ff1f9ad3b'),
    (0x1077b720, 0x1077b768, '1f9eadd348ef0ecc80ff3046885dcf156a1da6d0066242cf4729ab41c70715e0'),
)
FORMATTED_OUTPUT_ENTRIES = tuple(start for start, _, _ in CONTRACTS)
INSTRUCTION_COUNTS = (537, 149, 188, 213, 72, 4, 416, 905, 22, 27, 31, 28, 28, 11, 18)
PAGE_PINS = {
    (0x10778db0,0x10778): "d8a3a46d7faa47934015c27534559cf58e4098bc2f64c6bce827da5bfe88089c",
    (0x10778db0,0x10779): "0b75c6d901f2c3dc15707e4deb070d000d7ba959b5feafd1bf8f6a77053e8ae3",
    (0x1077a690,0x1077a): "48186562a7f910f730dfb6b644c161918554bd5cd55e58d5d7bae19af99b8ea6",
    (0x1077a690,0x1077b): "f9c4b88b7b74ddb6bb4a64638d71bdbad3b6affd69c9431395eb7469d44ad569",
}
SHARED_EDGE = (0x10779ff4,0x10779ed8)


def _require(records, expected, start):
    body = [{key: item.get(key) for key in ('address', 'assembly', 'ops', 'flows', 'fallthrough')}
            for item in records]
    actual = hashlib.sha256(json.dumps(body, sort_keys=True, separators=(',', ':')).encode()).hexdigest()
    if actual != expected:
        raise ValueError(f'Mac formatted output contract changed at {start:x}')


def require_formatted_output_contract(page, instructions):
    """Validate complete page fragments, allowing a legitimately split body."""
    for start, end, expected in CONTRACTS:
        if not start >> 12 <= page <= (end - 1) >> 12:
            continue
        records = [r for r in instructions if start <= r['address'] < end]
        if records:
            _require(records, PAGE_PINS.get((start,page),expected), start)


def require_formatted_output_bodies(instructions):
    """Validate the full bodies across pages before a completion manifest."""
    for start, end, expected in CONTRACTS:
        records = [r for r in instructions if start <= r['address'] < end]
        if records:
            _require(records, expected, start)


def require_formatted_output_edges(edges, require_shared=False):
    observed = set()
    for source, target in edges:
        for start, end, _ in CONTRACTS:
            if start <= target < end and not start <= source < end and target != start:
                if (source, target) == SHARED_EDGE:
                    observed.add(SHARED_EDGE)
                    continue
                raise ValueError(f'External formatted output interior edge from {source:x} to {target:x}')
    if require_shared and observed != {SHARED_EDGE}:
        raise ValueError('Missing shared formatted output reset edge')


def require_formatted_output_index(db, selected=None):
    if selected is None:
        instructions = [json.loads(row[0]) for row in db.execute(
            'SELECT body FROM instructions WHERE address>=? AND address<? ORDER BY address',
            (CONTRACTS[0][0], max(end for _, end, _ in CONTRACTS)))]
        addresses = {item['address'] for item in instructions}
        for entry in FORMATTED_OUTPUT_ENTRIES:
            if entry not in addresses:
                raise ValueError(f'Missing complete Mac formatted output body at {entry:x}')
    else:
        instructions = [item for _, item in sorted(selected.items())]
    require_formatted_output_bodies(instructions)
    require_formatted_output_edges(db.execute("""SELECT i.address, edge.value
        FROM instructions AS i,
          json_each(json_insert(json_extract(i.body, '$.flows'),
                                '$[#]', json_extract(i.body, '$.fallthrough'))) AS edge
        WHERE edge.value>=? AND edge.value<?""", (CONTRACTS[0][0], max(end for _, end, _ in CONTRACTS))), require_shared=True)


def require_formatted_output_hooks(page, instructions, code):
    addresses = {r['address'] for r in instructions}
    for entry in (*FORMATTED_OUTPUT_ENTRIES, SHARED_EDGE[1]):
        if entry >> 12 != page or entry not in addresses:
            continue
        label = f'i_{entry:x}:'
        hook = ('if (try_modern_formatted_output_shared_tail(s)) return true;' if entry == SHARED_EDGE[1]
                else f'if (try_modern_formatted_output(s, 0x{entry:x})) return true;')
        step = f's.step(0x{entry:x});'
        if code.count(hook) != 1 or label not in code:
            raise ValueError(f'Missing or duplicate formatted output hook at {entry:x}')
        fragment = code.split(label, 1)[1].split(f'i_{entry + 4:x}:', 1)[0]
        if hook not in fragment or step not in fragment or fragment.index(hook) > fragment.index(step):
            raise ValueError(f'Misplaced formatted output hook at {entry:x}')
