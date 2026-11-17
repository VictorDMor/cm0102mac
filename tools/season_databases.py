#!/usr/bin/env python3
"""Import database-only season packs; never execute the Windows installer."""
from __future__ import annotations

import argparse
import hashlib
import io
import json
from pathlib import Path
import re
import shutil
import struct
import tempfile
import zipfile

from language_compat import align_english

PROJECT = Path(__file__).resolve().parent.parent
PATCH_SHA256 = "c69de353bfd08859fc7efef110caa5efead2ffc60dd763733dfe7784515d57bd"
CM0001_SHA256 = "8686a17c35fd51cc8539ddde3dc468171445720239b06a41b4c553afedda9283"
# update.dat is 46 little-endian lengths followed by 46 uncompressed files.
# Only these database sections are imported. Executables, langlib (UI text),
# weather, fonts, tactics and the editor remain those of the Mac engine.
PATCH_DATA = dict(enumerate((
    "dan.lng", "eng.lng", "esp.lng", "fra.lng", "ita.lng", "nor.lng",
    "por.lng", "swe.lng", "hol.lng", "ger.lng"), 2))
PATCH_DATA[13] = "player_setup.cfg"
PATCH_DATA.update(enumerate((
    "city.dat", "club.dat", "club_comp.dat", "club_comp_history.dat",
    "colour.dat", "common_names.dat", "continent.dat", "first_names.dat",
    "index.dat", "nat_club.dat", "nation.dat", "nation_comp.dat",
    "nation_comp_history.dat", "officials.dat", "second_names.dat",
    "stadium.dat", "staff.dat", "staff_comp.dat", "staff_comp_history.dat",
    "staff_history.dat"), 16))
# CM 00/01 3.89 uses the same packed database layouts, with nine languages.
CM0001_DATA = {11: "player_setup.cfg", 52: "ger.lng"}
CM0001_DATA.update({i - 3: name for i, name in PATCH_DATA.items() if 16 <= i <= 35})
CM0001_DATA.update(enumerate(("eng.lng", "esp.lng", "fra.lng", "ita.lng",
                              "nor.lng", "por.lng", "swe.lng", "hol.lng"), 33))
# The existing DB_FILES readers accept precisely these packed record layouts.
RECORDS = {0: (581, 2), 1: (581, 2), 2: (58, 1), 3: (198, 1),
           4: (290, 2), 5: (78, 1), 6: (157, 1), 7: (43, 1),
           8: (0, 1), 9: (68, 2), 10: (70, 2), 11: (101, 2),
           12: (107, 2), 13: (60, 1), 14: (60, 1), 15: (60, 1),
           16: (107, 2), 17: (17, 1), 18: (58, 1), 19: (26, 1),
           20: (26, 1), 21: (56, 1)}


def digest(data):
    return hashlib.sha256(data).hexdigest()


def extract_sections(payload):
    if len(payload) < 184:
        raise ValueError("Truncated patch directory")
    sizes = struct.unpack_from("<46I", payload)
    if 184 + sum(sizes) != len(payload):
        raise ValueError("Patch section sizes do not cover the payload")
    result = {}
    offset = 184
    for index, size in enumerate(sizes):
        if index in PATCH_DATA:
            result[PATCH_DATA[index]] = payload[offset:offset + size]
        offset += size
    validate_windows_database(result)
    return result


def validate_windows_database(files):
    index = files["index.dat"]
    if len(index) != 1482 or index[:8] != bytes(8):
        raise ValueError("Unsupported database index header")
    entries, kinds = [], set()
    for number in range(22):
        at = 8 + number * 67
        raw_name = index[at:at + 51]
        if b"\0" not in raw_name:
            raise ValueError("Unterminated database filename")
        name = raw_name.split(b"\0", 1)[0].decode("ascii")
        kind, count, offset, version = struct.unpack_from("<4I", index, at + 51)
        if kind not in RECORDS or kind in kinds or name not in files:
            raise ValueError("Missing, duplicated or unsupported database table")
        stride, expected_version = RECORDS[kind]
        if version != expected_version or offset + count * stride > len(files[name]):
            raise ValueError(f"Invalid table extent/version: {name}, kind {kind}")
        if kind == 8 and count:
            raise ValueError("Unsupported unused staff section")
        kinds.add(kind)
        entries.append(dict(name=name, kind=kind, count=count, offset=offset,
                            version=version, stride=stride))
    if kinds != RECORDS.keys():
        raise ValueError("Incomplete database index")
    # Language records have their own counts, in a different order from index.
    counts = {entry["kind"]: entry["count"] for entry in entries}
    expected = (counts[0] + counts[1], counts[3], counts[16], counts[4],
                counts[21], counts[12], counts[5], counts[11])
    for name, data in files.items():
        if name.endswith(".lng"):
            if len(data) < 32 or struct.unpack_from("<8I", data) != expected:
                raise ValueError(f"Language/table counts disagree: {name}")
            if len(data) != 32 + sum(n * size for n, size in zip(expected, (79, 186, 83, 109, 52, 83, 52, 79))):
                raise ValueError(f"Invalid language extent: {name}")
    return entries


def write_pack(destination, files, identity, label, start_year, source):
    if destination.exists():
        raise ValueError(f"Pack already exists; preserved: {destination}")
    destination.parent.mkdir(parents=True, exist_ok=True)
    manifest = dict(format=1, id=identity, label=label, start_year=start_year,
                    source=source, files={name: dict(size=len(data), sha256=digest(data))
                                          for name, data in sorted(files.items())})
    with tempfile.TemporaryDirectory(prefix=".pack-", dir=destination.parent) as staging:
        stage = Path(staging) / identity
        (stage / "Data").mkdir(parents=True)
        for name, data in files.items():
            (stage / "Data" / name).write_bytes(data)
        (stage / "pack.json").write_text(json.dumps(manifest, indent=2) + "\n")
        stage.rename(destination)
    return manifest


def import_patch(archive, destination):
    data = archive.read_bytes()
    if digest(data) != PATCH_SHA256:
        raise ValueError("Not the audited official 3.9.68 patch archive")
    with zipfile.ZipFile(io.BytesIO(data)) as outer:
        with zipfile.ZipFile(io.BytesIO(outer.read("242.exe"))) as inner:
            files = extract_sections(inner.read("update.dat"))
    return write_pack(destination, files, "si-3968", "2002/2003", 2002,
                      {"release": "Sports Interactive 3.9.68", "sha256": PATCH_SHA256})


def extract_cm0001_sections(payload):
    if len(payload) < 228:
        raise ValueError("Truncated CM 00/01 patch directory")
    sizes = struct.unpack_from("<57I", payload)
    # Directory slot 42 (readme.txt) retains a 496-byte length but has no
    # payload: the installer ships its localized readmes beside update.dat.
    if sizes[42] != 496 or 228 + sum(sizes) - sizes[42] != len(payload):
        raise ValueError("CM 00/01 patch section sizes do not cover the payload")
    files, offset = {}, 228
    for index, size in enumerate(sizes):
        if index == 42:
            continue
        if index in CM0001_DATA:
            files[CM0001_DATA[index]] = payload[offset:offset + size]
        offset += size
    validate_windows_database(files)
    return files


def import_cm0001(archive, destination):
    data = archive.read_bytes()
    if digest(data) != CM0001_SHA256:
        raise ValueError("Not the audited official CM 00/01 3.89 patch archive")
    with zipfile.ZipFile(io.BytesIO(data)) as outer:
        with zipfile.ZipFile(io.BytesIO(outer.read("cm00_389.exe"))) as inner:
            files = extract_cm0001_sections(inner.read("update.dat"))
    return write_pack(destination, files, "si-389", "2000/2001", 2000,
                      {"release": "Sports Interactive CM 00/01 3.89", "sha256": CM0001_SHA256})


def cm0001_engine_resources(files, original):
    """Supply required presentation resources, without adding football records."""
    files = dict(files)
    # The archive writer always includes all ten database-name languages.
    # CM 00/01 has no Danish translation; use that season's English names.
    files.setdefault("dan.lng", files["eng.lng"])
    entries = validate_windows_database(files)
    colours = next(entry for entry in entries if entry["kind"] == 2)
    data = files[colours["name"]]
    if any(data[i + 4:i + 55].split(b"\0", 1)[0] == b"Green 4"
           for i in range(colours["offset"], colours["offset"] + colours["count"] * 58, 58)):
        return files
    # Green 4 is used by the CM01/02 interface. Append its original Mac RGB
    # record so every CM00/01 club's existing colour ID remains unchanged.
    palette = original["colours colour.dam"]
    candidates = [palette[i:i + 58] for i in range(0, len(palette) - 59, 60)
                  if palette[i + 4:i + 55].split(b"\0", 1)[0] == b"Green 4"]
    if len(candidates) != 1 or colours["offset"] != 0 or len(data) != colours["count"] * 58:
        raise ValueError("Unsupported CM 00/01 colour compatibility input")
    files[colours["name"]] = data + struct.pack("<I", colours["count"]) + candidates[0][4:]
    index = bytearray(files["index.dat"])
    for number in range(22):
        at = 8 + number * 67
        if struct.unpack_from("<I", index, at + 51)[0] == 2:
            struct.pack_into("<I", index, at + 55, colours["count"] + 1)
    files["index.dat"] = bytes(index)
    validate_windows_database(files)
    return files


def complete_2002_history(files):
    """Complete the audited 3.9.68 history in an installation copy only.

    Sources (checked 2026-11-17):
    https://www.fifa.com/pt/articles/copa-do-mundo-de-2002-videos
    https://www.fifa.com/pt/tournaments/mens/worldcup/articles/video-todos-gols-ronaldo-brasil-copa-do-mundo-1998-2002-2006
    https://www.uefa.com/uefachampionsleague/match/69489/
    https://www.uefa.com/uefaeuropaleague/match/69455/
    The first lists Brazil/Germany/Turkey and the second Ronaldo's eight goals.
    The original format has one host slot (not fourth place), so leave it
    unknown rather than misrepresent the joint Korea/Japan hosts. Likewise,
    leave the award runners-up empty: FIFA lists Rivaldo and Klose as joint
    Silver Shoe winners, while these slots impose second and third place.
    """
    entries = validate_windows_database(files)
    tables = {entry["kind"]: entry for entry in entries}
    result = dict(files)
    index = bytearray(files["index.dat"])

    def record(kind, row):
        table = tables[kind]
        if not 0 <= row < table["count"]:
            raise ValueError(f"Missing 2002 history reference: kind {kind}, row {row}")
        at = table["offset"] + row * table["stride"]
        return result[table["name"]][at:at + table["stride"]]

    def named(kind, row, name, offset=4):
        data = record(kind, row)
        if data[offset:offset + 51].split(b"\0", 1)[0].decode("latin1") != name:
            raise ValueError(f"Unexpected 2002 history reference: {name}")
        return data

    # IDs refer to the combined club/national-team and competition tables.
    competition = 447
    named(16, competition - tables[12]["count"], "FIFA World Cup")
    podium = (13241, 13288, 13406)
    for team, name in zip(podium, ("Brazil", "Germany", "Turkey")):
        named(1, team - tables[0]["count"], name)
    named(11, 159, "World Cup Golden Boot")
    ronaldo = 166160
    person = record(6, ronaldo)
    first, last = struct.unpack_from("<2i", person, 4)
    named(13, first, "Ronaldo Luiz", 0)
    named(14, last, "Nazário de Lima", 0)
    if struct.unpack_from("<i", person, 0x1a)[0] != 27:
        raise ValueError("Unexpected Ronaldo nationality")
    named(4, 27, "Brazil")
    for row, name in ((326, "European Champions Cup"), (328, "UEFA Cup")):
        named(12, row, name)
    for row, name in ((9413, "Real Madrid C.F."), (6524, "Bayer 04 Leverkusen"),
                      (4012, "Feyenoord"), (3234, "BV Borussia Dortmund")):
        named(0, row, name)

    def append(kind, layout, fields, shared_kinds):
        table = tables[kind]
        data = result[table["name"]]
        if table["offset"] != 0 or len(data) != table["count"] * layout.size:
            raise ValueError("Unsupported 2002 history table extent")
        rows = list(layout.iter_unpack(data))
        existing = [row for row in rows if row[1:3] == fields[:2]]
        if existing:
            if len(existing) != 1 or existing[0][1:] != fields:
                raise ValueError("Conflicting 2002 competition history preserved")
            return
        # National/club histories share an ID space; awards have their own.
        ids = [struct.unpack_from("<i", record(other, row))[0]
               for other in shared_kinds for row in range(tables[other]["count"])]
        result[table["name"]] = data + layout.pack(max(ids, default=-1) + 1, *fields)
        directory_entry = entries.index(table)
        struct.pack_into("<I", index, 8 + directory_entry * 67 + 55, table["count"] + 1)
        table["count"] += 1

    append(19, struct.Struct("<iih4i"), (326, 2002, 9413, 6524, -1, -1), (19, 20))
    append(19, struct.Struct("<iih4i"), (328, 2002, 4012, 3234, -1, -1), (19, 20))
    append(20, struct.Struct("<iih4i"), (competition, 2002, *podium, -1), (19, 20))
    # Each award placing stores first name, surname, staff ID and integer score.
    append(18, struct.Struct("<iih12i"),
           (159, 2002, first, last, ronaldo, 8, -1, -1, -1, 0, -1, -1, -1, 0), (18,))
    # These two files concatenate into one table. Inserting club rows shifts
    # national row IDs; all other bytes of pre-existing histories stay intact.
    identity = 0
    for kind in (19, 20):
        table = tables[kind]
        data = bytearray(result[table["name"]])
        for row in range(table["count"]):
            struct.pack_into("<i", data, row * 26, identity)
            identity += 1
        result[table["name"]] = bytes(data)
    result["index.dat"] = bytes(index)
    validate_windows_database(result)
    return result


def install_database_packs(game, runtime, packs=PROJECT / ".local/database-packs"):
    """Install private data/cache namespaces without changing original media.

    A content-addressed folder prevents caches from a previous pack revision
    being reused. Save loading uses the normal Data overlay, never these roots.
    """
    manifests = []
    original = {path.name: path.read_bytes() for path in (game / "Data").iterdir()
                if path.is_file() and (path.suffix.lower() in (".dam", ".lng")
                                       or path.name == "player_setup.cfg")}
    if "index.dam" not in original:
        raise ValueError("Original Mac database index is missing")
    original["eng.lng"] = align_english(original["eng.lng"])
    definitions = [(dict(format=1, id="original", label="2001/2002", start_year=2001), original)]
    if packs.exists():
        for manifest_path in sorted(packs.glob("*/pack.json")):
            manifest = json.loads(manifest_path.read_text())
            if (manifest.get("format") != 1 or not isinstance(manifest.get("id"), str)
                    or not re.fullmatch(r"[a-z0-9-]{1,32}", manifest["id"])
                    or manifest["id"] == "original"
                    or manifest.get("start_year") not in (2000, 2001, 2002)
                    or manifest.get("label") != f"{manifest['start_year']}/{manifest['start_year'] + 1}"):
                raise ValueError(f"Unsupported season pack: {manifest_path}")
            files = {}
            for name, expected in manifest["files"].items():
                if Path(name).name != name or name not in PATCH_DATA.values():
                    raise ValueError(f"Not a database-only pack member: {name}")
                data = (manifest_path.parent / "Data" / name).read_bytes()
                if len(data) != expected["size"] or digest(data) != expected["sha256"]:
                    raise ValueError(f"Season pack member changed: {name}")
                files[name] = data
            validate_windows_database(files)
            if manifest["id"] == "si-3968" and manifest["start_year"] == 2002:
                files = complete_2002_history(files)
            if manifest["id"] == "si-389" and manifest["start_year"] == 2000:
                files = cm0001_engine_resources(files, original)
            definitions.append((manifest, files))
    root = runtime / "Files/Databases"
    if root.is_symlink() or root.parent.is_symlink():
        raise ValueError("Database installation root must not be a symbolic link")
    root.mkdir(parents=True, exist_ok=True)
    for manifest, files in definitions:
        inventory = {name: dict(size=len(data), sha256=digest(data)) for name, data in sorted(files.items())}
        key = manifest["id"] + "-" + digest(json.dumps(inventory, sort_keys=True).encode())[:12]
        destination = root / key
        inventory_text = "CM_DATABASE_FILES 1\n" + "".join(
            f'{json.dumps(name)} {info["size"]}\n' for name, info in inventory.items())
        if destination.is_symlink() or (destination / "Data").is_symlink():
            raise ValueError(f"Database destination must not be a symbolic link: {destination}")
        if destination.exists():
            if ((destination / "files.txt").is_symlink() or
                    (destination / "files.txt").read_text() != inventory_text):
                raise ValueError(f"Changed private database inventory preserved: {destination}")
            for name, expected in inventory.items():
                path = destination / "Data" / name
                if path.is_symlink() or digest(path.read_bytes()) != expected["sha256"]:
                    raise ValueError(f"Changed private database preserved: {path}")
        else:
            with tempfile.TemporaryDirectory(prefix=".install-", dir=root) as staging:
                stage = Path(staging) / key
                (stage / "Data").mkdir(parents=True)
                for name, data in files.items():
                    (stage / "Data" / name).write_bytes(data)
                # A strict quoted-text inventory is also readable without a JSON
                # dependency by the host's small database catalog.
                (stage / "files.txt").write_text(inventory_text)
                stage.rename(destination)
        manifests.append((manifest["start_year"], manifest["label"], key))
    if len({label for _, label, _ in manifests}) != len(manifests):
        raise ValueError("Duplicate database season labels")
    # Arrows follow chronology, independently of the 2001 default. Future
    # manifests need engine compatibility before their years are enabled above.
    manifests.sort(key=lambda item: item[0])
    catalog = "CM_DATABASE_SEASONS 1\n" + "".join(
        f'{year} {json.dumps(label)} {json.dumps(key)}\n' for year, label, key in manifests)
    with tempfile.TemporaryDirectory(prefix=".catalog-", dir=root) as staging:
        staged = Path(staging) / "catalog.txt"
        staged.write_text(catalog)
        staged.replace(root / "catalog.txt")
    # Superseded revisions (and the conversion caches the game wrote in them)
    # are unreachable once the catalog stops naming them.
    live = {key for _, _, key in manifests}
    for stale in root.iterdir():
        if (stale.name not in live and not stale.is_symlink() and stale.is_dir()
                and re.fullmatch(r"[a-z0-9-]{1,32}-[0-9a-f]{12}", stale.name)
                and (stale / "files.txt").is_file()):
            shutil.rmtree(stale)
    return manifests


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("archive", type=Path)
    parser.add_argument("--release", choices=("3.9.68", "3.89"), default="3.9.68")
    parser.add_argument("--output", type=Path)
    args = parser.parse_args()
    output = args.output or PROJECT / ".local/database-packs" / ("si-389" if args.release == "3.89" else "si-3968")
    importer = import_cm0001 if args.release == "3.89" else import_patch
    manifest = importer(args.archive, output)
    print(f"Imported {manifest['label']}: {len(manifest['files'])} database files into {output}")


if __name__ == "__main__":
    main()
