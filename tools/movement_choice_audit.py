"""Pinned complete movement choice boundaries."""
import hashlib
import json

CONTRACTS = (
    (0x101d8260, 0x101d8d60, 704, 'b46f65b7fac16ac4e8738e361430af8a3abbae5d231704066f41bc5b37f4d06d'),
    (0x101d8d60, 0x101d8d68, 2, 'f71bea85961518f426367933724f34f9dc96c7ecdfd1a6e62ac632ae846b0b49'),
    (0x101ee9c0, 0x101ee9e8, 10, '8c5d178b8bb564ed42a8b547149c5bbeb41c7308b183a3d530037878682d8d52'),
    (0x101eea30, 0x101eeab4, 33, '50b964dbba17de8fcf7b9c0685db1d9de3ab5f1e76455edf68d767c56b649a03'),
)
FRAGMENTS = (
    (0x101d8260, 0x101d8, 704, 'b46f65b7fac16ac4e8738e361430af8a3abbae5d231704066f41bc5b37f4d06d'),
    (0x101d8d60, 0x101d8, 2, 'f71bea85961518f426367933724f34f9dc96c7ecdfd1a6e62ac632ae846b0b49'),
    (0x101ee9c0, 0x101ee, 10, '8c5d178b8bb564ed42a8b547149c5bbeb41c7308b183a3d530037878682d8d52'),
    (0x101eea30, 0x101ee, 33, '50b964dbba17de8fcf7b9c0685db1d9de3ab5f1e76455edf68d767c56b649a03'),
)
MOVEMENT_CHOICE_ENTRIES = tuple(start for start, _, _, _ in CONTRACTS)


def fingerprint(body):
    canonical = [{key: r.get(key) for key in ('address', 'assembly', 'ops', 'flows', 'fallthrough')} for r in body]
    return hashlib.sha256(json.dumps(canonical, sort_keys=True, separators=(',', ':')).encode()).hexdigest()


def require_movement_choice_bodies(instructions, complete=False):
    for start, end, count, expected in CONTRACTS:
        body = [r for r in instructions if start <= r['address'] < end]
        if not body and not complete: continue
        if len(body) != count or fingerprint(body) != expected:
            raise ValueError(f'Mac movement-choice contract changed or missing at {start:x}')


def require_movement_choice_edges(edges):
    for source, target in edges:
        for start, end, _, _ in CONTRACTS:
            if start < target < end and not start <= source < end:
                raise ValueError(f'External movement-choice interior edge from {source:x} to {target:x}')


def require_movement_choice_contract(page, instructions, selected=None):
    addresses = {r['address'] for r in instructions} | set(selected or ())
    for start, end, _, _ in CONTRACTS:
        if not start >> 12 <= page <= (end - 1) >> 12: continue
        low, high = max(start, page << 12), min(end, (page + 1) << 12)
        if start not in addresses and not any(low <= a < high for a in addresses): continue
        body = [r for r in instructions if low <= r['address'] < high]
        _, _, count, expected = next(f for f in FRAGMENTS if f[0] == start and f[1] == page)
        if len(body) != count or fingerprint(body) != expected:
            raise ValueError(f'Mac movement-choice contract changed or missing at {start:x} page {page:x}')


def require_movement_choice_index(db, selected=None):
    records = [json.loads(row[0]) for start, end, _, _ in CONTRACTS for row in db.execute(
        'SELECT body FROM instructions WHERE address>=? AND address<? ORDER BY address', (start, end))]
    if selected is None: require_movement_choice_bodies(records, complete=True)
    else:
        for page in sorted({f[1] for f in FRAGMENTS}):
            require_movement_choice_contract(page, [r for _, r in sorted(selected.items())], selected)
    require_movement_choice_edges(db.execute("""SELECT i.address,e.value
        FROM instructions AS i,
          json_each(json_insert(json_extract(i.body,'$.flows'),'$[#]',json_extract(i.body,'$.fallthrough'))) AS e
        WHERE e.value>=? AND e.value<?""", (CONTRACTS[0][0], CONTRACTS[-1][1])))


def movement_choice_hook(entry):
    return f'if (try_modern_movement_choice(s, 0x{entry:x})) return true;'


def require_movement_choice_hooks(page, instructions, code):
    addresses = {r['address'] for r in instructions}
    for entry in MOVEMENT_CHOICE_ENTRIES:
        if entry >> 12 != page or entry not in addresses: continue
        label, hook = f'i_{entry:x}:', movement_choice_hook(entry)
        fragment = code.split(label, 1)[-1].split(f'i_{entry + 4:x}:', 1)[0]
        steps = [token for token in (f's.step(0x{entry:x});', f'counter.step(0x{entry:x});') if token in fragment]
        if code.count(hook) != 1 or hook not in fragment or len(steps) != 1 or fragment.index(hook) > fragment.index(steps[0]):
            raise ValueError(f'Missing or misplaced movement-choice hook at {entry:x}')
