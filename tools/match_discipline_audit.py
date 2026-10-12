"""Pinned complete foul/card outcomes and match facades boundaries."""
import hashlib
import json

CONTRACTS = (
    (0x101f5110, 0x101f5180, 28, 'e33c3207298656fa995e0f9cf77a8d0573d9dc544e2e88fbf9dc2c69dd0257b7'),
    (0x101f5180, 0x101f51e0, 24, '6ef24ee6579d01008c5cbb4dadfd5f0df5f3da506b1176a1b6b3f86827a897d0'),
    (0x101f51e0, 0x101f5230, 20, '3274ce7971e69e18a96ab3de8ac3fced00f965be4b5de62615223b805decb89e'),
    (0x1021bec0, 0x1021c254, 229, '72b80ba67c08c1bb985561ef1371e361521cfd5279ce067fb0a704126b99f627'),
    (0x10221e00, 0x10222290, 292, '76d8fed64694d71aeb8750500fe29d0950645c18c50bb3c5caa96cd46875e8a1'),
    (0x10223100, 0x10223894, 485, '1b79c0902b04c8b888fd472d7fba5d6845d1a25bfdf4d442d19b0d453addbf26'),
)
FRAGMENTS = (
    (0x101f5110, 0x101f5, 28, 'e33c3207298656fa995e0f9cf77a8d0573d9dc544e2e88fbf9dc2c69dd0257b7'),
    (0x101f5180, 0x101f5, 24, '6ef24ee6579d01008c5cbb4dadfd5f0df5f3da506b1176a1b6b3f86827a897d0'),
    (0x101f51e0, 0x101f5, 20, '3274ce7971e69e18a96ab3de8ac3fced00f965be4b5de62615223b805decb89e'),
    (0x1021bec0, 0x1021b, 80, 'bdf5bad3046c16a159d2b60a71e9822c7994c3e6a2a4f6e060651d6ced90f822'),
    (0x1021bec0, 0x1021c, 149, '867aeaa55345a54feaf41f1fea6fd98e27b4ef7df0e968c0bb2c6722dbbd322b'),
    (0x10221e00, 0x10221, 128, '67c7bdbeb05e61620cb668fb1b5f67c56528994e9e345997657c2b6652e9557a'),
    (0x10221e00, 0x10222, 164, 'f1a2e5334c5fcce38861482689ee5b9cae9874fbe5e9ca11311decac4c986a5f'),
    (0x10223100, 0x10223, 485, '1b79c0902b04c8b888fd472d7fba5d6845d1a25bfdf4d442d19b0d453addbf26'),
)
MATCH_DISCIPLINE_ENTRIES = tuple(start for start, _, _, _ in CONTRACTS)


def fingerprint(body):
    canonical = [{key: r.get(key) for key in ('address', 'assembly', 'ops', 'flows', 'fallthrough')} for r in body]
    return hashlib.sha256(json.dumps(canonical, sort_keys=True, separators=(',', ':')).encode()).hexdigest()


def require_match_discipline_bodies(instructions, complete=False):
    for start, end, count, expected in CONTRACTS:
        body = [r for r in instructions if start <= r['address'] < end]
        if not body and not complete: continue
        if len(body) != count or fingerprint(body) != expected:
            raise ValueError(f'Mac match-discipline contract changed or missing at {start:x}')


def require_match_discipline_edges(edges):
    for source, target in edges:
        for start, end, _, _ in CONTRACTS:
            if start < target < end and not start <= source < end:
                raise ValueError(f'External match-discipline interior edge from {source:x} to {target:x}')


def require_match_discipline_contract(page, instructions, selected=None):
    addresses = {r['address'] for r in instructions} | set(selected or ())
    for start, end, _, _ in CONTRACTS:
        if not start >> 12 <= page <= (end - 1) >> 12: continue
        low, high = max(start, page << 12), min(end, (page + 1) << 12)
        if start not in addresses and not any(low <= a < high for a in addresses): continue
        body = [r for r in instructions if low <= r['address'] < high]
        _, _, count, expected = next(f for f in FRAGMENTS if f[0] == start and f[1] == page)
        if len(body) != count or fingerprint(body) != expected:
            raise ValueError(f'Mac match-discipline contract changed or missing at {start:x} page {page:x}')


def require_match_discipline_index(db, selected=None):
    records = [json.loads(row[0]) for start, end, _, _ in CONTRACTS for row in db.execute(
        'SELECT body FROM instructions WHERE address>=? AND address<? ORDER BY address', (start, end))]
    if selected is None: require_match_discipline_bodies(records, complete=True)
    else:
        for page in sorted({f[1] for f in FRAGMENTS}):
            require_match_discipline_contract(page, [r for _, r in sorted(selected.items())], selected)
    require_match_discipline_edges(db.execute("""SELECT i.address,e.value
        FROM instructions AS i,
          json_each(json_insert(json_extract(i.body,'$.flows'),'$[#]',json_extract(i.body,'$.fallthrough'))) AS e
        WHERE e.value>=? AND e.value<?""", (CONTRACTS[0][0], CONTRACTS[-1][1])))


def match_discipline_hook(entry):
    return f'if (try_modern_match_discipline(s, 0x{entry:x})) return true;'


def require_match_discipline_hooks(page, instructions, code):
    addresses = {r['address'] for r in instructions}
    for entry in MATCH_DISCIPLINE_ENTRIES:
        if entry >> 12 != page or entry not in addresses: continue
        label, hook = f'i_{entry:x}:', match_discipline_hook(entry)
        fragment = code.split(label, 1)[-1].split(f'i_{entry + 4:x}:', 1)[0]
        steps = [token for token in (f's.step(0x{entry:x});', f'counter.step(0x{entry:x});') if token in fragment]
        if code.count(hook) != 1 or hook not in fragment or len(steps) != 1 or fragment.index(hook) > fragment.index(steps[0]):
            raise ValueError(f'Missing or misplaced match-discipline hook at {entry:x}')
