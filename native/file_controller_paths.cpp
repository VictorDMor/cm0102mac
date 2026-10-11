#include "file_controller_internal.hpp"

namespace cm::aot::file_controller {
namespace {
void fork_open_permission(Abi& a, std::uint32_t first) {
    a.load<1>(0, a.r(1) + 1196, first); a.put(3, a.r(1) + 412); a.put(4, 0);
    a.put(0, (a.r(0) >> 3) & 7); a.compare(a.r(0), 1); a.put(5, 0); a.put(0, 3);
    if (a.s.r<1>(2304) & 2) a.put(0, 1);
    a.put(6, std::uint32_t(std::int32_t(std::int8_t(a.r(0))))); a.put(7, a.r(1) + 60);
    a.native_call(0x1077ffd0, first == 0x10776ef4 ? 0x10776f24 : 0x10776f60); a.put(30, a.r(3));
}
bool missing_write_allowed(Abi& a, std::uint32_t result, std::uint32_t mode_pc) {
    a.signed_word_record(0, result);
    if (!a.r(0)) return true;
    a.signed_word(0, result); a.compare(a.signed_r(0), -43);
    if (a.signed_r(0) != -43) { a.put(3, 1); return false; }
    a.load<1>(0, a.r(1) + 1196, mode_pc); a.put(0, (a.r(0) >> 6) & 3); a.record(a.r(0));
    if (!a.r(0)) { a.put(3, 1); return false; }
    return true;
}
}
void open_file(Abi& a) {
    a.save(31, 0x10776da0); a.capture_link(); a.put(31, a.r(5)); a.save(30, 0x10776dac); a.save(29, 0x10776db0);
    a.put(29, a.r(3)); a.save_link(0x10776db8); a.frame(1168, 0x10776dbc); a.store(a.r(1) + 1196, a.r(4), 0x10776dc0);
    a.call(0x10776bc0, 0x10776dc8); a.signed_byte_record(0, a.r(3));
    if (a.r(0)) {
        a.put(3, a.r(29)); a.put(4, a.r(1) + 412); a.call(0x10778870, 0x10776ddc); a.put(30, a.r(3));
        a.signed_word(0, a.r(30)); a.compare(a.signed_r(0), -43);
        if (a.signed_r(0) == -43) {
            a.put(3, a.r(29)); a.put(4, a.r(1) + 332); a.put(5, a.r(1) + 636); a.call(0x10778a80, 0x10776dfc);
            a.put(30, a.r(3)); a.signed_word_record(0, a.r(30)); if (!a.r(0)) a.put(30, std::uint32_t(-43));
        } else {
            a.put(3, a.r(1) + 412); a.put(6, a.r(1) + 636); a.put(7, a.r(1) + 260); a.put(8, a.r(1) + 332); a.put(4, 0); a.put(5, 0);
            a.native_call(0x1077ff88, 0x10776e2c);
            a.put(3, a.r(1) + 260); a.put(5, a.r(1) + 59); a.put(6, a.r(1) + 58); a.put(4, 1); a.native_call(0x1077ffa0, 0x10776e44);
            a.put(3, a.r(1) + 260); a.put(4, a.r(1) + 412); a.native_call(0x1077fef8, 0x10776e54);
        }
        if (!missing_write_allowed(a, a.r(30), 0x10776e6c)) goto done;
        a.signed_word_record(0, a.r(30));
        if (a.r(0)) {
            a.load<1>(0, a.r(1) + 1197, 0x10776e88); a.put(30, (a.r(0) >> 3) & 1); a.put(3, a.r(30)); a.call(0x10776d60, 0x10776e98);
            a.store(a.r(1) + 564, a.r(3), 0x10776e98); a.put(3, a.r(30)); a.call(0x10776d20, 0x10776ea4); a.put(0, 0);
            a.store(a.r(1) + 568, a.r(3), 0x10776ea8); a.store<2>(a.r(1) + 572, a.r(0), 0x10776eac); a.store<2>(a.r(1) + 576, a.r(0), 0x10776eb0);
            a.store<2>(a.r(1) + 574, a.r(0), 0x10776eb4); a.store<2>(a.r(1) + 578, a.r(0), 0x10776eb8);
            a.call(0x10778820, 0x10776ec0); a.store(a.r(1) + 632, a.r(3), 0x10776ec0);
            a.put(3, a.r(1) + 332); a.load<2>(4, a.r(1) + 636, 0x10776ec8); a.put(5, a.r(1) + 638); a.put(7, a.r(1) + 492); a.put(8, a.r(1) + 412); a.put(6, 0x801); a.put(9, 0);
            a.native_call(0x1077ffb8, 0x10776ee4); a.put(30, a.r(3)); a.signed_word_record(0, a.r(30));
            if (!a.r(0)) fork_open_permission(a, 0x10776ef4);
        } else {
            fork_open_permission(a, 0x10776f30); a.signed_word_record(0, a.r(30));
            if (!a.r(0)) {
                a.load<1>(0, a.r(1) + 1196, 0x10776f70); a.put(0, (a.r(0) >> 6) & 3); a.compare(a.r(0), 2);
                if (a.r(0) == 2) {
                    a.load<2>(3, a.r(1) + 60, 0x10776f80); a.signed_word(3, a.r(3)); a.put(4, 1); a.put(6, 0); a.put(5, 0);
                    a.native_call(0x1077ffe8, 0x10776f94); a.put(30, a.r(3)); a.signed_word_record(0, a.r(30));
                    if (a.r(0)) { a.load<2>(3, a.r(1) + 60, 0x10776fa4); a.signed_word(3, a.r(3)); a.native_call(0x1077fe68, 0x10776fac); }
                }
            }
        }
        a.signed_word_record(0, a.r(30));
        if (a.r(0)) a.put(3, 1);
        else { a.load<2>(0, a.r(1) + 60, 0x10776fc0); a.signed_word(0, a.r(0)); a.put(3, 0); a.store(a.r(31), a.r(0), 0x10776fc8); }
    } else {
        a.put(3, a.r(29)); a.put(4, a.r(1) + 188); a.call(0x107786f0, 0x10776fdc); a.put(30, a.r(3)); a.call(0x10776d10, 0x10776fe4); a.compare(a.signed_r(3));
        if (a.r(3)) { a.put(3, a.r(1) + 188); a.put(5, a.r(1) + 57); a.put(6, a.r(1) + 56); a.put(4, 1); a.native_call(0x1077ffa0, 0x10777000); }
        if (!missing_write_allowed(a, a.r(30), 0x10777018)) goto done;
        a.load<2>(0, a.r(1) + 188, 0x1077702c); a.signed_word(0, a.r(0)); a.put(3, a.r(1) + 194); a.store(a.r(1) + 82, a.r(3), 0x10777034); a.put(5, 3);
        a.store<2>(a.r(1) + 86, a.r(0), 0x1077703c); a.load<1>(0, a.r(1) + 1196, 0x10777040); a.put(0, (a.r(0) >> 3) & 7); a.compare(a.r(0), 1);
        if (a.r(0) == 1) a.put(5, 1);
        a.load(3, a.r(1) + 190, 0x10777054); a.put(4, 0); a.signed_word_record(0, a.r(30));
        a.store<1>(a.r(1) + 91, a.r(5), 0x10777060); a.store(a.r(1) + 92, a.r(4), 0x10777064); a.store<1>(a.r(1) + 90, a.r(4), 0x10777068); a.store(a.r(1) + 112, a.r(3), 0x1077706c);
        if (a.r(0)) {
            a.put(3, a.r(1) + 64); a.native_call(0x10780018, 0x1077707c); a.put(30, a.r(3)); a.signed_word_record(0, a.r(30));
            if (!a.r(0)) {
                a.load<1>(0, a.r(1) + 1197, 0x1077708c); a.put(3, a.r(1) + 188); a.put(4, (a.r(0) >> 3) & 1); a.call(0x10776c30, 0x1077709c);
                a.put(3, a.r(1) + 64); a.native_call(0x10780048, 0x107770a4); a.put(30, a.r(3));
            }
        } else {
            a.put(3, a.r(1) + 64); a.native_call(0x10780048, 0x107770b8); a.put(30, a.r(3)); a.signed_word_record(0, a.r(30));
            if (!a.r(0)) {
                a.load<1>(0, a.r(1) + 1196, 0x107770c8); a.put(0, (a.r(0) >> 6) & 3); a.compare(a.r(0), 2);
                if (a.r(0) == 2) {
                    a.put(0, 0); a.store(a.r(1) + 92, a.r(0), 0x107770dc); a.put(3, a.r(1) + 64); a.native_call(0x1077fd60, 0x107770e8);
                    a.put(30, a.r(3)); a.signed_word_record(0, a.r(30));
                    if (a.r(0)) { a.put(3, a.r(1) + 64); a.native_call(0x1077fe98, 0x10777100); }
                }
            }
        }
        a.signed_word_record(0, a.r(30));
        if (a.r(0)) a.put(3, 1);
        else { a.load<2>(0, a.r(1) + 88, 0x10777114); a.signed_word(0, a.r(0)); a.put(3, 0); a.store(a.r(31), a.r(0), 0x1077711c); }
    }
done:
    a.pop(1168, 0x10777120); a.restore(31, 0x10777128); a.restore(30, 0x1077712c); a.restore_link(); a.restore(29, 0x10777134); a.finish();
}

void remove_file(Abi& a) {
    a.save(31, 0x10777990); a.capture_link(); a.put(31, a.r(3)); a.save_link(0x1077799c); a.frame(352, 0x107779a0);
    a.call(0x10776bc0, 0x107779a8); a.signed_byte_record(0, a.r(3));
    if (a.r(0)) {
        a.put(3, a.r(31)); a.put(4, a.r(1) + 252); a.call(0x10778870, 0x107779bc); a.signed_word_record(0, a.r(3));
        if (a.r(0)) a.put(3, 1);
        else { a.put(3, a.r(1) + 252); a.native_call(0x1077fe80, 0x107779d4); a.signed_word(3, a.r(3)); a.boolean_error(); }
    } else {
        a.put(3, a.r(31)); a.put(4, a.r(1) + 180); a.call(0x107786f0, 0x107779f8); a.signed_word_record(0, a.r(3));
        if (a.r(0)) a.put(3, 1);
        else {
            a.load<2>(5, a.r(1) + 180, 0x10777a08); a.signed_word(5, a.r(5)); a.put(3, a.r(1) + 186);
            a.load(0, a.r(1) + 182, 0x10777a10); a.put(4, 0); a.store(a.r(1) + 74, a.r(3), 0x10777a18); a.put(3, a.r(1) + 56);
            a.store<2>(a.r(1) + 78, a.r(5), 0x10777a20); a.store<1>(a.r(1) + 82, a.r(4), 0x10777a24); a.store(a.r(1) + 104, a.r(0), 0x10777a28);
            a.native_call(0x1077feb0, 0x10777a30); a.signed_word(3, a.r(3)); a.boolean_error();
        }
    }
    a.pop(352, 0x10777a44); a.restore(31, 0x10777a4c); a.restore_link(); a.finish();
}

void rename_file(Abi& a) {
    a.save(31, 0x10777a60); a.capture_link(); a.put(31, a.r(4)); a.save(30, 0x10777a6c); a.put(30, a.r(3)); a.save_link(0x10777a74); a.frame(1088, 0x10777a78);
    a.call(0x10776bc0, 0x10777a80); a.signed_byte_record(0, a.r(3));
    if (a.r(0)) {
        a.put(3, a.r(30)); a.put(4, a.r(1) + 484); a.call(0x10778870, 0x10777a94); a.signed_word_record(0, a.r(3));
        if (a.r(0)) { a.signed_word(0, a.r(3)); a.compare(a.signed_r(0), -1302); if (a.signed_r(0) != -1302) { a.put(3, 1); goto done; } }
        a.put(3, a.r(1) + 484); a.put(8, a.r(1) + 404); a.put(4, 0); a.put(5, 0); a.put(6, 0); a.put(7, 0); a.native_call(0x1077ff88, 0x10777acc); a.signed_word_record(0, a.r(3));
        if (a.r(0)) { a.put(3, 1); goto done; }
        a.put(3, a.r(31)); a.put(4, a.r(1) + 324); a.put(5, a.r(1) + 564); a.call(0x10778a80, 0x10777af0); a.signed_word_record(0, a.r(3));
        if (a.r(0)) { a.signed_word(0, a.r(3)); a.compare(a.signed_r(0), -43); if (a.signed_r(0) != -43) { a.put(3, 1); goto done; } }
        a.put(3, a.r(1) + 404); a.put(4, a.r(1) + 324); a.native_call(0x10781098, 0x10777b18); a.signed_word_record(0, a.r(3));
        if (a.r(0)) { a.put(3, 1); goto done; }
        a.call(0x10778820, 0x10777b30); a.load<2>(4, a.r(1) + 564, 0x10777b30); a.put(6, a.r(3));
        a.put(3, a.r(1) + 484); a.put(5, a.r(1) + 566); a.put(7, 0); a.native_call(0x107810b0, 0x10777b48); a.signed_word(3, a.r(3)); a.boolean_error();
    } else {
        a.put(3, a.r(30)); a.put(4, a.r(1) + 252); a.call(0x107786f0, 0x10777b6c); a.signed_word_record(0, a.r(3));
        if (a.r(0)) { a.signed_word(0, a.r(3)); a.compare(a.signed_r(0), -1302); if (a.signed_r(0) != -1302) { a.put(3, 1); goto done; } }
        a.put(3, a.r(31)); a.put(4, a.r(1) + 180); a.call(0x107786f0, 0x10777b94); a.signed_word_record(0, a.r(3));
        if (a.r(0)) { a.signed_word(0, a.r(3)); a.compare(a.signed_r(0), -43); if (a.signed_r(0) != -43) { a.put(3, 1); goto done; } }
        a.load<2>(4, a.r(1) + 252, 0x10777bb0); a.signed_word(4, a.r(4)); a.load<2>(0, a.r(1) + 180, 0x10777bb4); a.signed_word(0, a.r(0)); a.compare(a.signed_r(4), a.signed_r(0));
        if (a.r(4) != a.r(0)) { a.put(3, 1); goto done; }
        a.load(5, a.r(1) + 254, 0x10777bc0); a.load(0, a.r(1) + 182, 0x10777bc4); a.compare(a.signed_r(5), a.signed_r(0));
        if (a.r(5) != a.r(0)) { a.put(3, 1); goto done; }
        a.signed_word_record(0, a.r(3)); if (!a.r(0)) { a.put(3, 1); goto done; }
        a.put(3, a.r(1) + 258); a.store<2>(a.r(1) + 78, a.r(4), 0x10777bec); a.put(0, a.r(1) + 186); a.put(4, 0);
        a.store(a.r(1) + 74, a.r(3), 0x10777bf8); a.put(3, a.r(1) + 56); a.store<1>(a.r(1) + 82, a.r(4), 0x10777c00);
        a.store(a.r(1) + 84, a.r(0), 0x10777c04); a.store(a.r(1) + 104, a.r(5), 0x10777c08); a.native_call(0x107810c8, 0x10777c10); a.signed_word(3, a.r(3)); a.boolean_error();
    }
done:
    a.pop(1088, 0x10777c24); a.restore(31, 0x10777c2c); a.restore_link(); a.restore(30, 0x10777c34); a.finish();
}
}
