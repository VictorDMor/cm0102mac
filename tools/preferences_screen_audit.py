"""Pinned complete original preferences presentation builder."""
import hashlib
import json
import struct

CONTRACTS = ((271831088, 271841344, 2562, '6759f7e3fde7f16410c58b0f9be5b8a347b0487589028212c1f8f1009149636e'),)
FRAGMENTS = ((271831088, 66365, 1010, '1676279eb5d1f92faf9db52c51559b196e8966425e2c87b3ed184e1a4d360dc8'), (271831088, 66366, 1024, '6c79d004454cac30c705aacc8c6a7532b5d75bbf84f1e6ece1c26f3c70d745ec'), (271831088, 66367, 528, '9a0ef46f3457bff23335e00bbdc43777506287f80782b017e8ce5327022a32a2'))
PREFERENCES_SCREEN_ENTRIES = tuple(start for start, _, _, _ in CONTRACTS)
PREPARED_PREFERENCES_SCREEN_ENTRIES = PREFERENCES_SCREEN_ENTRIES


def fingerprint(body):
    canonical = [{key: r.get(key) for key in ('address', 'assembly', 'ops', 'flows', 'fallthrough')} for r in body]
    return hashlib.sha256(json.dumps(canonical, sort_keys=True, separators=(',', ':')).encode()).hexdigest()


def require_preferences_screen_bodies(instructions, complete=False):
    for start, end, count, expected in CONTRACTS:
        body = [r for r in instructions if start <= r['address'] < end]
        if not body and not complete: continue
        if len(body) != count or fingerprint(body) != expected:
            raise ValueError(f'Mac preferences_screen contract changed or missing at {start:x}')


def require_preferences_screen_edges(edges):
    for source, target in edges:
        for start, end, _, _ in CONTRACTS:
            if start < target < end and not start <= source < end:
                raise ValueError(f'External preferences_screen interior edge from {source:x} to {target:x}')


def require_preferences_screen_contract(page, instructions, selected=None):
    addresses = {r['address'] for r in instructions} | set(selected or ())
    for start, end, _, _ in CONTRACTS:
        if not start >> 12 <= page <= (end - 1) >> 12: continue
        low, high = max(start, page << 12), min(end, (page + 1) << 12)
        if start not in addresses and not any(low <= a < high for a in addresses): continue
        body = [r for r in instructions if low <= r['address'] < high]
        _, _, count, expected = next(f for f in FRAGMENTS if f[0] == start and f[1] == page)
        if len(body) != count or fingerprint(body) != expected:
            raise ValueError(f'Mac preferences_screen contract changed or missing at {start:x} page {page:x}')


def require_preferences_screen_index(db, selected=None):
    records = [json.loads(row[0]) for start, end, _, _ in CONTRACTS for row in db.execute(
        'SELECT body FROM instructions WHERE address>=? AND address<? ORDER BY address', (start, end))]
    if selected is None: require_preferences_screen_bodies(records, complete=True)
    else:
        for page in sorted({f[1] for f in FRAGMENTS}):
            require_preferences_screen_contract(page, [r for _, r in sorted(selected.items())], selected)
    conditions = ' OR '.join('(e.value>=? AND e.value<?)' for _ in CONTRACTS)
    parameters = [value for start, end, _, _ in CONTRACTS for value in (start, end)]
    require_preferences_screen_edges(db.execute("""SELECT i.address,e.value
        FROM instructions AS i,
          json_each(json_insert(json_extract(i.body,'$.flows'),'$[#]',json_extract(i.body,'$.fallthrough'))) AS e
        WHERE """ + conditions, parameters))


TABLES = ((277561020, 280982064, 2, '96a296d224f285c67bee93c30f8a309157f0daa35dc5b87e410b78630a09cfc7'), (277561028, 280982048, 2, '96a296d224f285c67bee93c30f8a309157f0daa35dc5b87e410b78630a09cfc7'), (277561024, 280982052, 2, '96a296d224f285c67bee93c30f8a309157f0daa35dc5b87e410b78630a09cfc7'), (277561036, 280982026, 2, '96a296d224f285c67bee93c30f8a309157f0daa35dc5b87e410b78630a09cfc7'), (277561048, 280918704, 1, '6e340b9cffb37a989ca544e6bb780a2c78901d3fb33738768511a30617afa01d'), (277561056, 278458548, 4, 'df3f619804a92fdb4057192dc43dd748ea778adc52bc498ce80524c014b81119'), (277572728, 276860692, 3115, 'a1d828ad0e5216e9773e547139399e7423436ede63caea4bb33f546cf5f91be6'), (277572564, 276860596, 5, '7765bd6e9d8bce80b9b3a335f977bb132a5799a894caed357995ef75f44a1d31'), (277572560, 276860604, 5, '0d790ba321919a97cb3a0366e161f17e29616ea293abb14beb1617ca471e0a21'), (277561032, 280982062, 2, '96a296d224f285c67bee93c30f8a309157f0daa35dc5b87e410b78630a09cfc7'), (277561216, 280982022, 2, '96a296d224f285c67bee93c30f8a309157f0daa35dc5b87e410b78630a09cfc7'), (277572556, 276870680, 4, '6f7761f18e878134def35caee5c589591701547af7fe9d07aeb5e9cca3f9b6d4'), (277567440, 277747512, 44, 'b3dbbf72b95c67a7b42b6e564e87ca28a00df44c999581e1bbc308145b1bdb34'), (277561080, 277997176, 4, 'df3f619804a92fdb4057192dc43dd748ea778adc52bc498ce80524c014b81119'))
LANGUAGE_NAMES = ((277455798, 8, 'fea288faa0c191efbf7289ec4c86890f29f6b58908374ab1970abe8fc9d68e83'), (277455806, 13, '91a67ad6ddfcd4c9a201e96085f9d0e8772cd15149e66821b82f89ec3c90fc2b'), (277455819, 9, '3d55f14d5b63a14d2fd6589d1c4b9275490af423326fd763cb7cde568e140d40'), (277455828, 8, '6ddb09f5fbf83be07ad8f740ebc1b9ec6bb4f0e75659b891cd8d5c232b08045f'), (277455836, 9, '84ce80aa3a42eab54db37c2682b2c45f9f75effcd73ca5a82d37f4edf12b9ae9'), (277455845, 8, '1be07cb9c4fc6267f0df8f47dee591b2198760c56275df88884b59f6d2b30cba'), (277455853, 6, '13a3943029a3960c62ecea48bc09f46b7c7ee256712a33c876147a057d0e8c8e'), (277455859, 11, '26b0879c74a1c84d7c143ddd27fd5813832c1259a300539f63a77e1bfbf6b61f'), (277455870, 6, 'd6da365d54f7eacd8d5289a126882d7be6ccfa5bccc6f7b0cd8f0f249f24a1d1'), (277455876, 10, '55585cae049e30211596fcfa5649ce489a9dd4d1318c4d8416d3e2c804bafbae'), (277455886, 8, '718e1bad572bf6dc4e1500edd369432b4c2e20308f320f044caee6a5eff756ad'))

def require_preferences_screen_tables(read):
    for slot,base,size,expected in TABLES:
        if struct.unpack(">I",read(slot,4))[0]!=base or hashlib.sha256(read(base,size)).hexdigest()!=expected:
            raise ValueError("Changed original preference initialization table")
    for address,size,expected in LANGUAGE_NAMES:
        if hashlib.sha256(read(address,size)).hexdigest()!=expected:
            raise ValueError("Changed original language presentation name")

def load_preferences_screen_tables(directory):
    program=json.loads((directory/"program.json").read_text())
    segments=[(s["address"],(directory/s["file"]).read_bytes() if s.get("file") else bytes(s["size"])) for s in program["segments"]]
    def read(a,n):
        for base,data in segments:
            if base<=a and a+n<=base+len(data):return data[a-base:a-base+n]
        raise ValueError("Missing original preferences_screen table")
    require_preferences_screen_tables(read)
    require_preferences_screen_padding(read)
    require_preferences_screen_words(read)
    return (tuple(read(base,size) for _,base,size,_ in TABLES)
            +tuple(read(address,size) for address,size,_ in LANGUAGE_NAMES)
            +tuple(read(address,size) for address,size,_ in WORDS))


PADDING = ((271831272, '60000000'), (271832496, '60000000'))

def require_preferences_screen_padding(read):
    for address,expected in PADDING:
        if read(address,4)!=bytes.fromhex(expected):
            raise ValueError("Changed nonindexed preferences_screen alignment word")

WORDS = ((277578238, 1, '6e340b9cffb37a989ca544e6bb780a2c78901d3fb33738768511a30617afa01d'), (277578240, 2, '96a296d224f285c67bee93c30f8a309157f0daa35dc5b87e410b78630a09cfc7'), (277578236, 2, '67294d0eff78c6dbf4ae91576d495f81ca8f9967119cff318132f79d41285dfc'))

def require_preferences_screen_words(read):
    for address,size,expected in WORDS:
        if hashlib.sha256(read(address,size)).hexdigest()!=expected:
            raise ValueError("Changed original preference initialization")


def preferences_screen_hook(entry):
    return f'if (try_modern_preferences_screen(s, 0x{entry:x})) return true;'


def require_preferences_screen_hooks(page, instructions, code):
    addresses = {r['address'] for r in instructions}
    for entry in PREFERENCES_SCREEN_ENTRIES:
        if entry >> 12 != page or entry not in addresses: continue
        label, hook = f'i_{entry:x}:', preferences_screen_hook(entry)
        fragment = code.split(label, 1)[-1].split(f'i_{entry + 4:x}:', 1)[0]
        steps = [token for token in (f's.step(0x{entry:x});', f'counter.step(0x{entry:x});') if token in fragment]
        if code.count(hook) != 1 or hook not in fragment or len(steps) != 1 or fragment.index(hook) > fragment.index(steps[0]):
            raise ValueError(f'Missing or misplaced preferences_screen hook at {entry:x}')
