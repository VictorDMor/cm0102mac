"""Pinned complete injury physical maintenance boundaries."""
import hashlib
import json

CONTRACTS = ((270035488, 270036176, 172, '15d148ef58edd4b2371c0465eb825a14503d5a9809ad4898f1ba1c3c9cdccd11'), (270036176, 270036492, 78, 'f37de68c8d508f7542fd369dbd7f2b94998fee537c8269db0ea805b0d37f8a57'), (270036496, 270036628, 32, '0e43b4a707330fd55f6c2bbaae07146a5ea81be5f942215d1a3f99f8333bf8f2'), (270036640, 270036916, 69, 'b1ff37bf028514b2d2a29be693cd75a106d05454f282acf9f333a3eef9ea3d3b'), (270036928, 270038096, 292, 'd211eefe533981f76b63231f1f5de7ef85d9cf6d20c7ffba3c21644abfe014a0'))
FRAGMENTS = ((270035488, 65926, 172, '15d148ef58edd4b2371c0465eb825a14503d5a9809ad4898f1ba1c3c9cdccd11'), (270036176, 65926, 78, 'f37de68c8d508f7542fd369dbd7f2b94998fee537c8269db0ea805b0d37f8a57'), (270036496, 65926, 32, '0e43b4a707330fd55f6c2bbaae07146a5ea81be5f942215d1a3f99f8333bf8f2'), (270036640, 65926, 69, 'b1ff37bf028514b2d2a29be693cd75a106d05454f282acf9f333a3eef9ea3d3b'), (270036928, 65926, 16, '1258c0cfecb350f3434ef15e12a71d52809b619ef631971ed8d5b86f9ffb3c02'), (270036928, 65927, 276, '1c5c39ec07d88a67335f592a9970822cdd7f1a4a5df7b02f42eb040361457c55'))
INJURY_MAINTENANCE_ENTRIES = tuple(start for start, _, _, _ in CONTRACTS)


def fingerprint(body):
    canonical = [{key: r.get(key) for key in ('address', 'assembly', 'ops', 'flows', 'fallthrough')} for r in body]
    return hashlib.sha256(json.dumps(canonical, sort_keys=True, separators=(',', ':')).encode()).hexdigest()


def require_injury_maintenance_bodies(instructions, complete=False):
    for start, end, count, expected in CONTRACTS:
        body = [r for r in instructions if start <= r['address'] < end]
        if not body and not complete: continue
        if len(body) != count or fingerprint(body) != expected:
            raise ValueError(f'Mac injury-maintenance contract changed or missing at {start:x}')


def require_injury_maintenance_edges(edges):
    for source, target in edges:
        for start, end, _, _ in CONTRACTS:
            if start < target < end and not start <= source < end:
                raise ValueError(f'External injury-maintenance interior edge from {source:x} to {target:x}')


def require_injury_maintenance_contract(page, instructions, selected=None):
    addresses = {r['address'] for r in instructions} | set(selected or ())
    for start, end, _, _ in CONTRACTS:
        if not start >> 12 <= page <= (end - 1) >> 12: continue
        low, high = max(start, page << 12), min(end, (page + 1) << 12)
        if start not in addresses and not any(low <= a < high for a in addresses): continue
        body = [r for r in instructions if low <= r['address'] < high]
        _, _, count, expected = next(f for f in FRAGMENTS if f[0] == start and f[1] == page)
        if len(body) != count or fingerprint(body) != expected:
            raise ValueError(f'Mac injury-maintenance contract changed or missing at {start:x} page {page:x}')


def require_injury_maintenance_index(db, selected=None):
    records = [json.loads(row[0]) for start, end, _, _ in CONTRACTS for row in db.execute(
        'SELECT body FROM instructions WHERE address>=? AND address<? ORDER BY address', (start, end))]
    if selected is None: require_injury_maintenance_bodies(records, complete=True)
    else:
        for page in sorted({f[1] for f in FRAGMENTS}):
            require_injury_maintenance_contract(page, [r for _, r in sorted(selected.items())], selected)
    require_injury_maintenance_edges(db.execute("""SELECT i.address,e.value
        FROM instructions AS i,
          json_each(json_insert(json_extract(i.body,'$.flows'),'$[#]',json_extract(i.body,'$.fallthrough'))) AS e
        WHERE e.value>=? AND e.value<?""", (CONTRACTS[0][0], CONTRACTS[-1][1])))


def injury_maintenance_hook(entry):
    return f'if (try_modern_injury_maintenance(s, 0x{entry:x})) return true;'


def require_injury_maintenance_hooks(page, instructions, code):
    addresses = {r['address'] for r in instructions}
    for entry in INJURY_MAINTENANCE_ENTRIES:
        if entry >> 12 != page or entry not in addresses: continue
        label, hook = f'i_{entry:x}:', injury_maintenance_hook(entry)
        fragment = code.split(label, 1)[-1].split(f'i_{entry + 4:x}:', 1)[0]
        steps = [token for token in (f's.step(0x{entry:x});', f'counter.step(0x{entry:x});') if token in fragment]
        if code.count(hook) != 1 or hook not in fragment or len(steps) != 1 or fragment.index(hook) > fragment.index(steps[0]):
            raise ValueError(f'Missing or misplaced injury-maintenance hook at {entry:x}')


TABLES = ((277571156, 276560320, 40, 'b222f893a64d5df3f7a3a6d660d309a979f00a575e86476fbd1363ea8056a730'),)
PADDING = ((0x10186d24,0x60000000),(0x10186e4c,0x60000000))

def require_injury_maintenance_tables(word):
    import struct
    for address,expected in PADDING:
        if word(address)!=expected:raise ValueError('Changed unreachable injury maintenance padding')
    for slot,base,size,expected in TABLES:
        data=b''.join(struct.pack('>I',word(base+n)) for n in range(0,size,4))
        if word(slot)!=base or hashlib.sha256(data).hexdigest()!=expected:
            raise ValueError('Changed injury physical maintenance table')

def load_injury_maintenance_tables(input_directory):
    import struct
    program=json.loads((input_directory/'program.json').read_text())
    segments=[(s['address'],(input_directory/s['file']).read_bytes()) for s in program['segments'] if s.get('file')]
    def word(address):
        for base,data in segments:
            if base<=address and address+4<=base+len(data):return struct.unpack_from('>I',data,address-base)[0]
        raise ValueError('Missing injury physical maintenance table word')
    require_injury_maintenance_tables(word)
    return tuple(tuple(word(base+n) for n in range(0,size,4)) for _,base,size,_ in TABLES)
