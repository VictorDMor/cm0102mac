#include "code_fragment_internal.hpp"

namespace cm::aot::code_fragment {
void find_fragment(Abi& a) {
    a.load(4, a.r(2) - 0x3528, 0x107712b0); a.compare(a.r(4));
    if (a.r(4)) {
        a.load(0, a.r(4) + 8, 0x107712bc); a.compare(a.r(3), a.r(0));
        if (a.r(3) >= a.r(0)) {
            a.load(0, a.r(4) + 12, 0x107712c8); a.compare(a.r(3), a.r(0));
            if (a.r(3) < a.r(0)) { a.put(3, a.r(4)); a.finish(); return; }
        }
    }
    a.load(4, a.r(2) - 0x3524, 0x107712dc); a.compare(a.r(4));
    while (a.r(4)) {
        a.put(0, 8); a.put(5, a.r(4)); a.put(6, 0); a.s.wr<4>(4132, a.r(0));
        do {
            for (unsigned lane = 0; lane < 4; ++lane) {
                const auto start_pc = lane ? 0x10771328 + (lane - 1) * 0x30 : 0x107712f8;
                a.load(0, a.r(5) + (lane ? 0x24 : 8), start_pc);
                if (lane) a.put(5, a.r(5) + 28);
                a.compare(a.r(0));
                if (a.r(0)) {
                    a.compare(a.r(3), a.r(0));
                    if (a.r(3) >= a.r(0)) {
                        const auto end_pc = lane ? 0x10771340 + (lane - 1) * 0x30 : 0x1077130c;
                        a.load(0, a.r(5) + 12, end_pc); a.compare(a.r(3), a.r(0));
                        if (a.r(3) < a.r(0)) {
                            const auto store_pc = lane ? 0x1077134c + (lane - 1) * 0x30 : 0x10771318;
                            a.store(a.r(2) - 0x3528, a.r(5), store_pc); a.put(3, a.r(5)); a.finish(); return;
                        }
                    }
                }
            }
            // Retain the unrolled original's observable scratch increment.
            a.put(6, a.r(6) + 3); a.put(5, a.r(5) + 28); a.s.step(0x107712f8);
        } while (a.count_down());
        a.load(4, a.r(4) + 0x380, 0x107713c4); a.compare(a.r(4));
    }
    a.put(3, 0); a.finish();
}
}
