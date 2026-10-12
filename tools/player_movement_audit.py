"""Complete original movement/interaction boundary, before semantic replacement.

Computed destinations are live guest data. These pins validate the read-only
original input used to characterize them, not a host dispatch table.
"""
import hashlib
import json
import struct

ENTRY = 0x1022e7e0
END = 0x10235da4
COUNT = 7537
SHA256 = '96ccf1595c6d3d5fa2c95331bb0cf45d4dfb50cdb4c0dd93aa03fa3fa93d3a71'
FRAGMENTS = (
    (0x1022e, 520, '529379b079ec3bd8b3bd368e2dfcfdbed85c0bc7a6800d76d19078919537b5c2'),
    (0x1022f, 1024, '70f1c34397ab78c7f00f3023ae7ba4e46702015ba5afe156569ac5b8a3b9dc66'),
    (0x10230, 1024, '5e3e5bb507b2775e85f9218d90836fd7ae8846a5c3d02b29279e0471b8b61145'),
    (0x10231, 1024, 'fbd72cb7a49c3f11c19b8914c57dc41d4dfd68b2b5d629d7af29286a80dd1067'),
    (0x10232, 1024, 'cf3d04c698c4a417c042c1ce0f6c90a769c071fa4da661f55c57b01d0516c3e0'),
    (0x10233, 1024, '8ede9a901259c5a0b4321f11c452fd60c6da255c4925c72c1f714a99f6717937'),
    (0x10234, 1024, '833ff0536600cd1efbd14bc1130b0a5983a053bb36de273f9960c7532379e073'),
    (0x10235, 873, '7af3637f70ca65fc58afd7280d8cd2e8d431cd96f463c4dc41f6532b3b7a6ed4'),
)
# branch site, relocated original TOC slot, table, word count, original data pin
TABLES = (
    (0x1023041c, 0x108b689c, 0x108cd758, 47, '24b723f58e88516ef8774a5e97524997d2eda9f24bf694b5773db8e3fcf188ad'),
    (0x10233b18, 0x108b6898, 0x108cd69c, 47, '13c85580e354e62a8cf7edb5afd1bf053bfad719f9e64d0cac6ff528095592a0'),
    (0x10233d98, 0x108b6894, 0x108cd670, 11, 'e51435725d70bf007bac0eddcd572386f6ee09a51aa58725dfc4c548ad359d41'),
)


def fingerprint(records):
    fields = ('address', 'assembly', 'ops', 'flows', 'fallthrough')
    canonical = [{key: row.get(key) for key in fields} for row in records]
    return hashlib.sha256(json.dumps(canonical, sort_keys=True, separators=(',', ':')).encode()).hexdigest()


def require_player_movement_body(records):
    if any(not ENTRY <= row['address'] < END for row in records):
        raise ValueError('Original movement reference contains an unrelated body')
    if len(records) != COUNT or fingerprint(records) != SHA256:
        raise ValueError('Original movement body is changed, incomplete or reordered')
    for page, count, digest in FRAGMENTS:
        part = [row for row in records if row['address'] >> 12 == page]
        if len(part) != count or fingerprint(part) != digest:
            raise ValueError(f'Original movement fragment changed at {page:x}')


def require_player_movement_edges(edges):
    for source, target in edges:
        if ENTRY < target < END and not ENTRY <= source < END:
            raise ValueError(f'External movement interior entrance {source:x} -> {target:x}')


def load_player_movement(db):
    records = [json.loads(row[0]) for row in db.execute(
        'SELECT body FROM instructions WHERE address>=? AND address<? ORDER BY address', (ENTRY, END))]
    require_player_movement_body(records)
    require_player_movement_edges(db.execute("""SELECT i.address,e.value FROM instructions AS i,
        json_each(json_insert(json_extract(i.body,'$.flows'),'$[#]',json_extract(i.body,'$.fallthrough'))) AS e
        WHERE e.value>? AND e.value<?""", (ENTRY, END)))
    return records


def require_player_movement_tables(read_word):
    targets = {}
    for site, slot, table, count, digest in TABLES:
        if read_word(slot) != table:
            raise ValueError(f'Original movement table pointer changed at {slot:x}')
        words = [read_word(table + 4 * n) for n in range(count)]
        if hashlib.sha256(b''.join(struct.pack('>I', word) for word in words)).hexdigest() != digest:
            raise ValueError(f'Original movement table data changed at {table:x}')
        if any(not ENTRY <= word < END or word % 4 for word in words):
            raise ValueError(f'Original movement table has an exterior or unaligned target at {site:x}')
        targets[site] = words
    return targets


def load_player_movement_tables(input_directory):
    program = json.loads((input_directory / 'program.json').read_text())
    segments = [(segment['address'], (input_directory / segment['file']).read_bytes())
                for segment in program['segments'] if segment.get('file') and not segment.get('execute')]

    def read_word(address):
        for base, data in segments:
            if base <= address and address + 4 <= base + len(data):
                return struct.unpack_from('>I', data, address - base)[0]
        raise ValueError(f'Missing original movement table word at {address:x}')

    return require_player_movement_tables(read_word)
