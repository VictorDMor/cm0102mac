// MATCH_ENG.CPP, contact: the contact between two players (0x101f5350). Each player's strength for the contact is a
// float scaled by his fitness, his class and what the other does; chance then decides whether the contact injures
// someone or is reported, and as which kind (1..11) through 0x101f6940.
#include "match_eng_contact.hpp"
#include "match_eng_shots.hpp"

namespace cm::aot::match_eng {
using namespace guest;
using namespace shots_detail;

namespace {
constexpr std::uint32_t record_of = 0x1018c340, descriptor_of = 0x1018c180, level_of = 0x1018c270;
std::uint32_t flip(std::uint32_t value) { return value ^ 0x80000000u; }
// How the other player's action changes this player's strength (the table behind 0x101f5ab4): the double the other's
// float is divided or multiplied by, and what happens to the quality byte.
enum class Quality { none, doubled, scaled };
struct Change { std::uint32_t at; bool multiply; Quality quality; unsigned scratch; };
Change change_for(std::int32_t code) {
    switch (code) {
        case 45: return {0x48, false, Quality::none, 0};
        case 42: return {0x130, false, Quality::none, 1};
        case 47: return {0x2b0, false, Quality::doubled, 2};
        case 43: return {0x2b8, true, Quality::none, 3};
        case 44: return {0x2c0, false, Quality::doubled, 4};
        case 19: case 33: return {0x48, false, Quality::none, 5};
        case 34: return {0xc0, true, Quality::scaled, 6};
        case 41: return {0xc0, true, Quality::none, 7};
        case 1: return {0x2d0, false, Quality::none, 8};
        case 21: return {0x2d8, false, Quality::none, 9};
        case 46: return {0x48, false, Quality::none, 10};
        case 12: return {0x2e0, false, Quality::none, 11};
        default: return {0x30, false, Quality::none, 12};
    }
}
}

// 0x101f5350: players a and b (either may be 0) with their action codes; `flag` asks for a report of the contact itself.
void describe_contact(Cpu& c, std::uint32_t match, std::uint32_t player_a, std::uint32_t action_a, std::uint32_t player_b,
                      std::uint32_t action_b, std::uint32_t flag) {
    Frame frame(c, 0x101f5350, 0xc0, {.first_gpr = 21, .gpr_offset = -0x5c, .first_fpr = 27});
    const auto caller = frame.caller();
    c.put16(caller + 0x22, action_a);
    c.put16(caller + 0x2a, action_b);
    c.put8(caller + 0x2f, flag);
    keep(c, {{29, match}, {27, player_a}, {26, player_b}, {25, c.toc(-0x7ba0)}, {28, c.toc(-0x52c8)}, {30, c.toc(-0x52c4)}});
    const auto pool = c.r(28), table = c.r(30), names = c.r(25);
    const auto strength = [&](unsigned k) { return frame.at(0x40 + 4 * k); };      // the two floats
    const auto quality = [&](unsigned k) { return frame.at(0x38 + k); };           // the two quality bytes
    const auto descriptor = [&](unsigned k) { return frame.at(0x3a + k); };        // the two descriptor bytes
    const auto action = [&](unsigned k) { return frame.at(0x3c + 2 * k); };        // the two action codes
    const auto players = [&](unsigned k) { return frame.at(0x48 + 4 * k); };       // the two players
    c.put16(descriptor(0), c.u16(c.r(2) - 0x3914));
    keep(c, {{22, descriptor(0)}});
    c.set(5, c.u32(c.r(2) - 0x52dc));
    c.set(6, c.u32(c.r(5)));
    c.put32(strength(0), c.r(6));
    c.put32(strength(1), c.u32(c.r(5) + 4));
    keep(c, {{21, strength(0)}});
    Fpu fp(c);
    const auto P = [&](std::uint32_t at) { return double(c.f32(pool + at)); };
    const auto D = [&](std::uint32_t at) { return c.f64(table + at); };
    const auto magic = [&] { const double value = D(0x60); c.set_f(1, value); return value; };

    // Each player's quality byte: a draw of a draw of 102, and with one chance in a few a draw of a draw of 76 instead.
    c.set(7, draw(c, 0x66, 0x101f53b8));
    c.put8(quality(0), u(s8(draw(c, c.r(7), 0x101f53c4))));
    c.set(7, c.u8(quality(0)));
    c.set(8, u(s8(c.r(7))));
    if (compare(c, s8(c.r(8)), 0x32) > 0) {
        c.set(7, draw(c, 3, 0x101f53e8));
        if (compare(c, std::int32_t(c.r(7)), 0) != 0) {
            const auto better = draw(c, 0x4c, 0x101f53fc);
            c.set(5, draw(c, better, 0x101f5408));
            c.set(6, u(s8(c.r(5))));
            c.put8(quality(0), c.r(6));
        }
    }
    c.set(7, draw(c, 0x66, 0x101f541c));
    c.set(6, draw(c, c.r(7), 0x101f5428));
    c.set(8, u(s8(c.r(6))));
    c.put8(quality(1), c.r(8));
    c.set(7, u(s8(c.u8(quality(1)))));
    if (compare(c, std::int32_t(c.r(7)), 0x32) > 0) {
        if (compare(c, std::int32_t(draw(c, 3, 0x101f544c)), 0) != 0) {
            c.set(7, draw(c, 0x4c, 0x101f5460));
            c.set(6, draw(c, c.r(7), 0x101f546c));
            c.set(5, u(s8(c.r(6))));
            c.put8(quality(1), c.r(5));
        }
    }

    // One player's strength: his descriptor from the names object, the quality bytes, fitness and class.
    const auto shape = [&](unsigned k, std::uint32_t player) {
        const bool second = k == 1;
        const auto site = [&](std::uint32_t first_site, std::uint32_t second_site) { return second ? second_site : first_site; };
        c.set({{3, c.u32(names)}, {4, c.u32(player + 0x6c)}});
        call(c, record_of, site(0x101f548c, 0x101f5650));
        keep(c, {{24, c.r(3)}});
        if (compare_logical(c, c.r(24)) != 0) {
            c.set_f(1, P(0x11c));
            c.put_f32(strength(k), float(fp.fmuls(P(0x11c), c.f32(strength(k)))));
            c.set({{3, c.u32(names)}, {4, c.u8(c.r(24))}});
            call(c, descriptor_of, site(0x101f54b4, 0x101f5678));
            c.put8(descriptor(k), c.r(3));
            const auto roll = draw(c, 0x19, site(0x101f54c4, 0x101f5688));
            c.set(second ? 7 : 6, roll);
            if (compare(c, std::int32_t(roll), 0) != 0) {
                const auto before = c.u8(quality(k));
                if (!second) c.set(7, before);
                else c.set(7, before * 2);
                c.put8(quality(k), u(s8(before * 2)));
            }
        }
        const double fitness_now = c.f32(player + 0x50);
        const double scaled = fp.multiply_add(site(0x101f54ec, 0x101f56b0), fitness_now, P(0x54), P(0xc8));
        c.set_f(1, scaled);
        fp.fcmp(0, scaled, P(0xdc));
        double floor_value;
        if (c.cr(0) & 4) {
            c.set_f(1, P(0x54));
            floor_value = fp.multiply_add(site(0x101f5508, 0x101f56cc), fitness_now, P(0x54), P(0xc8));
        } else floor_value = P(0xdc);
        c.set_f(second ? 28 : 29, floor_value);
        c.set_f(1, c.f32(strength(k)));
        c.put_f32(strength(k), float(fp.fmuls(c.f32(strength(k)), fp.fdivs(P(0xcc), floor_value))));
        // The class byte of the record: the second player's test reads the first player's record (a quirk of the original).
        const auto test_record = second ? c.u32(player_a + 0x70) : c.u32(player + 0x70);
        const auto test_class = s8(c.u8(test_record + 0x2d));
        if (second) c.set({{7, test_record}, {6, c.u8(test_record + 0x2d)}});
        else c.set({{8, test_record}, {7, c.u8(test_record + 0x2d)}, {3, u(test_class)}});
        const auto record = c.u32(player + 0x70);
        const double strength_value = c.f32(strength(k));
        const auto grade = s8(c.u8(record + 0x2d));
        if (test_class > 10) {
            if (second) c.set({{6, record}, {7, flip(u(grade))}});
            else c.set({{4, record}, {3, c.u8(record + 0x2d)}, {23, u(grade)}, {5, flip(u(grade))}});
            const double scaled_grade = from_int_single(c, fp, frame.at(0x58), u(grade), magic());
            const double product = fp.fmuls(strength_value, scaled_grade);
            c.set_f(1, product);
            c.put_f32(strength(k), float(fp.fdivs(product, P(0x28))));
        } else {
            const auto lowered = u(grade - 10);
            if (second) c.set({{7, flip(lowered)}, {5, c.u8(record + 0x2d)}, {6, 0x43300000}});
            else c.set({{7, flip(lowered)}, {6, c.u8(record + 0x2d)}});
            const double lowered_grade = from_int_single(c, fp, frame.at(0x58), lowered, magic());
            c.set_f(1, lowered_grade);
            const double ratio = fp.fdivs(lowered_grade, P(0x28));
            c.put_f32(strength(k), float(fp.fmuls(strength_value, fp.fadds(P(0x10), ratio))));
        }
        // A level of the names object for him (0: none) turns into a further factor, capped by the pool's limit.
        c.set({{3, c.u32(names)}, {4, c.u32(player + 0x6c)}});
        call(c, level_of, site(0x101f55d0, 0x101f5794));
        c.set(7, c.r(3));
        if (!second) c.set(6, u(s8(c.r(7))));
        if (compare(c, s8(c.r(7)), 0) != 0) {
            c.set({{3, c.u32(names)}, {4, c.u32(player + 0x6c)}});
            call(c, level_of, site(0x101f55ec, 0x101f57b0));
            const auto level = s8(c.r(3));
            if (second) c.set({{8, u(level)}, {7, flip(u(level))}, {5, 0x43300000}});
            else c.set({{5, c.r(3)}, {6, 0x43300000}});
            const double conv = from_int(c, fp, frame.at(0x58), u(level), magic());
            c.set_f(1, D(0x90));
            double limit = fp.frsp(fp.multiply_add(site(0x101f5618, 0x101f57dc), D(0x90), conv, D(0x1b8)));
            fp.fcmp(0, limit, D(0x40));
            if (c.cr(0) & 4) limit = P(0x108);
            c.set_f(second ? 30 : 31, limit);
            c.put_f32(strength(k), float(fp.fmuls(c.f32(strength(k)), limit)));
        }
    };
    if (compare_logical(c, player_a) != 0) shape(0, player_a);
    if (compare_logical(c, player_b) != 0) shape(1, player_b);

    if (compare_logical(c, player_a) != 0) {
        c.set_f(1, c.f32(player_a + 0x50));
        fp.fcmp(0, c.f(1), P(0xd8));
        if (c.cr(0) & 8) {
            enter(c, 0x101f5830, {match, UINT32_MAX, 0, player_a, 0});
            apply_contact_injury(c, match, UINT32_MAX, 0, player_a, 0);
            return;
        }
    }
    c.set(6, c.u8(caller + 0x2f));
    if (s8(c.u8(caller + 0x2f)) != 0 && player_a != 0) {
        // The first player alone: strength against a draw, then the kind from his descriptor, else a draw.
        keep(c, {{23, draw(c, 0x10000 - 0x3cb0, 0x101f5858)}});
        c.set_f(1, P(0x28));
        c.put64(frame.at(0x58), fp.fctiwz(0, fp.fmuls(P(0x28), c.f32(strength(0)))));
        c.set(6, c.u32(frame.at(0x5c)));
        if (compare(c, std::int32_t(c.r(23)), std::int32_t(c.r(6))) < 0) {
            const auto backs = [&](std::uint32_t kind) {
                switch (kind) { case 1: return 0x101f58c8u; case 3: return 0x101f58e4u; case 4: return 0x101f5900u; case 2: return 0x101f591cu; default: return 0x101f5938u; }
            };
            const auto d = s8(c.u8(descriptor(0)));
            const auto pull = [&](std::uint32_t kind, std::uint32_t back) {
                enter(c, back, {match, kind, c.u8(quality(0)), player_a, 0});
                apply_contact_injury(c, match, kind, c.u8(quality(0)), player_a, 0);
            };
            if (d >= 1 && d <= 5) pull(u(d), backs(u(d)));
            else {
                // The compare chain of the original: a draw outside 0..10 ends with the last compare's condition.
                const auto roll = std::int32_t(draw(c, 11, 0x101f5944));
                if (compare(c, roll, 8) >= 0) {
                    const auto order = compare(c, roll, 0xa);
                    if (order == 0) pull(5, 0x101f5a08);
                    else if (order < 0) pull(2, 0x101f59ec);
                } else if (compare(c, roll, 3) >= 0) {
                    if (compare(c, roll, 6) >= 0) pull(4, 0x101f59d0);
                    else pull(3, 0x101f59b4);
                } else if (compare(c, roll, 0) >= 0) pull(1, 0x101f5998);
            }
        }
        return;
    }
    // Both players in turn: what the other player's action does to his strength, then chance and the kind of report.
    c.put32(players(0), player_a);
    c.put32(players(1), player_b);
    c.put16(action(0), c.i16(caller + 0x22));
    c.put16(action(1), c.i16(caller + 0x2a));
    const auto B38 = frame.at(0x38), B3a = frame.at(0x3a), B3c = frame.at(0x3c), B40 = frame.at(0x40), B48 = frame.at(0x48);
    const auto holder = [&] { return c.u32(match + pending_player); };
    Kept<31> index(c, 0);
    const auto more = [&] { c.set(7, u(s8(index))); return compare(c, s8(index), 2) < 0; };
    for (; more(); index = index + 1) {
        const std::uint32_t i = u(s8(index)), four_i = i * 4, other = u(s8(index) == 0), four_o = other * 4;
        const std::uint32_t leading = std::uint32_t(std::countl_zero(i));
        const auto player_of = [&](unsigned k) { return c.u32(players(k)); };
        const auto strength_i = strength(unsigned(i));
        c.set({{6, i}, {5, four_i}, {6, B48}, {7, player_of(unsigned(i))}});
        const auto mine = player_of(unsigned(i));
        if (mine == 0) continue;
        c.set({{6, i}, {7, B48}});
        c.set({{7, holder()}, {8, match + 0x10000}});
        const auto others_action = [&] { return std::int16_t(c.i16(action(other))); };
        bool modified = false;
        if (mine == holder()) {
            c.set({{7, other}, {6, B48}});
            if (player_of(other) != 0) {
                c.set({{7, i}, {7, other * 2}, {6, B3c}, {5, u(std::int32_t(others_action()))}});
                const auto code = std::int32_t(others_action());
                const auto how = (u(code) > 0x2f) ? change_for(-1) : change_for(code);
                if (u(code) <= 0x2f) c.set({{6, c.u32(c.toc(-0x52e0) + u(code) * 4)}, {7, u(code) * 4}});
                else c.set({{6, B3c}, {7, other * 2}});
                const auto other_player = player_of(other);
                const std::uint32_t q_before = c.u8(quality(unsigned(i)));
                const double other_float = c.f32(other_player + 0xdc);
                const double w = c.f32(strength_i);
                c.set_f(2, w);
                c.set_f(1, other_float);
                double factor = how.multiply ? fp.fmul(other_float, D(how.at)) : fp.fdiv(other_float, D(how.at));
                factor = fp.frsp(factor);
                c.put_f32(strength_i, float(fp.fmuls(w, factor)));
                if (how.quality == Quality::doubled) c.put8(quality(unsigned(i)), c.u8(quality(unsigned(i))) * 2);
                if (how.quality == Quality::scaled) {
                    const double conv = from_int(c, fp, frame.at(0x58), u(s8(c.u8(quality(unsigned(i))))), magic());
                    c.put64(frame.at(0x50), fp.fctiwz(0, fp.fmul(D(0x2c8), conv)));
                    c.put8(quality(unsigned(i)), c.u32(frame.at(0x54)));
                }
                // The registers the case bodies leave behind (the original's temporaries).
                const std::uint32_t q2 = q_before * 2;
                const std::uint32_t P_o = other_player;
                switch (how.scratch) {
                    case 0: c.set({{6, B48}, {7, P_o}, {8, other}, {9, B40}, {10, four_i}}); break;
                    case 1: c.set({{6, P_o}, {7, B48}, {8, four_o}, {9, B40}, {10, four_i}}); break;
                    case 2: c.set({{3, leading}, {5, q2}, {6, B38}, {7, i}, {8, four_i}}); break;
                    case 3: c.set({{5, P_o}, {6, B48}, {7, four_o}, {8, B40}, {9, four_i}}); break;
                    case 4: c.set({{3, q2}, {4, B38}, {5, i}, {6, B48}, {7, B40}, {8, four_i}}); break;
                    case 5: c.set({{3, other}, {4, B40}, {5, B48}, {6, P_o}, {7, four_i}}); break;
                    case 6: c.set({{5, P_o}, {6, B38}, {7, i}, {8, B40}, {9, four_i}}); break;
                    case 7: c.set({{5, i}, {6, i}, {7, P_o}, {8, B48}, {9, B40}, {10, four_i}}); break;
                    case 8: c.set({{5, four_i}, {6, other}, {7, B48}, {8, P_o}, {9, B40}}); break;
                    case 9: c.set({{5, B40}, {6, leading}, {7, P_o}, {8, four_i}}); break;
                    case 10: c.set({{5, i}, {6, four_o}, {7, P_o}, {8, B40}, {9, four_i}}); break;
                    case 11: c.set({{5, four_i}, {6, B40}, {7, B48}, {8, P_o}}); break;
                    default: c.set({{5, B40}, {6, leading}, {7, P_o}, {8, four_o}}); break;
                }
                modified = true;
            }
        }
        (void)modified;
        c.set({{7, i * 2}, {6, B3c}, {5, u(std::int32_t(c.i16(action(unsigned(i)))))}});
        if (c.i16(action(unsigned(i))) == 5) {
            keep(c, {{24, i}});
            c.set({{6, B48}, {7, mine}});
            const double base = c.f32(mine + 0x7c);
            c.set_f(1, base);
            fp.fcmp(0, base, D(0x98));
            double divisor;
            if (c.cr(0) & 4) {
                c.set({{8, B48}, {7, mine}});
                divisor = c.f32(mine + 0x7c);
                c.set_f(27, divisor);
            } else {
                divisor = D(0x98);
                c.set_f(27, divisor);
            }
            c.set({{4, four_i}, {3, B40}});
            c.set_f(1, P(0x28));
            const double w = c.f32(strength_i);
            c.put_f32(strength_i, float(fp.fmuls(w, fp.fdivs(P(0x28), fp.frsp(divisor)))));
        }
        keep(c, {{23, draw(c, 0x10000 - 0x3cb0, 0x101f5f28)}});
        c.set_f(1, P(0x28));
        c.set({{5, i}, {7, B40}});
        c.put64(frame.at(0x50), fp.fctiwz(0, fp.fmuls(P(0x28), c.f32(strength_i))));
        c.set(6, c.u32(frame.at(0x54)));
        if (compare(c, std::int32_t(c.r(23)), std::int32_t(c.r(6))) >= 0) continue;
        const auto q_i = c.u8(quality(unsigned(i)));
        // A report: 0x101f6940 with the kind, the quality byte, this player and the other (or 0); the scratch
        // registers of the call site follow.
        const auto fire = [&](std::uint32_t kind, std::uint32_t with_other, std::uint32_t back,
                              std::initializer_list<std::pair<unsigned, std::uint32_t>> extras) {
            const auto victim = player_of(unsigned(i));
            enter(c, back, {match, kind, q_i, victim, with_other ? player_of(other) : 0});
            c.set(extras);
            apply_contact_injury(c, match, kind, q_i, victim, with_other ? player_of(other) : 0);
        };
        c.set({{7, i}});
        const auto d_now = std::int32_t(c.i16(action(unsigned(i))));
        if (d_now == 5) {
            c.set({{7, i}, {6, B3a}, {5, c.u8(descriptor(unsigned(i)))}});
            const auto d = s8(c.u8(descriptor(unsigned(i))));
            if (d == 9) { fire(9, 0, 0x101f5fd0, {{24, i}}); return; }
            if (d == 6) { fire(6, 0, 0x101f6000, {}); return; }
            if (d == 10) { fire(10, 0, 0x101f6030, {}); return; }
            switch (draw(c, 5, 0x101f603c)) {
                case 0: case 1: case 2: fire(9, 0, 0x101f6090, {}); break;
                case 3: fire(6, 0, 0x101f60c0, {{8, B48}}); break;
                case 4: fire(10, 0, 0x101f60f0, {{23, B48}}); break;
                default: break;
            }
            continue;
        }
        c.set(6, c.u8(match + shot_state));
        if (s8(c.u8(match + shot_state)) == 4) {
            c.set({{7, four_o}, {6, B48}, {5, player_of(other)}});
            if (player_of(other) != 0) {
                fire(0xb, 1, 0x101f6164, {{8, other}});
                continue;
            }
        }
        c.set({{6, leading}, {7, B48}});
        if (player_of(other) == 0) {
            const auto d = s8(c.u8(descriptor(unsigned(i))));
            c.set({{7, B3a}, {6, c.u8(descriptor(unsigned(i)))}, {5, u(d)}});
            switch (d) {
                case 1: fire(1, 0, 0x101f61f0, {{9, B48}, {10, four_i}}); return;
                case 3: fire(3, 0, 0x101f6220, {{8, B38}}); return;
                case 4: fire(4, 0, 0x101f6250, {{9, four_i}, {10, i}}); return;
                case 2: fire(2, 0, 0x101f6280, {{8, i}}); return;
                case 5: fire(5, 0, 0x101f62b0, {{8, B38}}); return;
                default: break;
            }
            const auto roll = std::int32_t(draw(c, 0xb, 0x101f62bc));
            if (roll == 0 || (roll > 8 && roll < 0xb)) fire(1, 0, 0x101f6320, {});
            else if (roll >= 1 && roll <= 3) fire(3, 0, 0x101f6350, {{9, B48}});
            else if (roll == 4 || roll == 5) fire(4, 0, 0x101f6380, {{8, i}});
            else if (roll == 6 || roll == 7) {
                fire(2, 0, 0x101f63b0, {{8, four_i}});
                fire(5, 0, 0x101f63dc, {});
            } else if (roll == 8) fire(5, 0, 0x101f63dc, {});
            continue;
        }
        c.set({{3, B48}, {5, other}});
        const auto d = s8(c.u8(descriptor(unsigned(i))));
        c.set({{7, B3a}, {4, c.u8(descriptor(unsigned(i)))}});
        if (u(d) <= 9) {
            switch (d) {
                case 1: fire(1, 1, 0x101f646c, {{8, i}, {9, B48}}); return;
                case 2: fire(2, 1, 0x101f6538, {{8, four_o}, {9, i}}); return;
                case 3: fire(3, 1, 0x101f64b0, {}); return;
                case 4: fire(4, 1, 0x101f64f4, {{8, four_o}, {9, four_i}, {10, i}}); return;
                case 5: fire(5, 1, 0x101f657c, {{8, i}, {9, B48}}); return;
                case 6: fire(6, 1, 0x101f65c0, {{8, i}}); return;
                case 7: fire(7, 1, 0x101f6604, {{8, four_o}}); return;
                case 8: fire(8, 1, 0x101f6648, {{8, i}}); return;
                case 9: fire(9, 1, 0x101f668c, {{24, i}, {8, four_o}}); return;
                default: break;
            }
        }
        const auto kind_roll = draw(c, 9, 0x101f6698);
        switch (kind_roll) {
            case 0: fire(1, 1, 0x101f66f4, {{23, i}}); break;
            case 1: fire(2, 1, 0x101f6738, {{24, i}}); break;
            case 2: fire(3, 1, 0x101f677c, {}); break;
            case 3: fire(4, 1, 0x101f67c0, {{8, four_o}}); break;
            case 4: fire(5, 1, 0x101f6804, {{23, four_i}}); break;
            case 5: fire(6, 1, 0x101f6848, {{8, four_o}}); break;
            case 6: fire(7, 1, 0x101f688c, {}); break;
            case 7: fire(8, 1, 0x101f68d0, {{10, i}, {9, other}, {8, four_o}}); break;
            case 8: fire(9, 1, 0x101f6914, {{10, B48}, {9, i}, {8, B48}}); break;
            default: break;
        }
    }
}
}
