"""Pinned complete contract draft initialization and preparation operations."""
import hashlib
import json
import struct

CONTRACTS = ((272048624, 272048724, 25, 'a63c36c28be294b66fa42258bcabdfc63e32137ba4c6f5d3598174e111007e47'), (272048736, 272049072, 84, '1aa1bd7f5ed7b79510002019de08f76b31c56a9cd63e64aa96bb6a2d5cc3d4a7'), (272049072, 272049548, 119, '9ecada4b567afdb4d0081eee5bf422d32151a142a0191e5eb78700ffb97feaa4'))
FRAGMENTS = ((272048624, 66418, 25, 'a63c36c28be294b66fa42258bcabdfc63e32137ba4c6f5d3598174e111007e47'), (272048736, 66418, 84, '1aa1bd7f5ed7b79510002019de08f76b31c56a9cd63e64aa96bb6a2d5cc3d4a7'), (272049072, 66418, 119, '9ecada4b567afdb4d0081eee5bf422d32151a142a0191e5eb78700ffb97feaa4'))
CONTRACT_DRAFT_ENTRIES = tuple(start for start, _, _, _ in CONTRACTS)
PREPARED_CONTRACT_DRAFT_ENTRIES = CONTRACT_DRAFT_ENTRIES


def fingerprint(body):
    canonical = [{key: r.get(key) for key in ('address', 'assembly', 'ops', 'flows', 'fallthrough')} for r in body]
    return hashlib.sha256(json.dumps(canonical, sort_keys=True, separators=(',', ':')).encode()).hexdigest()


def require_contract_draft_bodies(instructions, complete=False):
    for start, end, count, expected in CONTRACTS:
        body = [r for r in instructions if start <= r['address'] < end]
        if not body and not complete: continue
        if len(body) != count or fingerprint(body) != expected:
            raise ValueError(f'Mac contract-draft contract changed or missing at {start:x}')


def require_contract_draft_edges(edges):
    for source, target in edges:
        for start, end, _, _ in CONTRACTS:
            if start < target < end and not start <= source < end:
                raise ValueError(f'External contract-draft interior edge from {source:x} to {target:x}')


def require_contract_draft_contract(page, instructions, selected=None):
    addresses = {r['address'] for r in instructions} | set(selected or ())
    for start, end, _, _ in CONTRACTS:
        if not start >> 12 <= page <= (end - 1) >> 12: continue
        low, high = max(start, page << 12), min(end, (page + 1) << 12)
        if start not in addresses and not any(low <= a < high for a in addresses): continue
        body = [r for r in instructions if low <= r['address'] < high]
        _, _, count, expected = next(f for f in FRAGMENTS if f[0] == start and f[1] == page)
        if len(body) != count or fingerprint(body) != expected:
            raise ValueError(f'Mac contract-draft contract changed or missing at {start:x} page {page:x}')


def require_contract_draft_index(db, selected=None):
    records = [json.loads(row[0]) for start, end, _, _ in CONTRACTS for row in db.execute(
        'SELECT body FROM instructions WHERE address>=? AND address<? ORDER BY address', (start, end))]
    if selected is None: require_contract_draft_bodies(records, complete=True)
    else:
        for page in sorted({f[1] for f in FRAGMENTS}):
            require_contract_draft_contract(page, [r for _, r in sorted(selected.items())], selected)
    conditions = ' OR '.join('(e.value>=? AND e.value<?)' for _ in CONTRACTS)
    parameters = [value for start, end, _, _ in CONTRACTS for value in (start, end)]
    require_contract_draft_edges(db.execute("""SELECT i.address,e.value
        FROM instructions AS i,
          json_each(json_insert(json_extract(i.body,'$.flows'),'$[#]',json_extract(i.body,'$.fallthrough'))) AS e
        WHERE """ + conditions, parameters))


TABLES = ((277572844, 276872680, 39, '844bd7a8bc75a3c707e5ec8ce347e5eabbe001a7b76dd9063cdf2ea687a8ff2c'),)

def require_contract_draft_tables(read):
    for slot,base,size,expected in TABLES:
        if struct.unpack(">I",read(slot,4))[0]!=base or hashlib.sha256(read(base,size)).hexdigest()!=expected:
            raise ValueError("Changed original contract draft table")

def load_contract_draft_tables(directory):
    program=json.loads((directory/"program.json").read_text())
    segments=[(s["address"],(directory/s["file"]).read_bytes()) for s in program["segments"] if s.get("file")]
    def read(a,n):
        for base,data in segments:
            if base<=a and a+n<=base+len(data):return data[a-base:a-base+n]
        raise ValueError("Missing original contract-draft table")
    require_contract_draft_tables(read)
    require_contract_draft_padding(read)
    return tuple(read(base,size) for _,base,size,_ in TABLES)


PADDING = ((272048724, '00000000'), (272048728, '00000000'), (272048732, '00000000'), (272049548, '00000000'))

def require_contract_draft_padding(read):
    for address,expected in PADDING:
        if read(address,4)!=bytes.fromhex(expected):
            raise ValueError("Changed nonindexed contract-draft alignment word")
