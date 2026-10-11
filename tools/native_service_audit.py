"""Complete framed AppleEvent and pointer-service facades."""
import hashlib
import json

CONTRACTS = (
    (0x10770a80, 0x10770aa4, 9, 'a475f3fc2cf643639103d80c2e64d2a080009d768ee2c1395ee40b6c24364f53'),
    (0x10778d50, 0x10778d74, 9, '0435d27f27f18b8ae917b17f81b1ac625d2653baad43357e426c55f5475f1cf9'),
    (0x10778d80, 0x10778da4, 9, '8e1bf4a84ed975fa5aec82dcde0d089b30b76dd59451a01f925c1fb1493e02b5'),
)
NATIVE_SERVICE_ENTRIES = tuple(start for start, _, _, _ in CONTRACTS)


def fingerprint(body):
    canonical = [{key: r.get(key) for key in ('address', 'assembly', 'ops', 'flows', 'fallthrough')} for r in body]
    return hashlib.sha256(json.dumps(canonical, sort_keys=True, separators=(',', ':')).encode()).hexdigest()


def require_native_service_bodies(instructions, complete=False):
    for start, end, count, expected in CONTRACTS:
        body = [r for r in instructions if start <= r['address'] < end]
        if not body and not complete: continue
        if len(body) != count or fingerprint(body) != expected:
            raise ValueError(f'Mac native-service contract changed or missing at {start:x}')


def require_native_service_edges(edges):
    for source, target in edges:
        for start, end, _, _ in CONTRACTS:
            if start < target < end and not start <= source < end:
                raise ValueError(f'External native-service interior edge from {source:x} to {target:x}')


def require_native_service_contract(page, instructions, selected=None):
    addresses = {r['address'] for r in instructions} | set(selected or ())
    for start, end, count, expected in CONTRACTS:
        if start >> 12 != page or not any(start <= a < end for a in addresses): continue
        body = [r for r in instructions if start <= r['address'] < end]
        if len(body) != count or fingerprint(body) != expected:
            raise ValueError(f'Mac native-service contract changed or missing at {start:x}')


def require_native_service_index(db, selected=None):
    records = [json.loads(row[0]) for start, end, _, _ in CONTRACTS for row in db.execute(
        'SELECT body FROM instructions WHERE address>=? AND address<? ORDER BY address', (start, end))]
    if selected is None: require_native_service_bodies(records, complete=True)
    else:
        for page in (0x10770, 0x10778):
            require_native_service_contract(page, [r for _, r in sorted(selected.items())], selected)
    require_native_service_edges(db.execute("""SELECT i.address,e.value
        FROM instructions AS i,
          json_each(json_insert(json_extract(i.body,'$.flows'),'$[#]',json_extract(i.body,'$.fallthrough'))) AS e
        WHERE e.value>=? AND e.value<?""", (CONTRACTS[0][0], CONTRACTS[-1][1])))


def native_service_hook(entry):
    return f'if (try_modern_native_service(s, 0x{entry:x})) return true;'


def require_native_service_hooks(page, instructions, code):
    addresses = {r['address'] for r in instructions}
    for entry in NATIVE_SERVICE_ENTRIES:
        if entry >> 12 != page or entry not in addresses: continue
        label, hook, step = f'i_{entry:x}:', native_service_hook(entry), f's.step(0x{entry:x});'
        fragment = code.split(label, 1)[-1].split(f'i_{entry + 4:x}:', 1)[0]
        if code.count(hook) != 1 or hook not in fragment or step not in fragment or fragment.index(hook) > fragment.index(step):
            raise ValueError(f'Missing or misplaced native-service hook at {entry:x}')
