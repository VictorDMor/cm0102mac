#!/usr/bin/env python3
"""Index a completed local Ghidra export for bounded AOT compilation.

The database contains copyrighted game-derived input and stays under .local.
It is build input, never a component of the native runtime.
"""
import argparse
import gzip
import hashlib
import json
from pathlib import Path
import sqlite3


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("export", type=Path)
    args = parser.parse_args()
    metadata = json.loads((args.export / "program.json").read_text())
    if metadata["language"] != "PowerPC:BE:32:default":
        raise ValueError("Expected PowerPC 32-bit big-endian export")
    source = args.export / "instructions.jsonl.gz"
    digest = hashlib.file_digest(source.open("rb"), "sha256").hexdigest()
    destination = args.export / "instructions.sqlite"
    if destination.exists():
        with sqlite3.connect(destination) as db:
            if db.execute("SELECT value FROM metadata WHERE key='sha256'").fetchone() == (digest,):
                print("Existing instruction index matches export")
                return
        raise ValueError("Existing index does not match export; use a fresh output directory")
    pending = destination.with_suffix(".pending.sqlite")
    pending.unlink(missing_ok=True)
    with sqlite3.connect(pending) as db:
        db.execute("PRAGMA journal_mode=OFF")
        db.execute("CREATE TABLE instructions(address INTEGER PRIMARY KEY, body TEXT NOT NULL)")
        db.execute("CREATE TABLE metadata(key TEXT PRIMARY KEY, value TEXT NOT NULL)")
        count = 0
        batch = []
        with gzip.open(source, "rt") as stream:
            for line in stream:
                item = json.loads(line)
                batch.append((item["address"], line.rstrip()))
                count += 1
                if len(batch) == 4096:
                    db.executemany("INSERT INTO instructions VALUES(?,?)", batch)
                    batch.clear()
            db.executemany("INSERT INTO instructions VALUES(?,?)", batch)
        if count != metadata["instructionCount"]:
            raise ValueError("Instruction count does not match completed export")
        db.executemany("INSERT INTO metadata VALUES(?,?)", [
            ("sha256", digest), ("count", str(count)),
            ("executableSHA256", metadata["executableSHA256"]),
        ])
    pending.rename(destination)
    print(f"Indexed {count:,} instructions")


if __name__ == "__main__":
    main()
