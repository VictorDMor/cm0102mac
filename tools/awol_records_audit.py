"""Pinned complete AWOL records boundaries."""
import hashlib
import json
import struct

CONTRACTS = ((268448176, 268448504, 82, '6c444df0ba785f8aff81ee7f47873b621ce1e796eae028d4f0972f5187ec24d2'), (268448512, 268448696, 46, '18413a746307a0335b7d64d2e1e8cab5ad8263bc29c72d8fa66452d2db366658'), (268448704, 268449500, 196, '568cab62b42fb5b42a5612b001445773cd4880d00a5e5b15257023033588c1d6'), (268449504, 268451264, 437, '1461383639232a41ed97e1554f50c1948272f6e09d3f8cedd820dfd3e5a0ac15'), (268451264, 268451520, 64, '121e8a2eb02079b4516d90b32514fbb20d4d1c36dd4035f8cc24defb801d8922'), (268451520, 268451744, 56, '5eeb62f6d19cc6e9476a3b60af56d06a7c412b3f40ddd3a823c2847a49bd1a05'), (268451744, 268451944, 50, 'f53d1244f14f43fced86fc3303c14410c071e8ba1c1cf109bc6cb2cf434ba808'), (268461568, 268461776, 52, 'c813cf55cc791f486d9a9c76e10fbd6e4b79184202cdbcfdce0e373079b838f1'), (268461776, 268462032, 64, '38901409be3351277d00f1dbe6c5913b0a870a3dcbaa96d4f0e7151d3042b884'), (268462032, 268462120, 22, '19ac9a0e54e09f3475d2c6f41d13e64449ad0a6c23110fd1a6351e1b293bb2eb'), (268462128, 268462336, 52, 'e8d1da8f7ff18e798c75858614207b43fb3388027366325e7f0f2249ef2c528c'), (268464384, 268464864, 120, '484e76b93f9836407f0e3d72615403f61d1bb3aa510faed2104c59b9d2bf64e8'), (268460160, 268461560, 350, '84dd7bf0e7779955d96a817a98b4ac3d5b482105199d3145f8eb56334ed0150e'), (268451952, 268460148, 2044, '8a5b82f5b506b776410dcb2bfe2ce187dd0710fd46aa52bb093028c34ecf3407'), (268462336, 268464384, 512, 'a47fa64c864b7959bb712f1e1c26888c0b2ad9499511125124951593961b7735'), (268470128, 268471128, 250, '57e078f67ecc606dffd45f33003924883eb68c75bfe03829b226fa5b64a68c40'), (268464864, 268466764, 475, '3f85d61be1ff65f71b2b4a47c3dffbb2d6542564bc36cd4d9198a2ae7cecc2a9'), (268471136, 268472476, 335, 'a52a150cc8ca5349c3028a252516fce358282096148fdde8485f92bb45fc80f5'), (268475232, 268476136, 226, '617da86865ff914977e8f381b84c4d59fc01f3cc0fcd19ae458ead8429916258'), (268466768, 268470128, 840, '5d5fa128c1bdd868bcd92ee8638014bb3e422bc5de1da119ea6fde45a80289ac'), (268472480, 268475220, 685, '2f7a6aecb7a6a699d9608a0772b55be28244ec6d0745598429b436c99c5d5482'))
FRAGMENTS = ((268448176, 65539, 82, '6c444df0ba785f8aff81ee7f47873b621ce1e796eae028d4f0972f5187ec24d2'), (268448512, 65539, 46, '18413a746307a0335b7d64d2e1e8cab5ad8263bc29c72d8fa66452d2db366658'), (268448704, 65539, 196, '568cab62b42fb5b42a5612b001445773cd4880d00a5e5b15257023033588c1d6'), (268449504, 65539, 437, '1461383639232a41ed97e1554f50c1948272f6e09d3f8cedd820dfd3e5a0ac15'), (268451264, 65539, 64, '121e8a2eb02079b4516d90b32514fbb20d4d1c36dd4035f8cc24defb801d8922'), (268451520, 65539, 56, '5eeb62f6d19cc6e9476a3b60af56d06a7c412b3f40ddd3a823c2847a49bd1a05'), (268451744, 65539, 24, 'd129c10e81dc141418445fd1c6274d5dfd0d5c85827a748ece78ebb8519b1502'), (268451744, 65540, 26, '00d2704af47cd481af69745a8375e72779685679faf05945d58793ca8f92effd'), (268461568, 65542, 52, 'c813cf55cc791f486d9a9c76e10fbd6e4b79184202cdbcfdce0e373079b838f1'), (268461776, 65542, 64, '38901409be3351277d00f1dbe6c5913b0a870a3dcbaa96d4f0e7151d3042b884'), (268462032, 65542, 22, '19ac9a0e54e09f3475d2c6f41d13e64449ad0a6c23110fd1a6351e1b293bb2eb'), (268462128, 65542, 52, 'e8d1da8f7ff18e798c75858614207b43fb3388027366325e7f0f2249ef2c528c'), (268464384, 65543, 120, '484e76b93f9836407f0e3d72615403f61d1bb3aa510faed2104c59b9d2bf64e8'), (268460160, 65542, 350, '84dd7bf0e7779955d96a817a98b4ac3d5b482105199d3145f8eb56334ed0150e'), (268451952, 65540, 993, 'ad31555eebdb21c4f5ca840b5aeafc740d4f1471079d0029ebf7c4aa579ec92f'), (268451952, 65541, 1022, '61f5491113f39bb1108ebb6b78ceefcebf6a1b8a8df3a51a41d594de46323f9e'), (268451952, 65542, 29, 'afa33734083eda3fa27f20c27cbd5591c39965c902f3b2b0b7f316f4b2e875d9'), (268462336, 65542, 448, '86293a8fdac0cc40477d7ca9562d67db4acbb9185502eee9c62c7b62f7c8ce12'), (268462336, 65543, 64, '7c36eef508d1e44078b6346b560bcd3d6cb6549d5c81abfec1285910d7694fb2'), (268470128, 65544, 250, '57e078f67ecc606dffd45f33003924883eb68c75bfe03829b226fa5b64a68c40'), (268464864, 65543, 475, '3f85d61be1ff65f71b2b4a47c3dffbb2d6542564bc36cd4d9198a2ae7cecc2a9'), (268471136, 65544, 296, '3e0990ff966dbdaa9b545853991003bf692f27453396047dfebaca0aea1e1eb9'), (268471136, 65545, 39, '0b80324a107e7f5a0030e09cf9ed78602af123bded1c98ef0ce305d5edeb1107'), (268475232, 65545, 226, '617da86865ff914977e8f381b84c4d59fc01f3cc0fcd19ae458ead8429916258'), (268466768, 65543, 364, 'eb087b2dc728bd4b114fa78a8b1787b1430b317f1d4911183102071f2c2f8887'), (268466768, 65544, 476, '135226bdec2bf47dc287108fb3c81c1b6a2832aa53328cb6f7e02758e8a747b4'), (268472480, 65545, 685, '2f7a6aecb7a6a699d9608a0772b55be28244ec6d0745598429b436c99c5d5482'))
AWOL_RECORDS_ENTRIES = tuple(start for start, _, _, _ in CONTRACTS)


def fingerprint(body):
    canonical = [{key: r.get(key) for key in ('address', 'assembly', 'ops', 'flows', 'fallthrough')} for r in body]
    return hashlib.sha256(json.dumps(canonical, sort_keys=True, separators=(',', ':')).encode()).hexdigest()


def require_awol_records_bodies(instructions, complete=False):
    for start, end, count, expected in CONTRACTS:
        body = [r for r in instructions if start <= r['address'] < end]
        if not body and not complete: continue
        if len(body) != count or fingerprint(body) != expected:
            raise ValueError(f'Mac awol-records contract changed or missing at {start:x}')


def require_awol_records_edges(edges):
    for source, target in edges:
        for start, end, _, _ in CONTRACTS:
            if start < target < end and not start <= source < end:
                raise ValueError(f'External awol-records interior edge from {source:x} to {target:x}')


def require_awol_records_contract(page, instructions, selected=None):
    addresses = {r['address'] for r in instructions} | set(selected or ())
    for start, end, _, _ in CONTRACTS:
        if not start >> 12 <= page <= (end - 1) >> 12: continue
        low, high = max(start, page << 12), min(end, (page + 1) << 12)
        if start not in addresses and not any(low <= a < high for a in addresses): continue
        body = [r for r in instructions if low <= r['address'] < high]
        _, _, count, expected = next(f for f in FRAGMENTS if f[0] == start and f[1] == page)
        if len(body) != count or fingerprint(body) != expected:
            raise ValueError(f'Mac awol-records contract changed or missing at {start:x} page {page:x}')


def require_awol_records_index(db, selected=None):
    records = [json.loads(row[0]) for start, end, _, _ in CONTRACTS for row in db.execute(
        'SELECT body FROM instructions WHERE address>=? AND address<? ORDER BY address', (start, end))]
    if selected is None: require_awol_records_bodies(records, complete=True)
    else:
        for page in sorted({f[1] for f in FRAGMENTS}):
            require_awol_records_contract(page, [r for _, r in sorted(selected.items())], selected)
    require_awol_records_edges(db.execute("""SELECT i.address,e.value
        FROM instructions AS i,
          json_each(json_insert(json_extract(i.body,'$.flows'),'$[#]',json_extract(i.body,'$.fallthrough'))) AS e
        WHERE e.value>=? AND e.value<?""", (min(c[0] for c in CONTRACTS), max(c[1] for c in CONTRACTS))))


TABLES = ((277569672, 276304928, 5916, 'fae7ca030e5b50008dc72666d6f4bae42388b45c4ad81524c26f904534ad7119'), (277569668, 276310844, 32, 'b41e5252074a70916a9a3594bab9b4fdf58a0042b8ea4aeedb09d70655768bc3'))
PADDING = ((268449088, 1610612736), (268449244, 1610612736), (268449428, 1610612736), (268450272, 1610612736), (268450680, 1610612736), (268451088, 1610612736), (268452036, 1610612736), (268452156, 1610612736), (268453964, 1610612736), (268457352, 1610612736), (268458844, 1610612736))

def require_awol_records_tables(word):
    for slot,base,size,expected in TABLES:
        data=b''.join(struct.pack('>I',word(base+n)) for n in range(0,size,4))
        if word(slot)!=base or hashlib.sha256(data).hexdigest()!=expected:
            raise ValueError('Changed AWOL records diagnostic table')
    for address,expected in PADDING:
        if word(address)!=expected:raise ValueError('Changed AWOL records padding word')

def load_awol_records_tables(input_directory):
    program=json.loads((input_directory/'program.json').read_text())
    segments=[(s['address'],(input_directory/s['file']).read_bytes()) for s in program['segments'] if s.get('file')]
    def word(address):
        for base,data in segments:
            if base<=address and address+4<=base+len(data):return struct.unpack_from('>I',data,address-base)[0]
        raise ValueError('Missing AWOL records table word')
    require_awol_records_tables(word)
    return tuple(tuple(word(base+n) for n in range(0,size,4)) for _,base,size,_ in TABLES)


def awol_records_hook(entry):
    return f'if (try_modern_awol_records(s, 0x{entry:x})) return true;'


def require_awol_records_hooks(page, instructions, code, taken_over=()):
    addresses = {r['address'] for r in instructions}
    for entry in AWOL_RECORDS_ENTRIES:
        if entry in taken_over: continue
        if entry >> 12 != page or entry not in addresses: continue
        label, hook = f'i_{entry:x}:', awol_records_hook(entry)
        fragment = code.split(label, 1)[-1].split(f'i_{entry + 4:x}:', 1)[0]
        steps = [token for token in (f's.step(0x{entry:x});', f'counter.step(0x{entry:x});') if token in fragment]
        if code.count(hook) != 1 or hook not in fragment or len(steps) != 1 or fragment.index(hook) > fragment.index(steps[0]):
            raise ValueError(f'Missing or misplaced AWOL-records hook at {entry:x}')
