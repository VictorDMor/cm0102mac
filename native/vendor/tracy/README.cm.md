# Tracy client

Unmodified `public/` and `LICENSE` from Tracy v0.15.0, commit
`f2a4f2529e7e71844c9e434b877770a21ecf7587`:
https://github.com/wolfpld/tracy/tree/v0.15.0

`upstream.json` records the upstream Git blob hashes for reproducible updates.
The client is licensed under BSD-3-Clause; bundled components retain their
license notices in the upstream files (including `public/libbacktrace/LICENSE`).
The profiler viewer/server is not bundled. Use a matching 0.15.0 collector.

`../../profiling.cmake` compiles only the documented single translation unit,
`public/TracyClient.cpp`, and only when `CM_ENABLE_TRACY=ON`. The project keeps
Tracy's compile settings private and uses explicit CPU zones/messages only.
