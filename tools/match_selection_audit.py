"""Pinned complete original match team selection and preset target boundaries."""
import hashlib
import json

CONTRACTS = ((270408656, 270409972, 329, 'd6754012226c703c7ac84c15470418bd8f36bc73aa9f429eab33c8ebc00817ec'), (270409984, 270410456, 118, '3bdf67dbb5a7f3c3b64ebfd44fefbd2bc0b6cbc4690e467ce1a4dfcd6118f71e'), (270410464, 270411160, 174, '7b5ee98fc508cebf4764f16a761e677bc102070780a3cad9a9ad6124f760b316'), (270411168, 270411944, 194, '48cd5e6c11be438b3746e0e463fb4b4cd7deb73b46276124c4bf6eeaa096c50e'))
FRAGMENTS = ((270408656, 66017, 268, '9169ed9a1ab39312ca9428e7439790159d6688e96201e10ea89b997857d0935f'), (270408656, 66018, 61, 'caa45355bc7ab23eb12c1527807b5150f614ac884ec92d386f8199924a7f3d81'), (270409984, 66018, 118, '3bdf67dbb5a7f3c3b64ebfd44fefbd2bc0b6cbc4690e467ce1a4dfcd6118f71e'), (270410464, 66018, 174, '7b5ee98fc508cebf4764f16a761e677bc102070780a3cad9a9ad6124f760b316'), (270411168, 66018, 194, '48cd5e6c11be438b3746e0e463fb4b4cd7deb73b46276124c4bf6eeaa096c50e'))
MATCH_SELECTION_ENTRIES = tuple(start for start, _, _, _ in CONTRACTS)


def fingerprint(body):
    canonical = [{key: r.get(key) for key in ('address', 'assembly', 'ops', 'flows', 'fallthrough')} for r in body]
    return hashlib.sha256(json.dumps(canonical, sort_keys=True, separators=(',', ':')).encode()).hexdigest()


def require_match_selection_bodies(instructions, complete=False):
    for start, end, count, expected in CONTRACTS:
        body = [r for r in instructions if start <= r['address'] < end]
        if not body and not complete: continue
        if len(body) != count or fingerprint(body) != expected:
            raise ValueError(f'Mac match-selection contract changed or missing at {start:x}')


def require_match_selection_edges(edges):
    for source, target in edges:
        for start, end, _, _ in CONTRACTS:
            if start < target < end and not start <= source < end:
                raise ValueError(f'External match-selection interior edge from {source:x} to {target:x}')


def require_match_selection_contract(page, instructions, selected=None):
    addresses = {r['address'] for r in instructions} | set(selected or ())
    for start, end, _, _ in CONTRACTS:
        if not start >> 12 <= page <= (end - 1) >> 12: continue
        low, high = max(start, page << 12), min(end, (page + 1) << 12)
        if start not in addresses and not any(low <= a < high for a in addresses): continue
        body = [r for r in instructions if low <= r['address'] < high]
        _, _, count, expected = next(f for f in FRAGMENTS if f[0] == start and f[1] == page)
        if len(body) != count or fingerprint(body) != expected:
            raise ValueError(f'Mac match-selection contract changed or missing at {start:x} page {page:x}')


def require_match_selection_index(db, selected=None):
    records = [json.loads(row[0]) for start, end, _, _ in CONTRACTS for row in db.execute(
        'SELECT body FROM instructions WHERE address>=? AND address<? ORDER BY address', (start, end))]
    if selected is None: require_match_selection_bodies(records, complete=True)
    else:
        for page in sorted({f[1] for f in FRAGMENTS}):
            require_match_selection_contract(page, [r for _, r in sorted(selected.items())], selected)
    require_match_selection_edges(db.execute("""SELECT i.address,e.value
        FROM instructions AS i,
          json_each(json_insert(json_extract(i.body,'$.flows'),'$[#]',json_extract(i.body,'$.fallthrough'))) AS e
        WHERE e.value>=? AND e.value<?""", (min(v[0] for v in CONTRACTS), max(v[1] for v in CONTRACTS))))


def match_selection_hook(entry):
    return f'if (try_modern_match_selection(s, 0x{entry:x})) return true;'


def require_match_selection_hooks(page, instructions, code):
    addresses = {r['address'] for r in instructions}
    for entry in MATCH_SELECTION_ENTRIES:
        if entry >> 12 != page or entry not in addresses: continue
        label, hook = f'i_{entry:x}:', match_selection_hook(entry)
        fragment = code.split(label, 1)[-1].split(f'i_{entry + 4:x}:', 1)[0]
        steps = [token for token in (f's.step(0x{entry:x});', f'counter.step(0x{entry:x});') if token in fragment]
        if code.count(hook) != 1 or hook not in fragment or len(steps) != 1 or fragment.index(hook) > fragment.index(steps[0]):
            raise ValueError(f'Missing or misplaced match-selection hook at {entry:x}')
