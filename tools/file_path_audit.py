"""Pinned Mac file-path contracts; production integration is separately verified."""
import hashlib
import json

CONTRACTS = (
    (0x107784c0, 0x1077851c, '30af00422f19a7ad460e6d93297f0721daa5bbbbd5890428aa401e66315ff7b6'),
    (0x10778520, 0x107786f0, '16f7752ef5e153ed03f05d474565307470b479b8df43de3bd449368f02ea4bd5'),
    (0x107786f0, 0x10778814, '737d3ee17981dfc01236bbcb20c60ff293f7d97d8140aab7303a0b1f52ba9beb'),
    (0x10778820, 0x1077886c, '7ddafbe933efaf3e103605c4e4cb008491a93930b03265f4673c119eeeae9895'),
    (0x10778870, 0x107788cc, '26f635b0d672d7190818d92c938bea0f720d8337e91eefc2a90a7f9a6f9c195a'),
    (0x107788d0, 0x10778a78, '627e4d76955cd109acc9670907da6643c651f603df838c5b3b40372959faa4d3'),
    (0x10778a80, 0x10778d48, '7eaea9c34b6c770224d118c6d599c915d8de6692b303bdaf9344deb89f59d57d'),
)
FILE_PATH_ENTRIES = tuple(start for start, _, _ in CONTRACTS)
INSTRUCTION_COUNTS = (23, 116, 73, 19, 23, 106, 177)
PAGE_FRAGMENTS = {}


def _require(records, expected, start):
    body = [{key: item.get(key) for key in ('address', 'assembly', 'ops', 'flows', 'fallthrough')}
            for item in records]
    actual = hashlib.sha256(json.dumps(body, sort_keys=True, separators=(',', ':')).encode()).hexdigest()
    if actual != expected:
        raise ValueError(f'Mac file_path contract changed at {start:x}')


def require_file_path_contract(page, instructions):
    for start, end, expected in CONTRACTS:
        if not start >> 12 <= page <= (end - 1) >> 12:
            continue
        records = [r for r in instructions if start <= r['address'] < end]
        if records:
            fragment = PAGE_FRAGMENTS.get((start, page))
            if fragment:
                count, digest = fragment
                if len(records) != count:
                    raise ValueError(f'Mac file_path fragment size changed at {start:x}/{page:x}')
                _require(records, digest, start)
            else:
                _require(records, expected, start)


def require_file_path_bodies(instructions, complete=False):
    for start, end, expected in CONTRACTS:
        records = [r for r in instructions if start <= r['address'] < end]
        if complete and not records:
            raise ValueError(f'Missing complete Mac file_path body at {start:x}')
        if records:
            _require(records, expected, start)


def require_file_path_edges(edges):
    for source, target in edges:
        for start, end, _ in CONTRACTS:
            if start <= target < end and not start <= source < end and target != start:
                raise ValueError(f'External file_path interior edge from {source:x} to {target:x}')


def require_file_path_index(db, selected=None):
    instructions = ([json.loads(row[0]) for row in db.execute(
        'SELECT body FROM instructions WHERE address>=? AND address<? ORDER BY address',
        (CONTRACTS[0][0], CONTRACTS[-1][1]))] if selected is None
        else [item for _, item in sorted(selected.items())])
    require_file_path_bodies(instructions, complete=selected is None)
    require_file_path_edges(db.execute("""SELECT i.address, edge.value
        FROM instructions AS i,
          json_each(json_insert(json_extract(i.body, '$.flows'),
                                '$[#]', json_extract(i.body, '$.fallthrough'))) AS edge
        WHERE edge.value>=? AND edge.value<?""", (CONTRACTS[0][0], CONTRACTS[-1][1])))


def require_file_path_hooks(page, instructions, code):
    addresses = {r['address'] for r in instructions}
    for entry in FILE_PATH_ENTRIES:
        if entry >> 12 != page or entry not in addresses:
            continue
        label = f'i_{entry:x}:'
        hook = f'if (try_modern_file_path(s, 0x{entry:x})) return true;'
        step = f's.step(0x{entry:x});'
        if code.count(hook) != 1 or label not in code:
            raise ValueError(f'Missing or duplicate file_path hook at {entry:x}')
        fragment = code.split(label, 1)[1].split(f'i_{entry + 4:x}:', 1)[0]
        if hook not in fragment or step not in fragment or fragment.index(hook) > fragment.index(step):
            raise ValueError(f'Misplaced file_path hook at {entry:x}')
