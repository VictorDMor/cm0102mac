#!/usr/bin/env python3
"""Generate the competition editor's bases: every competition's original values, per base season.

A competition's values are written by code when a career is set up, so the only way to list them is to let the
game set a career up. This drives an isolated copy (tools/game_driver.py) through New Game with every nation and
extra division selected, a few nations at a time, and merges what the game's recorder
(native/competition_defaults.hpp) wrote into .local/competition-bases/<edition>.toml. tools/dev.sh install then
copies those files next to the game data, where the editor reads them.

Nations go in batches because a New Game with all of them on the 2002/03 database stops at the original's
"no more people" message (database::refuse_more_people) and never sets the competitions up.

    tools/competition_bases.py                 every base season (about an hour; the game's windows open)
    tools/competition_bases.py --season 2002   one of 2000, 2001, 2002
    tools/competition_bases.py --season 2002 --no-extra-divisions

The files hold game-derived data: they stay in .local, never in Git.
"""
import argparse
import json
import subprocess
import sys
import time
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
RUN_NAME = "competition-bases"
RUN = ROOT / ".local/performance" / RUN_NAME
RECORDED = RUN / "Runtime/Competition Defaults"
OUT = ROOT / ".local/competition-bases"
EDITIONS = {2000: "cm0001_389", 2001: "cm0102", 2002: "si_3968"}
# The league selection's actions (control +0xa) and the handlers of the screens on the way.
SELECT_NONE, SELECTED, EXTRA_DIVISION, START_SEASON, NEXT = 11, 13, 56, 15, -3
PREVIOUS_SEASON, NEXT_SEASON = 0x7000, 0x7001
START_HANDLER, LEAGUES_HANDLER, SEASON_HANDLER = "0x10334940", "0x103366f0", "0x10337620"
BATCH = 7


def driver(*arguments):
    return subprocess.run([sys.executable, str(ROOT / "tools/game_driver.py"), *arguments],
                          capture_output=True, text=True)


def status():
    return json.loads((RUN / "Automation/status.json").read_text())


def send(request, required=True):
    result = driver("send", RUN_NAME, request, "--timeout", "900")
    refused = result.returncode or "refused" in result.stdout
    if refused and required:
        sys.exit(f"{request}: {result.stdout}{result.stderr}")
    return not refused


def controls(action):
    return [index for index, found, kind in status()["controls"] if found == action and kind == 2]


def wait_for(handler, seconds, what, present=True):
    """Until the game waits for input on (or, with present false, off) a screen. The status file is not
    refreshed while the game works, so only a fresh one counts."""
    deadline = time.monotonic() + seconds
    while time.monotonic() < deadline:
        time.sleep(3)
        try:
            current = status()
        except (FileNotFoundError, json.JSONDecodeError):
            continue
        fresh = time.time() - current["time"] / 1000 < 30
        if fresh and current["ready"] and (current["handler"] == handler) == present:
            return True
    print(f"  timed out waiting for {what}", flush=True)
    return False


def batch(season, first, last, extra_divisions):
    """One New Game with nations [first, last) of the list; returns the recorded text, or None."""
    driver("stop", RUN_NAME)
    time.sleep(2)
    for old in RECORDED.glob("*.toml"):
        old.unlink()
    # The copy starts from the installed game's files: a custom database chosen there would change what is
    # set up, and its competitions would not be recorded.
    (RUN / "Runtime/Custom Databases/selected.txt").unlink(missing_ok=True)
    if driver("launch", RUN_NAME).returncode:
        sys.exit("The isolated game did not start")
    time.sleep(5)
    for _ in range(8):  # the first click after launch is sometimes lost
        send(f"control {controls(1)[0]}")
        time.sleep(2)
        if status()["handler"] == LEAGUES_HANDLER:
            break
    else:
        sys.exit("New Game did not open the league selection")
    for _ in range(abs(season - 2001)):
        send(f"control {controls(PREVIOUS_SEASON if season < 2001 else NEXT_SEASON)[0]}")
        time.sleep(3)
    if controls(SELECT_NONE):
        send(f"control {controls(SELECT_NONE)[0]}")
    rows = controls(SELECTED)
    if first >= len(rows):
        driver("stop", RUN_NAME)
        return ""
    for index in rows[first:last]:
        send(f"control {index}")
    if extra_divisions:
        for index in controls(EXTRA_DIVISION):
            send(f"control {index}")
    send(f"control {controls(NEXT)[-1]}")
    if not wait_for(SEASON_HANDLER, 600, "the starting season screen"):
        return None
    for _ in range(6):  # a click right after a screen appears is sometimes lost: until the game is busy
        # Either click may find the game already busy with the setup the other one started.
        if choices := controls(START_SEASON):
            send(f"control {choices[0]}", required=False)
        if following := controls(NEXT):
            send(f"control {following[-1]}", required=False)
        time.sleep(15)
        current = status()
        if current["handler"] != SEASON_HANDLER or time.time() - current["time"] / 1000 > 10:
            break
    else:
        print("  the career's setup did not start", flush=True)
        return None
    # The competitions are set up while the status stays on this screen; the next screen ends it.
    done = wait_for(SEASON_HANDLER, 1800, "the career to be set up", present=False)
    text = "".join(path.read_text() for path in sorted(RECORDED.glob("*.toml"))) if done else None
    driver("stop", RUN_NAME)
    return text


def generate(season, extra_divisions):
    edition = EDITIONS[season]
    parts = []
    for first in range(0, 64, BATCH):
        print(f"{season}: nations {first}..{first + BATCH - 1}", flush=True)
        text = batch(season, first, first + BATCH, extra_divisions)
        if text is None:
            sys.exit(f"{season}: the batch did not finish; nothing written for this season")
        if text == "":
            break
        parts.append(text)
    OUT.mkdir(parents=True, exist_ok=True)
    temporary = OUT / f"{edition}.toml.writing"
    # The editor keeps the first record of each competition, so the batches are simply joined.
    temporary.write_text("".join(parts))
    temporary.rename(OUT / f"{edition}.toml")
    count = "".join(parts).count("# Competition values")
    print(f"{season}: wrote {OUT / (edition + '.toml')} ({count} records before duplicates)", flush=True)


def main():
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("--season", type=int, choices=sorted(EDITIONS), action="append")
    parser.add_argument("--no-extra-divisions", action="store_true",
                        help="leave the optional lower divisions out (their competitions are then missing)")
    arguments = parser.parse_args()
    if not (RUN / "manifest.json").is_file() and driver("prepare", RUN_NAME).returncode:
        sys.exit("Cannot prepare the isolated copy (tools/game_driver.py prepare)")
    for season in arguments.season or sorted(EDITIONS):
        generate(season, not arguments.no_extra_divisions)
    driver("stop", RUN_NAME)


if __name__ == "__main__":
    main()
