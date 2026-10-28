"""Whole original coordinate, heading, turn and possession contracts."""
import hashlib
import json

BODIES = (
    (0x102374f0, 0x10237830, 208, '5835c3c6b493d66f75df87effaa4966dfbad7d8c90580ac8af793bf0ca876135'), # set_coordinates
    (0x10237830, 0x10237af4, 177, 'babf2e8401ba3e2b72c1bf2e642e166c3308cbe74e2a65e033b259a2db858caf'), # set_heading
    (0x10237b00, 0x10237b08, 2, '0bb0a15d0183c71c0ffe27e477356df45d260d24db61d35f043dfa8ce2f02b66'), # heading
    (0x10245050, 0x1024515c, 67, 'eb619cd852d0303b7cf9ddf7d51863d0f20af1dcc67d025ee08b5a8da36ca65e'), # can_face_destination
    (0x102775c0, 0x102775e0, 8, '4cdc4f3028167bb137cdf43c839a08f37f5d509cc568b98339695189c9bde78a'), # has_possession
)
ACTOR_MOTION_ENTRIES = tuple(row[0] for row in BODIES)


def fingerprint(records):
    fields = ('address', 'assembly', 'ops', 'flows', 'fallthrough')
    return hashlib.sha256(json.dumps([{key: row.get(key) for key in fields} for row in records],
                                    sort_keys=True, separators=(',', ':')).encode()).hexdigest()


def require_actor_motion_contract(page, records, selected=None):
    for entry, end, count, digest in BODIES:
        if page != entry >> 12:
            continue
        addresses = {row['address'] for row in records} | set(selected or ())
        if not any(entry <= address < end for address in addresses):
            continue
        body = [row for row in records if entry <= row['address'] < end]
        if len(body) != count or fingerprint(body) != digest:
            raise ValueError(f'Original actor motion body changed or incomplete at {entry:x}')


def require_actor_motion_edges(edges):
    for source, target in edges:
        for entry, end, *_ in BODIES:
            if entry < target < end and not entry <= source < end:
                raise ValueError(f'Exterior actor motion interior entrance {source:x} -> {target:x}')


def load_actor_motion(db):
    records = []
    for entry, end, *_ in BODIES:
        records.extend(json.loads(row[0]) for row in db.execute(
            'SELECT body FROM instructions WHERE address>=? AND address<? ORDER BY address', (entry, end)))
    for page in {entry >> 12 for entry in ACTOR_MOTION_ENTRIES}:
        require_actor_motion_contract(page, records)
    return records


def require_actor_motion_index(db, selected=None):
    records = load_actor_motion(db) if selected is None else [row for _, row in sorted(selected.items())]
    for page in {entry >> 12 for entry in ACTOR_MOTION_ENTRIES}:
        require_actor_motion_contract(page, records, selected)
    where = ' OR '.join('(e.value>? AND e.value<?)' for _ in BODIES)
    limits = tuple(value for entry, end, *_ in BODIES for value in (entry, end))
    require_actor_motion_edges(db.execute("""SELECT i.address,e.value FROM instructions AS i,
        json_each(json_insert(json_extract(i.body,'$.flows'),'$[#]',json_extract(i.body,'$.fallthrough'))) AS e
        WHERE """ + where, limits))


def actor_motion_hook(entry):
    return f'if (try_modern_actor_motion(s, 0x{entry:x})) return true;'


def require_actor_motion_hooks(page, records, code, taken_over=()):
    for entry in ACTOR_MOTION_ENTRIES:
        if entry >> 12 != page or entry not in {row['address'] for row in records} or entry in taken_over:
            continue
        hook = actor_motion_hook(entry)
        fragment = code.split(f'i_{entry:x}:', 1)[-1].split(f'i_{entry + 4:x}:', 1)[0]
        steps = [token for token in (f's.step(0x{entry:x});', f'counter.step(0x{entry:x});') if token in fragment]
        if code.count(hook) != 1 or hook not in fragment or len(steps) != 1 or fragment.index(hook) > fragment.index(steps[0]):
            raise ValueError(f'Missing or misplaced complete actor motion hook at {entry:x}')
