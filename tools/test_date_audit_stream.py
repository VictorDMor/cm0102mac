import gzip
import lzma
import tempfile
import unittest
from pathlib import Path
from date_audit_stream import boundaries

ENTRY = 0x10715880


def chunk(kind, entry, index, body=b"private fixture bytes"):
    return bytes([kind]) + entry.to_bytes(4, "big") + index.to_bytes(8, "big") + len(body).to_bytes(8, "big") + body


class DateStreamTests(unittest.TestCase):
    def parse(self, raw):
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / "date.gz"
            with gzip.open(path, "wb") as stream:
                stream.write(raw)
            return boundaries(path)

    def test_nested_own_counts_and_first_indices(self):
        raw = b"CMDAT001" + chunk(1, ENTRY, 0) + chunk(1, ENTRY, 1) + chunk(2, ENTRY, 1) + chunk(2, ENTRY, 0)
        matrix = self.parse(raw)
        self.assertEqual(len(matrix), 26)
        self.assertEqual(matrix["0x10715880"], {"calls": 2, "firstIndex": 0})
        self.assertEqual(matrix["0x10718440"], {"calls": 0, "firstIndex": None})

    def test_truncation_entry_size_sequence_and_nesting(self):
        valid = b"CMDAT001" + chunk(1, ENTRY, 0) + chunk(2, ENTRY, 0)
        cases = [valid[:4], valid[:19], valid[:-1], valid + b"extra",
                 b"CMDAT001" + chunk(1, 0xdeadbeef, 0),
                 b"CMDAT001" + chunk(1, ENTRY, 1),
                 b"CMDAT001" + chunk(2, ENTRY, 0),
                 b"CMDAT001" + chunk(1, ENTRY, 0),
                 b"CMDAT001" + chunk(1, ENTRY, 0) + chunk(2, 0x10718440, 0),
                 b"CMDAT001" + chunk(3, ENTRY, 0),
                 b"CMDAT001" + bytes([1]) + ENTRY.to_bytes(4, "big") + bytes(8) + (129*1024*1024).to_bytes(8, "big")]
        for raw in cases:
            with self.subTest(length=len(raw)), self.assertRaises(ValueError):
                self.parse(raw)

    def test_checksum_is_consumed(self):
        raw = b"CMDAT001" + chunk(1, ENTRY, 0) + chunk(2, ENTRY, 0)
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / "date.gz"
            data = bytearray(gzip.compress(raw))
            data[-8] ^= 1
            path.write_bytes(data)
            with self.assertRaises(OSError):
                boundaries(path)

    def test_lossless_archive_preserves_counts_and_gzip_eof(self):
        raw = b"CMDAT001" + chunk(1, ENTRY, 0) + chunk(2, ENTRY, 0)
        expected = self.parse(raw)
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / "date.gz.xz"
            encoded = gzip.compress(raw)
            path.write_bytes(lzma.compress(encoded))
            self.assertEqual(boundaries(path), expected)
            damaged = bytearray(encoded)
            damaged[-8] ^= 1
            path.write_bytes(lzma.compress(damaged))
            with self.assertRaises(OSError):
                boundaries(path)


if __name__ == "__main__":
    unittest.main()
