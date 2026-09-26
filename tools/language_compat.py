"""Align the supplied release's English club names with its database IDs."""
import hashlib
from pathlib import Path
import subprocess
import tempfile

ORIGINAL_ENGLISH = "f305bc9731a06098f24dc860f9156f539d4d965e610f90196e3e2792be1ef194"
ALIGNED_ENGLISH = "d949309b50c822c337546e04b7eb893507aba3cf0bec9aa95d4ef7f9d8f6ffa1"
ORIGINAL_CLUBS = "1fb0859d587afb684667eb614a043b5b73234b36cabec9b803989f5231e32ece"


def sha256(data):
    return hashlib.sha256(data).hexdigest()


def align_english(data):
    """Move one 79-byte record; preserve every name and all other sections.

    The supplied clubs database puts Matav Compaq Sopron at ID 5791. Its
    English language table puts the same club (Matav-Sopron) at slot 8649.
    The original saved-game index builder uses database IDs as language-table
    positions. Only this exact, audited asset is eligible for the correction.
    """
    digest = sha256(data)
    if digest == ALIGNED_ENGLISH:
        return data
    if digest != ORIGINAL_ENGLISH:
        raise ValueError("English language data does not match the audited Mac release")
    first, last = 32 + 5791 * 79, 32 + 8649 * 79
    aligned = data[:first] + data[last:last + 79] + data[first:last] + data[last + 79:]
    if sha256(aligned) != ALIGNED_ENGLISH:
        raise ValueError("Aligned English language data failed verification")
    return aligned


def install_index_compatibility(game, runtime):
    """Install a private read overlay, leaving the verified original intact."""
    clubs = game / "Data/clubs club.dam"
    if sha256(clubs.read_bytes()) != ORIGINAL_CLUBS:
        raise ValueError("Club database does not match the audited language correction")
    source = game / "Data/eng.lng"
    aligned = align_english(source.read_bytes())
    target = runtime / "Files/Data/eng.lng"
    if target.exists():
        current = target.read_bytes()
        if current == aligned:
            return
        if sha256(current) != ORIGINAL_ENGLISH:
            raise ValueError("A custom private English language file was preserved")
    target.parent.mkdir(parents=True, exist_ok=True)
    with tempfile.TemporaryDirectory(prefix=".language-", dir=target.parent) as stage:
        staged = Path(stage) / "eng.lng"
        subprocess.run(["/usr/bin/ditto", "--rsrc", "--extattr", str(source), str(staged)], check=True)
        staged.write_bytes(aligned)
        if staged.read_bytes() != aligned:
            raise ValueError("Private English language file failed verification")
        staged.replace(target)
