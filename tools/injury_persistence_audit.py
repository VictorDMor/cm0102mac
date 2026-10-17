"""Pinned complete injury controller persistence boundaries."""
import hashlib
import json

CONTRACTS = (
    (0x10184530, 0x10184e58, 0x248, '2e4f10da06e4aa26c9fb8abc2dad797bc9b3fb5869356f914f77d3f5bf3196b2'),
    (0x10184e60, 0x10185404, 0x169, 'a32e2ba2a6162423e65b507febc701994e7d4356b0c0c1724819c82fdfc674bc'),
)
FRAGMENTS = (
    (0x10184530, 0x10184, 0x248, '2e4f10da06e4aa26c9fb8abc2dad797bc9b3fb5869356f914f77d3f5bf3196b2'),
    (0x10184e60, 0x10184, 0x68, '35fc3cf98e25113e59b3decd8a12f935d2d8f603d6e3be18b17e345eeead3342'),
    (0x10184e60, 0x10185, 0x101, 'eca848e9deaeda7feb9d0f9e87a5239a8e14a2db7f5ece8195029b3b18aa331c'),
)
INJURY_PERSISTENCE_ENTRIES = tuple(start for start, _, _, _ in CONTRACTS)


def fingerprint(body):
    canonical = [{key: r.get(key) for key in ('address', 'assembly', 'ops', 'flows', 'fallthrough')} for r in body]
    return hashlib.sha256(json.dumps(canonical, sort_keys=True, separators=(',', ':')).encode()).hexdigest()


def require_injury_persistence_bodies(instructions, complete=False):
    for start, end, count, expected in CONTRACTS:
        body = [r for r in instructions if start <= r['address'] < end]
        if not body and not complete: continue
        if len(body) != count or fingerprint(body) != expected:
            raise ValueError(f'Mac injury-persistence contract changed or missing at {start:x}')


def require_injury_persistence_edges(edges):
    for source, target in edges:
        for start, end, _, _ in CONTRACTS:
            if start < target < end and not start <= source < end:
                raise ValueError(f'External injury-persistence interior edge from {source:x} to {target:x}')


def require_injury_persistence_contract(page, instructions, selected=None):
    addresses = {r['address'] for r in instructions} | set(selected or ())
    for start, end, _, _ in CONTRACTS:
        if not start >> 12 <= page <= (end - 1) >> 12: continue
        low, high = max(start, page << 12), min(end, (page + 1) << 12)
        if start not in addresses and not any(low <= a < high for a in addresses): continue
        body = [r for r in instructions if low <= r['address'] < high]
        _, _, count, expected = next(f for f in FRAGMENTS if f[0] == start and f[1] == page)
        if len(body) != count or fingerprint(body) != expected:
            raise ValueError(f'Mac injury-persistence contract changed or missing at {start:x} page {page:x}')


def require_injury_persistence_index(db, selected=None):
    records = [json.loads(row[0]) for start, end, _, _ in CONTRACTS for row in db.execute(
        'SELECT body FROM instructions WHERE address>=? AND address<? ORDER BY address', (start, end))]
    if selected is None: require_injury_persistence_bodies(records, complete=True)
    else:
        for page in sorted({f[1] for f in FRAGMENTS}):
            require_injury_persistence_contract(page, [r for _, r in sorted(selected.items())], selected)
    require_injury_persistence_edges(db.execute("""SELECT i.address,e.value
        FROM instructions AS i,
          json_each(json_insert(json_extract(i.body,'$.flows'),'$[#]',json_extract(i.body,'$.fallthrough'))) AS e
        WHERE e.value>=? AND e.value<?""", (CONTRACTS[0][0], CONTRACTS[-1][1])))


def injury_persistence_hook(entry):
    return f'if (try_modern_injury_persistence(s, 0x{entry:x})) return true;'


def require_injury_persistence_hooks(page, instructions, code):
    addresses = {r['address'] for r in instructions}
    for entry in INJURY_PERSISTENCE_ENTRIES:
        if entry >> 12 != page or entry not in addresses: continue
        label, hook = f'i_{entry:x}:', injury_persistence_hook(entry)
        fragment = code.split(label, 1)[-1].split(f'i_{entry + 4:x}:', 1)[0]
        steps = [token for token in (f's.step(0x{entry:x});', f'counter.step(0x{entry:x});') if token in fragment]
        if code.count(hook) != 1 or hook not in fragment or len(steps) != 1 or fragment.index(hook) > fragment.index(steps[0]):
            raise ValueError(f'Missing or misplaced injury-persistence hook at {entry:x}')


TABLES = (
    (0x108b6658, 0x107bdc30, 0x7c, '2a9f66bb18f40f42b0540fcf710cdb1596e588e897d56661f58161a264d7d9ec'),
)

def require_injury_persistence_tables(word):
    import struct
    for slot,base,size,expected in TABLES:
        data=b''.join(struct.pack('>I',word(base+n)) for n in range(0,size,4))
        if word(slot)!=base or hashlib.sha256(data).hexdigest()!=expected:
            raise ValueError('Changed injury persistence table')

def load_injury_persistence_tables(input_directory):
    import struct
    program=json.loads((input_directory/'program.json').read_text())
    segments=[(s['address'],(input_directory/s['file']).read_bytes()) for s in program['segments'] if s.get('file')]
    def word(address):
        for base,data in segments:
            if base<=address and address+4<=base+len(data):return struct.unpack_from('>I',data,address-base)[0]
        raise ValueError('Missing injury persistence table word')
    require_injury_persistence_tables(word)
    return tuple(tuple(word(base+n) for n in range(0,size,4)) for _,base,size,_ in TABLES)
