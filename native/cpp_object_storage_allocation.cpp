#include "cpp_object_storage_internal.hpp"

namespace cm::aot::cpp_object_storage {
void allocate_object(Abi& a) {
    a.save(31, 0x10773520); a.capture_link(); a.load(31, a.r(2) - 0x5aac, 0x10773528);
    a.save(30, 0x1077352c); a.save(29, 0x10773530); a.put(29, a.r(3)); a.record(a.r(29));
    a.save_link(0x10773538); a.frame(80, 0x1077353c); if (!a.r(29)) a.put(29, 4);
    for (;;) {
        a.put(3, a.r(29)); a.call(0x10775130, 0x10773550); a.put(30, a.r(3)); a.record(a.r(30));
        if (a.r(30)) break;
        a.load(12, a.r(31), 0x10773558); a.compare(a.r(12));
        if (!a.r(12)) { a.call(0x10773630, 0x10773568); break; }
        a.callback(0x10773574); a.s.step(0x10773548);
    }
    a.put(3, a.r(30)); a.pop(80, 0x10773580); a.restore_link();
    a.restore(31, 0x1077358c); a.restore(30, 0x10773590); a.restore(29, 0x10773594); a.finish();
}
void release_object(Abi& a) {
    a.save(31, 0x107735a0); a.capture_link(); a.compare(a.r(3)); a.save_link(0x107735ac);
    a.frame(96, 0x107735b0); a.put(31, a.r(1)); if (a.r(3)) a.call(0x10775170, 0x107735c0);
    a.load(0, a.r(31) + 104, 0x107735d0); a.load(1, a.r(1), 0x107735d4);
    a.restore_link(); a.restore(31, 0x107735dc); a.finish();
}
void allocation_facade(Abi& a, bool releasing) {
    const auto entry = releasing ? 0x10773610u : 0x107735f0u;
    a.capture_link(); a.save_link(entry + 4); a.frame(64, entry + 8);
    a.call(releasing ? 0x107735a0 : 0x10773520, entry + 16);
    a.pop(64, entry + 16); a.restore_link(); a.finish();
}
void allocation_failure(Abi& a) {
    a.capture_link(); a.load(3, a.r(2) - 0x5aa0, 0x10773634);
    a.save_link(0x10773638); a.frame(64, 0x1077363c); a.load<1>(0, a.r(3), 0x10773640);
    a.signed_byte_record(0, a.r(0));
    if (a.r(0)) {
        a.load(3, a.r(2) - 0x5ab4, 0x1077364c); a.put(4, a.r(1) + 56);
        a.load(0, a.r(2) - 0x5aa4, 0x10773654); a.store(a.r(1) + 56, a.r(3), 0x10773658);
        a.load(3, a.r(2) - 0x3b80, 0x1077365c); a.store(a.r(1) + 56, a.r(0), 0x10773660);
        a.load(5, a.r(2) - 0x5aa8, 0x10773664); a.call(0x10772ad0, 0x1077366c);
    }
    a.pop(64, 0x1077366c); a.restore_link(); a.finish();
}
void destroy_failure(Abi& a) {
    a.save(31, 0x10773680); a.capture_link(); a.put(31, a.r(3)); a.record(a.r(31));
    a.save_link(0x1077368c); a.frame(64, 0x10773690);
    if (a.r(31)) {
        a.load(0, a.r(2) - 0x5aa4, 0x10773698); a.store(a.r(31), a.r(0), 0x1077369c);
        a.load(0, a.r(2) - 0x5ab4, 0x107736a4); a.store(a.r(31), a.r(0), 0x107736a8);
        a.signed_word_record(0, a.r(4));
        if (std::bit_cast<std::int32_t>(a.r(0)) > 0) { a.put(3, a.r(31)); a.call(0x107735a0, 0x107736bc); }
    }
    a.put(3, a.r(31)); a.pop(64, 0x107736bc); a.restore_link(); a.restore(31, 0x107736cc); a.finish();
}
void failure_type(Abi& a) { a.load(3, a.r(2) - 0x3b84, 0x107736e0); a.finish(); }
}
