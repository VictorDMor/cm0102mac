#pragma once

namespace cm::aot {
// ExitToShell cannot return through a nested original callback. Unwind to the
// application host, where normal RAII destruction releases native resources.
// This is a successful exit request, separate from diagnostic exceptions.
struct ApplicationExit final {};
}
