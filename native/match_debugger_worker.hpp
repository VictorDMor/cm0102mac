#pragma once

// The same executable runs either the AppKit interface or one private engine
// worker. Only the worker owns a guest State; the UI communicates over pipes.
int run_match_debugger_worker(int argc, char** argv);
