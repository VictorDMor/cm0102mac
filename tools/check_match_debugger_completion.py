#!/usr/bin/env python3
"""One reported pairing through actual worker full time and timeline replay."""
import argparse
from pathlib import Path
import tempfile

from check_match_debugger_worker import Worker, require


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("executable", type=Path)
    parser.add_argument("program", type=Path)
    parser.add_argument("game", type=Path)
    args = parser.parse_args()
    with tempfile.TemporaryDirectory(prefix="CMCompletionCheck.") as directory:
        worker = Worker(args.executable.resolve(), args.program.resolve(), args.game.resolve(), Path(directory) / "worker")
        try:
            worker.send({"command": "create", "homeID": 2262, "awayID": 5677, "seed": 12345})
            initial = worker.state(status="paused")
            require(initial["clock"] == "00:00", "Reported pairing did not start before kickoff")
            for _ in range(8):
                worker.send({"command": "seek", "seconds": 10800})
                final = worker.state()
                if final["status"] == "completed":
                    break
                # The original engine returns 4 at halftime and other explicit
                # pauses. A later seek continues from that recorded boundary.
                require(final["status"] == "paused", "Match faulted while seeking full time")
            require(final["status"] == "completed", "Match did not complete within its bounded advances")
            require(not final["failure"] and 5400 <= final["matchSeconds"] < 6000 and
                    final["matchEvents"][-1]["description"] == "Full time" and
                    final["matchEvents"][-1]["seconds"] == final["matchSeconds"],
                    "Actual full time, clock or football event is missing")
            worker.rejected({"command": "step"}, final)
            worker.send({"command": "back"})
            past = worker.state(status="paused")
            require(past["timelineCursor"] == final["timelineCursor"] - 1 and
                    past["matchEvents"][-1]["description"] != "Full time", "Undo retained full time")
            worker.send({"command": "forward"})
            replay = worker.state(status="completed")
            for key in ("homeScore", "awayScore", "players", "clock", "matchEvents", "timelineCursor", "timelineSize"):
                require(replay[key] == final[key], f"Full-time replay changed {key}")
            worker.close()
            print(f"PASS: Cruzeiro/Manchester United seed 12345, {final['stepCount']} real steps, "
                  f"{final['homeScore']}–{final['awayScore']} at {final['clock']}, readable Full time, "
                  "completed-step rejection, undo/redo and normal backing cleanup")
        finally:
            if worker.process.poll() is None:
                worker.close(eof=True)


if __name__ == "__main__":
    main()
