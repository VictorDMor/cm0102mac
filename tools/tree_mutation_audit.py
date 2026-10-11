"""Descriptor-tree insertion and postorder cleanup boundaries."""
import hashlib
import json

CONTRACTS = (
    (0x1074df40, 0x1074df5c, 7, '6b17d64817d46902a10f05d960e158dea98a784510fa612a30de80fed28ab0ec'),
    (0x1074e030, 0x1074e148, 70, '45446bff05f78f8c1b86c837ff02143b24d9c135f1db0265444497818720c8c6'),
    (0x1074ea30, 0x1074ed70, 208, 'ff2a32c6a790558435fd1d19729cd0b73aa7ca283c01c3146f6bcd7b26fc350d'),
)
TREE_MUTATION_ENTRIES = tuple(start for start, _, _, _ in CONTRACTS)


def fingerprint(body):
    canonical = [{key: r.get(key) for key in ('address', 'assembly', 'ops', 'flows', 'fallthrough')} for r in body]
    return hashlib.sha256(json.dumps(canonical, sort_keys=True, separators=(',', ':')).encode()).hexdigest()


def require_tree_mutation_bodies(instructions, complete=False):
    for start, end, count, expected in CONTRACTS:
        body = [r for r in instructions if start <= r['address'] < end]
        if not body and not complete: continue
        if len(body) != count or fingerprint(body) != expected:
            raise ValueError(f'Mac tree-mutation contract changed or missing at {start:x}')


def require_tree_mutation_edges(edges):
    for source, target in edges:
        for start, end, _, _ in CONTRACTS:
            if start < target < end and not start <= source < end:
                raise ValueError(f'External tree-mutation interior edge from {source:x} to {target:x}')


def require_tree_mutation_contract(page, instructions, selected=None):
    addresses = {r['address'] for r in instructions} | set(selected or ())
    for start, end, count, expected in CONTRACTS:
        if start >> 12 != page or not any(start <= a < end for a in addresses): continue
        body = [r for r in instructions if start <= r['address'] < end]
        if len(body) != count or fingerprint(body) != expected:
            raise ValueError(f'Mac tree-mutation contract changed or missing at {start:x}')


def require_tree_mutation_index(db, selected=None):
    records = [json.loads(row[0]) for start, end, _, _ in CONTRACTS for row in db.execute(
        'SELECT body FROM instructions WHERE address>=? AND address<? ORDER BY address', (start, end))]
    if selected is None: require_tree_mutation_bodies(records, complete=True)
    else:
        for page in tuple(sorted({start >> 12 for start, _, _, _ in CONTRACTS})):
            require_tree_mutation_contract(page, [r for _, r in sorted(selected.items())], selected)
    require_tree_mutation_edges(db.execute("""SELECT i.address,e.value
        FROM instructions AS i,
          json_each(json_insert(json_extract(i.body,'$.flows'),'$[#]',json_extract(i.body,'$.fallthrough'))) AS e
        WHERE e.value>=? AND e.value<?""", (CONTRACTS[0][0], CONTRACTS[-1][1])))


def tree_mutation_hook(entry):
    return f'if (try_modern_tree_mutation(s, 0x{entry:x})) return true;'


def require_tree_mutation_hooks(page, instructions, code):
    addresses = {r['address'] for r in instructions}
    for entry in TREE_MUTATION_ENTRIES:
        if entry >> 12 != page or entry not in addresses: continue
        label, hook, step = f'i_{entry:x}:', tree_mutation_hook(entry), f's.step(0x{entry:x});'
        fragment = code.split(label, 1)[-1].split(f'i_{entry + 4:x}:', 1)[0]
        if code.count(hook) != 1 or hook not in fragment or step not in fragment or fragment.index(hook) > fragment.index(step):
            raise ValueError(f'Missing or misplaced tree-mutation hook at {entry:x}')
