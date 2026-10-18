"""Pinned complete contract wage and end presentation operations."""
import hashlib
import json
import struct

CONTRACTS = ((272049552, 272049956, 101, '5dc7244ea02b416fe7b912ba3bb534a8c36b63f526d9208e57d9fddb3dcbf45c'), (272049968, 272050108, 35, '000f6e1e2eeb43a62ce9a01a484c9339253aa3629948abf787565cb33d0a0ed9'))
FRAGMENTS = ((272049552, 66418, 101, '5dc7244ea02b416fe7b912ba3bb534a8c36b63f526d9208e57d9fddb3dcbf45c'), (272049968, 66418, 35, '000f6e1e2eeb43a62ce9a01a484c9339253aa3629948abf787565cb33d0a0ed9'))
CONTRACT_DISPLAY_ENTRIES = tuple(start for start, _, _, _ in CONTRACTS)
PREPARED_CONTRACT_DISPLAY_ENTRIES = CONTRACT_DISPLAY_ENTRIES


def fingerprint(body):
    canonical = [{key: r.get(key) for key in ('address', 'assembly', 'ops', 'flows', 'fallthrough')} for r in body]
    return hashlib.sha256(json.dumps(canonical, sort_keys=True, separators=(',', ':')).encode()).hexdigest()


def require_contract_display_bodies(instructions, complete=False):
    for start, end, count, expected in CONTRACTS:
        body = [r for r in instructions if start <= r['address'] < end]
        if not body and not complete: continue
        if len(body) != count or fingerprint(body) != expected:
            raise ValueError(f'Mac contract-display contract changed or missing at {start:x}')


def require_contract_display_edges(edges):
    for source, target in edges:
        for start, end, _, _ in CONTRACTS:
            if start < target < end and not start <= source < end:
                raise ValueError(f'External contract-display interior edge from {source:x} to {target:x}')


def require_contract_display_contract(page, instructions, selected=None):
    addresses = {r['address'] for r in instructions} | set(selected or ())
    for start, end, _, _ in CONTRACTS:
        if not start >> 12 <= page <= (end - 1) >> 12: continue
        low, high = max(start, page << 12), min(end, (page + 1) << 12)
        if start not in addresses and not any(low <= a < high for a in addresses): continue
        body = [r for r in instructions if low <= r['address'] < high]
        _, _, count, expected = next(f for f in FRAGMENTS if f[0] == start and f[1] == page)
        if len(body) != count or fingerprint(body) != expected:
            raise ValueError(f'Mac contract-display contract changed or missing at {start:x} page {page:x}')


def require_contract_display_index(db, selected=None):
    records = [json.loads(row[0]) for start, end, _, _ in CONTRACTS for row in db.execute(
        'SELECT body FROM instructions WHERE address>=? AND address<? ORDER BY address', (start, end))]
    if selected is None: require_contract_display_bodies(records, complete=True)
    else:
        for page in sorted({f[1] for f in FRAGMENTS}):
            require_contract_display_contract(page, [r for _, r in sorted(selected.items())], selected)
    conditions = ' OR '.join('(e.value>=? AND e.value<?)' for _ in CONTRACTS)
    parameters = [value for start, end, _, _ in CONTRACTS for value in (start, end)]
    require_contract_display_edges(db.execute("""SELECT i.address,e.value
        FROM instructions AS i,
          json_each(json_insert(json_extract(i.body,'$.flows'),'$[#]',json_extract(i.body,'$.fallthrough'))) AS e
        WHERE """ + conditions, parameters))


TABLES = ((277572844, 276872680, 251, '67a676a79f654aebe9e5575aa84303a449b1da4f0c23e2b236da5ddb41e545ef'),)

def require_contract_display_tables(read):
    for slot,base,size,expected in TABLES:
        if struct.unpack(">I",read(slot,4))[0]!=base or hashlib.sha256(read(base,size)).hexdigest()!=expected:
            raise ValueError("Changed original contract display table")

def load_contract_display_tables(directory):
    program=json.loads((directory/"program.json").read_text())
    segments=[(s["address"],(directory/s["file"]).read_bytes()) for s in program["segments"] if s.get("file")]
    def read(a,n):
        for base,data in segments:
            if base<=a and a+n<=base+len(data):return data[a-base:a-base+n]
        raise ValueError("Missing original contract-display table")
    require_contract_display_tables(read)
    require_contract_display_padding(read)
    return tuple(read(base,size) for _,base,size,_ in TABLES)


PADDING = ((272049956, '00000000'), (272049960, '00000000'), (272049964, '00000000'), (272050108, '00000000'))

def require_contract_display_padding(read):
    for address,expected in PADDING:
        if read(address,4)!=bytes.fromhex(expected):
            raise ValueError("Changed nonindexed contract-display alignment word")


def contract_display_hook(entry):
    return f'if (try_modern_contract_display(s, 0x{entry:x})) return true;'


def require_contract_display_hooks(page, instructions, code):
    addresses = {r['address'] for r in instructions}
    for entry in CONTRACT_DISPLAY_ENTRIES:
        if entry >> 12 != page or entry not in addresses: continue
        label, hook = f'i_{entry:x}:', contract_display_hook(entry)
        fragment = code.split(label, 1)[-1].split(f'i_{entry + 4:x}:', 1)[0]
        steps = [token for token in (f's.step(0x{entry:x});', f'counter.step(0x{entry:x});') if token in fragment]
        if code.count(hook) != 1 or hook not in fragment or len(steps) != 1 or fragment.index(hook) > fragment.index(steps[0]):
            raise ValueError(f'Missing or misplaced contract-display hook at {entry:x}')
