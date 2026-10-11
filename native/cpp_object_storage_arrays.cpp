#include "cpp_object_storage_internal.hpp"

namespace cm::aot::cpp_object_storage {
void construct_array(Abi& a) {
    a.words(27, 0x10773150); a.capture_link(); a.put(29, a.r(3)); a.record(a.r(29));
    a.put(27, a.r(4)); a.put(28, a.r(6)); a.put(31, a.r(7));
    a.save_link(0x10773168); a.frame(112, 0x1077316c);
    if (!a.r(29)) goto complete;
    a.store(a.r(29), a.r(28), 0x10773174); a.compare(a.r(27));
    a.store(a.r(29) + 4, a.r(31), 0x1077317c); a.put(29, a.r(29) + 16);
    if (!a.r(27)) goto complete;
    a.store(a.r(1) + 72, a.r(31), 0x10773188); a.put(0, 0); a.put(30, a.r(29));
    a.store(a.r(1) + 56, a.r(29), 0x10773194); a.store(a.r(1) + 60, a.r(28), 0x10773198);
    a.store(a.r(1) + 64, a.r(31), 0x1077319c); a.store(a.r(1) + 68, a.r(5), 0x107731a0);
    a.store(a.r(1) + 72, a.r(0), 0x107731a4);
    for (;;) {
        a.load(4, a.r(1) + 72, 0x107731d4); a.compare(a.r(4), a.r(31));
        if (a.r(4) >= a.r(31)) break;
        a.put(3, a.r(30)); a.put(12, a.r(27)); a.put(4, 1); a.callback(0x107731c0);
        a.load(3, a.r(1) + 72, 0x107731c4); a.put(30, a.r(30) + a.r(28)); a.put(0, a.r(3) + 1);
        a.store(a.r(1) + 72, a.r(0), 0x107731d0); a.s.step(0x107731d4);
    }
    a.load(0, a.r(1) + 64, 0x107731e0); a.compare(a.r(4), a.r(0));
    if (a.r(4) >= a.r(0)) goto complete;
    a.load(0, a.r(1) + 68, 0x107731ec); a.compare(a.r(0)); if (!a.r(0)) goto complete;
    a.load(31, a.r(1) + 60, 0x107731f8); a.load(3, a.r(1) + 56, 0x107731fc);
    a.put(0, a.r(31) * a.r(4)); a.put(30, a.r(3) + a.r(0));
    for (;;) {
        a.load(0, a.r(1) + 72, 0x10773234); a.compare(a.r(0)); if (!a.r(0)) break;
        a.put(30, a.r(30) - a.r(31)); a.load(12, a.r(1) + 68, 0x10773214);
        a.put(3, a.r(30)); a.put(4, UINT32_MAX); a.callback(0x10773224);
        a.load(3, a.r(1) + 72, 0x10773228); a.put(0, a.r(3) - 1);
        a.store(a.r(1) + 72, a.r(0), 0x10773230); a.s.step(0x10773234);
    }
complete:
    a.put(3, a.r(29)); a.pop(112, 0x10773244); a.restore_link(); a.words(27, 0x10773250, true); a.finish();
}
void cleanup_partial_array(Abi& a) {
    a.save(31, 0x10773260); a.capture_link(); a.save(30, 0x10773268); a.put(30, a.r(4));
    a.save(29, 0x10773270); a.put(29, a.r(3)); a.record(a.r(29));
    a.save_link(0x10773278); a.frame(80, 0x1077327c); if (!a.r(29)) goto complete;
    a.load(4, a.r(29) + 16, 0x10773284); a.load(0, a.r(29) + 8, 0x10773288); a.compare(a.r(4), a.r(0));
    if (a.r(4) < a.r(0)) {
        a.load(0, a.r(29) + 12, 0x10773294); a.compare(a.r(0));
        if (a.r(0)) {
            a.load(0, a.r(29) + 4, 0x107732a0); a.load(3, a.r(29), 0x107732a4);
            a.put(0, a.r(0) * a.r(4)); a.put(31, a.r(3) + a.r(0));
            for (;;) {
                a.load(0, a.r(29) + 16, 0x107732e0); a.compare(a.r(0)); if (!a.r(0)) break;
                a.load(0, a.r(29) + 4, 0x107732b8); a.put(4, UINT32_MAX);
                a.load(12, a.r(29) + 12, 0x107732c0); a.put(31, a.r(31) - a.r(0)); a.put(3, a.r(31));
                a.callback(0x107732d0); a.load(3, a.r(29) + 16, 0x107732d4); a.put(0, a.r(3) - 1);
                a.store(a.r(29) + 16, a.r(0), 0x107732dc); a.s.step(0x107732e0);
            }
        }
    }
    a.signed_word_record(0, a.r(30));
    if (std::bit_cast<std::int32_t>(a.r(0)) > 0) { a.put(3, a.r(29)); a.call(0x107735a0, 0x107732fc); }
complete:
    a.put(3, a.r(29)); a.pop(80, 0x10773300); a.restore_link();
    a.restore(31, 0x1077330c); a.restore(30, 0x10773310); a.restore(29, 0x10773314); a.finish();
}
void construct_range(Abi& a) {
    a.save(31, 0x10773320); a.capture_link(); a.put(31, a.r(7)); a.save(30, 0x1077332c); a.put(30, a.r(3));
    a.save(29, 0x10773334); a.put(29, a.r(6)); a.save(28, 0x1077333c); a.put(28, a.r(4));
    a.save_link(0x10773344); a.put(0, 0); a.frame(96, 0x1077334c);
    a.store(a.r(1) + 72, a.r(31), 0x10773350); a.store(a.r(1) + 68, a.r(5), 0x10773354);
    a.store(a.r(1) + 56, a.r(3), 0x10773358); a.store(a.r(1) + 60, a.r(29), 0x1077335c);
    a.store(a.r(1) + 64, a.r(31), 0x10773360); a.store(a.r(1) + 72, a.r(0), 0x10773364);
    for (;;) {
        a.load(4, a.r(1) + 72, 0x10773394); a.compare(a.r(4), a.r(31)); if (a.r(4) >= a.r(31)) break;
        a.put(3, a.r(30)); a.put(12, a.r(28)); a.put(4, 1); a.callback(0x10773380);
        a.load(3, a.r(1) + 72, 0x10773384); a.put(30, a.r(30) + a.r(29)); a.put(0, a.r(3) + 1);
        a.store(a.r(1) + 72, a.r(0), 0x10773390); a.s.step(0x10773394);
    }
    a.load(0, a.r(1) + 64, 0x107733a0); a.compare(a.r(4), a.r(0)); if (a.r(4) >= a.r(0)) goto complete;
    a.load(0, a.r(1) + 68, 0x107733ac); a.compare(a.r(0)); if (!a.r(0)) goto complete;
    a.load(31, a.r(1) + 60, 0x107733b8); a.load(3, a.r(1) + 56, 0x107733bc);
    a.put(0, a.r(31) * a.r(4)); a.put(30, a.r(3) + a.r(0));
    for (;;) {
        a.load(0, a.r(1) + 72, 0x107733f4); a.compare(a.r(0)); if (!a.r(0)) break;
        a.put(30, a.r(30) - a.r(31)); a.load(12, a.r(1) + 68, 0x107733d4);
        a.put(3, a.r(30)); a.put(4, UINT32_MAX); a.callback(0x107733e4);
        a.load(3, a.r(1) + 72, 0x107733e8); a.put(0, a.r(3) - 1);
        a.store(a.r(1) + 72, a.r(0), 0x107733f0); a.s.step(0x107733f4);
    }
complete:
    a.pop(96, 0x10773400); a.restore(31, 0x10773408); a.restore(30, 0x1077340c);
    a.restore_link(); a.restore(29, 0x10773414); a.restore(28, 0x10773418); a.finish();
}
void destroy_range(Abi& a) {
    a.save(31, 0x10773420); a.capture_link(); a.save(30, 0x10773428); a.put(30, a.r(6));
    a.save(29, 0x10773430); a.put(29, a.r(5)); a.save(28, 0x10773438); a.put(28, a.r(4));
    a.save_link(0x10773440); a.put(0, a.r(29) * a.r(30)); a.frame(80, 0x10773448); a.put(31, a.r(3) + a.r(0));
    for (;;) {
        a.compare(a.r(30)); if (!a.r(30)) break;
        a.put(31, a.r(31) - a.r(29)); a.put(3, a.r(31)); a.put(12, a.r(28)); a.put(4, UINT32_MAX);
        a.callback(0x1077346c); a.put(30, a.r(30) - 1); a.s.step(0x10773474);
    }
    a.pop(80, 0x1077347c); a.restore(31, 0x10773484); a.restore(30, 0x10773488);
    a.restore_link(); a.restore(29, 0x10773490); a.restore(28, 0x10773494); a.finish();
}
void destroy_array(Abi& a) {
    a.words(26, 0x107734a0); a.capture_link(); a.put(26, a.r(3)); a.record(a.r(26)); a.put(27, a.r(4));
    a.save_link(0x107734b0); a.frame(80, 0x107734b4); if (!a.r(26)) goto complete;
    a.compare(a.r(27));
    if (a.r(27)) {
        a.load(29, a.r(26) - 16, 0x107734c4); a.put(31, 0); a.load(30, a.r(26) - 12, 0x107734cc);
        a.put(0, a.r(29) * a.r(30)); a.put(28, a.r(26) + a.r(0));
        for (;;) {
            a.compare(a.r(31), a.r(30)); if (a.r(31) >= a.r(30)) break;
            a.put(28, a.r(28) - a.r(29)); a.put(3, a.r(28)); a.put(12, a.r(27)); a.put(4, UINT32_MAX);
            a.callback(0x107734f4); a.put(31, a.r(31) + 1); a.s.step(0x107734fc);
        }
    }
    a.put(3, a.r(26) - 16); a.call(0x10773610, 0x1077350c);
complete:
    a.pop(80, 0x1077350c); a.words(26, 0x10773514, true); a.restore_link(); a.finish();
}
}
