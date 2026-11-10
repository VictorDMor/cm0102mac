// MANAGER_MANAGER.CPP weekly pass, Part D: 0x101a6958..0x101a7f7c. Included inside `class Club`
// (manager_manager_weekly.cpp) after part C, so the sections share the per-club members.
//
//   ladder       0x6958  the staff record's bytes +0x75/+0x76 order the club: below (the row's value climbs), equal (a
//                        level pick), above (f31 scales the gap): the value then moves toward a floor or ceiling by bands
//   quiet_stage  0x7840  r15 = 5 / 1 / -5 by +0x76 against r6 (the +0x75 byte the callers hold)
//   rows_stage   0x7864  r15 (the change) scaled by the league's size, the club's reputation, the manager's level and
//                        the row's doubles; added to the row's +6 / +8 (or +0x14 changes); ends at 0x7d28

// The five places the row's value is pushed toward a target through the manager's level: the draws' return addresses.
struct Sites {
    std::uint32_t pick, extra, none;
};
static constexpr Sites sites1{0x101a6a3c, 0, 0x101a6ba4}, sites2{0x101a6de8, 0, 0x101a6f50}, sites3{0x101a7074, 0, 0x101a71dc},
    sites4{0x101a7350, 0, 0x101a74b8}, sites5{0x101a7624, 0x101a776c, 0x101a77ec};

void put_value(std::uint32_t rows, std::uint32_t v) { c.put16(rows + row_offset + 6, v); }

// 0x101a6a08..0x101a6b64 and its four copies: the club's manager record (+0x80) names a person whose level (+0x3d) bounds a
// draw; a draw above the bound (or, with no person, a draw that is not a multiple of 10; 5: of 5) moves the value to `target`.
// The pick's floating-point stage (|level| > 0xffff) cannot be reached: the level is a sign-extended byte.
void pick_ladder(std::int32_t target, const Sites& s, bool last) {
    const bool special = last;
    bool hit = false;
    const auto manager = c.u32(club + 0x80);
    c.set(3, manager);
    bool person_found = false;
    if (nonzero_logical(c, manager)) {
        const auto person = c.u32(manager + 0x1c);
        c.set(3, person);
        if (nonzero_logical(c, person)) person_found = true, r27 = c.u8(person + 0x3d);
    }
    if (person_found) {
        const auto level = sbyte(r27);
        r27 = word(level);
        if (!nonzero(c, level)) {
            r28 = 0;
        } else {
            draw(s.pick);
            // The level is a sign-extended byte, so the floating-point stage of this draw (0x101a6a64 and
            // its four copies), which needs |level| > 0xffff, is not written.
            if (level > 0xffff || level < -0xffff) throw std::logic_error("MANAGER_MANAGER weekly pass: a pick's wide level is not written (0x101a6a64)");
            c.set({{3, 0x10000}, {0, 0xffff}});
            compare(c, level, 0xffff);
            c.set({{3, 0xffff0000u}, {0, 0xffff0001u}});
            compare(c, level, -0xffff);
            const auto cursor = c.u32(t.cursor_cell);
            c.set(3, cursor);
            compare(c, level);
            const auto sum = std::int32_t(c.u16(cursor) + c.u32(t.base_cell));
            const auto rest = std::int32_t(remainder(sum, level));
            c.set({{3, word(sum)}, {0, word(rest)}});
            r28 = word(level > 0 ? rest : std::int32_t(0u - word(rest)));
        }
        if (!special) {
            compare(c, std::int32_t(r28));
            hit = std::int32_t(r28) != 0;
        } else {
            // 0x101a774c: a second draw, modulo 30, must stay below the pick's number.
            const auto x = draw(s.extra);
            const auto cursor = c.u32(t.cursor_cell);
            const auto rest = x % 30;
            c.set({{5, cursor}, {4, word(x)}, {3, word(rest)}, {0, word(rest + 1)}});
            compare(c, std::int32_t(r28), rest + 1);
            hit = std::int32_t(r28) > rest + 1;
        }
    } else {
        const auto x = draw(s.none);
        const auto cursor = c.u32(t.cursor_cell);
        c.set({{5, cursor}, {4, word(x)}});
        if (!special) {
            const auto q = mulhw(0x66666667, x) >> 2;
            const auto rest = x % 10;
            c.set({{3, word(q) >> 31}, {0, word(rest)}});
            hit = compare(c, rest) != 0;
        } else {
            const auto q = mulhw(0x66666667, x) >> 1;
            const auto rest = x % 5;
            c.set({{3, word(q) >> 31}, {0, word(rest)}});
            hit = compare(c, rest) == 0;
        }
    }
    if (!hit) return;
    const auto rows = c.u32(object);
    c.set({{3, rows}, {0, row_offset + 6}, {4, word(target)}});
    r15 = 0;
    put_value(rows, word(target));
}

// The manager's level against the idle byte of the staff record: the value may be pulled to a ceiling when the idle byte is at
// least `pre + half the level + add`. The registers are those of the five copies (the person's level is built in `sign`).
enum class Tier { missing, low, passed };
Tier clamp(unsigned sign, unsigned idle_reg, unsigned sgn, unsigned acc, bool use_pre, std::int32_t pre, std::int32_t add) {
    const auto manager = c.u32(club + 0x80);
    c.set(sign, manager);
    if (!nonzero_logical(c, manager)) return Tier::missing;
    const auto person = c.u32(manager + 0x1c);
    c.set(sign, person);
    if (!nonzero_logical(c, person)) return Tier::missing;
    const auto level_byte = c.u8(person + 0x3d);
    c.set(sign, level_byte);
    const auto idle = std::int32_t(c.u8(r25 + 4));
    c.set(idle_reg, word(idle));
    const auto level = sbyte(level_byte);
    c.set(sgn, word(level));
    c.set(sign, word(level) >> 31);
    c.set(sign, word(level) + (word(level) >> 31));
    const auto half = half_of(word(level));
    c.set(sign, word(half));
    const auto need = std::int32_t((use_pre ? word(pre) : 0u) + word(half) + word(add));
    c.set(acc, word(need));
    compare(c, idle, need);
    return idle < need ? Tier::low : Tier::passed;
}

std::int32_t row_value(std::uint32_t rows) { return std::int32_t(c.i16(rows + row_offset + 6)); }

// ---- 0x101a6958: the order of the staff record's bytes. ----
std::uint32_t ladder() {
    const auto s76 = c.u8(r25 + 0x76);
    c.set(0, s76);
    const auto s75 = c.u8(r25 + 0x75);
    c.set(5, s75);
    compare_logical(c, s76, s75);
    if (s76 < s75) {  // 0x101a6968: the value climbs by 15 (or 10) per step of the gap
        const auto d = std::int32_t(std::int16_t(word(s75 - s76)));
        c.set(0, word(d));
        compare(c, d, 4);
        const auto rows = c.u32(object);
        c.set({{3, rows}, {5, row_offset + 6}});
        if (d > 4) {
            c.set(4, word(d * 15));
            const auto value = row_value(rows);
            c.set(0, word(value));
            compare(c, value, 0x1964);
            c.set(0, word(d * 15 + 0x28));
            r15 = word(std::int32_t(std::int16_t(word(d * 15 + 0x28))));
            if (value >= 0x1964) return label::rows_end;
            c.set(0, 0x1964);
            r15 = 0;
            put_value(rows, 0x1964);
            return label::rows_end;
        }
        c.set(4, word(d * 10));
        const auto value = row_value(rows);
        c.set(0, word(value));
        compare(c, value, 0x10b3);
        r15 = word(std::int32_t(std::int16_t(word(d * 10))));
        if (value >= 0x10b3) return label::rows_end;
        c.set(0, 0x10b3);
        r15 = 0;
        put_value(rows, 0x10b3);
        return label::rows_end;
    }
    if (s76 == s75) {  // 0x101a69d8
        const auto rows = c.u32(object);
        c.set({{3, rows}, {0, row_offset + 6}});
        r15 = 5;
        const auto value = row_value(rows);
        c.set(0, word(value));
        compare(c, value, 0x10b3);
        if (value >= 0x10b3) return label::rows_end;
        pick_ladder(0x10b3, sites1, false);
        return label::rows_end;
    }
    return pushed(s76, s75);
}

// ---- 0x101a6bf8: the +0x76 byte is above +0x75: the gap times f31, at least 2 - 2 * idle, scales into a step. ----
std::uint32_t pushed(std::uint32_t s76, std::uint32_t s75) {
    const auto idle = std::int32_t(c.u8(r25 + 4));
    c.set(3, high_word);
    c.set(4, word(idle - 2));
    c.set(4, word(idle - 2) << 1);
    c.set(4, 0u - (word(idle - 2) << 1));
    const auto minimum = std::int32_t(0u - (word(idle - 2) << 1));
    c.put32(frame.at(0xc0), s76);
    double f2 = pool_f(0x1c8);
    c.put32(frame.at(0xbc), high_word);
    double f0 = c.f64(frame.at(0xbc));
    c.put32(frame.at(0xb8), s75);
    f0 = fpu.fsub(f0, f2);
    c.put32(frame.at(0xb4), high_word);
    double f1 = c.f64(frame.at(0xb4));
    f0 = fpu.fmul(f0, c.f(31));
    f1 = fpu.fsub(f1, f2);
    f0 = fpu.multiply_add(0x101a6c34, f1, c.f(31), f0);
    c.put64(frame.at(0xc4), fpu.fctiwz(0, f0));
    const auto converted = c.u32(frame.at(0xc8));
    c.set(3, converted);
    auto step = std::int32_t(std::int16_t(converted));
    c.set(5, word(step));
    compare(c, minimum, step);
    if (minimum > step) step = minimum;
    c.set(5, word(step));
    c.set_f(1, f1);
    c.set_f(2, f2);
    // 0x101a6c54: the row's flag halfword (+0xe) bits 1-2 or 3 let a negative step also lift the row when the league's record
    // (+0xc1 byte) allows it; the step becomes 0.
    const auto rows = c.u32(object);
    const auto row = rows + row_offset;
    c.set(3, row);
    step = std::int32_t(std::int16_t(word(step)));
    c.set(5, word(step));
    const auto flags = std::int32_t(c.i16(row + 0xe));
    c.set(6, word(flags));
    const auto low_bits = flags & 6;
    c.set(4, word(low_bits));
    compare(c, low_bits);
    bool via_low = low_bits != 0;
    bool done = false;
    if (via_low) {
        const auto v = step;
        c.set(4, word(v));
        compare(c, v);
        if (v < 0) {
            const auto league = c.u32(club + club::league);
            c.set(4, league);
            if (nonzero_logical(c, league)) {
                done = true;
                const auto index = c.u32(league);
                c.set(3, index);
                const auto table = c.u32(c.r(18));
                c.set(4, table);
                c.set(3, index << 2);
                const auto record = c.u32(table + (index << 2));
                c.set(3, record);
                if (nonzero_logical(c, record)) {
                    const auto b = c.u8(record + 0xc1);
                    c.set(4, b);
                    const auto sb = sbyte(b);
                    c.set(3, word(sb));
                    if (nonzero(c, sb)) {
                        c.set(3, word(sb));
                        compare(c, std::int32_t(s76), sb);
                        if (!(std::int32_t(s76) > sb)) {
                            c.set(5, 0);
                            step = 0;
                        }
                    }
                }
            }
        }
    }
    if (!done) {
        const auto bit3 = flags & 8;
        c.set(4, word(bit3));
        compare(c, bit3);
        if (bit3 != 0) {
            const auto v = step;
            c.set(4, word(v));
            compare(c, v);
            if (v < 0) {
                const auto league = c.u32(club + club::league);
                c.set(4, league);
                if (nonzero_logical(c, league)) {
                    const auto index = c.u32(league);
                    c.set(4, index);
                    const auto table = c.u32(c.r(18));
                    c.set(6, table);
                    c.set(4, index << 2);
                    const auto record = c.u32(table + (index << 2));
                    c.set(4, record);
                    if (nonzero_logical(c, record)) {
                        const auto b = c.u8(record + 0xc1);
                        c.set(6, b);
                        const auto sb = sbyte(b);
                        c.set(4, word(sb));
                        if (nonzero(c, sb)) {
                            c.set(4, word(sb));
                            compare(c, std::int32_t(s76), sb);
                            if (!(std::int32_t(s76) > sb)) {
                                const auto value = std::int32_t(c.i16(row + 6));
                                c.set(0, word(value));
                                step = 0;
                                c.set(5, 0);
                                compare(c, value, 0x1964);
                                if (!(value >= 0x1964)) {
                                    c.set(0, 0x1964);
                                    c.put16(row + 6, 0x1964);
                                }
                            }
                        }
                    }
                }
            }
        }
    }
    return delta_ladder(step);
}

// ---- 0x101a6d1c: the step's size picks the band: the value is pushed to a ceiling or a floor. ----
std::uint32_t delta_ladder(std::int32_t step) {
    c.set(4, word(step));
    compare(c, step, -3);
    if (step > -3) return band_high(step);
    compare(c, step, -5);
    if (step > -5) return band_mid(step);
    compare(c, step, -10);
    if (step > -10) return band_low(step);
    return band_lowest(step);
}

// Ends a ceiling or floor: the value is `target`, the change 0.
void store_target(unsigned reg, std::uint32_t rows, std::uint32_t target) {
    c.set(reg, target);
    r15 = 0;
    put_value(rows, target);
}

// 0x101a6d28: step above -3.
std::uint32_t band_high(std::int32_t step) {
    const auto rows = c.u32(object);
    c.set({{3, rows}, {0, row_offset + 6}});
    r15 = word(-5);
    const auto value = row_value(rows);
    c.set(5, word(value));
    compare(c, value, 0x251c);
    if (value > 0x251c) {
        c.set(4, word(step) << 1);
        if (clamp(5, 7, 6, 4, true, step * 2, 10) == Tier::passed) store_target(4, rows, 0x251c);
        return label::rows_end;
    }
    compare(c, value, 0x10b3);
    if (value >= 0x10b3) return label::rows_end;
    pick_ladder(0x10b3, sites2, false);
    return label::rows_end;
}

// 0x101a6fac: step -4 or -3.
std::uint32_t band_mid(std::int32_t step) {
    const auto rows = c.u32(object);
    c.set({{7, rows}, {3, word(step - 2)}, {0, row_offset + 6}});
    c.set(3, word((step - 2) * 10));
    const auto value = row_value(rows);
    c.set(5, word(value));
    compare(c, value, 0x1964);
    c.set(3, word((step - 2) * 10 - 5));
    r15 = word(std::int32_t(std::int16_t(word((step - 2) * 10 - 5))));
    if (value > 0x1964) {
        if (clamp(3, 6, 5, 3, true, step, 12) == Tier::passed) {
            c.set(3, 0x1964);
            r15 = 0;
            put_value(rows, 0x1964);
        }
        return label::rows_end;
    }
    compare(c, value, 0xdac);
    if (value >= 0xdac) return label::rows_end;
    pick_ladder(0xdac, sites3, false);
    return label::rows_end;
}

// Shared by the two lowest bands: ceilings 0x1964 and 0x10b3 reached through the manager's level.
// Returns true when the band's floor ladder is to run (no ceiling applied).
bool ceilings(std::uint32_t rows, std::int32_t value) {
    if (value > 0x1964) {
        if (clamp(5, 7, 6, 5, false, 0, 10) == Tier::passed) {
            store_target(4, rows, 0x1964);
            return false;
        }
    }
    compare(c, value, 0x10b3);
    if (value > 0x10b3) {
        if (clamp(4, 6, 5, 4, false, 0, 15) == Tier::passed) store_target(4, rows, 0x10b3);
        return false;
    }
    return true;
}

// 0x101a7238: step -9 .. -5.
std::uint32_t band_low(std::int32_t step) {
    const auto rows = c.u32(object);
    c.set({{3, rows}, {4, word(step + 4)}, {0, row_offset + 6}});
    c.set(5, word((step + 4) * 25));
    const auto value = row_value(rows);
    c.set(4, word(value));
    compare(c, value, 0x1964);
    c.set(5, word((step + 4) * 25 - 40));
    r15 = word(std::int32_t(std::int16_t(word((step + 4) * 25 - 40))));
    if (!ceilings(rows, value)) return label::rows_end;
    compare(c, value, 0xbb8);
    if (value >= 0xbb8) return label::rows_end;
    pick_ladder(0xbb8, sites4, false);
    return label::rows_end;
}

// 0x101a750c: step -10 and below.
std::uint32_t band_lowest(std::int32_t step) {
    const auto rows = c.u32(object);
    c.set({{3, rows}, {4, word(step + 9)}, {0, row_offset + 6}});
    c.set(5, word((step + 9) * 30));
    const auto value = row_value(rows);
    c.set(4, word(value));
    compare(c, value, 0x1964);
    c.set(5, word((step + 9) * 30 - 165));
    r15 = word(std::int32_t(std::int16_t(word((step + 9) * 30 - 165))));
    if (!ceilings(rows, value)) return label::rows_end;
    compare(c, value, 0xdac);
    if (value >= 0xdac) return label::rows_end;
    pick_ladder(0xdac, sites5, true);
    return label::rows_end;
}

// ---- 0x101a7840 ----
std::uint32_t quiet_stage() {
    const auto s76 = c.u8(r25 + 0x76);
    c.set(0, s76);
    const auto r6 = c.r(6);
    compare_logical(c, s76, r6);
    if (s76 < r6) r15 = 5;
    else if (s76 == r6) r15 = 1;
    else r15 = word(-5);
    return label::rows_end;
}

// f0 = (r15 as int16) * factor in single precision, back into r15 through fctiwz and the frame (0xb4..0xc0).
void scale_by(double factor) {
    const auto v = std::int32_t(std::int16_t(r15));
    c.set(3, word(v) ^ 0x80000000u);
    c.set(0, high_word);
    c.put32(frame.at(0xb4), high_word);
    const double f1 = pool_f(0x38);
    c.put32(frame.at(0xb8), word(v) ^ 0x80000000u);
    double f0 = c.f64(frame.at(0xb4));
    f0 = fpu.fsubs(f0, f1);
    f0 = fpu.fmuls(f0, factor);
    c.put64(frame.at(0xbc), fpu.fctiwz(0, f0));
    r15 = c.u32(frame.at(0xc0));
}
std::int32_t r15_signed() {
    const auto v = std::int32_t(std::int16_t(r15));
    c.set(0, word(v));
    compare(c, v);
    return v;
}
// ---- 0x101a7864: the change r15, scaled and added to the row. ----
std::uint32_t rows_stage() {
    const auto league = c.u32(club + club::league);
    c.set(3, league);
    c.put32(frame.at(0xb4), high_word);
    const auto size = std::int32_t(c.i16(league + 0x18));
    c.set(3, word(size));
    double f2 = pool_f(0x38);
    c.set(0, word(size) ^ 0x80000000u);
    double f3 = pool_f(0x48);
    c.put32(frame.at(0xb8), word(size) ^ 0x80000000u);
    double f0 = pool_f(0x98);
    double f1 = c.f64(frame.at(0xb4));
    f1 = fpu.fsub(f1, f2);
    f1 = fpu.fmul(f3, f1);
    f1 = fpu.frsp(f1);
    if (fcmp(f1, f0) & 4) {
        f1 = news_f(0x20);
    } else {
        f0 = pool_f(0x20);
        if (fcmp(f1, f0) & 8) f1 = news_f(0x1c);
    }
    c.set_f(0, f0);
    c.set_f(1, f1);
    c.set_f(2, f2);
    c.set_f(3, f3);
    {
        const auto v = std::int32_t(std::int16_t(r15));
        c.set(0, word(v) ^ 0x80000000u);
        c.set(4, high_word);
    }
    const auto reputation = std::int32_t(c.i16(club + club::reputation));
    c.set(5, word(reputation));
    c.put32(frame.at(0xb8), word(std::int32_t(std::int16_t(r15))) ^ 0x80000000u);
    const double f4 = pool_f(0x38);
    compare(c, reputation, 0x1388);
    c.put32(frame.at(0xb4), high_word);
    f0 = c.f64(frame.at(0xb4));
    f0 = fpu.fsubs(f0, f4);
    f0 = fpu.fmuls(f0, f1);
    c.put64(frame.at(0xbc), fpu.fctiwz(0, f0));
    r15 = c.u32(frame.at(0xc0));
    c.set_f(4, f4);
    if (reputation > 0x1388) {
        c.set(0, word(size) * 0x190u);
        compare(c, reputation, std::int32_t(word(size) * 0x190u));
        if (reputation > std::int32_t(word(size) * 0x190u)) sharp_rise(f4);
    }
    return after_rise();
}

// 0x101a7900..0x101a79c4: reputation above 5000 and 400 per league size: a gap of the staff bytes (times f31) below pool+0x1a0 and
// the other ratio below pool+0x1a8 / f31: a negative change doubles toward a limit.
void sharp_rise(double f4) {
    const auto s76 = c.u8(r25 + 0x76);
    c.set(0, s76);
    const auto s75 = c.u8(r25 + 0x75);
    c.set(3, s75);
    c.put32(frame.at(0xc0), s76);
    double f3 = pool_f(0x1c8);
    c.put32(frame.at(0xbc), high_word);
    double f0 = pool_f(0x1a0);
    double f1 = c.f64(frame.at(0xbc));
    c.put32(frame.at(0xb8), s75);
    f1 = fpu.fsub(f1, f3);
    c.put32(frame.at(0xb4), high_word);
    double f2 = c.f64(frame.at(0xb4));
    f1 = fpu.fmul(f1, c.f(31));
    f2 = fpu.fsub(f2, f3);
    f2 = fpu.multiply_add(0x101a7934, f2, c.f(31), f1);
    c.set_f(0, f0);
    c.set_f(1, f1);
    c.set_f(2, f2);
    c.set_f(3, f3);
    if (!(fcmp(f2, f0) & 8)) return;
    f0 = pool_f(0x1a8);
    c.put32(frame.at(0xb8), s75);
    f0 = fpu.fdiv(f0, c.f(31));
    c.put32(frame.at(0xb4), high_word);
    f1 = c.f64(frame.at(0xb4));
    f1 = fpu.fsub(f1, f3);
    c.set_f(0, f0);
    c.set_f(1, f1);
    if (!(fcmp(f1, f0) & 8)) return;
    if (r15_signed() >= 0) return;
    const auto idle = c.u8(r25 + 4);
    c.set(0, idle);
    const auto v = std::int32_t(std::int16_t(r15));
    c.set(3, word(v));
    c.set(3, word(v) << 1);
    c.put32(frame.at(0xbc), high_word);
    c.set(3, (word(v) << 1) ^ 0x80000000u);
    f1 = pool_f(0x28);
    c.put32(frame.at(0xc0), idle);
    f0 = c.f64(frame.at(0xbc));
    c.put32(frame.at(0xb8), (word(v) << 1) ^ 0x80000000u);
    f0 = fpu.fsub(f0, f3);
    c.put32(frame.at(0xb4), high_word);
    f0 = fpu.fmul(f0, f2);
    f2 = c.f64(frame.at(0xb4));
    f2 = fpu.fsub(f2, f4);
    f0 = fpu.fmul(f1, f0);
    c.set_f(1, f1);
    c.set_f(2, f2);
    if (fcmp(f2, f0) & 8) {
        c.put32(frame.at(0xc8), (word(v) << 1) ^ 0x80000000u);
        c.put32(frame.at(0xc4), high_word);
        f0 = c.f64(frame.at(0xc4));
        f0 = fpu.fsub(f0, f4);
    }
    c.set_f(0, f0);
    c.put64(frame.at(0xcc), fpu.fctiwz(0, f0));
    r15 = c.u32(frame.at(0xd0));
}

// 0x101a79c8: the manager's level (+0x3d, kept in 1..20) scales a negative change; then the row stage.
std::uint32_t after_rise() {
    const auto manager = c.u32(club + 0x80);
    c.set(3, manager);
    bool scaled = false;
    if (nonzero_logical(c, manager)) {
        const auto person = c.u32(manager + 0x1c);
        c.set(3, person);
        if (nonzero_logical(c, person) && r15_signed() < 0) {
            auto level = sbyte(c.u8(person + 0x3d));
            c.set(0, word(level));
            compare(c, level, 1);
            if (level < 1 || (compare(c, level, 0x14), level > 0x14)) {
                c.set(0, 10);
                c.put8(person + 0x3d, 10);
            }
            const auto person_again = c.u32(c.u32(club + 0x80) + 0x1c);
            c.set(3, person_again);
            level = sbyte(c.u8(person_again + 0x3d));
            c.set(0, word(level));
            double f2;
            compare(c, level, 1);
            if (level == 1) {
                f2 = news_f(0x34);
            } else {
                compare(c, level, 0x14);
                if (level == 0x14) {
                    f2 = news_f(0x24);
                } else {
                    c.set(3, word(0x15 - level) ^ 0x80000000u);
                    c.set(0, high_word);
                    c.put32(frame.at(0xb4), high_word);
                    const double f3 = pool_f(0x38);
                    c.put32(frame.at(0xb8), word(0x15 - level) ^ 0x80000000u);
                    const double f1 = pool_f(0x180);
                    f2 = c.f64(frame.at(0xb4));
                    double f0 = pool_f(0x98);
                    f2 = fpu.fsubs(f2, f3);
                    f2 = fpu.fdiv(f2, f1);
                    f2 = fpu.frsp(f2);
                    c.set_f(0, f0);
                    c.set_f(1, f1);
                    c.set_f(3, f3);
                    if (fcmp(f2, f0) & 4) {
                        f2 = news_f(0x20);
                    } else {
                        f0 = pool_f(0x20);
                        c.set_f(0, f0);
                        if (fcmp(f2, f0) & 8) f2 = news_f(0x1c);
                    }
                }
            }
            c.set_f(2, f2);
            const auto v = std::int32_t(std::int16_t(r15));
            c.set(3, word(v) ^ 0x80000000u);
            c.set(0, high_word);
            c.put32(frame.at(0xb4), high_word);
            const double f1 = pool_f(0x38);
            c.put32(frame.at(0xb8), word(v) ^ 0x80000000u);
            double f0 = c.f64(frame.at(0xb4));
            f0 = fpu.fsubs(f0, f1);
            f0 = fpu.fmuls(f0, f2);
            c.set_f(0, f0);
            c.set_f(1, f1);
            c.put64(frame.at(0xbc), fpu.fctiwz(0, f0));
            r15 = c.u32(frame.at(0xc0));
            scaled = true;
        }
    }
    (void)scaled;
    return unflagged_rows();
}

// 0x101a7ab8: an unflagged club's doubles (+0x38 over +0x30) change the +0x14 count; then the change reaches the row.
std::uint32_t unflagged_rows() {
    c.set(3, club);
    c.call(callee::club_flag, 0x101a7ac0);
    const auto flagged = sbyte(c.r(3));
    c.set(0, word(flagged));
    if (!nonzero(c, flagged)) {
        const auto rows = c.u32(object);
        c.set(0, rows);
        const auto row = rows + row_offset;
        c.set(5, row);
        double f0 = c.f64(row + 0x38);
        double f1 = c.f64(row + 0x30);
        c.set_f(0, f0);
        c.set_f(1, f1);
        if (fcmp(f0, f1) & 4) {
            if (r15_signed() < 0) {
                double f2 = fpu.frsp(f0);
                f0 = pool_f(0x120);
                f1 = fpu.frsp(f1);
                f2 = fpu.fdivs(f2, f1);
                c.set_f(0, f0);
                c.set_f(1, f1);
                if (fcmp(f2, f0) & 4) f2 = news_f(0x34);
                c.set_f(2, f2);
                scale_by(f2);
            }
            const auto count = std::int32_t(c.i16(row + 0x14));
            c.set(3, word(count));
            c.set(0, word(count - 0x4b));
            c.put16(row + 0x14, word(count - 0x4b));
        } else if (r15_signed() > 0) {
            const auto v = std::int32_t(std::int16_t(r15));
            c.set(0, word(v) ^ 0x80000000u);
            c.set(4, high_word);
            const auto count = std::int32_t(c.i16(row + 0x14));
            c.set(3, word(count));
            c.put32(frame.at(0xb8), word(v) ^ 0x80000000u);
            c.set(0, word(count + 5));
            const double f1b = pool_f(0x38);
            c.put32(frame.at(0xb4), high_word);
            const double f2 = pool_f(0x1b0);
            double f0b = c.f64(frame.at(0xb4));
            c.put16(row + 0x14, word(count + 5));
            f0b = fpu.fsub(f0b, f1b);
            f0b = fpu.fmul(f2, f0b);
            c.set_f(1, f1b);
            c.set_f(2, f2);
            c.set_f(0, f0b);
            c.put64(frame.at(0xbc), fpu.fctiwz(0, f0b));
            r15 = c.u32(frame.at(0xc0));
        }
    }
    return last_scalings();
}

// 0x101a7b8c..0x101a7d28: a negative change is scaled again for a club not flagged +0x23d, for a region whose +0x68 is one of
// three TOC values; then it is added to the row's +6 and +8 (a club with +0x14 != -1 also keeps the double's scale).
std::uint32_t last_scalings() {
    auto scale = [&](double factor) {
        const auto v = std::int32_t(std::int16_t(r15));
        c.set(3, word(v) ^ 0x80000000u);
        c.set(0, high_word);
        c.put32(frame.at(0xb4), high_word);
        const double f1 = pool_f(0x38);
        c.put32(frame.at(0xb8), word(v) ^ 0x80000000u);
        double f0 = c.f64(frame.at(0xb4));
        f0 = fpu.fsub(f0, f1);
        f0 = fpu.fmul(factor, f0);
        c.set_f(0, f0);
        c.set_f(1, f1);
        c.set_f(2, factor);
        c.put64(frame.at(0xbc), fpu.fctiwz(0, f0));
        r15 = c.u32(frame.at(0xc0));
    };
    if (r15_signed() < 0) {
        const auto flag = c.u8(club + 0x23d);
        c.set(0, flag);
        compare_logical(c, flag, 1);
        if (flag != 1) scale(pool_f(0x10));
    }
    if (r15_signed() < 0) {
        const auto region = c.u32(club + club::region);
        c.set(3, region);
        if (nonzero_logical(c, region)) {
            const auto code = c.u32(region + 0x68);
            c.set(4, code);
            bool match = false;
            for (const std::int32_t slot_offset : {-0x757c, -0x7574, -0x7578}) {
                const auto cell = c.toc(slot_offset);
                c.set(3, cell);
                const auto other = c.u32(cell);
                c.set(0, other);
                compare(c, std::int32_t(code), std::int32_t(other));
                if (code == other) {
                    match = true;
                    break;
                }
            }
            if (match) scale(pool_f(0x10));
        }
    }
    const auto marker = c.i32(club + 0x14);
    c.set(0, word(marker));
    compare(c, marker, -1);
    if (marker != -1 && r15_signed() < 0) {
        const auto v = std::int32_t(std::int16_t(r15));
        c.set(5, word(v));
        c.set(3, word(v) ^ 0x80000000u);
        c.set(0, high_word);
        c.put32(frame.at(0xb4), high_word);
        const double f1 = pool_f(0x38);
        c.set({{5, row_offset + 6}, {4, row_offset + 8}});
        c.put32(frame.at(0xb8), word(v) ^ 0x80000000u);
        const auto rows = c.u32(object);
        c.set(6, rows);
        double f0 = c.f64(frame.at(0xb4));
        const double f2 = pool_f(0);
        f0 = fpu.fsub(f0, f1);
        const auto value = std::int32_t(c.i16(rows + row_offset + 6));
        c.set(3, word(value));
        f0 = fpu.fmul(f2, f0);
        const auto bits = fpu.fctiwz(0, f0);
        c.put64(frame.at(0xbc), bits);
        const auto low = c.u32(frame.at(0xc0));
        c.set(0, low);
        c.put64(frame.at(0xc4), bits);
        c.set(0, word(value) + low);
        c.put16(rows + row_offset + 6, word(value) + low);
        const auto again = c.u32(frame.at(0xc8));
        c.set(0, again);
        const auto rows2 = c.u32(object);
        c.set(5, rows2);
        const auto target = std::int32_t(c.i16(rows2 + row_offset + 8));
        c.set(3, word(target));
        c.set(0, word(target) + again);
        c.put16(rows2 + row_offset + 8, word(target) + again);
        c.set_f(1, f1);
        c.set_f(2, f2);
        return label::own_end;
    }
    const auto v = std::int32_t(std::int16_t(r15));
    c.set(5, word(v));
    c.set(3, word(v) ^ 0x80000000u);
    c.set(0, high_word);
    c.put32(frame.at(0xb4), high_word);
    const double f1 = pool_f(0x38);
    c.set({{6, row_offset + 6}, {4, row_offset + 8}});
    c.put32(frame.at(0xb8), word(v) ^ 0x80000000u);
    const auto rows = c.u32(object);
    c.set(3, rows);
    double f0 = c.f64(frame.at(0xb4));
    const auto value = std::int32_t(c.i16(rows + row_offset + 6));
    c.set(0, word(value));
    f0 = fpu.fsub(f0, f1);
    const double f1b = pool_f(0x1b0);
    c.set(0, word(value) + word(v));
    c.put16(rows + row_offset + 6, word(value) + word(v));
    f0 = fpu.fmul(f1b, f0);
    const auto rows2 = c.u32(object);
    c.set(5, rows2);
    const auto target = std::int32_t(c.i16(rows2 + row_offset + 8));
    c.set(3, word(target));
    c.put64(frame.at(0xbc), fpu.fctiwz(0, f0));
    const auto low = c.u32(frame.at(0xc0));
    c.set(0, word(target) + low);
    c.put16(rows2 + row_offset + 8, word(target) + low);
    c.set_f(1, f1b);
    return label::own_end;
}

// ---- 0x101a7d28..0x101a7f7c: the club's last row stage. The row's flag halfword bit 0x400 and the row's byte +2 bit 0x10 (with the
// club's +0x14 against a game value and the board's word at +0x14 of the club under sp+0xec) clamp the value toward 0x1964 or
// 0x10b3; the value and target are cut to [0, 0x2710] and, by the manager's standing (0x100d9580 / 0x100d9520 for the value,
// 0x100d94d0 / 0x100d9480 for the target), to 0x1482, 0x109a, 0xbb8 or 0x1b58; a row byte with bits 0x80, 0x20 or 0x10 ends the club.
// Returns 0x101a8a78 or 0x101a7f7c (part E). ----
bool clamp_gate() {
    const auto cell = c.toc(-0x7be8);
    c.set(4, cell);
    const auto array = c.u32(cell);
    c.set(5, array);
    const auto slot_value = c.u32(frame.at(slot::club_pointer));
    c.set(4, slot_value);
    c.set(0, slot_value + 0x14);
    const auto word_at = c.i32(array + slot_value + 0x14);
    c.set(0, word(word_at));
    compare(c, word_at, -1);
    if (word_at == -1) return false;
    const auto game_cell = c.toc(-0x7a2c);
    c.set(4, game_cell);
    const auto mine = c.u32(club + 0x14);
    c.set(5, mine);
    const auto game = c.u32(game_cell);
    c.set(0, game);
    compare(c, std::int32_t(mine), std::int32_t(game));
    return mine != game;
}

std::uint32_t closing() {
    const auto rows0 = c.u32(object);
    c.set(0, rows0);
    const auto row = rows0 + row_offset;
    c.set(3, row);
    const auto flags = c.i16(row + 0xe);
    c.set(0, word(std::int32_t(std::int16_t(flags))));
    const auto high_flag = std::uint32_t(std::int32_t(std::int16_t(flags))) & 0x400u;
    c.set(6, high_flag);
    compare(c, std::int32_t(high_flag));
    if (high_flag == 0) {
        if (clamp_gate()) {
            const auto value = std::int32_t(c.i16(row + 6));
            c.set(0, word(value));
            compare(c, value, 0x1964);
            if (value < 0x1964) {
                c.set(0, 0x1964);
                c.put16(row + 6, 0x1964);
            }
        }
    } else {
        if (clamp_gate()) {
            const auto byte = c.u8(row + 2);
            c.set(0, byte);
            const bool bit = (byte & 0x10) != 0;
            c.set(0, bit ? 0xffffffffu : 0u);
            compare(c, bit ? -1 : 0);
            const auto value = std::int32_t(c.i16(row + 6));
            c.set(0, word(value));
            if (!bit) {
                compare(c, value, 0x1964);
                if (value < 0x1964) {
                    c.set(0, 0x1964);
                    c.put16(row + 6, 0x1964);
                }
            } else {
                compare(c, value, 0x10b3);
                if (value < 0x10b3) {
                    c.set(0, 0x10b3);
                    c.put16(row + 6, 0x10b3);
                }
            }
        }
    }
    // 0x101a7dfc: the value's change since the row stage's start (r15), cut to [0, 0x2710].
    {
        const auto rows = c.u32(object);
        c.set({{3, rows}, {5, row_offset + 6}});
        const auto old = std::int32_t(c.i16(frame.at(slot::stage_half)));
        c.set(0, word(old));
        const auto value = std::int32_t(c.i16(rows + row_offset + 6));
        c.set(4, word(value));
        compare(c, value, 0x2710);
        c.set(0, word(value) - word(old));
        r15 = word(std::int32_t(std::int16_t(word(value) - word(old))));
        if (value > 0x2710) {
            c.set(0, 0x2710);
            put_value(rows, 0x2710);
        } else {
            c.set(0, word(value));
            compare(c, value);
            if (value < 0) {
                c.set(0, 0);
                put_value(rows, 0);
            }
        }
    }
    // 0x101a7e38: the manager's standing bounds the value.
    const auto standing = [&](std::uint32_t callee_address, std::uint32_t back, bool by_club) {
        if (by_club) {
            c.set({{3, c.u32(frame.at(slot::record))}, {4, club}});
        } else {
            c.set({{4, c.u32(club + 0x80)}, {3, c.u32(frame.at(slot::record))}});
        }
        c.call(callee_address, back);
        const auto answer = sbyte(c.r(3));
        c.set(0, word(answer));
        return nonzero(c, answer);
    };
    auto cut_value = [&](std::int32_t limit, bool upper) {
        const auto rows = c.u32(object);
        c.set({{3, rows}, {5, row_offset + 6}, {0, word(limit)}});
        const auto v = std::int32_t(c.i16(rows + row_offset + 6));
        c.set(4, word(v));
        compare(c, v, limit);
        const bool take_limit = upper ? v >= limit : v <= limit;
        if (!take_limit) c.set(0, word(v));
        put_value(rows, take_limit ? word(limit) : word(v));
    };
    if (standing(0x100d9580, 0x101a7e44, false))
        cut_value(0x1482, true);
    else if (standing(0x100d9520, 0x101a7e7c, false))
        cut_value(0x109a, false);
    // 0x101a7ea4: the target (+8), cut to [0, 0x2710].
    {
        const auto rows = c.u32(object);
        c.set({{3, rows}, {4, row_offset + 8}});
        const auto target = std::int32_t(c.i16(rows + row_offset + 8));
        c.set(0, word(target));
        compare(c, target, 0x2710);
        if (target > 0x2710) {
            c.set(0, 0x2710);
            c.put16(rows + row_offset + 8, 0x2710);
        } else {
            compare(c, target);
            if (target < 0) {
                c.set(0, 0);
                c.put16(rows + row_offset + 8, 0);
            }
        }
    }
    const auto cut_target = [&](std::int32_t limit, bool upper) {
        const auto rows = c.u32(object);
        c.set({{3, rows}, {5, row_offset + 8}, {0, word(limit)}});
        const auto v = std::int32_t(c.i16(rows + row_offset + 8));
        c.set(4, word(v));
        compare(c, v, limit);
        const bool take_limit = upper ? v >= limit : v <= limit;
        if (!take_limit) c.set(0, word(v));
        c.put16(rows + row_offset + 8, take_limit ? word(limit) : word(v));
    };
    if (standing(0x100d94d0, 0x101a7ee0, true))
        cut_target(0xbb8, true);
    else if (standing(0x100d9480, 0x101a7f18, true))
        cut_target(0x1b58, false);
    // 0x101a7f40: bits 0x80, 0x20, 0x10 of the row byte +2 end the club.
    const auto rows = c.u32(object);
    c.set(0, row_offset + 2);
    const auto byte = c.u8(rows + row_offset + 2);
    c.set(3, byte);
    for (const std::uint32_t bit : {0x80u, 0x20u, 0x10u}) {
        const bool set = (byte & bit) != 0;
        c.set(0, set ? 0xffffffffu : 0u);
        compare(c, set ? -1 : 0);
        if (set) return label::club_end;
    }
    return 0x101a7f7c;
}
