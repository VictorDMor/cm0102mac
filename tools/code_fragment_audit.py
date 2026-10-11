"""Pinned Mac code-fragment contracts; production integration is separately verified."""
import hashlib
import json

CONTRACTS = (
    (0x10771150, 0x107712a4, 'b7a3d90f8ddc93317de754bc24be92b2103149d5e69d8bc469ee5a8e93e4750e'),
    (0x107712b0, 0x107713d8, '0ef9fa3af79ddaf0ae84142bd2ba4ea7ceea9a3ba1eb0718a3ab77f267be1d9d'),
    (0x107713e0, 0x107715f0, '1dcacf0f3737338010b1505b729eec9c192c086a3f8be73514a432403e6b0e01'),
    (0x107715f0, 0x10771614, 'dad91c6b4a36c5c12c01480e0a6c172037e6e74446573d207e23d037b56e4666'),
    (0x10771620, 0x10771640, '1784828c0cb21e31f11f266d06117a71c891dfcfb7d11f23de0456e865131782'),
)
CODE_FRAGMENT_ENTRIES = tuple(start for start, _, _ in CONTRACTS)
INSTRUCTION_COUNTS = (85, 73, 126, 9, 8)
PAGE_FRAGMENTS = {}


def _require(records, expected, start):
    body = [{key: item.get(key) for key in ('address', 'assembly', 'ops', 'flows', 'fallthrough')}
            for item in records]
    actual = hashlib.sha256(json.dumps(body, sort_keys=True, separators=(',', ':')).encode()).hexdigest()
    if actual != expected:
        raise ValueError(f'Mac code_fragment contract changed at {start:x}')


def require_code_fragment_contract(page, instructions):
    for start, end, expected in CONTRACTS:
        if not start >> 12 <= page <= (end - 1) >> 12:
            continue
        records = [r for r in instructions if start <= r['address'] < end]
        if records:
            fragment = PAGE_FRAGMENTS.get((start, page))
            if fragment:
                count, digest = fragment
                if len(records) != count:
                    raise ValueError(f'Mac code_fragment fragment size changed at {start:x}/{page:x}')
                _require(records, digest, start)
            else:
                _require(records, expected, start)


def require_code_fragment_bodies(instructions, complete=False):
    for start, end, expected in CONTRACTS:
        records = [r for r in instructions if start <= r['address'] < end]
        if complete and not records:
            raise ValueError(f'Missing complete Mac code_fragment body at {start:x}')
        if records:
            _require(records, expected, start)


def require_code_fragment_edges(edges):
    for source, target in edges:
        for start, end, _ in CONTRACTS:
            if start <= target < end and not start <= source < end and target != start:
                raise ValueError(f'External code_fragment interior edge from {source:x} to {target:x}')


def require_code_fragment_index(db, selected=None):
    instructions = ([json.loads(row[0]) for row in db.execute(
        'SELECT body FROM instructions WHERE address>=? AND address<? ORDER BY address',
        (CONTRACTS[0][0], CONTRACTS[-1][1]))] if selected is None
        else [item for _, item in sorted(selected.items())])
    require_code_fragment_bodies(instructions, complete=selected is None)
    require_code_fragment_edges(db.execute("""SELECT i.address, edge.value
        FROM instructions AS i,
          json_each(json_insert(json_extract(i.body, '$.flows'),
                                '$[#]', json_extract(i.body, '$.fallthrough'))) AS edge
        WHERE edge.value>=? AND edge.value<?""", (CONTRACTS[0][0], CONTRACTS[-1][1])))


def require_code_fragment_hooks(page, instructions, code):
    addresses = {r['address'] for r in instructions}
    for entry in CODE_FRAGMENT_ENTRIES:
        if entry >> 12 != page or entry not in addresses:
            continue
        label = f'i_{entry:x}:'
        hook = f'if (try_modern_code_fragment(s, 0x{entry:x})) return true;'
        if code.count(hook) != 1 or label not in code:
            raise ValueError(f'Missing or duplicate code_fragment hook at {entry:x}')
        fragment = code.split(label, 1)[1].split(f'i_{entry + 4:x}:', 1)[0]
        step = f'counter.step(0x{entry:x});' if f'counter.step(0x{entry:x});' in fragment else f's.step(0x{entry:x});'
        if hook not in fragment or step not in fragment or fragment.index(hook) > fragment.index(step):
            raise ValueError(f'Misplaced code_fragment hook at {entry:x}')
