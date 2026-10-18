"""Pinned complete shortlist recruitment distribution and registration operations."""
import hashlib
import json
import struct

CONTRACTS = ((271989920, 271990620, 174, 'c5bf309015fa86dad98723ab3a3ad82bbd505c853cc2978baaa951150d6e9601'), (271990624, 271994112, 871, 'f8c5f3f9c37d3366ebcfdd952b26723f1d12b943994300a76cdfe57c90f03156'), (271994112, 271996296, 546, '14e33e9df9dc0d80c588f0a8e9a7a12156e577d8becf5e00ec3d95844932e6ca'))
FRAGMENTS = ((271989920, 66403, 174, 'c5bf309015fa86dad98723ab3a3ad82bbd505c853cc2978baaa951150d6e9601'), (271990624, 66403, 40, '62c3cb8d6f8cf545844f5ec151ba036d6006c364d97e8b594f60d7979b61e4d8'), (271990624, 66404, 831, '54d417d82cd4b8542f01d970b0e23c746761e537949b121bc21537034c17afea'), (271994112, 66404, 192, '805b7b91d9a6388618bc0491987dd321e0fa06f1c06dcbf8c2bd212eab50cb81'), (271994112, 66405, 354, '563469e20d5fa85856561e1c9aa023f9f577088618eaeaf40c7be7a1d94d0cdc'))
SHORTLIST_RECRUITMENT_ENTRIES = tuple(start for start, _, _, _ in CONTRACTS)
PREPARED_SHORTLIST_RECRUITMENT_ENTRIES = SHORTLIST_RECRUITMENT_ENTRIES


def fingerprint(body):
    canonical = [{key: r.get(key) for key in ('address', 'assembly', 'ops', 'flows', 'fallthrough')} for r in body]
    return hashlib.sha256(json.dumps(canonical, sort_keys=True, separators=(',', ':')).encode()).hexdigest()


def require_shortlist_recruitment_bodies(instructions, complete=False):
    for start, end, count, expected in CONTRACTS:
        body = [r for r in instructions if start <= r['address'] < end]
        if not body and not complete: continue
        if len(body) != count or fingerprint(body) != expected:
            raise ValueError(f'Mac shortlist-recruitment contract changed or missing at {start:x}')


def require_shortlist_recruitment_edges(edges):
    for source, target in edges:
        for start, end, _, _ in CONTRACTS:
            if start < target < end and not start <= source < end:
                raise ValueError(f'External shortlist-recruitment interior edge from {source:x} to {target:x}')


def require_shortlist_recruitment_contract(page, instructions, selected=None):
    addresses = {r['address'] for r in instructions} | set(selected or ())
    for start, end, _, _ in CONTRACTS:
        if not start >> 12 <= page <= (end - 1) >> 12: continue
        low, high = max(start, page << 12), min(end, (page + 1) << 12)
        if start not in addresses and not any(low <= a < high for a in addresses): continue
        body = [r for r in instructions if low <= r['address'] < high]
        _, _, count, expected = next(f for f in FRAGMENTS if f[0] == start and f[1] == page)
        if len(body) != count or fingerprint(body) != expected:
            raise ValueError(f'Mac shortlist-recruitment contract changed or missing at {start:x} page {page:x}')


def require_shortlist_recruitment_index(db, selected=None):
    records = [json.loads(row[0]) for start, end, _, _ in CONTRACTS for row in db.execute(
        'SELECT body FROM instructions WHERE address>=? AND address<? ORDER BY address', (start, end))]
    if selected is None: require_shortlist_recruitment_bodies(records, complete=True)
    else:
        for page in sorted({f[1] for f in FRAGMENTS}):
            require_shortlist_recruitment_contract(page, [r for _, r in sorted(selected.items())], selected)
    conditions = ' OR '.join('(e.value>=? AND e.value<?)' for _ in CONTRACTS)
    parameters = [value for start, end, _, _ in CONTRACTS for value in (start, end)]
    require_shortlist_recruitment_edges(db.execute("""SELECT i.address,e.value
        FROM instructions AS i,
          json_each(json_insert(json_extract(i.body,'$.flows'),'$[#]',json_extract(i.body,'$.fallthrough'))) AS e
        WHERE """ + conditions, parameters))


TABLES = ((277572788, 276871384, 720, 'ff1e88ea55f212796f6a3e936d5697398e91538d12e6a01e8a76161a6622d322'), (277572776, 276871340, 12, 'd46db81a246a59f3ecdd20b7efc2a3b7c74c6e53c06b553e15013db31526bf4d'), (277567464, 277582348, 8, '27cc863287f340c6fc75f1da988cca5cef8e2abff48ca0bf2150e4047753576f'))

def require_shortlist_recruitment_tables(read):
    for slot,base,size,expected in TABLES:
        if struct.unpack(">I",read(slot,4))[0]!=base or hashlib.sha256(read(base,size)).hexdigest()!=expected:
            raise ValueError("Changed original shortlist recruitment table")

def load_shortlist_recruitment_tables(directory):
    program=json.loads((directory/"program.json").read_text())
    segments=[(s["address"],(directory/s["file"]).read_bytes()) for s in program["segments"] if s.get("file")]
    def read(a,n):
        for base,data in segments:
            if base<=a and a+n<=base+len(data):return data[a-base:a-base+n]
        raise ValueError("Missing original shortlist-recruitment table")
    require_shortlist_recruitment_tables(read)
    require_shortlist_recruitment_padding(read)
    return tuple(read(base,size) for _,base,size,_ in TABLES)


PADDING = ((271990480, '60000000'), (271990620, '00000000'), (271994052, '60000000'), (271996296, '00000000'), (271996300, '00000000'))

def require_shortlist_recruitment_padding(read):
    for address,expected in PADDING:
        if read(address,4)!=bytes.fromhex(expected):
            raise ValueError("Changed nonindexed recruitment alignment word")
