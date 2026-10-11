"""Current-record queries and argument-publication boundaries."""
import hashlib
import json

CONTRACTS = (
    (0x1073e990, 0x1073e9b0, 8, 'c9354e753ad5384aa16f13a82d9785889419b57c7eeb2ba5db74b68b8e62abc8'),
    (0x1073e9b0, 0x1073ea14, 25, 'c53cc349f72035738ecfe04a3c7fff6a62160411927db4737f7246a8a883dc33'),
    (0x1073ea20, 0x1073eaf4, 53, '379dfce53b881afee450b8034ae64861a9dde6e465ba56ba4faa6a7517faab11'),
    (0x1073eb00, 0x1073ebfc, 63, '0cc74cf9b3730893857f183b9325806dde8800a2062c0164112a80190a2378f7'),
    (0x1073ec00, 0x1073ecc8, 50, '9370ea978adb7a3aa42b6d3c7a3811256d833bf6208d986f0a630066b1b609aa'),
    (0x1073ecd0, 0x1073eda8, 54, '08527aee88cafef55374876b207d4d60623228413d32eb87a6c7b6cc797cd3fc'),
)
FRAGMENTS = ()
RECORD_QUERY_ENTRIES = tuple(start for start, _, _, _ in CONTRACTS)


def fingerprint(body):
    canonical = [{key: r.get(key) for key in ('address', 'assembly', 'ops', 'flows', 'fallthrough')} for r in body]
    return hashlib.sha256(json.dumps(canonical, sort_keys=True, separators=(',', ':')).encode()).hexdigest()


def require_record_query_bodies(instructions, complete=False):
    for start, end, count, expected in CONTRACTS:
        body = [r for r in instructions if start <= r['address'] < end]
        if not body and not complete: continue
        if len(body) != count or fingerprint(body) != expected:
            raise ValueError(f'Mac record-query contract changed or missing at {start:x}')


def require_record_query_edges(edges):
    for source, target in edges:
        for start, end, _, _ in CONTRACTS:
            if start < target < end and not start <= source < end:
                raise ValueError(f'External record-query interior edge from {source:x} to {target:x}')


def require_record_query_contract(page, instructions, selected=None):
    addresses = {r['address'] for r in instructions} | set(selected or ())
    for start, end, count, expected in CONTRACTS:
        if not start >> 12 <= page <= (end - 1) >> 12: continue
        low, high = max(start, page << 12), min(end, (page + 1) << 12)
        if not any(low <= a < high for a in addresses): continue
        body = [r for r in instructions if low <= r['address'] < high]
        fragment = next((f for f in FRAGMENTS if f[0] == start and f[1] == page), None)
        if fragment: count, expected = fragment[2:]
        if len(body) != count or fingerprint(body) != expected:
            raise ValueError(f'Mac record-query contract changed or missing at {start:x} page {page:x}')


def require_record_query_index(db, selected=None):
    records = [json.loads(row[0]) for start, end, _, _ in CONTRACTS for row in db.execute(
        'SELECT body FROM instructions WHERE address>=? AND address<? ORDER BY address', (start, end))]
    if selected is None: require_record_query_bodies(records, complete=True)
    else:
        for page in tuple(sorted({page for start, end, _, _ in CONTRACTS for page in range(start >> 12, ((end - 1) >> 12) + 1)})):
            require_record_query_contract(page, [r for _, r in sorted(selected.items())], selected)
    require_record_query_edges(db.execute("""SELECT i.address,e.value
        FROM instructions AS i,
          json_each(json_insert(json_extract(i.body,'$.flows'),'$[#]',json_extract(i.body,'$.fallthrough'))) AS e
        WHERE e.value>=? AND e.value<?""", (CONTRACTS[0][0], CONTRACTS[-1][1])))


def record_query_hook(entry):
    return f'if (try_modern_record_query(s, 0x{entry:x})) return true;'


def require_record_query_hooks(page, instructions, code):
    addresses = {r['address'] for r in instructions}
    for entry in RECORD_QUERY_ENTRIES:
        if entry >> 12 != page or entry not in addresses: continue
        label, hook = f'i_{entry:x}:', record_query_hook(entry)
        fragment = code.split(label, 1)[-1].split(f'i_{entry + 4:x}:', 1)[0]
        steps = [token for token in (f's.step(0x{entry:x});', f'counter.step(0x{entry:x});') if token in fragment]
        if code.count(hook) != 1 or hook not in fragment or len(steps) != 1 or fragment.index(hook) > fragment.index(steps[0]):
            raise ValueError(f'Missing or misplaced record-query hook at {entry:x}')
