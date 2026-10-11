#include "code_fragment_internal.hpp"

namespace cm::aot::code_fragment {
namespace {
void publish_record(Abi& a, unsigned record, std::uint32_t first) {
    a.store(a.r(record) + 8, a.r(25), first);
    a.put(3, a.r(23));
    a.store(a.r(record) + 12, a.r(26), first + 8);
    a.store(a.r(record) + 16, a.r(27), first + 12);
    a.store(a.r(record) + 20, a.r(28), first + 16);
    a.store(a.r(record), a.r(29), first + 20);
    a.store(a.r(record) + 4, a.r(30), first + 24);
    a.store(a.r(record) + 24, a.r(31), first + 28);
}
}
void register_fragment(Abi& a) {
    a.capture_link(); a.words(23, 0x10771154);
    for (unsigned n = 25; n < 32; ++n) a.put(n, a.r(n - 22));
    a.save_link(0x10771174); a.frame(112, 0x10771178);
    a.load<1>(0, a.r(2) - 0x3529, 0x1077117c); a.compare(a.r(0));
    if (!a.r(0)) {
        a.load(0, a.r(2) - 0x7e3c, 0x10771188); a.put(3, 1);
        a.store<1>(a.r(2) - 0x3529, a.r(3), 0x10771190); a.compare(a.r(0));
        if (a.r(0)) {
            a.put(3, 0x70706366); a.put(4, a.r(1) + 56); a.native_call(0x1077fad8, 0x107711ac);
            a.signed_word_record(0, a.r(3));
            if (!a.r(0)) {
                a.load(0, a.r(1) + 56, 0x107711b8); a.put(0, a.r(0) & 0x10); a.record(a.r(0));
                if (a.r(0)) { a.put(0, 1); a.store<1>(a.r(2) - 0x352a, a.r(0), 0x107711c8); }
            }
        }
    }
    a.load(4, a.r(2) - 0x3524, 0x107711cc); a.put(23, 0); a.put(24, 0); a.put(0, 32);
    a.compare(a.r(4));
    while (a.r(4)) {
        a.put(5, a.r(4)); a.s.wr<4>(4132, a.r(0));
        do {
            a.load(3, a.r(5) + 8, 0x107711e8); a.compare(a.r(3));
            if (!a.r(3)) { publish_record(a, 5, 0x107711f4); goto complete; }
            a.put(5, a.r(5) + 28); a.put(23, a.r(23) + 1); a.s.step(0x107711e8);
        } while (a.count_down());
        a.put(24, a.r(4)); a.load(4, a.r(4) + 0x380, 0x10771228); a.compare(a.r(4));
    }
    a.load(0, a.r(2) - 0x7d74, 0x10771234); a.compare(a.r(0));
    if (!a.r(0)) a.put(3, UINT32_MAX);
    else {
        a.put(3, 0x384); a.native_call(0x1077faf0, 0x10771248); a.compare(a.r(3));
        if (!a.r(3)) a.put(3, UINT32_MAX);
        else {
            a.compare(a.r(24));
            if (!a.r(24)) a.store(a.r(2) - 0x3524, a.r(3), 0x1077125c);
            else a.store(a.r(24) + 0x380, a.r(3), 0x10771264);
            // New blocks publish fields in a different order from reused slots.
            a.store(a.r(3) + 8, a.r(25), 0x10771268); a.store(a.r(3) + 12, a.r(26), 0x1077126c);
            a.store(a.r(3) + 16, a.r(27), 0x10771270); a.store(a.r(3) + 20, a.r(28), 0x10771274);
            a.store(a.r(3), a.r(29), 0x10771278); a.store(a.r(3) + 4, a.r(30), 0x1077127c);
            a.store(a.r(3) + 24, a.r(31), 0x10771280); a.put(3, a.r(23));
        }
    }
complete:
    a.pop(112, 0x10771290); a.restore_link(); a.words(23, 0x1077129c, true); a.finish();
}
}
