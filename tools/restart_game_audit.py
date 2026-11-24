"""Restart Game: the two calls of the Mac original's empty restart routine (0x1076f880) in the menu bar's
handler, after the player confirmed. The host takes them (native/restart_game.cpp)."""
RESTART_CALLS = {0x102a67b0: "bl 0x1076f880", 0x102a6a74: "bl 0x1076f880"}
