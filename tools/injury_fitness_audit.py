"""Pinned complete injury fitness and recovery boundaries."""
import hashlib
import json

CONTRACTS = (
    (0x10187450, 0x10187644, 0x7d, 'b02282701eb9a35594724b34a1f671d4a00f52bf159bb31b1eddc5eb1b62f9b8'),
    (0x10187650, 0x10187680, 0xc, 'ba9ea5934cd16e94dacec20304d2f9e3ab2b8103d99dec483f8849002d7fd44c'),
    (0x10187680, 0x101877dc, 0x57, '7db6901350acd1729ac79de24d5c9b846028fab6453b948ad58e30b937bb5f84'),
    (0x101877e0, 0x101878d0, 0x3c, '75ed82c07e8157772ab32b791a2d81910a7605588b77ef3c2e34b0c3808266ee'),
    (0x101878d0, 0x10187a1c, 0x53, '9eced854756a34c572ff8e72b1c6a6673cd41d7d405d543ddc0ae1e12965236f'),
    (0x10187a40, 0x10188488, 0x292, 'b3f2824f1f997c70bd8384e3ca6672cf70a9822aad70e7160e2b8ac8b20043fb'),
)
FRAGMENTS = (
    (0x10187450, 0x10187, 0x7d, 'b02282701eb9a35594724b34a1f671d4a00f52bf159bb31b1eddc5eb1b62f9b8'),
    (0x10187650, 0x10187, 0xc, 'ba9ea5934cd16e94dacec20304d2f9e3ab2b8103d99dec483f8849002d7fd44c'),
    (0x10187680, 0x10187, 0x57, '7db6901350acd1729ac79de24d5c9b846028fab6453b948ad58e30b937bb5f84'),
    (0x101877e0, 0x10187, 0x3c, '75ed82c07e8157772ab32b791a2d81910a7605588b77ef3c2e34b0c3808266ee'),
    (0x101878d0, 0x10187, 0x53, '9eced854756a34c572ff8e72b1c6a6673cd41d7d405d543ddc0ae1e12965236f'),
    (0x10187a40, 0x10187, 0x170, '8e71b13c57b5a3a91ba2d9df84024abbf82fae85ad744dee22c863b3b0ce682d'),
    (0x10187a40, 0x10188, 0x122, '47ba597bd84fcfa38512d97dd629f7b2e2040df58c943c30b3f5d74eb48303cf'),
)
INJURY_FITNESS_ENTRIES = tuple(start for start, _, _, _ in CONTRACTS)


def fingerprint(body):
    canonical = [{key: r.get(key) for key in ('address', 'assembly', 'ops', 'flows', 'fallthrough')} for r in body]
    return hashlib.sha256(json.dumps(canonical, sort_keys=True, separators=(',', ':')).encode()).hexdigest()


def require_injury_fitness_bodies(instructions, complete=False):
    for start, end, count, expected in CONTRACTS:
        body = [r for r in instructions if start <= r['address'] < end]
        if not body and not complete: continue
        if len(body) != count or fingerprint(body) != expected:
            raise ValueError(f'Mac injury-fitness contract changed or missing at {start:x}')


def require_injury_fitness_edges(edges):
    for source, target in edges:
        for start, end, _, _ in CONTRACTS:
            if start < target < end and not start <= source < end:
                raise ValueError(f'External injury-fitness interior edge from {source:x} to {target:x}')


def require_injury_fitness_contract(page, instructions, selected=None):
    addresses = {r['address'] for r in instructions} | set(selected or ())
    for start, end, _, _ in CONTRACTS:
        if not start >> 12 <= page <= (end - 1) >> 12: continue
        low, high = max(start, page << 12), min(end, (page + 1) << 12)
        if start not in addresses and not any(low <= a < high for a in addresses): continue
        body = [r for r in instructions if low <= r['address'] < high]
        _, _, count, expected = next(f for f in FRAGMENTS if f[0] == start and f[1] == page)
        if len(body) != count or fingerprint(body) != expected:
            raise ValueError(f'Mac injury-fitness contract changed or missing at {start:x} page {page:x}')


def require_injury_fitness_index(db, selected=None):
    records = [json.loads(row[0]) for start, end, _, _ in CONTRACTS for row in db.execute(
        'SELECT body FROM instructions WHERE address>=? AND address<? ORDER BY address', (start, end))]
    if selected is None: require_injury_fitness_bodies(records, complete=True)
    else:
        for page in sorted({f[1] for f in FRAGMENTS}):
            require_injury_fitness_contract(page, [r for _, r in sorted(selected.items())], selected)
    require_injury_fitness_edges(db.execute("""SELECT i.address,e.value
        FROM instructions AS i,
          json_each(json_insert(json_extract(i.body,'$.flows'),'$[#]',json_extract(i.body,'$.fallthrough'))) AS e
        WHERE e.value>=? AND e.value<?""", (CONTRACTS[0][0], CONTRACTS[-1][1])))


def injury_fitness_hook(entry):
    return f'if (try_modern_injury_fitness(s, 0x{entry:x})) return true;'


def require_injury_fitness_hooks(page, instructions, code):
    addresses = {r['address'] for r in instructions}
    for entry in INJURY_FITNESS_ENTRIES:
        if entry >> 12 != page or entry not in addresses: continue
        label, hook = f'i_{entry:x}:', injury_fitness_hook(entry)
        fragment = code.split(label, 1)[-1].split(f'i_{entry + 4:x}:', 1)[0]
        steps = [token for token in (f's.step(0x{entry:x});', f'counter.step(0x{entry:x});') if token in fragment]
        if code.count(hook) != 1 or hook not in fragment or len(steps) != 1 or fragment.index(hook) > fragment.index(steps[0]):
            raise ValueError(f'Missing or misplaced injury-fitness hook at {entry:x}')


TABLES = (
    (0x108b6654, 0x107bf9c0, 0x70, '96ac90b9e6d61e2a7dfb2c3f8fbd64fd612adccb0b63db1364ca15b5a36d148c'),
    (0x108b664c, 0x107bf9bc, 0x4, '9dc1b62d60acff118f222dc22cb76a1c7e9f2fcc1ae7d16c56861eb0301bd59a'),
    (0x108b6658, 0x107bdc30, 0x7c, '2a9f66bb18f40f42b0540fcf710cdb1596e588e897d56661f58161a264d7d9ec'),
)

def require_injury_fitness_tables(word):
    import struct
    for slot,base,size,expected in TABLES:
        data=b''.join(struct.pack('>I',word(base+n)) for n in range(0,size,4))
        if word(slot)!=base or hashlib.sha256(data).hexdigest()!=expected:
            raise ValueError('Changed injury fitness table')

def load_injury_fitness_tables(input_directory):
    import struct
    program=json.loads((input_directory/'program.json').read_text())
    segments=[(s['address'],(input_directory/s['file']).read_bytes()) for s in program['segments'] if s.get('file')]
    def word(address):
        for base,data in segments:
            if base<=address and address+4<=base+len(data):return struct.unpack_from('>I',data,address-base)[0]
        raise ValueError('Missing injury fitness table word')
    require_injury_fitness_tables(word)
    return tuple(tuple(word(base+n) for n in range(0,size,4)) for _,base,size,_ in TABLES)
