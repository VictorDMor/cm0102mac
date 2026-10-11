"""Complete pinned Mac clock, calendar and daylight runtime."""
import hashlib
import json

CONTRACTS = (
    (0x1077f6d0, 0x1077f6d8, 2, 'c08bd868583cc19a2eb098e924ed9a61d2c60ce985afbd20caf94c7a4488e8a5'),
    (0x1077f6e0, 0x1077f754, 29, '7470642964c75709c692ec058c3e3ad21dc00913a60df9511f64686c4fd3f6ee'),
    (0x1077f760, 0x1077f8b8, 86, '635c07671a24d37e679357c2c000749931619ce0455e12fd2c4a52578d87eab5'),
    (0x1077f8c0, 0x1077f8f4, 13, 'a70545ff29a422a30a08139b07eb9b660674c6c24847e6d6c5896a7bf6fe56f6'),
    (0x1077f900, 0x1077f938, 14, 'f7b6ec8ccb77019e9dab4641488801b3aa66ebb1c422f919a36b93447eee33c8'),
    (0x1077f940, 0x1077f994, 21, '502fb1ba8b5ae5c2f9997494b87e077a0f7a457c32776b26aae48778c15e675d'),
    (0x1077f9a0, 0x1077f9d4, 13, '8f97d91bbc7a6a0254c67130476ebf1a4630a15362fbbd062d40defe2dc23fb9'),
    (0x1077f9e0, 0x1077fa48, 26, '18617496cd991b9de25bc0378e8b1794362d546bb7bda5232934b6ff40a39746'),
)
TIME_RUNTIME_ENTRIES = tuple(start for start, _, _, _ in CONTRACTS)


def fingerprint(body):
    canonical = [{key: r.get(key) for key in ('address', 'assembly', 'ops', 'flows', 'fallthrough')} for r in body]
    return hashlib.sha256(json.dumps(canonical, sort_keys=True, separators=(',', ':')).encode()).hexdigest()


def require_time_runtime_bodies(instructions, complete=False):
    for start, end, count, expected in CONTRACTS:
        body = [r for r in instructions if start <= r['address'] < end]
        if not body:
            if complete: raise ValueError(f'Missing complete Mac time-runtime body at {start:x}')
            continue
        if len(body) != count or fingerprint(body) != expected:
            raise ValueError(f'Mac time-runtime contract changed at {start:x}')


def require_time_runtime_edges(edges):
    for source, target in edges:
        for start, end, _, _ in CONTRACTS:
            if start < target < end and not start <= source < end:
                raise ValueError(f'External time-runtime interior edge from {source:x} to {target:x}')


def require_time_runtime_contract(page, instructions, selected=None):
    if page != 0x1077f: return
    addresses = {r['address'] for r in instructions} | set(selected or ())
    for start, end, count, expected in CONTRACTS:
        if start not in addresses: continue
        body = [r for r in instructions if start <= r['address'] < end]
        if len(body) != count or fingerprint(body) != expected:
            raise ValueError(f'Mac time-runtime contract changed at {start:x}')


def require_time_runtime_index(db, selected=None):
    records = [json.loads(row[0]) for start, end, _, _ in CONTRACTS for row in db.execute(
        'SELECT body FROM instructions WHERE address>=? AND address<? ORDER BY address', (start, end))]
    if selected is None: require_time_runtime_bodies(records, complete=True)
    else: require_time_runtime_contract(0x1077f, [r for _, r in sorted(selected.items())], selected)
    require_time_runtime_edges(db.execute("""SELECT i.address,e.value
        FROM instructions AS i,
          json_each(json_insert(json_extract(i.body,'$.flows'),'$[#]',json_extract(i.body,'$.fallthrough'))) AS e
        WHERE e.value>=? AND e.value<?""", (CONTRACTS[0][0], CONTRACTS[-1][1])))


def time_runtime_hook(entry):
    return f'if (try_modern_time_runtime(s, 0x{entry:x})) return true;'


def require_time_runtime_hooks(page, instructions, code):
    addresses = {r['address'] for r in instructions}
    for entry in TIME_RUNTIME_ENTRIES:
        if entry >> 12 != page or entry not in addresses: continue
        label, hook, step = f'i_{entry:x}:', time_runtime_hook(entry), f's.step(0x{entry:x});'
        fragment = code.split(label, 1)[-1].split(f'i_{entry + 4:x}:', 1)[0]
        if code.count(hook) != 1 or hook not in fragment or step not in fragment or fragment.index(hook) > fragment.index(step):
            raise ValueError(f'Missing or misplaced time-runtime hook at {entry:x}')
