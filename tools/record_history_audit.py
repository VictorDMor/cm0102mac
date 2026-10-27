"""Record-history insertion and cached-group traversal boundaries."""
import hashlib
import json

CONTRACTS = (
    (0x1073e350, 0x1073e4a8, 86, 'fa2eb26667b7bbbd53beeff6fef7059e698fae8a491284f7bc2e1296506a1872'),
    (0x1073e4b0, 0x1073e984, 307, '24bc7910af5ad4e6e04444a3b94c25a9e88b308dd7c65829ccfee482b8510353'),
)
FRAGMENTS = ()
RECORD_HISTORY_ENTRIES = tuple(start for start, _, _, _ in CONTRACTS)


def fingerprint(body):
    canonical = [{key: r.get(key) for key in ('address', 'assembly', 'ops', 'flows', 'fallthrough')} for r in body]
    return hashlib.sha256(json.dumps(canonical, sort_keys=True, separators=(',', ':')).encode()).hexdigest()


def require_record_history_bodies(instructions, complete=False):
    for start, end, count, expected in CONTRACTS:
        body = [r for r in instructions if start <= r['address'] < end]
        if not body and not complete: continue
        if len(body) != count or fingerprint(body) != expected:
            raise ValueError(f'Mac record-history contract changed or missing at {start:x}')


def require_record_history_edges(edges):
    for source, target in edges:
        for start, end, _, _ in CONTRACTS:
            if start < target < end and not start <= source < end:
                raise ValueError(f'External record-history interior edge from {source:x} to {target:x}')


def require_record_history_contract(page, instructions, selected=None):
    addresses = {r['address'] for r in instructions} | set(selected or ())
    for start, end, count, expected in CONTRACTS:
        if not start >> 12 <= page <= (end - 1) >> 12: continue
        low, high = max(start, page << 12), min(end, (page + 1) << 12)
        if not any(low <= a < high for a in addresses): continue
        body = [r for r in instructions if low <= r['address'] < high]
        fragment = next((f for f in FRAGMENTS if f[0] == start and f[1] == page), None)
        if fragment: count, expected = fragment[2:]
        if len(body) != count or fingerprint(body) != expected:
            raise ValueError(f'Mac record-history contract changed or missing at {start:x} page {page:x}')


def require_record_history_index(db, selected=None):
    records = [json.loads(row[0]) for start, end, _, _ in CONTRACTS for row in db.execute(
        'SELECT body FROM instructions WHERE address>=? AND address<? ORDER BY address', (start, end))]
    if selected is None: require_record_history_bodies(records, complete=True)
    else:
        for page in tuple(sorted({page for start, end, _, _ in CONTRACTS for page in range(start >> 12, ((end - 1) >> 12) + 1)})):
            require_record_history_contract(page, [r for _, r in sorted(selected.items())], selected)
    require_record_history_edges(db.execute("""SELECT i.address,e.value
        FROM instructions AS i,
          json_each(json_insert(json_extract(i.body,'$.flows'),'$[#]',json_extract(i.body,'$.fallthrough'))) AS e
        WHERE e.value>=? AND e.value<?""", (CONTRACTS[0][0], CONTRACTS[-1][1])))


def record_history_hook(entry):
    return f'if (try_modern_record_history(s, 0x{entry:x})) return true;'


def require_record_history_hooks(page, instructions, code, taken_over=()):
    addresses = {r['address'] for r in instructions}
    for entry in RECORD_HISTORY_ENTRIES:
        if entry >> 12 != page or entry not in addresses or entry in taken_over: continue
        label, hook = f'i_{entry:x}:', record_history_hook(entry)
        fragment = code.split(label, 1)[-1].split(f'i_{entry + 4:x}:', 1)[0]
        steps = [token for token in (f's.step(0x{entry:x});', f'counter.step(0x{entry:x});') if token in fragment]
        if code.count(hook) != 1 or hook not in fragment or len(steps) != 1 or fragment.index(hook) > fragment.index(steps[0]):
            raise ValueError(f'Missing or misplaced record-history hook at {entry:x}')
