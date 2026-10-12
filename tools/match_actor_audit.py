"""Pinned injury withdrawal and original position access boundaries."""
import hashlib
import json

CONTRACTS = (
    (0x101f1530, 0x101f185c, 203, 'a0cfcad85bee03beab0f9c951255b35730f6fa155105c04ef98b6dc4b6b6c276'),
    (0x101f1860, 0x101f1868, 2, 'bd8775d0ee86dac6ef8927a89159675b44f054731e6de52b6da1c1fe1e0bbc5d'),
    (0x101f1870, 0x101f1878, 2, '724d580a79bad141d3e8e66521be258dd35aa6803580f7e3a6c83ca123a5b716'),
)
FRAGMENTS = (
    (0x101f1530, 0x101f1, 203, 'a0cfcad85bee03beab0f9c951255b35730f6fa155105c04ef98b6dc4b6b6c276'),
    (0x101f1860, 0x101f1, 2, 'bd8775d0ee86dac6ef8927a89159675b44f054731e6de52b6da1c1fe1e0bbc5d'),
    (0x101f1870, 0x101f1, 2, '724d580a79bad141d3e8e66521be258dd35aa6803580f7e3a6c83ca123a5b716'),
)
MATCH_ACTOR_ENTRIES = tuple(start for start, _, _, _ in CONTRACTS)


def fingerprint(body):
    canonical = [{key: r.get(key) for key in ('address', 'assembly', 'ops', 'flows', 'fallthrough')} for r in body]
    return hashlib.sha256(json.dumps(canonical, sort_keys=True, separators=(',', ':')).encode()).hexdigest()


def require_match_actor_bodies(instructions, complete=False):
    for start, end, count, expected in CONTRACTS:
        body = [r for r in instructions if start <= r['address'] < end]
        if not body and not complete: continue
        if len(body) != count or fingerprint(body) != expected:
            raise ValueError(f'Mac match-actor contract changed or missing at {start:x}')


def require_match_actor_edges(edges):
    for source, target in edges:
        for start, end, _, _ in CONTRACTS:
            if start < target < end and not start <= source < end:
                raise ValueError(f'External match-actor interior edge from {source:x} to {target:x}')


def require_match_actor_contract(page, instructions, selected=None):
    addresses = {r['address'] for r in instructions} | set(selected or ())
    for start, end, _, _ in CONTRACTS:
        if not start >> 12 <= page <= (end - 1) >> 12: continue
        low, high = max(start, page << 12), min(end, (page + 1) << 12)
        if not any(low <= a < high for a in addresses): continue
        body = [r for r in instructions if low <= r['address'] < high]
        _, _, count, expected = next(f for f in FRAGMENTS if f[0] == start and f[1] == page)
        if len(body) != count or fingerprint(body) != expected:
            raise ValueError(f'Mac match-actor contract changed or missing at {start:x} page {page:x}')


def require_match_actor_index(db, selected=None):
    records = [json.loads(row[0]) for start, end, _, _ in CONTRACTS for row in db.execute(
        'SELECT body FROM instructions WHERE address>=? AND address<? ORDER BY address', (start, end))]
    if selected is None: require_match_actor_bodies(records, complete=True)
    else:
        for page in sorted({f[1] for f in FRAGMENTS}):
            require_match_actor_contract(page, [r for _, r in sorted(selected.items())], selected)
    require_match_actor_edges(db.execute("""SELECT i.address,e.value
        FROM instructions AS i,
          json_each(json_insert(json_extract(i.body,'$.flows'),'$[#]',json_extract(i.body,'$.fallthrough'))) AS e
        WHERE e.value>=? AND e.value<?""", (CONTRACTS[0][0], CONTRACTS[-1][1])))


def match_actor_hook(entry):
    return f'if (try_modern_match_actor(s, 0x{entry:x})) return true;'


def require_match_actor_hooks(page, instructions, code):
    addresses = {r['address'] for r in instructions}
    for entry in MATCH_ACTOR_ENTRIES:
        if entry >> 12 != page or entry not in addresses: continue
        label, hook = f'i_{entry:x}:', match_actor_hook(entry)
        fragment = code.split(label, 1)[-1].split(f'i_{entry + 4:x}:', 1)[0]
        steps = [token for token in (f's.step(0x{entry:x});', f'counter.step(0x{entry:x});') if token in fragment]
        if code.count(hook) != 1 or hook not in fragment or len(steps) != 1 or fragment.index(hook) > fragment.index(steps[0]):
            raise ValueError(f'Missing or misplaced match-actor hook at {entry:x}')
