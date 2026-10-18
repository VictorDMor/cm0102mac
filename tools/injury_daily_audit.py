"""Pinned complete daily injury processing boundaries."""
import hashlib
import json

CONTRACTS = ((270029840, 270034080, 1060, '92319aa69751de9a4dce416a6d9fb50cedfa440baa6c264e6889f91fd8c72813'),)
FRAGMENTS = ((270029840, 65925, 764, 'cb7394469e3f6a421903274c86dde906a0137ca9f8f577a3482b91f1184d4887'), (270029840, 65926, 296, 'a93aa27831c32afccda3142464779ebc4355db412d2c1d0ec0b7a61298576392'))
INJURY_DAILY_ENTRIES = tuple(start for start, _, _, _ in CONTRACTS)


def fingerprint(body):
    canonical = [{key: r.get(key) for key in ('address', 'assembly', 'ops', 'flows', 'fallthrough')} for r in body]
    return hashlib.sha256(json.dumps(canonical, sort_keys=True, separators=(',', ':')).encode()).hexdigest()


def require_injury_daily_bodies(instructions, complete=False):
    for start, end, count, expected in CONTRACTS:
        body = [r for r in instructions if start <= r['address'] < end]
        if not body and not complete: continue
        if len(body) != count or fingerprint(body) != expected:
            raise ValueError(f'Mac injury-daily contract changed or missing at {start:x}')


def require_injury_daily_edges(edges):
    for source, target in edges:
        for start, end, _, _ in CONTRACTS:
            if start < target < end and not start <= source < end:
                raise ValueError(f'External injury-daily interior edge from {source:x} to {target:x}')


def require_injury_daily_contract(page, instructions, selected=None):
    addresses = {r['address'] for r in instructions} | set(selected or ())
    for start, end, _, _ in CONTRACTS:
        if not start >> 12 <= page <= (end - 1) >> 12: continue
        low, high = max(start, page << 12), min(end, (page + 1) << 12)
        if start not in addresses and not any(low <= a < high for a in addresses): continue
        body = [r for r in instructions if low <= r['address'] < high]
        _, _, count, expected = next(f for f in FRAGMENTS if f[0] == start and f[1] == page)
        if len(body) != count or fingerprint(body) != expected:
            raise ValueError(f'Mac injury-daily contract changed or missing at {start:x} page {page:x}')


def require_injury_daily_index(db, selected=None):
    records = [json.loads(row[0]) for start, end, _, _ in CONTRACTS for row in db.execute(
        'SELECT body FROM instructions WHERE address>=? AND address<? ORDER BY address', (start, end))]
    if selected is None: require_injury_daily_bodies(records, complete=True)
    else:
        for page in sorted({f[1] for f in FRAGMENTS}):
            require_injury_daily_contract(page, [r for _, r in sorted(selected.items())], selected)
    require_injury_daily_edges(db.execute("""SELECT i.address,e.value
        FROM instructions AS i,
          json_each(json_insert(json_extract(i.body,'$.flows'),'$[#]',json_extract(i.body,'$.fallthrough'))) AS e
        WHERE e.value>=? AND e.value<?""", (CONTRACTS[0][0], CONTRACTS[-1][1])))


def injury_daily_hook(entry):
    return f'if (try_modern_injury_daily(s, 0x{entry:x})) return true;'


def require_injury_daily_hooks(page, instructions, code):
    addresses = {r['address'] for r in instructions}
    for entry in INJURY_DAILY_ENTRIES:
        if entry >> 12 != page or entry not in addresses: continue
        label, hook = f'i_{entry:x}:', injury_daily_hook(entry)
        fragment = code.split(label, 1)[-1].split(f'i_{entry + 4:x}:', 1)[0]
        steps = [token for token in (f's.step(0x{entry:x});', f'counter.step(0x{entry:x});') if token in fragment]
        if code.count(hook) != 1 or hook not in fragment or len(steps) != 1 or fragment.index(hook) > fragment.index(steps[0]):
            raise ValueError(f'Missing or misplaced injury-daily hook at {entry:x}')


TABLES = ((277566912, 277658808, 36, 'f251cb28bdb36b9aa1005af30eb86f639c95fbd34ea1e1428bf77dc5020553d5'), (277571156, 276560320, 112, '96ac90b9e6d61e2a7dfb2c3f8fbd64fd612adccb0b63db1364ca15b5a36d148c'), (277571152, 277658844, 196, 'bb7b90b24a0e2e29420a730a4bfc5d9c52c1438b4c3ad06d865d9b818bfce64a'))

def require_injury_daily_tables(word):
    import struct
    for slot,base,size,expected in TABLES:
        data=b''.join(struct.pack('>I',word(base+n)) for n in range(0,size,4))
        if word(slot)!=base or hashlib.sha256(data).hexdigest()!=expected:
            raise ValueError('Changed daily injury processing table')

def load_injury_daily_tables(input_directory):
    import struct
    program=json.loads((input_directory/'program.json').read_text())
    segments=[(s['address'],(input_directory/s['file']).read_bytes()) for s in program['segments'] if s.get('file')]
    def word(address):
        for base,data in segments:
            if base<=address and address+4<=base+len(data):return struct.unpack_from('>I',data,address-base)[0]
        raise ValueError('Missing daily injury processing table word')
    require_injury_daily_tables(word)
    return tuple(tuple(word(base+n) for n in range(0,size,4)) for _,base,size,_ in TABLES)
