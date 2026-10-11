#include "runtime_lifecycle_internal.hpp"

namespace cm::aot::runtime_lifecycle {
void raise_signal(Abi& a) {
    a.save(31, 0x1077db20); a.capture_link(); a.save(30, 0x1077db28); a.put(30, a.r(3)); a.compare(a.signed_r(30), 1);
    a.save_link(0x1077db34); a.frame(64, 0x1077db38);
    bool valid = a.signed_r(30) >= 1;
    if (valid) { a.compare(a.signed_r(30), 6); valid = a.signed_r(30) <= 6; }
    if (!valid) a.put(3, UINT32_MAX);
    else {
        a.load(3, a.r(2) - 0x3b3c, 0x1077db50); a.put(0, a.r(30) * 4); a.put(3, a.r(3) + a.r(0));
        const auto slot = a.r(3) - 4; a.load(31, slot, 0x1077db5c); a.put(3, slot); a.compare(a.r(31), 1);
        if (a.r(31) != 1) { a.put(0, 0); a.store(a.r(3), a.r(0), 0x1077db6c); }
        a.compare(a.r(31), 1); bool ignored = a.r(31) == 1;
        if (!ignored) { a.compare(a.r(31)); if (!a.r(31)) { a.compare(a.signed_r(30), 1); ignored = a.r(30) == 1; } }
        if (ignored) a.put(3, 0);
        else {
            a.compare(a.r(31));
            if (!a.r(31)) { a.put(3, 0); a.call(0x10773ff0, 0x1077dba0); }
            a.put(3, a.r(30)); a.put(12, a.r(31)); a.callback(0x1077dbac); a.put(3, 0);
        }
    }
    a.pop(64, 0x1077dbb4); a.restore(31, 0x1077dbbc); a.restore_link(); a.restore(30, 0x1077dbc4); a.finish();
}
}
