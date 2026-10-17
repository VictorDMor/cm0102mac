"""Pinned complete injury status readouts boundaries."""
import hashlib
import json

CONTRACTS = (
    (0x1018c1a0, 0x1018c26c, 51, 'a8699ac27b74c3a4bdca73078421e10ecdf5fb8a60cef482168a1209ab009154'),
    (0x1018c270, 0x1018c2e4, 29, 'f2fa3729965d014e13b4cc2eeac8c287b5bbb42431484ad05a2525e0698cf69e'),
    (0x1018c2f0, 0x1018c33c, 19, '58012abb69bde208c3bddc9790c810db51736eb1f9f4fbb857a23fd3ba3060c4'),
    (0x1018c450, 0x1018c560, 68, '4e58b20bde90668c2a73f2c3e46f47420222fb4cbe4df370a9d476a9d8cff309'),
    (0x1018c560, 0x1018d140, 760, '2f801161ae9ccb03b2163c2b8c85ec3713700e50cddab0979c5ba037ef62db19'),
    (0x1018d140, 0x1018d94c, 515, '7c8e9b921a9ff394f5d920fd5f24d0c53bfe9fe45229f0137db59d0e6819fd31'),
    (0x1018d950, 0x1018daa8, 86, '57a941b5946ac78c2764a17d2adf96cbcbb700fa74d3b4d157d699825b143e05'),
    (0x1018dab0, 0x1018dcb0, 128, '51a852c17060d378a3f403dc1fbc16bda5e81fe2a7a9399d9f2b800e77f6dba9'),
)
FRAGMENTS = (
    (0x1018c1a0, 0x1018c, 51, 'a8699ac27b74c3a4bdca73078421e10ecdf5fb8a60cef482168a1209ab009154'),
    (0x1018c270, 0x1018c, 29, 'f2fa3729965d014e13b4cc2eeac8c287b5bbb42431484ad05a2525e0698cf69e'),
    (0x1018c2f0, 0x1018c, 19, '58012abb69bde208c3bddc9790c810db51736eb1f9f4fbb857a23fd3ba3060c4'),
    (0x1018c450, 0x1018c, 68, '4e58b20bde90668c2a73f2c3e46f47420222fb4cbe4df370a9d476a9d8cff309'),
    (0x1018c560, 0x1018c, 680, '267dc8842ea5c557b82625880dff52affc98df9e505b64ad55d3fab1750278f4'),
    (0x1018c560, 0x1018d, 80, 'a41418576e45387c4b516499e344b9d2767ab94a64a15a0a5489f6c4262743be'),
    (0x1018d140, 0x1018d, 515, '7c8e9b921a9ff394f5d920fd5f24d0c53bfe9fe45229f0137db59d0e6819fd31'),
    (0x1018d950, 0x1018d, 86, '57a941b5946ac78c2764a17d2adf96cbcbb700fa74d3b4d157d699825b143e05'),
    (0x1018dab0, 0x1018d, 128, '51a852c17060d378a3f403dc1fbc16bda5e81fe2a7a9399d9f2b800e77f6dba9'),
)
INJURY_READOUTS_ENTRIES = tuple(start for start, _, _, _ in CONTRACTS)


def fingerprint(body):
    canonical = [{key: r.get(key) for key in ('address', 'assembly', 'ops', 'flows', 'fallthrough')} for r in body]
    return hashlib.sha256(json.dumps(canonical, sort_keys=True, separators=(',', ':')).encode()).hexdigest()


def require_injury_readouts_bodies(instructions, complete=False):
    for start, end, count, expected in CONTRACTS:
        body = [r for r in instructions if start <= r['address'] < end]
        if not body and not complete: continue
        if len(body) != count or fingerprint(body) != expected:
            raise ValueError(f'Mac injury-readouts contract changed or missing at {start:x}')


def require_injury_readouts_edges(edges):
    for source, target in edges:
        for start, end, _, _ in CONTRACTS:
            if start < target < end and not start <= source < end:
                raise ValueError(f'External injury-readouts interior edge from {source:x} to {target:x}')


def require_injury_readouts_contract(page, instructions, selected=None):
    addresses = {r['address'] for r in instructions} | set(selected or ())
    for start, end, _, _ in CONTRACTS:
        if not start >> 12 <= page <= (end - 1) >> 12: continue
        low, high = max(start, page << 12), min(end, (page + 1) << 12)
        if start not in addresses and not any(low <= a < high for a in addresses): continue
        body = [r for r in instructions if low <= r['address'] < high]
        _, _, count, expected = next(f for f in FRAGMENTS if f[0] == start and f[1] == page)
        if len(body) != count or fingerprint(body) != expected:
            raise ValueError(f'Mac injury-readouts contract changed or missing at {start:x} page {page:x}')


def require_injury_readouts_index(db, selected=None):
    records = [json.loads(row[0]) for start, end, _, _ in CONTRACTS for row in db.execute(
        'SELECT body FROM instructions WHERE address>=? AND address<? ORDER BY address', (start, end))]
    if selected is None: require_injury_readouts_bodies(records, complete=True)
    else:
        for page in sorted({f[1] for f in FRAGMENTS}):
            require_injury_readouts_contract(page, [r for _, r in sorted(selected.items())], selected)
    require_injury_readouts_edges(db.execute("""SELECT i.address,e.value
        FROM instructions AS i,
          json_each(json_insert(json_extract(i.body,'$.flows'),'$[#]',json_extract(i.body,'$.fallthrough'))) AS e
        WHERE e.value>=? AND e.value<?""", (CONTRACTS[0][0], CONTRACTS[-1][1])))


def injury_readouts_hook(entry):
    return f'if (try_modern_injury_readouts(s, 0x{entry:x})) return true;'


def require_injury_readouts_hooks(page, instructions, code):
    addresses = {r['address'] for r in instructions}
    for entry in INJURY_READOUTS_ENTRIES:
        if entry >> 12 != page or entry not in addresses: continue
        label, hook = f'i_{entry:x}:', injury_readouts_hook(entry)
        fragment = code.split(label, 1)[-1].split(f'i_{entry + 4:x}:', 1)[0]
        steps = [token for token in (f's.step(0x{entry:x});', f'counter.step(0x{entry:x});') if token in fragment]
        if code.count(hook) != 1 or hook not in fragment or len(steps) != 1 or fragment.index(hook) > fragment.index(steps[0]):
            raise ValueError(f'Missing or misplaced injury-readouts hook at {entry:x}')


TABLES = (
    (0x108b662c,0x108cbff0,82,'90dc09799bd09338bdb38c318cffe4ce8d9cc08677280bab2391acaad8b162cc'),
    (0x108b6628,0x108cbea8,82,'620b65cdc138f42d3f06edcaf07972efe645b27a7f45e61da8386859ae2da920'),
    (0x108b6624,0x108cc3a0,77,'be07d5e2b64cf60cd142fffe0d0ec9ac6cb1067f4de078a6e6cae171c7c39b7c'),
    (0x108b6620,0x108cc26c,77,'be03a7b3fc3a8451d84f69d110050722e54fd525ff33f433ae56739597237ffd'),
    (0x108b661c,0x108cc138,77,'60fbaf89fb3b12b99c615155b89ff471c4ffa8cb8466258fb7c1a363cbeca71b'),
    (0x108b6618,0x108cc4d4,13,'7cff4af7a267fbe129501134ab8a40187f4e62f3e578ae59ab593826b52089f4'),
)

def require_injury_readouts_tables(word):
    import struct
    for slot,base,count,expected in TABLES:
        if word(slot)!=base or hashlib.sha256(b''.join(struct.pack('>I',word(base+4*n)) for n in range(count))).hexdigest()!=expected:
            raise ValueError('Changed injury readout choice table')

def load_injury_readouts_tables(input_directory):
    import struct
    program=json.loads((input_directory/'program.json').read_text())
    segments=[(s['address'],(input_directory/s['file']).read_bytes()) for s in program['segments'] if s.get('file')]
    def word(address):
        for base,data in segments:
            if base<=address and address+4<=base+len(data):return struct.unpack_from('>I',data,address-base)[0]
        raise ValueError('Missing injury readout choice-table word')
    require_injury_readouts_tables(word)
    return [tuple(word(base+4*n) for n in range(count)) for _,base,count,_ in TABLES]
