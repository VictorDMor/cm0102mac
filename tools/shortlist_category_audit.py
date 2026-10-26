"""Pinned complete shortlist category, query and pair-comparison operations."""
import hashlib
import json
import struct

CONTRACTS = ((271998224, 271999140, 229, 'dd64ac3fb73cd19fbc651e9eb619aae5cf1b8a39dffec1fa2f38dfed3174922b'), (271999152, 271999568, 103, '511130009fc02c8e4b989dd3acef52dfb393c350f3dec9b70a84c0b85aac43fe'), (271999568, 271999920, 87, '250c5458814b481c795dd306c72c4cb1105906dd0fec006687590d532c7b49c8'), (271999920, 272000016, 24, '27caaa9f8f3ce6d424323ffe59758abc3336527e7935e2961e7fd4c5555e789b'), (272000016, 272000112, 24, 'b72880344108baac80bcf1a252488a0e8200c7ae0d0e3810f2225a491cda6cc8'))
FRAGMENTS = ((271998224, 66405, 188, '65467c8d2bbb6e2a470cade3c33942e95110c52450e824e905850417de45f040'), (271998224, 66406, 41, 'aa29c61808c7e71eed0f6adb4850ee58f5a7b10af66bea46fd009891e1ef45c5'), (271999152, 66406, 103, '511130009fc02c8e4b989dd3acef52dfb393c350f3dec9b70a84c0b85aac43fe'), (271999568, 66406, 87, '250c5458814b481c795dd306c72c4cb1105906dd0fec006687590d532c7b49c8'), (271999920, 66406, 24, '27caaa9f8f3ce6d424323ffe59758abc3336527e7935e2961e7fd4c5555e789b'), (272000016, 66406, 24, 'b72880344108baac80bcf1a252488a0e8200c7ae0d0e3810f2225a491cda6cc8'))
SHORTLIST_CATEGORY_ENTRIES = tuple(start for start, _, _, _ in CONTRACTS)
PREPARED_SHORTLIST_CATEGORY_ENTRIES = SHORTLIST_CATEGORY_ENTRIES


def fingerprint(body):
    canonical = [{key: r.get(key) for key in ('address', 'assembly', 'ops', 'flows', 'fallthrough')} for r in body]
    return hashlib.sha256(json.dumps(canonical, sort_keys=True, separators=(',', ':')).encode()).hexdigest()


def require_shortlist_category_bodies(instructions, complete=False):
    for start, end, count, expected in CONTRACTS:
        body = [r for r in instructions if start <= r['address'] < end]
        if not body and not complete: continue
        if len(body) != count or fingerprint(body) != expected:
            raise ValueError(f'Mac shortlist-category contract changed or missing at {start:x}')


def require_shortlist_category_edges(edges):
    for source, target in edges:
        for start, end, _, _ in CONTRACTS:
            if start < target < end and not start <= source < end:
                raise ValueError(f'External shortlist-category interior edge from {source:x} to {target:x}')


def require_shortlist_category_contract(page, instructions, selected=None):
    addresses = {r['address'] for r in instructions} | set(selected or ())
    for start, end, _, _ in CONTRACTS:
        if not start >> 12 <= page <= (end - 1) >> 12: continue
        low, high = max(start, page << 12), min(end, (page + 1) << 12)
        if start not in addresses and not any(low <= a < high for a in addresses): continue
        body = [r for r in instructions if low <= r['address'] < high]
        _, _, count, expected = next(f for f in FRAGMENTS if f[0] == start and f[1] == page)
        if len(body) != count or fingerprint(body) != expected:
            raise ValueError(f'Mac shortlist-category contract changed or missing at {start:x} page {page:x}')


def require_shortlist_category_index(db, selected=None):
    records = [json.loads(row[0]) for start, end, _, _ in CONTRACTS for row in db.execute(
        'SELECT body FROM instructions WHERE address>=? AND address<? ORDER BY address', (start, end))]
    if selected is None: require_shortlist_category_bodies(records, complete=True)
    else:
        for page in sorted({f[1] for f in FRAGMENTS}):
            require_shortlist_category_contract(page, [r for _, r in sorted(selected.items())], selected)
    conditions = ' OR '.join('(e.value>=? AND e.value<?)' for _ in CONTRACTS)
    parameters = [value for start, end, _, _ in CONTRACTS for value in (start, end)]
    require_shortlist_category_edges(db.execute("""SELECT i.address,e.value
        FROM instructions AS i,
          json_each(json_insert(json_extract(i.body,'$.flows'),'$[#]',json_extract(i.body,'$.fallthrough'))) AS e
        WHERE """ + conditions, parameters))


TABLES = ((0x108b6cb8,0x1080b64c,41,'ce07ac15ec347dbdfdd04f996404bc24d13385e5b0400df475d23959b9d5d4b7'),(0x108b6c9c,0x108ceecc,64,'61396f578ecb57e4d131b30a94cc88199cf90776438e6716f274ddf5037e50a5'))

def require_shortlist_category_tables(read):
    for slot,base,size,expected in TABLES:
        if struct.unpack(">I",read(slot,4))[0]!=base or hashlib.sha256(read(base,size)).hexdigest()!=expected:
            raise ValueError("Changed original shortlist category table")

def load_shortlist_category_tables(directory):
    program=json.loads((directory/"program.json").read_text())
    segments=[(s["address"],(directory/s["file"]).read_bytes()) for s in program["segments"] if s.get("file")]
    def read(a,n):
        for base,data in segments:
            if base<=a and a+n<=base+len(data):return data[a-base:a-base+n]
        raise ValueError("Missing original shortlist-category table")
    require_shortlist_category_tables(read)
    require_shortlist_category_padding(read)
    return tuple(read(base,size) for _,base,size,_ in TABLES)


PADDING = (0x103660f4,0x1036628c)

def require_shortlist_category_padding(read):
    for address in PADDING:
        if read(address,4)!=bytes.fromhex("60000000"):
            raise ValueError("Changed non-indexed shortlist padding")


def shortlist_category_hook(entry):
    return f'if (try_modern_shortlist_category(s, 0x{entry:x})) return true;'


def require_shortlist_category_hooks(page, instructions, code, taken_over=()):
    """Each entry has its hook, except those another family took over."""
    addresses = {r['address'] for r in instructions}
    for entry in SHORTLIST_CATEGORY_ENTRIES:
        if entry >> 12 != page or entry not in addresses or entry in taken_over: continue
        label, hook = f'i_{entry:x}:', shortlist_category_hook(entry)
        fragment = code.split(label, 1)[-1].split(f'i_{entry + 4:x}:', 1)[0]
        steps = [token for token in (f's.step(0x{entry:x});', f'counter.step(0x{entry:x});') if token in fragment]
        if code.count(hook) != 1 or hook not in fragment or len(steps) != 1 or fragment.index(hook) > fragment.index(steps[0]):
            raise ValueError(f'Missing or misplaced shortlist-category hook at {entry:x}')
