"""Pinned complete original actor periodic balance and counter boundaries."""
import hashlib
import json

CONTRACTS = ((270760784, 270767036, 1563, '6cea20bfac393fa0de1e31e2c756cca92ae1eda038e970f326d0c0c2a936f587'), (271081168, 271081184, 4, '00cba90b5a955a2fe7160e1e1dd7c411d72edbe3195e4de1746b2b4296582870'), (271081184, 271081192, 2, '18baec14ffea770eafc617375b4c77496ceb38eb16e818d6a3ae50b3d4b56430'))
FRAGMENTS = ((270760784, 66103, 300, 'cddeaaad725775869ee3251ff61fb7cf6f57e763fafc7f8109a8e8ccdf4219d9'), (270760784, 66104, 1024, 'aab85cd1ade6f89fc455727227a9f5de9f09b634352387bd29651fbf74072e15'), (270760784, 66105, 239, '8b439098197a5016902b69c22db85a53d9070023caf4d3967370b292e1f2502d'), (271081168, 66181, 4, '00cba90b5a955a2fe7160e1e1dd7c411d72edbe3195e4de1746b2b4296582870'), (271081184, 66181, 2, '18baec14ffea770eafc617375b4c77496ceb38eb16e818d6a3ae50b3d4b56430'))
ACTOR_EFFECTS_ENTRIES = tuple(start for start, _, _, _ in CONTRACTS)


def fingerprint(body):
    canonical = [{key: r.get(key) for key in ('address', 'assembly', 'ops', 'flows', 'fallthrough')} for r in body]
    return hashlib.sha256(json.dumps(canonical, sort_keys=True, separators=(',', ':')).encode()).hexdigest()


def require_actor_effects_bodies(instructions, complete=False):
    for start, end, count, expected in CONTRACTS:
        body = [r for r in instructions if start <= r['address'] < end]
        if not body and not complete: continue
        if len(body) != count or fingerprint(body) != expected:
            raise ValueError(f'Mac actor-effects contract changed or missing at {start:x}')


def require_actor_effects_edges(edges):
    for source, target in edges:
        for start, end, _, _ in CONTRACTS:
            if start < target < end and not start <= source < end:
                raise ValueError(f'External actor-effects interior edge from {source:x} to {target:x}')


def require_actor_effects_contract(page, instructions, selected=None):
    addresses = {r['address'] for r in instructions} | set(selected or ())
    for start, end, _, _ in CONTRACTS:
        if not start >> 12 <= page <= (end - 1) >> 12: continue
        low, high = max(start, page << 12), min(end, (page + 1) << 12)
        if start not in addresses and not any(low <= a < high for a in addresses): continue
        body = [r for r in instructions if low <= r['address'] < high]
        _, _, count, expected = next(f for f in FRAGMENTS if f[0] == start and f[1] == page)
        if len(body) != count or fingerprint(body) != expected:
            raise ValueError(f'Mac actor-effects contract changed or missing at {start:x} page {page:x}')


def require_actor_effects_index(db, selected=None):
    records = [json.loads(row[0]) for start, end, _, _ in CONTRACTS for row in db.execute(
        'SELECT body FROM instructions WHERE address>=? AND address<? ORDER BY address', (start, end))]
    if selected is None: require_actor_effects_bodies(records, complete=True)
    else:
        for page in sorted({f[1] for f in FRAGMENTS}):
            require_actor_effects_contract(page, [r for _, r in sorted(selected.items())], selected)
    require_actor_effects_edges(db.execute("""SELECT i.address,e.value
        FROM instructions AS i,
          json_each(json_insert(json_extract(i.body,'$.flows'),'$[#]',json_extract(i.body,'$.fallthrough'))) AS e
        WHERE e.value>=? AND e.value<?""", (min(v[0] for v in CONTRACTS), max(v[1] for v in CONTRACTS))))


def actor_effects_hook(entry):
    return f'if (try_modern_actor_effects(s, 0x{entry:x})) return true;'


def require_actor_effects_hooks(page, instructions, code, taken_over=()):
    addresses = {r['address'] for r in instructions}
    for entry in ACTOR_EFFECTS_ENTRIES:
        if entry >> 12 != page or entry not in addresses or entry in taken_over: continue
        label, hook = f'i_{entry:x}:', actor_effects_hook(entry)
        fragment = code.split(label, 1)[-1].split(f'i_{entry + 4:x}:', 1)[0]
        steps = [token for token in (f's.step(0x{entry:x});', f'counter.step(0x{entry:x});') if token in fragment]
        if code.count(hook) != 1 or hook not in fragment or len(steps) != 1 or fragment.index(hook) > fragment.index(steps[0]):
            raise ValueError(f'Missing or misplaced actor-effects hook at {entry:x}')
