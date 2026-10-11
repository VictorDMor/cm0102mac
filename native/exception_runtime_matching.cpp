#include "exception_runtime_context.hpp"
#include "mac_abi.hpp"

namespace cm::aot::exception_runtime {
namespace {
struct MatchAbi : MacAbi {
    explicit MatchAbi(State& state) : MacAbi(state, state.exception_runtime_dispatch_escapes) {}
    std::int32_t signed_r(unsigned n) const { return std::bit_cast<std::int32_t>(r(n)); }
    void byte(unsigned n, std::uint32_t value) { put(n, std::uint32_t(std::int32_t(std::int8_t(value)))); }
    void next_expected(std::uint32_t pc) { load<1>(0, r(6) + 1, pc); put(6, r(6) + 1); }
    bool void_pointer() {
        load<1>(0, r(4), 0x10772f28); compare(r(0), 'P'); if (r(0) != 'P') return false;
        next_expected(0x10772f34); compare(r(0), 'C'); if (r(0) == 'C') put(6, r(6) + 1);
        load<1>(0, r(6), 0x10772f44); compare(r(0), 'V'); if (r(0) == 'V') put(6, r(6) + 1);
        load<1>(0, r(6), 0x10772f54); compare(r(0), 'v');
        if (r(0) == 'v') {
            load<1>(0, r(3), 0x10772f60); byte(0, r(0)); compare(signed_r(0), 'P');
            if (r(0) == 'P') return true;
            compare(signed_r(0), '*'); if (r(0) == '*') return true;
        }
        put(6, r(4)); return false;
    }
    void skip_field(std::uint32_t pc) {
        for (;;) {
            load<1>(0, r(3), pc); put(3, r(3) + 1); compare(r(0), '!');
            if (r(0) == '!') return;
            s.step(pc);
        }
    }
    bool encoded_class() {
        load<1>(7, r(3), 0x10772fa4); put(3, r(3) + 1);
        load<1>(0, r(6), 0x10772fac); put(6, r(6) + 1); byte(7, r(7)); byte(0, r(0));
        compare(signed_r(7), signed_r(0)); if (r(7) != r(0)) return false;
        for (;;) {
            load<1>(0, r(6), 0x10772fcc); put(6, r(6) + 1);
            load<1>(7, r(3), 0x10772fd4); byte(0, r(0)); byte(7, r(7)); compare(signed_r(7), signed_r(0));
            if (r(7) == r(0)) {
                compare(signed_r(7), '!'); put(3, r(3) + 1);
                if (r(7) != '!') { s.step(0x10772fcc); continue; }
                put(4, 0);
                // Retail metadata uses unchecked decimal accumulation. Even
                // non-digit signed bytes and overflow retain low-word results.
                for (;;) {
                    load<1>(0, r(3), 0x10773018); compare(r(0), '!'); if (r(0) == '!') break;
                    put(4, r(4) * 10); load<1>(0, r(3), 0x10773004); byte(0, r(0));
                    put(4, r(4) - 0x30); put(4, r(0) + r(4)); put(3, r(3) + 1); s.step(0x10773018);
                }
                store(r(5), r(4), 0x10773024); return true;
            }
            skip_field(0x10773030); skip_field(0x10773040);
            load<1>(0, r(3), 0x10773050); byte(0, r(0)); record(r(0)); if (!r(0)) return false;
            put(6, r(4) + 1); s.step(0x10772fcc);
        }
    }
    bool qualified_encoding() {
        for (;;) {
            load<1>(4, r(3), 0x107730e0); byte(0, r(4)); compare(signed_r(0), 'P');
            bool prefix = r(0) == 'P';
            if (!prefix) { compare(signed_r(0), 'R'); prefix = r(0) == 'R'; }
            if (!prefix) break;
            load<1>(0, r(6), 0x107730f8); byte(4, r(4)); byte(0, r(0)); compare(signed_r(4), signed_r(0));
            if (r(4) != r(0)) break;
            next_expected(0x10773070); put(3, r(3) + 1); compare(r(0), 'C');
            if (r(0) == 'C') {
                load<1>(0, r(3), 0x10773080); compare(r(0), 'C'); if (r(0) == 'C') put(3, r(3) + 1);
                put(6, r(6) + 1);
            }
            load<1>(0, r(3), 0x10773094); byte(4, r(0)); compare(signed_r(4), 'C'); if (r(4) == 'C') return false;
            load<1>(0, r(6), 0x107730b0); compare(r(0), 'V');
            if (r(0) == 'V') {
                compare(signed_r(4), 'V'); if (r(4) == 'V') put(3, r(3) + 1);
                put(6, r(6) + 1);
            }
            load<1>(0, r(3), 0x107730cc); compare(r(0), 'V'); if (r(0) == 'V') return false;
            s.step(0x107730e0);
        }
        for (;;) {
            load<1>(5, r(3), 0x10773128); load<1>(0, r(6), 0x1077312c);
            byte(4, r(5)); byte(0, r(0)); compare(signed_r(4), signed_r(0)); if (r(4) != r(0)) return false;
            byte(0, r(5)); record(r(0)); if (!r(0)) return true;
            put(3, r(3) + 1); put(6, r(6) + 1); s.step(0x10773128);
        }
    }
};
}
void match_types(State& state) {
    MatchAbi a(state);
    a.put(0, 0); a.put(6, a.r(4)); a.record(a.r(6)); a.store(a.r(5), a.r(0), 0x10772f18);
    bool matches = !a.r(6);
    if (!matches) {
        matches = a.void_pointer();
        if (!matches) {
            a.load<1>(0, a.r(3), 0x10772f84); a.byte(0, a.r(0)); a.compare(a.signed_r(0), '*');
            bool encoded = a.r(0) == '*';
            if (!encoded && a.signed_r(0) < '*') { a.compare(a.signed_r(0), '!'); encoded = a.r(0) == '!'; }
            matches = encoded ? a.encoded_class() : a.qualified_encoding();
        }
    }
    a.put(3, matches ? 1 : 0); a.finish();
}
void matches_specification(State& state) {
    MatchAbi a(state);
    a.capture_link(); a.save(31, 0x10772434); a.put(31, 0); a.save(30, 0x1077243c); a.put(30, 0);
    a.save(29, 0x10772444); a.put(29, a.r(4)); a.save(28, 0x1077244c); a.put(28, a.r(3));
    a.save_link(0x10772454); a.frame(80, 0x10772458);
    for (;;) {
        a.load<2>(0, a.r(29) + 2, 0x1077248c); a.compare(a.signed_r(30), a.r(0));
        if (a.signed_r(30) >= std::int32_t(a.r(0))) { a.put(3, 0); break; }
        a.put(0, a.r(31) + 12); a.put(3, a.r(28)); a.load(4, a.r(29) + a.r(0), 0x10772468);
        a.put(5, a.r(1) + 0x38); state.wr<4>(2440, a.r(2)); state.wr<4>(4128, 0x10772474); state.pc = 0x10772f10;
        invoke_exception_runtime(state, 0x10772f10); a.byte(0, a.r(3)); a.record(a.r(0));
        if (a.r(0)) { a.put(3, 1); break; }
        a.put(31, a.r(31) + 4); a.put(30, a.r(30) + 1); state.step(0x1077248c);
    }
    a.pop(80, 0x1077249c); a.restore_link();
    a.restore(31, 0x107724a8); a.restore(30, 0x107724ac); a.restore(29, 0x107724b0); a.restore(28, 0x107724b4); a.finish();
}
}
