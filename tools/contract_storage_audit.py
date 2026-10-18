"""Pinned complete contract storage creation scope; initializer implementation pending."""
import hashlib
import json
import struct

CONTRACTS = ((268930672, 268930808, 34, '96a249ddc2b00ae5b8378bb68555c6e04fc33370481c9faa99a770e5c7ab8ebc'), (268930816, 268931144, 82, '4684d8fb2895b578ffcd256ce1056b6ac98e98bc74a4f22f9c7bf55c76415552'), (272050112, 272057712, 1900, '41f20d8cf1998f947d031ea3a1415f128f30833dfb6b30fb4be10940b1165b10'))
FRAGMENTS = ((268930672, 65656, 34, '96a249ddc2b00ae5b8378bb68555c6e04fc33370481c9faa99a770e5c7ab8ebc'), (268930816, 65656, 64, '31792d1bd0d7724952fc1e2948041473db13d5c01f9d31f52305387fe137466b'), (268930816, 65657, 18, 'ee786ed5467c693bb95e486abd9adbdcc6b9815b85a94cceb6cdfe03952522ea'), (272050112, 66418, 528, 'ab7aa43529fa5d358071d10f38709fcbd5d9d8636de7e9a20a5da1f8b03a4a1c'), (272050112, 66419, 1024, '656724a38d13c95ac6ed83ba8ffbefd091ab787021a04d1f18422da1d1f9bb89'), (272050112, 66420, 348, '1f352c7da8f1bdf84632de25ea5d2d56b7358d7c5e51ea29be7c5c7b9fe628f8'))
CONTRACT_STORAGE_ENTRIES = tuple(start for start, _, _, _ in CONTRACTS)
PREPARED_CONTRACT_STORAGE_ENTRIES = CONTRACT_STORAGE_ENTRIES[:2]


def fingerprint(body):
    canonical = [{key: r.get(key) for key in ('address', 'assembly', 'ops', 'flows', 'fallthrough')} for r in body]
    return hashlib.sha256(json.dumps(canonical, sort_keys=True, separators=(',', ':')).encode()).hexdigest()


def require_contract_storage_bodies(instructions, complete=False):
    for start, end, count, expected in CONTRACTS:
        body = [r for r in instructions if start <= r['address'] < end]
        if not body and not complete: continue
        if len(body) != count or fingerprint(body) != expected:
            raise ValueError(f'Mac contract-storage contract changed or missing at {start:x}')


def require_contract_storage_edges(edges):
    for source, target in edges:
        for start, end, _, _ in CONTRACTS:
            if start < target < end and not start <= source < end:
                raise ValueError(f'External contract-storage interior edge from {source:x} to {target:x}')


def require_contract_storage_contract(page, instructions, selected=None):
    addresses = {r['address'] for r in instructions} | set(selected or ())
    for start, end, _, _ in CONTRACTS:
        if not start >> 12 <= page <= (end - 1) >> 12: continue
        low, high = max(start, page << 12), min(end, (page + 1) << 12)
        if start not in addresses and not any(low <= a < high for a in addresses): continue
        body = [r for r in instructions if low <= r['address'] < high]
        _, _, count, expected = next(f for f in FRAGMENTS if f[0] == start and f[1] == page)
        if len(body) != count or fingerprint(body) != expected:
            raise ValueError(f'Mac contract-storage contract changed or missing at {start:x} page {page:x}')


def require_contract_storage_index(db, selected=None):
    records = [json.loads(row[0]) for start, end, _, _ in CONTRACTS for row in db.execute(
        'SELECT body FROM instructions WHERE address>=? AND address<? ORDER BY address', (start, end))]
    if selected is None: require_contract_storage_bodies(records, complete=True)
    else:
        for page in sorted({f[1] for f in FRAGMENTS}):
            require_contract_storage_contract(page, [r for _, r in sorted(selected.items())], selected)
    conditions = ' OR '.join('(e.value>=? AND e.value<?)' for _ in CONTRACTS)
    parameters = [value for start, end, _, _ in CONTRACTS for value in (start, end)]
    require_contract_storage_edges(db.execute("""SELECT i.address,e.value
        FROM instructions AS i,
          json_each(json_insert(json_extract(i.body,'$.flows'),'$[#]',json_extract(i.body,'$.fallthrough'))) AS e
        WHERE """ + conditions, parameters))


TABLES = ((277570424, 276338664, 552, 'c1e372e52a2e4ee62372de9d33ca18cbd45e9f64c3bd11b2445fdab6f9d56b6a'), (277572836, 276874552, 208, '569ff0266824b3a88db5909fe26760bfa5b58bfeb3d75d0b9f4d1b967465545b'), (277572840, 276874160, 20, 'a4485dea57ec7aa586616c83d4ae1ca1469c9c2ef20cabdbb5c14a052e18632b'))
PADDING = ((0x10078ef8, 0), (0x10078efc, 0))

def require_contract_storage_tables(word):
    for slot,base,size,expected in TABLES:
        data=b''.join(struct.pack('>I',word(base+n)) for n in range(0,size,4))
        if word(slot)!=base or hashlib.sha256(data).hexdigest()!=expected:
            raise ValueError('Changed contract storage diagnostic table')
    for address,expected in PADDING:
        if word(address)!=expected:raise ValueError('Changed contract storage padding word')

def load_contract_storage_tables(input_directory):
    program=json.loads((input_directory/'program.json').read_text())
    segments=[(s['address'],(input_directory/s['file']).read_bytes()) for s in program['segments'] if s.get('file')]
    def word(address):
        for base,data in segments:
            if base<=address and address+4<=base+len(data):return struct.unpack_from('>I',data,address-base)[0]
        raise ValueError('Missing contract storage table word')
    require_contract_storage_tables(word)
    return tuple(tuple(word(base+n) for n in range(0,size,4)) for _,base,size,_ in TABLES)
