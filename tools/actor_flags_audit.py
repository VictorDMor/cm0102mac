"""Pinned complete original actor strategy flag query boundaries."""
import hashlib
import json

CONTRACTS = ((271082160, 271082272, 28, '822224bb6205d71991d4f33da4287c478360229b4af4e78b63509e5fe281bb7f'), (271082272, 271082384, 28, 'db80eb0a144b88de421f2b8b745b2b2387df1a8faa17e25913cf9c69cf20f987'), (271082384, 271082496, 28, 'f8c2f5d60b944f505662089aa7cf7b4a88cb8580b79bd6a468c6379fe8f0e2a8'), (271082496, 271082608, 28, 'b991cabfa936bf7bc62a5847b866d4b5a897698cbff9e3603da3d4569e54973d'), (271082608, 271082668, 15, '62ea3ccb8f13ddf0045dd6390438e7ecdfe929fc53b9b067c38e94e97181c0f4'), (271082672, 271082732, 15, '5b50ff3cb8b1655c511013499f8eb619358da591545c377827407a93d585b4d9'), (271082736, 271082908, 43, '0f561e87527b28fb9251e0f4b88c1b40c5368105f80d49454b0e7c2a997aa66f'))
FRAGMENTS = ((271082160, 66182, 28, '822224bb6205d71991d4f33da4287c478360229b4af4e78b63509e5fe281bb7f'), (271082272, 66182, 28, 'db80eb0a144b88de421f2b8b745b2b2387df1a8faa17e25913cf9c69cf20f987'), (271082384, 66182, 28, 'f8c2f5d60b944f505662089aa7cf7b4a88cb8580b79bd6a468c6379fe8f0e2a8'), (271082496, 66182, 28, 'b991cabfa936bf7bc62a5847b866d4b5a897698cbff9e3603da3d4569e54973d'), (271082608, 66182, 15, '62ea3ccb8f13ddf0045dd6390438e7ecdfe929fc53b9b067c38e94e97181c0f4'), (271082672, 66182, 15, '5b50ff3cb8b1655c511013499f8eb619358da591545c377827407a93d585b4d9'), (271082736, 66182, 43, '0f561e87527b28fb9251e0f4b88c1b40c5368105f80d49454b0e7c2a997aa66f'))
ACTOR_FLAGS_ENTRIES = tuple(start for start, _, _, _ in CONTRACTS)


def fingerprint(body):
    canonical = [{key: r.get(key) for key in ('address', 'assembly', 'ops', 'flows', 'fallthrough')} for r in body]
    return hashlib.sha256(json.dumps(canonical, sort_keys=True, separators=(',', ':')).encode()).hexdigest()


def require_actor_flags_bodies(instructions, complete=False):
    for start, end, count, expected in CONTRACTS:
        body = [r for r in instructions if start <= r['address'] < end]
        if not body and not complete: continue
        if len(body) != count or fingerprint(body) != expected:
            raise ValueError(f'Mac actor-flags contract changed or missing at {start:x}')


def require_actor_flags_edges(edges):
    for source, target in edges:
        for start, end, _, _ in CONTRACTS:
            if start < target < end and not start <= source < end:
                raise ValueError(f'External actor-flags interior edge from {source:x} to {target:x}')


def require_actor_flags_contract(page, instructions, selected=None):
    addresses = {r['address'] for r in instructions} | set(selected or ())
    for start, end, _, _ in CONTRACTS:
        if not start >> 12 <= page <= (end - 1) >> 12: continue
        low, high = max(start, page << 12), min(end, (page + 1) << 12)
        if start not in addresses and not any(low <= a < high for a in addresses): continue
        body = [r for r in instructions if low <= r['address'] < high]
        _, _, count, expected = next(f for f in FRAGMENTS if f[0] == start and f[1] == page)
        if len(body) != count or fingerprint(body) != expected:
            raise ValueError(f'Mac actor-flags contract changed or missing at {start:x} page {page:x}')


def require_actor_flags_index(db, selected=None):
    records = [json.loads(row[0]) for start, end, _, _ in CONTRACTS for row in db.execute(
        'SELECT body FROM instructions WHERE address>=? AND address<? ORDER BY address', (start, end))]
    if selected is None: require_actor_flags_bodies(records, complete=True)
    else:
        for page in sorted({f[1] for f in FRAGMENTS}):
            require_actor_flags_contract(page, [r for _, r in sorted(selected.items())], selected)
    require_actor_flags_edges(db.execute("""SELECT i.address,e.value
        FROM instructions AS i,
          json_each(json_insert(json_extract(i.body,'$.flows'),'$[#]',json_extract(i.body,'$.fallthrough'))) AS e
        WHERE e.value>=? AND e.value<?""", (min(v[0] for v in CONTRACTS), max(v[1] for v in CONTRACTS))))


def actor_flags_hook(entry):
    return f'if (try_modern_actor_flags(s, 0x{entry:x})) return true;'


def require_actor_flags_hooks(page, instructions, code, taken_over=()):
    addresses = {r['address'] for r in instructions}
    for entry in ACTOR_FLAGS_ENTRIES:
        if entry >> 12 != page or entry not in addresses or entry in taken_over: continue
        label, hook = f'i_{entry:x}:', actor_flags_hook(entry)
        fragment = code.split(label, 1)[-1].split(f'i_{entry + 4:x}:', 1)[0]
        steps = [token for token in (f's.step(0x{entry:x});', f'counter.step(0x{entry:x});') if token in fragment]
        if code.count(hook) != 1 or hook not in fragment or len(steps) != 1 or fragment.index(hook) > fragment.index(steps[0]):
            raise ValueError(f'Missing or misplaced actor-flags hook at {entry:x}')
