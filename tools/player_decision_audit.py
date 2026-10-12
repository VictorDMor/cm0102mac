"""Complete source boundary for the in-progress original player decision port.

This contract describes an original dependency, not a semantic replacement.
"""
import hashlib
import json

ENTRY = 0x102836a0
END = 0x10285748
COUNT = 2090
SHA256 = '0e6502a309eb476fbaba0bbde1274e23622b3fba464efb4bb7ec5aca2f094ae6'
FRAGMENTS = (
    (0x10283, 600, 'f80ac80869737a7c029947961c22d58201e502efbc3dfded160763129460abac'),
    (0x10284, 1024, '77b17c2fe97afebc7273c42197f0fad5c7e38f55a9d6ad01b3dfcbf1e488a653'),
    (0x10285, 466, '57a8d2e00691fa54ff104812dfb065871a6c6bc4d63247f20aa5cec81fe00b6d'),
)


def fingerprint(records):
    fields = ('address', 'assembly', 'ops', 'flows', 'fallthrough')
    canonical = [{key: row.get(key) for key in fields} for row in records]
    return hashlib.sha256(json.dumps(canonical, sort_keys=True, separators=(',', ':')).encode()).hexdigest()


def require_player_decision_body(records):
    body = [row for row in records if ENTRY <= row['address'] < END]
    if len(body) != COUNT or fingerprint(body) != SHA256:
        raise ValueError('Original player decision body is changed, incomplete or reordered')
    for page, count, digest in FRAGMENTS:
        part = [row for row in body if row['address'] >> 12 == page]
        if len(part) != count or fingerprint(part) != digest:
            raise ValueError(f'Original player decision fragment changed at {page:x}')


def require_player_decision_edges(edges):
    for source, target in edges:
        if ENTRY < target < END and not ENTRY <= source < END:
            raise ValueError(f'External player decision interior entrance {source:x} -> {target:x}')


def load_player_decision(db):
    records = [json.loads(row[0]) for row in db.execute(
        'SELECT body FROM instructions WHERE address>=? AND address<? ORDER BY address', (ENTRY, END))]
    require_player_decision_body(records)
    require_player_decision_edges(db.execute("""SELECT i.address,e.value FROM instructions AS i,
        json_each(json_insert(json_extract(i.body,'$.flows'),'$[#]',json_extract(i.body,'$.fallthrough'))) AS e
        WHERE e.value>? AND e.value<?""", (ENTRY, END)))
    return records
