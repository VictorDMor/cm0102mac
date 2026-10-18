"""Pinned complete offer evaluation, economic terms and exchange operations."""
import hashlib
import json
import struct

CONTRACTS = ((272719440, 272720840, 350, 'cac94e8237b071e1bfdd39310abb9ca3f8c2a4dbe353ce642fb71e574c34a112'), (272721984, 272723236, 312, 'b5e21bc6da8bd95f8356f1784faf78854ab491e804deca8f93c750a21fcef128'), (272723248, 272723292, 11, '327454ec12fc3c72691be54672ad96811e7875e0f7fec87fd5efc1f43ef981a9'), (272723296, 272723460, 40, 'b6b5a41bad08aa7630f4e38e7196ea30ba2870952531189fb40274528b91b491'), (272723472, 272724208, 183, '84d5fede08dcbc0a5d65c481c547dcb71e2b179a931a987fe2e09503ecdbb53c'), (272724208, 272724576, 92, '7bf0456800fb793589a4a4015df143d862d45b5e86b26f0bcd0ddde8382b7a53'), (272724576, 272724708, 32, '9d3f8acab3937079786caa29ef9034cec2ed2a0c259089e153e9deb55e684b80'), (272724720, 272725392, 168, '5fcf7274d1e99b096672c76f78fa14d831baeb0064631432e9c892392fc4a2d4'))
FRAGMENTS = ((272719440, 66581, 108, '1a14a7d0cfdc32167307683b43773ee1bf13609acf3b5e819f0dd9cb3e09192c'), (272719440, 66582, 242, '663ac12cc69ce866add1ee0b8c6bddb4185b6bef8144716f40db30e04b9b6699'), (272721984, 66582, 312, 'b5e21bc6da8bd95f8356f1784faf78854ab491e804deca8f93c750a21fcef128'), (272723248, 66582, 11, '327454ec12fc3c72691be54672ad96811e7875e0f7fec87fd5efc1f43ef981a9'), (272723296, 66582, 40, 'b6b5a41bad08aa7630f4e38e7196ea30ba2870952531189fb40274528b91b491'), (272723472, 66582, 124, '8af5b2d1ae72677800de15d6530cb6871fc3d53b4df40b0985825c2364969d6a'), (272723472, 66583, 59, 'ae4ad57345d14bbc0d082b086f502980c27015d0e891e36223f041d4b9de1b8a'), (272724208, 66583, 92, '7bf0456800fb793589a4a4015df143d862d45b5e86b26f0bcd0ddde8382b7a53'), (272724576, 66583, 32, '9d3f8acab3937079786caa29ef9034cec2ed2a0c259089e153e9deb55e684b80'), (272724720, 66583, 168, '5fcf7274d1e99b096672c76f78fa14d831baeb0064631432e9c892392fc4a2d4'))
TRANSFER_VALUATION_ENTRIES = tuple(start for start, _, _, _ in CONTRACTS)
PREPARED_TRANSFER_VALUATION_ENTRIES = TRANSFER_VALUATION_ENTRIES


def fingerprint(body):
    canonical = [{key: r.get(key) for key in ('address', 'assembly', 'ops', 'flows', 'fallthrough')} for r in body]
    return hashlib.sha256(json.dumps(canonical, sort_keys=True, separators=(',', ':')).encode()).hexdigest()


def require_transfer_valuation_bodies(instructions, complete=False):
    for start, end, count, expected in CONTRACTS:
        body = [r for r in instructions if start <= r['address'] < end]
        if not body and not complete: continue
        if len(body) != count or fingerprint(body) != expected:
            raise ValueError(f'Mac transfer-valuation contract changed or missing at {start:x}')


def require_transfer_valuation_edges(edges):
    for source, target in edges:
        for start, end, _, _ in CONTRACTS:
            if start < target < end and not start <= source < end:
                raise ValueError(f'External transfer-valuation interior edge from {source:x} to {target:x}')


def require_transfer_valuation_contract(page, instructions, selected=None):
    addresses = {r['address'] for r in instructions} | set(selected or ())
    for start, end, _, _ in CONTRACTS:
        if not start >> 12 <= page <= (end - 1) >> 12: continue
        low, high = max(start, page << 12), min(end, (page + 1) << 12)
        if start not in addresses and not any(low <= a < high for a in addresses): continue
        body = [r for r in instructions if low <= r['address'] < high]
        _, _, count, expected = next(f for f in FRAGMENTS if f[0] == start and f[1] == page)
        if len(body) != count or fingerprint(body) != expected:
            raise ValueError(f'Mac transfer-valuation contract changed or missing at {start:x} page {page:x}')


def require_transfer_valuation_index(db, selected=None):
    records = [json.loads(row[0]) for start, end, _, _ in CONTRACTS for row in db.execute(
        'SELECT body FROM instructions WHERE address>=? AND address<? ORDER BY address', (start, end))]
    if selected is None: require_transfer_valuation_bodies(records, complete=True)
    else:
        for page in sorted({f[1] for f in FRAGMENTS}):
            require_transfer_valuation_contract(page, [r for _, r in sorted(selected.items())], selected)
    conditions = ' OR '.join('(e.value>=? AND e.value<?)' for _ in CONTRACTS)
    parameters = [value for start, end, _, _ in CONTRACTS for value in (start, end)]
    require_transfer_valuation_edges(db.execute("""SELECT i.address,e.value
        FROM instructions AS i,
          json_each(json_insert(json_extract(i.body,'$.flows'),'$[#]',json_extract(i.body,'$.fallthrough'))) AS e
        WHERE """ + conditions, parameters))


TABLES = ((277573380, 276985168, 248, '3af56051bed292a3a51c3102ee3d17e6dbc215cf2c3624b1a87dfc7c5a6e19bd'), (277573376, 276985128, 40, '23816c30b7590309b04786592faa68fc6eac4b3da78b70dc0669a16d9fe287bd'), (277573384, 276985088, 38, 'efac358bcd8b498a1ea44da137d2e18a006c9f76bc210b0f698e801ad0330414'))

def require_transfer_valuation_tables(read):
    for slot,base,size,expected in TABLES:
        if struct.unpack(">I",read(slot,4))[0]!=base or hashlib.sha256(read(base,size)).hexdigest()!=expected:
            raise ValueError("Changed original transfer valuation table")

def load_transfer_valuation_tables(directory):
    program=json.loads((directory/"program.json").read_text())
    segments=[(s["address"],(directory/s["file"]).read_bytes()) for s in program["segments"] if s.get("file")]
    def read(a,n):
        for base,data in segments:
            if base<=a and a+n<=base+len(data):return data[a-base:a-base+n]
        raise ValueError("Missing original transfer-valuation table")
    require_transfer_valuation_tables(read)
    return tuple(read(base,size) for _,base,size,_ in TABLES)
