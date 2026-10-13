"""Pinned complete team spatial-query boundaries."""
import hashlib
import json

CONTRACTS = (
    (0x101e32e0, 0x101e3d50, 668, '65448f2c00607b7593fa776b02fcf302e6bed0d9fdee774ad7ebb8d55db07e33'),
    (0x101e3f10, 0x101e4020, 68, '07ce0ebd5d1c1465c0c2f960215b4b87b940eae80c8be24aef8dcf29fcb0d368'),
    (0x101e4020, 0x101e4128, 66, 'c6dfb93fa084fa2ddaff64c2b79f1e6b750887de139737338df56f39ca57ece2'),
    (0x101e4130, 0x101e4e08, 822, 'ce113c94fefa412571d45d5cce76ef2a69ba8e4770e7f1076edafa7824315580'),
)
FRAGMENTS = (
    (0x101e32e0, 0x101e3, 668, '65448f2c00607b7593fa776b02fcf302e6bed0d9fdee774ad7ebb8d55db07e33'),
    (0x101e3f10, 0x101e3, 60, '2be4c08ff11377b9569e6d26c31b4e080e964c95674da9a7cb3774e425e24528'),
    (0x101e3f10, 0x101e4, 8, 'c5da9a0d89e53aba41d733a8dc3bf79e32d64dd7771b27ceedfe759bc4bbd6d0'),
    (0x101e4020, 0x101e4, 66, 'c6dfb93fa084fa2ddaff64c2b79f1e6b750887de139737338df56f39ca57ece2'),
    (0x101e4130, 0x101e4, 822, 'ce113c94fefa412571d45d5cce76ef2a69ba8e4770e7f1076edafa7824315580'),
)
TEAM_SPATIAL_ENTRIES = tuple(start for start, _, _, _ in CONTRACTS)


def fingerprint(body):
    canonical = [{key: r.get(key) for key in ('address', 'assembly', 'ops', 'flows', 'fallthrough')} for r in body]
    return hashlib.sha256(json.dumps(canonical, sort_keys=True, separators=(',', ':')).encode()).hexdigest()


def require_team_spatial_bodies(instructions, complete=False):
    for start, end, count, expected in CONTRACTS:
        body = [r for r in instructions if start <= r['address'] < end]
        if not body and not complete: continue
        if len(body) != count or fingerprint(body) != expected:
            raise ValueError(f'Mac team-spatial contract changed or missing at {start:x}')


def require_team_spatial_edges(edges):
    for source, target in edges:
        for start, end, _, _ in CONTRACTS:
            if start < target < end and not start <= source < end:
                raise ValueError(f'External team-spatial interior edge from {source:x} to {target:x}')


def require_team_spatial_contract(page, instructions, selected=None):
    addresses = {r['address'] for r in instructions} | set(selected or ())
    for start, end, _, _ in CONTRACTS:
        if not start >> 12 <= page <= (end - 1) >> 12: continue
        low, high = max(start, page << 12), min(end, (page + 1) << 12)
        if start not in addresses and not any(low <= a < high for a in addresses): continue
        body = [r for r in instructions if low <= r['address'] < high]
        _, _, count, expected = next(f for f in FRAGMENTS if f[0] == start and f[1] == page)
        if len(body) != count or fingerprint(body) != expected:
            raise ValueError(f'Mac team-spatial contract changed or missing at {start:x} page {page:x}')


def require_team_spatial_index(db, selected=None):
    records = [json.loads(row[0]) for start, end, _, _ in CONTRACTS for row in db.execute(
        'SELECT body FROM instructions WHERE address>=? AND address<? ORDER BY address', (start, end))]
    if selected is None: require_team_spatial_bodies(records, complete=True)
    else:
        for page in sorted({f[1] for f in FRAGMENTS}):
            require_team_spatial_contract(page, [r for _, r in sorted(selected.items())], selected)
    require_team_spatial_edges(db.execute("""SELECT i.address,e.value
        FROM instructions AS i,
          json_each(json_insert(json_extract(i.body,'$.flows'),'$[#]',json_extract(i.body,'$.fallthrough'))) AS e
        WHERE e.value>=? AND e.value<?""", (CONTRACTS[0][0], CONTRACTS[-1][1])))


def team_spatial_hook(entry):
    return f'if (try_modern_team_spatial(s, 0x{entry:x})) return true;'


def require_team_spatial_hooks(page, instructions, code):
    addresses = {r['address'] for r in instructions}
    for entry in TEAM_SPATIAL_ENTRIES:
        if entry >> 12 != page or entry not in addresses: continue
        label, hook = f'i_{entry:x}:', team_spatial_hook(entry)
        fragment = code.split(label, 1)[-1].split(f'i_{entry + 4:x}:', 1)[0]
        steps = [token for token in (f's.step(0x{entry:x});', f'counter.step(0x{entry:x});') if token in fragment]
        if code.count(hook) != 1 or hook not in fragment or len(steps) != 1 or fragment.index(hook) > fragment.index(steps[0]):
            raise ValueError(f'Missing or misplaced team-spatial hook at {entry:x}')
