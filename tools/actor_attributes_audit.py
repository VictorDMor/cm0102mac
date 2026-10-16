"""Pinned complete original actor action-attribute refresh and maximum boundaries."""
import hashlib
import json

CONTRACTS = (
    (0x10235ea0, 0x102373a0, 1344, 'a65bfe1809ad4e6fb714b913d35b9c7093adf4fe78f9c9595588a77c2c2d4e60'),
    (0x102373a0, 0x102373b4, 5, '7b3379b11aede3d3f834c85c1e0c343e03f480a62ba9034ca67ce459d749ebad'),
)
FRAGMENTS = (
    (0x10235ea0, 0x10235, 88, '45665d75865ef78ca968b68d3ce392dac9ebad88484f795c921040bd8088e8ff'),
    (0x10235ea0, 0x10236, 1024, '30dca2158a57debcd27adbf414eeaaac96f2184619ed9de3948311403fbb1a0b'),
    (0x10235ea0, 0x10237, 232, '7f6438ff1b3e15b39c948e96385f18ef9ef28b0edb7d321071bcebc7f47df7e0'),
    (0x102373a0, 0x10237, 5, '7b3379b11aede3d3f834c85c1e0c343e03f480a62ba9034ca67ce459d749ebad'),
)
ACTOR_ATTRIBUTES_ENTRIES = tuple(start for start, _, _, _ in CONTRACTS)


def fingerprint(body):
    canonical = [{key: r.get(key) for key in ('address', 'assembly', 'ops', 'flows', 'fallthrough')} for r in body]
    return hashlib.sha256(json.dumps(canonical, sort_keys=True, separators=(',', ':')).encode()).hexdigest()


def require_actor_attributes_bodies(instructions, complete=False):
    for start, end, count, expected in CONTRACTS:
        body = [r for r in instructions if start <= r['address'] < end]
        if not body and not complete: continue
        if len(body) != count or fingerprint(body) != expected:
            raise ValueError(f'Mac actor-attributes contract changed or missing at {start:x}')


def require_actor_attributes_edges(edges):
    for source, target in edges:
        for start, end, _, _ in CONTRACTS:
            if start < target < end and not start <= source < end:
                raise ValueError(f'External actor-attributes interior edge from {source:x} to {target:x}')


def require_actor_attributes_contract(page, instructions, selected=None):
    addresses = {r['address'] for r in instructions} | set(selected or ())
    for start, end, _, _ in CONTRACTS:
        if not start >> 12 <= page <= (end - 1) >> 12: continue
        low, high = max(start, page << 12), min(end, (page + 1) << 12)
        if start not in addresses and not any(low <= a < high for a in addresses): continue
        body = [r for r in instructions if low <= r['address'] < high]
        _, _, count, expected = next(f for f in FRAGMENTS if f[0] == start and f[1] == page)
        if len(body) != count or fingerprint(body) != expected:
            raise ValueError(f'Mac actor-attributes contract changed or missing at {start:x} page {page:x}')


def require_actor_attributes_index(db, selected=None):
    records = [json.loads(row[0]) for start, end, _, _ in CONTRACTS for row in db.execute(
        'SELECT body FROM instructions WHERE address>=? AND address<? ORDER BY address', (start, end))]
    if selected is None: require_actor_attributes_bodies(records, complete=True)
    else:
        for page in sorted({f[1] for f in FRAGMENTS}):
            require_actor_attributes_contract(page, [r for _, r in sorted(selected.items())], selected)
    require_actor_attributes_edges(db.execute("""SELECT i.address,e.value
        FROM instructions AS i,
          json_each(json_insert(json_extract(i.body,'$.flows'),'$[#]',json_extract(i.body,'$.fallthrough'))) AS e
        WHERE e.value>=? AND e.value<?""", (CONTRACTS[0][0], CONTRACTS[-1][1])))
