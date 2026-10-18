"""Pinned complete complete twenty-five-choice currency configuration operations."""
import hashlib
import json
import struct

CONTRACTS = ((268477504, 268480840, 834, 'abd1727200fd45a08e17cd7c11dcf149b22d3952e34fb38939d688b4da6a967a'),)
FRAGMENTS = ((268477504, 65546, 752, '707ebd0f1d09e495bedc38862d62d826d1222b0daeafd975c3163a0d5e4eadd0'), (268477504, 65547, 82, 'c50d18ea8d4d6da861779b9ca726d9875d9822b2d6e20109241c710fcff5218c'))
MONEY_CONFIGURATION_ENTRIES = tuple(start for start, _, _, _ in CONTRACTS)
PREPARED_MONEY_CONFIGURATION_ENTRIES = MONEY_CONFIGURATION_ENTRIES


def fingerprint(body):
    canonical = [{key: r.get(key) for key in ('address', 'assembly', 'ops', 'flows', 'fallthrough')} for r in body]
    return hashlib.sha256(json.dumps(canonical, sort_keys=True, separators=(',', ':')).encode()).hexdigest()


def require_money_configuration_bodies(instructions, complete=False):
    for start, end, count, expected in CONTRACTS:
        body = [r for r in instructions if start <= r['address'] < end]
        if not body and not complete: continue
        if len(body) != count or fingerprint(body) != expected:
            raise ValueError(f'Mac money-configuration contract changed or missing at {start:x}')


def require_money_configuration_edges(edges):
    for source, target in edges:
        for start, end, _, _ in CONTRACTS:
            if start < target < end and not start <= source < end:
                raise ValueError(f'External money-configuration interior edge from {source:x} to {target:x}')


def require_money_configuration_contract(page, instructions, selected=None):
    addresses = {r['address'] for r in instructions} | set(selected or ())
    for start, end, _, _ in CONTRACTS:
        if not start >> 12 <= page <= (end - 1) >> 12: continue
        low, high = max(start, page << 12), min(end, (page + 1) << 12)
        if start not in addresses and not any(low <= a < high for a in addresses): continue
        body = [r for r in instructions if low <= r['address'] < high]
        _, _, count, expected = next(f for f in FRAGMENTS if f[0] == start and f[1] == page)
        if len(body) != count or fingerprint(body) != expected:
            raise ValueError(f'Mac money-configuration contract changed or missing at {start:x} page {page:x}')


def require_money_configuration_index(db, selected=None):
    records = [json.loads(row[0]) for start, end, _, _ in CONTRACTS for row in db.execute(
        'SELECT body FROM instructions WHERE address>=? AND address<? ORDER BY address', (start, end))]
    if selected is None: require_money_configuration_bodies(records, complete=True)
    else:
        for page in sorted({f[1] for f in FRAGMENTS}):
            require_money_configuration_contract(page, [r for _, r in sorted(selected.items())], selected)
    conditions = ' OR '.join('(e.value>=? AND e.value<?)' for _ in CONTRACTS)
    parameters = [value for start, end, _, _ in CONTRACTS for value in (start, end)]
    require_money_configuration_edges(db.execute("""SELECT i.address,e.value
        FROM instructions AS i,
          json_each(json_insert(json_extract(i.body,'$.flows'),'$[#]',json_extract(i.body,'$.fallthrough'))) AS e
        WHERE """ + conditions, parameters))


TABLES = ((277569692, 277642840, 100, '1d342a9c44e66e03275283b7e462f38a608ad059b8db11e9442a79e939cd4d34'), (277569724, 276311732, 328, 'd9a07dbf4a364fa95f8f55d8ddfcf4584fb71cd479c4a16284c30c6d84248a8d'), (277569720, 276310876, 357, '08c92623569e40f3e98aa380bc1516808c4fbb50b1a6cf4e6bc213c72e0a5688'), (277569696, 277991152, 52, '7955cb2de90dd9efc6df9fdbf5f5d10c114f4135a9a6b52db1003be749e32f7a'), (277569700, 277991204, 52, '7955cb2de90dd9efc6df9fdbf5f5d10c114f4135a9a6b52db1003be749e32f7a'), (277569704, 277991256, 52, '7955cb2de90dd9efc6df9fdbf5f5d10c114f4135a9a6b52db1003be749e32f7a'), (277569708, 277991308, 52, '7955cb2de90dd9efc6df9fdbf5f5d10c114f4135a9a6b52db1003be749e32f7a'), (277569712, 277991360, 52, '7955cb2de90dd9efc6df9fdbf5f5d10c114f4135a9a6b52db1003be749e32f7a'), (277569728, 277991412, 52, '7955cb2de90dd9efc6df9fdbf5f5d10c114f4135a9a6b52db1003be749e32f7a'))

def require_money_configuration_tables(read):
    for slot,base,size,expected in TABLES:
        if struct.unpack(">I",read(slot,4))[0]!=base or hashlib.sha256(read(base,size)).hexdigest()!=expected:
            raise ValueError("Changed original monetary value table")

def load_money_configuration_tables(directory):
    program=json.loads((directory/"program.json").read_text())
    segments=[(s["address"],(directory/s["file"]).read_bytes() if s.get("file") else bytes(s["size"])) for s in program["segments"]]
    def read(a,n):
        for base,data in segments:
            if base<=a and a+n<=base+len(data):return data[a-base:a-base+n]
        raise ValueError("Missing original money-configuration table")
    require_money_configuration_tables(read)
    require_money_configuration_padding(read)
    require_money_configuration_words(read)
    return tuple(read(base,size) for _,base,size,_ in TABLES)+tuple(read(address,size) for address,size,_ in WORDS)


PADDING = ((268480840, '00000000'), (268480844, '00000000'))

def require_money_configuration_padding(read):
    for address,expected in PADDING:
        if read(address,4)!=bytes.fromhex(expected):
            raise ValueError("Changed nonindexed money-configuration alignment word")

WORDS = ((277577614, 4, '81ff65efc4487853bdb4625559e69ab44f19e0f5efbd6d5b2af5e3ab267c8e06'), (277577620, 24, '9d908ecfb6b256def8b49a7c504e6c889c4b0e41fe6ce3e01863dd7b61a20aa0'))

def require_money_configuration_words(read):
    for address,size,expected in WORDS:
        if hashlib.sha256(read(address,size)).hexdigest()!=expected:
            raise ValueError("Changed original monetary scale initialization")
