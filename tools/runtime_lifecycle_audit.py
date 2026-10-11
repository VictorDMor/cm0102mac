"""Pinned Mac runtime-lifecycle contracts; production integration is separately verified."""
import hashlib
import json

CONTRACTS = (
    (0x10772df0, 0x10772e0c, '50559af8d1871af006b2f1e420ba496a86227d93bdd330e483b80666b9782640'),
    (0x10772e10, 0x10772e64, '08660daaa3fd1e587b56f63106c1ddc669e6c93b2198ab7a22ae8fffd4d43e5f'),
    (0x10773fb0, 0x10773fe8, '5deebb97b7f7025b52b860a02a56f2e85d408af0b833d0892527f9be8e316407'),
    (0x10773ff0, 0x1077405c, '14653993f6f58af5c8ae50a89465fb70279041c7294830b418386339d2f6e601'),
    (0x10774060, 0x107740e4, 'b60742fc3b5da3458566cf3a895753e70d09fd6bc39167a7db72f7b9409aa094'),
    (0x1077db20, 0x1077dbcc, '8112cd00fdc13e0f85574d819149801b91b7a1ac245826a1f51480adae9595c5'),
)
RUNTIME_LIFECYCLE_ENTRIES = tuple(start for start, _, _ in CONTRACTS)
INSTRUCTION_COUNTS = (7, 21, 14, 27, 33, 43)
PAGE_FRAGMENTS = {
    (0x10773ff0, 0x10773): (4, 'cbe209679ded7ec28c17d1e6f839caf286862a361ce091a87e1dd59ea180c5b0'),
    (0x10773ff0, 0x10774): (23, 'cf52bab1845520d6d3657ba67ac6aeb50b6f36f67da8c5eb9e5f2f4220613cca'),
}


def _require(records, expected, start):
    body = [{key: item.get(key) for key in ('address', 'assembly', 'ops', 'flows', 'fallthrough')}
            for item in records]
    actual = hashlib.sha256(json.dumps(body, sort_keys=True, separators=(',', ':')).encode()).hexdigest()
    if actual != expected:
        raise ValueError(f'Mac runtime_lifecycle contract changed at {start:x}')


def require_runtime_lifecycle_contract(page, instructions):
    for start, end, expected in CONTRACTS:
        if not start >> 12 <= page <= (end - 1) >> 12:
            continue
        records = [r for r in instructions if start <= r['address'] < end]
        if records:
            fragment = PAGE_FRAGMENTS.get((start, page))
            if fragment:
                count, digest = fragment
                if len(records) != count:
                    raise ValueError(f'Mac runtime_lifecycle fragment size changed at {start:x}/{page:x}')
                _require(records, digest, start)
            else:
                _require(records, expected, start)


def require_runtime_lifecycle_bodies(instructions, complete=False):
    for start, end, expected in CONTRACTS:
        records = [r for r in instructions if start <= r['address'] < end]
        if complete and not records:
            raise ValueError(f'Missing complete Mac runtime_lifecycle body at {start:x}')
        if records:
            _require(records, expected, start)


def require_runtime_lifecycle_edges(edges):
    for source, target in edges:
        for start, end, _ in CONTRACTS:
            if start <= target < end and not start <= source < end and target != start:
                raise ValueError(f'External runtime_lifecycle interior edge from {source:x} to {target:x}')


def require_runtime_lifecycle_index(db, selected=None):
    instructions = ([json.loads(row[0]) for row in db.execute(
        'SELECT body FROM instructions WHERE address>=? AND address<? ORDER BY address',
        (CONTRACTS[0][0], CONTRACTS[-1][1]))] if selected is None
        else [item for _, item in sorted(selected.items())])
    require_runtime_lifecycle_bodies(instructions, complete=selected is None)
    require_runtime_lifecycle_edges(db.execute("""SELECT i.address, edge.value
        FROM instructions AS i,
          json_each(json_insert(json_extract(i.body, '$.flows'),
                                '$[#]', json_extract(i.body, '$.fallthrough'))) AS edge
        WHERE edge.value>=? AND edge.value<?""", (CONTRACTS[0][0], CONTRACTS[-1][1])))


def require_runtime_lifecycle_hooks(page, instructions, code):
    addresses = {r['address'] for r in instructions}
    for entry in RUNTIME_LIFECYCLE_ENTRIES:
        if entry >> 12 != page or entry not in addresses:
            continue
        label = f'i_{entry:x}:'
        hook = f'if (try_modern_runtime_lifecycle(s, 0x{entry:x})) return true;'
        if code.count(hook) != 1 or label not in code:
            raise ValueError(f'Missing or duplicate runtime_lifecycle hook at {entry:x}')
        fragment = code.split(label, 1)[1].split(f'i_{entry + 4:x}:', 1)[0]
        step = f'counter.step(0x{entry:x});' if f'counter.step(0x{entry:x});' in fragment else f's.step(0x{entry:x});'
        if hook not in fragment or step not in fragment or fragment.index(hook) > fragment.index(step):
            raise ValueError(f'Misplaced runtime_lifecycle hook at {entry:x}')
