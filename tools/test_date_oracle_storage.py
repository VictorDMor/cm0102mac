import gzip
import hashlib
import lzma
from pathlib import Path
import random
import sys
import tempfile
import unittest
from date_oracle_storage import DateOracle


class DateOracleStorageTests(unittest.TestCase):
    def test_identical_bytes_full_eof_early_negative_and_fresh_replay(self):
        payload = random.Random(42).randbytes(2 * 1024 * 1024)
        encoded = gzip.compress(payload, mtime=0)
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            original = root / "original.bin"
            original.write_bytes(encoded)
            oracle = DateOracle(root / "date.gz", archived=True)
            capture = "import sys; from pathlib import Path; Path(sys.argv[2]).write_bytes(Path(sys.argv[3]).read_bytes())"
            run = oracle.run([sys.executable, "-c", capture, "--date-record", str(oracle.path), str(original)], timeout=20)
            self.assertEqual(run.returncode, 0, run.stderr)
            self.assertEqual(oracle.digest(), hashlib.sha256(encoded).hexdigest())
            self.assertEqual(lzma.decompress(oracle.archive.read_bytes()), encoded)
            reader = "import gzip,hashlib,sys; data=gzip.open(sys.argv[2],'rb').read(); print(len(data),hashlib.sha256(data).hexdigest())"
            arguments = [sys.executable, "-c", reader, "--date-check", str(oracle.path)]
            expected = f"{len(payload)} {hashlib.sha256(payload).hexdigest()}\n"
            self.assertEqual(oracle.run(arguments, timeout=20).stdout, expected)
            early = "import sys; source=open(sys.argv[2],'rb',buffering=0); source.read(17); source.close(); sys.exit(1)"
            self.assertEqual(oracle.run([sys.executable, "-c", early, "--date-check", str(oracle.path)], timeout=20).returncode, 1)
            self.assertEqual(oracle.run(arguments, timeout=20).stdout, expected)
            self.assertEqual(oracle.digest(), hashlib.sha256(encoded).hexdigest())
            self.assertFalse(list(root.glob("*.pipe")))
            with self.assertRaises(FileExistsError):
                oracle.run([sys.executable, "-c", capture, "--date-record", str(oracle.path), str(original)])

    def test_archive_and_gzip_checksums_remain_required(self):
        with tempfile.TemporaryDirectory() as directory:
            oracle = DateOracle(Path(directory) / "date.gz", archived=True)
            encoded = bytearray(gzip.compress(b"observable data", mtime=0))
            encoded[-8] ^= 1
            oracle.archive.write_bytes(lzma.compress(encoded))
            reader = "import gzip,sys; gzip.open(sys.argv[2],'rb').read()"
            run = oracle.run([sys.executable, "-c", reader, "--date-check", str(oracle.path)], timeout=20)
            self.assertNotEqual(run.returncode, 0)
            archive = bytearray(oracle.archive.read_bytes())
            archive[-12] ^= 1
            oracle.archive.write_bytes(archive)
            with self.assertRaises(ValueError):
                oracle.digest()
            oracle.archive.write_bytes(bytes(archive[:20]))
            with self.assertRaises(ValueError):
                oracle.digest()

    def test_regular_storage_and_consumer_failure_before_open(self):
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / "date.gz"
            path.write_bytes(gzip.compress(b"regular", mtime=0))
            regular = DateOracle(path)
            self.assertEqual(regular.digest(), hashlib.sha256(path.read_bytes()).hexdigest())
            archived = DateOracle(path, archived=True)
            archived.archive.write_bytes(lzma.compress(path.read_bytes()))
            run = archived.run([sys.executable, "-c", "import sys; sys.exit(7)", "--date-check", str(path)], timeout=20)
            self.assertEqual(run.returncode, 7)
            self.assertFalse(list(path.parent.glob("*.pipe")))


if __name__ == "__main__":
    unittest.main()
