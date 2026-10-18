"""Pinned complete twelve-entry preference lifecycle."""
import hashlib
import json
import struct

CONTRACTS = ((269742128, 269743108, 245, '73760379549bb097640e8550b788faed52546bd9832dbcd3ddf545d3b6cc9976'), (269743120, 269743300, 45, 'bc3aba97dc433fb9e79829a5621205ceccec84f4ba80267248abec367676838e'), (269743312, 269743460, 37, '566694777e770adcb4b6d82e5375345fae12874c887d4201127719e2b8ad13ab'), (269743472, 269743740, 67, '490092420c8e726c30dc74dd9bdab2b8347b4d588d86db9b39067d7cd6204173'), (269743744, 269744984, 310, '88ea78e9f4b2387e62ec87579edeec2514914c09609072ca2bd2c9dcfc7c04ec'), (269744992, 269745220, 57, '9f5abdec37f8ee1902561132ea8d5acbede1a9c88d905fa9faab59e7a14253e2'), (269745232, 269745404, 43, '275187f4ba5c7d4ed64dadb5c5c06e5d8209778bf4109bec2ba0902ac7787a4c'), (269745408, 269745652, 61, 'bd8e4369c448ec250e79451e0d15a24333d00ef2c7d40759ed5a3a702072c04b'), (269745664, 269745836, 43, 'bc3f971a36fd65aa276e1a0136e23b594fe214845060f63ac108b88ca93dfddc'), (269745840, 269747924, 521, '286f142580d797113d58261304c346d57ee2d5e98a5f38413eea8c6c0f7ef971'), (269747936, 269747944, 2, 'fb9d0bd3d1635183fad8669254febca6d882626768829416e4c19973dd8a1771'), (269747952, 269748004, 13, '315afc6deedea141805419c27533e25cdc902460e7a1457007b85edd4670951f'))
FRAGMENTS = ((269742128, 65855, 245, '73760379549bb097640e8550b788faed52546bd9832dbcd3ddf545d3b6cc9976'), (269743120, 65855, 45, 'bc3aba97dc433fb9e79829a5621205ceccec84f4ba80267248abec367676838e'), (269743312, 65855, 37, '566694777e770adcb4b6d82e5375345fae12874c887d4201127719e2b8ad13ab'), (269743472, 65855, 67, '490092420c8e726c30dc74dd9bdab2b8347b4d588d86db9b39067d7cd6204173'), (269743744, 65855, 310, '88ea78e9f4b2387e62ec87579edeec2514914c09609072ca2bd2c9dcfc7c04ec'), (269744992, 65855, 57, '9f5abdec37f8ee1902561132ea8d5acbede1a9c88d905fa9faab59e7a14253e2'), (269745232, 65855, 43, '275187f4ba5c7d4ed64dadb5c5c06e5d8209778bf4109bec2ba0902ac7787a4c'), (269745408, 65855, 61, 'bd8e4369c448ec250e79451e0d15a24333d00ef2c7d40759ed5a3a702072c04b'), (269745664, 65855, 43, 'bc3f971a36fd65aa276e1a0136e23b594fe214845060f63ac108b88ca93dfddc'), (269745840, 65855, 84, 'a497b16cb653dca65bdad626fd41429df931b267222104c85716db21654c0ae6'), (269745840, 65856, 437, 'f9b22fdd70c00a0df5f8ec93ffb16a04120c5a079558c420707845023bb5e822'), (269747936, 65856, 2, 'fb9d0bd3d1635183fad8669254febca6d882626768829416e4c19973dd8a1771'), (269747952, 65856, 13, '315afc6deedea141805419c27533e25cdc902460e7a1457007b85edd4670951f'))
PREFERENCES_ENTRIES = tuple(start for start, _, _, _ in CONTRACTS)
PREPARED_PREFERENCES_ENTRIES = PREFERENCES_ENTRIES


def fingerprint(body):
    canonical = [{key: r.get(key) for key in ('address', 'assembly', 'ops', 'flows', 'fallthrough')} for r in body]
    return hashlib.sha256(json.dumps(canonical, sort_keys=True, separators=(',', ':')).encode()).hexdigest()


def require_preferences_bodies(instructions, complete=False):
    for start, end, count, expected in CONTRACTS:
        body = [r for r in instructions if start <= r['address'] < end]
        if not body and not complete: continue
        if len(body) != count or fingerprint(body) != expected:
            raise ValueError(f'Mac preferences contract changed or missing at {start:x}')


def require_preferences_edges(edges):
    for source, target in edges:
        for start, end, _, _ in CONTRACTS:
            if start < target < end and not start <= source < end:
                raise ValueError(f'External preferences interior edge from {source:x} to {target:x}')


def require_preferences_contract(page, instructions, selected=None):
    addresses = {r['address'] for r in instructions} | set(selected or ())
    for start, end, _, _ in CONTRACTS:
        if not start >> 12 <= page <= (end - 1) >> 12: continue
        low, high = max(start, page << 12), min(end, (page + 1) << 12)
        if start not in addresses and not any(low <= a < high for a in addresses): continue
        body = [r for r in instructions if low <= r['address'] < high]
        _, _, count, expected = next(f for f in FRAGMENTS if f[0] == start and f[1] == page)
        if len(body) != count or fingerprint(body) != expected:
            raise ValueError(f'Mac preferences contract changed or missing at {start:x} page {page:x}')


def require_preferences_index(db, selected=None):
    records = [json.loads(row[0]) for start, end, _, _ in CONTRACTS for row in db.execute(
        'SELECT body FROM instructions WHERE address>=? AND address<? ORDER BY address', (start, end))]
    if selected is None: require_preferences_bodies(records, complete=True)
    else:
        for page in sorted({f[1] for f in FRAGMENTS}):
            require_preferences_contract(page, [r for _, r in sorted(selected.items())], selected)
    conditions = ' OR '.join('(e.value>=? AND e.value<?)' for _ in CONTRACTS)
    parameters = [value for start, end, _, _ in CONTRACTS for value in (start, end)]
    require_preferences_edges(db.execute("""SELECT i.address,e.value
        FROM instructions AS i,
          json_each(json_insert(json_extract(i.body,'$.flows'),'$[#]',json_extract(i.body,'$.fallthrough'))) AS e
        WHERE """ + conditions, parameters))


TABLES = ((277570984, 276461756, 262, '53b065a0e17040059a77ab6bbf3b6d6398afcda638fa8b6af007b970a711ed0f'), (277562920, 280916600, 4, 'df3f619804a92fdb4057192dc43dd748ea778adc52bc498ce80524c014b81119'), (277563008, 277999068, 1, '6e340b9cffb37a989ca544e6bb780a2c78901d3fb33738768511a30617afa01d'), (277563016, 280945768, 4, 'df3f619804a92fdb4057192dc43dd748ea778adc52bc498ce80524c014b81119'), (277561056, 278458548, 4, 'df3f619804a92fdb4057192dc43dd748ea778adc52bc498ce80524c014b81119'), (277561080, 277997176, 4, 'df3f619804a92fdb4057192dc43dd748ea778adc52bc498ce80524c014b81119'), (277563012, 277856176, 4, 'bf906cd362964d265fdb27547a75d2ad2ce86cccec49cdc613764a77dc5f149d'), (277561060, 280982829, 1, '6e340b9cffb37a989ca544e6bb780a2c78901d3fb33738768511a30617afa01d'), (277561064, 278453760, 1, '6e340b9cffb37a989ca544e6bb780a2c78901d3fb33738768511a30617afa01d'))

def require_preferences_tables(read):
    for slot,base,size,expected in TABLES:
        if struct.unpack(">I",read(slot,4))[0]!=base or hashlib.sha256(read(base,size)).hexdigest()!=expected:
            raise ValueError("Changed original preference initialization table")

def load_preferences_tables(directory):
    program=json.loads((directory/"program.json").read_text())
    segments=[(s["address"],(directory/s["file"]).read_bytes() if s.get("file") else bytes(s["size"])) for s in program["segments"]]
    def read(a,n):
        for base,data in segments:
            if base<=a and a+n<=base+len(data):return data[a-base:a-base+n]
        raise ValueError("Missing original preferences table")
    require_preferences_tables(read)
    require_preferences_padding(read)
    require_preferences_words(read)
    return tuple(read(base,size) for _,base,size,_ in TABLES)+tuple(read(address,size) for address,size,_ in WORDS)


PADDING = ((269743108, '00000000'), (269743112, '00000000'), (269743116, '00000000'), (269743300, '00000000'), (269743304, '00000000'), (269743308, '00000000'), (269743460, '00000000'), (269743464, '00000000'), (269743468, '00000000'), (269743740, '00000000'), (269744984, '00000000'), (269744988, '00000000'), (269745220, '00000000'), (269745224, '00000000'), (269745228, '00000000'), (269745404, '00000000'), (269745652, '00000000'), (269745656, '00000000'), (269745660, '00000000'), (269745836, '00000000'), (269747924, '00000000'), (269747928, '00000000'), (269747932, '00000000'), (269747944, '00000000'), (269747948, '00000000'), (269748004, '00000000'), (269748008, '00000000'), (269748012, '00000000'))

def require_preferences_padding(read):
    for address,expected in PADDING:
        if read(address,4)!=bytes.fromhex(expected):
            raise ValueError("Changed nonindexed preferences alignment word")

WORDS = ()

def require_preferences_words(read):
    for address,size,expected in WORDS:
        if hashlib.sha256(read(address,size)).hexdigest()!=expected:
            raise ValueError("Changed original preference initialization")


def preferences_hook(entry):
    return f'if (try_modern_preferences(s, 0x{entry:x})) return true;'


def require_preferences_hooks(page, instructions, code):
    addresses = {r['address'] for r in instructions}
    for entry in PREFERENCES_ENTRIES:
        if entry >> 12 != page or entry not in addresses: continue
        label, hook = f'i_{entry:x}:', preferences_hook(entry)
        fragment = code.split(label, 1)[-1].split(f'i_{entry + 4:x}:', 1)[0]
        steps = [token for token in (f's.step(0x{entry:x});', f'counter.step(0x{entry:x});') if token in fragment]
        if code.count(hook) != 1 or hook not in fragment or len(steps) != 1 or fragment.index(hook) > fragment.index(steps[0]):
            raise ValueError(f'Missing or misplaced preferences hook at {entry:x}')
