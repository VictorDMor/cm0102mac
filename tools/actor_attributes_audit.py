"""Pinned complete original actor attributes, recovery and condition boundaries."""
import hashlib
import json
import struct

CONTRACTS = (
    (0x10235ea0, 0x102373a0, 1344, 'a65bfe1809ad4e6fb714b913d35b9c7093adf4fe78f9c9595588a77c2c2d4e60'),
    (0x102373a0, 0x102373b4, 5, '7b3379b11aede3d3f834c85c1e0c343e03f480a62ba9034ca67ce459d749ebad'),
    (0x10235db0, 0x10235e9c, 59, '17e5a34c3733548ffc95d3e909a4ae354ea8663a5e9cbe62629ded0dc939d3b9'),
    (0x102393c0, 0x1023958c, 115, 'addda6b99214c34324bbea92428a4beb68ea2e457f7c9c398745295e289bb838'),
    (0x10239590, 0x1023978c, 127, 'b514ca05b3df358902c9b0fb2e3177913d861deded9868d7149e8a0c6f1b676d'),
    (0x10239790, 0x10239d10, 352, '15cc37bee63e36b721b40627e1501e34f691091390efe4560d1c6974f18528d0'),
    (0x10187650, 0x10187680, 12, 'ba9ea5934cd16e94dacec20304d2f9e3ab2b8103d99dec483f8849002d7fd44c'),
    (0x10187680, 0x101877dc, 87, '7db6901350acd1729ac79de24d5c9b846028fab6453b948ad58e30b937bb5f84'),
    (0x101878d0, 0x10187a1c, 83, '9eced854756a34c572ff8e72b1c6a6673cd41d7d405d543ddc0ae1e12965236f'),
    (0x10187a40, 0x10188488, 658, 'b3f2824f1f997c70bd8384e3ca6672cf70a9822aad70e7160e2b8ac8b20043fb'),
    (0x101877e0, 0x101878d0, 60, '75ed82c07e8157772ab32b791a2d81910a7605588b77ef3c2e34b0c3808266ee'),
    (0x10186a20, 0x10186cd0, 172, '15d148ef58edd4b2371c0465eb825a14503d5a9809ad4898f1ba1c3c9cdccd11'),
    (0x1018b9f0, 0x1018bb4c, 87, 'be53226d6f82a3dc3b25728cb6b98bdb51e04709eedc10016ebbce41c306b414'),
    (0x10186ea0, 0x10186fb4, 69, 'b1ff37bf028514b2d2a29be693cd75a106d05454f282acf9f333a3eef9ea3d3b'),
)
FRAGMENTS = (
    (0x10235ea0, 0x10235, 88, '45665d75865ef78ca968b68d3ce392dac9ebad88484f795c921040bd8088e8ff'),
    (0x10235ea0, 0x10236, 1024, '30dca2158a57debcd27adbf414eeaaac96f2184619ed9de3948311403fbb1a0b'),
    (0x10235ea0, 0x10237, 232, '7f6438ff1b3e15b39c948e96385f18ef9ef28b0edb7d321071bcebc7f47df7e0'),
    (0x102373a0, 0x10237, 5, '7b3379b11aede3d3f834c85c1e0c343e03f480a62ba9034ca67ce459d749ebad'),
    (0x10235db0, 0x10235, 59, '17e5a34c3733548ffc95d3e909a4ae354ea8663a5e9cbe62629ded0dc939d3b9'),
    (0x102393c0, 0x10239, 115, 'addda6b99214c34324bbea92428a4beb68ea2e457f7c9c398745295e289bb838'),
    (0x10239590, 0x10239, 127, 'b514ca05b3df358902c9b0fb2e3177913d861deded9868d7149e8a0c6f1b676d'),
    (0x10239790, 0x10239, 352, '15cc37bee63e36b721b40627e1501e34f691091390efe4560d1c6974f18528d0'),
    (0x10187650, 0x10187, 12, 'ba9ea5934cd16e94dacec20304d2f9e3ab2b8103d99dec483f8849002d7fd44c'),
    (0x10187680, 0x10187, 87, '7db6901350acd1729ac79de24d5c9b846028fab6453b948ad58e30b937bb5f84'),
    (0x101878d0, 0x10187, 83, '9eced854756a34c572ff8e72b1c6a6673cd41d7d405d543ddc0ae1e12965236f'),
    (0x10187a40, 0x10187, 368, '8e71b13c57b5a3a91ba2d9df84024abbf82fae85ad744dee22c863b3b0ce682d'),
    (0x10187a40, 0x10188, 290, '47ba597bd84fcfa38512d97dd629f7b2e2040df58c943c30b3f5d74eb48303cf'),
    (0x101877e0, 0x10187, 60, '75ed82c07e8157772ab32b791a2d81910a7605588b77ef3c2e34b0c3808266ee'),
    (0x10186a20, 0x10186, 172, '15d148ef58edd4b2371c0465eb825a14503d5a9809ad4898f1ba1c3c9cdccd11'),
    (0x1018b9f0, 0x1018b, 87, 'be53226d6f82a3dc3b25728cb6b98bdb51e04709eedc10016ebbce41c306b414'),
    (0x10186ea0, 0x10186, 69, 'b1ff37bf028514b2d2a29be693cd75a106d05454f282acf9f333a3eef9ea3d3b'),
)
ACTOR_ATTRIBUTES_ENTRIES = tuple(start for start, _, _, _ in CONTRACTS)
COST_TABLE = (0x108b6890, 0x108cd814, 28, '4471df4ee72c2421688d5d0aac3fdfe2f63ee36d31fa0e391a8850b95fb22e8d')


def require_actor_attributes_tables(read_word):
    slot, table, count, digest = COST_TABLE
    if read_word(slot) != table:
        raise ValueError('Changed original actor-condition table pointer')
    words = [read_word(table + 4 * n) for n in range(count)]
    actual = hashlib.sha256(b''.join(struct.pack('>I', word) for word in words)).hexdigest()
    if actual != digest or any(word not in (0x10239690, 0x102396e4, 0x10239738) for word in words):
        raise ValueError('Changed original actor-condition table destinations')
    return words


def load_actor_attributes_tables(input_directory):
    program = json.loads((input_directory / 'program.json').read_text())
    segments = [(segment['address'], (input_directory / segment['file']).read_bytes())
                for segment in program['segments'] if segment.get('file') and not segment.get('execute')]
    def read_word(address):
        for base, data in segments:
            if base <= address and address + 4 <= base + len(data):
                return struct.unpack_from('>I', data, address - base)[0]
        raise ValueError(f'Missing actor-condition table word at {address:x}')
    return require_actor_attributes_tables(read_word)


def fingerprint(body):
    canonical = [{key: r.get(key) for key in ('address', 'assembly', 'ops', 'flows', 'fallthrough')} for r in body]
    return hashlib.sha256(json.dumps(canonical, sort_keys=True, separators=(',', ':')).encode()).hexdigest()


def require_actor_attributes_bodies(instructions, complete=False):
    for start, end, count, expected in CONTRACTS:
        body = [r for r in instructions if start <= r['address'] < end]
        if not body and not complete: continue
        if len(body) != count or fingerprint(body) != expected:
            raise ValueError(f'Mac actor-attributes contract changed or missing at {start:x}')


def require_actor_attributes_edges(edges):
    for source, target in edges:
        for start, end, _, _ in CONTRACTS:
            if start < target < end and not start <= source < end:
                raise ValueError(f'External actor-attributes interior edge from {source:x} to {target:x}')


def require_actor_attributes_contract(page, instructions, selected=None):
    addresses = {r['address'] for r in instructions} | set(selected or ())
    for start, end, _, _ in CONTRACTS:
        if not start >> 12 <= page <= (end - 1) >> 12: continue
        low, high = max(start, page << 12), min(end, (page + 1) << 12)
        if start not in addresses and not any(low <= a < high for a in addresses): continue
        body = [r for r in instructions if low <= r['address'] < high]
        _, _, count, expected = next(f for f in FRAGMENTS if f[0] == start and f[1] == page)
        if len(body) != count or fingerprint(body) != expected:
            raise ValueError(f'Mac actor-attributes contract changed or missing at {start:x} page {page:x}')


def require_actor_attributes_index(db, selected=None):
    records = [json.loads(row[0]) for start, end, _, _ in CONTRACTS for row in db.execute(
        'SELECT body FROM instructions WHERE address>=? AND address<? ORDER BY address', (start, end))]
    if selected is None: require_actor_attributes_bodies(records, complete=True)
    else:
        for page in sorted({f[1] for f in FRAGMENTS}):
            require_actor_attributes_contract(page, [r for _, r in sorted(selected.items())], selected)
    require_actor_attributes_edges(db.execute("""SELECT i.address,e.value
        FROM instructions AS i,
          json_each(json_insert(json_extract(i.body,'$.flows'),'$[#]',json_extract(i.body,'$.fallthrough'))) AS e
        WHERE e.value>=? AND e.value<?""", (min(c[0] for c in CONTRACTS), max(c[1] for c in CONTRACTS))))


def actor_attributes_hook(entry):
    return f'if (try_modern_actor_attributes(s, 0x{entry:x})) return true;'


def require_actor_attributes_hooks(page, instructions, code):
    addresses = {r['address'] for r in instructions}
    for entry in ACTOR_ATTRIBUTES_ENTRIES:
        if entry >> 12 != page or entry not in addresses: continue
        label, hook = f'i_{entry:x}:', actor_attributes_hook(entry)
        fragment = code.split(label, 1)[-1].split(f'i_{entry + 4:x}:', 1)[0]
        steps = [token for token in (f's.step(0x{entry:x});', f'counter.step(0x{entry:x});') if token in fragment]
        if code.count(hook) != 1 or hook not in fragment or len(steps) != 1 or fragment.index(hook) > fragment.index(steps[0]):
            raise ValueError(f'Missing or misplaced actor-attributes hook at {entry:x}')
