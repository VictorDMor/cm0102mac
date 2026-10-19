"""Pinned complete localization catalog lifecycle and initialization operations."""
import hashlib
import json
import struct

CONTRACTS = ((275934400, 275934468, 17, '474185883daf8023d8124768a3a0be77fc6ac43a9e6a82bacb65d275a696245b'), (275934480, 275934616, 34, 'b55e47827d685c7067aefdb9836fdb3830d0df8fb3e517d812b67cff61d11ae3'), (275934624, 275934856, 58, '9147bbaf57d006de25a4d9716edb7217530c4727e97923c72a07a4fda6700e8e'), (275934864, 275937364, 625, '82078e9222b95d5fbad76a272c9ac00799765ca037d61be0e261a083c897db13'))
FRAGMENTS = ((275934400, 67366, 17, '474185883daf8023d8124768a3a0be77fc6ac43a9e6a82bacb65d275a696245b'), (275934480, 67366, 34, 'b55e47827d685c7067aefdb9836fdb3830d0df8fb3e517d812b67cff61d11ae3'), (275934624, 67366, 58, '9147bbaf57d006de25a4d9716edb7217530c4727e97923c72a07a4fda6700e8e'), (275934864, 67366, 92, 'c14b3bfb5ae4f67b9129dbe0d3d9d86034bb8969ce4fdaf7bb589c6e337ee348'), (275934864, 67367, 533, '1923a01d24de37987920cb8262fb5ea1e293b610007cadf3a8590797943803d6'))
LOCALIZATION_CATALOG_ENTRIES = tuple(start for start, _, _, _ in CONTRACTS)
PREPARED_LOCALIZATION_CATALOG_ENTRIES = LOCALIZATION_CATALOG_ENTRIES


def fingerprint(body):
    canonical = [{key: r.get(key) for key in ('address', 'assembly', 'ops', 'flows', 'fallthrough')} for r in body]
    return hashlib.sha256(json.dumps(canonical, sort_keys=True, separators=(',', ':')).encode()).hexdigest()


def require_localization_catalog_bodies(instructions, complete=False):
    for start, end, count, expected in CONTRACTS:
        body = [r for r in instructions if start <= r['address'] < end]
        if not body and not complete: continue
        if len(body) != count or fingerprint(body) != expected:
            raise ValueError(f'Mac localization_catalog contract changed or missing at {start:x}')


def require_localization_catalog_edges(edges):
    for source, target in edges:
        for start, end, _, _ in CONTRACTS:
            if start < target < end and not start <= source < end:
                raise ValueError(f'External localization_catalog interior edge from {source:x} to {target:x}')


def require_localization_catalog_contract(page, instructions, selected=None):
    addresses = {r['address'] for r in instructions} | set(selected or ())
    for start, end, _, _ in CONTRACTS:
        if not start >> 12 <= page <= (end - 1) >> 12: continue
        low, high = max(start, page << 12), min(end, (page + 1) << 12)
        if start not in addresses and not any(low <= a < high for a in addresses): continue
        body = [r for r in instructions if low <= r['address'] < high]
        _, _, count, expected = next(f for f in FRAGMENTS if f[0] == start and f[1] == page)
        if len(body) != count or fingerprint(body) != expected:
            raise ValueError(f'Mac localization_catalog contract changed or missing at {start:x} page {page:x}')


def require_localization_catalog_index(db, selected=None):
    records = [json.loads(row[0]) for start, end, _, _ in CONTRACTS for row in db.execute(
        'SELECT body FROM instructions WHERE address>=? AND address<? ORDER BY address', (start, end))]
    if selected is None: require_localization_catalog_bodies(records, complete=True)
    else:
        for page in sorted({f[1] for f in FRAGMENTS}):
            require_localization_catalog_contract(page, [r for _, r in sorted(selected.items())], selected)
    conditions = ' OR '.join('(e.value>=? AND e.value<?)' for _ in CONTRACTS)
    parameters = [value for start, end, _, _ in CONTRACTS for value in (start, end)]
    require_localization_catalog_edges(db.execute("""SELECT i.address,e.value
        FROM instructions AS i,
          json_each(json_insert(json_extract(i.body,'$.flows'),'$[#]',json_extract(i.body,'$.fallthrough'))) AS e
        WHERE """ + conditions, parameters))


TABLES = ((277576688, 278455980, 550, '25778f162c4243167f8eaa876f1b0619e67afc158de7805600471a563ec5e8b7'), (277561056, 278458548, 4, 'df3f619804a92fdb4057192dc43dd748ea778adc52bc498ce80524c014b81119'), (277576692, 278456572, 12, '15ec7bf0b50732b49f8228e07d24365338f9e3ab994b00af08e5a3bffe55fd8b'), (277561060, 280982829, 1, '6e340b9cffb37a989ca544e6bb780a2c78901d3fb33738768511a30617afa01d'), (277561064, 278453760, 512, '076a27c79e5ace2a3d47f9dd2e83e4ff6ea8872b3c2218f66c92b89b55f36560'), (277576696, 277455748, 1024, 'af657c671e27d8e901c3acc4b3ea6798fe897a17abdbae82b7e6c45952bb854e'), (277569224, 277599716, 8, 'f19b67065071b0b4884aa35a12a357e6d6dd954fdfecee7c6634fc179120f710'), (277569220, 277599708, 8, 'f145926b56e6f85319d4249f3a165d4ae05268510a70fbce933f517e51a5d411'), (277576708, 278456584, 256, '5341e6b2646979a70e57653007a1f310169421ec9bdd9f1a5648f75ade005af1'), (277576704, 278457236, 128, '38723a2e5e8a17aa7950dc008209944e898f69a7bd10a23c839d341e935fd5ca'), (277576684, 277599700, 8, 'd4560123ab042216c7d43266c2ca6a375e4ae43b99ff3ca198a11fd64f0575e2'))

def require_localization_catalog_tables(read):
    for slot,base,size,expected in TABLES:
        if struct.unpack(">I",read(slot,4))[0]!=base or hashlib.sha256(read(base,size)).hexdigest()!=expected:
            raise ValueError("Changed original localization lookup table")

def load_localization_catalog_tables(directory):
    program=json.loads((directory/"program.json").read_text())
    segments=[(s["address"],(directory/s["file"]).read_bytes() if s.get("file") else bytes(s["size"])) for s in program["segments"]]
    def read(a,n):
        for base,data in segments:
            if base<=a and a+n<=base+len(data):return data[a-base:a-base+n]
        raise ValueError("Missing original localization_catalog table")
    require_localization_catalog_tables(read)
    require_localization_catalog_padding(read)
    require_localization_catalog_words(read)
    return tuple(read(base,size) for _,base,size,_ in TABLES)+tuple(read(address,size) for address,size,_ in WORDS)


PADDING = ((275934468, '00000000'), (275934472, '00000000'), (275934476, '00000000'), (275934616, '00000000'), (275934620, '00000000'), (275934856, '00000000'), (275934860, '00000000'), (275937364, '00000000'), (275937368, '00000000'), (275937372, '00000000'))

def require_localization_catalog_padding(read):
    for address,expected in PADDING:
        if read(address,4)!=bytes.fromhex(expected):
            raise ValueError("Changed nonindexed localization_catalog alignment word")

WORDS = ((277579008, 4, 'df3f619804a92fdb4057192dc43dd748ea778adc52bc498ce80524c014b81119'), (277579004, 4, 'df3f619804a92fdb4057192dc43dd748ea778adc52bc498ce80524c014b81119'), (277579000, 4, 'df3f619804a92fdb4057192dc43dd748ea778adc52bc498ce80524c014b81119'), (277578992, 4, 'df3f619804a92fdb4057192dc43dd748ea778adc52bc498ce80524c014b81119'), (277578988, 4, 'df3f619804a92fdb4057192dc43dd748ea778adc52bc498ce80524c014b81119'), (277578996, 1, '6e340b9cffb37a989ca544e6bb780a2c78901d3fb33738768511a30617afa01d'), (277578980, 1, 'a8100ae6aa1940d0b663bb31cd466142ebbdbd5187131b92d93818987832eb89'))

def require_localization_catalog_words(read):
    for address,size,expected in WORDS:
        if hashlib.sha256(read(address,size)).hexdigest()!=expected:
            raise ValueError("Changed original localization lookup data")


def localization_catalog_hook(entry):
    return f'if (try_modern_localization_catalog(s, 0x{entry:x})) return true;'


def require_localization_catalog_hooks(page, instructions, code):
    addresses = {r['address'] for r in instructions}
    for entry in LOCALIZATION_CATALOG_ENTRIES:
        if entry >> 12 != page or entry not in addresses: continue
        label, hook = f'i_{entry:x}:', localization_catalog_hook(entry)
        fragment = code.split(label, 1)[-1].split(f'i_{entry + 4:x}:', 1)[0]
        steps = [token for token in (f's.step(0x{entry:x});', f'counter.step(0x{entry:x});') if token in fragment]
        if code.count(hook) != 1 or hook not in fragment or len(steps) != 1 or fragment.index(hook) > fragment.index(steps[0]):
            raise ValueError(f'Missing or misplaced localization_catalog hook at {entry:x}')
