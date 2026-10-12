"""Pinned complete match actor construction boundaries."""
import hashlib
import json

CONTRACTS = (
    (0x1020e8f0, 0x1020e9f8, 66, '9cabd8c6fafdaef743ef162bb4171a8ab66ca550dd4b2be8d792a149e5420544'),
    (0x10223ac0, 0x10226520, 2712, '263340be710966e6996d21df30a30a76d07c3b68259c6ba6b5c0298974adfc80'),
)
FRAGMENTS = (
    (0x1020e8f0, 0x1020e, 66, '9cabd8c6fafdaef743ef162bb4171a8ab66ca550dd4b2be8d792a149e5420544'),
    (0x10223ac0, 0x10223, 336, '0b5484c8e0230315ea0bc99c9b2c5fd1fd8321c84b42579379c4a80d8db4525f'),
    (0x10223ac0, 0x10224, 1024, '94afafd87b428874c3bcd65f8d1388ecaf4d133a631c4a41dbd51d536fba67f5'),
    (0x10223ac0, 0x10225, 1024, 'adad478b924681ffd80f6abea028c3a1bc09253ef0a9acf1e289bbf0dbcda359'),
    (0x10223ac0, 0x10226, 328, '04a2f86e3b2a7f7969d5c88eaa06c86f8fa483b569670a7bfe0279e6f942e479'),
)
ACTOR_CONSTRUCTION_ENTRIES = tuple(start for start, _, _, _ in CONTRACTS)


def fingerprint(body):
    canonical = [{key: r.get(key) for key in ('address', 'assembly', 'ops', 'flows', 'fallthrough')} for r in body]
    return hashlib.sha256(json.dumps(canonical, sort_keys=True, separators=(',', ':')).encode()).hexdigest()


def require_actor_construction_bodies(instructions, complete=False):
    for start, end, count, expected in CONTRACTS:
        body = [r for r in instructions if start <= r['address'] < end]
        if not body and not complete: continue
        if len(body) != count or fingerprint(body) != expected:
            raise ValueError(f'Mac actor-construction contract changed or missing at {start:x}')


def require_actor_construction_edges(edges):
    for source, target in edges:
        for start, end, _, _ in CONTRACTS:
            if start < target < end and not start <= source < end:
                raise ValueError(f'External actor-construction interior edge from {source:x} to {target:x}')


def require_actor_construction_contract(page, instructions, selected=None):
    addresses = {r['address'] for r in instructions} | set(selected or ())
    for start, end, _, _ in CONTRACTS:
        if not start >> 12 <= page <= (end - 1) >> 12: continue
        low, high = max(start, page << 12), min(end, (page + 1) << 12)
        if not any(low <= a < high for a in addresses): continue
        body = [r for r in instructions if low <= r['address'] < high]
        _, _, count, expected = next(f for f in FRAGMENTS if f[0] == start and f[1] == page)
        if len(body) != count or fingerprint(body) != expected:
            raise ValueError(f'Mac actor-construction contract changed or missing at {start:x} page {page:x}')


def require_actor_construction_index(db, selected=None):
    records = [json.loads(row[0]) for start, end, _, _ in CONTRACTS for row in db.execute(
        'SELECT body FROM instructions WHERE address>=? AND address<? ORDER BY address', (start, end))]
    if selected is None: require_actor_construction_bodies(records, complete=True)
    else:
        for page in sorted({f[1] for f in FRAGMENTS}):
            require_actor_construction_contract(page, [r for _, r in sorted(selected.items())], selected)
    require_actor_construction_edges(db.execute("""SELECT i.address,e.value
        FROM instructions AS i,
          json_each(json_insert(json_extract(i.body,'$.flows'),'$[#]',json_extract(i.body,'$.fallthrough'))) AS e
        WHERE e.value>=? AND e.value<?""", (CONTRACTS[0][0], CONTRACTS[-1][1])))


def actor_construction_hook(entry):
    return f'if (try_modern_actor_construction(s, 0x{entry:x})) return true;'


def require_actor_construction_hooks(page, instructions, code):
    addresses = {r['address'] for r in instructions}
    for entry in ACTOR_CONSTRUCTION_ENTRIES:
        if entry >> 12 != page or entry not in addresses: continue
        label, hook = f'i_{entry:x}:', actor_construction_hook(entry)
        fragment = code.split(label, 1)[-1].split(f'i_{entry + 4:x}:', 1)[0]
        steps = [token for token in (f's.step(0x{entry:x});', f'counter.step(0x{entry:x});') if token in fragment]
        if code.count(hook) != 1 or hook not in fragment or len(steps) != 1 or fragment.index(hook) > fragment.index(steps[0]):
            raise ValueError(f'Missing or misplaced actor-construction hook at {entry:x}')
