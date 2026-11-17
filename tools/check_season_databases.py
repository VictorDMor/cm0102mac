#!/usr/bin/env python3
"""Exercise season conversion and persistence only in a disposable runtime."""
from pathlib import Path
import subprocess
import sys
import tempfile

from language_compat import install_index_compatibility
from season_databases import install_database_packs


def main():
    binary, program, game = map(Path, sys.argv[1:4])
    with tempfile.TemporaryDirectory(prefix="cm-seasons-") as temporary:
        runtime = Path(temporary)
        install_database_packs(game, runtime)
        install_index_compatibility(game, runtime)
        # The C++ fixture configures the original folder cache for this explicit
        # destination; history checks retain the current-directory fallback.
        if "--history-2002" not in sys.argv[4:]:
            (runtime / "Files/Temp").mkdir()
        subprocess.run([str(binary), str(program), str(game), str(runtime), *sys.argv[4:]], check=True)


if __name__ == "__main__":
    main()
