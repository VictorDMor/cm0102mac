// Part E of the weekly pass (included inside class Club, after part D): 0x101a7f7c..0x101a8a78, the end of a club's
// body: the change r15 scaled by the reputation, the three standing values of the manager's record (+0x28, +0x2a,
// +0x2c) moved by it and cut into bands, the club's flag, the switch over 0x100f55b0 and the row clamp at 0x101a8a48.
// The registers r0..r12 are kept in an array `g` between the original's calls (each call and the end store them, as the
// original's instructions leave them); the floating-point registers in `fr`.

static std::uint32_t s16(std::uint32_t v) { return word(std::int32_t(std::int16_t(v))); }
static std::uint32_t sb8(std::uint32_t v) { return word(sbyte(v)); }
static std::int32_t i32(std::uint32_t v) { return std::int32_t(v); }
static std::uint32_t divide(std::uint32_t a, std::uint32_t b) { return word(std::int32_t(a) / std::int32_t(b)); }
// ---- 0x101a7f7c..0x101a800c: a negative change is scaled by the reputation (above 5000) or by a constant. ----
void scale_change() {
    const auto change = std::int32_t(std::int16_t(r15));
    c.set(0, word(change));
    compare(c, change);
    if (change >= 0) return;
    const auto reputation = std::int32_t(c.i16(club + club::reputation));
    c.set(0, word(reputation));
    compare(c, reputation, 0x1388);
    if (reputation > 0x1388) {
        c.set(3, high_word);
        c.set(0, word(reputation) ^ 0x80000000u);
        c.put32(frame.at(0xc0), c.r(0));
        c.set(0, std::uint32_t(r15) ^ 0x80000000u);
        double f2 = pool_f(0x38);
        c.put32(frame.at(0xbc), c.r(3));
        double f1 = pool_f(0x18);
        double f0 = c.f64(frame.at(0xbc));
        c.put32(frame.at(0xb8), c.r(0));
        f0 = fpu.fsub(f0, f2);
        c.put32(frame.at(0xb4), c.r(3));
        f0 = fpu.fdiv(f1, f0);
        f1 = c.f64(frame.at(0xb4));
        f1 = fpu.fsub(f1, f2);
        f0 = fpu.fmul(f1, f0);
        c.put64(frame.at(0xc4), fpu.fctiwz(0, f0));
        r15 = c.u32(frame.at(0xc8));
        c.set_f(1, f1);
        c.set_f(2, f2);
    } else {
        const auto r3 = std::uint32_t(r15) ^ 0x80000000u;
        c.set(3, r3);
        c.set(0, high_word);
        c.put32(frame.at(0xb8), r3);
        double f1 = pool_f(0x38);
        c.put32(frame.at(0xb4), high_word);
        double f2 = pool_f(0x20);
        double f0 = c.f64(frame.at(0xb4));
        f0 = fpu.fsub(f0, f1);
        f0 = fpu.fmul(f2, f0);
        c.put64(frame.at(0xbc), fpu.fctiwz(0, f0));
        r15 = c.u32(frame.at(0xc0));
        c.set_f(1, f1);
        c.set_f(2, f2);
    }
}

void load(std::uint32_t (&g)[13]) const {
    for (unsigned i = 0; i < 13; ++i) g[i] = c.r(i);
}
void store(const std::uint32_t (&g)[13]) {
    for (unsigned i = 0; i < 13; ++i) c.set(i, g[i]);
}

// ---- 0x101a800c..0x101a86b0, then the call at 0x101a86b0 returns: the three values of the manager's record. ----
// Returns true when the club's flag (0x100c7d00) ends the club.
bool standings() {
    std::uint32_t g[13];
    load(g);
    constexpr std::uint32_t sign = 0x80000000u;
    // 0x101a800c: the change added to each value; r15, r25, r27 hold the sums as halfwords.
    g[0] = word(std::int32_t(c.i16(r14 + 0x2a)));
    g[8] = s16(r15);
    g[3] = word(std::int32_t(c.i16(club + club::reputation)));
    g[6] = g[0] + g[8];
    g[4] = word(std::int32_t(c.i16(r14 + 0x28)));
    g[5] = word(std::int32_t(c.i16(r14 + 0x2c)));
    r15 = s16(g[6]);
    g[9] = g[3] + 0xbb8;
    g[7] = g[4] + g[8];
    g[6] = g[5] + g[8];
    compare(c, i32(r15), i32(g[9]));
    r25 = s16(g[7]);
    r27 = s16(g[6]);
    // The first two sums are held between the reputation -+ 0xbb8 (at most 0x2710, at least 1).
    const auto band = [&](auto& value) {
        if (i32(value) > i32(g[9])) {
            compare(c, i32(g[9]), 0x2710);
            g[6] = 0x2710;
            if (!(i32(g[9]) > 0x2710)) g[6] = g[9];
            value = s16(g[6]);
        } else {
            g[7] = g[3] - 0xbb8;
            compare(c, i32(value), i32(g[7]));
            if (i32(value) < i32(g[7])) {
                compare(c, i32(g[7]), 1);
                g[6] = 1;
                if (!(i32(g[7]) < 1)) g[6] = g[7];
                value = s16(g[6]);
            }
        }
    };
    band(r15);
    compare(c, i32(r25), i32(g[9]));
    band(r25);
    g[6] = g[3] + 0x3e8;
    compare(c, i32(r27), i32(g[6]));
    if (i32(r27) > i32(g[6])) {
        compare(c, i32(g[6]), 0x2710);
        g[3] = 0x2710;
        if (!(i32(g[6]) > 0x2710)) g[3] = g[6];
        r27 = s16(g[3]);
    } else {
        g[6] = g[3] - 0x1388;
        compare(c, i32(r27), i32(g[6]));
        if (i32(r27) < i32(g[6])) {
            compare(c, i32(g[6]), 1);
            g[3] = 1;
            if (!(i32(g[6]) < 1)) g[3] = g[6];
            r27 = s16(g[3]);
        }
    }
    // 0x101a8100: each value over 1000, plus 16, as a byte, at least 0x16 (r0, r28, r29).
    g[3] = 0x10624dd3;
    g[0] = word(mulhw(g[3], i32(g[0])));
    g[0] = word(i32(g[0]) >> 6);
    g[3] = g[0] >> 31;
    g[3] = g[0] + g[3];
    g[0] = g[3] + 0x10;
    g[0] = sb8(g[0]);
    compare(c, i32(g[0]), 0x16);
    if (!(i32(g[0]) >= 0x16)) g[0] = 0x16;
    g[3] = 0x10624dd3;
    g[3] = word(mulhw(g[3], i32(g[4])));
    g[3] = word(i32(g[3]) >> 6);
    g[4] = g[3] >> 31;
    g[3] = g[3] + g[4];
    g[3] = g[3] + 0x10;
    r28 = sb8(g[3]);
    compare(c, i32(r28), 0x16);
    if (!(i32(r28) >= 0x16)) r28 = 0x16;
    g[3] = 0x10624dd3;
    g[3] = word(mulhw(g[3], i32(g[5])));
    g[3] = word(i32(g[3]) >> 6);
    g[4] = g[3] >> 31;
    g[3] = g[3] + g[4];
    g[3] = g[3] + 0x10;
    c.set(29, sb8(g[3]));
    compare(c, sbyte(c.r(29)), 0x16);
    if (!(sbyte(c.r(29)) >= 0x16)) c.set(29, 0x16);
    g[3] = sb8(g[0]);
    g[0] = g[3] - 0x13;
    c.set(30, s16(g[0]));
    compare(c, i32(c.r(30)));
    if (i32(c.r(30)) == 0) c.set(30, 1);
    // 0x101a8198: the club's flag (0x100c7d00) shifts the second and third value's base.
    g[3] = club;
    store(g);
    c.call(callee::club_flag, 0x101a81a0);
    load(g);
    g[0] = sb8(g[3]);
    compare(c, sbyte(g[3]));
    bool shifted = true;
    if (sbyte(g[3]) == 0) {
        g[3] = c.u32(frame.at(slot::record));
        g[4] = c.u32(club + club::region);
        g[0] = c.u32(g[3] + 0xc);
        compare_logical(c, g[4], g[0]);
        shifted = g[4] == g[0];
    }
    if (shifted) {
        g[3] = sb8(r28);
        g[0] = g[3] - 0x13;
        r28 = s16(g[0]);
    } else {
        g[3] = 0x10624dd3;
        g[0] = word(std::int32_t(c.i16(club + club::reputation)));
        g[3] = word(mulhw(g[3], i32(g[0])));
        g[0] = word(i32(g[3]) >> 5);
        g[3] = g[0] >> 31;
        g[0] = g[0] + g[3];
        g[0] = 0x17u - g[0];
        r28 = s16(g[0]);
    }
    g[0] = s16(r28);
    compare(c, i32(g[0]));
    if (i32(g[0]) == 0) r28 = 1;
    g[3] = club;
    store(g);
    c.call(callee::club_flag, 0x101a8204);
    load(g);
    g[0] = sb8(g[3]);
    compare(c, sbyte(g[3]));
    if (sbyte(g[3]) != 0) {
        g[3] = sb8(c.r(29));
        g[0] = g[3] - 0x13;
        g[5] = s16(g[0]);
    } else {
        g[3] = 0x10624dd3;
        g[0] = word(std::int32_t(c.i16(club + club::reputation)));
        g[3] = word(mulhw(g[3], i32(g[0])));
        g[0] = word(i32(g[3]) >> 5);
        g[3] = g[0] >> 31;
        g[0] = g[0] + g[3];
        g[0] = 0x17u - g[0];
        g[5] = s16(g[0]);
    }
    g[0] = s16(g[5]);
    compare(c, i32(g[0]));
    if (i32(g[0]) == 0) g[5] = 1;
    // 0x101a824c: the three values scaled in floating point; fr[n] is f<n>.
    double fr[8];
    for (unsigned i = 0; i < 8; ++i) fr[i] = c.f(i);
    const auto ctiwz = [&](unsigned n, double v) {
        const auto bits = fpu.fctiwz(n, v);
        fr[n] = std::bit_cast<double>(bits);
        return bits;
    };
    const auto fc = [&](double a, double b) { return fcmp(a, b); };
    g[6] = word(std::int32_t(c.i16(r14 + 0x2a)));
    g[10] = high_word;
    g[7] = word(std::int32_t(c.i16(r14 + 0x28)));
    g[3] = s16(c.r(30));
    g[11] = g[6] ^ sign;
    g[8] = word(std::int32_t(c.i16(r14 + 0x2c)));
    c.put32(frame.at(0xb8), g[11]);
    g[9] = g[7] ^ sign;
    c.set(29, g[8] ^ sign);
    g[0] = g[3] - 1;
    c.put32(frame.at(0xb4), g[10]);
    g[4] = s16(r28);
    fr[6] = pool_f(0x38);
    g[5] = s16(g[5]);
    fr[0] = c.f64(frame.at(0xb4));
    r28 = g[4] - 1;
    fr[3] = pool_f(0x1b8);
    g[12] = g[5] - 1;
    fr[0] = fpu.fsub(fr[0], fr[6]);
    c.put32(frame.at(0xc0), g[9]);
    g[0] = g[0] * g[6];
    g[9] = s16(r15);
    c.put32(frame.at(0xbc), g[10]);
    r15 = s16(r25);
    fr[1] = c.f64(frame.at(0xbc));
    fr[5] = fpu.fdiv(fr[0], fr[3]);
    c.put32(frame.at(0xc8), c.r(29));
    g[0] = g[9] + g[0];
    g[0] = divide(g[0], g[3]);
    fr[7] = pool_f(0x98);
    c.put32(frame.at(0xc4), g[10]);
    fr[0] = c.f64(frame.at(0xc4));
    fr[1] = fpu.fsub(fr[1], fr[6]);
    r25 = std::uint32_t(r28) * g[7];
    g[0] = s16(g[0]);
    fr[0] = fpu.fsub(fr[0], fr[6]);
    g[9] = s16(r27);
    fr[2] = fpu.fdiv(fr[1], fr[3]);
    g[0] = g[0] ^ sign;
    g[3] = g[12] * g[8];
    g[12] = std::uint32_t(r15) + std::uint32_t(r25);
    c.put32(frame.at(0xd0), g[11]);
    fr[4] = news_f(0x38);
    fr[1] = fpu.fdiv(fr[0], fr[3]);
    g[3] = g[9] + g[3];
    g[4] = divide(g[12], g[4]);
    c.put32(frame.at(0xcc), g[10]);
    fr[3] = c.f64(frame.at(0xcc));
    c.put32(frame.at(0xd8), g[0]);
    g[3] = divide(g[3], g[5]);
    fr[5] = fpu.fsub(fr[7], fr[5]);
    c.put32(frame.at(0xd4), g[10]);
    fr[0] = fpu.fsub(fr[7], fr[2]);
    fr[1] = fpu.fsub(fr[7], fr[1]);
    g[9] = s16(g[4]);
    fr[2] = c.f64(frame.at(0xd4));
    fr[5] = fpu.frsp(fr[5]);
    fr[3] = fpu.fsubs(fr[3], fr[6]);
    g[12] = s16(g[3]);
    fr[2] = fpu.fsubs(fr[2], fr[6]);
    fr[0] = fpu.frsp(fr[0]);
    fr[3] = fpu.multiply_add(0x101a8340, fr[4], fr[5], fr[3]);
    fr[1] = fpu.frsp(fr[1]);
    if (!(fc(fr[3], fr[2]) & 4)) {  // 0x101a8354
        c.put32(frame.at(0xb8), g[11]);
        fr[4] = news_f(0x3c);
        c.put32(frame.at(0xb4), g[10]);
        fr[2] = c.f64(frame.at(0xb4));
        c.put32(frame.at(0xc0), g[0]);
        fr[3] = fpu.fsubs(fr[2], fr[6]);
        c.put32(frame.at(0xbc), g[10]);
        fr[2] = c.f64(frame.at(0xbc));
        fr[3] = fpu.multiply_add(0x101a8374, fr[4], fr[5], fr[3]);
        fr[2] = fpu.fsubs(fr[2], fr[6]);
        if (!(fc(fr[3], fr[2]) & 8)) {
            c.put32(frame.at(0xc8), g[0]);
            c.put32(frame.at(0xc4), g[10]);
            fr[2] = c.f64(frame.at(0xc4));
            fr[3] = fpu.fsubs(fr[2], fr[6]);
        }
    }
    // 0x101a8398
    g[4] = high_word;
    g[5] = g[7] ^ sign;
    c.put32(frame.at(0xc0), g[5]);
    g[3] = g[9] ^ sign;
    const auto first = ctiwz(6, fr[3]);
    fr[5] = pool_f(0x38);
    c.put32(frame.at(0xbc), g[4]);
    fr[4] = news_f(0x38);
    fr[2] = c.f64(frame.at(0xbc));
    c.put32(frame.at(0xc8), g[3]);
    fr[3] = fpu.fsubs(fr[2], fr[5]);
    c.put32(frame.at(0xc4), g[4]);
    fr[2] = c.f64(frame.at(0xc4));
    fr[3] = fpu.multiply_add(0x101a83cc, fr[4], fr[0], fr[3]);
    c.put64(frame.at(0xb4), first);
    fr[2] = fpu.fsubs(fr[2], fr[5]);
    g[0] = c.u32(frame.at(0xb8));
    if (!(fc(fr[3], fr[2]) & 4)) {  // 0x101a83e8
        c.put32(frame.at(0xb8), g[5]);
        fr[4] = news_f(0x40);
        c.put32(frame.at(0xb4), g[4]);
        fr[2] = c.f64(frame.at(0xb4));
        c.put32(frame.at(0xc0), g[3]);
        fr[3] = fpu.fsubs(fr[2], fr[5]);
        c.put32(frame.at(0xbc), g[4]);
        fr[2] = c.f64(frame.at(0xbc));
        fr[3] = fpu.multiply_add(0x101a8408, fr[4], fr[0], fr[3]);
        fr[0] = fpu.fsubs(fr[2], fr[5]);
        if (!(fc(fr[3], fr[0]) & 8)) {
            c.put32(frame.at(0xc8), g[3]);
            c.put32(frame.at(0xc4), g[4]);
            fr[0] = c.f64(frame.at(0xc4));
            fr[3] = fpu.fsubs(fr[0], fr[5]);
        }
    }
    // 0x101a842c
    g[5] = high_word;
    g[9] = g[8] ^ sign;
    c.put32(frame.at(0xc0), g[9]);
    g[4] = g[12] ^ sign;
    fr[0] = news_f(0xc);
    const auto second = ctiwz(3, fr[3]);
    c.put32(frame.at(0xbc), g[5]);
    fr[2] = pool_f(0x38);
    fr[4] = fpu.fmuls(fr[0], fr[1]);
    fr[0] = c.f64(frame.at(0xbc));
    c.put32(frame.at(0xc8), g[4]);
    fr[1] = fpu.fsubs(fr[0], fr[2]);
    c.put32(frame.at(0xc4), g[5]);
    fr[0] = c.f64(frame.at(0xc4));
    fr[1] = fpu.fsubs(fr[1], fr[4]);
    c.put64(frame.at(0xb4), second);
    fr[0] = fpu.fsubs(fr[0], fr[2]);
    g[3] = c.u32(frame.at(0xb8));
    if (!(fc(fr[1], fr[0]) & 4)) {  // 0x101a8480
        c.put32(frame.at(0xb8), g[9]);
        c.put32(frame.at(0xb4), g[5]);
        fr[0] = c.f64(frame.at(0xb4));
        c.put32(frame.at(0xc0), g[4]);
        fr[1] = fpu.fsubs(fr[0], fr[2]);
        c.put32(frame.at(0xbc), g[5]);
        fr[0] = c.f64(frame.at(0xbc));
        fr[1] = fpu.fadds(fr[1], fr[4]);
        fr[0] = fpu.fsubs(fr[0], fr[2]);
        if (!(fc(fr[1], fr[0]) & 8)) {
            c.put32(frame.at(0xc8), g[4]);
            c.put32(frame.at(0xc4), g[5]);
            fr[0] = c.f64(frame.at(0xc4));
            fr[1] = fpu.fsubs(fr[0], fr[2]);
        }
    }
    // 0x101a84c0: the league's size times 50 (or the reputation) bounds three halfwords and the ratio of the last.
    const auto third = ctiwz(0, fr[1]);
    g[9] = c.u32(club + club::league);
    g[10] = word(std::int32_t(c.i16(club + club::reputation)));
    compare_logical(c, g[9], 0);
    c.put64(frame.at(0xb4), third);
    g[5] = g[10];
    g[4] = c.u32(frame.at(0xb8));
    if (g[9] != 0) {
        g[5] = word(std::int32_t(c.i16(g[9] + 0x18)));
        g[5] = g[5] * 0x32u;
        compare(c, i32(g[5]), i32(g[10]));
        if (i32(g[5]) > i32(g[10])) g[10] = g[5];
        g[5] = s16(g[10]);
    }
    g[10] = s16(g[5]);
    compare(c, i32(g[10]), 0x1388);
    if (i32(g[10]) < 0x1388) {
        g[9] = 0x1388;
    } else {
        compare(c, i32(g[10]), 0x2710);
        g[9] = 0x2710;
        if (!(i32(g[10]) > 0x2710)) g[9] = g[10];
    }
    g[9] = s16(g[9]);
    compare(c, i32(g[9]), i32(g[7]));
    if (i32(g[9]) > i32(g[7])) g[7] = g[9];
    g[10] = s16(g[3]);
    g[9] = s16(g[7]);
    compare(c, i32(g[10]), i32(g[9]));
    if (i32(g[10]) > i32(g[9])) g[3] = g[7];
    g[7] = s16(g[3]);
    compare(c, i32(g[7]), 1);
    if (!(i32(g[7]) >= 1)) g[3] = 1;
    g[9] = s16(g[5]);
    compare(c, i32(g[9]), 0x1388);
    if (i32(g[9]) < 0x1388) {
        g[7] = 0x1388;
    } else {
        compare(c, i32(g[9]), 0x2710);
        g[7] = 0x2710;
        if (!(i32(g[9]) > 0x2710)) g[7] = g[9];
    }
    g[7] = s16(g[7]);
    g[10] = g[6];
    compare(c, i32(g[7]), i32(g[6]));
    if (i32(g[7]) > i32(g[6])) g[10] = g[7];
    g[9] = s16(g[0]);
    g[7] = s16(g[10]);
    compare(c, i32(g[9]), i32(g[7]));
    if (i32(g[9]) > i32(g[7])) g[0] = g[10];
    g[7] = s16(g[0]);
    compare(c, i32(g[7]), 1);
    if (!(i32(g[7]) >= 1)) g[0] = 1;
    g[7] = s16(g[5]);
    g[5] = high_word;
    g[7] = g[7] ^ sign;
    c.put32(frame.at(0xb4), g[5]);
    fr[2] = pool_f(0x38);
    c.put32(frame.at(0xb8), g[7]);
    fr[0] = pool_f(0x28);
    fr[1] = c.f64(frame.at(0xb4));
    fr[3] = pool_f(0x1c0);
    fr[1] = fpu.fsub(fr[1], fr[2]);
    fr[0] = fpu.fdiv(fr[1], fr[0]);
    if (fc(fr[3], fr[0]) & 4) {
        // fr[3] stays the upper limit
    } else {
        fr[3] = pool_f(0x1b8);
        if (!(fc(fr[3], fr[0]) & 8)) fr[3] = fr[0];
    }
    c.put64(frame.at(0xbc), ctiwz(0, fr[3]));
    g[7] = c.u32(frame.at(0xc0));
    g[5] = s16(g[7]);
    compare(c, i32(g[5]), i32(g[8]));
    if (i32(g[5]) > i32(g[8])) g[8] = g[7];
    g[7] = s16(g[4]);
    g[5] = s16(g[8]);
    compare(c, i32(g[7]), i32(g[5]));
    if (i32(g[7]) > i32(g[5])) g[4] = g[8];
    g[5] = s16(g[4]);
    compare(c, i32(g[5]), 1);
    if (!(i32(g[5]) >= 1)) g[4] = 1;
    // 0x101a8638: the three values halved (rounded toward zero) with their old halves are written back.
    g[5] = g[6] >> 31;
    g[7] = s16(g[0]);
    g[6] = g[5] + g[6];
    g[5] = s16(g[3]);
    g[0] = g[7] >> 31;
    g[8] = word(i32(g[6]) >> 1);
    g[3] = s16(g[4]);
    g[6] = g[0] + g[7];
    g[4] = g[5] >> 31;
    g[6] = word(i32(g[6]) >> 1);
    g[0] = g[3] >> 31;
    g[4] = g[4] + g[5];
    g[5] = g[8] + g[6];
    g[0] = g[0] + g[3];
    c.put16(r14 + 0x2a, g[5]);
    g[3] = club;
    g[6] = word(std::int32_t(c.i16(r14 + 0x28)));
    g[5] = g[6] >> 31;
    g[5] = g[5] + g[6];
    g[5] = word(i32(g[5]) >> 1);
    g[4] = word(i32(g[4]) >> 1);
    g[4] = g[5] + g[4];
    c.put16(r14 + 0x28, g[4]);
    g[5] = word(std::int32_t(c.i16(r14 + 0x2c)));
    g[4] = g[5] >> 31;
    g[4] = g[4] + g[5];
    g[4] = word(i32(g[4]) >> 1);
    g[0] = word(i32(g[0]) >> 1);
    g[0] = g[4] + g[0];
    c.put16(r14 + 0x2c, g[0]);
    for (unsigned i = 0; i < 8; ++i) c.set_f(i, fr[i]);
    store(g);
    c.call(callee::club_flag, 0x101a86b4);
    const auto flagged = sbyte(c.r(3));
    c.set(0, word(flagged));
    compare(c, flagged);
    return flagged != 0;
}

// The assertion at 0x101a89e8 (line 0x28aa): the calls' return addresses are the original's here.
void report_unknown_standing() {
    const auto pool = c.toc(-0x5368);
    guest::report_assertion(
        c,
        {.at = 0x101a8a00, .file = pool + 0x37, .format = pool + 0x4b, .title = pool + 0x58, .name = frame.at(0x94),
         .extension = frame.at(0x84), .line = 0x28aa},
        [&](std::uint32_t entry, std::uint32_t) {
            const std::uint32_t back = entry == 0x10739890 ? 0x101a8a04 : entry == 0x10768810 ? 0x101a8a08
                                       : entry == 0x1077b720 ? 0x101a8a28 : 0x101a8a3c;
            c.call(entry, back);
        });
    c.set(3, c.toc(-0x7bac));
    c.set(0, 0);
}

// 0x101a8a48: the row's value cut to [0, 0x2710].
void clamp_value() {
    const auto rows = c.u32(object);
    c.set({{3, rows}, {4, row_offset + 6}});
    const auto value = std::int32_t(c.i16(rows + row_offset + 6));
    c.set(0, word(value));
    compare(c, value, 0x2710);
    if (value > 0x2710) {
        c.set(0, 0x2710);
        c.put16(rows + row_offset + 6, 0x2710);
    } else {
        compare(c, value);
        if (value < 0) {
            c.set(0, 0);
            c.put16(rows + row_offset + 6, 0);
        }
    }
}

// The row's +6 and +0x10 halfwords moved by d6 and d10 (two register shapes: the original's index and value registers).
void bump_a(std::int32_t d6, std::int32_t d10) {  // base r6, index r5; then base r5, index r4
    const auto rows = c.r(6);
    c.set({{5, row_offset + 6}, {4, row_offset + 0x10}});
    const auto first = std::int32_t(c.i16(rows + row_offset + 6));
    c.set(3, word(first));
    c.set(0, word(first + d6));
    c.put16(rows + row_offset + 6, word(first + d6));
    const auto again = c.u32(object);
    c.set(5, again);
    const auto second = std::int32_t(c.i16(again + row_offset + 0x10));
    c.set(3, word(second));
    c.set(0, word(second + d10));
    c.put16(again + row_offset + 0x10, word(second + d10));
}
void bump_b(std::int32_t d6, std::int32_t d10) {  // base r3, index r6, value r5; then base r5, index r4
    const auto rows = c.r(3);
    c.set({{6, row_offset + 6}, {4, row_offset + 0x10}});
    const auto first = std::int32_t(c.i16(rows + row_offset + 6));
    c.set(5, word(first));
    c.set(0, word(first + d6));
    c.put16(rows + row_offset + 6, word(first + d6));
    const auto again = c.u32(object);
    c.set(5, again);
    const auto second = std::int32_t(c.i16(again + row_offset + 0x10));
    c.set(3, word(second));
    c.set(0, word(second + d10));
    c.put16(again + row_offset + 0x10, word(second + d10));
}
// ---- 0x101a86b0..0x101a8a78: the switch over 0x100f55b0's answer moves the row's value and spread. ----
void standing_switch() {
    c.set(3, c.toc(-0x7ab8));
    c.set(4, club);
    c.set(3, c.u32(c.r(3)));
    c.call(0x10103510, 0x101a86cc);
    const auto found = c.r(3);
    if (compare_logical(c, found) == 0) return clamp_value();
    c.call(0x100f55b0, 0x101a86d8);
    const auto kind = sbyte(c.r(3));
    c.set(0, word(kind));
    compare(c, kind);
    const auto rep_below = [&](std::int32_t limit) {
        const auto reputation = std::int32_t(c.i16(club + club::reputation));
        c.set(0, word(reputation));
        compare(c, reputation, limit);
        return reputation < limit;
    };
    const auto is_flagged_241 = [&]() {
        const auto flag = sbyte(c.u8(club + 0x241));
        c.set(0, word(flag));
        compare(c, flag);
        return flag != 0;
    };
    if (kind == 0) {
        c.set(0, c.u32(club));
        const auto rows = c.u32(object);
        c.set(3, rows);
        const auto scaled = c.r(0) * 0x4cu;
        c.set(4, scaled);
        c.set(0, scaled + 1);
        const auto raw = c.u8(rows + scaled + 1);
        c.set(4, raw);
        c.set(0, word(sbyte(raw)));
        compare(c, sbyte(raw));
        if (sbyte(raw) < 0) {
            if (rep_below(0xdac)) bump_b(0x19, 0x19); else bump_b(5, 5);
        } else {
            c.set(0, word(sbyte(raw)));
            compare(c, sbyte(raw));
            if (sbyte(raw) > 0) bump_b(-5, -5);
        }
    } else if (kind == 1 || kind == 2) {
        c.set(0, c.u32(club));
        const auto rows = c.u32(object);
        c.set(6, rows);
        const auto scaled = c.r(0) * 0x4cu;
        c.set(3, scaled);
        c.set(0, scaled + 1);
        const auto raw = c.u8(rows + scaled + 1);
        c.set(0, raw);
        c.set(0, word(sbyte(raw)));
        compare(c, sbyte(raw), kind);
        if (sbyte(raw) < kind) {
            if (kind == 2) {
                if (rep_below(0xdac)) bump_a(0x23, 0x23); else bump_a(0xf, 0xf);
            } else {
                if (rep_below(0xdac)) bump_a(0x1e, 0x1e); else bump_a(0xa, 0xa);
            }
        }
    } else if (kind == -1) {
        c.set(0, c.u32(club));
        const auto rows = c.u32(object);
        c.set(3, rows);
        const auto scaled = c.r(0) * 0x4cu;
        c.set(4, scaled);
        c.set(0, scaled + 1);
        const auto raw = c.u8(rows + scaled + 1);
        c.set(0, word(sbyte(raw)));
        compare(c, sbyte(raw));
        if (sbyte(raw) < 0) {
            bump_b(5, 5);
        } else if (is_flagged_241()) {
            bump_b(-0x32, -0x7d);
        } else {
            bump_b(-0x19, -0x32);
        }
    } else if (kind == -2) {
        c.set(0, c.u32(club));
        const auto rows = c.u32(object);
        c.set(3, rows);
        const auto scaled = c.r(0) * 0x4cu;
        c.set(4, scaled);
        c.set(0, scaled + 1);
        const auto raw = c.u8(rows + scaled + 1);
        c.set(0, raw);
        c.set(0, word(sbyte(raw)));
        compare(c, sbyte(raw), -2);
        if (sbyte(raw) == -2) {
            bump_b(5, 5);
        } else if (is_flagged_241()) {
            bump_b(-0x4b, -0xaf);
        } else {
            bump_b(-0x32, -0x4b);
        }
    } else {
        report_unknown_standing();
    }
    clamp_value();
}

// 0x101a7f7c: everything the club's body still does; the club is over after it (the loop's advance follows).
std::uint32_t finish() {
    scale_change();
    if (!standings()) standing_switch();
    return label::club_end;
}
