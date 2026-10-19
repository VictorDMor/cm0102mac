"""Pinned complete original actor action routing and selection boundaries."""
import hashlib
import json

CONTRACTS = ((271021536, 271024668, 783, '4e8217907f14f4ee33ad810c0f2b225a602eeeb25f698d08a07f143d96ab4118'), (271024672, 271062904, 9558, '1bf734ae373257f8a5547cb2f8c33fdd48f5098e640b9f295ee7e5b92c6aea7e'), (271062912, 271063280, 92, '38f11089d695d759efbe7ec4eb31d214705e5f2a62849342d0b6c12b06bfad33'), (271063280, 271063788, 127, 'f091127fdd9ba49ecb4a7317c9d7a45bf4bdae66a45fc88ff5f5b2ec13624645'), (271063792, 271070524, 1683, 'fe56ef285181d25d91b6c24abd90b89fca11c9479d86b123a95092edee1df73d'))
FRAGMENTS = ((271021536, 66167, 648, '57e7a811476d3cb97cdf5a8f91719bf1f6ba793a0cf5482170d5cab1ad17de02'), (271021536, 66168, 135, '7a4c726782078086655acc0a7d6ebe392be6559173f6e8813b9a637507dcc6f5'), (271024672, 66168, 888, 'c06a0b835bbdc93eae54ec5caf7514723892ccf4ce6578bfb146044a6bf64c5f'), (271024672, 66169, 1024, 'c37ed1c8cfd476a6fb27d81e60b1f634e3abc3a751182247d841dbd5b357f834'), (271024672, 66170, 1024, '66fe2b7ff19f0c46735d592024a340affaff7e131c67423ae242f3b3c1a20f76'), (271024672, 66171, 1024, 'e1d6771e8c230fe563c13c3a113acecce0a321aec8fc8e7dabab9f33c8418631'), (271024672, 66172, 1024, 'ff2423e3efdeae1aa1887a66d6dce2798127bf03c0b06857f4761ed74fdde06b'), (271024672, 66173, 1024, 'd698313276f47b17da6a81b79480ef0d5d2ae9069e3b65e797ad6cdf133dd1ff'), (271024672, 66174, 1024, '7261245e5dd0d114d48a97aeed344f95c543fe54af33380746bf52194471be82'), (271024672, 66175, 1024, '3606763b01836212c06bc838b64d3f45000fe967735f62acb86d14a66327e6ce'), (271024672, 66176, 1024, '4b175d06a924f30cd7db3efdc6ca447b4c5a649c193598d75e391350dff519ed'), (271024672, 66177, 478, 'b9fa7f5d77fb231326126e267e4aafb26a903431fc58711e9d5b10f74a4f8d24'), (271062912, 66177, 92, '38f11089d695d759efbe7ec4eb31d214705e5f2a62849342d0b6c12b06bfad33'), (271063280, 66177, 127, 'f091127fdd9ba49ecb4a7317c9d7a45bf4bdae66a45fc88ff5f5b2ec13624645'), (271063792, 66177, 324, '6459f5ddc1201eac9b6b1eef116eb486790ae45751da62a6cd5e81698b6f0aa3'), (271063792, 66178, 1024, 'dbf45f8c43397e8a7dd9412e1d91324ee4bf15daed4230b98039198888549c6d'), (271063792, 66179, 335, '832d9f328da23c37d95bf23d095d03a460eded8f77f1fe764a9d3956e02f6f0f'))
ACTOR_ACTION_ENTRIES = tuple(start for start, _, _, _ in CONTRACTS)


def fingerprint(body):
    canonical = [{key: r.get(key) for key in ('address', 'assembly', 'ops', 'flows', 'fallthrough')} for r in body]
    return hashlib.sha256(json.dumps(canonical, sort_keys=True, separators=(',', ':')).encode()).hexdigest()


def require_actor_action_bodies(instructions, complete=False):
    for start, end, count, expected in CONTRACTS:
        body = [r for r in instructions if start <= r['address'] < end]
        if not body and not complete: continue
        if len(body) != count or fingerprint(body) != expected:
            raise ValueError(f'Mac actor-action contract changed or missing at {start:x}')


def require_actor_action_edges(edges):
    for source, target in edges:
        for start, end, _, _ in CONTRACTS:
            if start < target < end and not start <= source < end:
                raise ValueError(f'External actor-action interior edge from {source:x} to {target:x}')


def require_actor_action_contract(page, instructions, selected=None):
    addresses = {r['address'] for r in instructions} | set(selected or ())
    for start, end, _, _ in CONTRACTS:
        if not start >> 12 <= page <= (end - 1) >> 12: continue
        low, high = max(start, page << 12), min(end, (page + 1) << 12)
        if start not in addresses and not any(low <= a < high for a in addresses): continue
        body = [r for r in instructions if low <= r['address'] < high]
        _, _, count, expected = next(f for f in FRAGMENTS if f[0] == start and f[1] == page)
        if len(body) != count or fingerprint(body) != expected:
            raise ValueError(f'Mac actor-action contract changed or missing at {start:x} page {page:x}')


def require_actor_action_index(db, selected=None):
    records = [json.loads(row[0]) for start, end, _, _ in CONTRACTS for row in db.execute(
        'SELECT body FROM instructions WHERE address>=? AND address<? ORDER BY address', (start, end))]
    if selected is None: require_actor_action_bodies(records, complete=True)
    else:
        for page in sorted({f[1] for f in FRAGMENTS}):
            require_actor_action_contract(page, [r for _, r in sorted(selected.items())], selected)
    require_actor_action_edges(db.execute("""SELECT i.address,e.value
        FROM instructions AS i,
          json_each(json_insert(json_extract(i.body,'$.flows'),'$[#]',json_extract(i.body,'$.fallthrough'))) AS e
        WHERE e.value>=? AND e.value<?""", (min(v[0] for v in CONTRACTS), max(v[1] for v in CONTRACTS))))


def actor_action_hook(entry):
    return f'if (try_modern_actor_action(s, 0x{entry:x})) return true;'


def require_actor_action_hooks(page, instructions, code):
    addresses = {r['address'] for r in instructions}
    for entry in ACTOR_ACTION_ENTRIES:
        if entry >> 12 != page or entry not in addresses: continue
        label, hook = f'i_{entry:x}:', actor_action_hook(entry)
        fragment = code.split(label, 1)[-1].split(f'i_{entry + 4:x}:', 1)[0]
        steps = [token for token in (f's.step(0x{entry:x});', f'counter.step(0x{entry:x});') if token in fragment]
        if code.count(hook) != 1 or hook not in fragment or len(steps) != 1 or fragment.index(hook) > fragment.index(steps[0]):
            raise ValueError(f'Missing or misplaced actor-action hook at {entry:x}')


PRIMARY_ACTION_TABLE = (277571636, 277669624, 36, '3e6802b30d3d1390c4a4dc16641bebcd52b88720bed7374f12a840c32554caba')

def require_actor_action_tables(read):
    import struct
    slot, base, size, expected = PRIMARY_ACTION_TABLE
    if struct.unpack('>I', read(slot,4))[0] != base or hashlib.sha256(read(base,size)).hexdigest() != expected:
        raise ValueError('Changed original actor action classification table')

def load_actor_action_table(directory):
    from pathlib import Path
    directory=Path(directory)
    program=json.loads((directory/'program.json').read_text())
    segments=[(s['address'],(directory/s['file']).read_bytes())for s in program['segments']if s.get('file')]
    def read(address,size):
        for base,data in segments:
            if base<=address and address+size<=base+len(data):return data[address-base:address-base+size]
        raise ValueError('Missing original actor action data')
    require_actor_action_tables(read)
    return read(PRIMARY_ACTION_TABLE[1],PRIMARY_ACTION_TABLE[2])
