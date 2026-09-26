"""Boundary tests for the read-only parser and separate-copy save repair."""
import struct
import unittest

from language_compat import align_english
from repair_save_language import PAYLOAD_START, compress, decompress, entries, repair


def fixture():
    data = bytearray(PAYLOAD_START)
    data[0] = 4
    struct.pack_into(">I", data, 4, 2)
    struct.pack_into(">II", data, 8, PAYLOAD_START, 5)
    data[16:21] = b"first"
    struct.pack_into(">II", data, 272, PAYLOAD_START + 3, 1)
    data[280:286] = b"second"
    # Independent literal fixture: four As, a B, and one high-bit byte.
    return data + b"\x84AB\x81\xff"


class SaveLanguageChecks(unittest.TestCase):
    def test_literal_and_run_boundaries(self):
        self.assertEqual(decompress(b"\x00\x80\x83A\x81\xff", 6), b"\0\x80AAA\xff")
        self.assertEqual(compress(b"\0\x80AAA\xff"), b"\x00\x80\x83A\x81\xff")
        self.assertEqual(compress(b"A" * 128), b"\xffAA")
        self.assertEqual(decompress(b"\xffAA", 128), b"A" * 128)

    def test_empty_block(self):
        self.assertEqual(decompress(b"", 0), b"")

    def test_truncated_or_overlong_runs(self):
        for payload, expected in [(b"\x83", 3), (b"\x83A", 2), (b"A", 2), (b"AB", 1)]:
            with self.subTest(payload=payload, expected=expected), self.assertRaises(ValueError):
                decompress(payload, expected)

    def test_container_payloads(self):
        result = entries(fixture())
        self.assertEqual([(e.name, e.size, e.payload) for e in result],
                         [("first", 5, b"\x84AB"), ("second", 1, b"\x81\xff")])

    def test_invalid_boundaries(self):
        for offset in [PAYLOAD_START - 1, PAYLOAD_START + 100]:
            data = fixture()
            struct.pack_into(">I", data, 272, offset)
            with self.subTest(offset=offset), self.assertRaises(ValueError):
                entries(data)

    def test_duplicate_names(self):
        data = fixture()
        data[280:286] = b"first\0"
        with self.assertRaises(ValueError):
            entries(data)

    def test_unknown_assets_are_rejected(self):
        with self.assertRaises(ValueError):
            align_english(b"unrecognized language data")
        with self.assertRaises(ValueError):
            repair(fixture())

    def test_invalid_header(self):
        for data in [b"", bytes(PAYLOAD_START)]:
            with self.assertRaises(ValueError):
                entries(data)


if __name__ == "__main__":
    unittest.main()
