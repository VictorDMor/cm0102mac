"""Pinned complete contract review, status and presentation scope."""
import hashlib
import json
import struct

CONTRACTS = ((272101456, 272101648, 48, '3f0431a11dd4f7f0fcd8705b23981a28bfebbc0071069496a7d72cff6966427e'), (272101648, 272102792, 286, 'a2fe0b14fd027a842860aaa1a4143d21075a94c23794503fd4d6f28e5405fc4e'), (272102800, 272103364, 141, '322ce9ebab0a497988f9e7d3649cb41fa9f4276dcfddd03581d06a9a0b4ef312'), (272103376, 272103472, 24, 'f4061bec7cc2fc0850956b58fb7dabbb9dee70ae21bfb5a091e82c61c442056f'), (272103472, 272103576, 26, 'ebb19c4277743a3292e5d49e28c279cee6a180e5ecbaa029dbf19c3e6074ae4b'), (272103584, 272104008, 106, 'ed97e1c256bd645bc559044bad640be0102204408dcbb69490e57172edc2e84e'), (272104016, 272104032, 4, 'ada3b7020af16f143ed1d79c2a772ebfd70ac5380076d4f566a51a475b351a7c'), (272104032, 272104044, 3, 'b4c96e8177d1f09b168fcd6159ac279909f6d8155f59e5e50e6988c032a8e4d0'), (272104048, 272104056, 2, 'd0ced270d2b69a762ff392b53697169ef3cf00b73b3fbc9902dd4f82ecaf976c'), (272104064, 272106084, 505, 'ae527ed44d10d104984222a1a9c02188bd4b10fb1e521a6edfeb3905c84a3d72'), (272108560, 272108876, 79, '2d9b8c95f5d42c576c21bc84f5a2d3eb4bb800087a756ea09977bd2fb9e77a17'), (272108880, 272109340, 115, 'f439bc51ae52e311f384b263bae3a2131c3bf127f8fe67493217f81ea7e47776'), (272109344, 272109776, 108, 'd603f222e562d3a72bf5d1d044258433d3dd4167ab6009f6b8b7cd9084681803'))
FRAGMENTS = ((272101456, 66431, 48, '3f0431a11dd4f7f0fcd8705b23981a28bfebbc0071069496a7d72cff6966427e'), (272101648, 66431, 286, 'a2fe0b14fd027a842860aaa1a4143d21075a94c23794503fd4d6f28e5405fc4e'), (272102800, 66431, 141, '322ce9ebab0a497988f9e7d3649cb41fa9f4276dcfddd03581d06a9a0b4ef312'), (272103376, 66431, 24, 'f4061bec7cc2fc0850956b58fb7dabbb9dee70ae21bfb5a091e82c61c442056f'), (272103472, 66431, 26, 'ebb19c4277743a3292e5d49e28c279cee6a180e5ecbaa029dbf19c3e6074ae4b'), (272103584, 66431, 106, 'ed97e1c256bd645bc559044bad640be0102204408dcbb69490e57172edc2e84e'), (272104016, 66431, 4, 'ada3b7020af16f143ed1d79c2a772ebfd70ac5380076d4f566a51a475b351a7c'), (272104032, 66431, 3, 'b4c96e8177d1f09b168fcd6159ac279909f6d8155f59e5e50e6988c032a8e4d0'), (272104048, 66431, 2, 'd0ced270d2b69a762ff392b53697169ef3cf00b73b3fbc9902dd4f82ecaf976c'), (272104064, 66431, 352, 'c42a8b9232f60f3cafdd827ce3b8e305789db8b2abb1f88c1efb6e694336dc2a'), (272104064, 66432, 153, '078b066553a6f06ee91eea71f7fa0e248eb68ba6ea36768282b2b39ed0a1bd0f'), (272108560, 66432, 79, '2d9b8c95f5d42c576c21bc84f5a2d3eb4bb800087a756ea09977bd2fb9e77a17'), (272108880, 66432, 115, 'f439bc51ae52e311f384b263bae3a2131c3bf127f8fe67493217f81ea7e47776'), (272109344, 66432, 56, '2ac6a52712c044a0640c71a7291c27ce7957eb4258dcfa532004e7e7d859be1d'), (272109344, 66433, 52, '76fc5657627140d50db976685ee6735a613b8dacf6d876d9de2717758596119a'))
CONTRACT_REVIEW_ENTRIES = tuple(start for start, _, _, _ in CONTRACTS)
PREPARED_CONTRACT_REVIEW_ENTRIES = CONTRACT_REVIEW_ENTRIES


def fingerprint(body):
    canonical = [{key: r.get(key) for key in ('address', 'assembly', 'ops', 'flows', 'fallthrough')} for r in body]
    return hashlib.sha256(json.dumps(canonical, sort_keys=True, separators=(',', ':')).encode()).hexdigest()


def require_contract_review_bodies(instructions, complete=False):
    for start, end, count, expected in CONTRACTS:
        body = [r for r in instructions if start <= r['address'] < end]
        if not body and not complete: continue
        if len(body) != count or fingerprint(body) != expected:
            raise ValueError(f'Mac contract-review contract changed or missing at {start:x}')


def require_contract_review_edges(edges):
    for source, target in edges:
        for start, end, _, _ in CONTRACTS:
            if start < target < end and not start <= source < end:
                raise ValueError(f'External contract-review interior edge from {source:x} to {target:x}')


def require_contract_review_contract(page, instructions, selected=None):
    addresses = {r['address'] for r in instructions} | set(selected or ())
    for start, end, _, _ in CONTRACTS:
        if not start >> 12 <= page <= (end - 1) >> 12: continue
        low, high = max(start, page << 12), min(end, (page + 1) << 12)
        if start not in addresses and not any(low <= a < high for a in addresses): continue
        body = [r for r in instructions if low <= r['address'] < high]
        _, _, count, expected = next(f for f in FRAGMENTS if f[0] == start and f[1] == page)
        if len(body) != count or fingerprint(body) != expected:
            raise ValueError(f'Mac contract-review contract changed or missing at {start:x} page {page:x}')


def require_contract_review_index(db, selected=None):
    records = [json.loads(row[0]) for start, end, _, _ in CONTRACTS for row in db.execute(
        'SELECT body FROM instructions WHERE address>=? AND address<? ORDER BY address', (start, end))]
    if selected is None: require_contract_review_bodies(records, complete=True)
    else:
        for page in sorted({f[1] for f in FRAGMENTS}):
            require_contract_review_contract(page, [r for _, r in sorted(selected.items())], selected)
    conditions = ' OR '.join('(e.value>=? AND e.value<?)' for _ in CONTRACTS)
    parameters = [value for start, end, _, _ in CONTRACTS for value in (start, end)]
    require_contract_review_edges(db.execute("""SELECT i.address,e.value
        FROM instructions AS i,
          json_each(json_insert(json_extract(i.body,'$.flows'),'$[#]',json_extract(i.body,'$.fallthrough'))) AS e
        WHERE """ + conditions, parameters))


TABLES = ((277572844, 276872680, 1476, 'bb8a79d7f1bf086a98b15e87f372edf2f08fdc6b3ceedf39f4aca0432b13d036'), (277572836, 276874552, 208, '569ff0266824b3a88db5909fe26760bfa5b58bfeb3d75d0b9f4d1b967465545b'), (277572816, 277672256, 32, '45fe92dfc0736f51ca09921c2339c23842369a1aea3dca77e48af57e36f52589'))

def require_contract_review_tables(read):
    for slot,base,size,expected in TABLES:
        if struct.unpack(">I",read(slot,4))[0]!=base or hashlib.sha256(read(base,size)).hexdigest()!=expected:
            raise ValueError("Changed original contract review table")

def load_contract_review_tables(directory):
    program=json.loads((directory/"program.json").read_text())
    segments=[(s["address"],(directory/s["file"]).read_bytes()) for s in program["segments"] if s.get("file")]
    def read(a,n):
        for base,data in segments:
            if base<=a and a+n<=base+len(data):return data[a-base:a-base+n]
        raise ValueError("Missing original review table")
    require_contract_review_tables(read)
    return tuple(read(base,size) for _,base,size,_ in TABLES)
