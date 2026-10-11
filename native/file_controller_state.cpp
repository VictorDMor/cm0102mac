#include "file_controller_internal.hpp"

namespace cm::aot::file_controller {
void parse_mode(Abi& a) {
    // These stores precede mode validation. An aliased output can change the
    // subsequently read mode bytes; retain the live reads and partial effects.
    a.load<2>(0, a.r(4), 0x10776a40); a.put(5, 1);
    a.put(0, (a.r(0) & ~0x1c0u) | 0x40); a.store<2>(a.r(4), a.r(0), 0x10776a4c);
    a.put(5, 0); a.load<1>(0, a.r(4) + 1, 0x10776a54);
    a.put(0, a.r(0) & ~0x30u); a.store<1>(a.r(4) + 1, a.r(0), 0x10776a5c);
    a.load<1>(0, a.r(4) + 1, 0x10776a60); a.put(0, a.r(0) & ~8u);
    a.store<1>(a.r(4) + 1, a.r(0), 0x10776a68);
    a.load<1>(0, a.r(3), 0x10776a6c); a.put(6, std::uint32_t(std::int32_t(std::int8_t(a.r(0)))));
    a.compare(a.signed_r(6), 'r');
    if (a.r(6) == 'r') a.put(5, 0);
    else if (a.signed_r(6) < 'r') {
        a.compare(a.signed_r(6), 'a');
        if (a.r(6) != 'a') { a.put(3, 0); a.finish(); return; }
        a.put(5, 1);
    } else {
        a.compare(a.signed_r(6), 'w');
        if (a.r(6) != 'w') { a.put(3, 0); a.finish(); return; }
        a.put(5, 2);
    }
    a.load<1>(0, a.r(4), 0x10776ab8); a.put(0, (a.r(0) & ~0xc0u) | (a.r(5) << 6));
    a.store<1>(a.r(4), a.r(0), 0x10776ac0);
    a.load<1>(0, a.r(3) + 1, 0x10776ac4); a.put(0, std::uint32_t(std::int32_t(std::int8_t(a.r(0)))));
    a.compare(a.signed_r(0), 'b');
    if (a.r(0) == 'b') {
        a.load<1>(0, a.r(4) + 1, 0x10776ae4); a.put(5, 1); a.put(0, a.r(0) | 8);
        a.store<1>(a.r(4) + 1, a.r(0), 0x10776af0);
        a.load<1>(0, a.r(3) + 2, 0x10776af4); a.compare(a.signed_r(0), '+');
        if (a.r(0) == '+') { a.put(0, a.r(6) << 8); a.put(6, a.r(0) | '+'); }
    } else if (a.signed_r(0) < 'b') {
        a.compare(a.signed_r(0), '+');
        if (a.r(0) == '+') {
            a.load<1>(0, a.r(3) + 2, 0x10776b0c); a.put(3, a.r(6) << 8); a.put(6, a.r(3) | '+');
            a.compare(a.signed_r(0), 'b');
            if (a.r(0) == 'b') {
                a.load<1>(0, a.r(4) + 1, 0x10776b20); a.put(3, 1); a.put(0, a.r(0) | 8);
                a.store<1>(a.r(4) + 1, a.r(0), 0x10776b2c);
            }
        }
    }
    // The original accepts a valid prefix, including trailing unknown bytes.
    // Its comparison tree leaves the final condition field observable.
    a.compare(a.signed_r(6), 0x612b);
    if (a.r(6) == 0x612b) a.put(7, 7);
    else if (a.signed_r(6) < 0x612b) {
        a.compare(a.signed_r(6), 'r');
        if (a.r(6) == 'r') a.put(7, 1);
        else if (a.signed_r(6) < 'r') { a.compare(a.signed_r(6), 'a'); if (a.r(6) == 'a') a.put(7, 6); }
        else { a.compare(a.signed_r(6), 'w'); if (a.r(6) == 'w') a.put(7, 2); }
    } else {
        a.compare(a.signed_r(6), 0x772b);
        if (a.r(6) == 0x772b) a.put(7, 3);
        else if (a.signed_r(6) < 0x772b) { a.compare(a.signed_r(6), 0x722b); if (a.r(6) == 0x722b) a.put(7, 3); }
    }
    a.load<1>(0, a.r(4), 0x10776ba4); a.put(0, (a.r(0) & ~0x38u) | ((a.r(7) << 3) & 0x38));
    a.put(3, 1); a.store<1>(a.r(4), a.r(0), 0x10776bb0); a.finish();
}

void fork_capability(Abi& a) {
    a.capture_link(); a.save_link(0x10776bc4); a.frame(64, 0x10776bc8);
    a.load<1>(0, a.r(2) - 0x3504, 0x10776bcc);
    a.put(0, std::uint32_t(std::int32_t(std::int8_t(a.r(0))))); a.record(a.r(0));
    if (!a.r(0)) {
        a.put(3, 0x66732020); a.put(4, a.r(1) + 56); a.call(0x1077fad8, 0x10776be8);
        a.load(2, a.r(1) + 20, 0x10776be8);
        a.put(0, std::uint32_t(std::int32_t(std::int16_t(a.r(3))))); a.record(a.r(0));
        bool available = !a.r(0);
        if (available) { a.load(0, a.r(1) + 56, 0x10776bf4); a.put(0, a.r(0) & 0x1000); a.record(a.r(0)); available = a.r(0) != 0; }
        if (available) { a.put(0, 1); a.store<1>(a.r(2) - 0x3503, a.r(0), 0x10776c04); }
        a.put(0, 1); a.store<1>(a.r(2) - 0x3504, a.r(0), 0x10776c0c);
    }
    a.load<1>(3, a.r(2) - 0x3503, 0x10776c10);
    a.pop(64, 0x10776c14); a.restore_link(); a.finish();
}

void find_file_record(Abi& a) {
    a.load(4, a.r(2) - 0x3508, 0x10776cd0);
    a.put(0, std::uint32_t(std::int32_t(std::int16_t(a.r(3)))));
    for (;;) {
        a.compare(a.r(4));
        if (!a.r(4)) { a.put(3, 0); a.finish(); return; }
        a.load<2>(3, a.r(4), 0x10776ce0); a.put(3, std::uint32_t(std::int32_t(std::int16_t(a.r(3)))));
        a.compare(a.signed_r(3), a.signed_r(0));
        if (a.r(3) == a.r(0)) { a.put(3, a.r(4)); a.finish(); return; }
        a.load(4, a.r(4) + 152, 0x10776cf8);
        // One step per traversal retains a finite diagnostic budget for corrupt
        // cyclic lists without adding an application-visible rejection policy.
        a.s.step(0x10776cfc);
    }
}
void alias_policy(Abi& a) { a.put(3, 1); a.finish(); }
void metadata_code(Abi& a, bool creator) {
    const auto entry = creator ? 0x10776d20u : 0x10776d60u;
    a.load(4, a.r(2) - (creator ? 0x5a5c : 0x5a60), entry);
    a.load(0, a.r(4), entry + 4); a.compare(a.signed_r(0));
    if (a.r(0)) a.put(3, a.r(0));
    else {
        a.compare(a.signed_r(3)); const bool text = !a.r(3);
        a.put(3, creator ? 0x43574945 : 0x54455854);
        if (!text) a.put(3, creator ? 0x3f3f3f3f : 0x42494e41);
    }
    a.finish();
}

void tell_position(Abi& a) {
    a.load<2>(0, a.r(3) + 4, 0x10777c40); a.put(0, (a.r(0) >> 6) & 7); a.compare(a.r(0), 1);
    bool valid = a.r(0) == 1;
    if (!valid) { a.compare(a.r(0), 2); valid = a.r(0) == 2; }
    if (valid) { a.load<1>(0, a.r(3) + 13, 0x10777c58); a.compare(a.r(0)); valid = !a.r(0); }
    if (!valid) {
        a.load(4, a.r(2) - 0x5a58, 0x10777c64); a.put(0, 40); a.put(3, UINT32_MAX);
        a.store(a.r(4), a.r(0), 0x10777c70); a.finish(); return;
    }
    a.load<1>(0, a.r(3) + 8, 0x10777c78); a.put(5, (a.r(0) >> 5) & 7); a.record(a.r(5));
    if (!a.r(5)) { a.load(3, a.r(3) + 28, 0x10777c84); a.finish(); return; }
    a.load(4, a.r(3) + 32, 0x10777c8c); a.compare(a.r(5), 3);
    a.load(0, a.r(3) + 40, 0x10777c94); a.load(3, a.r(3) + 56, 0x10777c98);
    a.put(0, a.r(0) - a.r(4)); a.put(3, a.r(3) + a.r(0));
    if (a.r(5) >= 3) { a.put(0, a.r(5) - 2); a.put(3, a.r(3) - a.r(0)); }
    a.finish();
}
void tell_facade(Abi& a) {
    a.capture_link(); a.save_link(0x10777cc4); a.frame(64, 0x10777cc8);
    a.call(0x10777c40, 0x10777cd0); a.pop(64, 0x10777cd0); a.restore_link(); a.finish();
}
void get_position(Abi& a) {
    a.save(31, 0x10777ce0); a.capture_link(); a.put(31, a.r(4)); a.save_link(0x10777cec); a.frame(64, 0x10777cf0);
    a.call(0x10777c40, 0x10777cf8); a.store(a.r(31), a.r(3), 0x10777cf8);
    a.load(0, a.r(31), 0x10777cfc);
    // subfic with -1 always sets carry for a word32 operand. The subsequent
    // count-leading-zero result exposes whether tell returned the -1 sentinel.
    a.s.wr<1>(1027, 1); a.put(0, UINT32_MAX - a.r(0));
    a.put(0, std::countl_zero(a.r(0))); a.put(3, a.r(0) >> 5);
    a.pop(64, 0x10777d0c); a.restore_link(); a.restore(31, 0x10777d18); a.finish();
}

void seek_position(Abi& a) {
    a.save(31, 0x10777d20); a.capture_link(); a.load(31, a.r(2) - 0x5a58, 0x10777d28);
    a.save(30, 0x10777d2c); a.put(30, a.r(5)); a.save(29, 0x10777d34); a.put(29, a.r(3));
    a.save_link(0x10777d3c); a.frame(80, 0x10777d40); a.store(a.r(1) + 108, a.r(4), 0x10777d44);
    a.load<2>(0, a.r(3) + 4, 0x10777d48); a.put(0, (a.r(0) >> 6) & 7); a.compare(a.r(0), 1);
    bool valid = a.r(0) == 1;
    if (valid) { a.load<1>(0, a.r(29) + 13, 0x10777d58); a.compare(a.r(0)); valid = !a.r(0); }
    if (!valid) { a.put(0, 40); a.store(a.r(31), a.r(0), 0x10777d68); a.put(3, UINT32_MAX); goto done; }
    a.load<1>(0, a.r(29) + 8, 0x10777d74); a.put(0, (a.r(0) >> 5) & 7); a.compare(a.r(0), 1);
    if (a.r(0) == 1) {
        a.put(4, 0); a.call(0x107758a0, 0x10777d8c); a.compare(a.signed_r(3));
        if (a.r(3)) {
            a.put(0, 1); a.store<1>(a.r(29) + 13, a.r(0), 0x10777d98); a.put(3, 0); a.put(0, 40);
            a.store(a.r(29) + 44, a.r(3), 0x10777da4); a.put(3, UINT32_MAX); a.store(a.r(31), a.r(0), 0x10777dac); goto done;
        }
    }
    a.compare(a.signed_r(30), 1);
    if (a.r(30) == 1) {
        a.put(3, a.r(29)); a.put(30, 0); a.call(0x10777c40, 0x10777dc8);
        a.load(0, a.r(1) + 108, 0x10777dc8); a.put(0, a.r(0) + a.r(3)); a.store(a.r(1) + 108, a.r(0), 0x10777dd0);
    }
    a.compare(a.signed_r(30), 2);
    {
        bool buffered = a.r(30) != 2;
        if (buffered) { a.load<1>(0, a.r(29) + 4, 0x10777ddc); a.put(0, (a.r(0) >> 3) & 7); a.compare(a.r(0), 3); buffered = a.r(0) != 3; }
        if (buffered) {
            a.load<1>(0, a.r(29) + 8, 0x10777dec); a.put(0, (a.r(0) >> 5) & 7); a.compare(a.r(0), 2);
            buffered = a.r(0) == 2;
            if (!buffered) { a.compare(a.r(0), 3); buffered = a.r(0) == 3; }
        }
        if (buffered) {
            a.load(3, a.r(1) + 108, 0x10777e04); a.load(0, a.r(29) + 28, 0x10777e08); a.compare(a.r(3), a.r(0));
            bool inside = a.r(3) < a.r(0);
            if (inside) { a.load(0, a.r(29) + 56, 0x10777e14); a.compare(a.r(3), a.r(0)); inside = a.r(3) >= a.r(0); }
            if (inside) {
                a.load(4, a.r(29) + 32, 0x10777e34); a.put(0, a.r(3) - a.r(0)); a.put(3, 2); a.put(0, a.r(4) + a.r(0));
                a.store(a.r(29) + 40, a.r(0), 0x10777e44); a.load(4, a.r(1) + 108, 0x10777e48);
                a.load(0, a.r(29) + 28, 0x10777e4c); a.put(0, a.r(0) - a.r(4)); a.store(a.r(29) + 44, a.r(0), 0x10777e54);
                a.load<1>(0, a.r(29) + 8, 0x10777e58); a.put(0, (a.r(0) & ~0xe0u) | 0x40); a.store<1>(a.r(29) + 8, a.r(0), 0x10777e60);
            } else {
                a.load<1>(0, a.r(29) + 8, 0x10777e20); a.put(3, 0); a.put(0, a.r(0) & ~0xe0u); a.store<1>(a.r(29) + 8, a.r(0), 0x10777e2c);
            }
        } else {
            a.load<1>(0, a.r(29) + 8, 0x10777e68); a.put(3, 0); a.put(0, a.r(0) & ~0xe0u); a.store<1>(a.r(29) + 8, a.r(0), 0x10777e74);
        }
    }
    a.load<1>(0, a.r(29) + 8, 0x10777e78); a.put(0, (a.r(0) >> 5) & 7); a.record(a.r(0));
    if (!a.r(0)) {
        a.load(12, a.r(29) + 60, 0x10777e84); a.compare(a.r(12));
        if (a.r(12)) {
            a.load(3, a.r(29), 0x10777e90); a.put(5, a.r(30)); a.load(6, a.r(29) + 76, 0x10777e98);
            a.put(4, a.r(1) + 108); a.callback(0x10777ea4); a.compare(a.signed_r(3));
            if (a.r(3)) {
                a.put(0, 1); a.store<1>(a.r(29) + 13, a.r(0), 0x10777eb4); a.put(3, 0); a.put(0, 40);
                a.store(a.r(29) + 44, a.r(3), 0x10777ec0); a.put(3, UINT32_MAX); a.store(a.r(31), a.r(0), 0x10777ec8); goto done;
            }
        }
        a.put(3, 0); a.store<1>(a.r(29) + 12, a.r(3), 0x10777ed4);
        a.load(0, a.r(1) + 108, 0x10777ed8); a.store(a.r(29) + 28, a.r(0), 0x10777edc); a.store(a.r(29) + 44, a.r(3), 0x10777ee0);
    }
    // Seeking within an existing read buffer preserves EOF; only the actual
    // callback/position-update path above clears it.
    a.put(3, 0);
done:
    a.pop(80, 0x10777ee8); a.restore(31, 0x10777ef0); a.restore(30, 0x10777ef4);
    a.restore_link(); a.restore(29, 0x10777efc); a.finish();
}
void seek_facade(Abi& a) {
    a.capture_link(); a.save_link(0x10777f14); a.frame(64, 0x10777f18);
    a.call(0x10777d20, 0x10777f20); a.pop(64, 0x10777f20); a.restore_link(); a.finish();
}
void set_position(Abi& a) {
    a.capture_link(); a.put(5, 0); a.save_link(0x10777f38); a.frame(64, 0x10777f3c);
    a.load(4, a.r(4), 0x10777f40); a.call(0x10777d20, 0x10777f48);
    a.pop(64, 0x10777f48); a.restore_link(); a.finish();
}
void clear_status(Abi& a) {
    a.put(0, 0); a.store<1>(a.r(3) + 12, a.r(0), 0x107784a4); a.store<1>(a.r(3) + 13, a.r(0), 0x107784a8); a.finish();
}
void initialize_stdio(Abi& a) {
    a.load(0, a.r(2) - 0x5a54, 0x107784b0); a.load(3, a.r(2) - 0x5a80, 0x107784b4);
    a.store(a.r(3), a.r(0), 0x107784b8); a.finish();
}
}
