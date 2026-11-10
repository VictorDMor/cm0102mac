// MANAGER_MANAGER.CPP weekly pass, Part C: 0x101a4e08..0x101a6958. Included inside `class Club`
// (manager_manager_weekly.cpp), so the sections share the per-club members.
//
// The stretch is a row of stanzas. Each opens with the club's tests (a league, its region with the byte
// at +0x120, a manager record; some also the league record's word +0x1c against -1), then draws
// from the table and compares them with the staff record r25 (idle +4, +0x75/+0x76/+0x77, counts
// +0x1c/+0x30/+0x44/+0x58) and the stats call's frame bytes (+0x38/+0x39/+0x3a). Every test that
// fails goes on at the next stanza; one that passes all of them hires (0x106487d0 by index or
// 0x10647730) and posts an event (0x106891b0) and leaves for 0x101a6958 (or posts one with no person).
//
//   stanza   starts   event  hire  notes
//   s60      0x4e08   0x60   yes   +0x1c <= 1 asks a second draw; league virtual 0xa4; two %3; level arithmetic
//   s43      0x50e8   0x43   yes   %200; counts +0x44/+0x58 < 3; a floating-point compare (idle bytes +0x6c/+0x6d)
//   s63      0x5498   0x63   yes   chain w == -1; %10, %5; +0x76 in (1,4), +0x75 in (3,6)
//   s61      0x576c   0x61   yes   as s63 with +0x75 > 5
//   s62      0x5a38   0x62   yes   chain w == -1; +0x76 == 1, +0x75 == 1; then `tri_tail`
//   s40      0x5c90   0x40   yes   chain w != -1; league record's +0xc0/+0xc1; `tri_tail`
//   s41      0x5f10   0x41   yes   chain w == -1; stats byte 0x39 in {2,3}; `tri_tail`
//   s5e      0x6178   0x5e   no    reputation against a draw; +0x76 < +0x75
//   s3e      0x6350   0x3e   no    league virtual 0xa4; level arithmetic
//   s3f      0x6504   0x3f   yes   %15; league virtual 0xa4; level arithmetic with stats bytes 0x39/0x3a
//   s6790    0x6790   0x3c   no    idle > 20, +0x75 - +0x76 < -5, a manager test 0x100d94d0, row target < 0x10b3; ends at 0x101a6958 either way
// Every path of the row ends at 0x101a6958.

// The first four tests of the stanzas that open with `lwz r0,0x30(r31)`: league, region, its byte at +0x120,
// the manager record.
bool guard_low() {
    const auto league = c.u32(club + club::league);
    c.set(0, league);
    if (!nonzero_logical(c, league)) return false;
    const auto region = c.u32(club + club::region);
    c.set(3, region);
    if (!nonzero_logical(c, region)) return false;
    const auto flag = sbyte(c.u8(region + 0x120));
    c.set(0, word(flag));
    if (!nonzero(c, flag)) return false;
    const auto manager = c.u32(club + club::manager);
    c.set(0, manager);
    return nonzero_logical(c, manager);
}

// The league object's virtual method at +0xa4 (through the pointer glue); true when it answers nonzero.
bool league_virtual(std::uint32_t back) {
    const auto league = c.u32(club + club::league);
    const auto table = c.u32(c.r(18));
    const auto record = c.u32(table + (c.u32(league) << 2));
    c.set({{3, record}, {4, table}});
    c.set(12, c.u32(c.u32(record) + 0xa4));
    c.call(0x10781320, back);
    c.set(2, c.u32(frame.at(0x14)));
    const auto result = sbyte(c.r(3));
    c.set(0, word(result));
    return nonzero(c, result);
}

// The last draw of a stanza (its parity decides the hire's kind), the hire and the event; true: leave for 0x101a6958.
bool final_hire(std::uint32_t back_draw, std::uint32_t bl_index, std::uint32_t bl_any, std::uint32_t bl_post,
                std::uint32_t code) {
    const auto x = draw(back_draw);
    const auto r = rem(x, 2);
    c.set({{3, word(x) >> 31}, {0, word(r)}});
    const bool by_index = compare(c, r) == 0;
    hire_post(r15, by_index, bl_index, bl_any, bl_post, code);
    return true;
}

static std::uint32_t quotient_sign(std::uint32_t magic, unsigned shift, std::int32_t x) {
    return std::uint32_t(mulhw(magic, x) >> shift) >> 31;
}
static std::int32_t half_of(std::uint32_t v) { return std::int32_t(v + (v >> 31)) >> 1; }  // srawi after the sign add

// 0x101a4e08..: the row of stanzas; every path leaves for 0x101a6958 (part D).
std::uint32_t stanzas() {
    using Stanza = bool (Club::*)();
    for (const Stanza stanza : {&Club::s60, &Club::s43, &Club::s63, &Club::s61, &Club::s62, &Club::s40, &Club::s41, &Club::s5e,
                                &Club::s3e, &Club::s3f})
        if ((this->*stanza)()) return label::wide;
    s6790();
    return label::wide;
}

// ---- 0x101a4e08..0x101a50e8 ----
bool s60() {
    if (!guard_low()) return false;
    auto x = draw(0x101a4e58);
    auto idle = std::int32_t(c.u8(r25 + 4));
    auto r = rem(x, 10);
    c.set({{5, word(x)}, {6, word(idle)}, {4, word(r)}, {0, word(r + 0x14)}});
    compare(c, idle, r + 0x14);
    if (idle <= r + 0x14) return false;
    auto w = c.i32(r25 + 0x1c);
    c.set(0, word(w));
    if (compare(c, w, 1) <= 0) {
        x = draw(0x101a4ec8);
        const auto w44 = c.i32(r25 + 0x44);
        r = rem(x, 3);
        c.set({{3, word(r)}, {4, word(x)}, {5, word(w44)}, {0, word(r + 2)}});
        compare(c, w44, r + 2);
        if (w44 <= r + 2) return false;
    }
    if (!league_virtual(0x101a4f2c)) return false;
    x = draw(0x101a4f58);
    {
        const auto cursor = c.u32(t.cursor_cell);
        c.set({{7, cursor}, {5, word(x)}, {6, c.u16(cursor)}, {4, word(mulhw(0x55555556, x))}});
        r15 = word(rem(x, 3));
    }
    const auto x4 = draw(0x101a4fa8);
    const auto cursor = c.u32(t.cursor_cell);
    const auto league = c.u32(club + club::league);
    const auto table = c.u32(c.r(18));
    const auto record = c.u32(table + (c.u32(league) << 2));
    const auto c3 = sbyte(c.u8(record + 0xc3));
    const auto b76 = std::int32_t(c.u8(r25 + 0x76));
    const auto hi = std::int32_t(mulhw(0x55555556, x4));
    const auto q3 = (word(hi) + (word(hi) >> 31)) * 3u;
    const auto sum = word(b76) + r15;
    const auto gap = std::int32_t(word(r13) - word(c3));
    c.set({{3, cursor}, {4, word(c3)}, {5, sum}, {6, q3}, {7, word(gap)}});
    const auto lhs = std::int32_t(sum - (word(x4) - q3));
    compare(c, lhs, gap);
    if (lhs <= gap) return false;
    const auto b3a = std::int32_t(c.u8(frame.at(0x3a)));
    const auto limit = std::int32_t(word(gap) - word(half_of(r13)));
    c.set(4, word(b3a));
    compare(c, b3a, limit);
    if (b3a >= limit) return false;
    const auto b38 = std::int32_t(c.u8(frame.at(0x38)));
    compare(c, b38, gap);
    if (b38 < gap) return false;
    return final_hire(0x101a5054, 0x101a5090, 0x101a50bc, 0x101a50e0, 0x60);
}

// ---- 0x101a50e8..0x101a5498 ----
bool s43() {
    if (!guard_low()) return false;
    auto x = draw(0x101a5138);
    const auto idle = std::int32_t(c.u8(r25 + 4));
    auto r = rem(x, 200);
    c.set({{5, word(x)}, {6, word(idle)}, {4, word(r)}, {0, word(r + 10)}});
    compare(c, idle, r + 10);
    if (idle <= r + 10) return false;
    auto w = c.i32(r25 + 0x44);
    c.set(0, word(w));
    if (compare(c, w, 3) >= 0) return false;
    w = c.i32(r25 + 0x58);
    c.set(0, word(w));
    if (compare(c, w, 3) >= 0) return false;
    x = draw(0x101a51b4);
    {
        const auto cursor = c.u32(t.cursor_cell);
        const auto idle_now = c.u8(r25 + 4);
        const auto sum = std::uint32_t(c.u8(r25 + 0x6c)) + c.u8(r25 + 0x6d);
        const auto hi = mulhw(0x2aaaaaab, x);
        const auto rem6 = word(x) - (word(hi) + (word(hi) >> 31)) * 6u;
        c.put32(frame.at(0xc4), high_word);
        c.put32(frame.at(0xc0), idle_now);
        c.put32(frame.at(0xbc), high_word);
        c.put32(frame.at(0xb8), sum ^ 0x80000000u);
        c.put32(frame.at(0xb4), high_word);
        c.put32(frame.at(0xc8), rem6 ^ 0x80000000u);
        const double f5 = c.f64(pool + 0x38), f0_in = c.f64(pool + 0x180), f3_in = c.f64(pool + 0x1c8), f2 = c.f64(pool + 0x198);
        double f1 = c.f64(frame.at(0xbc));
        const double f3 = fpu.fsub(f1, f3_in);
        double f4 = c.f64(frame.at(0xb4));
        f4 = fpu.fsub(f4, f5);
        f1 = c.f64(frame.at(0xc4));
        f1 = fpu.fsub(f1, f5);
        double f0 = fpu.fdiv(f1, f0_in);
        f0 = fpu.fadd(f2, f0);
        f0 = fpu.fmul(f3, f0);
        c.set({{3, sum ^ 0x80000000u}, {4, word(x)}, {5, cursor}, {6, sum}, {7, high_word}, {8, c.u8(r25 + 0x6c)}});
        c.set_f(0, f0);
        c.set_f(1, f1);
        c.set_f(2, f2);
        c.set_f(3, f3);
        c.set_f(4, f4);
        c.set_f(5, f5);
        if (!(fcmp(f4, f0) & 8)) return false;
    }
    x = draw(0x101a5274);
    r15 = word(rem(x, 4));
    c.set({{3, word(x) >> 31}, {4, (word(x) & 3u) << 30}, {5, word(x)}});
    x = draw(0x101a52bc);
    {
        const auto b76 = std::int32_t(c.u8(r25 + 0x76));
        const auto half = half_of(r13);
        const auto rem4 = rem(x, 4);
        c.set({{3, c.u32(t.cursor_cell)}, {4, word(rem4)}, {5, word(x) >> 31}, {6, word(half)}});
        const auto want = std::int32_t(word(b76) + r15 - word(rem4));
        compare(c, half, want);
        if (half != want) return false;
    }
    {
        const auto b3a = std::int32_t(c.u8(frame.at(0x3a)));
        const auto hi = mulhw(0x66666667, std::int32_t(r13));
        const auto q = hi >> 1;
        const auto tenth = std::int32_t(word(q) + (word(q) >> 31));
        c.set({{4, word(q) >> 31}, {5, word(b3a)}});
        compare(c, b3a, tenth);
        if (b3a <= tenth) return false;
    }
    x = draw(0x101a5348);
    r15 = word(rem(x, 2));
    c.set({{5, c.u16(c.u32(t.cursor_cell))}, {3, word(x) >> 31}, {4, word(x)}});
    const auto xd = draw(0x101a538c);
    {
        const auto cursor = c.u32(t.cursor_cell);
        const auto league = c.u32(club + club::league);
        const auto table = c.u32(c.r(18));
        const auto record = c.u32(table + (c.u32(league) << 2));
        const auto c3 = sbyte(c.u8(record + 0xc3));
        const auto b38 = std::int32_t(c.u8(frame.at(0x38)));
        const auto rem_d = rem(xd, 2);
        const auto lhs = std::int32_t(word(b38) + r15 - word(rem_d));
        const auto rhs = std::int32_t(word(r13) - word(c3));
        c.set({{3, cursor}, {4, word(lhs)}, {5, word(rem_d)}, {6, word(xd) & 1u}, {7, word(xd) >> 31}});
        compare(c, lhs, rhs);
        if (lhs >= rhs) return false;
    }
    return final_hire(0x101a5404, 0x101a5440, 0x101a546c, 0x101a5490, 0x43);
}

// ---- s63 (0x101a5498) and s61 (0x101a576c): %10, %5 against the idle byte, bytes +0x76/+0x75, the stats
// bytes, then one or two draws against +0x1c / +0x44 ----
struct Mid {
    std::uint32_t back1, back2, back3, back4, bl_draw, bl_index, bl_any, bl_post, code;
    bool narrow;  // +0x75 in (3,6) (s63); else > 5 (s61)
};
bool mid(const Mid& m) {
    std::int32_t w;
    if (!chain(w) || w != -1) return false;
    auto x = draw(m.back1);
    auto idle = std::int32_t(c.u8(r25 + 4));
    auto r = rem(x, 10);
    c.set({{5, word(x)}, {6, word(idle)}, {4, word(r)}, {0, word(r + 10)}});
    compare(c, idle, r + 10);
    if (idle < r + 10) return false;
    x = draw(m.back2);
    idle = std::int32_t(c.u8(r25 + 4));
    r = rem(x, 5);
    c.set({{5, word(x)}, {6, word(idle)}, {4, word(r)}, {0, word(r + 0x19)}});
    compare(c, idle, r + 0x19);
    if (idle >= r + 0x19) return false;
    const auto b76 = c.u8(r25 + 0x76);
    c.set(0, b76);
    compare_logical(c, b76, 4);
    if (b76 >= 4) return false;
    compare_logical(c, b76, 1);
    if (b76 <= 1) return false;
    const auto b75 = c.u8(r25 + 0x75);
    c.set(0, b75);
    compare_logical(c, b75, m.narrow ? 3 : 5);
    if (b75 <= (m.narrow ? 3u : 5u)) return false;
    if (m.narrow) {
        compare_logical(c, b75, 6);
        if (b75 >= 6) return false;
    }
    const auto b3a = c.u8(frame.at(0x3a));
    c.set(0, b3a);
    compare_logical(c, b3a, 1);
    if (b3a != 1) return false;
    const auto b38 = c.u8(frame.at(0x38));
    c.set(0, b38);
    compare_logical(c, b38, 1);
    if (b38 == 1) return false;
    x = draw(m.back3);
    auto w1c = c.i32(r25 + 0x1c);
    r = rem(x, 5);
    c.set({{5, word(x)}, {6, word(w1c)}, {4, word(r)}, {0, word(r + 2)}});
    compare(c, w1c, r + 2);
    if (!(w1c > r + 2)) {
        x = draw(m.back4);
        const auto w44 = c.i32(r25 + 0x44);
        r = rem(x, 5);
        c.set({{3, word(r)}, {4, word(x)}, {5, word(w44)}, {0, word(r + 4)}});
        compare(c, w44, r + 4);
        if (w44 <= r + 4) return false;
    }
    return final_hire(m.bl_draw + 4, m.bl_index, m.bl_any, m.bl_post, m.code);
}
bool s63() { return mid({0x101a5504, 0x101a5568, 0x101a560c, 0x101a5670, 0x101a56d4, 0x101a5714, 0x101a5740, 0x101a5764, 0x63, true}); }
bool s61() { return mid({0x101a57d8, 0x101a583c, 0x101a58d8, 0x101a593c, 0x101a59a0, 0x101a59e0, 0x101a5a0c, 0x101a5a30, 0x61, false}); }

// The draws s62, s40 and s41 end with: %3 against a count +2 (else %4 against another count +4), then the parity draw.
bool tri_tail(std::uint32_t off1, std::uint32_t off2, std::uint32_t back2, std::uint32_t back3, std::uint32_t bl_draw,
              std::uint32_t bl_index, std::uint32_t bl_any, std::uint32_t bl_post, std::uint32_t code) {
    auto x = draw(back2);
    const auto cursor = c.u32(t.cursor_cell);
    const auto w = c.i32(r25 + off1);
    auto r = rem(x, 3);
    c.set({{3, word(r)}, {4, word(x)}, {5, cursor}, {6, word(w)}, {0, word(r + 2)}});
    compare(c, w, r + 2);
    if (!(w > r + 2)) {
        x = draw(back3);
        const auto w2 = c.i32(r25 + off2);
        r = rem(x, 4);
        c.set({{3, word(r)}, {4, word(w2)}, {0, word(r + 4)}});
        compare(c, w2, r + 4);
        if (w2 <= r + 4) return false;
    }
    return final_hire(bl_draw + 4, bl_index, bl_any, bl_post, code);
}

// ---- 0x101a5a38..0x101a5c90 ----
bool s62() {
    std::int32_t w;
    if (!chain(w) || w != -1) return false;
    auto x = draw(0x101a5aa4);
    const auto idle = std::int32_t(c.u8(r25 + 4));
    auto r = rem(x, 10);
    c.set({{5, word(x)}, {6, word(idle)}, {4, word(r)}, {0, word(r + 10)}});
    compare(c, idle, r + 10);
    if (idle < r + 10) return false;
    compare_logical(c, word(idle), 0x1e);
    if (idle >= 0x1e) return false;
    const auto b76 = c.u8(r25 + 0x76);
    c.set(0, b76);
    compare_logical(c, b76, 1);
    if (b76 != 1) return false;
    const auto b75 = c.u8(r25 + 0x75);
    c.set(0, b75);
    compare_logical(c, b75, 1);
    if (b75 != 1) return false;
    if (!stats_bytes()) return false;
    return tri_tail(0x1c, 0x44, 0x101a5b40, 0x101a5ba0, 0x101a5bf8, 0x101a5c38, 0x101a5c64, 0x101a5c88, 0x62);
}

// byte 0x3a == 1 and byte 0x38 != 1 (the stats call's outputs), or false.
bool stats_bytes() {
    const auto b3a = c.u8(frame.at(0x3a));
    c.set(0, b3a);
    compare_logical(c, b3a, 1);
    if (b3a != 1) return false;
    const auto b38 = c.u8(frame.at(0x38));
    c.set(0, b38);
    compare_logical(c, b38, 1);
    return b38 != 1;
}

// ---- 0x101a5c90..0x101a5f10 ----
bool s40() {
    std::int32_t w;
    if (!chain(w) || w == -1) return false;
    auto x = draw(0x101a5cfc);
    const auto idle = std::int32_t(c.u8(r25 + 4));
    auto r = rem(x, 10);
    c.set({{5, word(x)}, {6, word(idle)}, {4, word(r)}, {0, word(r + 0x19)}});
    compare(c, idle, r + 0x19);
    if (idle < r + 0x19) return false;
    const auto b76 = c.u8(r25 + 0x76);
    c.set(0, b76);
    compare_logical(c, b76, 9);
    if (b76 >= 9) return false;
    compare_logical(c, b76, 1);
    if (b76 <= 1) return false;
    const auto b75 = c.u8(r25 + 0x75);
    c.set(0, b75);
    compare_logical(c, b75, 7);
    if (b75 >= 7) return false;
    {
        const auto league = c.u32(club + club::league);
        const auto table = c.u32(c.r(18));
        const auto record = c.u32(table + (c.u32(league) << 2));
        const auto b3a = std::int32_t(c.u8(frame.at(0x3a)));
        const auto sum = sbyte(c.u8(record + 0xc0)) + sbyte(c.u8(record + 0xc1));
        c.set({{4, word(sum)}, {5, record}, {6, word(b3a)}});
        compare(c, b3a, sum);
        if (b3a > sum) return false;
        const auto b38 = std::int32_t(c.u8(frame.at(0x38)));
        c.set(0, word(b38));
        compare(c, b38, sum);
        if (b38 <= sum) return false;
    }
    return tri_tail(0x30, 0x58, 0x101a5dc0, 0x101a5e20, 0x101a5e78, 0x101a5eb8, 0x101a5ee4, 0x101a5f08, 0x40);
}

// ---- 0x101a5f10..0x101a6178 ----
bool s41() {
    std::int32_t w;
    if (!chain(w) || w != -1) return false;
    auto x = draw(0x101a5f7c);
    const auto idle = std::int32_t(c.u8(r25 + 4));
    auto r = rem(x, 10);
    c.set({{3, word(r)}, {4, word(x)}, {5, word(idle)}, {0, word(r + 0x19)}});
    compare(c, idle, r + 0x19);
    if (idle < r + 0x19) return false;
    const auto b76 = c.u8(r25 + 0x76);
    c.set(0, b76);
    compare_logical(c, b76, 6);
    if (b76 >= 6) return false;
    const auto b75 = c.u8(r25 + 0x75);
    c.set(0, b75);
    compare_logical(c, b75, 5);
    if (b75 >= 5) return false;
    const auto b39 = c.u8(frame.at(0x39));
    c.set(0, b39);
    compare_logical(c, b39, 2);
    if (b39 != 2) {
        compare_logical(c, b39, 3);
        if (b39 != 3) return false;
    }
    if (!stats_bytes()) return false;
    return tri_tail(0x30, 0x58, 0x101a6028, 0x101a6088, 0x101a60e0, 0x101a6120, 0x101a614c, 0x101a6170, 0x41);
}

// ---- 0x101a6178..0x101a6350: event 0x5e, no hire ----
bool s5e() {
    if (!guard_low()) return false;
    auto x = draw(0x101a61c8);
    const auto rep = std::int32_t(c.i16(club + club::reputation));
    auto r = rem(x, 2000);
    c.set({{5, word(x)}, {6, word(rep)}, {4, word(r)}});
    const auto limit = std::int32_t(std::int16_t(word(r + 0x1c52)));
    compare(c, rep, limit);
    if (rep >= limit) return false;
    x = draw(0x101a6230);
    auto idle = std::int32_t(c.u8(r25 + 4));
    r = rem(x, 10);
    c.set({{5, word(x)}, {6, word(idle)}, {4, word(r)}, {0, word(r + 5)}});
    compare(c, idle, r + 5);
    if (idle <= r + 5) return false;
    x = draw(0x101a6294);
    idle = std::int32_t(c.u8(r25 + 4));
    r = rem(x, 5);
    c.set({{3, quotient_sign(0x66666667, 1, x)}, {4, word(x)}, {5, word(idle)}, {0, word(25 - r)}});
    compare(c, idle, 25 - r);
    if (idle >= 25 - r) return false;
    const auto b76 = c.u8(r25 + 0x76), b75 = c.u8(r25 + 0x75);
    c.set({{3, b76}, {0, b75}});
    compare_logical(c, b76, b75);
    if (b76 >= b75) return false;
    compare_logical(c, b76, 5);
    if (b76 >= 5) return false;
    if (!stats_bytes()) return false;
    auto w = c.i32(r25 + 0x1c);
    c.set(0, word(w));
    if (compare(c, w, 0) == 0) return false;
    if (compare(c, w, 4) <= 0) {
        w = c.i32(r25 + 0x44);
        c.set(0, word(w));
        compare(c, w, 7);
        if (w <= 7) return false;
    }
    post_with(0, 0x5e, 0xffffffffu, 0x101a6348);
    return true;
}

// ---- 0x101a6350..0x101a6504: event 0x3e, no hire ----
bool s3e() {
    if (!guard_low()) return false;
    auto b76 = c.u8(r25 + 0x76);
    auto b75 = c.u8(r25 + 0x75);
    c.set({{3, b76}, {0, b75}});
    compare_logical(c, b76, b75);
    if (b76 <= b75) return false;
    auto x = draw(0x101a63b0);
    auto idle = std::int32_t(c.u8(r25 + 4));
    auto r = rem(x, 10);
    c.set({{5, word(x)}, {6, word(idle)}, {4, word(r)}, {0, word(r + 5)}});
    compare(c, idle, r + 5);
    if (idle <= r + 5) return false;
    x = draw(0x101a6414);
    idle = std::int32_t(c.u8(r25 + 4));
    r = rem(x, 5);
    c.set({{3, quotient_sign(0x66666667, 1, x)}, {4, word(x)}, {5, word(idle)}, {0, word(25 - r)}});
    compare(c, idle, 25 - r);
    if (idle >= 25 - r) return false;
    auto w = c.i32(r25 + 0x30);
    c.set(0, word(w));
    if (compare(c, w, 0) == 0) return false;
    if (compare(c, w, 4) <= 0) {
        w = c.i32(r25 + 0x58);
        c.set(0, word(w));
        compare(c, w, 7);
        if (w <= 7) return false;
    }
    if (!league_virtual(0x101a649c)) return false;
    const auto league = c.u32(club + club::league);
    const auto table = c.u32(c.r(18));
    const auto record = c.u32(table + (c.u32(league) << 2));
    b76 = c.u8(r25 + 0x76);
    const auto gap = std::int32_t(word(r13) - word(sbyte(c.u8(record + 0xc3))));
    c.set({{3, word(gap)}, {4, table}, {5, b76}});
    compare(c, std::int32_t(b76), gap);
    if (std::int32_t(b76) < gap) return false;
    const auto b3a = std::int32_t(c.u8(frame.at(0x3a)));
    c.set(0, word(b3a));
    compare(c, b3a, gap);
    if (b3a >= gap) return false;
    post_with(0, 0x3e, 0xffffffffu, 0x101a64fc);
    return true;
}

// ---- 0x101a6504..0x101a6790: event 0x3f with a hire ----
bool s3f() {
    if (!guard_low()) return false;
    auto x = draw(0x101a6554);
    const auto idle = std::int32_t(c.u8(r25 + 4));
    auto r = rem(x, 15);
    c.set({{5, word(x)}, {6, word(idle)}, {4, word(r)}, {0, word(r + 15)}});
    compare(c, idle, r + 15);
    if (idle <= r + 15) return false;
    auto w = c.i32(r25 + 0x30);
    c.set(0, word(w));
    if (compare(c, w, 1) <= 0) {
        x = draw(0x101a65c8);
        const auto w58 = c.i32(r25 + 0x58);
        r = rem(x, 3);
        c.set({{3, word(r)}, {4, word(x)}, {5, word(w58)}, {0, word(r + 2)}});
        compare(c, w58, r + 2);
        if (w58 <= r + 2) return false;
    }
    if (!league_virtual(0x101a662c)) return false;
    x = draw(0x101a6658);
    std::int32_t gap;
    {
        const auto cursor = c.u32(t.cursor_cell);
        const auto league = c.u32(club + club::league);
        const auto table = c.u32(c.r(18));
        const auto record = c.u32(table + (c.u32(league) << 2));
        const auto b76 = std::int32_t(c.u8(r25 + 0x76));
        const auto c3 = sbyte(c.u8(record + 0xc3));
        gap = std::int32_t(word(r13) - word(c3));
        const auto lhs = std::int32_t(word(b76) + word(rem(x, 2)));
        c.set({{3, cursor}, {4, word(lhs)}, {5, word(gap)}, {6, word(x)}, {7, word(b76)}});
        compare(c, lhs, gap);
        if (lhs <= gap) return false;
    }
    const auto b39 = std::int32_t(c.u8(frame.at(0x39)));
    c.set(0, word(b39));
    compare(c, b39, gap);
    if (b39 < gap) return false;
    const auto b3a = std::int32_t(c.u8(frame.at(0x3a)));
    c.set(4, word(b3a));
    compare(c, b3a, gap);
    if (b3a >= gap) return false;
    const auto limit = std::int32_t(word(half_of(r13)) + word(gap));
    c.set(0, word(limit));
    compare(c, b3a, limit);
    if (b3a <= limit) return false;
    return final_hire(0x101a66fc, 0x101a6738, 0x101a6764, 0x101a6788, 0x3f);
}

// ---- 0x101a6790..0x101a6958: the staff record idle for more than 20 and +0x76 more than five above +0x75: a
// manager test (0x100d94d0), the row's target below 0x10b3, the club's word +0x14 == -1 and two draws may post
// event 0x3c and lower the row's +0x16 by 250. Every path ends at 0x101a6958. ----
void s6790() {
    const auto idle = c.u8(r25 + 4);
    c.set(0, idle);
    compare_logical(c, idle, 0x14);
    if (idle <= 0x14) return;
    const auto b76 = c.u8(r25 + 0x76), b75 = c.u8(r25 + 0x75);
    const auto diff = std::int32_t(std::int16_t(b75 - b76));
    c.set({{3, b75}, {0, word(diff)}});
    compare(c, diff, -5);
    if (diff >= -5) return;
    const auto b3a = c.u8(frame.at(0x3a));
    c.set(0, b3a);
    compare_logical(c, b3a, b75);
    if (b3a <= b75) return;
    c.set({{3, c.u32(club + club::manager)}, {4, club}});
    c.call(0x100d94d0, 0x101a67cc);
    const auto answer = sbyte(c.r(3));
    c.set(0, word(answer));
    if (!nonzero(c, answer)) {
        const auto x = draw(0x101a67f4);
        const auto r = rem(x, 4);
        c.set({{3, word(x) >> 31}, {0, word(r)}});
        if (compare(c, r) != 0) return;
    }
    std::int32_t target = 0;
    compare_logical(c, club, 0);
    if (club != 0) {
        const auto rows = c.u32(object);
        const auto id = c.u32(club);
        c.set({{4, rows}, {3, id * 0x4c}});
        target = c.i16(rows + id * 0x4c + 8);
    }
    c.set(0, word(target));
    compare(c, target, 0x10b3);
    if (target >= 0x10b3) return;
    const auto link = c.i32(club + 0x14);
    c.set(0, word(link));
    if (compare(c, link, -1) != 0) return;
    auto x = draw(0x101a6880);
    {
        const auto w30 = c.i32(r25 + 0x30);
        const auto r = rem(x, 4);
        c.set({{3, word(r)}, {4, c.u32(t.cursor_cell)}, {5, word(w30)}, {0, word(r + 2)}});
        compare(c, w30, r + 2);
        if (!(w30 > r + 2)) {
            x = draw(0x101a68d8);
            const auto w58 = c.i32(r25 + 0x58);
            const auto r7 = rem(x, 7);
            c.set({{3, word(r7)}, {4, word(x)}, {5, word(w58)}, {0, word(r7 + 3)}});
            compare(c, w58, r7 + 3);
            if (w58 <= r7 + 3) return;
        }
    }
    post_with(0, 0x3c, 0xffffffffu, 0x101a6940);
    const auto rows = c.u32(object);
    const auto at = row_offset + 0x16;
    const auto value = c.i16(rows + at);
    c.set({{5, rows}, {4, at}, {3, word(std::int32_t(value))}, {0, word(std::int32_t(value)) - 0xfa}});
    c.put16(rows + at, word(std::int32_t(value)) - 0xfa);
}
