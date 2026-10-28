"""Pinned complete original target facing, eligibility and corridor boundaries."""
import hashlib
import json

CONTRACTS = ((270814304, 270815312, 252, '9feea0e9a266bb736c8d2ed3622c8f58c20541f8f1c349fb690f8c34d7cf5d5b'), (270461632, 270462116, 121, '3a7b0ea843c6b619c3e842e8bcd7958e5c7b9f563c3ce33cf119d38757934c96'), (270462640, 270463936, 324, 'f851249b792db193ec54a3ca35125949b3d987e9e78396d1ab29d020f2af4b24'), (270469584, 270469828, 61, '4d2efbf7a4a66e73405a8d4ceb47a350239ef2ad69fd983918856a515a33f8a4'))
FRAGMENTS = ((270814304, 66116, 232, '1a4a11afc275ce43e593608c1e579e30ac5adf57daf8d8bc11b0174fc2de8274'), (270814304, 66117, 20, '88195a2d9643a97e43d54c703130b2c2db52bf5cee42aee38567e346e4843e47'), (270461632, 66030, 121, '3a7b0ea843c6b619c3e842e8bcd7958e5c7b9f563c3ce33cf119d38757934c96'), (270462640, 66030, 84, '8a5549660a34beefb921bb977ca1310384ef5503f7c3b2c445135c0c4b8f8572'), (270462640, 66031, 240, 'fff15c1eb343bcd63a227a73af54a752f65a29834ed1cced594da6f75c886137'), (270469584, 66032, 61, '4d2efbf7a4a66e73405a8d4ceb47a350239ef2ad69fd983918856a515a33f8a4'))
TARGET_ACCEPTANCE_ENTRIES = tuple(start for start, _, _, _ in CONTRACTS)


def fingerprint(body):
    canonical = [{key: r.get(key) for key in ('address', 'assembly', 'ops', 'flows', 'fallthrough')} for r in body]
    return hashlib.sha256(json.dumps(canonical, sort_keys=True, separators=(',', ':')).encode()).hexdigest()


def require_target_acceptance_bodies(instructions, complete=False):
    for start, end, count, expected in CONTRACTS:
        body = [r for r in instructions if start <= r['address'] < end]
        if not body and not complete: continue
        if len(body) != count or fingerprint(body) != expected:
            raise ValueError(f'Mac target-acceptance contract changed or missing at {start:x}')


def require_target_acceptance_edges(edges):
    for source, target in edges:
        for start, end, _, _ in CONTRACTS:
            if start < target < end and not start <= source < end:
                raise ValueError(f'External target-acceptance interior edge from {source:x} to {target:x}')


def require_target_acceptance_contract(page, instructions, selected=None):
    addresses = {r['address'] for r in instructions} | set(selected or ())
    for start, end, _, _ in CONTRACTS:
        if not start >> 12 <= page <= (end - 1) >> 12: continue
        low, high = max(start, page << 12), min(end, (page + 1) << 12)
        if start not in addresses and not any(low <= a < high for a in addresses): continue
        body = [r for r in instructions if low <= r['address'] < high]
        _, _, count, expected = next(f for f in FRAGMENTS if f[0] == start and f[1] == page)
        if len(body) != count or fingerprint(body) != expected:
            raise ValueError(f'Mac target-acceptance contract changed or missing at {start:x} page {page:x}')


def require_target_acceptance_index(db, selected=None):
    records = [json.loads(row[0]) for start, end, _, _ in CONTRACTS for row in db.execute(
        'SELECT body FROM instructions WHERE address>=? AND address<? ORDER BY address', (start, end))]
    if selected is None: require_target_acceptance_bodies(records, complete=True)
    else:
        for page in sorted({f[1] for f in FRAGMENTS}):
            require_target_acceptance_contract(page, [r for _, r in sorted(selected.items())], selected)
    require_target_acceptance_edges(db.execute("""SELECT i.address,e.value
        FROM instructions AS i,
          json_each(json_insert(json_extract(i.body,'$.flows'),'$[#]',json_extract(i.body,'$.fallthrough'))) AS e
        WHERE e.value>=? AND e.value<?""", (min(v[0] for v in CONTRACTS), max(v[1] for v in CONTRACTS))))


def target_acceptance_hook(entry):
    return f'if (try_modern_target_acceptance(s, 0x{entry:x})) return true;'


def require_target_acceptance_hooks(page, instructions, code, taken_over=()):
    addresses = {r['address'] for r in instructions}
    for entry in TARGET_ACCEPTANCE_ENTRIES:
        if entry >> 12 != page or entry not in addresses or entry in taken_over: continue
        label, hook = f'i_{entry:x}:', target_acceptance_hook(entry)
        fragment = code.split(label, 1)[-1].split(f'i_{entry + 4:x}:', 1)[0]
        steps = [token for token in (f's.step(0x{entry:x});', f'counter.step(0x{entry:x});') if token in fragment]
        if code.count(hook) != 1 or hook not in fragment or len(steps) != 1 or fragment.index(hook) > fragment.index(steps[0]):
            raise ValueError(f'Missing or misplaced target-acceptance hook at {entry:x}')
