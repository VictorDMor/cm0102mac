"""Pinned complete contract terms, presentation and expiry-band scope."""
import hashlib
import json
import struct

CONTRACTS = ((272057712, 272059112, 350, '0156a76ec467aabbed2214e84f6733d9887ee20b91a2e75e494b54b8d0c98c01'), (272059120, 272059188, 17, '2ace967de346c0606ca6587c04ef4466c29219929035bcd4da5bee442cc2b1b1'), (272059200, 272059220, 5, '9c9d26ca165c1b3a3a356da67966e1926cf5afccd48be6c87f37acbed716537d'), (272059232, 272059252, 5, '1c18c081b7b30a749b7407fb67c3d9bb31dc144e2f99a62b2afa0e5a1ce3c054'), (272059264, 272059288, 6, '0ed3e7ccf43a2d2f8ef9389af346f188f8b57dc20654b8ec2acc0a75feeb844a'), (272059296, 272059348, 13, 'cd0bac77582722beb0eb154ca29b90fb24f0688bf3fd9002641208a22c8c9820'), (272059360, 272059556, 49, '436a1b94775a5da3e00cfa468c879fca128246ddbd6744913736a1031e728642'), (272059568, 272059608, 10, 'de8ad0986bdf6762916ca1f2a8425f3c163e8b459e61104de06386c241ce527f'), (272059616, 272059652, 9, '6c8a1fd1a675b419fbaef84a86dea124c983bd26aa87165aad2036c6f44bd514'), (272059664, 272059716, 13, '93bc93b3bec1ce1c687b111008c81719a51e4365733b8d08f470d5de4c63d69f'), (272059728, 272059764, 9, '861194486b321acb59243e3680cd3599e90ae946932987da92a5b41ecb4bdfeb'), (272059776, 272060228, 113, 'acc218785a240bbde6e4b8e260dacfb62fa287a2333eef1c1e1b9c9d08a73e88'), (272106096, 272106228, 33, '96daaa102cd724abfaca627d7b18be2207f4658e512e12bc2fbf7a07b4bcf0a2'), (272106240, 272106412, 43, '74521f1ef69a1c8a68920abdacdbcfc1b3cf06921f2feed6da88ed6832c2c58d'), (272106416, 272106496, 20, '5ecfb89783ab0df770ddc7ef257e97d067429af24527c776a1820a3062662076'), (272106496, 272106784, 72, '7a475857d60fa8788f2beefa867bf8bd99bd33d3272896b96bd7e9de348b5e4a'), (272106784, 272106836, 13, '34b775530c6e53f10e1911800e60fd4e030298222282cbb782aee1406f1d367e'), (272106848, 272107056, 52, '4ad586cba983c1e584b30b9f9139d12aca839f6fca083afb337d4df388e1e351'), (272107056, 272107264, 52, 'ddc29a5005eaaa5f088aad45b5ca1f169bc111d3491e491d2dbc7f85e0e000b7'), (272107264, 272107272, 2, '133643b318e8653293c39af84b4d7a745aae5598720d5a58b8c86c5038cadf5b'), (272107280, 272108548, 317, '08b3af2189ee71ac1cb0e058169595541d5684e41bf4ee27a0ba97b71e5584f4'))
FRAGMENTS = ((272057712, 66420, 350, '0156a76ec467aabbed2214e84f6733d9887ee20b91a2e75e494b54b8d0c98c01'), (272059120, 66420, 17, '2ace967de346c0606ca6587c04ef4466c29219929035bcd4da5bee442cc2b1b1'), (272059200, 66420, 5, '9c9d26ca165c1b3a3a356da67966e1926cf5afccd48be6c87f37acbed716537d'), (272059232, 66420, 5, '1c18c081b7b30a749b7407fb67c3d9bb31dc144e2f99a62b2afa0e5a1ce3c054'), (272059264, 66420, 6, '0ed3e7ccf43a2d2f8ef9389af346f188f8b57dc20654b8ec2acc0a75feeb844a'), (272059296, 66420, 13, 'cd0bac77582722beb0eb154ca29b90fb24f0688bf3fd9002641208a22c8c9820'), (272059360, 66420, 49, '436a1b94775a5da3e00cfa468c879fca128246ddbd6744913736a1031e728642'), (272059568, 66420, 10, 'de8ad0986bdf6762916ca1f2a8425f3c163e8b459e61104de06386c241ce527f'), (272059616, 66420, 9, '6c8a1fd1a675b419fbaef84a86dea124c983bd26aa87165aad2036c6f44bd514'), (272059664, 66420, 13, '93bc93b3bec1ce1c687b111008c81719a51e4365733b8d08f470d5de4c63d69f'), (272059728, 66420, 9, '861194486b321acb59243e3680cd3599e90ae946932987da92a5b41ecb4bdfeb'), (272059776, 66420, 113, 'acc218785a240bbde6e4b8e260dacfb62fa287a2333eef1c1e1b9c9d08a73e88'), (272106096, 66432, 33, '96daaa102cd724abfaca627d7b18be2207f4658e512e12bc2fbf7a07b4bcf0a2'), (272106240, 66432, 43, '74521f1ef69a1c8a68920abdacdbcfc1b3cf06921f2feed6da88ed6832c2c58d'), (272106416, 66432, 20, '5ecfb89783ab0df770ddc7ef257e97d067429af24527c776a1820a3062662076'), (272106496, 66432, 72, '7a475857d60fa8788f2beefa867bf8bd99bd33d3272896b96bd7e9de348b5e4a'), (272106784, 66432, 13, '34b775530c6e53f10e1911800e60fd4e030298222282cbb782aee1406f1d367e'), (272106848, 66432, 52, '4ad586cba983c1e584b30b9f9139d12aca839f6fca083afb337d4df388e1e351'), (272107056, 66432, 52, 'ddc29a5005eaaa5f088aad45b5ca1f169bc111d3491e491d2dbc7f85e0e000b7'), (272107264, 66432, 2, '133643b318e8653293c39af84b4d7a745aae5598720d5a58b8c86c5038cadf5b'), (272107280, 66432, 317, '08b3af2189ee71ac1cb0e058169595541d5684e41bf4ee27a0ba97b71e5584f4'))
CONTRACT_TERMS_ENTRIES = tuple(start for start, _, _, _ in CONTRACTS)
PREPARED_CONTRACT_TERMS_ENTRIES = CONTRACT_TERMS_ENTRIES


def fingerprint(body):
    canonical = [{key: r.get(key) for key in ('address', 'assembly', 'ops', 'flows', 'fallthrough')} for r in body]
    return hashlib.sha256(json.dumps(canonical, sort_keys=True, separators=(',', ':')).encode()).hexdigest()


def require_contract_terms_bodies(instructions, complete=False):
    for start, end, count, expected in CONTRACTS:
        body = [r for r in instructions if start <= r['address'] < end]
        if not body and not complete: continue
        if len(body) != count or fingerprint(body) != expected:
            raise ValueError(f'Mac contract-terms contract changed or missing at {start:x}')


def require_contract_terms_edges(edges):
    for source, target in edges:
        for start, end, _, _ in CONTRACTS:
            if start < target < end and not start <= source < end:
                raise ValueError(f'External contract-terms interior edge from {source:x} to {target:x}')


def require_contract_terms_contract(page, instructions, selected=None):
    addresses = {r['address'] for r in instructions} | set(selected or ())
    for start, end, _, _ in CONTRACTS:
        if not start >> 12 <= page <= (end - 1) >> 12: continue
        low, high = max(start, page << 12), min(end, (page + 1) << 12)
        if start not in addresses and not any(low <= a < high for a in addresses): continue
        body = [r for r in instructions if low <= r['address'] < high]
        _, _, count, expected = next(f for f in FRAGMENTS if f[0] == start and f[1] == page)
        if len(body) != count or fingerprint(body) != expected:
            raise ValueError(f'Mac contract-terms contract changed or missing at {start:x} page {page:x}')


def require_contract_terms_index(db, selected=None):
    records = [json.loads(row[0]) for start, end, _, _ in CONTRACTS for row in db.execute(
        'SELECT body FROM instructions WHERE address>=? AND address<? ORDER BY address', (start, end))]
    if selected is None: require_contract_terms_bodies(records, complete=True)
    else:
        for page in sorted({f[1] for f in FRAGMENTS}):
            require_contract_terms_contract(page, [r for _, r in sorted(selected.items())], selected)
    conditions = ' OR '.join('(e.value>=? AND e.value<?)' for _ in CONTRACTS)
    parameters = [value for start, end, _, _ in CONTRACTS for value in (start, end)]
    require_contract_terms_edges(db.execute("""SELECT i.address,e.value
        FROM instructions AS i,
          json_each(json_insert(json_extract(i.body,'$.flows'),'$[#]',json_extract(i.body,'$.fallthrough'))) AS e
        WHERE """ + conditions, parameters))


TABLES = ((277572844, 276872680, 866, 'cf10755f892ef0bdc78692205a42e8857e6d68047570e6ac0c2a165757a2b4b9'), (277572836, 276874552, 208, '569ff0266824b3a88db5909fe26760bfa5b58bfeb3d75d0b9f4d1b967465545b'), (277572832, 277672088, 48, '61a333e992443289b668e077d580dfd62770c343a85d9573dadd7c981332158b'))

def require_contract_terms_tables(read):
    for slot,base,size,expected in TABLES:
        if struct.unpack(">I",read(slot,4))[0]!=base or hashlib.sha256(read(base,size)).hexdigest()!=expected:
            raise ValueError("Changed original contract terms table")

def load_contract_terms_tables(directory):
    program=json.loads((directory/"program.json").read_text())
    segments=[(s["address"],(directory/s["file"]).read_bytes()) for s in program["segments"] if s.get("file")]
    def read(a,n):
        for base,data in segments:
            if base<=a and a+n<=base+len(data):return data[a-base:a-base+n]
        raise ValueError("Missing original terms table")
    require_contract_terms_tables(read)
    return tuple(read(base,size) for _,base,size,_ in TABLES)


def contract_terms_hook(entry):
    return f'if (try_modern_contract_terms(s, 0x{entry:x})) return true;'


def require_contract_terms_hooks(page, instructions, code):
    addresses = {r['address'] for r in instructions}
    for entry in CONTRACT_TERMS_ENTRIES:
        if entry >> 12 != page or entry not in addresses: continue
        label, hook = f'i_{entry:x}:', contract_terms_hook(entry)
        fragment = code.split(label, 1)[-1].split(f'i_{entry + 4:x}:', 1)[0]
        steps = [token for token in (f's.step(0x{entry:x});', f'counter.step(0x{entry:x});') if token in fragment]
        if code.count(hook) != 1 or hook not in fragment or len(steps) != 1 or fragment.index(hook) > fragment.index(steps[0]):
            raise ValueError(f'Missing or misplaced contract-terms hook at {entry:x}')
