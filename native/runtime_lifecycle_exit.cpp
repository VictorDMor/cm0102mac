#include "runtime_lifecycle_internal.hpp"

namespace cm::aot::runtime_lifecycle {
void abort_runtime(Abi& a) {
    a.capture_link(); a.put(3, 1); a.save_link(0x10773fb8); a.frame(64, 0x10773fbc);
    a.call(0x1077db20, 0x10773fc4); a.load(4, a.r(2) - 0x5a7c, 0x10773fc4); a.put(0, 1); a.put(3, 1);
    a.store(a.r(4), a.r(0), 0x10773fd0); a.call(0x10773ff0, 0x10773fd8);
    a.pop(64, 0x10773fd8); a.restore_link(); a.finish();
}
void exit_runtime(Abi& a) {
    a.save(31, 0x10773ff0); a.capture_link(); a.load(4, a.r(2) - 0x5a7c, 0x10773ff8); a.save(30, 0x10773ffc);
    a.put(30, a.r(3)); a.load(31, a.r(2) - 0x5a80, 0x10774004); a.save_link(0x10774008); a.frame(64, 0x1077400c);
    a.load(0, a.r(4), 0x10774010); a.compare(a.signed_r(0));
    if (!a.r(0)) {
        a.call(0x10772e10, 0x10774020); a.load(12, a.r(31), 0x10774020); a.compare(a.r(12));
        if (a.r(12)) { a.callback(0x10774030); a.put(0, 0); a.store(a.r(31), a.r(0), 0x10774038); }
    }
    a.put(3, a.r(30)); a.call(0x10774060, 0x10774044);
    a.pop(64, 0x10774044); a.restore(31, 0x1077404c); a.restore_link(); a.restore(30, 0x10774054); a.finish();
}
void terminate_runtime(Abi& a) {
    a.save(31, 0x10774060); a.capture_link(); a.load(31, a.r(2) - 0x3b78, 0x10774068);
    a.save(30, 0x1077406c); a.load(30, a.r(2) - 0x5a84, 0x10774070); a.save_link(0x10774074); a.frame(64, 0x10774078);
    for (;;) {
        a.load(0, a.r(2) - 0x3514, 0x1077409c); a.compare(a.signed_r(0)); if (a.signed_r(0) <= 0) break;
        // The count is intentionally loaded again before decrementing. Retain
        // live reverse order and callback-visible publication before dispatch.
        a.load(3, a.r(2) - 0x3514, 0x10774080); a.put(3, a.r(3) - 1); a.put(0, a.r(3) * 4);
        a.store(a.r(2) - 0x3514, a.r(3), 0x1077408c); a.load(12, a.r(31) + a.r(0), 0x10774090); a.callback(0x10774098);
        a.s.step(0x1077409c);
    }
    a.load(12, a.r(30), 0x107740a8); a.compare(a.r(12));
    if (a.r(12)) { a.callback(0x107740b8); a.put(0, 0); a.store(a.r(30), a.r(0), 0x107740c0); }
    a.native_call(0x10780540, 0x107740c8);
    // Production ExitToShell unwinds through ApplicationExit; the epilogue is
    // retained for a dependency-return characterization fixture only.
    a.pop(64, 0x107740cc); a.restore(31, 0x107740d4); a.restore_link(); a.restore(30, 0x107740dc); a.finish();
}
}
