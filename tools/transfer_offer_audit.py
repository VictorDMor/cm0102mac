"""Pinned complete transfer-offer query, membership and rejection-policy scope."""
import hashlib
import json
import struct

CONTRACTS = ((272523920, 272524120, 50, '6497df71652786d3712c9799e928091daebadc7664b6ca7fbd3c135c7b9c346b'), (272524128, 272524388, 64, '5bb88ae88ea4d4018228f3df461d796ed1bfb36ad9d26c990a2cc1865beabb38'), (272524400, 272524508, 27, '9e5018171713fcb2ea418fd2e98d1a84439dcd7195c7c417bf67b5e8756580a3'), (272540240, 272540268, 7, '135204275680f90fe6a81f08510c23b26ff41f4e6720a0fbcb96ed6a089b88b0'), (272540272, 272540300, 7, '173a6577d2c13127b49225d6170694f8c48cbe8e2790566b0b16fed4f167ace5'), (272540304, 272540488, 46, '6f5be85f20e7d032d3696320ef2a023e786f806012f617ed8fb43cedefdecbb3'), (272540496, 272540732, 59, '630a083b4d251788aea3fe9fa89d8416fc85bd85be24ca394b33a133a6aab9d1'), (272540736, 272541176, 110, 'ba0c780c6681d69078cec198a51c18c1e09a335622e6050bd414179ab5421634'), (272541184, 272541532, 87, 'c0cbb62da4cb301b5e7d2b9db069fea9112aa45dd8af5755c9d5e76a2ba6cb0e'), (272541536, 272541756, 55, '3e5077c0d0d74341009e637fee0750d487779b239fedd24aad4750b05b66dfa8'), (272543392, 272543764, 93, 'c66e428379167145cd2dd9e66e9d9efd5e9afed00ab4b7b65d844ae8707100d6'), (272543776, 272544228, 113, '41655f93df9a819273d2c45733f0a36b3a9ce7196bca3b08854663dafc354a73'), (272544240, 272544600, 89, '690ac3d4ae0e29d9f6351831a038412352281c96ec06674e2ece32502d8e7c6e'), (272544608, 272544820, 53, '887f9747eb48836aa911a244a119a52650cd8c9975dac3b62dadebd133c13640'), (272544832, 272544868, 9, '25c109cd48ce3764fc43f3fc2701f9fc9501de61a3e99b8cdcad4cfd0a937dad'), (272544880, 272544916, 9, '4382d3f46b6caecd10aa9e3211c2c03749b8bade1c9fe55f8ff2209709cc7331'), (272581184, 272581540, 88, 'd6e8610097405768b6098af52f45582c50f537ebc831a29a31ad724c5ba7b93f'), (272581552, 272582036, 119, '57fdb7797f40fb532b4740e8a253720ed7c9bec00abebd2d7b60ecb93a7e393d'), (272610192, 272610296, 26, 'a5d437a9180f5c0e70c90d5215275863cf4201b915764686609a0fcc9e45dfed'))
FRAGMENTS = ((272523920, 66534, 50, '6497df71652786d3712c9799e928091daebadc7664b6ca7fbd3c135c7b9c346b'), (272524128, 66534, 64, '5bb88ae88ea4d4018228f3df461d796ed1bfb36ad9d26c990a2cc1865beabb38'), (272524400, 66534, 27, '9e5018171713fcb2ea418fd2e98d1a84439dcd7195c7c417bf67b5e8756580a3'), (272540240, 66538, 7, '135204275680f90fe6a81f08510c23b26ff41f4e6720a0fbcb96ed6a089b88b0'), (272540272, 66538, 7, '173a6577d2c13127b49225d6170694f8c48cbe8e2790566b0b16fed4f167ace5'), (272540304, 66538, 46, '6f5be85f20e7d032d3696320ef2a023e786f806012f617ed8fb43cedefdecbb3'), (272540496, 66538, 59, '630a083b4d251788aea3fe9fa89d8416fc85bd85be24ca394b33a133a6aab9d1'), (272540736, 66538, 110, 'ba0c780c6681d69078cec198a51c18c1e09a335622e6050bd414179ab5421634'), (272541184, 66538, 87, 'c0cbb62da4cb301b5e7d2b9db069fea9112aa45dd8af5755c9d5e76a2ba6cb0e'), (272541536, 66538, 55, '3e5077c0d0d74341009e637fee0750d487779b239fedd24aad4750b05b66dfa8'), (272543392, 66538, 88, 'e536edd504b6981cee7a164d4cc8c50789a5ab7419494ab1f9bc151d9092df08'), (272543392, 66539, 5, '5b9324355fbc9cc339b95c7f87660c1848542f8f89842f07988bf9f933984ff1'), (272543776, 66539, 113, '41655f93df9a819273d2c45733f0a36b3a9ce7196bca3b08854663dafc354a73'), (272544240, 66539, 89, '690ac3d4ae0e29d9f6351831a038412352281c96ec06674e2ece32502d8e7c6e'), (272544608, 66539, 53, '887f9747eb48836aa911a244a119a52650cd8c9975dac3b62dadebd133c13640'), (272544832, 66539, 9, '25c109cd48ce3764fc43f3fc2701f9fc9501de61a3e99b8cdcad4cfd0a937dad'), (272544880, 66539, 9, '4382d3f46b6caecd10aa9e3211c2c03749b8bade1c9fe55f8ff2209709cc7331'), (272581184, 66548, 88, 'd6e8610097405768b6098af52f45582c50f537ebc831a29a31ad724c5ba7b93f'), (272581552, 66548, 119, '57fdb7797f40fb532b4740e8a253720ed7c9bec00abebd2d7b60ecb93a7e393d'), (272610192, 66555, 26, 'a5d437a9180f5c0e70c90d5215275863cf4201b915764686609a0fcc9e45dfed'))
TRANSFER_OFFER_ENTRIES = tuple(start for start, _, _, _ in CONTRACTS)
PREPARED_TRANSFER_OFFER_ENTRIES = TRANSFER_OFFER_ENTRIES


def fingerprint(body):
    canonical = [{key: r.get(key) for key in ('address', 'assembly', 'ops', 'flows', 'fallthrough')} for r in body]
    return hashlib.sha256(json.dumps(canonical, sort_keys=True, separators=(',', ':')).encode()).hexdigest()


def require_transfer_offer_bodies(instructions, complete=False):
    for start, end, count, expected in CONTRACTS:
        body = [r for r in instructions if start <= r['address'] < end]
        if not body and not complete: continue
        if len(body) != count or fingerprint(body) != expected:
            raise ValueError(f'Mac transfer-offer contract changed or missing at {start:x}')


def require_transfer_offer_edges(edges):
    for source, target in edges:
        for start, end, _, _ in CONTRACTS:
            if start < target < end and not start <= source < end:
                raise ValueError(f'External transfer-offer interior edge from {source:x} to {target:x}')


def require_transfer_offer_contract(page, instructions, selected=None):
    addresses = {r['address'] for r in instructions} | set(selected or ())
    for start, end, _, _ in CONTRACTS:
        if not start >> 12 <= page <= (end - 1) >> 12: continue
        low, high = max(start, page << 12), min(end, (page + 1) << 12)
        if start not in addresses and not any(low <= a < high for a in addresses): continue
        body = [r for r in instructions if low <= r['address'] < high]
        _, _, count, expected = next(f for f in FRAGMENTS if f[0] == start and f[1] == page)
        if len(body) != count or fingerprint(body) != expected:
            raise ValueError(f'Mac transfer-offer contract changed or missing at {start:x} page {page:x}')


def require_transfer_offer_index(db, selected=None):
    records = [json.loads(row[0]) for start, end, _, _ in CONTRACTS for row in db.execute(
        'SELECT body FROM instructions WHERE address>=? AND address<? ORDER BY address', (start, end))]
    if selected is None: require_transfer_offer_bodies(records, complete=True)
    else:
        for page in sorted({f[1] for f in FRAGMENTS}):
            require_transfer_offer_contract(page, [r for _, r in sorted(selected.items())], selected)
    conditions = ' OR '.join('(e.value>=? AND e.value<?)' for _ in CONTRACTS)
    parameters = [value for start, end, _, _ in CONTRACTS for value in (start, end)]
    require_transfer_offer_edges(db.execute("""SELECT i.address,e.value
        FROM instructions AS i,
          json_each(json_insert(json_extract(i.body,'$.flows'),'$[#]',json_extract(i.body,'$.fallthrough'))) AS e
        WHERE """ + conditions, parameters))


TABLES = ((277573364, 276914832, 6630, '665a908ee160bf60ff35ea03090d9642047ed1f0ed25be0910f362ed5788b33b'),)

def require_transfer_offer_tables(read):
    for slot,base,size,expected in TABLES:
        if struct.unpack(">I",read(slot,4))[0]!=base or hashlib.sha256(read(base,size)).hexdigest()!=expected:
            raise ValueError("Changed original transfer offer table")

def load_transfer_offer_tables(directory):
    program=json.loads((directory/"program.json").read_text())
    segments=[(s["address"],(directory/s["file"]).read_bytes()) for s in program["segments"] if s.get("file")]
    def read(a,n):
        for base,data in segments:
            if base<=a and a+n<=base+len(data):return data[a-base:a-base+n]
        raise ValueError("Missing original transfer-offer table")
    require_transfer_offer_tables(read)
    return tuple(read(base,size) for _,base,size,_ in TABLES)
