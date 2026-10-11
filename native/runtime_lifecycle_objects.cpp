#include "runtime_lifecycle_internal.hpp"

namespace cm::aot::runtime_lifecycle {
void register_destructor(Abi& a) {
    a.load(6, a.r(2) - 0x5ab0, 0x10772df0); a.load(0, a.r(6), 0x10772df4);
    a.store(a.r(5), a.r(0), 0x10772df8); a.store(a.r(5) + 4, a.r(4), 0x10772dfc);
    a.store(a.r(5) + 8, a.r(3), 0x10772e00); a.store(a.r(6), a.r(5), 0x10772e04); a.finish();
}
void drain_destructors(Abi& a) {
    a.save(31, 0x10772e10); a.capture_link(); a.load(31, a.r(2) - 0x5ab0, 0x10772e18);
    a.save_link(0x10772e1c); a.frame(64, 0x10772e20);
    for (;;) {
        a.load(5, a.r(31), 0x10772e44); a.compare(a.r(5)); if (!a.r(5)) break;
        // Unlink before invoking the destructor, then reread the live head.
        // A destructor can register another object in this guest chain.
        a.load(0, a.r(5), 0x10772e28); a.put(4, UINT32_MAX); a.store(a.r(31), a.r(0), 0x10772e30);
        a.load(3, a.r(5) + 8, 0x10772e34); a.load(12, a.r(5) + 4, 0x10772e38); a.callback(0x10772e40);
        a.s.step(0x10772e44);
    }
    a.pop(64, 0x10772e50); a.restore(31, 0x10772e58); a.restore_link(); a.finish();
}
}
