"""Pinned watched/background callers and shared match advancement boundaries."""
import hashlib
import json

CONTRACTS = (
    (0x101ccdb0, 0x101cd250, 296, '527418ba2a681667ed734b16006cc80e36568a22bd035c60cec4920b59caa6b4'),
    (0x101cd5c0, 0x101cd87c, 174, 'af00bafe26be1f80c76935e07dbc09158f51549602b92942c2aee4a6b6cfd2d6'),
    (0x101d3f60, 0x101d5658, 1470, 'c21ff22c3c40908b1dd40760a36f6a4444dc33f499b042205187c7bb451eae10'),
)
FRAGMENTS = (
    (0x101ccdb0, 0x101cc, 148, '0374d6d34b764c8d509da79b216ffdc0c2c0c0f853d618ba2c2733a711ddc67d'),
    (0x101ccdb0, 0x101cd, 148, '5861c9745d2fa7d9f428e4a772bf6cbee864a6893c0ef4889874afb648f0aff7'),
    (0x101cd5c0, 0x101cd, 174, 'af00bafe26be1f80c76935e07dbc09158f51549602b92942c2aee4a6b6cfd2d6'),
    (0x101d3f60, 0x101d3, 40, 'b291133fac3417c65502133dcc7b330e7e7c4dcf5df2eeebc9dc06730266340a'),
    (0x101d3f60, 0x101d4, 1024, '588fa0b3961a32366654c38893fdc1c19cc0b62a172b4c6b4744f9360ee286b4'),
    (0x101d3f60, 0x101d5, 406, '985765550a5470b86923c782abc9f2a8a0f2a5ba60be8ab4c4b90d3016ca1f4f'),
)
MATCH_ENGINE_ENTRIES = tuple(start for start, _, _, _ in CONTRACTS)


def fingerprint(body):
    canonical = [{key: r.get(key) for key in ('address', 'assembly', 'ops', 'flows', 'fallthrough')} for r in body]
    return hashlib.sha256(json.dumps(canonical, sort_keys=True, separators=(',', ':')).encode()).hexdigest()


def require_match_engine_bodies(instructions, complete=False):
    for start, end, count, expected in CONTRACTS:
        body = [r for r in instructions if start <= r['address'] < end]
        if not body and not complete: continue
        if len(body) != count or fingerprint(body) != expected:
            raise ValueError(f'Mac match-engine contract changed or missing at {start:x}')


def require_match_engine_edges(edges):
    for source, target in edges:
        for start, end, _, _ in CONTRACTS:
            if start < target < end and not start <= source < end:
                raise ValueError(f'External match-engine interior edge from {source:x} to {target:x}')


def require_match_engine_contract(page, instructions, selected=None):
    addresses = {r['address'] for r in instructions} | set(selected or ())
    for start, end, _, _ in CONTRACTS:
        if not start >> 12 <= page <= (end - 1) >> 12: continue
        low, high = max(start, page << 12), min(end, (page + 1) << 12)
        if not any(low <= a < high for a in addresses): continue
        body = [r for r in instructions if low <= r['address'] < high]
        _, _, count, expected = next(f for f in FRAGMENTS if f[0] == start and f[1] == page)
        if len(body) != count or fingerprint(body) != expected:
            raise ValueError(f'Mac match-engine contract changed or missing at {start:x} page {page:x}')


def require_match_engine_index(db, selected=None):
    records = [json.loads(row[0]) for start, end, _, _ in CONTRACTS for row in db.execute(
        'SELECT body FROM instructions WHERE address>=? AND address<? ORDER BY address', (start, end))]
    if selected is None: require_match_engine_bodies(records, complete=True)
    else:
        for page in sorted({f[1] for f in FRAGMENTS}):
            require_match_engine_contract(page, [r for _, r in sorted(selected.items())], selected)
    require_match_engine_edges(db.execute("""SELECT i.address,e.value
        FROM instructions AS i,
          json_each(json_insert(json_extract(i.body,'$.flows'),'$[#]',json_extract(i.body,'$.fallthrough'))) AS e
        WHERE e.value>=? AND e.value<?""", (CONTRACTS[0][0], CONTRACTS[-1][1])))


def match_engine_hook(entry):
    return f'if (try_modern_match_engine(s, 0x{entry:x})) return true;'


def require_match_engine_hooks(page, instructions, code):
    addresses = {r['address'] for r in instructions}
    for entry in MATCH_ENGINE_ENTRIES:
        if entry >> 12 != page or entry not in addresses: continue
        label, hook = f'i_{entry:x}:', match_engine_hook(entry)
        fragment = code.split(label, 1)[-1].split(f'i_{entry + 4:x}:', 1)[0]
        steps = [token for token in (f's.step(0x{entry:x});', f'counter.step(0x{entry:x});') if token in fragment]
        if code.count(hook) != 1 or hook not in fragment or len(steps) != 1 or fragment.index(hook) > fragment.index(steps[0]):
            raise ValueError(f'Missing or misplaced match-engine hook at {entry:x}')
