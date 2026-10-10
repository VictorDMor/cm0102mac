"""Pinned Mac stream storage, callbacks and lifecycle contracts."""
import hashlib
import json

CONTRACTS = (
    (0x10775240, 0x107752cc, '727ebbeecba70fa1b66085ac8aebb7ce7b7af631e03733834d6d4b359bc00f53'),
    (0x107752d0, 0x107753a8, '5ffe78935c6e6804c121f787bdebfeebd1662a8c9fa724772505015b96f344f3'),
    (0x107753b0, 0x10775440, '0d7459eda69f6e5e9a518e6e1bed8df88074024ec8f699ab8baf4301113b4b6f'),
    (0x10775440, 0x107754a8, 'f3b43472f6083e1ee7598fe29a0d444df8b0552dc4adf39cf489874df8e8c910'),
    (0x107754b0, 0x10775534, 'ab620579770f90bb04c88e5817d423266999458c37041763d2c00b765dda5a78'),
    (0x107756f0, 0x10775734, '31f69e361d3765f759b2f0797bbd4d31b9a002ff48eb02e932f24332bfd9a971'),
    (0x10775740, 0x10775784, '0a7302713b0dc2dd3bf4930504813bf463671aa12d0bdc9c3eddd4baab437208'),
    (0x10775790, 0x107757c4, 'a9920b6c21e2b26b0f2323439ea3ff5d22ce4504572cd284b470e710b0746f25'),
    (0x107757d0, 0x10775898, 'd6c366198ea2ac1d35fcf38b9693d36dcb3ac6f794d721a2957f3a1ab07eb218'),
    (0x107758a0, 0x10775954, '3d109d0a7092f287dd63799d58f58d7c8f0f0323409bcba9922fa3ac554f4022'),
    (0x10775960, 0x10775ac4, '28c1c19daa15dccf33b92ca35486a9e39d6aa431b1d0550cf1b7abe0846353a8'),
    (0x107766f0, 0x107767b0, '665d5317a3ff438254c1c8a3648492379ac07201bc2bef3a3135ceb8aeda026d'),
    (0x107767b0, 0x107768e8, '57b8f86a7a17877a7b5a7c166da8190517cf20a8898277c21643930366541648'),
)
STREAM_BUFFER_ENTRIES = tuple(start for start, _, _ in CONTRACTS)
INSTRUCTION_COUNTS = (35, 54, 36, 26, 33, 17, 17, 13, 50, 45, 89, 48, 78)


def _require(records, expected, start):
    body = [{key: item.get(key) for key in ('address', 'assembly', 'ops', 'flows', 'fallthrough')}
            for item in records]
    actual = hashlib.sha256(json.dumps(body, sort_keys=True, separators=(',', ':')).encode()).hexdigest()
    if actual != expected:
        raise ValueError(f'Mac stream_buffer contract changed at {start:x}')


def require_stream_buffer_contract(page, instructions):
    for start, end, expected in CONTRACTS:
        if not start >> 12 <= page <= (end - 1) >> 12:
            continue
        records = [r for r in instructions if start <= r['address'] < end]
        if records:
            _require(records, expected, start)


def require_stream_buffer_bodies(instructions, complete=False):
    for start, end, expected in CONTRACTS:
        records = [r for r in instructions if start <= r['address'] < end]
        if complete and not records:
            raise ValueError(f'Missing complete Mac stream_buffer body at {start:x}')
        if records:
            _require(records, expected, start)


def require_stream_buffer_edges(edges):
    for source, target in edges:
        for start, end, _ in CONTRACTS:
            if start <= target < end and not start <= source < end and target != start:
                raise ValueError(f'External stream_buffer interior edge from {source:x} to {target:x}')


def require_stream_buffer_index(db, selected=None):
    instructions = ([json.loads(row[0]) for row in db.execute(
        'SELECT body FROM instructions WHERE address>=? AND address<? ORDER BY address',
        (CONTRACTS[0][0], CONTRACTS[-1][1]))] if selected is None
        else [item for _, item in sorted(selected.items())])
    require_stream_buffer_bodies(instructions, complete=selected is None)
    require_stream_buffer_edges(db.execute("""SELECT i.address, edge.value
        FROM instructions AS i,
          json_each(json_insert(json_extract(i.body, '$.flows'),
                                '$[#]', json_extract(i.body, '$.fallthrough'))) AS edge
        WHERE edge.value>=? AND edge.value<?""", (CONTRACTS[0][0], CONTRACTS[-1][1])))


def require_stream_buffer_hooks(page, instructions, code):
    addresses = {r['address'] for r in instructions}
    for entry in STREAM_BUFFER_ENTRIES:
        if entry >> 12 != page or entry not in addresses:
            continue
        label = f'i_{entry:x}:'
        hook = f'if (try_modern_stream_buffer(s, 0x{entry:x})) return true;'
        step = f's.step(0x{entry:x});'
        if code.count(hook) != 1 or label not in code:
            raise ValueError(f'Missing or duplicate stream_buffer hook at {entry:x}')
        fragment = code.split(label, 1)[1].split(f'i_{entry + 4:x}:', 1)[0]
        if hook not in fragment or step not in fragment or fragment.index(hook) > fragment.index(step):
            raise ValueError(f'Misplaced stream_buffer hook at {entry:x}')
