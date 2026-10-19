"""Pinned complete original actor decision policy boundaries."""
import hashlib
import json

CONTRACTS = ((270488192, 270488392, 50, '3e7d0866b8ea45f46ed02b4b74e2e9bfe2f9d71955f2b5976b84b32465cb283c'), (271079248, 271079312, 16, 'd4c6ee7364c904e3df369921b005268d78bebba34143eaccb38da03991d5b562'), (271079312, 271079348, 9, 'd707581e3f1e2d850cb7929b0dbc7d49aca4b189ef5ff4c9ef0bfaba3694c77a'), (271079360, 271081156, 449, '4bed18aa8a083c106a5eac835fab2e8e0839fd75e5418e30eea20bb8b0bd2cd7'), (271081200, 271081664, 116, 'fdced12e8ad7876c470de4ae9cd1a554e435bcb33b2d0041ad1bab7be80638eb'))
FRAGMENTS = ((270488192, 66037, 50, '3e7d0866b8ea45f46ed02b4b74e2e9bfe2f9d71955f2b5976b84b32465cb283c'), (271079248, 66181, 16, 'd4c6ee7364c904e3df369921b005268d78bebba34143eaccb38da03991d5b562'), (271079312, 66181, 9, 'd707581e3f1e2d850cb7929b0dbc7d49aca4b189ef5ff4c9ef0bfaba3694c77a'), (271079360, 66181, 449, '4bed18aa8a083c106a5eac835fab2e8e0839fd75e5418e30eea20bb8b0bd2cd7'), (271081200, 66181, 68, '7472d43819f5c55e4d539f02e3a741334e1fd3572b8039db60576dd49c57785c'), (271081200, 66182, 48, 'b4208c994edba62006ba5cde28a697acaabb81d3b0dc775aee3861d69668c3b9'))
ACTOR_POLICY_ENTRIES = tuple(start for start, _, _, _ in CONTRACTS)


def fingerprint(body):
    canonical = [{key: r.get(key) for key in ('address', 'assembly', 'ops', 'flows', 'fallthrough')} for r in body]
    return hashlib.sha256(json.dumps(canonical, sort_keys=True, separators=(',', ':')).encode()).hexdigest()


def require_actor_policy_bodies(instructions, complete=False):
    for start, end, count, expected in CONTRACTS:
        body = [r for r in instructions if start <= r['address'] < end]
        if not body and not complete: continue
        if len(body) != count or fingerprint(body) != expected:
            raise ValueError(f'Mac actor-policy contract changed or missing at {start:x}')


def require_actor_policy_edges(edges):
    for source, target in edges:
        for start, end, _, _ in CONTRACTS:
            if start < target < end and not start <= source < end:
                raise ValueError(f'External actor-policy interior edge from {source:x} to {target:x}')


def require_actor_policy_contract(page, instructions, selected=None):
    addresses = {r['address'] for r in instructions} | set(selected or ())
    for start, end, _, _ in CONTRACTS:
        if not start >> 12 <= page <= (end - 1) >> 12: continue
        low, high = max(start, page << 12), min(end, (page + 1) << 12)
        if start not in addresses and not any(low <= a < high for a in addresses): continue
        body = [r for r in instructions if low <= r['address'] < high]
        _, _, count, expected = next(f for f in FRAGMENTS if f[0] == start and f[1] == page)
        if len(body) != count or fingerprint(body) != expected:
            raise ValueError(f'Mac actor-policy contract changed or missing at {start:x} page {page:x}')


def require_actor_policy_index(db, selected=None):
    records = [json.loads(row[0]) for start, end, _, _ in CONTRACTS for row in db.execute(
        'SELECT body FROM instructions WHERE address>=? AND address<? ORDER BY address', (start, end))]
    if selected is None: require_actor_policy_bodies(records, complete=True)
    else:
        for page in sorted({f[1] for f in FRAGMENTS}):
            require_actor_policy_contract(page, [r for _, r in sorted(selected.items())], selected)
    require_actor_policy_edges(db.execute("""SELECT i.address,e.value
        FROM instructions AS i,
          json_each(json_insert(json_extract(i.body,'$.flows'),'$[#]',json_extract(i.body,'$.fallthrough'))) AS e
        WHERE e.value>=? AND e.value<?""", (min(v[0] for v in CONTRACTS), max(v[1] for v in CONTRACTS))))


def actor_policy_hook(entry):
    return f'if (try_modern_actor_policy(s, 0x{entry:x})) return true;'


def require_actor_policy_hooks(page, instructions, code):
    addresses = {r['address'] for r in instructions}
    for entry in ACTOR_POLICY_ENTRIES:
        if entry >> 12 != page or entry not in addresses: continue
        label, hook = f'i_{entry:x}:', actor_policy_hook(entry)
        fragment = code.split(label, 1)[-1].split(f'i_{entry + 4:x}:', 1)[0]
        steps = [token for token in (f's.step(0x{entry:x});', f'counter.step(0x{entry:x});') if token in fragment]
        if code.count(hook) != 1 or hook not in fragment or len(steps) != 1 or fragment.index(hook) > fragment.index(steps[0]):
            raise ValueError(f'Missing or misplaced actor-policy hook at {entry:x}')
