#!/usr/bin/env python3
"""Malformed database/patch input must fail before private pack publication."""
import struct
import tempfile
import unittest
from unittest.mock import patch
from pathlib import Path

import season_databases as packs
import language_compat


def fixture():
    names = {kind: f"table{kind}.dat" for kind in packs.RECORDS}
    files = {name: bytes(8) for name in names.values()}
    index = bytearray(1482)
    for i, (kind, (stride, version)) in enumerate(packs.RECORDS.items()):
        at = 8 + i * 67
        name = names[kind].encode()
        index[at:at + len(name)] = name
        struct.pack_into("<4I", index, at + 51, kind, 0, 0, version)
    files["index.dat"] = bytes(index)
    files["eng.lng"] = bytes(32)
    return files


class DatabasePacks(unittest.TestCase):
    def test_index_extents_and_language_counts(self):
        files = fixture()
        self.assertEqual(len(packs.validate_windows_database(files)), 22)
        index = bytearray(files["index.dat"])
        struct.pack_into("<I", index, 8 + 55, 1000000)
        files["index.dat"] = bytes(index)
        with self.assertRaisesRegex(ValueError, "extent/version"):
            packs.validate_windows_database(files)
        files = fixture()
        files["eng.lng"] = struct.pack("<8I", 1, 0, 0, 0, 0, 0, 0, 0)
        with self.assertRaisesRegex(ValueError, "counts disagree"):
            packs.validate_windows_database(files)

    def test_missing_and_duplicate_tables(self):
        files = fixture()
        del files["table0.dat"]
        with self.assertRaisesRegex(ValueError, "Missing"):
            packs.validate_windows_database(files)
        files = fixture()
        index = bytearray(files["index.dat"])
        index[75:142] = index[8:75]
        files["index.dat"] = bytes(index)
        with self.assertRaisesRegex(ValueError, "duplicated"):
            packs.validate_windows_database(files)

    def test_patch_boundary_and_identity(self):
        for payload in (bytes(20), bytes(185), struct.pack("<46I", *([0xffffffff] * 46))):
            with self.assertRaises(ValueError):
                packs.extract_sections(payload)
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            archive = root / "patch.zip"
            archive.write_bytes(b"wrong archive")
            with self.assertRaisesRegex(ValueError, "audited"):
                packs.import_patch(archive, root / "pack")
            self.assertFalse((root / "pack").exists())

    def test_database_only_allowlist(self):
        self.assertEqual(len(packs.PATCH_DATA), 31)
        self.assertFalse(any(name.endswith((".exe", ".fnt", ".pct", ".tct")) for name in packs.PATCH_DATA.values()))
        self.assertNotIn("langlib", packs.PATCH_DATA.values())
        self.assertNotIn("weather.cfg", packs.PATCH_DATA.values())

    def test_cm0001_directory_and_identity(self):
        sections = [bytes([i]) * (i + 1) for i in range(57)]
        sizes = [len(section) for section in sections]
        sizes[42] = 496
        payload = struct.pack("<57I", *sizes) + b"".join(
            data for i, data in enumerate(sections) if i != 42)
        with patch.object(packs, "validate_windows_database"):
            files = packs.extract_cm0001_sections(payload)
        self.assertEqual(len(files), 30)
        self.assertEqual(files["index.dat"], sections[21])
        self.assertEqual(files["eng.lng"], sections[33])
        self.assertEqual(files["ger.lng"], sections[52])
        self.assertEqual(files["player_setup.cfg"], sections[11])
        self.assertLessEqual(files.keys(), set(packs.PATCH_DATA.values()))
        for malformed in (payload[:20], payload[:-1], payload + b"x"):
            with self.assertRaises(ValueError):
                packs.extract_cm0001_sections(malformed)
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            archive = root / "patch.zip"
            archive.write_bytes(b"unrecognized source")
            with self.assertRaisesRegex(ValueError, "audited"):
                packs.import_cm0001(archive, root / "pack")
            self.assertFalse((root / "pack").exists())

    def test_existing_pack_preserved(self):
        with tempfile.TemporaryDirectory() as directory:
            target = Path(directory) / "pack"
            target.mkdir()
            (target / "keep").write_bytes(b"preserve")
            with self.assertRaisesRegex(ValueError, "already exists"):
                packs.write_pack(target, {}, "pack", "2002/2003", 2002, {})
            self.assertEqual((target / "keep").read_bytes(), b"preserve")

    def test_cm0001_presentation_compatibility_preserves_football_data(self):
        files = fixture()
        files["table2.dat"] = b""
        colour = bytearray(64)
        colour[4:12] = b"Green 4\0"
        colour[55:58] = bytes((10, 120, 30))
        original = {"colours colour.dam": bytes(colour)}
        updated = packs.cm0001_engine_resources(files, original)
        self.assertEqual(updated["dan.lng"], files["eng.lng"])
        self.assertEqual(updated["table2.dat"], bytes(colour[:58]))
        for name, data in files.items():
            if name not in ("table2.dat", "index.dat"):
                self.assertEqual(updated[name], data)
        self.assertEqual(files["table2.dat"], b"")
        self.assertEqual(packs.cm0001_engine_resources(updated, original), updated)
        counts = {e["kind"]: e["count"] for e in packs.validate_windows_database(updated)}
        self.assertEqual(counts[2], 1)

    def test_existing_inventory_verified(self):
        with tempfile.TemporaryDirectory() as directory, patch.object(packs, "align_english", side_effect=lambda data: data):
            root = Path(directory)
            game, runtime = root / "game", root / "runtime"
            (game / "Data").mkdir(parents=True)
            (game / "Data/index.dam").write_bytes(b"fixture index")
            (game / "Data/eng.lng").write_bytes(b"fixture language")
            installed = packs.install_database_packs(game, runtime, root / "no-packs")
            self.assertEqual(packs.install_database_packs(game, runtime, root / "no-packs"), installed)
            inventory = runtime / "Files/Databases" / installed[0][2] / "files.txt"
            inventory.write_text("changed")
            with self.assertRaisesRegex(ValueError, "inventory preserved"):
                packs.install_database_packs(game, runtime, root / "no-packs")
            self.assertEqual(inventory.read_text(), "changed")

    def test_archived_season_language_survives_reinstall(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            game, runtime = root / "game", root / "runtime"
            (game / "Data").mkdir(parents=True)
            (runtime / "Files/Data").mkdir(parents=True)
            (game / "Data/clubs club.dam").write_bytes(b"fixture clubs")
            (game / "Data/eng.lng").write_bytes(b"original")
            target = runtime / "Files/Data/eng.lng"
            target.write_bytes(b"archived 2002 language")
            with patch.object(language_compat, "ORIGINAL_CLUBS", packs.digest(b"fixture clubs")), \
                 patch.object(language_compat, "align_english", return_value=b"aligned"):
                language_compat.install_index_compatibility(game, runtime, {packs.digest(target.read_bytes())})
                self.assertEqual(target.read_bytes(), b"archived 2002 language")
                with self.assertRaisesRegex(ValueError, "preserved"):
                    language_compat.install_index_compatibility(game, runtime, {packs.digest(b"different")})
                self.assertEqual(target.read_bytes(), b"archived 2002 language")


class History2002(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        # Sparse synthetic tables, with the audited release's reference slots.
        # No original game bytes are checked into the repository.
        files = fixture()
        del files["eng.lng"]
        index = bytearray(files["index.dat"])
        cls.count_offsets = {}
        counts = {0: 13214, 1: 426, 4: 28, 6: 166161, 11: 160, 12: 433,
                  13: 40262, 14: 99683, 16: 19, 18: 1, 19: 1, 20: 1}
        for i, (kind, (stride, _)) in enumerate(packs.RECORDS.items()):
            cls.count_offsets[kind] = 8 + i * 67 + 55
            count = counts.get(kind, 0)
            struct.pack_into("<I", index, cls.count_offsets[kind], count)
            files[f"table{kind}.dat"] = bytearray(count * stride)

        def name(kind, row, text, offset=4):
            data = text.encode("latin1") + b"\0"
            at = row * packs.RECORDS[kind][0] + offset
            files[f"table{kind}.dat"][at:at + len(data)] = data

        name(16, 14, "FIFA World Cup")
        for row, text in ((27, "Brazil"), (74, "Germany"), (192, "Turkey")):
            name(1, row, text)
        name(4, 27, "Brazil")
        name(11, 159, "World Cup Golden Boot")
        name(13, 40261, "Ronaldo Luiz", 0)
        name(14, 99682, "Nazário de Lima", 0)
        at = 166160 * 157
        struct.pack_into("<4i", files["table6.dat"], at, 166160, 40261, 99682, 0)
        struct.pack_into("<i", files["table6.dat"], at + 0x1a, 27)
        for row, text in ((326, "European Champions Cup"), (328, "UEFA Cup")):
            name(12, row, text)
        for row, text in ((9413, "Real Madrid C.F."), (6524, "Bayer 04 Leverkusen"),
                          (4012, "Feyenoord"), (3234, "BV Borussia Dortmund")):
            name(0, row, text)
        files["table19.dat"] = struct.pack("<iih4i", 0, 326, 2001, 10, 11, -1, -1)
        files["table20.dat"] = struct.pack("<iih4i", 1, 447, 1998, 12, 13, 14, 12)
        files["table18.dat"] = struct.pack("<iih12i", 0, 159, 1998, *([-1] * 12))
        files["index.dat"] = index
        cls.files = {key: bytes(value) for key, value in files.items()}

    def test_results_counts_and_combined_ids(self):
        updated = packs.complete_2002_history(self.files)
        self.assertEqual(list(struct.iter_unpack("<iih4i", updated["table19.dat"]))[1:],
                         [(1, 326, 2002, 9413, 6524, -1, -1),
                          (2, 328, 2002, 4012, 3234, -1, -1)])
        self.assertEqual(list(struct.iter_unpack("<iih4i", updated["table20.dat"]))[-1],
                         (4, 447, 2002, 13241, 13288, 13406, -1))
        self.assertEqual(struct.unpack("<iih12i", updated["table18.dat"][-58:]),
                         (1, 159, 2002, 40261, 99682, 166160, 8,
                          -1, -1, -1, 0, -1, -1, -1, 0))
        counts = {e["kind"]: e["count"] for e in packs.validate_windows_database(updated)}
        self.assertEqual([counts[k] for k in (18, 19, 20)], [2, 3, 2])
        combined = updated["table19.dat"] + updated["table20.dat"]
        self.assertEqual([r[0] for r in struct.iter_unpack("<iih4i", combined)], list(range(5)))

    def test_original_bytes_and_source_preserved(self):
        before = dict(self.files)
        updated = packs.complete_2002_history(self.files)
        self.assertEqual(before, self.files)
        self.assertEqual({k for k in updated if updated[k] != before[k]},
                         {"index.dat", "table18.dat", "table19.dat", "table20.dat"})
        for kind, stride in ((18, 58), (19, 26)):
            self.assertEqual(updated[f"table{kind}.dat"][:stride], before[f"table{kind}.dat"])
        self.assertEqual(updated["table20.dat"][4:26], before["table20.dat"][4:26])
        index = bytearray(updated["index.dat"])
        for kind in (18, 19, 20):
            at = self.count_offsets[kind]
            index[at:at + 4] = before["index.dat"][at:at + 4]
        self.assertEqual(bytes(index), before["index.dat"])

    def test_repeated_and_partial_correction(self):
        updated = packs.complete_2002_history(self.files)
        self.assertEqual(packs.complete_2002_history(updated), updated)
        partial = dict(updated)
        partial["table19.dat"] = partial["table19.dat"][:-26]
        index = bytearray(partial["index.dat"])
        struct.pack_into("<I", index, self.count_offsets[19], 2)
        partial["index.dat"] = bytes(index)
        self.assertEqual(packs.complete_2002_history(partial), updated)

    def test_conflict_never_overwrites_history(self):
        for kind, stride, offset in ((18, 58, 22), (19, 26, 10), (20, 26, 10)):
            files = packs.complete_2002_history(self.files)
            data = bytearray(files[f"table{kind}.dat"])
            struct.pack_into("<i", data, len(data) - stride + offset, 123)
            files[f"table{kind}.dat"] = bytes(data)
            before = dict(files)
            with self.assertRaisesRegex(ValueError, "Conflicting"):
                packs.complete_2002_history(files)
            self.assertEqual(files, before)

    def test_wrong_references_and_truncated_history(self):
        files = dict(self.files)
        data = bytearray(files["table14.dat"])
        data[99682 * 60] = ord("X")
        files["table14.dat"] = bytes(data)
        with self.assertRaisesRegex(ValueError, "reference"):
            packs.complete_2002_history(files)
        files = dict(self.files)
        files["table20.dat"] = files["table20.dat"][:-1]
        with self.assertRaisesRegex(ValueError, "extent/version"):
            packs.complete_2002_history(files)


if __name__ == "__main__":
    unittest.main()
