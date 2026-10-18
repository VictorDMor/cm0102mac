"""Pinned complete money text formatting, labels and settings operations."""
import hashlib
import json
import struct

CONTRACTS = ((268476144, 268476184, 10, '3ea95f1d22bb233d1f4eadc631309ba609fc6de9a0d3217d27b6c9999e5fb7d2'), (268480848, 268480856, 2, 'faa7f5d1635a96f6d8bfc9965b832bc044bd833a9cdf3097255a0cf607c0571e'), (268480864, 268481552, 172, '561d6e7e181ddea5ec469bf182a23b385d22fe8af36f49ae851190b69b0efa09'), (268481552, 268486500, 1236, '7c162092a49887dee9415e8909991ca11825c400cf6b9f9773a16a49fba3d9ec'))
FRAGMENTS = ((268476144, 65545, 10, '3ea95f1d22bb233d1f4eadc631309ba609fc6de9a0d3217d27b6c9999e5fb7d2'), (268480848, 65547, 2, 'faa7f5d1635a96f6d8bfc9965b832bc044bd833a9cdf3097255a0cf607c0571e'), (268480864, 65547, 172, '561d6e7e181ddea5ec469bf182a23b385d22fe8af36f49ae851190b69b0efa09'), (268481552, 65547, 763, '115670f56c828cff30398a3c564282379c2202dc66eba4a62801e99fae3f5dec'), (268481552, 65548, 473, '1cefae5fb8bbc9ea959d3e6e8786a386bf9cc52426b16ddfa3febf8cdec3c2a6'))
MONEY_TEXT_ENTRIES = tuple(start for start, _, _, _ in CONTRACTS)
PREPARED_MONEY_TEXT_ENTRIES = MONEY_TEXT_ENTRIES


def fingerprint(body):
    canonical = [{key: r.get(key) for key in ('address', 'assembly', 'ops', 'flows', 'fallthrough')} for r in body]
    return hashlib.sha256(json.dumps(canonical, sort_keys=True, separators=(',', ':')).encode()).hexdigest()


def require_money_text_bodies(instructions, complete=False):
    for start, end, count, expected in CONTRACTS:
        body = [r for r in instructions if start <= r['address'] < end]
        if not body and not complete: continue
        if len(body) != count or fingerprint(body) != expected:
            raise ValueError(f'Mac money-text contract changed or missing at {start:x}')


def require_money_text_edges(edges):
    for source, target in edges:
        for start, end, _, _ in CONTRACTS:
            if start < target < end and not start <= source < end:
                raise ValueError(f'External money-text interior edge from {source:x} to {target:x}')


def require_money_text_contract(page, instructions, selected=None):
    addresses = {r['address'] for r in instructions} | set(selected or ())
    for start, end, _, _ in CONTRACTS:
        if not start >> 12 <= page <= (end - 1) >> 12: continue
        low, high = max(start, page << 12), min(end, (page + 1) << 12)
        if start not in addresses and not any(low <= a < high for a in addresses): continue
        body = [r for r in instructions if low <= r['address'] < high]
        _, _, count, expected = next(f for f in FRAGMENTS if f[0] == start and f[1] == page)
        if len(body) != count or fingerprint(body) != expected:
            raise ValueError(f'Mac money-text contract changed or missing at {start:x} page {page:x}')


def require_money_text_index(db, selected=None):
    records = [json.loads(row[0]) for start, end, _, _ in CONTRACTS for row in db.execute(
        'SELECT body FROM instructions WHERE address>=? AND address<? ORDER BY address', (start, end))]
    if selected is None: require_money_text_bodies(records, complete=True)
    else:
        for page in sorted({f[1] for f in FRAGMENTS}):
            require_money_text_contract(page, [r for _, r in sorted(selected.items())], selected)
    conditions = ' OR '.join('(e.value>=? AND e.value<?)' for _ in CONTRACTS)
    parameters = [value for start, end, _, _ in CONTRACTS for value in (start, end)]
    require_money_text_edges(db.execute("""SELECT i.address,e.value
        FROM instructions AS i,
          json_each(json_insert(json_extract(i.body,'$.flows'),'$[#]',json_extract(i.body,'$.fallthrough'))) AS e
        WHERE """ + conditions, parameters))


TABLES = ((277569684, 277642660, 128, 'd9557b12fbaf96fe8d309520bf38d691682870910c3163d9c527e03c8f1a011c'), (277569688, 277642940, 100, 'a5fd5836dd438c545ae042c76cc9e5915d64126e41906f537d5de6abfced1cb7'), (277569680, 277643104, 64, '2d59eddb4124c857b7646d049e1a0483f7af9d89f45dd73b4f453173c3bd2d91'), (277569676, 277643040, 64, '620084037574d97c714844cf4e644eb616ab382c6d5f23e8e5606ccaf83cc6f9'), (277569720, 276310876, 856, 'bb6ab5e83a1267ea6a304f2dd535a138a7edac831c5b8a949cef065caeaf0a71'), (277569696, 277991152, 52, '7955cb2de90dd9efc6df9fdbf5f5d10c114f4135a9a6b52db1003be749e32f7a'), (277569700, 277991204, 52, '7955cb2de90dd9efc6df9fdbf5f5d10c114f4135a9a6b52db1003be749e32f7a'), (277569704, 277991256, 52, '7955cb2de90dd9efc6df9fdbf5f5d10c114f4135a9a6b52db1003be749e32f7a'), (277569708, 277991308, 52, '7955cb2de90dd9efc6df9fdbf5f5d10c114f4135a9a6b52db1003be749e32f7a'), (277569712, 277991360, 52, '7955cb2de90dd9efc6df9fdbf5f5d10c114f4135a9a6b52db1003be749e32f7a'), (277569728, 277991412, 52, '7955cb2de90dd9efc6df9fdbf5f5d10c114f4135a9a6b52db1003be749e32f7a'))

def require_money_text_tables(read):
    for slot,base,size,expected in TABLES:
        if struct.unpack(">I",read(slot,4))[0]!=base or hashlib.sha256(read(base,size)).hexdigest()!=expected:
            raise ValueError("Changed original monetary value table")

def load_money_text_tables(directory):
    program=json.loads((directory/"program.json").read_text())
    segments=[(s["address"],(directory/s["file"]).read_bytes() if s.get("file") else bytes(s["size"])) for s in program["segments"]]
    def read(a,n):
        for base,data in segments:
            if base<=a and a+n<=base+len(data):return data[a-base:a-base+n]
        raise ValueError("Missing original money-text table")
    require_money_text_tables(read)
    require_money_text_padding(read)
    require_money_text_words(read)
    return tuple(read(base,size) for _,base,size,_ in TABLES)+tuple(read(address,size) for address,size,_ in WORDS)


PADDING = ((268476184, '00000000'), (268476188, '00000000'), (268480856, '00000000'), (268480860, '00000000'), (268481828, '60000000'), (268486500, '00000000'), (268486504, '00000000'), (268486508, '00000000'))

def require_money_text_padding(read):
    for address,expected in PADDING:
        if read(address,4)!=bytes.fromhex(expected):
            raise ValueError("Changed nonindexed money-text alignment word")

WORDS = ((277577614, 4, '81ff65efc4487853bdb4625559e69ab44f19e0f5efbd6d5b2af5e3ab267c8e06'),)

def require_money_text_words(read):
    for address,size,expected in WORDS:
        if hashlib.sha256(read(address,size)).hexdigest()!=expected:
            raise ValueError("Changed original monetary scale initialization")


def money_text_hook(entry):
    return f'if (try_modern_money_text(s, 0x{entry:x})) return true;'


def require_money_text_hooks(page, instructions, code):
    addresses = {r['address'] for r in instructions}
    for entry in MONEY_TEXT_ENTRIES:
        if entry >> 12 != page or entry not in addresses: continue
        label, hook = f'i_{entry:x}:', money_text_hook(entry)
        fragment = code.split(label, 1)[-1].split(f'i_{entry + 4:x}:', 1)[0]
        steps = [token for token in (f's.step(0x{entry:x});', f'counter.step(0x{entry:x});') if token in fragment]
        if code.count(hook) != 1 or hook not in fragment or len(steps) != 1 or fragment.index(hook) > fragment.index(steps[0]):
            raise ValueError(f'Missing or misplaced money-text hook at {entry:x}')
