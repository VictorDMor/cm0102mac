"""Complete pinned Mac word utilities and comparator binary search."""
import hashlib
import json

CONTRACTS = (
    (0x10775540, 0x10775550, 4, '0f500638018273f5fd6d10f499518290a68a3687b321300079d11100995064a6'),
    (0x10775550, 0x10775560, 4, '447493b5c44ecbc8cb16adcdc2d5cd38d0c49a243e80d497a3efe3c5cd532b6b'),
    (0x10775560, 0x107755bc, 23, 'e115afb1026c1f9a383f0c46ec48f39b31b40d4b6b1c20e4258bd7b7a28393e0'),
    (0x10775600, 0x107756f0, 59, 'af1d8266f781790cda7244658ead2f0d29138fc287628157cd56cb3b28857fc9'),
)
STDLIB_CORE_ENTRIES = tuple(start for start, _, _, _ in CONTRACTS)


def fingerprint(body):
    canonical = [{key: r.get(key) for key in ('address', 'assembly', 'ops', 'flows', 'fallthrough')} for r in body]
    return hashlib.sha256(json.dumps(canonical, sort_keys=True, separators=(',', ':')).encode()).hexdigest()


def require_stdlib_core_bodies(instructions, complete=False):
    for start, end, count, expected in CONTRACTS:
        body = [r for r in instructions if start <= r['address'] < end]
        if not body:
            if complete: raise ValueError(f'Missing complete Mac stdlib body at {start:x}')
            continue
        if len(body) != count or fingerprint(body) != expected:
            raise ValueError(f'Mac stdlib contract changed at {start:x}')


def require_stdlib_core_edges(edges):
    for source, target in edges:
        for start, end, _, _ in CONTRACTS:
            if start < target < end and not start <= source < end:
                raise ValueError(f'External stdlib interior edge from {source:x} to {target:x}')


def require_stdlib_core_contract(page, instructions, selected=None):
    if page != 0x10775: return
    addresses = {r['address'] for r in instructions} | set(selected or ())
    for start, end, count, expected in CONTRACTS:
        if start not in addresses: continue
        body = [r for r in instructions if start <= r['address'] < end]
        if len(body) != count or fingerprint(body) != expected:
            raise ValueError(f'Mac stdlib contract changed at {start:x}')


def require_stdlib_core_index(db, selected=None):
    records = [json.loads(row[0]) for start, end, _, _ in CONTRACTS for row in db.execute(
        'SELECT body FROM instructions WHERE address>=? AND address<? ORDER BY address', (start, end))]
    if selected is None: require_stdlib_core_bodies(records, complete=True)
    else: require_stdlib_core_contract(0x10775, [r for _, r in sorted(selected.items())], selected)
    require_stdlib_core_edges(db.execute("""SELECT i.address,e.value
        FROM instructions AS i,
          json_each(json_insert(json_extract(i.body,'$.flows'),'$[#]',json_extract(i.body,'$.fallthrough'))) AS e
        WHERE e.value>=? AND e.value<?""", (CONTRACTS[0][0], CONTRACTS[-1][1])))


def stdlib_core_hook(entry):
    return f'if (try_modern_stdlib_core(s, 0x{entry:x})) return true;'


def require_stdlib_core_hooks(page, instructions, code):
    addresses = {r['address'] for r in instructions}
    for entry in STDLIB_CORE_ENTRIES:
        if entry >> 12 != page or entry not in addresses: continue
        label, hook, step = f'i_{entry:x}:', stdlib_core_hook(entry), f's.step(0x{entry:x});'
        fragment = code.split(label, 1)[-1].split(f'i_{entry + 4:x}:', 1)[0]
        if code.count(hook) != 1 or hook not in fragment or step not in fragment or fragment.index(hook) > fragment.index(step):
            raise ValueError(f'Missing or misplaced stdlib-core hook at {entry:x}')
