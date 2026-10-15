"""Complete original ball-action body contract.

Computed destinations are live guest data. These pins validate the read-only
original input used to characterize them, not a host dispatch table.
"""
import hashlib
import json
import struct

ENTRY = 0x1023cd20
END = 0x10244c58
COUNT = 8142
SHA256 = '5e7dc184cdba66892eebd01d5cf77e027b4195623d09b73b30074125b4fd3646'
FRAGMENTS = (
    (0x1023c, 184, '9996089b8a33af151d8795297c1c48ce874a62b780809146c71c08b38b8c8333'),
    (0x1023d, 1024, '6e096301bb9b87f9885760f183e7ad461f7bfcf5d78c8e77b0a2db190c1eaf9d'),
    (0x1023e, 1024, 'a09e0201fc942e3f80ce76c49e2ef18bcda81984808c2bd51432d50af0e9696f'),
    (0x1023f, 1024, '6f64d61dced387a6ffc518e33de4fc807b1986f630fa539715ab45c8879a136c'),
    (0x10240, 1024, '5697b59e4bdb461a1e75abaf214ef1293bdb31c353f7e597de2ca1f6e05a8880'),
    (0x10241, 1024, 'e788f786bf0372ba6056e1ea65e4307ac7402547c0cc09e3a1bb48ef1cc1566a'),
    (0x10242, 1024, '407fd3deeb18372e1a796756aa78e57cd6ccadda684f80ab42752586d28cdf39'),
    (0x10243, 1024, 'dcf4cc08c1f5f8db9f1ef078c1f5e8ca7b4199e013d4b46e7632ba2f6bafce8f'),
    (0x10244, 790, '93643b534a7837c1aa112a94371fcfa9cd7912ce27e92d75488156676aae0442'),
)
# branch site, relocated original TOC slot, table, word count, data pin
TABLES = (
    (0x1023ff68, 0x108b688c, 0x108cdb08, 47, 'c6e7d95bf3c4fde0a1bbaeb4c3f5d952dd446d36e7ee8ec9edc88611549071e1'),
    (0x10241470, 0x108b6888, 0x108cda24, 57, '27817a90852b942323aab442ada94565f222d7d386824846bb8c564a3593d564'),
    (0x1024368c, 0x108b6884, 0x108cd968, 47, '4abcb03783ff485a7531acf123308f7168a1e18bf2b2489f795fa387e6b5ccd3'),
    (0x10243918, 0x108b6880, 0x108cd940, 10, 'c155273430f512a3b1e065f5ce61d96544bba8beb377a8f2b7d972d51babf44e'),
    (0x102439f0, 0x108b687c, 0x108cd884, 47, '0d01920167739f34d96c6441b2b63fd9cfe8cee353614901c77346fe6615bf42'),
)


def fingerprint(records):
    fields = ('address', 'assembly', 'ops', 'flows', 'fallthrough')
    canonical = [{key: row.get(key) for key in fields} for row in records]
    return hashlib.sha256(json.dumps(canonical, sort_keys=True, separators=(',', ':')).encode()).hexdigest()


def require_ball_action_body(records):
    if any(not ENTRY <= row['address'] < END for row in records):
        raise ValueError('Original ball-action reference contains an unrelated body')
    if len(records) != COUNT or fingerprint(records) != SHA256:
        raise ValueError('Original ball-action body is changed, incomplete or reordered')
    for page, count, digest in FRAGMENTS:
        part = [row for row in records if row['address'] >> 12 == page]
        if len(part) != count or fingerprint(part) != digest:
            raise ValueError(f'Original ball-action fragment changed at {page:x}')


def require_ball_action_edges(edges):
    for source, target in edges:
        if ENTRY < target < END and not ENTRY <= source < END:
            raise ValueError(f'External ball-action interior entrance {source:x} -> {target:x}')


def load_ball_action(db):
    records = [json.loads(row[0]) for row in db.execute(
        'SELECT body FROM instructions WHERE address>=? AND address<? ORDER BY address', (ENTRY, END))]
    require_ball_action_body(records)
    require_ball_action_edges(db.execute("""SELECT i.address,e.value FROM instructions AS i,
        json_each(json_insert(json_extract(i.body,'$.flows'),'$[#]',json_extract(i.body,'$.fallthrough'))) AS e
        WHERE e.value>? AND e.value<?""", (ENTRY, END)))
    return records


def require_ball_action_tables(read_word):
    targets = {}
    for site, slot, table, count, digest in TABLES:
        if read_word(slot) != table:
            raise ValueError(f'Original ball-action table pointer changed at {slot:x}')
        words = [read_word(table + 4 * n) for n in range(count)]
        if hashlib.sha256(b''.join(struct.pack('>I', word) for word in words)).hexdigest() != digest:
            raise ValueError(f'Original ball-action table data changed at {table:x}')
        if any(not ENTRY <= word < END or word % 4 for word in words):
            raise ValueError(f'Original ball-action table has an exterior or unaligned target at {site:x}')
        targets[site] = words
    return targets


def load_ball_action_tables(input_directory):
    program = json.loads((input_directory / 'program.json').read_text())
    segments = [(segment['address'], (input_directory / segment['file']).read_bytes())
                for segment in program['segments'] if segment.get('file') and not segment.get('execute')]

    def read_word(address):
        for base, data in segments:
            if base <= address and address + 4 <= base + len(data):
                return struct.unpack_from('>I', data, address - base)[0]
        raise ValueError(f'Missing original ball-action table word at {address:x}')

    return require_ball_action_tables(read_word)


BALL_ACTION_ENTRIES = (ENTRY,)


def require_ball_action_contract(page, records, selected=None):
    if not any(page == fragment_page for fragment_page, *_ in FRAGMENTS):
        return
    addresses = {row['address'] for row in records} | set(selected or ())
    if not any(ENTRY <= address < END for address in addresses):
        return
    part = [row for row in records if ENTRY <= row['address'] < END]
    _, count, digest = next(fragment for fragment in FRAGMENTS if fragment[0] == page)
    if len(part) != count or fingerprint(part) != digest:
        raise ValueError(f'Original ball-action fragment changed or incomplete at {page:x}')


def require_ball_action_index(db, selected=None):
    if selected is None:
        load_ball_action(db)
    else:
        records = [row for _, row in sorted(selected.items())]
        for page, *_ in FRAGMENTS:
            require_ball_action_contract(page, records, selected)
        require_ball_action_edges(db.execute("""SELECT i.address,e.value FROM instructions AS i,
            json_each(json_insert(json_extract(i.body,'$.flows'),'$[#]',json_extract(i.body,'$.fallthrough'))) AS e
            WHERE e.value>? AND e.value<?""", (ENTRY, END)))


def ball_action_hook(entry):
    if entry != ENTRY:
        raise ValueError('Only the complete ball-action root is callable')
    return f'if (try_modern_ball_action(s, 0x{entry:x})) return true;'


def require_ball_action_hooks(page, records, code):
    if page != ENTRY >> 12 or ENTRY not in {row['address'] for row in records}:
        return
    hook = ball_action_hook(ENTRY)
    fragment = code.split(f'i_{ENTRY:x}:', 1)[-1].split(f'i_{ENTRY + 4:x}:', 1)[0]
    steps = [token for token in (f's.step(0x{ENTRY:x});', f'counter.step(0x{ENTRY:x});') if token in fragment]
    if code.count(hook) != 1 or hook not in fragment or len(steps) != 1 or fragment.index(hook) > fragment.index(steps[0]):
        raise ValueError('Missing or misplaced complete ball-action hook')
