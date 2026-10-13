"""Pinned complete original match range sampler boundary."""
import hashlib
import json

CONTRACTS = ((0x101d3c90, 0x101d3e48, 110, '17e9330216425b9b39ef2830f09567441660ba1c2fc6282a6009aad902b2eaa9'),)
FRAGMENTS = ((0x101d3c90, 0x101d3, 110, '17e9330216425b9b39ef2830f09567441660ba1c2fc6282a6009aad902b2eaa9'),)
MATCH_RANDOM_ENTRIES = tuple(start for start, _, _, _ in CONTRACTS)


def fingerprint(body):
    canonical = [{key: r.get(key) for key in ('address', 'assembly', 'ops', 'flows', 'fallthrough')} for r in body]
    return hashlib.sha256(json.dumps(canonical, sort_keys=True, separators=(',', ':')).encode()).hexdigest()


def require_match_random_bodies(instructions, complete=False):
    for start, end, count, expected in CONTRACTS:
        body = [r for r in instructions if start <= r['address'] < end]
        if not body and not complete: continue
        if len(body) != count or fingerprint(body) != expected:
            raise ValueError(f'Mac match-random contract changed or missing at {start:x}')


def require_match_random_edges(edges):
    for source, target in edges:
        for start, end, _, _ in CONTRACTS:
            if start < target < end and not start <= source < end:
                raise ValueError(f'External match-random interior edge from {source:x} to {target:x}')


def require_match_random_contract(page, instructions, selected=None):
    addresses = {r['address'] for r in instructions} | set(selected or ())
    for start, end, _, _ in CONTRACTS:
        if not start >> 12 <= page <= (end - 1) >> 12: continue
        low, high = max(start, page << 12), min(end, (page + 1) << 12)
        if start not in addresses and not any(low <= a < high for a in addresses): continue
        body = [r for r in instructions if low <= r['address'] < high]
        _, _, count, expected = next(f for f in FRAGMENTS if f[0] == start and f[1] == page)
        if len(body) != count or fingerprint(body) != expected:
            raise ValueError(f'Mac match-random contract changed or missing at {start:x} page {page:x}')


def require_match_random_index(db, selected=None):
    records = [json.loads(row[0]) for start, end, _, _ in CONTRACTS for row in db.execute(
        'SELECT body FROM instructions WHERE address>=? AND address<? ORDER BY address', (start, end))]
    if selected is None: require_match_random_bodies(records, complete=True)
    else:
        for page in sorted({f[1] for f in FRAGMENTS}):
            require_match_random_contract(page, [r for _, r in sorted(selected.items())], selected)
    require_match_random_edges(db.execute("""SELECT i.address,e.value
        FROM instructions AS i,
          json_each(json_insert(json_extract(i.body,'$.flows'),'$[#]',json_extract(i.body,'$.fallthrough'))) AS e
        WHERE e.value>=? AND e.value<?""", (CONTRACTS[0][0], CONTRACTS[-1][1])))


def match_random_hook(entry):
    return f'if (try_modern_match_random(s, 0x{entry:x})) return true;'


def require_match_random_hooks(page, instructions, code):
    addresses = {r['address'] for r in instructions}
    for entry in MATCH_RANDOM_ENTRIES:
        if entry >> 12 != page or entry not in addresses: continue
        label, hook = f'i_{entry:x}:', match_random_hook(entry)
        fragment = code.split(label, 1)[-1].split(f'i_{entry + 4:x}:', 1)[0]
        steps = [token for token in (f's.step(0x{entry:x});', f'counter.step(0x{entry:x});') if token in fragment]
        if code.count(hook) != 1 or hook not in fragment or len(steps) != 1 or fragment.index(hook) > fragment.index(steps[0]):
            raise ValueError(f'Missing or misplaced match-random hook at {entry:x}')
