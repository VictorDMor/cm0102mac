"""Pinned complete injury controller bootstrap boundaries."""
import hashlib
import json

CONTRACTS = (
    (0x101840e0, 0x10184308, 0x89, '5d4914cda97af5552de9c4a0fd83ed20e924088b88767434469974269f40b289'),
    (0x10184310, 0x101843c4, 0x2d, '086be41608605f81a996c0d9471a4f3a0a3d9ab7a067794ba32a82b6e059b548'),
    (0x101843d0, 0x10184528, 0x55, '7c6f976cdea71fc80c1e1aff31dc4c80eea6f369c2ce3779340481bc935b8b59'),
    (0x10187a20, 0x10187a38, 0x6, 'c8aedb2d198cd62af06b47f42700884e6438313d130ec15a2d37954193c2fbec'),
    (0x10188490, 0x101885f0, 0x58, '253ee1c495c39ce6dd8aaa6ca77f874277743b048c1278e2be944e8f6bc9631a'),
    (0x101885f0, 0x10188924, 0xcd, '3a7abf871ab88d8d3a6fb90ade91358b2563229ce25033fb221bdea0cf502f6e'),
)
FRAGMENTS = (
    (0x101840e0, 0x10184, 0x89, '5d4914cda97af5552de9c4a0fd83ed20e924088b88767434469974269f40b289'),
    (0x10184310, 0x10184, 0x2d, '086be41608605f81a996c0d9471a4f3a0a3d9ab7a067794ba32a82b6e059b548'),
    (0x101843d0, 0x10184, 0x55, '7c6f976cdea71fc80c1e1aff31dc4c80eea6f369c2ce3779340481bc935b8b59'),
    (0x10187a20, 0x10187, 0x6, 'c8aedb2d198cd62af06b47f42700884e6438313d130ec15a2d37954193c2fbec'),
    (0x10188490, 0x10188, 0x58, '253ee1c495c39ce6dd8aaa6ca77f874277743b048c1278e2be944e8f6bc9631a'),
    (0x101885f0, 0x10188, 0xcd, '3a7abf871ab88d8d3a6fb90ade91358b2563229ce25033fb221bdea0cf502f6e'),
)
INJURY_BOOTSTRAP_ENTRIES = tuple(start for start, _, _, _ in CONTRACTS)


def fingerprint(body):
    canonical = [{key: r.get(key) for key in ('address', 'assembly', 'ops', 'flows', 'fallthrough')} for r in body]
    return hashlib.sha256(json.dumps(canonical, sort_keys=True, separators=(',', ':')).encode()).hexdigest()


def require_injury_bootstrap_bodies(instructions, complete=False):
    for start, end, count, expected in CONTRACTS:
        body = [r for r in instructions if start <= r['address'] < end]
        if not body and not complete: continue
        if len(body) != count or fingerprint(body) != expected:
            raise ValueError(f'Mac injury-bootstrap contract changed or missing at {start:x}')


def require_injury_bootstrap_edges(edges):
    for source, target in edges:
        for start, end, _, _ in CONTRACTS:
            if start < target < end and not start <= source < end:
                raise ValueError(f'External injury-bootstrap interior edge from {source:x} to {target:x}')


def require_injury_bootstrap_contract(page, instructions, selected=None):
    addresses = {r['address'] for r in instructions} | set(selected or ())
    for start, end, _, _ in CONTRACTS:
        if not start >> 12 <= page <= (end - 1) >> 12: continue
        low, high = max(start, page << 12), min(end, (page + 1) << 12)
        if start not in addresses and not any(low <= a < high for a in addresses): continue
        body = [r for r in instructions if low <= r['address'] < high]
        _, _, count, expected = next(f for f in FRAGMENTS if f[0] == start and f[1] == page)
        if len(body) != count or fingerprint(body) != expected:
            raise ValueError(f'Mac injury-bootstrap contract changed or missing at {start:x} page {page:x}')


def require_injury_bootstrap_index(db, selected=None):
    records = [json.loads(row[0]) for start, end, _, _ in CONTRACTS for row in db.execute(
        'SELECT body FROM instructions WHERE address>=? AND address<? ORDER BY address', (start, end))]
    if selected is None: require_injury_bootstrap_bodies(records, complete=True)
    else:
        for page in sorted({f[1] for f in FRAGMENTS}):
            require_injury_bootstrap_contract(page, [r for _, r in sorted(selected.items())], selected)
    require_injury_bootstrap_edges(db.execute("""SELECT i.address,e.value
        FROM instructions AS i,
          json_each(json_insert(json_extract(i.body,'$.flows'),'$[#]',json_extract(i.body,'$.fallthrough'))) AS e
        WHERE e.value>=? AND e.value<?""", (CONTRACTS[0][0], CONTRACTS[-1][1])))


def injury_bootstrap_hook(entry):
    return f'if (try_modern_injury_bootstrap(s, 0x{entry:x})) return true;'


def require_injury_bootstrap_hooks(page, instructions, code):
    addresses = {r['address'] for r in instructions}
    for entry in INJURY_BOOTSTRAP_ENTRIES:
        if entry >> 12 != page or entry not in addresses: continue
        label, hook = f'i_{entry:x}:', injury_bootstrap_hook(entry)
        fragment = code.split(label, 1)[-1].split(f'i_{entry + 4:x}:', 1)[0]
        steps = [token for token in (f's.step(0x{entry:x});', f'counter.step(0x{entry:x});') if token in fragment]
        if code.count(hook) != 1 or hook not in fragment or len(steps) != 1 or fragment.index(hook) > fragment.index(steps[0]):
            raise ValueError(f'Missing or misplaced injury-bootstrap hook at {entry:x}')


TABLES = (
    (0x108b665c, 0x107bdc20, 0x10, '6c4b958950fa3b400cdab1c7428d1fc146b5ee40418fa4cd6af042ccaba7f7e5'),
    (0x108b6660, 0x107bdc0c, 0x14, '1df9697630f129a66d6237588313e0a8ee55b244399ec5d33a1c71d2746ad8c8'),
    (0x108b55bc, 0x108cb8e0, 0x3d8, 'b248e4d53091f6e220e694dd4b053ca878eede63059a959e7ae52480e1f6be51'),
)

def require_injury_bootstrap_tables(word):
    import struct
    for slot,base,size,expected in TABLES:
        data=b''.join(struct.pack('>I',word(base+n)) for n in range(0,size,4))
        if word(slot)!=base or hashlib.sha256(data).hexdigest()!=expected:
            raise ValueError('Changed injury bootstrap table')

def load_injury_bootstrap_tables(input_directory):
    import struct
    program=json.loads((input_directory/'program.json').read_text())
    segments=[(s['address'],(input_directory/s['file']).read_bytes()) for s in program['segments'] if s.get('file')]
    def word(address):
        for base,data in segments:
            if base<=address and address+4<=base+len(data):return struct.unpack_from('>I',data,address-base)[0]
        raise ValueError('Missing injury bootstrap table word')
    require_injury_bootstrap_tables(word)
    return tuple(tuple(word(base+n) for n in range(0,size,4)) for _,base,size,_ in TABLES)
