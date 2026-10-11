#include "exception_runtime_context.hpp"
#include "mac_abi.hpp"

namespace cm::aot::exception_runtime {
namespace {
struct SearchAbi : MacAbi {
    std::uint32_t guest_return;
    explicit SearchAbi(State& state) : MacAbi(state, state.exception_runtime_dispatch_escapes), guest_return(std::uint32_t(state.r<4>(4128))) {}
    std::int32_t signed_r(unsigned n) const { return std::bit_cast<std::int32_t>(r(n)); }
    void signed_half(unsigned n) { put(n, std::uint32_t(std::int32_t(std::int16_t(r(n))))); }
    void signed_byte(unsigned n) { put(n, std::uint32_t(std::int32_t(std::int8_t(r(n))))); }
    void child(std::uint32_t entry, std::uint32_t link) {
        s.wr<4>(2440, r(2)); s.wr<4>(4128, link); s.pc = entry; invoke_exception_runtime(s, entry);
    }
    void copy_search_cursor(std::uint32_t pc) {
        load(8, r(1) + 0x3c, pc); put(3, r(1) + 0x54);
        for (unsigned n = 0; n < 5; ++n) load(n == 4 ? 0 : 7 - n, r(1) + 0x40 + n * 4, pc + 8 + n * 4);
        store(r(1) + 0x54, r(8), pc + 28);
        for (unsigned n = 0; n < 5; ++n) store(r(1) + 0x58 + n * 4, r(n == 4 ? 0 : 7 - n), pc + 32 + n * 4);
        load(0, r(30) + 0x39c, pc + 52); store(r(1) + 0x6c, r(0), pc + 56);
        load(0, r(30) + 0x3a0, pc + 60); store(r(1) + 0x70, r(0), pc + 64);
        load(0, r(30) + 0x394, pc + 68); store(r(1) + 0x74, r(0), pc + 72);
        call(0x10771620, pc + 80);
    }
    void advance(std::uint32_t link) {
        put(3, r(1) + 0x54); child(0x10771640, link);
    }
    void rethrow() {
        copy_search_cursor(0x10772874);
        for (;;) {
            put(0, r(3) & 0xff); compare(signed_r(0), 13);
            if (r(0) == 13) break;
            bool next = false;
            if (signed_r(0) < 13) {
                compare(signed_r(0), 1);
                if (r(0) != 1) {
                    if (signed_r(0) >= 1) next = true;
                    else { compare(signed_r(0), 0); next = signed_r(0) >= 0; }
                }
            } else { compare(signed_r(0), 15); next = r(0) == 15; }
            if (!next) { call(0x10772eb0, 0x107728f8); break; }
            advance(0x10772908); s.step(0x107728c4);
        }
        load(3, r(1) + 0x5c, 0x1077290c); put(0, 0); load(4, r(1) + 0x70, 0x10772914);
        load<2>(3, r(3) + 2, 0x10772918); signed_half(3); put(4, r(4) + r(3));
        load(3, r(4) + 4, 0x10772920); store(r(30) + 0x3ac, r(3), 0x10772924);
        load(3, r(4), 0x10772928); store(r(30) + 0x3b0, r(3), 0x1077292c);
        store(r(30) + 0x3b4, r(0), 0x10772930); store(r(30) + 0x3b8, r(4), 0x10772934);
    }
    void publish_handler() {
        load(4, r(30) + 0x3a0, 0x10772a2c); load<2>(3, r(31) + 8, 0x10772a30); signed_half(3);
        load(0, r(30) + 0x3b0, 0x10772a34); put(4, r(4) + r(3)); store(r(4), r(0), 0x10772a3c);
        load(0, r(30) + 0x3ac, 0x10772a40); store(r(4) + 4, r(0), 0x10772a44);
        load(0, r(30) + 0x3b4, 0x10772a48); store(r(4) + 8, r(0), 0x10772a4c);
        load(3, r(30) + 0x3ac, 0x10772a50); load<1>(0, r(3), 0x10772a54); signed_byte(0); compare(signed_r(0), '*');
        if (r(0) == '*') {
            put(0, r(4) + 0x10); store(r(4) + 0xc, r(0), 0x10772a68);
            load(3, r(30) + 0x3b0, 0x10772a6c); load(0, r(1) + 0x38, 0x10772a70);
            load(3, r(3), 0x10772a74); put(0, r(3) + r(0)); store(r(4) + 0x10, r(0), 0x10772a7c);
        } else {
            load(3, r(30) + 0x3b0, 0x10772a84); load(0, r(1) + 0x38, 0x10772a88);
            put(0, r(3) + r(0)); store(r(4) + 0xc, r(0), 0x10772a90);
        }
    }
};
}
void install_catch(State& state) {
    SearchAbi a(state);
    a.capture_link(); a.save(31, 0x10772784); a.save(30, 0x10772788); a.save(29, 0x1077278c);
    a.save_link(0x10772790); a.frame(80, 0x10772794); a.put(29, a.r(3)); a.put(30, a.r(4)); a.put(31, a.r(5));
    a.child(0x10771b30, 0x107727a8);
    a.load(5, a.r(29) + 0x3a0, 0x107727a8); a.put(3, a.r(29)); a.load(4, a.r(31) + 8, 0x107727b0);
    a.load(0, a.r(29) + 0x3b0, 0x107727b4); a.put(4, a.r(5) + a.r(4)); a.store(a.r(4), a.r(0), 0x107727bc);
    a.load(0, a.r(29) + 0x3ac, 0x107727c0); a.store(a.r(4) + 4, a.r(0), 0x107727c4);
    a.load(0, a.r(29) + 0x3b4, 0x107727c8); a.store(a.r(4) + 8, a.r(0), 0x107727cc);
    a.store(a.r(4) + 0x14, a.r(31), 0x107727d0);
    a.load(5, a.r(30) + 4, 0x107727d4); record_context_function(state, a.r(5)); a.load(0, a.r(31) + 4, 0x107727d8);
    a.load(4, a.r(30) + 0x14, 0x107727dc); a.load<1>(6, a.r(2) - 0x352a, 0x107727e0);
    a.put(5, a.r(5) + a.r(0)); a.child(0x10772650, 0x107727ec);
    // Most restorations transfer directly to a handler. A deliberately selected
    // original link destination instead executes the original returning tail.
    if (state.pc != 0x107727ec) return;
    a.pop(80, 0x107727ec); a.restore_link(); a.restore(31, 0x107727f8);
    a.restore(30, 0x107727fc); a.restore(29, 0x10772800); a.finish();
}
void search_throw(State& state) {
    SearchAbi a(state);
    a.capture_link(); a.save(31, 0x10772814); a.save(30, 0x10772818); a.save_link(0x1077281c);
    a.frame(128, 0x10772820); a.put(30, a.r(3)); a.load(3, a.r(30) + 0x3a8, 0x10772828);
    a.put(4, a.r(1) + 0x3c); a.call(0x107713e0, 0x10772834);
    a.load(0, a.r(1) + 0x3c, 0x10772834); a.compare(a.r(0)); if (!a.r(0)) a.call(0x10772eb0, 0x10772844);
    a.load(3, a.r(1) + 0x3c, 0x10772844); a.load<1>(0, a.r(3) + 1, 0x10772848);
    a.put(0, (a.r(0) >> 4) & 1); a.compare(a.r(0));
    a.load(0, a.r(30) + (a.r(0) ? 0x394 : 0x39c), a.r(0) ? 0x10772858 : 0x10772860);
    a.store(a.r(30) + 0x3a0, a.r(0), 0x10772864); a.load(0, a.r(30) + 0x3ac, 0x10772868); a.compare(a.r(0));
    if (!a.r(0)) a.rethrow(); else { a.put(0, 0); a.store(a.r(30) + 0x3b8, a.r(0), 0x10772940); }
    a.copy_search_cursor(0x10772944);
    bool found = false;
    while (!found) {
        a.put(0, a.r(3) & 0xff); a.compare(a.r(0), 15);
        std::uint32_t target = 0x10772a00;
        if (a.r(0) <= 15) {
            a.load(3, a.r(2) - 0x3b9c, 0x107729a0); a.put(0, a.r(0) * 4);
            a.load(3, a.r(3) + a.r(0), 0x107729a8); state.wr<4>(4132, a.r(3)); target = a.r(3);
        }
        switch (target) {
            case 0x107729b4:
                a.load(4, a.r(1) + 0x5c, 0x107729b4); a.put(5, a.r(1) + 0x38);
                a.load(3, a.r(30) + 0x3ac, 0x107729bc); a.load(4, a.r(4) + 2, 0x107729c0);
                a.child(0x10772f10, 0x107729c8); a.put(0, a.r(3)); a.signed_byte(0); a.compare(a.signed_r(0));
                found = a.r(0) != 0; break;
            case 0x107729d8:
                a.load(3, a.r(30) + 0x3ac, 0x107729d8); a.load(4, a.r(1) + 0x5c, 0x107729dc);
                a.child(0x10772430, 0x107729e4); a.compare(a.signed_r(3));
                if (!a.r(3)) {
                    a.load(5, a.r(1) + 0x5c, 0x107729ec); a.put(3, a.r(30)); a.put(4, a.r(1) + 0x3c);
                    a.child(0x10772780, 0x107729fc); if (state.pc != 0x107729fc) return;
                }
                break;
            case 0x10772a00: a.call(0x10772eb0, 0x10772a04); found = true; break;
            case 0x10772a0c: break;
            default:
                resume_exception_unknown_transfer(state, target, a.guest_return); return;
        }
        if (!found) { a.advance(0x10772a14); state.step(0x10772994); }
    }
    a.load(31, a.r(1) + 0x5c, 0x10772a18); a.put(3, a.r(30)); a.put(4, a.r(1) + 0x3c); a.put(5, a.r(31));
    a.child(0x10771b30, 0x10772a2c); a.publish_handler();
    a.load(5, a.r(1) + 0x40, 0x10772a94); record_context_function(state, a.r(5)); a.put(3, a.r(30)); a.load<2>(0, a.r(31) + 6, 0x10772a9c);
    a.load(4, a.r(1) + 0x50, 0x10772aa0); a.load<1>(6, a.r(2) - 0x352a, 0x10772aa4);
    a.put(5, a.r(5) + a.r(0)); a.child(0x10772650, 0x10772ab0);
    if (state.pc != 0x10772ab0) return;
    a.pop(128, 0x10772ab0); a.restore_link(); a.restore(31, 0x10772abc); a.restore(30, 0x10772ac0); a.finish();
}
}
