# Yoga layout core

- Upstream: https://github.com/react/yoga
- Version: v3.2.2
- Commit: 8e31207cf5d5e6bd9ec06b55a5a5b2d492dde2d9
- License: MIT (see LICENSE).

Only the unmodified native `.h` and `.cpp` files from upstream `yoga/` are
vendored. JavaScript bindings, tests and upstream build flags are excluded.
The project builds the core as an isolated static library and uses its public
C API through the form layout adapter. Update this revision and rerun layout
and original-control checks when changing the dependency.
