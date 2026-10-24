"""Pinned complete profile qualification and category-reference operations."""
import hashlib
import json
import struct

CONTRACTS = ((269579104, 269579400, 73, '6dd78a9cf46bd4f98b0a2f57eea04ee4cc21f19c9fb5da321480cb319a0a2cbe'), (269579408, 269579912, 126, 'f90c4019dac33c646c761211fe450ed4c0bb5747d23e1f99629f2c7151c76c85'), (272037488, 272037608, 30, '55e124a9baea3b33541fd274de687e399639412d70add51658ffbed9af0bd80c'), (272037616, 272037848, 58, '995168cbae5caa1a25ef7856ac028328d338422d25969b17d784a9233970e8ce'))
FRAGMENTS = ((269579104, 65815, 73, '6dd78a9cf46bd4f98b0a2f57eea04ee4cc21f19c9fb5da321480cb319a0a2cbe'), (269579408, 65815, 126, 'f90c4019dac33c646c761211fe450ed4c0bb5747d23e1f99629f2c7151c76c85'), (272037488, 66415, 30, '55e124a9baea3b33541fd274de687e399639412d70add51658ffbed9af0bd80c'), (272037616, 66415, 58, '995168cbae5caa1a25ef7856ac028328d338422d25969b17d784a9233970e8ce'))
PROFILE_QUALIFICATION_ENTRIES = tuple(start for start, _, _, _ in CONTRACTS)
PREPARED_PROFILE_QUALIFICATION_ENTRIES = PROFILE_QUALIFICATION_ENTRIES


def fingerprint(body):
    canonical = [{key: r.get(key) for key in ('address', 'assembly', 'ops', 'flows', 'fallthrough')} for r in body]
    return hashlib.sha256(json.dumps(canonical, sort_keys=True, separators=(',', ':')).encode()).hexdigest()


def require_profile_qualification_bodies(instructions, complete=False):
    for start, end, count, expected in CONTRACTS:
        body = [r for r in instructions if start <= r['address'] < end]
        if not body and not complete: continue
        if len(body) != count or fingerprint(body) != expected:
            raise ValueError(f'Mac profile-qualification contract changed or missing at {start:x}')


def require_profile_qualification_edges(edges):
    for source, target in edges:
        for start, end, _, _ in CONTRACTS:
            if start < target < end and not start <= source < end:
                raise ValueError(f'External profile-qualification interior edge from {source:x} to {target:x}')


def require_profile_qualification_contract(page, instructions, selected=None):
    addresses = {r['address'] for r in instructions} | set(selected or ())
    for start, end, _, _ in CONTRACTS:
        if not start >> 12 <= page <= (end - 1) >> 12: continue
        low, high = max(start, page << 12), min(end, (page + 1) << 12)
        if start not in addresses and not any(low <= a < high for a in addresses): continue
        body = [r for r in instructions if low <= r['address'] < high]
        _, _, count, expected = next(f for f in FRAGMENTS if f[0] == start and f[1] == page)
        if len(body) != count or fingerprint(body) != expected:
            raise ValueError(f'Mac profile-qualification contract changed or missing at {start:x} page {page:x}')


def require_profile_qualification_index(db, selected=None):
    records = [json.loads(row[0]) for start, end, _, _ in CONTRACTS for row in db.execute(
        'SELECT body FROM instructions WHERE address>=? AND address<? ORDER BY address', (start, end))]
    if selected is None: require_profile_qualification_bodies(records, complete=True)
    else:
        for page in sorted({f[1] for f in FRAGMENTS}):
            require_profile_qualification_contract(page, [r for _, r in sorted(selected.items())], selected)
    conditions = ' OR '.join('(e.value>=? AND e.value<?)' for _ in CONTRACTS)
    parameters = [value for start, end, _, _ in CONTRACTS for value in (start, end)]
    require_profile_qualification_edges(db.execute("""SELECT i.address,e.value
        FROM instructions AS i,
          json_each(json_insert(json_extract(i.body,'$.flows'),'$[#]',json_extract(i.body,'$.fallthrough'))) AS e
        WHERE """ + conditions, parameters))


TABLES = ((277570928, 276458408, 16, 'fe3266793852b25e4ee5127e424fdaa67e084d4bbcd2a3bb6df4487c05e335fb'), (277570924, 276458424, 12, '80f8830b9c5e1dc2e85faf50d9fd451410bced11866b1a58955ee02ad85e3e03'), (277570948, 276458540, 47, '87e53113372977822dd01beda641355cf1371a4d2b430559c6023513cbc129d5'))

def require_profile_qualification_tables(read):
    for slot,base,size,expected in TABLES:
        if struct.unpack(">I",read(slot,4))[0]!=base or hashlib.sha256(read(base,size)).hexdigest()!=expected:
            raise ValueError("Changed original profile qualification table")

def load_profile_qualification_tables(directory):
    program=json.loads((directory/"program.json").read_text())
    segments=[(s["address"],(directory/s["file"]).read_bytes()) for s in program["segments"] if s.get("file")]
    def read(a,n):
        for base,data in segments:
            if base<=a and a+n<=base+len(data):return data[a-base:a-base+n]
        raise ValueError("Missing original profile-qualification table")
    require_profile_qualification_tables(read)
    require_profile_qualification_padding(read)
    return tuple(read(base,size) for _,base,size,_ in TABLES)


PADDING = ((269579268, '60000000'), (269579400, '00000000'), (269579404, '00000000'), (269579912, '00000000'), (269579916, '00000000'), (272037608, '00000000'), (272037612, '00000000'), (272037848, '00000000'), (272037852, '00000000'))

def require_profile_qualification_padding(read):
    for address,expected in PADDING:
        if read(address,4)!=bytes.fromhex(expected):
            raise ValueError("Changed nonindexed profile qualification word")


def profile_qualification_hook(entry):
    return f'if (try_modern_profile_qualification(s, 0x{entry:x})) return true;'


def require_profile_qualification_hooks(page, instructions, code, taken_over=()):
    """Each entry has its hook, except those another family took over."""
    addresses = {r['address'] for r in instructions}
    for entry in PROFILE_QUALIFICATION_ENTRIES:
        if entry >> 12 != page or entry not in addresses or entry in taken_over: continue
        label, hook = f'i_{entry:x}:', profile_qualification_hook(entry)
        fragment = code.split(label, 1)[-1].split(f'i_{entry + 4:x}:', 1)[0]
        steps = [token for token in (f's.step(0x{entry:x});', f'counter.step(0x{entry:x});') if token in fragment]
        if code.count(hook) != 1 or hook not in fragment or len(steps) != 1 or fragment.index(hook) > fragment.index(steps[0]):
            raise ValueError(f'Missing or misplaced profile-qualification hook at {entry:x}')
