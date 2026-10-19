"""Pinned complete original match context/score selection boundaries."""
import hashlib
import json

CONTRACTS = ((270407232, 270408260, 257, '45e35cf7ee78cfd85eaafb1a7ff186d7dbf533884055650e792ee79ff660a1c7'), (270408272, 270408644, 93, '5954ad4103d5dcc8393e844626bbdeef5744d327a23c8b9fac74ec1f2b37a587'), (271087088, 271087288, 50, 'e611340254b666f0d88c0ed326a6dd8b610f81258f510259a8bab73be9fe0035'))
FRAGMENTS = ((270407232, 66017, 257, '45e35cf7ee78cfd85eaafb1a7ff186d7dbf533884055650e792ee79ff660a1c7'), (270408272, 66017, 93, '5954ad4103d5dcc8393e844626bbdeef5744d327a23c8b9fac74ec1f2b37a587'), (271087088, 66183, 50, 'e611340254b666f0d88c0ed326a6dd8b610f81258f510259a8bab73be9fe0035'))
MATCH_SELECTION_SUPPORT_ENTRIES = tuple(start for start, _, _, _ in CONTRACTS)


def fingerprint(body):
    canonical = [{key: r.get(key) for key in ('address', 'assembly', 'ops', 'flows', 'fallthrough')} for r in body]
    return hashlib.sha256(json.dumps(canonical, sort_keys=True, separators=(',', ':')).encode()).hexdigest()


def require_match_selection_support_bodies(instructions, complete=False):
    for start, end, count, expected in CONTRACTS:
        body = [r for r in instructions if start <= r['address'] < end]
        if not body and not complete: continue
        if len(body) != count or fingerprint(body) != expected:
            raise ValueError(f'Mac match-selection-support contract changed or missing at {start:x}')


def require_match_selection_support_edges(edges):
    for source, target in edges:
        for start, end, _, _ in CONTRACTS:
            if start < target < end and not start <= source < end:
                raise ValueError(f'External match-selection-support interior edge from {source:x} to {target:x}')


def require_match_selection_support_contract(page, instructions, selected=None):
    addresses = {r['address'] for r in instructions} | set(selected or ())
    for start, end, _, _ in CONTRACTS:
        if not start >> 12 <= page <= (end - 1) >> 12: continue
        low, high = max(start, page << 12), min(end, (page + 1) << 12)
        if start not in addresses and not any(low <= a < high for a in addresses): continue
        body = [r for r in instructions if low <= r['address'] < high]
        _, _, count, expected = next(f for f in FRAGMENTS if f[0] == start and f[1] == page)
        if len(body) != count or fingerprint(body) != expected:
            raise ValueError(f'Mac match-selection-support contract changed or missing at {start:x} page {page:x}')


def require_match_selection_support_index(db, selected=None):
    records = [json.loads(row[0]) for start, end, _, _ in CONTRACTS for row in db.execute(
        'SELECT body FROM instructions WHERE address>=? AND address<? ORDER BY address', (start, end))]
    if selected is None: require_match_selection_support_bodies(records, complete=True)
    else:
        for page in sorted({f[1] for f in FRAGMENTS}):
            require_match_selection_support_contract(page, [r for _, r in sorted(selected.items())], selected)
    require_match_selection_support_edges(db.execute("""SELECT i.address,e.value
        FROM instructions AS i,
          json_each(json_insert(json_extract(i.body,'$.flows'),'$[#]',json_extract(i.body,'$.fallthrough'))) AS e
        WHERE e.value>=? AND e.value<?""", (min(v[0] for v in CONTRACTS), max(v[1] for v in CONTRACTS))))


def match_selection_support_hook(entry):
    return f'if (try_modern_match_selection_support(s, 0x{entry:x})) return true;'


def require_match_selection_support_hooks(page, instructions, code):
    addresses = {r['address'] for r in instructions}
    for entry in MATCH_SELECTION_SUPPORT_ENTRIES:
        if entry >> 12 != page or entry not in addresses: continue
        label, hook = f'i_{entry:x}:', match_selection_support_hook(entry)
        fragment = code.split(label, 1)[-1].split(f'i_{entry + 4:x}:', 1)[0]
        steps = [token for token in (f's.step(0x{entry:x});', f'counter.step(0x{entry:x});') if token in fragment]
        if code.count(hook) != 1 or hook not in fragment or len(steps) != 1 or fragment.index(hook) > fragment.index(steps[0]):
            raise ValueError(f'Missing or misplaced match-selection-support hook at {entry:x}')
