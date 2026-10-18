"""Pinned complete automatic offer synthesis boundary."""
import hashlib
import json
import struct

CONTRACTS = ((272705232, 272719436, 3551, '40789d9a9152c219308322911eab4ffc1b92f03829e55e522661bc9cb0ce63ed'),)
FRAGMENTS = ((272705232, 66578, 588, 'eb40dfeb67097fc08b28f9b9f34696bc9f19dd0acfd0865aa7e39761cd090088'), (272705232, 66579, 1024, '08bbcb6722c84eec885238c74f9d936a72ebebd181e9ec915488a5f3a50a1bb4'), (272705232, 66580, 1024, '2e9945441ce612a51c661e942b371a08aa279229eca3fc49ad32b0edc38dcbdf'), (272705232, 66581, 915, 'def34a24ddc954748f484ebb50741f644be1eda605e9aba856ad2cd15b8c3229'))
TRANSFER_SYNTHESIS_ENTRIES = tuple(start for start, _, _, _ in CONTRACTS)
PREPARED_TRANSFER_SYNTHESIS_ENTRIES = TRANSFER_SYNTHESIS_ENTRIES


def fingerprint(body):
    canonical = [{key: r.get(key) for key in ('address', 'assembly', 'ops', 'flows', 'fallthrough')} for r in body]
    return hashlib.sha256(json.dumps(canonical, sort_keys=True, separators=(',', ':')).encode()).hexdigest()


def require_transfer_synthesis_bodies(instructions, complete=False):
    for start, end, count, expected in CONTRACTS:
        body = [r for r in instructions if start <= r['address'] < end]
        if not body and not complete: continue
        if len(body) != count or fingerprint(body) != expected:
            raise ValueError(f'Mac transfer-synthesis contract changed or missing at {start:x}')


def require_transfer_synthesis_edges(edges):
    for source, target in edges:
        for start, end, _, _ in CONTRACTS:
            if start < target < end and not start <= source < end:
                raise ValueError(f'External transfer-synthesis interior edge from {source:x} to {target:x}')


def require_transfer_synthesis_contract(page, instructions, selected=None):
    addresses = {r['address'] for r in instructions} | set(selected or ())
    for start, end, _, _ in CONTRACTS:
        if not start >> 12 <= page <= (end - 1) >> 12: continue
        low, high = max(start, page << 12), min(end, (page + 1) << 12)
        if start not in addresses and not any(low <= a < high for a in addresses): continue
        body = [r for r in instructions if low <= r['address'] < high]
        _, _, count, expected = next(f for f in FRAGMENTS if f[0] == start and f[1] == page)
        if len(body) != count or fingerprint(body) != expected:
            raise ValueError(f'Mac transfer-synthesis contract changed or missing at {start:x} page {page:x}')


def require_transfer_synthesis_index(db, selected=None):
    records = [json.loads(row[0]) for start, end, _, _ in CONTRACTS for row in db.execute(
        'SELECT body FROM instructions WHERE address>=? AND address<? ORDER BY address', (start, end))]
    if selected is None: require_transfer_synthesis_bodies(records, complete=True)
    else:
        for page in sorted({f[1] for f in FRAGMENTS}):
            require_transfer_synthesis_contract(page, [r for _, r in sorted(selected.items())], selected)
    conditions = ' OR '.join('(e.value>=? AND e.value<?)' for _ in CONTRACTS)
    parameters = [value for start, end, _, _ in CONTRACTS for value in (start, end)]
    require_transfer_synthesis_edges(db.execute("""SELECT i.address,e.value
        FROM instructions AS i,
          json_each(json_insert(json_extract(i.body,'$.flows'),'$[#]',json_extract(i.body,'$.fallthrough'))) AS e
        WHERE """ + conditions, parameters))


TABLES = ((277573380, 276985168, 248, '3af56051bed292a3a51c3102ee3d17e6dbc215cf2c3624b1a87dfc7c5a6e19bd'), (277573376, 276985128, 40, '23816c30b7590309b04786592faa68fc6eac4b3da78b70dc0669a16d9fe287bd'), (277573384, 276985088, 38, 'efac358bcd8b498a1ea44da137d2e18a006c9f76bc210b0f698e801ad0330414'))

def require_transfer_synthesis_tables(read):
    for slot,base,size,expected in TABLES:
        if struct.unpack(">I",read(slot,4))[0]!=base or hashlib.sha256(read(base,size)).hexdigest()!=expected:
            raise ValueError("Changed original transfer synthesis table")

def load_transfer_synthesis_tables(directory):
    program=json.loads((directory/"program.json").read_text())
    segments=[(s["address"],(directory/s["file"]).read_bytes()) for s in program["segments"] if s.get("file")]
    def read(a,n):
        for base,data in segments:
            if base<=a and a+n<=base+len(data):return data[a-base:a-base+n]
        raise ValueError("Missing original transfer-synthesis table")
    require_transfer_synthesis_tables(read)
    return tuple(read(base,size) for _,base,size,_ in TABLES)


def transfer_synthesis_hook(entry):
    return f'if (try_modern_transfer_synthesis(s, 0x{entry:x})) return true;'


def require_transfer_synthesis_hooks(page, instructions, code):
    addresses = {r['address'] for r in instructions}
    for entry in TRANSFER_SYNTHESIS_ENTRIES:
        if entry >> 12 != page or entry not in addresses: continue
        label, hook = f'i_{entry:x}:', transfer_synthesis_hook(entry)
        fragment = code.split(label, 1)[-1].split(f'i_{entry + 4:x}:', 1)[0]
        steps = [token for token in (f's.step(0x{entry:x});', f'counter.step(0x{entry:x});') if token in fragment]
        if code.count(hook) != 1 or hook not in fragment or len(steps) != 1 or fragment.index(hook) > fragment.index(steps[0]):
            raise ValueError(f'Missing or misplaced transfer-synthesis hook at {entry:x}')
