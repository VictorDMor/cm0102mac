"""Pinned complete AWOL records boundaries."""
import hashlib
import json
import struct

CONTRACTS = ((268448176, 268448504, 82, '6c444df0ba785f8aff81ee7f47873b621ce1e796eae028d4f0972f5187ec24d2'), (268448512, 268448696, 46, '18413a746307a0335b7d64d2e1e8cab5ad8263bc29c72d8fa66452d2db366658'), (268448704, 268449500, 196, '568cab62b42fb5b42a5612b001445773cd4880d00a5e5b15257023033588c1d6'), (268449504, 268451264, 437, '1461383639232a41ed97e1554f50c1948272f6e09d3f8cedd820dfd3e5a0ac15'), (268451264, 268451520, 64, '121e8a2eb02079b4516d90b32514fbb20d4d1c36dd4035f8cc24defb801d8922'), (268451520, 268451744, 56, '5eeb62f6d19cc6e9476a3b60af56d06a7c412b3f40ddd3a823c2847a49bd1a05'), (268451744, 268451944, 50, 'f53d1244f14f43fced86fc3303c14410c071e8ba1c1cf109bc6cb2cf434ba808'), (268461568, 268461776, 52, 'c813cf55cc791f486d9a9c76e10fbd6e4b79184202cdbcfdce0e373079b838f1'), (268461776, 268462032, 64, '38901409be3351277d00f1dbe6c5913b0a870a3dcbaa96d4f0e7151d3042b884'), (268462032, 268462120, 22, '19ac9a0e54e09f3475d2c6f41d13e64449ad0a6c23110fd1a6351e1b293bb2eb'), (268462128, 268462336, 52, 'e8d1da8f7ff18e798c75858614207b43fb3388027366325e7f0f2249ef2c528c'), (268464384, 268464864, 120, '484e76b93f9836407f0e3d72615403f61d1bb3aa510faed2104c59b9d2bf64e8'), (268460160, 268461560, 350, '84dd7bf0e7779955d96a817a98b4ac3d5b482105199d3145f8eb56334ed0150e'))
FRAGMENTS = ((268448176, 65539, 82, '6c444df0ba785f8aff81ee7f47873b621ce1e796eae028d4f0972f5187ec24d2'), (268448512, 65539, 46, '18413a746307a0335b7d64d2e1e8cab5ad8263bc29c72d8fa66452d2db366658'), (268448704, 65539, 196, '568cab62b42fb5b42a5612b001445773cd4880d00a5e5b15257023033588c1d6'), (268449504, 65539, 437, '1461383639232a41ed97e1554f50c1948272f6e09d3f8cedd820dfd3e5a0ac15'), (268451264, 65539, 64, '121e8a2eb02079b4516d90b32514fbb20d4d1c36dd4035f8cc24defb801d8922'), (268451520, 65539, 56, '5eeb62f6d19cc6e9476a3b60af56d06a7c412b3f40ddd3a823c2847a49bd1a05'), (268451744, 65539, 24, 'd129c10e81dc141418445fd1c6274d5dfd0d5c85827a748ece78ebb8519b1502'), (268451744, 65540, 26, '00d2704af47cd481af69745a8375e72779685679faf05945d58793ca8f92effd'), (268461568, 65542, 52, 'c813cf55cc791f486d9a9c76e10fbd6e4b79184202cdbcfdce0e373079b838f1'), (268461776, 65542, 64, '38901409be3351277d00f1dbe6c5913b0a870a3dcbaa96d4f0e7151d3042b884'), (268462032, 65542, 22, '19ac9a0e54e09f3475d2c6f41d13e64449ad0a6c23110fd1a6351e1b293bb2eb'), (268462128, 65542, 52, 'e8d1da8f7ff18e798c75858614207b43fb3388027366325e7f0f2249ef2c528c'), (268464384, 65543, 120, '484e76b93f9836407f0e3d72615403f61d1bb3aa510faed2104c59b9d2bf64e8'), (268460160, 65542, 350, '84dd7bf0e7779955d96a817a98b4ac3d5b482105199d3145f8eb56334ed0150e'))
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
        WHERE e.value>=? AND e.value<?""", (CONTRACTS[0][0], CONTRACTS[-1][1])))


TABLES = ((277569672, 276304928, 40, '1b8c9fc1a7afb6793835799ab1ee41f425155dddf1a701b768e3cbd9dbaea44c'), (277569668, 276310844, 16, '051eec9cc699d2886923b2ab7dba0f7f98fb8473e67773d956a16466a4fefb2a'))
PADDING = ((268449088, 1610612736), (268449244, 1610612736), (268449428, 1610612736), (268450272, 1610612736), (268450680, 1610612736), (268451088, 1610612736))

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
