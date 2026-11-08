// MATCH_MAN.CPP match block (match_man_match.hpp): 0x1020d7d0, the lineup pick.
#include "match_man_adapter.hpp"
#include "match_man_match.hpp"
#include "match_man_match_detail.hpp"

namespace cm::aot::match_man {
namespace {
using guest::compare;
using guest::compare_logical;
using guest::Fpu;
using guest::Frame;
using guest::Kept;
using guest::nonzero;
using guest::nonzero_logical;
constexpr std::int32_t s8(std::uint32_t value) { return std::int8_t(value); }
constexpr std::int32_t s16(std::uint32_t value) { return std::int16_t(value); }

// Offsets of the pool of doubles at TOC -0x5278 (named by role where the code shows it).
namespace k {
constexpr std::uint32_t unit = 0x0, scale = 0x8, bias = 0x90, reduction = 0x20, cap_weight = 0x40, grade = 0x78,
                        penalty = 0x80, offset_a = 0x88, coefficient = 0x100, full_weight = 0x178, factor_a = 0x1c0,
                        step = 0x200, factor_b = 0x210, ceiling = 0x260, floor_value = 0x268, ceiling_b = 0x270,
                        no_favourite = 0x278, shape_a = 0x280, shape_b = 0x288, shape_c = 0x290, slope_a = 0x298,
                        slope_b = 0x2a0;
}
// Stack: outgoing arguments 0x38.., assertion buffers 0x58/0x60, score table (7 groups x 5) at 0x80,
// double temporaries 0x10c-0x120.
namespace local {
constexpr std::uint32_t table = 0x80, best = 0x124, mode = 0x128, threshold = 0x12c, group_offset = 0x130;
constexpr std::uint32_t persons = 0x1fc, count = 0x203;
}
}

// 0x1020d7d0. Frame 0x1e0; f23-f31 saved at -0x48 and r13-r31 at -0x9c (the helper 0x107737f4
// and stmw). r15 the side, r16 `team`, r19 `mode`, r27 the pool at TOC -0x5278. `persons` and
// `count` are stored in the caller's linkage area (0x1fc, 0x203).
std::uint32_t pick_lineup(Cpu& c, std::uint32_t side, std::uint32_t persons, std::uint32_t count, std::uint32_t team,
                          std::uint32_t mode) {
    constexpr std::uint32_t entry = 0x1020d7d0;
    using namespace lineup_entries;
    c.set(0, c.lr());
    Frame frame(c, entry, 0x1e0, {.first_gpr = 13, .gpr_offset = -0x9c, .first_fpr = 23, .fpr_offset = -0x48});
    Fpu fpu(c);
    const auto pool_address = c.toc(-0x5278);
    const RandomStream stream{c.toc(-0x7b84), c.toc(-0x7b90), c.toc(-0x7b88), c.toc(-0x7b8c)};
    c.put32(frame.at(local::persons), persons);
    c.put8(frame.at(local::count), count);
    Kept<27> r27(c, pool_address);
    Kept<15> r15(c, side);
    Kept<16> r16(c, team);
    Kept<18> r18(c, stream.seed_slot);
    Kept<19> r19(c, mode);
    Kept<17> r17(c, stream.cursor_slot);
    Kept<13> r13(c, c.toc(-0x7a54));
    Kept<14> r14(c, c.r(14));
    Kept<20> r20(c, c.r(20));
    Kept<21> r21(c, c.r(21));
    Kept<22> r22(c, c.r(22));
    Kept<23> r23(c, c.r(23));
    Kept<24> r24(c, c.r(24));
    Kept<25> r25(c, c.r(25));
    Kept<26> r26(c, c.r(26));
    Kept<28> r28(c, c.r(28));
    Kept<29> r29(c, c.r(29));
    Kept<30> r30(c, c.r(30));
    Kept<31> r31(c, c.r(31));
    const auto pool = [&](std::uint32_t at) { return c.f64(pool_address + at); };
    const auto registry = [&] { return c.u32(c.toc(-0x7a6c)); };
    const auto club = [&] { return c.u32(side + lineup_side::club); };
    const auto candidates = [&] {
        const auto global = registry();
        return c.u32(global + registry::group_b) + c.u32(global + registry::group_a);
    };
    const auto at = [&](std::uint32_t offset) { return frame.at(std::int32_t(offset)); };
    const auto restore_toc = [&] { c.set(2, c.u32(frame.at(0x14))); };

    c.put32(at(local::best), 0xffffffff);
    c.set_f(30, pool(k::full_weight));
    c.set(3, side);
    call(c, 0x1020c5d0, 0x1020d81c);
    c.set(4, club());
    c.put32(at(local::threshold), 2000);
    c.put32(at(local::mode), c.r(3));
    // At least 2000, else eight times the club's rating (halfword +0x1ec; rlwinm 3,0,28).
    const std::int32_t wanted = std::int32_t(std::uint32_t(c.i16(club() + 0x1ec)) << 3);
    if (compare(c, wanted, 2000) > 0) c.put32(at(local::threshold), std::uint32_t(wanted));

    enum class Path { random, finish_b, human, scored } path;
    const auto match = c.u32(side + lineup_side::match);
    c.set(3, match);
    if (nonzero(c, c.i8(match))) path = Path::random;
    else if (compare(c, s8(r19), 2) != 0) path = Path::finish_b;
    else path = Path::scored;

    if (path == Path::scored && !nonzero(c, c.i8(side + lineup_side::kind))) {
        // A human manager of the club (a person among the last 16 of the table, with a flag byte
        // at +0x6b) has his stored lineup taken. Quirk: the original's "no person" arm after the
        // cmplwi is unreachable.
        const auto person = c.u32(club() + 0x90);
        c.set(4, person);
        if (nonzero_logical(c, person)) {
            const auto people = c.u32(c.toc(-0x7be4));
            const auto id = std::int32_t(c.u32(person));
            c.set({{3, people}, {5, std::uint32_t(id)}});
            const bool valid = !(id < std::int32_t(people) - 16) && !(id >= std::int32_t(people)) && nonzero(c, c.i8(person + 0x6b));
            if (valid) {
                c.set(3, c.u32(r13));
                c.call(registry_check, 0x1020da44);
                if (nonzero(c, s8(c.r(3)))) {
                    c.set({{5, c.u32(club() + 0x90)}, {4, 2}, {3, c.u32(r13)}});
                    c.call(registry_member, 0x1020da60);
                    if (nonzero(c, s8(c.r(3)))) {
                        c.set({{5, club()}, {4, 0}, {3, c.u32(r13)}, {6, c.u32(club() + 0x90)}});
                        c.call(0x10146830, 0x1020da7c);
                        c.set({{4, c.u32(club() + 0x90)}, {3, c.u32(r13)}});
                        c.call(0x101466c0, 0x1020da8c);
                        r16 = c.r(3);
                        path = Path::human;
                    }
                }
            }
        }
    }

    if (path == Path::random) {
        c.set(3, registry());
        r13 = candidates();
        if (compare(c, std::int32_t(std::uint32_t(r13))) == 0) {
            r14 = 0;
        } else {
            advance_stream(c, stream, 0x1020d898, true);
            random_spread(c, fpu, stream, std::int32_t(std::uint32_t(r13)), pool_address,
                          {at(0x11c), at(0x114), at(0x10c), 0x1020d960, true, 14});
        }
        c.put32(at(local::best), std::uint32_t(s16(r14)));
    } else if (path == Path::scored) {
        const auto table = at(local::table);
        r28 = table;
        c.put32(at(local::group_offset), 0);
        r21 = 0;
        r13 = 0;
        for (; compare(c, std::int32_t(std::uint32_t(r21)), 7) < 0; r21 = r21 + 1, r13 = r13 + 0x14) {
            const auto group = std::uint32_t(r21);
            // The jump table at TOC -0x5280 sets the group's bit (1 << group) in r14.
            c.set(3, c.u32(c.toc(-0x5280) + c.u32(at(local::group_offset))));
            r14 = 1u << group;
            r20 = 0;
            r25 = 0;
            for (; compare(c, std::int32_t(std::uint32_t(r20)), 5) < 0; r20 = r20 + 1, r25 = r25 + 4) {
                const auto slot = std::uint32_t(r20);
                r24 = r13 + r25;
                c.put32(table + r24, 0);
                if (group == 0) {
                    // Group 0 only scores its middle slot, with no slot bit.
                    if (slot != 2) continue;
                    r23 = 0;
                } else {
                    static constexpr std::uint32_t bits[5] = {0x80, 0x100, 0x200, 0x400, 0x800};
                    r23 = bits[slot];
                }
                r29 = std::uint32_t(r14) | std::uint32_t(r23);
                r31 = c.u32(at(local::persons));
                r19 = 0;
                r30 = std::uint32_t(s8(c.u8(at(local::count))));
                r18 = 0xffffd8f0;  // -10000
                r17 = 0xffffd8f0;
                r22 = 0;
                r26 = 0;
                // The two best ratings of the candidates for this group and slot.
                for (; compare(c, std::int32_t(std::uint32_t(r22)), std::int32_t(std::uint32_t(r30))) < 0;
                     r26 = r26 + 4, r31 = r31 + 4, r22 = r22 + 1) {
                    const auto who = c.u32(r31);
                    c.set({{3, who}, {4, 0xffffffff}, {7, r16}, {9, side}, {6, std::uint32_t(s16(r29))}, {5, 0},
                           {8, club()}, {10, c.u32(side + lineup_side::manager)}});
                    c.put32(at(0x38), 0xffffffff);
                    for (unsigned word = 0x3c; word <= 0x50; word += 4) c.put32(at(word), 0);
                    c.set(4, c.u32(who + 0x14));
                    call(c, 0x10211e70, 0x1020dbe4);
                    std::int32_t rating = std::int32_t(c.r(3));
                    if (compare(c, rating, -10000) < 0) { rating = -10000; c.set(3, std::uint32_t(rating)); }
                    const auto seen = s8(r19);
                    const auto best = std::int32_t(std::uint32_t(r18)), second = std::int32_t(std::uint32_t(r17));
                    compare(c, seen, 1);
                    if (seen == 0) {
                        r18 = std::uint32_t(rating);
                        r19 = std::uint32_t(r19) + 1;
                    } else if (seen == 1) {
                        compare(c, rating, best);
                        if (rating > best) { r17 = std::uint32_t(best); r18 = std::uint32_t(rating); }
                        else r17 = std::uint32_t(rating);
                        r19 = std::uint32_t(r19) + 1;
                    } else {
                        // Two or more seen (or a negative count): keep the top two.
                        compare(c, rating, best);
                        if (rating > best) { r17 = std::uint32_t(best); r18 = std::uint32_t(rating); }
                        else { compare(c, rating, second); if (rating > second) r17 = std::uint32_t(rating); }
                    }
                }
                std::int32_t total = std::int32_t(std::uint32_t(r18) + std::uint32_t(r17));
                compare(c, s8(r30), 11);
                c.put32(table + r24, std::uint32_t(total));
                if (s8(r30) < 11 && group == 6) {
                    // Few candidates: the group's score is halved (srawi 1 plus the sign: toward zero).
                    c.compare(0, std::int32_t(group), 6);
                    c.put32(table + r24, std::uint32_t(total / 2));
                }
            }
            c.put32(at(local::group_offset), c.u32(at(local::group_offset)) + 4);
        }
        // With fewer than 11 candidates the paired groups share their lower score.
        if (s8(c.u8(at(local::count))) < 11) {
            for (const auto [a, b] : {std::pair{0x94u, 0xa4u}, {0xa8u, 0xb8u}, {0xbcu, 0xccu}}) {
                const auto low = std::min(c.i32(at(a)), c.i32(at(b)));
                c.put32(at(a), std::uint32_t(low));
                c.put32(at(b), std::uint32_t(low));
            }
        }
        // One double per candidate record, to keep the scores.
        c.set({{3, candidates()}, {4, 8}});
        c.call(0x10775200, 0x1020dd74);
        r31 = c.r(3);
        r25 = 0;
        r14 = at(local::table);
        r23 = std::uint32_t(s8(c.u32(at(local::mode))));
        r22 = 0;
        r13 = r31;
        r30 = c.u32(at(local::threshold)) ^ 0x80000000u;
        r20 = 0x43300000;
        const auto convert = [&](std::uint32_t where, std::int32_t value) { return converted(c, fpu, where, value, pool(k::bias)); };
        for (; compare(c, std::int32_t(std::uint32_t(r25)), std::int32_t(candidates())) < 0;
             r22 = r22 + 0x28, r13 = r13 + 8, r25 = r25 + 1) {
            const auto global = registry();
            const auto record = c.u32(global + registry::candidates) + r22;
            std::int32_t value = c.i16(record + candidate::value);
            c.put32(at(0x10c), 0x43300000);
            double value_double = convert(at(0x10c), value);
            c.set_f(1, value_double);
            fpu.fcmp(0, value_double, pool(k::ceiling));
            if (c.cr(0) & 4) value = 3000;
            // The weight of the formation mode: +0x10 for 1, +0x20 for 3 to 5, else +0x18.
            const auto mode_value = s8(r23) == 0 ? 0 : std::int32_t(std::uint32_t(r23));
            (void)mode_value;
            const std::int32_t m = std::int32_t(std::uint32_t(r23));
            std::uint32_t weight_at, site;
            if (m == 2 || m >= 6 || m < 1) { weight_at = record + candidate::weight_b; site = 0x1020df20; }
            else if (m == 1) { weight_at = record + candidate::weight_a; site = 0x1020de50; }
            else { weight_at = record + candidate::weight_c; site = 0x1020deb8; }
            double weight = pool(k::floor_value);
            const double weight_cap = c.f64(weight_at);
            fpu.fcmp(0, weight, weight_cap);
            if (!(weight < weight_cap)) weight = weight_cap;
            const double centre = pool(k::ceiling);
            const double a0 = convert(at(0x114), value);
            c.put32(at(0x10c), 0x43300000);
            c.put32(at(0x110), std::uint32_t(value) ^ 0x80000000u);
            double f0 = fpu.fmul(a0, weight);
            double f1 = fpu.fsub(std::bit_cast<double>(std::uint64_t(0x43300000) << 32 | (std::uint32_t(value) ^ 0x80000000u)), pool(k::bias));
            f1 = fpu.fsub(centre, f1);
            f0 = fpu.multiply_add(site, pool(k::coefficient), f1, f0);
            f0 = fpu.fmul(pool(k::scale), f0);
            double score = fpu.fdiv(f0, centre);  // f31
            c.set_f(31, score);

            c.set({{3, global}, {4, std::uint32_t(c.i16(record))}, {5, 0}});
            c.call(0x103af480, 0x1020df38);
            r26 = c.r(3);
            r21 = c.u32(r26 + 0x588);
            const auto coach = c.u32(side + lineup_side::coach);
            c.set(4, coach);
            const std::uint32_t flags = r21;
            if (nonzero_logical(c, coach)) {
                const auto add_converted = [&](std::uint32_t where, std::int32_t v) {
                    const double d = convert(where, v);
                    score = fpu.fadd(score, d);
                    c.set_f(31, score);
                };
                if (flags & 0x400) add_converted(at(0x10c), c.i8(coach + 0x3c) - 10);
                if (flags & 0x1000) add_converted(at(0x114), 10 - c.i8(coach + 0x2e));
                if (flags & 0x100) add_converted(at(0x11c), c.i8(coach + 0x3f) - 10);
                // The coach's favourite (a registry word indexed by his byte at +0x43, 1 to 9).
                const auto preference = c.i8(coach + 0x43);
                std::uint32_t favourite = 0;
                if (preference > 0 && preference < 10) favourite = c.u32(registry() + 4 * std::uint32_t(preference) + registry::preferred);
                if (favourite != 0 && std::int32_t(favourite) == c.i32(r26 + 0x1880)) {
                    const auto size = s8(c.u8(at(local::count)));
                    if (size >= 11) {
                        const double sized = convert(at(0x10c), size);
                        double share = fpu.fdiv(sized, pool(k::coefficient));
                        double limited = fpu.fsub(share, pool(k::step));
                        double cap = pool(k::ceiling_b);
                        fpu.fcmp(0, cap, limited);
                        if (!(cap < limited)) cap = limited;
                        score = fpu.fmul(score, cap);
                    } else {
                        score = fpu.fmul(score, pool(k::cap_weight));
                    }
                } else {
                    score = fpu.fmul(score, pool(k::no_favourite));
                }
                c.set_f(31, score);
            }

            // The slot loop: each of the 11 slots adds its category's weight to the counters.
            const double start = double(c.f32(c.toc(-0x527c) + 0xc));
            double f26 = start, f27 = start, f28 = start, f29 = start;
            const auto set_counters = [&] { c.set_f(26, f26); c.set_f(27, f27); c.set_f(28, f28); c.set_f(29, f29); };
            set_counters();
            r29 = 0;
            r28 = 0;
            for (; compare(c, std::int32_t(std::uint32_t(r28)), 11) < 0; r28 = r28 + 1) {
                const std::uint32_t slot_index = std::uint32_t(s8(r28));
                c.set({{3, r26}, {4, slot_index}, {5, 0}});
                c.call(0x10111240, 0x1020e0c8);
                r19 = c.r(3);
                c.set({{3, r26}, {4, slot_index}, {5, 1}});
                c.call(0x10111240, 0x1020e0dc);
                r18 = c.r(3);
                r24 = 0xffffffff;
                std::uint32_t category = 0;
                if (nonzero_logical(c, c.u32(side + lineup_side::coach))) {
                    c.set({{3, r26}, {4, slot_index}});
                    c.call(person_rating, 0x1020e0fc);
                    r17 = c.r(4);
                    r16 = c.r(3);
                    c.set({{3, r26}, {4, slot_index}});
                    c.call(person_form, 0x1020e110);
                    category = c.r(3);
                    const auto coach_now = [&] { return c.u32(side + lineup_side::coach); };
                    const std::uint32_t low = r17;
                    if ((low & 0x8000) && !(flags & 0x100)) {
                        const double v = convert(at(0x10c), c.i8(coach_now() + 0x3f));
                        const double add = fpu.multiply_add(0x1020e16c, v, pool(k::factor_a), pool(k::penalty));
                        score = fpu.fadd(score, add);
                    }
                    if (low & 0x200000) {
                        const double v = convert(at(0x10c), c.i8(coach_now() + 0x35));
                        score = fpu.fadd(score, fpu.fsub(v, pool(k::offset_a)));
                    }
                    if (low & 0x800000) {
                        const double v = convert(at(0x114), c.i8(coach_now() + 0x2e));
                        score = fpu.fadd(score, fpu.multiply_add(0x1020e220, v, pool(k::factor_b), pool(k::coefficient)));
                    }
                    if (category & 4) {
                        c.put32(at(0x10c), 0x43300000);
                        const double v = convert(at(0x10c), c.i8(coach_now() + 0x3a));
                        score = fpu.fadd(score, fpu.multiply_add(0x1020e260, pool(k::reduction), v, pool(k::scale)));
                    }
                    c.set_f(31, score);
                }
                (void)category;
                // The slot's category from the two masks (halfwords).
                const std::uint32_t first = std::uint32_t(s16(r19)), second = std::uint32_t(s16(r18));
                std::uint32_t row = 0;
                const auto add = [&](double& counter, double amount) { counter = fpu.frsp(fpu.fadd(amount, counter)); set_counters(); };
                if (first & 1) { row = 0; r24 = 2; }
                else if (first & 2) { add(f29, pool(k::unit)); row = 1; }
                else if (first & 4) { add(f29, pool(k::unit)); row = 2; }
                else if (first & 8) { add(f28, pool(k::unit)); row = 3; }
                else if (first & 0x10) row = 4;
                else if (first & 0x20) { add(f27, pool(k::unit)); row = 5; }
                else { add(f26, pool(k::unit)); row = 6; }
                if (second & 2) { add(f29, pool(k::factor_b)); row = 1; }
                else if (second & 4) { add(f29, pool(k::factor_b)); row = 2; }
                else if (second & 8) { add(f28, pool(k::factor_b)); row = 3; }
                else if (second & 0x10) row = 4;
                else if (second & 0x20) { add(f27, pool(k::factor_b)); row = 5; }
                else if (second & 0x40) { add(f26, pool(k::factor_b)); row = 6; }
                if (s8(r24) == -1) {
                    if (first & 0x80) r24 = 0;
                    else if (first & 0x100) r24 = 1;
                    else if (first & 0x200) r24 = 2;
                    else if (first & 0x400) r24 = 3;
                    else r24 = 4;
                }
                r29 = std::uint32_t(r29) + c.u32(r14 + row * 0x14 + (std::uint32_t(r24) << 2));
                c.set(3, row * 0x14);
            }
            // The shape of the counters: powers of the grade, and the mode's adjustment.
            const double f23 = fpu.fsubs(f29, f26);
            c.set_f(23, f23);
            double f24 = std::fabs(fpu.multiply_add(0x1020e428, f27, pool(k::factor_b), f23));
            c.set_f(24, f24);
            c.set_f(1, f28);
            c.set_f(2, pool(k::grade));
            c.call(0x10780fc0, 0x1020e434);
            restore_toc();
            const double f25 = c.f(1);
            c.set_f(25, f25);
            c.set_f(2, pool(k::grade));
            c.set_f(1, f27);
            c.call(0x10780fc0, 0x1020e448);
            restore_toc();
            f28 = c.f(1);
            c.set_f(28, f28);
            c.set_f(2, pool(k::grade));
            c.set_f(1, f26);
            c.call(0x10780fc0, 0x1020e45c);
            restore_toc();
            f26 = fpu.multiply_add(0x1020e464, pool(k::grade), c.f(1), f28);
            c.set_f(26, f26);
            c.set_f(1, f29);
            c.call(0x10780fc0, 0x1020e470);
            restore_toc();
            const double f1_pow = c.f(1);
            double f2 = fpu.fsubs(f23, f27);
            compare(c, m, 3);
            double f4 = fpu.multiply_add(0x1020e484, pool(k::grade), f1_pow, f26);
            f4 = fpu.fsub(f4, f25);
            const double f3 = fpu.fsub(pool(k::shape_a), f4);
            f2 = fpu.frsp(fpu.multiply_add(0x1020e498, f2, f24, f3));
            f2 = fpu.frsp(fpu.multiply_add(0x1020e4a0, pool(k::shape_c), f2, pool(k::shape_b)));
            if (m == 3) {
                score = fpu.fmul(score, fpu.multiply_add(0x1020e4e4, pool(k::slope_a), f2, pool(k::unit)));
            } else if (m == 1) {
                score = fpu.fmul(score, fpu.multiply_add(0x1020e4d0, pool(k::slope_a), f2, pool(k::unit)));
            } else if (m == 4) {
                score = fpu.fmul(score, fpu.multiply_add(0x1020e4f8, pool(k::slope_b), f2, pool(k::unit)));
            }
            c.set_f(31, score);
            // The candidate's weighted score against the best so far.
            c.put32(at(0x10c), 0x43300000);
            c.put32(at(0x110), std::uint32_t(r29) ^ 0x80000000u);
            const double sum = fpu.fsub(std::bit_cast<double>(std::uint64_t(0x43300000) << 32 | (std::uint32_t(r29) ^ 0x80000000u)), pool(k::bias));
            const auto best_index = c.u32(at(local::best));
            c.put32(at(0x118), r30);
            c.put32(at(0x114), 0x43300000);
            const double limit = fpu.fsub(std::bit_cast<double>(std::uint64_t(0x43300000) << 32 | std::uint32_t(r30)), pool(k::bias));
            double result = fpu.fmul(score, fpu.fdiv(sum, limit));
            c.put64(r13, std::bit_cast<std::uint64_t>(result));
            compare(c, std::int32_t(best_index), -1);
            bool take = best_index == 0xffffffff;
            if (!take) {
                fpu.fcmp(0, result, c.f(30));
                take = c.cr(0) & 4;
            }
            if (take) {
                c.set_f(30, result);
                c.put32(at(local::best), r25);
            }
        }
        if (nonzero_logical(c, r31)) {
            c.set(3, r31);
            c.call(0x10775170, 0x1020e588);
        }
    }

    // The chosen candidate is registered (0x103af6c0, unless the mode skipped the pick), then
    // the club's lineup record is read (0x103af570).
    if (path == Path::random || path == Path::scored) {
        c.set({{3, registry()}, {4, club()}, {5, std::uint32_t(s8(c.u32(at(local::best))))},
               {6, c.u8(side + lineup_side::kind)}});
        c.call(0x103af6c0, 0x1020e5a4);
    }
    if (path != Path::human) {
        c.set({{3, registry()}, {4, club()}, {5, c.u8(side + lineup_side::kind)}});
        c.call(0x103af570, 0x1020e5b8);
        r16 = c.r(3);
    }

    // The lineup record is copied into the match's area for this side. The original copies it
    // piece by piece, skipping padding bytes, so the same pieces are copied here.
    r13 = c.u32(side + lineup_side::match);
    r14 = std::uint32_t(c.i8(side + lineup_side::index)) * lineup_match::side_stride;
    r17 = r13 + r14;
    const std::uint32_t area = r17, record = r16;
    const std::uint32_t shift = lineup_match::lineups;
    const auto copy_call = [&](std::uint32_t src, std::uint32_t size, std::uint32_t back) {
        c.set({{3, area + shift + src}, {4, record + src}, {5, size}});
        c.call(0x10771110, back);
    };
    const auto copy_words = [&](std::uint32_t src, std::uint32_t end) {
        for (auto offset = src; offset < end; offset += 4) c.put32(area + shift + offset, c.u32(record + offset));
    };
    const auto copy_halves = [&](std::uint32_t src, std::uint32_t end) {
        for (auto offset = src; offset < end; offset += 2) c.put16(area + shift + offset, c.u16(record + offset));
    };
    copy_call(0x0, 0x33, 0x1020e5e0);
    copy_call(0x33, 0x32, 0x1020e5f0);
    copy_call(0x65, 0xc8, 0x1020e600);
    copy_words(0x12e, 0x142);
    copy_halves(0x142, 0x144);
    copy_call(0x144, 0xb, 0x1020e640);
    copy_words(0x150, 0x164);
    copy_halves(0x164, 0x166);
    copy_halves(0x166, 0x586);
    copy_words(0x588, 0x5b4);
    copy_halves(0x5b4, 0x5b8);
    copy_call(0x5b8, 0xb, 0x1020e724);
    copy_words(0x5c4, 0x5ec);
    copy_halves(0x5ec, 0x5f0);
    copy_words(0x5f0, 0xf38);
    copy_words(0xf38, 0x1880 + 4);
    for (std::uint32_t offset = 0x1884; offset < 0x1887; ++offset) c.put8(area + shift + offset, c.u8(record + offset));
    copy_words(0x1888, 0x18e0);
    const std::uint32_t base = r13 + 0x10000 + r14;
    c.set({{6, base}, {7, base}, {8, area + 0x81f2}});
    if (nonzero_logical(c, r16) && nonzero_logical(c, c.u32(side + lineup_side::tactics))) {
        c.set({{3, c.u32(side + lineup_side::tactics)}, {4, r16}, {5, 0x18e0}});
        c.call(0x10739ef0, 0x1020e854);
    } else {
        const auto strings = c.toc(-0x5274);
        guest::report_assertion(c, {.at = 0x1020e86c, .file = strings, .format = strings + 0xe, .title = strings + 0x1b,
                                    .name = at(0x60), .extension = at(0x58), .line = 0xc1b});
        c.set(3, 0xffffffff);
        c.call(0x1076f7d0, 0x1020e8bc);
    }
    c.set(3, side);
    call(c, lineup_entries::side_cleanup, 0x1020e8c4);
    return guest::result(c, 0);
}
}
