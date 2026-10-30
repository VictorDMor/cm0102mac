"""Pinned complete monetary value conversion and editing operations."""
import hashlib
import json
import struct

CONTRACTS = ((268476192, 268476424, 58, '7e6c8ca6d2a742bd389d88373129513a53ddb4b07bf93a70b210ea4f6d0dd1d3'), (268476432, 268476508, 19, '1558e2ae7a59be7803bb1b926c505c48c3244d129c8088d9eff673cf7affce60'), (268476512, 268476772, 65, 'e06fb76cb11080e80369a8e449a64ad9bca3141f79130a7ec6001ea6bc2f38ad'), (268476784, 268477036, 63, 'a70950f21c026f743e6bc8fc8821b7631a45f5492b65c7c2373276ffd6e06f4a'), (268477040, 268477228, 47, 'efb03a575804235eb02d5cfffb0d88db968388d69ea09922f33e740bc65d5fc7'), (268477232, 268477504, 68, 'e85162dfd27121c8c526dd7042ce7e14d19cde9e856522b37836dd513f72ad30'), (268486512, 268487604, 273, '30bba798fe2a21bf68f2f5367c89805bc2075f0922644554fb796b3d2e73130d'))
FRAGMENTS = ((268476192, 65545, 56, '6a8353bb35adb4d4bc3d21b2908d37be05f99023a858826be919b6e636c7b520'), (268476192, 65546, 2, 'bae427f3ca3c083e61001fe8aec40e907981b76ef2b0e7cfdbd7b4b9805a3007'), (268476432, 65546, 19, '1558e2ae7a59be7803bb1b926c505c48c3244d129c8088d9eff673cf7affce60'), (268476512, 65546, 65, 'e06fb76cb11080e80369a8e449a64ad9bca3141f79130a7ec6001ea6bc2f38ad'), (268476784, 65546, 63, 'a70950f21c026f743e6bc8fc8821b7631a45f5492b65c7c2373276ffd6e06f4a'), (268477040, 65546, 47, 'efb03a575804235eb02d5cfffb0d88db968388d69ea09922f33e740bc65d5fc7'), (268477232, 65546, 68, 'e85162dfd27121c8c526dd7042ce7e14d19cde9e856522b37836dd513f72ad30'), (268486512, 65548, 273, '30bba798fe2a21bf68f2f5367c89805bc2075f0922644554fb796b3d2e73130d'))
MONEY_VALUE_ENTRIES = tuple(start for start, _, _, _ in CONTRACTS)
PREPARED_MONEY_VALUE_ENTRIES = MONEY_VALUE_ENTRIES


def fingerprint(body):
    canonical = [{key: r.get(key) for key in ('address', 'assembly', 'ops', 'flows', 'fallthrough')} for r in body]
    return hashlib.sha256(json.dumps(canonical, sort_keys=True, separators=(',', ':')).encode()).hexdigest()


def require_money_value_bodies(instructions, complete=False):
    for start, end, count, expected in CONTRACTS:
        body = [r for r in instructions if start <= r['address'] < end]
        if not body and not complete: continue
        if len(body) != count or fingerprint(body) != expected:
            raise ValueError(f'Mac money-value contract changed or missing at {start:x}')


def require_money_value_edges(edges):
    for source, target in edges:
        for start, end, _, _ in CONTRACTS:
            if start < target < end and not start <= source < end:
                raise ValueError(f'External money-value interior edge from {source:x} to {target:x}')


def require_money_value_contract(page, instructions, selected=None):
    addresses = {r['address'] for r in instructions} | set(selected or ())
    for start, end, _, _ in CONTRACTS:
        if not start >> 12 <= page <= (end - 1) >> 12: continue
        low, high = max(start, page << 12), min(end, (page + 1) << 12)
        if start not in addresses and not any(low <= a < high for a in addresses): continue
        body = [r for r in instructions if low <= r['address'] < high]
        _, _, count, expected = next(f for f in FRAGMENTS if f[0] == start and f[1] == page)
        if len(body) != count or fingerprint(body) != expected:
            raise ValueError(f'Mac money-value contract changed or missing at {start:x} page {page:x}')


def require_money_value_index(db, selected=None):
    records = [json.loads(row[0]) for start, end, _, _ in CONTRACTS for row in db.execute(
        'SELECT body FROM instructions WHERE address>=? AND address<? ORDER BY address', (start, end))]
    if selected is None: require_money_value_bodies(records, complete=True)
    else:
        for page in sorted({f[1] for f in FRAGMENTS}):
            require_money_value_contract(page, [r for _, r in sorted(selected.items())], selected)
    conditions = ' OR '.join('(e.value>=? AND e.value<?)' for _ in CONTRACTS)
    parameters = [value for start, end, _, _ in CONTRACTS for value in (start, end)]
    require_money_value_edges(db.execute("""SELECT i.address,e.value
        FROM instructions AS i,
          json_each(json_insert(json_extract(i.body,'$.flows'),'$[#]',json_extract(i.body,'$.fallthrough'))) AS e
        WHERE """ + conditions, parameters))


TABLES = ((277569724, 276311732, 448, 'f74a6f00cf6f8daf046d7b0e1d135cf40eb142d9ea10711fe075da5e699ca5b8'), (277569716, 277642812, 28, 'cc562dcdc6584194980b528db7e8d04cc14812e4bc4c3c3c6ff0862193560978'), (277569720, 276310876, 28, 'a25881a843d0ad1e44dcc3c510192136e63626f6b0680159009523bc838822b8'))

def require_money_value_tables(read):
    for slot,base,size,expected in TABLES:
        if struct.unpack(">I",read(slot,4))[0]!=base or hashlib.sha256(read(base,size)).hexdigest()!=expected:
            raise ValueError("Changed original monetary value table")

def load_money_value_tables(directory):
    program=json.loads((directory/"program.json").read_text())
    segments=[(s["address"],(directory/s["file"]).read_bytes()) for s in program["segments"] if s.get("file")]
    def read(a,n):
        for base,data in segments:
            if base<=a and a+n<=base+len(data):return data[a-base:a-base+n]
        raise ValueError("Missing original money-value table")
    require_money_value_tables(read)
    require_money_value_padding(read)
    require_money_value_words(read)
    return tuple(read(base,size) for _,base,size,_ in TABLES)+tuple(read(address,size) for address,size,_ in WORDS)


PADDING = ((268476424, '00000000'), (268476428, '00000000'), (268476508, '00000000'), (268476772, '00000000'), (268476776, '00000000'), (268476780, '00000000'), (268477036, '00000000'), (268477228, '00000000'), (268487604, '00000000'), (268487608, '00000000'), (268487612, '00000000'))

def require_money_value_padding(read):
    for address,expected in PADDING:
        if read(address,4)!=bytes.fromhex(expected):
            raise ValueError("Changed nonindexed money-value alignment word")

WORDS = ((277577620, 24, '9d908ecfb6b256def8b49a7c504e6c889c4b0e41fe6ce3e01863dd7b61a20aa0'),)

def require_money_value_words(read):
    for address,size,expected in WORDS:
        if hashlib.sha256(read(address,size)).hexdigest()!=expected:
            raise ValueError("Changed original monetary scale initialization")


def money_value_hook(entry):
    return f'if (try_modern_money_value(s, 0x{entry:x})) return true;'


def require_money_value_hooks(page, instructions, code, taken_over=()):
    addresses = {r['address'] for r in instructions}
    for entry in MONEY_VALUE_ENTRIES:
        if entry in taken_over: continue
        if entry >> 12 != page or entry not in addresses: continue
        label, hook = f'i_{entry:x}:', money_value_hook(entry)
        fragment = code.split(label, 1)[-1].split(f'i_{entry + 4:x}:', 1)[0]
        steps = [token for token in (f's.step(0x{entry:x});', f'counter.step(0x{entry:x});') if token in fragment]
        if code.count(hook) != 1 or hook not in fragment or len(steps) != 1 or fragment.index(hook) > fragment.index(steps[0]):
            raise ValueError(f'Missing or misplaced money-value hook at {entry:x}')
