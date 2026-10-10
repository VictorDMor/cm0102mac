"""Pinned Mac buffered byte and item I/O contracts."""
import hashlib
import json

CONTRACTS = (
    (0x10775ad0, 0x10775c04, '162461279eab557ab0115dda7f04984b4f74750f453802c206ae0625e5a51dfe'),
    (0x10775c10, 0x10775c80, '9049d04e891eba4d580228fbf49a3af4a41196b0b3836fef5654f542709a69f1'),
    (0x10775c80, 0x10775d78, 'bd3a3acdf94f63c8e0364cdba0e9b9c701d636fdecb96d960ecfdcf57cd0def8'),
    (0x10775d80, 0x10775e5c, 'f3e8e6966664893074a6ce3ec965c7954b2f3a4ff3979b8681f7457da87ad2f3'),
    (0x10775fe0, 0x10776000, '818a9159b17268c51c844beb14952940b7ddb4fe98b6462764c49f48e192a110'),
    (0x10776000, 0x10776360, '21ff0ebab7e18057c44d46a22d70fa4adbc25dbf30677b9714c2ed03f7849b37'),
    (0x10776360, 0x10776380, '307003035ca3f591b1a9ab184fc5807857a906f1c325c8b5ed6a69e4cc4ce9b8'),
    (0x10776380, 0x1077668c, '567e3390bc0953d9f2116b92df6d2e1cee878985129a4c1d843005587ade6845'),
    (0x1077fa50, 0x1077fad8, '825de1cd6849578fc2abe2ade4032cb1dc57aef04e5433922d8935aba200e507'),
)
STREAM_IO_ENTRIES = tuple(start for start, _, _ in CONTRACTS)
INSTRUCTION_COUNTS = (77, 28, 61, 55, 8, 216, 8, 195, 34)


def _require(records, expected, start):
    body = [{key: item.get(key) for key in ('address', 'assembly', 'ops', 'flows', 'fallthrough')}
            for item in records]
    actual = hashlib.sha256(json.dumps(body, sort_keys=True, separators=(',', ':')).encode()).hexdigest()
    if actual != expected:
        raise ValueError(f'Mac stream_io contract changed at {start:x}')


def require_stream_io_contract(page, instructions):
    for start, end, expected in CONTRACTS:
        if not start >> 12 <= page <= (end - 1) >> 12:
            continue
        records = [r for r in instructions if start <= r['address'] < end]
        if records:
            _require(records, expected, start)


def require_stream_io_bodies(instructions, complete=False):
    for start, end, expected in CONTRACTS:
        records = [r for r in instructions if start <= r['address'] < end]
        if complete and not records:
            raise ValueError(f'Missing complete Mac stream_io body at {start:x}')
        if records:
            _require(records, expected, start)


def require_stream_io_edges(edges):
    for source, target in edges:
        for start, end, _ in CONTRACTS:
            if start <= target < end and not start <= source < end and target != start:
                raise ValueError(f'External stream_io interior edge from {source:x} to {target:x}')


def require_stream_io_index(db, selected=None):
    instructions = ([json.loads(row[0]) for row in db.execute(
        'SELECT body FROM instructions WHERE address>=? AND address<? ORDER BY address',
        (CONTRACTS[0][0], CONTRACTS[-1][1]))] if selected is None
        else [item for _, item in sorted(selected.items())])
    require_stream_io_bodies(instructions, complete=selected is None)
    require_stream_io_edges(db.execute("""SELECT i.address, edge.value
        FROM instructions AS i,
          json_each(json_insert(json_extract(i.body, '$.flows'),
                                '$[#]', json_extract(i.body, '$.fallthrough'))) AS edge
        WHERE edge.value>=? AND edge.value<?""", (CONTRACTS[0][0], CONTRACTS[-1][1])))


def require_stream_io_hooks(page, instructions, code):
    addresses = {r['address'] for r in instructions}
    for entry in STREAM_IO_ENTRIES:
        if entry >> 12 != page or entry not in addresses:
            continue
        label = f'i_{entry:x}:'
        hook = f'if (try_modern_stream_io(s, 0x{entry:x})) return true;'
        step = f's.step(0x{entry:x});'
        if code.count(hook) != 1 or label not in code:
            raise ValueError(f'Missing or duplicate stream_io hook at {entry:x}')
        fragment = code.split(label, 1)[1].split(f'i_{entry + 4:x}:', 1)[0]
        if hook not in fragment or step not in fragment or fragment.index(hook) > fragment.index(step):
            raise ValueError(f'Misplaced stream_io hook at {entry:x}')
