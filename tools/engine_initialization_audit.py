"""Complete engine root and its nineteen global initializers."""
import hashlib
import json

CONTRACTS = (
    (0x10000000, 0x10000068, 26, '98aeeb7752286379c7fde9a8a0eb88432bbed36d37deaa1badc70e1c13bb54ed'),
    (0x10064770, 0x100647d0, 24, 'bf7726f65414f49d477b17ead5523d53537ca745e9e5cc8dcf21097fedbd8b23'),
    (0x100939b0, 0x10093a4c, 39, 'c68c93b460d5b924f5834a99a424021601e4b3d3356dd0463ff4bcb7b40b37da'),
    (0x100de3b0, 0x100de408, 22, '348683c0932527bd1be0a350bb4f3c3b5441ff627546a9586076e5a7ac9cedaf'),
    (0x1013f000, 0x1013f02c, 11, 'a80a240f58a0a0cc827f2f6e3fb7cebb36705970415c2fda4f8d7983b8d0f349'),
    (0x10143240, 0x10143274, 13, 'cd33f817f8e3bd8391e36c1894f5cc2009a42916f165ee5fdde69f47c8cb6024'),
    (0x101935b0, 0x101935e4, 13, 'aa6ed8706f3d98406737390cca443382f2e77b118a26ac8ffab1b7c0c47ed701'),
    (0x102aac90, 0x102aacd4, 17, 'c5a272a5ca0169d108e2f7373e40fa5530976f1a434b6d220bb3ba6f620030ec'),
    (0x1030d470, 0x1030d49c, 11, '2e4f1d07253ce748d226889277e81c09745d388acd1e9cdd58a1f6b6fceecce5'),
    (0x10315050, 0x10315080, 12, '01c7d4d89bbe1b7fc98d72f33a1afaed065997b9271c392e2ec44537123629d4'),
    (0x103563e0, 0x1035640c, 11, 'c796c3adee01aa4eb08b0fb504e11c11cca4c8d37b428ab117e433d3325932e5'),
    (0x103810d0, 0x10381104, 13, '1287a8cbf6c3a2d6bc9e8fe6ed5a24f29b31e4b17777b23fddf6f05c58c2dee0'),
    (0x1048eb80, 0x1048ebb0, 12, '59fd7683e1764a4ff7fe2c4a7e3f6f9c7d90bbc65dd0d43d6016aa29ed549853'),
    (0x106b6e60, 0x106b6e8c, 11, '46b37f02054f4962df57ca554c6528439692e0c2cb273fd0c9dd13666a347174'),
    (0x106bbe10, 0x106bbe3c, 11, 'd0ca5e75d96becd47a0aed5050f9fbb80b06ed1f258666de2f693d3dda40d958'),
    (0x107259a0, 0x107259fc, 23, 'dfeef0130947df64e8719afb91c90bc8cb97e9a9dd9b49fad93218b9835f343d'),
    (0x10739f30, 0x10739f68, 14, '475327c5ba084573c9cf240ce2f430a726430f8677b89a3ca47ff99342a0ca66'),
    (0x10744640, 0x10744670, 12, 'd97b9b77fb2d3095f5da84b38e3311b5d2255e65b0880c6e6ef5c35429f4a831'),
    (0x1074d340, 0x1074d37c, 15, '92457175436da5e390d2f4d209578c9da6753bed5df53d30f2f7af5d0530314a'),
    (0x1076ea20, 0x1076ea64, 17, 'b541e25e416dbbd000f97a68368173de75c8860a85f2177da65006a3e5d0f698'),
)
ENGINE_INITIALIZATION_ENTRIES = tuple(start for start, _, _, _ in CONTRACTS)


def fingerprint(body):
    canonical = [{key: r.get(key) for key in ('address', 'assembly', 'ops', 'flows', 'fallthrough')} for r in body]
    return hashlib.sha256(json.dumps(canonical, sort_keys=True, separators=(',', ':')).encode()).hexdigest()


def require_engine_initialization_bodies(instructions, complete=False):
    for start, end, count, expected in CONTRACTS:
        body = [r for r in instructions if start <= r['address'] < end]
        if not body and not complete: continue
        if len(body) != count or fingerprint(body) != expected:
            raise ValueError(f'Mac engine-initialization contract changed or missing at {start:x}')


def require_engine_initialization_edges(edges):
    for source, target in edges:
        for start, end, _, _ in CONTRACTS:
            if start < target < end and not start <= source < end:
                raise ValueError(f'External engine-initialization interior edge from {source:x} to {target:x}')


def require_engine_initialization_contract(page, instructions, selected=None):
    addresses = {r['address'] for r in instructions} | set(selected or ())
    for start, end, count, expected in CONTRACTS:
        if start >> 12 != page or not any(start <= a < end for a in addresses): continue
        body = [r for r in instructions if start <= r['address'] < end]
        if len(body) != count or fingerprint(body) != expected:
            raise ValueError(f'Mac engine-initialization contract changed or missing at {start:x}')


def require_engine_initialization_index(db, selected=None):
    records = [json.loads(row[0]) for start, end, _, _ in CONTRACTS for row in db.execute(
        'SELECT body FROM instructions WHERE address>=? AND address<? ORDER BY address', (start, end))]
    if selected is None: require_engine_initialization_bodies(records, complete=True)
    else:
        for page in (65536, 65636, 65683, 65758, 65855, 65859, 65939, 66218, 66317, 66325, 66390, 66433, 66702, 67254, 67259, 67365, 67385, 67396, 67405, 67438):
            require_engine_initialization_contract(page, [r for _, r in sorted(selected.items())], selected)
    require_engine_initialization_edges(db.execute("""SELECT i.address,e.value
        FROM instructions AS i,
          json_each(json_insert(json_extract(i.body,'$.flows'),'$[#]',json_extract(i.body,'$.fallthrough'))) AS e
        WHERE e.value>=? AND e.value<?""", (CONTRACTS[0][0], CONTRACTS[-1][1])))


def engine_initialization_hook(entry):
    return f'if (try_modern_engine_initialization(s, 0x{entry:x})) return true;'


def require_engine_initialization_hooks(page, instructions, code):
    addresses = {r['address'] for r in instructions}
    for entry in ENGINE_INITIALIZATION_ENTRIES:
        if entry >> 12 != page or entry not in addresses: continue
        label, hook, step = f'i_{entry:x}:', engine_initialization_hook(entry), f's.step(0x{entry:x});'
        fragment = code.split(label, 1)[-1].split(f'i_{entry + 4:x}:', 1)[0]
        if code.count(hook) != 1 or hook not in fragment or step not in fragment or fragment.index(hook) > fragment.index(step):
            raise ValueError(f'Missing or misplaced engine-initialization hook at {entry:x}')
