#!/usr/bin/env python3
"""Exercise card editing through the real debugger's bounded process protocol."""
import argparse
from pathlib import Path
import tempfile
from check_match_debugger_worker import Worker, require


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    for name in ("executable", "program", "game"):
        parser.add_argument(name, type=Path)
    args = parser.parse_args()
    with tempfile.TemporaryDirectory(prefix="CMCardsCheck.") as directory:
        worker = Worker(args.executable.resolve(), args.program.resolve(), args.game.resolve(), Path(directory) / "worker")
        try:
            worker.send({"command": "create", "homeID": 677, "awayID": 5677, "seed": 12345})
            initial = worker.state(status="paused")
            card = {"command": "card", "team": 0, "actor": 8, "color": "yellow", "action": "add"}
            worker.rejected(card, initial)
            worker.send({"command": "step"})
            kickoff = worker.state(status="paused", stepCount=1)
            for change in ({"color": "blue"}, {"action": True}, {"actor": 20}, {"team": True}, {"extra": 1}, {"action": "remove"}):
                worker.rejected({**card, **change}, kickoff)
            for yellow in (1, 2):
                worker.send(card)
                state = worker.state(status="paused", cardEditCount=yellow)
                player = state["players"][8]
                require(player["yellowCards"] == yellow and player["redCards"] == yellow - 1 and
                        player["active"] == (yellow == 1), "Yellow or second-yellow engine effects")
            require(state["homeOnPitch"] == 10 and any("second yellow card" in e["description"].lower() for e in state["matchEvents"]),
                    "Second-yellow dismissal is not visible")
            worker.send({**card, "action": "remove"})
            returned = worker.state(status="paused", cardEditCount=3)
            require(returned["players"][8]["yellowCards"] == 1 and returned["players"][8]["redCards"] == 0 and
                    returned["homeOnPitch"] == 11 and returned["clock"] == state["clock"], "Cancel second yellow")
            worker.send({"command": "back"})
            undone = worker.state(cardEditCount=2)
            require(undone["players"] == state["players"] and undone["matchEvents"] == state["matchEvents"], "Undo card removal")
            worker.send({"command": "forward"})
            replay = worker.state(cardEditCount=3)
            require(replay["players"] == returned["players"] and replay["matchEvents"] == returned["matchEvents"], "Redo card removal")
            worker.send({**card, "color": "red"})
            red = worker.state(cardEditCount=4)
            require(red["homeOnPitch"] == 10 and red["players"][8]["redCards"] == 1, "Direct red after yellow")
            worker.send({**card, "color": "red", "action": "remove"})
            restored = worker.state(cardEditCount=5)
            require(restored["homeOnPitch"] == 11 and restored["players"][8]["yellowCards"] == 1, "Remove red retains yellow")
            worker.send({"command": "seek", "seconds": 60})
            later = worker.state(status="paused")
            require(later["matchSeconds"] >= 60 and later["modernMovementCalls"] > 0 and
                    later["originalMovementCalls"] == later["movementFaults"] == later["movementDispatchEscapes"] == 0,
                    "Real following match did not use productive movement")
            worker.send({"command": "seek", "seconds": 0})
            reset = worker.state(cardEditCount=0)
            require(reset["players"] == initial["players"] and reset["clock"] == "00:00", "Time rewind retained cards")
            worker.close()
            print("PASS: real worker card add/remove, second yellow and direct red, visible counts/events, malformed controls, undo/redo/time travel, productive movement and normal cleanup")
        finally:
            if worker.process.poll() is None:
                worker.close(eof=True)


if __name__ == "__main__":
    main()
