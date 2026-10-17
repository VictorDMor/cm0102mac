"""Pinned complete injury reporting and history boundaries."""
import hashlib
import json

CONTRACTS = (
    (0x1018f340, 0x1018f468, 0x4a, '305f73466f1e560aa437985b204899fd6a712f8233aed9d9f25236b2a1628c0a'),
    (0x1018f470, 0x1018fbd8, 0x1da, 'b556bb604b7614f9b68a774fc82d6ee7dcdc5730e9178e86686d94a88ddd85cd'),
    (0x1018fbe0, 0x1018fdf0, 0x84, 'd926f8e81c0b264ea588cfa225cfb63959c7171910a796eeb4694c48d9a80704'),
    (0x1018fdf0, 0x1018ff04, 0x45, '2d00718d97ba1b6fb150abc8e9ca12b1a3e38863718411441ef31b0fddd06b6f'),
)
FRAGMENTS = (
    (0x1018f340, 0x1018f, 0x4a, '305f73466f1e560aa437985b204899fd6a712f8233aed9d9f25236b2a1628c0a'),
    (0x1018f470, 0x1018f, 0x1da, 'b556bb604b7614f9b68a774fc82d6ee7dcdc5730e9178e86686d94a88ddd85cd'),
    (0x1018fbe0, 0x1018f, 0x84, 'd926f8e81c0b264ea588cfa225cfb63959c7171910a796eeb4694c48d9a80704'),
    (0x1018fdf0, 0x1018f, 0x45, '2d00718d97ba1b6fb150abc8e9ca12b1a3e38863718411441ef31b0fddd06b6f'),
)
INJURY_REPORTING_ENTRIES = tuple(start for start, _, _, _ in CONTRACTS)


def fingerprint(body):
    canonical = [{key: r.get(key) for key in ('address', 'assembly', 'ops', 'flows', 'fallthrough')} for r in body]
    return hashlib.sha256(json.dumps(canonical, sort_keys=True, separators=(',', ':')).encode()).hexdigest()


def require_injury_reporting_bodies(instructions, complete=False):
    for start, end, count, expected in CONTRACTS:
        body = [r for r in instructions if start <= r['address'] < end]
        if not body and not complete: continue
        if len(body) != count or fingerprint(body) != expected:
            raise ValueError(f'Mac injury-reporting contract changed or missing at {start:x}')


def require_injury_reporting_edges(edges):
    for source, target in edges:
        for start, end, _, _ in CONTRACTS:
            if start < target < end and not start <= source < end:
                raise ValueError(f'External injury-reporting interior edge from {source:x} to {target:x}')


def require_injury_reporting_contract(page, instructions, selected=None):
    addresses = {r['address'] for r in instructions} | set(selected or ())
    for start, end, _, _ in CONTRACTS:
        if not start >> 12 <= page <= (end - 1) >> 12: continue
        low, high = max(start, page << 12), min(end, (page + 1) << 12)
        if start not in addresses and not any(low <= a < high for a in addresses): continue
        body = [r for r in instructions if low <= r['address'] < high]
        _, _, count, expected = next(f for f in FRAGMENTS if f[0] == start and f[1] == page)
        if len(body) != count or fingerprint(body) != expected:
            raise ValueError(f'Mac injury-reporting contract changed or missing at {start:x} page {page:x}')


def require_injury_reporting_index(db, selected=None):
    records = [json.loads(row[0]) for start, end, _, _ in CONTRACTS for row in db.execute(
        'SELECT body FROM instructions WHERE address>=? AND address<? ORDER BY address', (start, end))]
    if selected is None: require_injury_reporting_bodies(records, complete=True)
    else:
        for page in sorted({f[1] for f in FRAGMENTS}):
            require_injury_reporting_contract(page, [r for _, r in sorted(selected.items())], selected)
    require_injury_reporting_edges(db.execute("""SELECT i.address,e.value
        FROM instructions AS i,
          json_each(json_insert(json_extract(i.body,'$.flows'),'$[#]',json_extract(i.body,'$.fallthrough'))) AS e
        WHERE e.value>=? AND e.value<?""", (CONTRACTS[0][0], CONTRACTS[-1][1])))


def injury_reporting_hook(entry):
    return f'if (try_modern_injury_reporting(s, 0x{entry:x})) return true;'


def require_injury_reporting_hooks(page, instructions, code):
    addresses = {r['address'] for r in instructions}
    for entry in INJURY_REPORTING_ENTRIES:
        if entry >> 12 != page or entry not in addresses: continue
        label, hook = f'i_{entry:x}:', injury_reporting_hook(entry)
        fragment = code.split(label, 1)[-1].split(f'i_{entry + 4:x}:', 1)[0]
        steps = [token for token in (f's.step(0x{entry:x});', f'counter.step(0x{entry:x});') if token in fragment]
        if code.count(hook) != 1 or hook not in fragment or len(steps) != 1 or fragment.index(hook) > fragment.index(steps[0]):
            raise ValueError(f'Missing or misplaced injury-reporting hook at {entry:x}')


DETAIL_TABLE = (0x108b55bc,0x108cb8e0,246,'b248e4d53091f6e220e694dd4b053ca878eede63059a959e7ae52480e1f6be51')

def require_injury_reporting_tables(word):
    import struct
    slot,base,count,expected=DETAIL_TABLE
    if word(slot)!=base or hashlib.sha256(b''.join(struct.pack('>I',word(base+4*n)) for n in range(count))).hexdigest()!=expected:
        raise ValueError('Changed injury reporting detail table')

def load_injury_reporting_tables(input_directory):
    import struct
    program=json.loads((input_directory/'program.json').read_text())
    segments=[(s['address'],(input_directory/s['file']).read_bytes()) for s in program['segments'] if s.get('file')]
    def word(address):
        for base,data in segments:
            if base<=address and address+4<=base+len(data):return struct.unpack_from('>I',data,address-base)[0]
        raise ValueError('Missing injury reporting detail-table word')
    require_injury_reporting_tables(word)
    _,base,count,_=DETAIL_TABLE
    return tuple(word(base+4*n) for n in range(count))
