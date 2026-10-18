"""Pinned complete offer record lifecycle, persistence, explicit initialization and value scope."""
import hashlib
import json
import struct

CONTRACTS = ((272703072, 272703136, 16, 'ff3fb73d4330272f04c267b9d1cf43e5ffa300a84564218c93a5038069c12d96'), (272703136, 272703240, 26, '98b9ddca19326156a8410fd06a64dffd302a0ffd84e7069c43ff0787461e3560'), (272703248, 272703388, 35, 'd53b95103def1c94125535ec1643241daf4beb9179f60f23b568484feb16e85f'), (272703392, 272704468, 266, '573dfeb526bc99425604db0a8048ab5308222bc2f9eb602416792e417bb966c0'), (272704480, 272705228, 185, 'b37d414c9063d82012b931d8d835959ba135a4e38f58cd9e70a3992842dec892'), (272720848, 272721124, 69, '206ae477791d5019efba1189c2d57515b23766ce160f96bd7aa2965fe0e51894'), (272721136, 272721680, 136, '3b1e1cf53b3cca886ea1c237faca3d15418079e317753192f529fda04ce214b6'), (272721680, 272721972, 73, '39707c8960d350f6128ebf3c4a0f562eaa2844f42f6c9b1f65a314fd4b161f2d'))
FRAGMENTS = ((272703072, 66577, 16, 'ff3fb73d4330272f04c267b9d1cf43e5ffa300a84564218c93a5038069c12d96'), (272703136, 66577, 26, '98b9ddca19326156a8410fd06a64dffd302a0ffd84e7069c43ff0787461e3560'), (272703248, 66577, 35, 'd53b95103def1c94125535ec1643241daf4beb9179f60f23b568484feb16e85f'), (272703392, 66577, 24, '400ad028fc9b6c266f303b78eb3b4ccfa80048beb709e7f484b4c6838d0e7a1a'), (272703392, 66578, 242, 'd91cf4092dadeb56bd56d4bb7313f9f70f32913bbdb9502dc0dbba987f43e2da'), (272704480, 66578, 185, 'b37d414c9063d82012b931d8d835959ba135a4e38f58cd9e70a3992842dec892'), (272720848, 66582, 69, '206ae477791d5019efba1189c2d57515b23766ce160f96bd7aa2965fe0e51894'), (272721136, 66582, 136, '3b1e1cf53b3cca886ea1c237faca3d15418079e317753192f529fda04ce214b6'), (272721680, 66582, 73, '39707c8960d350f6128ebf3c4a0f562eaa2844f42f6c9b1f65a314fd4b161f2d'))
TRANSFER_RECORD_ENTRIES = tuple(start for start, _, _, _ in CONTRACTS)
PREPARED_TRANSFER_RECORD_ENTRIES = TRANSFER_RECORD_ENTRIES


def fingerprint(body):
    canonical = [{key: r.get(key) for key in ('address', 'assembly', 'ops', 'flows', 'fallthrough')} for r in body]
    return hashlib.sha256(json.dumps(canonical, sort_keys=True, separators=(',', ':')).encode()).hexdigest()


def require_transfer_record_bodies(instructions, complete=False):
    for start, end, count, expected in CONTRACTS:
        body = [r for r in instructions if start <= r['address'] < end]
        if not body and not complete: continue
        if len(body) != count or fingerprint(body) != expected:
            raise ValueError(f'Mac transfer-record contract changed or missing at {start:x}')


def require_transfer_record_edges(edges):
    for source, target in edges:
        for start, end, _, _ in CONTRACTS:
            if start < target < end and not start <= source < end:
                raise ValueError(f'External transfer-record interior edge from {source:x} to {target:x}')


def require_transfer_record_contract(page, instructions, selected=None):
    addresses = {r['address'] for r in instructions} | set(selected or ())
    for start, end, _, _ in CONTRACTS:
        if not start >> 12 <= page <= (end - 1) >> 12: continue
        low, high = max(start, page << 12), min(end, (page + 1) << 12)
        if start not in addresses and not any(low <= a < high for a in addresses): continue
        body = [r for r in instructions if low <= r['address'] < high]
        _, _, count, expected = next(f for f in FRAGMENTS if f[0] == start and f[1] == page)
        if len(body) != count or fingerprint(body) != expected:
            raise ValueError(f'Mac transfer-record contract changed or missing at {start:x} page {page:x}')


def require_transfer_record_index(db, selected=None):
    records = [json.loads(row[0]) for start, end, _, _ in CONTRACTS for row in db.execute(
        'SELECT body FROM instructions WHERE address>=? AND address<? ORDER BY address', (start, end))]
    if selected is None: require_transfer_record_bodies(records, complete=True)
    else:
        for page in sorted({f[1] for f in FRAGMENTS}):
            require_transfer_record_contract(page, [r for _, r in sorted(selected.items())], selected)
    conditions = ' OR '.join('(e.value>=? AND e.value<?)' for _ in CONTRACTS)
    parameters = [value for start, end, _, _ in CONTRACTS for value in (start, end)]
    require_transfer_record_edges(db.execute("""SELECT i.address,e.value
        FROM instructions AS i,
          json_each(json_insert(json_extract(i.body,'$.flows'),'$[#]',json_extract(i.body,'$.fallthrough'))) AS e
        WHERE """ + conditions, parameters))


TABLES = ((277573384, 276985088, 38, 'efac358bcd8b498a1ea44da137d2e18a006c9f76bc210b0f698e801ad0330414'),)

def require_transfer_record_tables(read):
    for slot,base,size,expected in TABLES:
        if struct.unpack(">I",read(slot,4))[0]!=base or hashlib.sha256(read(base,size)).hexdigest()!=expected:
            raise ValueError("Changed original transfer record table")

def load_transfer_record_tables(directory):
    program=json.loads((directory/"program.json").read_text())
    segments=[(s["address"],(directory/s["file"]).read_bytes()) for s in program["segments"] if s.get("file")]
    def read(a,n):
        for base,data in segments:
            if base<=a and a+n<=base+len(data):return data[a-base:a-base+n]
        raise ValueError("Missing original transfer-record table")
    require_transfer_record_tables(read)
    return tuple(read(base,size) for _,base,size,_ in TABLES)
