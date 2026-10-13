"""Pinned complete pitch distance-query boundaries."""
import hashlib
import json

CONTRACTS = (
    (0x101ef8d0, 0x101ef99c, 51, 'b7ba1b2e3f494dfc9a679f8686abaa4730380698f1e0f4bc8de39b77fa8bb6e3'),
    (0x1022e760, 0x1022e7dc, 31, '284c993cbb789a23f29e9b5d268fc78b5c537a9499c4a9144de9f956b3ca71a4'),
    (0x10245160, 0x1024521c, 47, 'e66478ca6e3837188c6bc26e1537a3223deeaca687cf19e679cdc0fa4ead6ef1'),
)
FRAGMENTS = (
    (0x101ef8d0, 0x101ef, 51, 'b7ba1b2e3f494dfc9a679f8686abaa4730380698f1e0f4bc8de39b77fa8bb6e3'),
    (0x1022e760, 0x1022e, 31, '284c993cbb789a23f29e9b5d268fc78b5c537a9499c4a9144de9f956b3ca71a4'),
    (0x10245160, 0x10245, 47, 'e66478ca6e3837188c6bc26e1537a3223deeaca687cf19e679cdc0fa4ead6ef1'),
)
PITCH_DISTANCE_ENTRIES = tuple(start for start, _, _, _ in CONTRACTS)


def fingerprint(body):
    canonical = [{key: r.get(key) for key in ('address', 'assembly', 'ops', 'flows', 'fallthrough')} for r in body]
    return hashlib.sha256(json.dumps(canonical, sort_keys=True, separators=(',', ':')).encode()).hexdigest()


def require_pitch_distance_bodies(instructions, complete=False):
    for start, end, count, expected in CONTRACTS:
        body = [r for r in instructions if start <= r['address'] < end]
        if not body and not complete: continue
        if len(body) != count or fingerprint(body) != expected:
            raise ValueError(f'Mac pitch-distance contract changed or missing at {start:x}')


def require_pitch_distance_edges(edges):
    for source, target in edges:
        for start, end, _, _ in CONTRACTS:
            if start < target < end and not start <= source < end:
                raise ValueError(f'External pitch-distance interior edge from {source:x} to {target:x}')


def require_pitch_distance_contract(page, instructions, selected=None):
    addresses = {r['address'] for r in instructions} | set(selected or ())
    for start, end, _, _ in CONTRACTS:
        if not start >> 12 <= page <= (end - 1) >> 12: continue
        low, high = max(start, page << 12), min(end, (page + 1) << 12)
        if start not in addresses and not any(low <= a < high for a in addresses): continue
        body = [r for r in instructions if low <= r['address'] < high]
        _, _, count, expected = next(f for f in FRAGMENTS if f[0] == start and f[1] == page)
        if len(body) != count or fingerprint(body) != expected:
            raise ValueError(f'Mac pitch-distance contract changed or missing at {start:x} page {page:x}')


def require_pitch_distance_index(db, selected=None):
    records = [json.loads(row[0]) for start, end, _, _ in CONTRACTS for row in db.execute(
        'SELECT body FROM instructions WHERE address>=? AND address<? ORDER BY address', (start, end))]
    if selected is None: require_pitch_distance_bodies(records, complete=True)
    else:
        for page in sorted({f[1] for f in FRAGMENTS}):
            require_pitch_distance_contract(page, [r for _, r in sorted(selected.items())], selected)
    require_pitch_distance_edges(db.execute("""SELECT i.address,e.value
        FROM instructions AS i,
          json_each(json_insert(json_extract(i.body,'$.flows'),'$[#]',json_extract(i.body,'$.fallthrough'))) AS e
        WHERE e.value>=? AND e.value<?""", (CONTRACTS[0][0], CONTRACTS[-1][1])))


def pitch_distance_hook(entry):
    return f'if (try_modern_pitch_distance(s, 0x{entry:x})) return true;'


def require_pitch_distance_hooks(page, instructions, code):
    addresses = {r['address'] for r in instructions}
    for entry in PITCH_DISTANCE_ENTRIES:
        if entry >> 12 != page or entry not in addresses: continue
        label, hook = f'i_{entry:x}:', pitch_distance_hook(entry)
        fragment = code.split(label, 1)[-1].split(f'i_{entry + 4:x}:', 1)[0]
        steps = [token for token in (f's.step(0x{entry:x});', f'counter.step(0x{entry:x});') if token in fragment]
        if code.count(hook) != 1 or hook not in fragment or len(steps) != 1 or fragment.index(hook) > fragment.index(steps[0]):
            raise ValueError(f'Missing or misplaced pitch-distance hook at {entry:x}')
