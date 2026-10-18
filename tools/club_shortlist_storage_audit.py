"""Pinned complete club shortlist storage lifecycle, reset, sampling and persistence."""
import hashlib
import json
import struct

CONTRACTS = ((271934480, 271935400, 230, '60d1d8f8aec7b92ab04a91f2c59c72fc96ab7f161d08048dbe65f03a67f856db'), (271935408, 271935700, 72, 'cc37faab941c78501871bc0ea513885a553e6ca534a6b5c38c50a11a25aca9b7'), (271935712, 271937084, 342, '6888d5b3f8d355213a1e626e585441bd3539c5a4deba453470887f414ae13b47'), (271937088, 271938488, 350, '8709923867d571623ab3069b7bb7e0ae7deb660bc96855d801377f172acc05f7'), (271941664, 271944432, 690, '7bfd359b3e657637306ff361139c64be3691220c29e2d43f12bab1d5ec8a41d0'), (271944432, 271944484, 13, '4619513e61a9f6bb87e3009fb57654b5f7589ff13fb641523820fd74a5ee3a10'), (271944496, 271945492, 248, 'c1585cd42c3343dd3ec952b84240feb0abae5a6ec44190c21ddb321081e63664'))
FRAGMENTS = ((271934480, 66390, 230, '60d1d8f8aec7b92ab04a91f2c59c72fc96ab7f161d08048dbe65f03a67f856db'), (271935408, 66390, 72, 'cc37faab941c78501871bc0ea513885a553e6ca534a6b5c38c50a11a25aca9b7'), (271935712, 66390, 342, '6888d5b3f8d355213a1e626e585441bd3539c5a4deba453470887f414ae13b47'), (271937088, 66390, 112, '8e05261f312a7b3a4df8d621bed071af252e58da5d6bf68dd497aaa50c59f9b9'), (271937088, 66391, 238, 'a96f6824b5a48a4ddf42c9dff00d81aad5c071a410a449a2ee37dfe2fbf58a3d'), (271941664, 66392, 690, '7bfd359b3e657637306ff361139c64be3691220c29e2d43f12bab1d5ec8a41d0'), (271944432, 66392, 13, '4619513e61a9f6bb87e3009fb57654b5f7589ff13fb641523820fd74a5ee3a10'), (271944496, 66392, 248, 'c1585cd42c3343dd3ec952b84240feb0abae5a6ec44190c21ddb321081e63664'))
CLUB_SHORTLIST_STORAGE_ENTRIES = tuple(start for start, _, _, _ in CONTRACTS)
PREPARED_CLUB_SHORTLIST_STORAGE_ENTRIES = CLUB_SHORTLIST_STORAGE_ENTRIES


def fingerprint(body):
    canonical = [{key: r.get(key) for key in ('address', 'assembly', 'ops', 'flows', 'fallthrough')} for r in body]
    return hashlib.sha256(json.dumps(canonical, sort_keys=True, separators=(',', ':')).encode()).hexdigest()


def require_club_shortlist_storage_bodies(instructions, complete=False):
    for start, end, count, expected in CONTRACTS:
        body = [r for r in instructions if start <= r['address'] < end]
        if not body and not complete: continue
        if len(body) != count or fingerprint(body) != expected:
            raise ValueError(f'Mac club-shortlist-storage contract changed or missing at {start:x}')


def require_club_shortlist_storage_edges(edges):
    for source, target in edges:
        for start, end, _, _ in CONTRACTS:
            if start < target < end and not start <= source < end:
                raise ValueError(f'External club-shortlist-storage interior edge from {source:x} to {target:x}')


def require_club_shortlist_storage_contract(page, instructions, selected=None):
    addresses = {r['address'] for r in instructions} | set(selected or ())
    for start, end, _, _ in CONTRACTS:
        if not start >> 12 <= page <= (end - 1) >> 12: continue
        low, high = max(start, page << 12), min(end, (page + 1) << 12)
        if start not in addresses and not any(low <= a < high for a in addresses): continue
        body = [r for r in instructions if low <= r['address'] < high]
        _, _, count, expected = next(f for f in FRAGMENTS if f[0] == start and f[1] == page)
        if len(body) != count or fingerprint(body) != expected:
            raise ValueError(f'Mac club-shortlist-storage contract changed or missing at {start:x} page {page:x}')


def require_club_shortlist_storage_index(db, selected=None):
    records = [json.loads(row[0]) for start, end, _, _ in CONTRACTS for row in db.execute(
        'SELECT body FROM instructions WHERE address>=? AND address<? ORDER BY address', (start, end))]
    if selected is None: require_club_shortlist_storage_bodies(records, complete=True)
    else:
        for page in sorted({f[1] for f in FRAGMENTS}):
            require_club_shortlist_storage_contract(page, [r for _, r in sorted(selected.items())], selected)
    conditions = ' OR '.join('(e.value>=? AND e.value<?)' for _ in CONTRACTS)
    parameters = [value for start, end, _, _ in CONTRACTS for value in (start, end)]
    require_club_shortlist_storage_edges(db.execute("""SELECT i.address,e.value
        FROM instructions AS i,
          json_each(json_insert(json_extract(i.body,'$.flows'),'$[#]',json_extract(i.body,'$.fallthrough'))) AS e
        WHERE """ + conditions, parameters))


TABLES = ((277572792, 276870732, 41, 'ce07ac15ec347dbdfdd04f996404bc24d13385e5b0400df475d23959b9d5d4b7'), (277572788, 276871384, 16, '051eec9cc699d2886923b2ab7dba0f7f98fb8473e67773d956a16466a4fefb2a'), (277567476, 277582372, 8, '33086940b56e2c760b96e724edce826f78da5ce00634004a8b2e51a6635d03c3'), (277567480, 277582380, 8, '3203ea67eaa0c1d4d49ef46930b25ed45bd72110025d9217a29f38b90359d681'))

def require_club_shortlist_storage_tables(read):
    for slot,base,size,expected in TABLES:
        if struct.unpack(">I",read(slot,4))[0]!=base or hashlib.sha256(read(base,size)).hexdigest()!=expected:
            raise ValueError("Changed original club shortlist storage table")

def load_club_shortlist_storage_tables(directory):
    program=json.loads((directory/"program.json").read_text())
    segments=[(s["address"],(directory/s["file"]).read_bytes()) for s in program["segments"] if s.get("file")]
    def read(a,n):
        for base,data in segments:
            if base<=a and a+n<=base+len(data):return data[a-base:a-base+n]
        raise ValueError("Missing original club-shortlist-storage table")
    require_club_shortlist_storage_tables(read)
    require_club_shortlist_storage_padding(read)
    return tuple(read(base,size) for _,base,size,_ in TABLES)


PADDING = (271935468, 271936192, 271942692, 271943144, 271945324)

def require_club_shortlist_storage_padding(read):
    for address in PADDING:
        if read(address,4)!=bytes.fromhex("60000000"):
            raise ValueError("Changed non-indexed shortlist padding")
