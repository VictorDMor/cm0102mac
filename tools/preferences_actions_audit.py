"""Pinned complete preference-session opening and action controllers."""
import hashlib
import json
import struct

CONTRACTS = ((271830640, 271830804, 41, '76895d2471579f05b4be967ff34b76c4ee274be2920dd1c4fca40c5647f1e214'), (271830816, 271831080, 66, '01fe910528c19404f92cb949f4c759252f945636d9a841555fdbbe71e8c7c7c8'), (271841344, 271842400, 264, 'f2770661c12a7d867e98e722b1bafe00779915edc5a584e39cd95ecabdabcb21'))
FRAGMENTS = ((271830640, 66364, 41, '76895d2471579f05b4be967ff34b76c4ee274be2920dd1c4fca40c5647f1e214'), (271830816, 66364, 56, '30bc0601c4fd90030b8b1e9630dfa1a3a0490c11515a3e17eeb521319fd0f457'), (271830816, 66365, 10, 'ca14e19a20273ae0f3cc90e51f7beb28ee66704d09bd68a7d0deb7fa1d9b3ecc'), (271841344, 66367, 264, 'f2770661c12a7d867e98e722b1bafe00779915edc5a584e39cd95ecabdabcb21'))
PREFERENCES_ACTIONS_ENTRIES = tuple(start for start, _, _, _ in CONTRACTS)
PREPARED_PREFERENCES_ACTIONS_ENTRIES = PREFERENCES_ACTIONS_ENTRIES


def fingerprint(body):
    canonical = [{key: r.get(key) for key in ('address', 'assembly', 'ops', 'flows', 'fallthrough')} for r in body]
    return hashlib.sha256(json.dumps(canonical, sort_keys=True, separators=(',', ':')).encode()).hexdigest()


def require_preferences_actions_bodies(instructions, complete=False):
    for start, end, count, expected in CONTRACTS:
        body = [r for r in instructions if start <= r['address'] < end]
        if not body and not complete: continue
        if len(body) != count or fingerprint(body) != expected:
            raise ValueError(f'Mac preferences_actions contract changed or missing at {start:x}')


def require_preferences_actions_edges(edges):
    for source, target in edges:
        for start, end, _, _ in CONTRACTS:
            if start < target < end and not start <= source < end:
                raise ValueError(f'External preferences_actions interior edge from {source:x} to {target:x}')


def require_preferences_actions_contract(page, instructions, selected=None):
    addresses = {r['address'] for r in instructions} | set(selected or ())
    for start, end, _, _ in CONTRACTS:
        if not start >> 12 <= page <= (end - 1) >> 12: continue
        low, high = max(start, page << 12), min(end, (page + 1) << 12)
        if start not in addresses and not any(low <= a < high for a in addresses): continue
        body = [r for r in instructions if low <= r['address'] < high]
        _, _, count, expected = next(f for f in FRAGMENTS if f[0] == start and f[1] == page)
        if len(body) != count or fingerprint(body) != expected:
            raise ValueError(f'Mac preferences_actions contract changed or missing at {start:x} page {page:x}')


def require_preferences_actions_index(db, selected=None):
    records = [json.loads(row[0]) for start, end, _, _ in CONTRACTS for row in db.execute(
        'SELECT body FROM instructions WHERE address>=? AND address<? ORDER BY address', (start, end))]
    if selected is None: require_preferences_actions_bodies(records, complete=True)
    else:
        for page in sorted({f[1] for f in FRAGMENTS}):
            require_preferences_actions_contract(page, [r for _, r in sorted(selected.items())], selected)
    conditions = ' OR '.join('(e.value>=? AND e.value<?)' for _ in CONTRACTS)
    parameters = [value for start, end, _, _ in CONTRACTS for value in (start, end)]
    require_preferences_actions_edges(db.execute("""SELECT i.address,e.value
        FROM instructions AS i,
          json_each(json_insert(json_extract(i.body,'$.flows'),'$[#]',json_extract(i.body,'$.fallthrough'))) AS e
        WHERE """ + conditions, parameters))


TABLES = ((277561056, 278458548, 4, 'df3f619804a92fdb4057192dc43dd748ea778adc52bc498ce80524c014b81119'), (277561288, 277999168, 4, 'df3f619804a92fdb4057192dc43dd748ea778adc52bc498ce80524c014b81119'), (277561080, 277997176, 4, 'df3f619804a92fdb4057192dc43dd748ea778adc52bc498ce80524c014b81119'), (277572572, 277582092, 8, '747e0f9cc5ec1b49b3666e106869bfea5cacc6d53c80f29118c7d6b564f568d2'), (277572568, 277582084, 8, 'de27d4ec064ec4cea2df7adbc0c624ddce799ed77818230270ba6f294266722a'))

def require_preferences_actions_tables(read):
    for slot,base,size,expected in TABLES:
        if struct.unpack(">I",read(slot,4))[0]!=base or hashlib.sha256(read(base,size)).hexdigest()!=expected:
            raise ValueError("Changed original preference initialization table")

def load_preferences_actions_tables(directory):
    program=json.loads((directory/"program.json").read_text())
    segments=[(s["address"],(directory/s["file"]).read_bytes() if s.get("file") else bytes(s["size"])) for s in program["segments"]]
    def read(a,n):
        for base,data in segments:
            if base<=a and a+n<=base+len(data):return data[a-base:a-base+n]
        raise ValueError("Missing original preferences_actions table")
    require_preferences_actions_tables(read)
    require_preferences_actions_padding(read)
    require_preferences_actions_words(read)
    return tuple(read(base,size) for _,base,size,_ in TABLES)+tuple(read(address,size) for address,size,_ in WORDS)


PADDING = ((271830804, '00000000'), (271830808, '00000000'), (271830812, '00000000'), (271831080, '00000000'), (271831084, '00000000'))

def require_preferences_actions_padding(read):
    for address,expected in PADDING:
        if read(address,4)!=bytes.fromhex(expected):
            raise ValueError("Changed nonindexed preferences_actions alignment word")

WORDS = ()

def require_preferences_actions_words(read):
    for address,size,expected in WORDS:
        if hashlib.sha256(read(address,size)).hexdigest()!=expected:
            raise ValueError("Changed original preference initialization")
