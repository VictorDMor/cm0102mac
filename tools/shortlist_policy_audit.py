"""Pinned complete recruitment offer, country and row policy operations."""
import hashlib
import json
import struct

CONTRACTS = ((272036208, 272037256, 262, '20ea89b64d9ba47c6b31b3a9e487822ae76aea39bde1c942167dd4f0a1e1fa47'), (272037264, 272037476, 53, '69d35e7d869c706e8ffb380847d8edb081b8046c4fdf874728c97d569360789e'), (272038896, 272039680, 196, 'ddc850be9e58640a184b35dbd5a6a2b00d1d1a282cb529e73169a04679df356e'))
FRAGMENTS = ((272036208, 66415, 262, '20ea89b64d9ba47c6b31b3a9e487822ae76aea39bde1c942167dd4f0a1e1fa47'), (272037264, 66415, 53, '69d35e7d869c706e8ffb380847d8edb081b8046c4fdf874728c97d569360789e'), (272038896, 66415, 196, 'ddc850be9e58640a184b35dbd5a6a2b00d1d1a282cb529e73169a04679df356e'))
SHORTLIST_POLICY_ENTRIES = tuple(start for start, _, _, _ in CONTRACTS)
PREPARED_SHORTLIST_POLICY_ENTRIES = SHORTLIST_POLICY_ENTRIES


def fingerprint(body):
    canonical = [{key: r.get(key) for key in ('address', 'assembly', 'ops', 'flows', 'fallthrough')} for r in body]
    return hashlib.sha256(json.dumps(canonical, sort_keys=True, separators=(',', ':')).encode()).hexdigest()


def require_shortlist_policy_bodies(instructions, complete=False):
    for start, end, count, expected in CONTRACTS:
        body = [r for r in instructions if start <= r['address'] < end]
        if not body and not complete: continue
        if len(body) != count or fingerprint(body) != expected:
            raise ValueError(f'Mac shortlist-policy contract changed or missing at {start:x}')


def require_shortlist_policy_edges(edges):
    for source, target in edges:
        for start, end, _, _ in CONTRACTS:
            if start < target < end and not start <= source < end:
                raise ValueError(f'External shortlist-policy interior edge from {source:x} to {target:x}')


def require_shortlist_policy_contract(page, instructions, selected=None):
    addresses = {r['address'] for r in instructions} | set(selected or ())
    for start, end, _, _ in CONTRACTS:
        if not start >> 12 <= page <= (end - 1) >> 12: continue
        low, high = max(start, page << 12), min(end, (page + 1) << 12)
        if start not in addresses and not any(low <= a < high for a in addresses): continue
        body = [r for r in instructions if low <= r['address'] < high]
        _, _, count, expected = next(f for f in FRAGMENTS if f[0] == start and f[1] == page)
        if len(body) != count or fingerprint(body) != expected:
            raise ValueError(f'Mac shortlist-policy contract changed or missing at {start:x} page {page:x}')


def require_shortlist_policy_index(db, selected=None):
    records = [json.loads(row[0]) for start, end, _, _ in CONTRACTS for row in db.execute(
        'SELECT body FROM instructions WHERE address>=? AND address<? ORDER BY address', (start, end))]
    if selected is None: require_shortlist_policy_bodies(records, complete=True)
    else:
        for page in sorted({f[1] for f in FRAGMENTS}):
            require_shortlist_policy_contract(page, [r for _, r in sorted(selected.items())], selected)
    conditions = ' OR '.join('(e.value>=? AND e.value<?)' for _ in CONTRACTS)
    parameters = [value for start, end, _, _ in CONTRACTS for value in (start, end)]
    require_shortlist_policy_edges(db.execute("""SELECT i.address,e.value
        FROM instructions AS i,
          json_each(json_insert(json_extract(i.body,'$.flows'),'$[#]',json_extract(i.body,'$.fallthrough'))) AS e
        WHERE """ + conditions, parameters))


TABLES = ((277572788, 276871384, 728, '35829e19e1516c30112da0b3c91006434866179ef7dab5e8941e9c992b7f7e32'),)

def require_shortlist_policy_tables(read):
    for slot,base,size,expected in TABLES:
        if struct.unpack(">I",read(slot,4))[0]!=base or hashlib.sha256(read(base,size)).hexdigest()!=expected:
            raise ValueError("Changed original shortlist policy table")

def load_shortlist_policy_tables(directory):
    program=json.loads((directory/"program.json").read_text())
    segments=[(s["address"],(directory/s["file"]).read_bytes()) for s in program["segments"] if s.get("file")]
    def read(a,n):
        for base,data in segments:
            if base<=a and a+n<=base+len(data):return data[a-base:a-base+n]
        raise ValueError("Missing original shortlist-policy table")
    require_shortlist_policy_tables(read)
    require_shortlist_policy_padding(read)
    return tuple(read(base,size) for _,base,size,_ in TABLES)


PADDING = ((272037256, '00000000'), (272037260, '00000000'), (272037476, '00000000'), (272037480, '00000000'), (272037484, '00000000'))

def require_shortlist_policy_padding(read):
    for address,expected in PADDING:
        if read(address,4)!=bytes.fromhex(expected):
            raise ValueError("Changed nonindexed policy alignment word")


def shortlist_policy_hook(entry):
    return f'if (try_modern_shortlist_policy(s, 0x{entry:x})) return true;'


def require_shortlist_policy_hooks(page, instructions, code, taken_over=()):
    """Each entry has its hook, except those another family took over."""
    addresses = {r['address'] for r in instructions}
    for entry in SHORTLIST_POLICY_ENTRIES:
        if entry >> 12 != page or entry not in addresses or entry in taken_over: continue
        label, hook = f'i_{entry:x}:', shortlist_policy_hook(entry)
        fragment = code.split(label, 1)[-1].split(f'i_{entry + 4:x}:', 1)[0]
        steps = [token for token in (f's.step(0x{entry:x});', f'counter.step(0x{entry:x});') if token in fragment]
        if code.count(hook) != 1 or hook not in fragment or len(steps) != 1 or fragment.index(hook) > fragment.index(steps[0]):
            raise ValueError(f'Missing or misplaced shortlist-policy hook at {entry:x}')
