#!/usr/bin/env python3
"""Replay archived report screens with the files belonging to their captured career."""
from pathlib import Path
import subprocess
import sys
from tempfile import TemporaryDirectory

from repair_save_language import decompress, entries


def main(checker, snapshot, career, mode="--group-stage"):
    members = {entry.name: entry for entry in entries(Path(career).read_bytes())}
    with TemporaryDirectory() as directory:
        user = Path(directory)
        files = user / "Files"
        files.mkdir()
        names = ["eng.lng", "por.lng", "match_stats_matches.tmp"]
        if mode == "--archived-commentary":
            names.extend(["human events1.tmp", "computer events1.tmp"])
        for name in names:
            member = members[name]
            (files / name).write_bytes(decompress(member.payload, member.size))
        subprocess.run([checker, snapshot, mode, directory], check=True)


if __name__ == "__main__":
    main(*sys.argv[1:])
