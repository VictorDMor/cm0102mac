"""Pinned complete actor processing-order boundaries."""
import hashlib
import json

CONTRACTS = (
    (0x101d5670, 0x101d5678, 2, '4e9fac9d50f21ea23085b9cbfa4c4501151ea88c9a73b2520b5d9be3b151ad2b'),
    (0x101d56b0, 0x101d56b8, 2, 'df65c229fc24ebbee6d47f19d7cd88932671d530a16e76a5b3a7350f46ce77a8'),
    (0x101d9080, 0x101d91a4, 73, '7ecc2777d4e1bee894754c5b74b423b72bd25d1758b5062c45265fa9546f14c3'),
    (0x101d91b0, 0x101d939c, 123, '6de714f2da154207b958be7d18ada02959f405cb85c708196bec8a146231969a'),
    (0x101d93a0, 0x101d93a8, 2, '9d938e14d2ced70323aad8143eb42c0773bf3d8c0693abf8d65434a02acc76d5'),
)
FRAGMENTS = (
    (0x101d5670, 0x101d5, 2, '4e9fac9d50f21ea23085b9cbfa4c4501151ea88c9a73b2520b5d9be3b151ad2b'),
    (0x101d56b0, 0x101d5, 2, 'df65c229fc24ebbee6d47f19d7cd88932671d530a16e76a5b3a7350f46ce77a8'),
    (0x101d9080, 0x101d9, 73, '7ecc2777d4e1bee894754c5b74b423b72bd25d1758b5062c45265fa9546f14c3'),
    (0x101d91b0, 0x101d9, 123, '6de714f2da154207b958be7d18ada02959f405cb85c708196bec8a146231969a'),
    (0x101d93a0, 0x101d9, 2, '9d938e14d2ced70323aad8143eb42c0773bf3d8c0693abf8d65434a02acc76d5'),
)
ACTOR_ORDER_ENTRIES = tuple(start for start, _, _, _ in CONTRACTS)


def fingerprint(body):
    canonical = [{key: r.get(key) for key in ('address', 'assembly', 'ops', 'flows', 'fallthrough')} for r in body]
    return hashlib.sha256(json.dumps(canonical, sort_keys=True, separators=(',', ':')).encode()).hexdigest()


def require_actor_order_bodies(instructions, complete=False):
    for start, end, count, expected in CONTRACTS:
        body = [r for r in instructions if start <= r['address'] < end]
        if not body and not complete: continue
        if len(body) != count or fingerprint(body) != expected:
            raise ValueError(f'Mac actor-order contract changed or missing at {start:x}')


def require_actor_order_edges(edges):
    for source, target in edges:
        for start, end, _, _ in CONTRACTS:
            if start < target < end and not start <= source < end:
                raise ValueError(f'External actor-order interior edge from {source:x} to {target:x}')


def require_actor_order_contract(page, instructions, selected=None):
    addresses = {r['address'] for r in instructions} | set(selected or ())
    for start, end, _, _ in CONTRACTS:
        if not start >> 12 <= page <= (end - 1) >> 12: continue
        low, high = max(start, page << 12), min(end, (page + 1) << 12)
        if start not in addresses and not any(low <= a < high for a in addresses): continue
        body = [r for r in instructions if low <= r['address'] < high]
        _, _, count, expected = next(f for f in FRAGMENTS if f[0] == start and f[1] == page)
        if len(body) != count or fingerprint(body) != expected:
            raise ValueError(f'Mac actor-order contract changed or missing at {start:x} page {page:x}')


def require_actor_order_index(db, selected=None):
    records = [json.loads(row[0]) for start, end, _, _ in CONTRACTS for row in db.execute(
        'SELECT body FROM instructions WHERE address>=? AND address<? ORDER BY address', (start, end))]
    if selected is None: require_actor_order_bodies(records, complete=True)
    else:
        for page in sorted({f[1] for f in FRAGMENTS}):
            require_actor_order_contract(page, [r for _, r in sorted(selected.items())], selected)
    require_actor_order_edges(db.execute("""SELECT i.address,e.value
        FROM instructions AS i,
          json_each(json_insert(json_extract(i.body,'$.flows'),'$[#]',json_extract(i.body,'$.fallthrough'))) AS e
        WHERE e.value>=? AND e.value<?""", (CONTRACTS[0][0], CONTRACTS[-1][1])))


def actor_order_hook(entry):
    return f'if (try_modern_actor_order(s, 0x{entry:x})) return true;'


def require_actor_order_hooks(page, instructions, code):
    addresses = {r['address'] for r in instructions}
    for entry in ACTOR_ORDER_ENTRIES:
        if entry >> 12 != page or entry not in addresses: continue
        label, hook = f'i_{entry:x}:', actor_order_hook(entry)
        fragment = code.split(label, 1)[-1].split(f'i_{entry + 4:x}:', 1)[0]
        steps = [token for token in (f's.step(0x{entry:x});', f'counter.step(0x{entry:x});') if token in fragment]
        if code.count(hook) != 1 or hook not in fragment or len(steps) != 1 or fragment.index(hook) > fragment.index(steps[0]):
            raise ValueError(f'Missing or misplaced actor-order hook at {entry:x}')
