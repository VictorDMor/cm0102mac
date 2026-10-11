"""Owned buffer, linked-list and descriptor-object lifetime boundaries."""
import hashlib
import json

CONTRACTS = (
    (0x10738b20, 0x10738b9c, 31, '5dd0cef3ad945f186baf35781eda3938de4eca589dbf5f2d15db8f24002c051d'),
    (0x10738ba0, 0x10738bf4, 21, '33c8158664065066acd738d7a97314a5d946a75e24951795f9f20529bf757da8'),
    (0x10738c00, 0x10738c18, 6, 'eba97f101d2fb53ed56cdc1cefe2b75d61a63e68d35a63bba9e2842dcad236f1'),
    (0x10738c20, 0x10738c94, 29, '625be6bc6372f8aaa3141a3bca6f315c465b8f2e49ed70d2cd0babb772dccd07'),
    (0x10738ca0, 0x10738d08, 25, '1b2b683af072ce940f2f7bacf7ad0d839ce373a50fe5cd751083e14888aac7f9'),
    (0x1073ae10, 0x1073aed4, 49, 'b2702d43aed9a18e50bc90774ffb5290b8d6455ce67a38cf31454d10d34d3d36'),
    (0x1073aee0, 0x1073af44, 25, '1b5071a5eb5ef2576764275b9a3ea88cbdaeede4ad6ef8028c03e851294c056b'),
    (0x1074dec0, 0x1074ded8, 6, 'c0570316ee9b1ea590fe1f71a8794a7d11b0d356818b129a071bc57ae0454379'),
    (0x1074dee0, 0x1074df30, 20, 'a9fee06e559a8f30375fec0145b22f6edc8642218df6911d36de07b34a5c2a45'),
    (0x1074dfc0, 0x1074dffc, 15, '3d4f60c987c3bf6977fc10ae53c57f923d6d6b8303bbcd58d3ec4f3b9fffb317'),
    (0x1074e000, 0x1074e030, 12, 'f57b14c28954b5cf7ad14c19916bc37e957245b3bd8013bb95e07c22bbb22cf4'),
)
UTILITY_OBJECT_ENTRIES = tuple(start for start, _, _, _ in CONTRACTS)


def fingerprint(body):
    canonical = [{key: r.get(key) for key in ('address', 'assembly', 'ops', 'flows', 'fallthrough')} for r in body]
    return hashlib.sha256(json.dumps(canonical, sort_keys=True, separators=(',', ':')).encode()).hexdigest()


def require_utility_object_bodies(instructions, complete=False):
    for start, end, count, expected in CONTRACTS:
        body = [r for r in instructions if start <= r['address'] < end]
        if not body and not complete: continue
        if len(body) != count or fingerprint(body) != expected:
            raise ValueError(f'Mac utility-object contract changed or missing at {start:x}')


def require_utility_object_edges(edges):
    for source, target in edges:
        for start, end, _, _ in CONTRACTS:
            if start < target < end and not start <= source < end:
                raise ValueError(f'External utility-object interior edge from {source:x} to {target:x}')


def require_utility_object_contract(page, instructions, selected=None):
    addresses = {r['address'] for r in instructions} | set(selected or ())
    for start, end, count, expected in CONTRACTS:
        if start >> 12 != page or not any(start <= a < end for a in addresses): continue
        body = [r for r in instructions if start <= r['address'] < end]
        if len(body) != count or fingerprint(body) != expected:
            raise ValueError(f'Mac utility-object contract changed or missing at {start:x}')


def require_utility_object_index(db, selected=None):
    records = [json.loads(row[0]) for start, end, _, _ in CONTRACTS for row in db.execute(
        'SELECT body FROM instructions WHERE address>=? AND address<? ORDER BY address', (start, end))]
    if selected is None: require_utility_object_bodies(records, complete=True)
    else:
        for page in tuple(sorted({start >> 12 for start, _, _, _ in CONTRACTS})):
            require_utility_object_contract(page, [r for _, r in sorted(selected.items())], selected)
    require_utility_object_edges(db.execute("""SELECT i.address,e.value
        FROM instructions AS i,
          json_each(json_insert(json_extract(i.body,'$.flows'),'$[#]',json_extract(i.body,'$.fallthrough'))) AS e
        WHERE e.value>=? AND e.value<?""", (CONTRACTS[0][0], CONTRACTS[-1][1])))


def utility_object_hook(entry):
    return f'if (try_modern_utility_object(s, 0x{entry:x})) return true;'


def require_utility_object_hooks(page, instructions, code):
    addresses = {r['address'] for r in instructions}
    for entry in UTILITY_OBJECT_ENTRIES:
        if entry >> 12 != page or entry not in addresses: continue
        label, hook, step = f'i_{entry:x}:', utility_object_hook(entry), f's.step(0x{entry:x});'
        fragment = code.split(label, 1)[-1].split(f'i_{entry + 4:x}:', 1)[0]
        if code.count(hook) != 1 or hook not in fragment or step not in fragment or fragment.index(hook) > fragment.index(step):
            raise ValueError(f'Missing or misplaced utility-object hook at {entry:x}')
