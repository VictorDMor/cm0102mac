"""Pinned complete injury registration/notice/propagation boundaries."""
import hashlib
import json

CONTRACTS = (
    (0x10188cc0, 0x1018aec4, 2175, '40d623f4c9cb3d21bce78355612c0d76c44ae91075cb6cb6b1767a28f5e61243'),
    (0x1018aed0, 0x1018b2b0, 248, 'c8391b6320e9b4f501a6f1733f0810de8ecb757abd234c4a649d47fedacb9187'),
    (0x1018b6e0, 0x1018b9ec, 194, 'fb1fd0f6133e050797b652f3cf585533d460e796faadcae1fc4b53503ea16283'),
)
FRAGMENTS = (
    (0x10188cc0, 0x10188, 208, 'c27881546f0b28223956b3919a1754c59bbaa057634b3ec423a61626de8c43d9'),
    (0x10188cc0, 0x10189, 1022, '8aed169992e29c829049cde46e9b69f2d3b72268f0946ea901ea92156eaffc2e'),
    (0x10188cc0, 0x1018a, 945, 'a407440ef096d21fc9adbd8d03c3181d3100bc98ac0c6cdfaa5e24d6fb284243'),
    (0x1018aed0, 0x1018a, 76, '2f5fbed32bed7faf454de3a3bffe1e5e5a3accd9c1ba870ff9027c1377a654ed'),
    (0x1018aed0, 0x1018b, 172, '7452bd95886ce1c74865c46fc51faefee6e6038cebe0b95c8a4a4f05a5f8e0b5'),
    (0x1018b6e0, 0x1018b, 194, 'fb1fd0f6133e050797b652f3cf585533d460e796faadcae1fc4b53503ea16283'),
)
PROPAGATION_TABLE = (0x108b6634, 0x108cbda0, 66, 'ee557a972d4976b0d6cd2dade64a1c793dbe0f98fa027d0c336feaad37d626a1')
INJURY_RECORD_ENTRIES = tuple(start for start, _, _, _ in CONTRACTS)


def fingerprint(body):
    canonical = [{key: r.get(key) for key in ('address', 'assembly', 'ops', 'flows', 'fallthrough')} for r in body]
    return hashlib.sha256(json.dumps(canonical, sort_keys=True, separators=(',', ':')).encode()).hexdigest()


def require_injury_record_bodies(instructions, complete=False):
    for start, end, count, expected in CONTRACTS:
        body = [r for r in instructions if start <= r['address'] < end]
        if not body and not complete: continue
        if len(body) != count or fingerprint(body) != expected:
            raise ValueError(f'Mac injury-record contract changed or missing at {start:x}')


def require_injury_record_edges(edges):
    for source, target in edges:
        for start, end, _, _ in CONTRACTS:
            if start < target < end and not start <= source < end:
                raise ValueError(f'External injury-record interior edge from {source:x} to {target:x}')


def require_injury_record_contract(page, instructions, selected=None):
    addresses = {r['address'] for r in instructions} | set(selected or ())
    for start, end, _, _ in CONTRACTS:
        if not start >> 12 <= page <= (end - 1) >> 12: continue
        low, high = max(start, page << 12), min(end, (page + 1) << 12)
        if start not in addresses and not any(low <= a < high for a in addresses): continue
        body = [r for r in instructions if low <= r['address'] < high]
        _, _, count, expected = next(f for f in FRAGMENTS if f[0] == start and f[1] == page)
        if len(body) != count or fingerprint(body) != expected:
            raise ValueError(f'Mac injury-record contract changed or missing at {start:x} page {page:x}')


def require_injury_record_index(db, selected=None):
    records = [json.loads(row[0]) for start, end, _, _ in CONTRACTS for row in db.execute(
        'SELECT body FROM instructions WHERE address>=? AND address<? ORDER BY address', (start, end))]
    if selected is None: require_injury_record_bodies(records, complete=True)
    else:
        for page in sorted({f[1] for f in FRAGMENTS}):
            require_injury_record_contract(page, [r for _, r in sorted(selected.items())], selected)
    require_injury_record_edges(db.execute("""SELECT i.address,e.value
        FROM instructions AS i,
          json_each(json_insert(json_extract(i.body,'$.flows'),'$[#]',json_extract(i.body,'$.fallthrough'))) AS e
        WHERE e.value>=? AND e.value<?""", (CONTRACTS[0][0], CONTRACTS[-1][1])))


def injury_record_hook(entry):
    return f'if (try_modern_injury_record(s, 0x{entry:x})) return true;'


def require_injury_record_hooks(page, instructions, code):
    addresses = {r['address'] for r in instructions}
    for entry in INJURY_RECORD_ENTRIES:
        if entry >> 12 != page or entry not in addresses: continue
        label, hook = f'i_{entry:x}:', injury_record_hook(entry)
        fragment = code.split(label, 1)[-1].split(f'i_{entry + 4:x}:', 1)[0]
        steps = [token for token in (f's.step(0x{entry:x});', f'counter.step(0x{entry:x});') if token in fragment]
        if code.count(hook) != 1 or hook not in fragment or len(steps) != 1 or fragment.index(hook) > fragment.index(steps[0]):
            raise ValueError(f'Missing or misplaced injury-record hook at {entry:x}')


def load_injury_record_tables(input_directory):
    import struct
    from actor_attributes_audit import INJURY_TABLES
    program=json.loads((input_directory/'program.json').read_text())
    segments=[(s['address'],(input_directory/s['file']).read_bytes()) for s in program['segments'] if s.get('file')]
    def word(address):
        for base,data in segments:
            if base<=address and address+4<=base+len(data):return struct.unpack_from('>I',data,address-base)[0]
        raise ValueError('Missing original injury-record table word')
    for address in (0x10189304,0x10189324,0x1018b7c8):
        if word(address)!=0x60000000:raise ValueError('Changed skipped injury-record alignment word')
    tables=(INJURY_TABLES[0],PROPAGATION_TABLE)
    values={address:word(address) for slot,table,count,_ in tables for address in (slot,*range(table,table+4*count,4))}
    require_injury_record_tables(values.__getitem__)
    return [word(tables[-1][1]+4*n) for n in range(tables[-1][2])]


def require_injury_record_tables(word):
    import struct
    from actor_attributes_audit import INJURY_TABLES
    result=[]
    for slot,table,count,digest in (INJURY_TABLES[0],PROPAGATION_TABLE):
        if word(slot)!=table:raise ValueError('Changed original injury-record table pointer')
        words=[word(table+4*n) for n in range(count)]
        if hashlib.sha256(b''.join(struct.pack('>I',w) for w in words)).hexdigest()!=digest:
            raise ValueError('Changed original injury-record table bytes')
        result.append(words)
    if any(w not in (0x1018b988,0x1018b990,0x1018b998) for w in result[-1]):
        raise ValueError('Changed original related-injury destinations')
    return result
