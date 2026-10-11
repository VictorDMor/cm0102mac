"""Aggregate and embedded-table lifetime boundaries."""
import hashlib
import json

CONTRACTS = (
    (0x10718b40, 0x10718ba0, 24, 'e7913fb414349c7180f2498e347e3e3cf15d729a8b69f0ff2eb00c6ac42f9aaf'),
    (0x10718ba0, 0x10718d74, 117, 'ef07c7b8530fb902c58705e8aec7bbbfefd477011b07288f849a6ee1f4396245'),
    (0x1073c1b0, 0x1073c45c, 171, '5f2ea432e446a5c155f907a1a8aa6eb9dfcdba8dcead08c790d26bca3582d62f'),
    (0x1073c460, 0x1073c4e0, 32, '4a414b8713224b377c066651f912a8f1c72db94ea9ca1f22c2ce066507d24633'),
    (0x1073c4e0, 0x1073c5a0, 48, '669a4d8cd26cebe07f657fff6e81a1e71440278abe676b3265309013193b4c09'),
    (0x1073f4e0, 0x1073f52c, 19, '44b50bd8c93cf53e016f1a39df6bde3df6834654981cfef699b3ac5771b43f1f'),
)
AGGREGATE_OBJECT_ENTRIES = tuple(start for start, _, _, _ in CONTRACTS)


def fingerprint(body):
    canonical = [{key: r.get(key) for key in ('address', 'assembly', 'ops', 'flows', 'fallthrough')} for r in body]
    return hashlib.sha256(json.dumps(canonical, sort_keys=True, separators=(',', ':')).encode()).hexdigest()


def require_aggregate_object_bodies(instructions, complete=False):
    for start, end, count, expected in CONTRACTS:
        body = [r for r in instructions if start <= r['address'] < end]
        if not body and not complete: continue
        if len(body) != count or fingerprint(body) != expected:
            raise ValueError(f'Mac aggregate-object contract changed or missing at {start:x}')


def require_aggregate_object_edges(edges):
    for source, target in edges:
        for start, end, _, _ in CONTRACTS:
            if start < target < end and not start <= source < end:
                raise ValueError(f'External aggregate-object interior edge from {source:x} to {target:x}')


def require_aggregate_object_contract(page, instructions, selected=None):
    addresses = {r['address'] for r in instructions} | set(selected or ())
    for start, end, count, expected in CONTRACTS:
        if start >> 12 != page or not any(start <= a < end for a in addresses): continue
        body = [r for r in instructions if start <= r['address'] < end]
        if len(body) != count or fingerprint(body) != expected:
            raise ValueError(f'Mac aggregate-object contract changed or missing at {start:x}')


def require_aggregate_object_index(db, selected=None):
    records = [json.loads(row[0]) for start, end, _, _ in CONTRACTS for row in db.execute(
        'SELECT body FROM instructions WHERE address>=? AND address<? ORDER BY address', (start, end))]
    if selected is None: require_aggregate_object_bodies(records, complete=True)
    else:
        for page in tuple(sorted({start >> 12 for start, _, _, _ in CONTRACTS})):
            require_aggregate_object_contract(page, [r for _, r in sorted(selected.items())], selected)
    require_aggregate_object_edges(db.execute("""SELECT i.address,e.value
        FROM instructions AS i,
          json_each(json_insert(json_extract(i.body,'$.flows'),'$[#]',json_extract(i.body,'$.fallthrough'))) AS e
        WHERE e.value>=? AND e.value<?""", (CONTRACTS[0][0], CONTRACTS[-1][1])))


def aggregate_object_hook(entry):
    return f'if (try_modern_aggregate_object(s, 0x{entry:x})) return true;'


def require_aggregate_object_hooks(page, instructions, code):
    addresses = {r['address'] for r in instructions}
    for entry in AGGREGATE_OBJECT_ENTRIES:
        if entry >> 12 != page or entry not in addresses: continue
        label, hook = f'i_{entry:x}:', aggregate_object_hook(entry)
        fragment = code.split(label, 1)[-1].split(f'i_{entry + 4:x}:', 1)[0]
        steps = [token for token in (f's.step(0x{entry:x});', f'counter.step(0x{entry:x});') if token in fragment]
        if code.count(hook) != 1 or hook not in fragment or len(steps) != 1 or fragment.index(hook) > fragment.index(steps[0]):
            raise ValueError(f'Missing or misplaced aggregate-object hook at {entry:x}')
