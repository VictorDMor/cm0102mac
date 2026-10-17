"""Pinned complete daily injury progression boundaries."""
import hashlib
import json

CONTRACTS = (
    (0x1018b2b0, 0x1018b6e0, 268, '44ba916e0abd85996a698c75c653644bacf84bf6fa701ea4653e69fbf963c6e0'),
    (0x1018dcb0, 0x1018f33c, 1443, '69c0d365faf338349e68dcba25ba21bb4125fe044447f49e938dcc59071890c9'),
    (0x1018ff10, 0x1018ffe0, 52, 'b793fab49dd86f0a1396e65a41439a1695083efec1b92542fa0f437906be1fd4'),
    (0x1018ffe0, 0x101900b0, 52, '935c147863a40c9e14645abb038a2ff84ccaa8694f39b248b2e7c62e69d025db'),
)
FRAGMENTS = (
    (0x1018b2b0, 0x1018b, 268, '44ba916e0abd85996a698c75c653644bacf84bf6fa701ea4653e69fbf963c6e0'),
    (0x1018dcb0, 0x1018d, 212, '5093b75903d07ee9bc6479063f53ad167359cdcd5e72fda734a2094fa9942b8a'),
    (0x1018dcb0, 0x1018e, 1024, '23b0aac0892a93f04138c9b0df6ad53667eaf285b7d30b8a1ac375b7ba74a388'),
    (0x1018dcb0, 0x1018f, 207, 'e1f0333221b04d75a17bbf29d24c605e3e9002e2c452308c66d26e51c6fa1ac6'),
    (0x1018ff10, 0x1018f, 52, 'b793fab49dd86f0a1396e65a41439a1695083efec1b92542fa0f437906be1fd4'),
    (0x1018ffe0, 0x1018f, 8, 'adc07d0377b6b319f374be7530f55e7fa9a98fea604cf141e5ace07157b9d1b2'),
    (0x1018ffe0, 0x10190, 44, '59b42ed1f2b4584c32855af6edc560cc4cbb86c277ca15a58940b171f749d601'),
)
INJURY_PROGRESSION_ENTRIES = tuple(start for start, _, _, _ in CONTRACTS)


def fingerprint(body):
    canonical = [{key: r.get(key) for key in ('address', 'assembly', 'ops', 'flows', 'fallthrough')} for r in body]
    return hashlib.sha256(json.dumps(canonical, sort_keys=True, separators=(',', ':')).encode()).hexdigest()


def require_injury_progression_bodies(instructions, complete=False):
    for start, end, count, expected in CONTRACTS:
        body = [r for r in instructions if start <= r['address'] < end]
        if not body and not complete: continue
        if len(body) != count or fingerprint(body) != expected:
            raise ValueError(f'Mac injury-progression contract changed or missing at {start:x}')


def require_injury_progression_edges(edges):
    for source, target in edges:
        for start, end, _, _ in CONTRACTS:
            if start < target < end and not start <= source < end:
                raise ValueError(f'External injury-progression interior edge from {source:x} to {target:x}')


def require_injury_progression_contract(page, instructions, selected=None):
    addresses = {r['address'] for r in instructions} | set(selected or ())
    for start, end, _, _ in CONTRACTS:
        if not start >> 12 <= page <= (end - 1) >> 12: continue
        low, high = max(start, page << 12), min(end, (page + 1) << 12)
        if start not in addresses and not any(low <= a < high for a in addresses): continue
        body = [r for r in instructions if low <= r['address'] < high]
        _, _, count, expected = next(f for f in FRAGMENTS if f[0] == start and f[1] == page)
        if len(body) != count or fingerprint(body) != expected:
            raise ValueError(f'Mac injury-progression contract changed or missing at {start:x} page {page:x}')


def require_injury_progression_index(db, selected=None):
    records = [json.loads(row[0]) for start, end, _, _ in CONTRACTS for row in db.execute(
        'SELECT body FROM instructions WHERE address>=? AND address<? ORDER BY address', (start, end))]
    if selected is None: require_injury_progression_bodies(records, complete=True)
    else:
        for page in sorted({f[1] for f in FRAGMENTS}):
            require_injury_progression_contract(page, [r for _, r in sorted(selected.items())], selected)
    require_injury_progression_edges(db.execute("""SELECT i.address,e.value
        FROM instructions AS i,
          json_each(json_insert(json_extract(i.body,'$.flows'),'$[#]',json_extract(i.body,'$.fallthrough'))) AS e
        WHERE e.value>=? AND e.value<?""", (CONTRACTS[0][0], CONTRACTS[-1][1])))


def injury_progression_hook(entry):
    return f'if (try_modern_injury_progression(s, 0x{entry:x})) return true;'


def require_injury_progression_hooks(page, instructions, code):
    addresses = {r['address'] for r in instructions}
    for entry in INJURY_PROGRESSION_ENTRIES:
        if entry >> 12 != page or entry not in addresses: continue
        label, hook = f'i_{entry:x}:', injury_progression_hook(entry)
        fragment = code.split(label, 1)[-1].split(f'i_{entry + 4:x}:', 1)[0]
        steps = [token for token in (f's.step(0x{entry:x});', f'counter.step(0x{entry:x});') if token in fragment]
        if code.count(hook) != 1 or hook not in fragment or len(steps) != 1 or fragment.index(hook) > fragment.index(steps[0]):
            raise ValueError(f'Missing or misplaced injury-progression hook at {entry:x}')


CALLBACK_DESCRIPTORS = ((0x108b6638,0x108b8b4c,0x1018ffe0,0x108bba90),
                        (0x108b663c,0x108b8b54,0x1018ff10,0x108bba90))

def require_injury_progression_tables(word):
    from actor_attributes_audit import INJURY_TABLES
    import struct
    slot,table,count,expected=INJURY_TABLES[0]
    if word(slot)!=table or hashlib.sha256(b''.join(struct.pack('>I',word(table+4*n)) for n in range(count))).hexdigest()!=expected:
        raise ValueError('Changed injury progression detail table')
    for slot,descriptor,entry,toc in CALLBACK_DESCRIPTORS:
        if word(slot)!=descriptor or word(descriptor)!=entry or word(descriptor+4)!=toc:
            raise ValueError('Changed queued injury comparison descriptor')

def load_injury_progression_tables(input_directory):
    import struct
    program=json.loads((input_directory/'program.json').read_text())
    segments=[(s['address'],(input_directory/s['file']).read_bytes()) for s in program['segments'] if s.get('file')]
    def word(address):
        for base,data in segments:
            if base<=address and address+4<=base+len(data):return struct.unpack_from('>I',data,address-base)[0]
        raise ValueError('Missing injury progression table word')
    require_injury_progression_tables(word)
