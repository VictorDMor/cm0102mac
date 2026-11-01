// MATCH_ENG.CPP, events: the match played minute by minute without the pitch view (0x101db340).
#include "match_eng_events.hpp"
#include <bit>

namespace cm::aot::match_eng {
using namespace guest;
using namespace events_detail;

namespace {
constexpr std::uint32_t entry_point = 0x101db340;
// Routines called (own block: the adjust_players_* family; the others belong to other blocks).
constexpr std::uint32_t usable_at = 0x101d3b50, role_flags_at = 0x10289560, team_at = 0x101d3c50,
                        tendencies_at = 0x101df110, notice_at = 0x101d6710, caution_at = 0x101f1530,
                        by_mode_at = 0x101e1a50, ranked_at = 0x101e1bd0, weighted_at = 0x101e1640, simple_at = 0x101e2100,
                        traits_at = 0x101e22e0, award_at = 0x101f4860, action_at = 0x101f6940, shoot_out_check_at = 0x10494d20,
                        adjust_a_at = 0x101de0a0, adjust_b_at = 0x101de340, adjust_c_at = 0x101de610, adjust_d_at = 0x101de8b0,
                        adjust_e_at = 0x101deb50, adjust_f_at = 0x101dee30, count_a_at = 0x101de040, count_b_at = 0x101de060,
                        count_c_at = 0x101de080;
// Stack locals of the original (sp-relative bytes).
constexpr std::uint32_t l_round = 0x3f, l_first = 0x40, l_attempt = 0x3e, l_sweep = 0x3d, l_sweep_a = 0x3c, l_sweep_b = 0x3b,
                        l_pass = 0x3a, l_weight = 0x39, l_role = 0x38, l_golden = 0x41, l_ended = 0x42, l_shootout = 0x43,
                        l_44 = 0x44, l_45 = 0x45, l_46 = 0x46, l_47 = 0x47, l_48 = 0x48, l_49 = 0x49;
// The match's per-side statistics block (a record of 0x36 bytes per side), addressed by the original as
// match + 0x10000 - low + side * 0x36.
constexpr std::uint32_t stat_base = 0x10000;

class Simulation {
public:
    Simulation(Cpu& cpu, std::uint32_t match_object)
        : c(cpu), frame(cpu, entry_point, 0xe0, {.first_gpr = 14, .first_fpr = 30}), fp(cpu), match(match_object) {}
    void run();

private:
    Cpu& c;
    Frame frame;
    Fpu fp;
    const std::uint32_t match;
    // Registers the original keeps across its calls.
    Held<14> t14{c};
    Held<15> t15{c};
    Held<16> contribution{c};
    Held<17> margin{c};
    Held<18> counter_chance{c};
    Held<19> tries{c};
    Held<20> pass_slot{c};
    Held<21> defender{c};
    Held<22> overtime{c};
    Held<23> scorer{c};
    Held<24> finished{c};
    Held<25> column{c};
    Held<26> team{c};
    Held<28> side{c};
    Held<30> table{c};
    Held<31> pool{c};

    // --- Stack locals, fixture and statistics ---
    std::uint32_t lb(std::uint32_t at) const { return c.u8(frame.at(at)); }
    void sb(std::uint32_t at, std::uint32_t value) { c.put8(frame.at(at), value); }
    std::uint32_t fixture() const { return c.u32(match + fixture_at); }
    std::uint32_t fb(std::uint32_t at) const { return c.u8(fixture() + at); }
    std::int32_t fbs(std::uint32_t at) const { return s8(fb(at)); }
    void set_fb(std::uint32_t at, std::uint32_t value) { c.put8(fixture() + at, value); }
    std::int32_t fix_flags() const { return c.i16(fixture() + 0x38); }
    // The side's statistic bytes/halfwords at match + 0x10000 - low + index * 0x36.
    std::uint32_t stat(std::int32_t index, std::uint32_t low) const { return match + u(index * 0x36) + stat_base - low; }
    std::uint32_t player(std::int32_t index, std::int32_t slot) const {
        return match + u(index * std::int32_t(side_size) + slot * std::int32_t(player_size)) + players_at;
    }
    std::int32_t own() const { return s8(side); }
    std::int32_t other() const { return s8(side) == 0; }  // cntlzw >> 5: the opposing side
    void bump8(std::uint32_t at) { c.put8(at, c.u8(at) + 1u); }
    void bump16(std::uint32_t at) { c.put16(at, c.u16(at) + 1u); }
    void add16(std::uint32_t at, std::int32_t delta) { c.put16(at, u(c.i16(at) + delta)); }
    std::int32_t pair_sum(std::int32_t index) const {
        return std::int32_t(c.i16(match + 0x1f4 + u(index) * 2)) + c.i16(match + 0x1f8 + u(index) * 2);
    }

    // --- Calls ---
    std::int32_t draw(std::int32_t bound, std::uint32_t back) {
        return std::int32_t(match_eng::draw(c, u(bound), back));
    }
    std::uint32_t usable(std::uint32_t candidate, std::uint32_t back) {
        c.set(3, candidate);
        match_eng::call(c, usable_at, back);
        return c.r(3);
    }
    std::uint32_t pick(std::uint32_t entry, std::uint32_t chosen_side, std::uint32_t argument, std::uint32_t back) {
        c.set({{3, match}, {4, chosen_side}, {5, argument}});
        match_eng::call(c, entry, back);
        return c.r(3);
    }
    void with_side(std::uint32_t entry, std::uint32_t chosen_side, std::uint32_t amount, std::uint32_t back) {
        c.set({{3, match}, {4, chosen_side}, {5, amount}});
        match_eng::call(c, entry, back);
    }
    void count_event(std::uint32_t entry, std::uint32_t back) {
        c.set({{3, match}, {4, side}});
        match_eng::call(c, entry, back);
    }
    void notice(std::uint32_t kind, std::uint32_t first, std::uint32_t second, std::uint32_t back) {
        c.set({{3, c.u32(match + message_target)}, {4, kind}, {5, match}, {6, first}, {7, second}, {8, 0}});
        match_eng::call(c, notice_at, back);
    }
    // --- Floating point helpers (stack slot, converted integer) ---
    double magic() const { return c.f64(table + 0x60); }
    double converted(std::uint32_t slot, std::uint32_t value) {
        c.put32(frame.at(slot + 4), value ^ 0x80000000);
        c.put32(frame.at(slot), 0x43300000);
        return c.f64(frame.at(slot));
    }
    double to_double(std::uint32_t slot, std::uint32_t value) { return fp.fsub(converted(slot, value), magic()); }
    double to_single(std::uint32_t slot, std::uint32_t value) { return fp.fsubs(converted(slot, value), magic()); }
    std::uint32_t truncated(std::uint32_t slot, double value) {
        c.put64(frame.at(slot), fp.fctiwz(0, value));
        return c.u32(frame.at(slot + 4));
    }

    void minute();
    void clamp(std::uint32_t a, std::uint32_t b, std::uint32_t first_a, std::uint32_t first_b, int mode);
    bool take_turn();
    void attempts();
    bool first_contest();
    bool second_contest();
    bool chance_phase();
    void bookings(bool major);
    bool abandon();
    void rate_chances();
    bool scoring();
    bool scored();
    void missed();
    void advance_clock();
    void tail();
    void cup_upset();
    void player_events();
    void note_player(std::uint32_t member);
    void ratings();
    void final_counts();
};

void Simulation::run() {
    c.set(29, match);
    c.set(27, c.toc(-0x7b54));
    pool = c.toc(-0x52c8);
    table = c.toc(-0x52c4);
    c.set_f(31, c.f32(pool + 0xc));
    margin = 0;
    contribution = 0;
    finished = 0;
    for (const auto at : {l_49, l_48, l_47, l_46, l_45}) sb(at, 0);
    counter_chance = 0;
    overtime = 0;
    scorer = 0;
    sb(l_44, 0);
    defender = 0;
    for (const auto at : {l_shootout, l_ended, l_golden}) sb(at, 0);
    tries = 0;
    t14 = 0;
    c.put16(match + clock_at, 0);
    c.put16(match + period_end, 0x1ef);
    c.set({{3, 0}, {4, 0}, {5, 0x1ef}, {6, 0}});
    for (;;) {
        c.set(6, u(s8(finished)));
        if (compare(c, s8(finished)) != 0) break;
        minute();
    }
    tail();
}

// One halfword pair of the sides' tendencies: the first is kept between 0.? and ?.? times the second
// (constants at pool 0x90 and 0x98). `mode` says which scratch value the original leaves in r14.
void Simulation::clamp(std::uint32_t a, std::uint32_t b, std::uint32_t first_a, std::uint32_t first_b, int mode) {
    const auto word_a = [&] { return u(std::int32_t(c.i16(match + a))); };
    const auto word_b = [&] { return u(std::int32_t(c.i16(match + b))); };
    const double f3 = to_double(first_a, word_a());
    if (mode == 2) t14 = word_b() ^ 0x80000000;
    if (mode == 3) c.set(8, word_b());
    const double below = fp.fmul(c.f64(table + 0x90), to_double(first_b, word_b()));
    fp.fcmp(0, f3, below);
    if (f3 < below) {
        if (mode == 3) t14 = word_b();
        if (mode == 2) c.set(8, 0x43300000);
        const double limit = fp.fmul(c.f64(table + 0x90), to_double(0x70, word_b()));
        c.put16(match + a, truncated(0x78, limit));
        return;
    }
    if (mode == 2) c.set(9, word_a() ^ 0x80000000);
    const double now = to_double(0x70, word_a());
    const double above = fp.fmul(c.f64(table + 0x98), to_double(0x78, word_b()));
    fp.fcmp(0, now, above);
    if (now > above) {
        if (mode == 1) t14 = word_b() ^ 0x80000000;
        const double limit = fp.fmul(c.f64(table + 0x98), to_double(0x70, word_b()));
        const auto result = truncated(0x78, limit);
        if (mode == 2) t14 = result;
        c.put16(match + a, result);
    }
}

void Simulation::minute() {
    c.set({{3, match}, {4, 0}});
    match_eng::call(c, tendencies_at, 0x101db3f4);
    c.set({{3, match}, {4, 1}});
    match_eng::call(c, tendencies_at, 0x101db400);
    clamp(0x1f4, 0x1f6, 0x78, 0x70, 1);
    clamp(0x1f8, 0x1fa, 0x70, 0x78, 2);
    clamp(0x1fc, 0x1fe, 0x70, 0x78, 3);
    clamp(0x200, 0x202, 0x70, 0x78, 0);
    const auto starter = draw(2, 0x101db858);
    c.set(8, u(s8(u(starter))));
    sb(l_first, starter);
    c.set(7, 0);
    sb(l_round, 0);
    for (;;) {
        const auto round = s8(lb(l_round));
        c.set(4, lb(l_round));
        c.set(6, u(round));
        if (compare(c, round, 2) >= 0) break;
        if (compare(c, s8(c.u8(match + extra_time_flag))) != 0) break;
        if (take_turn()) break;
        t14 = lb(l_round);
        sb(l_round, lb(l_round) + 1u);
    }
    advance_clock();
}

// The side order of the minute, three attempts at the opposition, the contests that follow and the scoring.
// Returns true when the original jumps to the clock at once.
bool Simulation::take_turn() {
    if (compare(c, s8(lb(l_first))) != 0) side = lb(l_round);
    else side = u(s8(std::uint32_t(std::countl_zero(u(s8(lb(l_round))))) >> 5));
    sb(l_attempt, 0);
    attempts();
    if (first_contest()) return true;
    if (second_contest()) return true;
    if (chance_phase()) return true;
    rate_chances();
    return scoring();
}

void Simulation::attempts() {
    for (;;) {
        if (compare(c, s8(lb(l_attempt)), 3) >= 0) return;
        if (compare(c, s8(lb(l_attempt)), 2) >= 0) {
            t14 = u(draw(pair_sum(other()), 0x101db8ec));
            t15 = u(own());
            const auto mine = draw(pair_sum(own()), 0x101db918);
            if (compare(c, mine, std::int32_t(t14)) <= 0) {
                sb(l_attempt, lb(l_attempt) + 1u);
                continue;
            }
        }
        bump16(stat(own(), 0x1f9e));
        tries = 0;
        for (;;) {
            sb(l_48, u(s8(u(draw(0x14, 0x101db948)))));
            tries = tries + 1;
            t15 = usable(player(own(), s8(lb(l_48))), 0x101db978);
            if (compare(c, s8(t15)) >= 0) break;
            if (compare(c, s8(tries), 0x78) >= 0) break;
        }
        if (compare(c, s8(tries), 0x78) >= 0) {
            sb(l_ended, 1);
            return;
        }
        t14 = u(std::int32_t(c.i16(match + 0x1f4 + u(other()) * 2)) + 0xc8);
        const auto bound = magic_quotient(std::int32_t(t14), 0x6bca1af3, 3);
        const double f2 = to_single(0x70, u(draw(bound, 0x101db9e4)));
        const auto candidate = player(own(), s8(lb(l_48)));
        const double f0 = fp.fadds(c.f32(pool + 0x38), c.f32(candidate + 0xc8));
        fp.fcmp(0, f0, f2);
        if (f0 > f2) {
            bump16(stat(own(), 0x1f9c));
            add16(candidate + 0x36, 0x28);
        } else {
            add16(candidate + 0x36, -0x1e);
        }
        sb(l_attempt, lb(l_attempt) + 1u);
    }
}

bool Simulation::first_contest() {
    c.set(8, u(std::int32_t(c.i16(match + 0x1f4 + u(other()) * 2))));
    t14 = u(draw(pair_sum(other()), 0x101dbaf4));
    const auto mine = draw(pair_sum(own()), 0x101dbb20);
    if (compare(c, mine, std::int32_t(t14)) <= 0) return false;
    bump16(stat(own(), 0x1f9a));
    c.set({{3, match}, {4, side}});
    match_eng::call(c, traits_at, 0x101dbb54);
    sb(l_47, c.r(3));
    t14 = lb(l_47);
    if (compare(c, s8(t14), -1) == 0) {
        sb(l_ended, 1);
        return true;
    }
    const auto bound = magic_quotient(c.i16(match + 0x1f8 + u(other()) * 2), 0x66666667, 1);
    const double f2 = to_single(0x70, u(draw(bound, 0x101dbba8)));
    const auto candidate = player(own(), s8(lb(l_47)));
    const double f0 = fp.fadds(c.f32(pool + 0x3c), c.f32(candidate + 0xe0));
    fp.fcmp(0, f0, f2);
    if (f0 > f2) {
        add16(candidate + 0x36, 0x28);
        t14 = u(own());
        bump16(stat(own(), 0x1f98));
    } else {
        add16(candidate + 0x36, -10);
    }
    return false;
}

bool Simulation::second_contest() {
    t14 = u(c.i16(match + 0x1f4 + u(other()) * 2));
    t14 = u(draw(std::int32_t(t14) + c.i16(match + 0x1f8 + u(other()) * 2), 0x101dbc9c));
    c.set(8, u(std::int32_t(c.i16(match + 0x1f4 + u(own()) * 2))));
    c.set(9, 0x1f8 + u(own()) * 2);
    const auto mine = draw(pair_sum(own()), 0x101dbcc8);
    if (compare(c, mine, std::int32_t(t14)) > 0) {
        bump16(stat(own(), 0x1f96));
        tries = 0;
        for (;;) {
            const auto roll = draw(0x14, 0x101dbcfc);
            t14 = u(roll);
            sb(l_45, u(s8(u(roll))));
            tries = tries + 1;
            c.set(8, lb(l_45));
            t14 = u(own() * std::int32_t(side_size) + s8(lb(l_45)) * std::int32_t(player_size));
            const auto found = usable(match + t14 + players_at, 0x101dbd30);
            if (compare(c, s8(found)) >= 0) break;
            if (compare(c, s8(tries), 0x78) >= 0) break;
        }
        if (compare(c, s8(tries), 0x78) >= 0) {
            sb(l_ended, 1);
            return true;
        }
        const auto bound = magic_quotient(c.i16(match + 0x20c + u(other()) * 2) + 0x64, std::int32_t(0x92492493), 2);
        const double f2 = to_single(0x70, u(draw(bound, 0x101dbda0)));
        const auto candidate = player(own(), s8(lb(l_45)));
        const double f0 = fp.fadds(c.f32(pool + 0x3c), c.f32(candidate + 0xb0));
        fp.fcmp(0, f0, f2);
        if (f0 > f2) {
            c.set(8, lb(l_45));
            bump16(stat(own(), 0x1f94));
            add16(candidate + 0x36, 0x14);
        } else {
            add16(candidate + 0x36, -8);
        }
    }
    if (compare(c, draw(10, 0x101dbe64)) == 0) bump8(stat(own(), 0x1fb8));
    c.set({{3, match}, {4, side}});
    match_eng::call(c, simple_at, 0x101dbe94);
    sb(l_46, c.r(3));
    t14 = u(draw(2, 0x101dbea4));
    t15 = u(own());
    const auto candidate = player(own(), s8(lb(l_46)));
    const double f0 = fp.fadds(c.f32(pool + 0x38), c.f32(candidate + 0xbc));
    const auto bound = std::int32_t(truncated(0x70, f0));
    const auto roll = draw(bound, 0x101dbee0);
    if (compare(c, roll, std::int32_t(t14)) < 0) {
        add16(candidate + 0x36, -0xf);
        bump8(stat(own(), 0x1fb6));
    }
    return false;
}

// The opposition's booking (0x101dbfbc..0x101dc0c8): a straight red card (major) or a booking, which
// is a red card on the second one.
void Simulation::bookings(bool major) {
    const auto opposing = other();
    defender = pick(by_mode_at, u(opposing), major ? 4 : 2, major ? 0x101dbfd8 : 0x101dc118);
    const auto victim = player(opposing, s8(defender));
    if (major) {
        add16(victim + 0x36, -0xfa);
        bump8(victim + 0x23);
        bump8(stat(opposing, 0x1fb5));
    } else {
        add16(victim + 0x36, -0x32);
        t14 = u(s8(defender) * std::int32_t(player_size));
        bump8(victim + 0x24);
        bump8(stat(opposing, 0x1fb4));
        t14 = u(std::countl_zero(u(own())));
        // A second booking is a red card.
        if (compare(c, s8(c.u8(victim + 0x24)), 2) != 0) return;
        bump8(victim + 0x23);
        t14 = u(opposing);
        bump8(stat(opposing, 0x1fb5));
        t14 = u(opposing);
    }
    c.set({{3, match}, {4, victim}, {5, 1}});
    match_eng::call(c, caution_at, major ? 0x101dc088 : 0x101dc248);
    if (!major) c.set(9, u(opposing));  // rlwinm r9 at 0x101dc254, only on this path
    c.set({{3, match}, {4, u(opposing)}});
    match_eng::call(c, tendencies_at, major ? 0x101dc0a0 : 0x101dc260);
    notice(0x1ff8, defender, u(opposing), major ? 0x101dc0c8 : 0x101dc288);
}

// Returns true when the original goes to the clock at once.
bool Simulation::chance_phase() {
    overtime = 0;
    counter_chance = overtime;
    const auto opposing = other();
    t15 = u(draw(0x640, 0x101dbf40));
    if (compare(c, draw(c.i16(match + 0x204 + u(opposing) * 2), 0x101dbf60), std::int32_t(t15)) <= 0) return false;
    bump8(stat(opposing, 0x1fb7));
    t14 = u(draw(0x61a8, 0x101dbf94));
    if (compare(c, draw(c.i16(match + 0x204 + u(opposing) * 2), 0x101dbfb4), std::int32_t(t14)) > 0) {
        bookings(true);
    } else {
        t14 = u(draw(0x6a4, 0x101dc0d4));
        if (compare(c, draw(c.i16(match + 0x204 + u(opposing) * 2), 0x101dc0f4), std::int32_t(t14)) > 0) {
            bookings(false);
            t14 = u(opposing * 0x36);
            // More than four of the opposition's dismissals end the match.
            if (compare_logical(c, c.u8(stat(opposing, 0x1fb5)), 4) > 0) return abandon();
        }
    }
    c.set(5, u(draw(0x1e, 0x101dc3f0)));
    if (compare(c, std::int32_t(c.r(5))) == 0) {
        overtime = 1;
    } else {
        counter_chance = 1;
        bump8(stat(opposing, 0x1fba));
    }
    return false;
}

// 0x101dc2ac: the opposition has lost five players: the match is over, the bookings reset.
bool Simulation::abandon() {
    const auto opposing = other();
    sb(l_ended, 1);
    c.set(8, 0);
    t14 = u(std::countl_zero(u(own())));
    c.put8(stat(opposing, 0x1fc3), 0);
    const auto count = c.u8(stat(own(), 0x1fc3));
    c.put32(frame.at(0x5c), compare_logical(c, count, 3) > 0 ? c.u8(stat(own(), 0x1fc3)) : 3u);
    c.put8(stat(own(), 0x1fc3), c.u32(frame.at(0x5c)));
    if (compare(c, own()) != 0) {
        set_fb(0x43, 3);
        set_fb(0x44, 0);
    } else {
        set_fb(0x43, 0);
        set_fb(0x44, 3);
    }
    if (compare(c, fbs(0x4b), -1) == 0) return true;
    const auto left = fbs(0x4b) + fbs(0x43);
    const auto right = fbs(0x4c) + fbs(0x44);
    c.set(8, u(left));
    if (compare(c, left, right) != 0) return true;
    set_fb(0x43, fb(0x43) + 1u);
    c.put8(match + stat_base - 0x1fc3, c.u8(match + stat_base - 0x1fc3) + 1u);
    return true;
}

// 0x101dc42c: a last draw for a counter-chance, then the margin of the chances.
void Simulation::rate_chances() {
    const auto opposing = other();
    if (compare(c, s8(counter_chance)) == 0) {
        t14 = u(s8(overtime));
        if (compare(c, s8(t14)) == 0) {
            t15 = u(draw(0xbb8, 0x101dc44c));
            if (compare(c, draw(c.i16(match + 0x1f8 + u(own()) * 2), 0x101dc464), std::int32_t(t15)) > 0) {
                bump8(stat(own(), 0x1fb9));
                counter_chance = 1;
            }
        }
    }
    // f31: the scoring-chance factor of this side (the pool's 0.? ... depending on the clock).
    const auto level = [&](bool clock_based) {
        if (!clock_based) {
            c.set_f(31, c.f32(pool + 0x10));
            return;
        }
        c.set_f(31, c.f32(pool + (compare(c, c.i16(match + clock_at), 0x3c) > 0 ? 0x40u : 0x44u)));
    };
    const auto from_difference = [&](std::int32_t d) {
        c.set_f(31, fp.fsubs(converted(0x70, u(d)), magic()));
    };
    const auto played = [&](std::int32_t index) { return std::int32_t(c.u8(stat(index, 0x1fc3))); };
    if (compare(c, own()) == 0) {
        const auto against = played(opposing) + fbs(0x4c);
        t15 = u(own() * 0x36);
        const auto for_side = played(own()) + fbs(0x4b);
        if (compare(c, for_side - against, 3) > 0) {
            t14 = u(own());
            from_difference(played(own()) - played(opposing));
        } else if (compare(c, played(own()) + played(opposing)) != 0) {
            level(false);
        } else {
            level(true);
        }
    } else {
        const auto against = played(opposing) + fbs(0x4b);
        const auto for_side = played(own()) + fbs(0x4c);
        if (compare(c, for_side - against, 3) > 0) {
            c.set(8, u(own()));
            from_difference(played(own()) - played(opposing));
        } else {
            t14 = u(own());
            if (compare(c, played(own()) + played(opposing)) != 0) level(false);
            else level(true);
        }
    }
    // The margin: the side's draw against a draw bounded by the opposition's chances, scaled by f31.
    const bool counter = compare(c, s8(counter_chance)) != 0;
    t14 = u(draw(c.i16(match + 0x1f8 + u(own()) * 2), counter ? 0x101dc7ac : 0x101dc6d4));
    const double scale = c.f64(table + (counter ? 0x128u : 0x120u));
    const auto basis = counter ? c.i16(match + 0x20c + u(opposing) * 2) : c.i16(match + 0x1f4 + u(opposing) * 2);
    double product = fp.fmul(scale, to_double(0x70, u(basis)));
    product = fp.fmul(product, c.f(31));
    const auto bound = std::int32_t(truncated(0x78, product));
    const auto roll = draw(bound, counter ? 0x101dc800 : 0x101dc728);
    margin = u(std::int32_t(std::int16_t(std::int32_t(t14) - roll)));
    compare(c, std::int32_t(margin));
    const bool positive = std::int32_t(margin) > 0;
    const std::uint32_t first_back = counter ? (positive ? 0x101dc820 : 0x101dc850) : (positive ? 0x101dc748 : 0x101dc778);
    c.set({{3, match}, {4, side}, {5, positive ? 0x1cu : u(-6)}});
    match_eng::call(c, adjust_a_at, first_back);
    const auto opposing_register = u(s8(u(opposing)));
    if (counter) {
        c.set({{3, match}, {4, opposing_register}, {5, positive ? u(-0x14) : 0x28u}});
        match_eng::call(c, adjust_e_at, positive ? 0x101dc83c : 0x101dc86c);
    } else {
        c.set({{3, match}, {4, opposing_register}, {5, positive ? u(-0x14) : 0x28u}});
        match_eng::call(c, adjust_b_at, positive ? 0x101dc764 : 0x101dc794);
    }
    if (compare(c, std::int32_t(std::int16_t(margin))) > 0) {
        t14 = c.u8(match + stat_base - 0x1fc3);
        const auto total = std::int32_t(t14) + c.u8(match + stat_base - 0x1f8d);
        if (compare(c, draw(total, 0x101dc890), 3) > 0) margin = 0;
    }
}

// 0x101dc89c: a margin in the side's favour may become a chance, then a goal.
bool Simulation::scoring() {
    const auto opposing = other();
    if (compare(c, std::int32_t(std::int16_t(margin))) <= 0) return false;
    if (compare(c, s8(overtime)) != 0) {
        t14 = u(own() * 0x36);
        bump8(stat(own(), 0x1fc0));
    }
    c.set(8, u(own()));
    c.set(9, c.u8(stat(own(), 0x1fc2)));
    bump8(stat(own(), 0x1fc2));
    sb(l_49, pick(ranked_at, side, counter_chance, 0x101dc8fc));
    if (compare(c, s8(counter_chance)) != 0) {
        contribution = u(std::int32_t(std::int16_t(draw(c.i16(match + 0x208 + u(own()) * 2), 0x101dc924))));
    } else {
        t14 = u(own());
        c.set(8, u(s8(overtime)));
        const auto bound = std::int32_t(u(c.i16(match + 0x1fc + u(own()) * 2)) * u(s8(overtime) + 1));
        contribution = u(std::int32_t(std::int16_t(draw(bound, 0x101dc950))));
    }
    t14 = u(draw(0x172, 0x101dc960));
    const auto limit = 0x168 - std::int32_t(std::int16_t(margin)) + std::int32_t(t14);
    const auto contribution_value = std::int32_t(std::int16_t(contribution));
    if (compare(c, contribution_value + 0xa5, limit) <= 0) return false;
    bump8(stat(own(), 0x1fc1));
    if (compare(c, s8(counter_chance)) != 0) with_side(adjust_c_at, side, u(contribution_value), 0x101dc9b8);
    else with_side(adjust_d_at, side, u(contribution_value), 0x101dc9cc);
    const auto weight = c.i16(match + 0x200 + u(opposing) * 2) * (7 - 3 * s8(overtime));
    const auto roll = draw(weight, 0x101dc9f8);
    if (compare(c, contribution_value + 0x2d, roll) > 0) return scored();
    missed();
    return false;
}

// The chance is a goal (0x101dca08).
bool Simulation::scored() {
    const auto opposing = other();
    scorer = pick(weighted_at, side, overtime, 0x101dca18);
    const auto taker = player(own(), s8(scorer));
    bump8(taker + 0xc);
    add16(taker + 0x36, 0x1db);
    c.set(8, u(s8(scorer)));
    sb(l_sweep, 0);
    // Opposing players on the pitch who are goalkeepers (role flags == 1) concede one.
    while (compare(c, s8(lb(l_sweep)), 0x14) < 0) {
        const auto keeper = player(opposing, s8(lb(l_sweep)));
        const auto found = usable(keeper, 0x101dca9c);
        if (compare(c, s8(found)) >= 0) {
            t14 = u(s8(lb(l_sweep)) * std::int32_t(player_size));
            t15 = u(opposing * std::int32_t(side_size)) + t14;
            c.set(3, match + t15 + players_at);
            match_eng::call(c, role_flags_at, 0x101dcad8);
            c.set(4, c.r(3));
            if (compare(c, std::int32_t(std::int16_t(c.r(3))), 1) == 0) bump8(keeper + 0xf);
        }
        sb(l_sweep, lb(l_sweep) + 1u);
    }
    if (compare(c, s8(overtime)) != 0) {
        notice(0x1fcf, scorer, side, 0x101dcb5c);
        t15 = c.u8(stat(own(), 0x1fbf));
        c.put8(stat(own(), 0x1fbf), t15 + 1u);
        bump8(taker + 0xd);
        t14 = u(own() * std::int32_t(side_size) + s8(scorer) * std::int32_t(player_size));
        bump8(match + t14 + 0x312a);
    } else {
        notice(0x1fcc, scorer, side, 0x101dcbe0);
        if (compare(c, s8(counter_chance)) != 0) {
            const auto kind = draw(4, 0x101dcbf4);
            c.set(4, u(kind));
            if (compare(c, kind, 2) == 0) count_event(count_b_at, 0x101dcc2c);
            else if (kind > 2) count_event(count_a_at, 0x101dcc3c);
            else if (compare(c, kind) >= 0) count_event(count_c_at, 0x101dcc1c);
            else count_event(count_a_at, 0x101dcc3c);
        }
    }
    if (compare(c, s8(lb(l_49)), -1) > 0) {
        const auto assist = player(own(), s8(lb(l_49)));
        bump8(assist + 0x10);
        add16(assist + 0x36, 0x15e);
    }
    bump8(stat(opposing, 0x1fbe));
    bump8(stat(own(), 0x1fc3));
    c.set({{3, match}, {4, side}, {5, 0x4b}});
    match_eng::call(c, award_at, 0x101dccec);
    c.set({{3, match}, {4, u(s8(u(opposing)))}, {5, u(-0x43)}});
    match_eng::call(c, award_at, 0x101dcd08);
    if (compare(c, s8(lb(l_golden))) != 0) {
        c.put8(match + extra_time_flag, 1);
        return true;
    }
    return false;
}

// The chance is missed (0x101dcd24).
void Simulation::missed() {
    const auto opposing = other();
    c.set(8, u(own()));
    with_side(adjust_f_at, u(s8(u(opposing))), 0xe1, 0x101dcd40);
    if (compare(c, s8(overtime)) == 0) return;
    sb(l_44, pick(weighted_at, side, overtime, 0x101dcd5c));
    c.set({{3, c.u32(match + message_target)}, {4, 0x1fce}, {5, match}, {6, lb(l_44)}, {7, side}, {8, 0}});
    match_eng::call(c, notice_at, 0x101dcd80);
    t14 = lb(l_44);
    const auto shooter = player(own(), s8(lb(l_44)));
    bump8(shooter + 0xd);
    add16(shooter + 0x36, -0x1f4);
}

// 0x101dcdfc: the clock moves a minute; at the whiles of the periods the half-time, full-time and
// extra-time bookkeeping stores the score into the fixture and decides whether the match goes on.
void Simulation::advance_clock() {
    c.put16(match + clock_at, c.u16(match + clock_at) + 1u);
    const auto home = [&] { return c.u8(match + stat_base - 0x1fc3); };
    const auto away = [&] { return c.u8(match + stat_base - 0x1f8d); };
    const auto bit = [&](std::int32_t mask) { return compare(c, fix_flags() & mask) != 0; };
    if (compare(c, s8(lb(l_ended))) != 0) {
        if (compare(c, c.i16(match + clock_at), 0x5a) <= 0) {
            set_fb(0x43, home());
            t14 = match + stat_base;
            set_fb(0x44, away());
            if (compare(c, fbs(0x4b), -1) != 0) {
                const auto left = fbs(0x4b) + fbs(0x43);
                t14 = u(fbs(0x44));
                const auto right = fbs(0x4c) + fbs(0x44);
                if (compare(c, left, right) == 0) set_fb(0x43, fb(0x43) + 1u);
            } else {
                t14 = u(fbs(0x43));
                if (compare(c, std::int32_t(t14), fbs(0x44)) == 0) {
                    c.set(8, fixture());
                    bool extra = bit(0x80);
                    if (!extra) {
                        c.set(9, fixture());
                        extra = bit(2) || bit(1);
                    }
                    if (!extra) {
                        t14 = u(fix_flags() & 0x20);
                        extra = compare(c, std::int32_t(t14)) != 0;
                    }
                    if (extra) set_fb(0x43, fb(0x43) + 1u);
                }
            }
        } else {
            c.set(8, match + stat_base);
            set_fb(0x47, home());
            t14 = fixture();
            set_fb(0x48, away());
            if (compare(c, fbs(0x4b), -1) != 0) {
                const auto left = fbs(0x4b) + fbs(0x43);
                const auto right = fbs(0x4c) + fbs(0x44);
                if (compare(c, left, right) == 0) set_fb(0x47, fb(0x47) + 1u);
            } else if (compare(c, fbs(0x43), fbs(0x44)) == 0) {
                bool extra = bit(0x80);
                if (!extra) {
                    c.set(8, u(fix_flags()));
                    extra = bit(2);
                }
                if (!extra) extra = bit(1) || bit(0x20);
                if (extra) set_fb(0x47, fb(0x47) + 1u);
            }
        }
        finished = 1;
        return;
    }
    if (compare(c, s8(c.u8(match + extra_time_flag))) != 0) {
        set_fb(0x47, home());
        set_fb(0x48, away());
        finished = 1;
        return;
    }
    if (compare(c, c.i16(match + clock_at), 0x2d) == 0) {
        c.put16(match + period_end, 0x3de);
        t14 = match + stat_base;
        t15 = home();
        set_fb(0x45, home());
        set_fb(0x46, away());
        return;
    }
    if (compare(c, c.i16(match + clock_at), 0x5a) == 0) {
        set_fb(0x43, home());
        set_fb(0x44, away());
        c.put16(match + period_end, 0x483);
        if (compare(c, fbs(0x4b), -1) != 0) {
            t15 = u(fbs(0x4b));
            const auto left = std::int32_t(t15) + fbs(0x43);
            const auto right = fbs(0x4c) + fbs(0x44);
            if (compare(c, left, right) != 0) {
                finished = 1;
                return;
            }
            t14 = u(fix_flags() & 0x80);
            if (compare(c, std::int32_t(t14)) != 0) {
                finished = 0;
                sb(l_golden, 1);
                return;
            }
            if (bit(2)) {
                finished = 0;
                return;
            }
            if (!bit(4)) {
                if (compare(c, fbs(0x44), fbs(0x4b)) != 0 && !bit(4) && !bit(0x4000)) {
                    finished = 1;
                    return;
                }
            }
        } else {
            if (compare(c, fbs(0x43), fbs(0x44)) != 0) {
                finished = 1;
                return;
            }
            if (bit(0x80)) {
                finished = 0;
                sb(l_golden, 1);
                return;
            }
            if (bit(2)) {
                finished = 0;
                return;
            }
            // The last test keeps the flags in r8.
            if (!bit(1)) {
                c.set(8, u(fix_flags()));
                if (bit(0x20)) sb(l_shootout, 1);
            } else {
                sb(l_shootout, 1);
            }
            finished = 1;
            return;
        }
        if (bit(1) || bit(0x20)) sb(l_shootout, 1);
        finished = 1;
        return;
    }
    if (compare(c, c.i16(match + clock_at), 0x69) == 0) {
        c.put16(match + period_end, 0x528);
        return;
    }
    if (compare(c, c.i16(match + clock_at), 0x78) != 0) return;
    t14 = home();
    set_fb(0x47, home());
    set_fb(0x48, away());
    finished = 1;
    if (compare(c, fbs(0x4b), -1) != 0) {
        const auto left = fbs(0x47) + fbs(0x4b);
        const auto right = fbs(0x48) + fbs(0x4c);
        if (bit(0x4000)) {
            t14 = fixture();
            if (compare(c, left, right) != 0) return;
            if (compare(c, fbs(0x48), fbs(0x4b)) != 0) return;
            if (bit(1)) {
                sb(l_shootout, 1);
                return;
            }
            t14 = fixture();
            if (bit(0x20)) sb(l_shootout, 1);
        } else {
            if (compare(c, left, right) != 0) return;
            t14 = u(fix_flags());
            if (compare(c, std::int32_t(t14) & 1) != 0 || bit(0x20)) sb(l_shootout, 1);
        }
    } else {
        t14 = fb(0x47);
        c.set(8, u(fbs(0x48)));
        if (compare(c, s8(t14), fbs(0x48)) != 0) return;
        c.set(9, u(fix_flags() & 1));
        if (bit(1) || bit(0x20)) sb(l_shootout, 1);
    }
}

// 0x101dd4d0: the match is over: the shoot-out, the final score, the penalty-box tidy-up of the players'
// ratings and condition.
void Simulation::tail() {
    if (compare(c, s8(lb(l_shootout))) != 0) {
        // Each side's shoot-out score is the better of two draws below 5.
        const auto better_of_two = [&](std::uint32_t slot, std::uint32_t first_back, std::uint32_t second_back, std::uint32_t high_back, std::uint32_t low_back) {
            t14 = u(draw(5, first_back));
            const auto second = draw(5, second_back);
            if (compare(c, second, std::int32_t(t14)) > 0) c.put32(frame.at(slot), u(draw(5, high_back)));
            else c.put32(frame.at(slot), u(draw(5, low_back)));
        };
        better_of_two(0x58, 0x101dd4e8, 0x101dd4f4, 0x101dd508, 0x101dd51c);
        c.set(8, c.u32(frame.at(0x58)));
        set_fb(0x49, c.u32(frame.at(0x58)));
        better_of_two(0x54, 0x101dd53c, 0x101dd548, 0x101dd55c, 0x101dd570);
        set_fb(0x4a, c.u32(frame.at(0x54)));
        if (compare(c, fbs(0x49), fbs(0x4a)) == 0) {
            if (compare(c, draw(2, 0x101dd5b0)) != 0) set_fb(0x49, fb(0x49) + 1u);
            else set_fb(0x4a, fb(0x4a) + 1u);
        }
    }
    if (compare(c, fbs(0x4b), -1) != 0) {
        if (compare(c, fbs(0x47), -1) != 0) {
            set_fb(0x4b, fb(0x4b) + fb(0x47));
            c.set(8, fb(0x4c) + fb(0x48));
            set_fb(0x4c, fb(0x4c) + fb(0x48));
        } else {
            set_fb(0x4b, fb(0x4b) + fb(0x43));
            set_fb(0x4c, fb(0x4c) + fb(0x44));
        }
    }
    cup_upset();
    player_events();
    ratings();
    final_counts();
}

// 0x101dd668: in a cup tie between sides of different standing, a one-in-four draw lets the weaker
// side's best player score an extra goal when the match ends level and low-scoring.
void Simulation::cup_upset() {
    if (compare_logical(c, c.u32(fixture() + 0x1c)) == 0) return;
    if (compare_logical(c, c.u32(fixture() + 0x14), c.u32(c.u32(fixture() + 0x1c) + 0x30)) != 0) return;
    const auto left = fbs(0x43);
    t14 = u(fbs(0x44));
    t15 = u(left + std::int32_t(t14));
    if (compare(c, std::int32_t(t15), 5) >= 0) return;
    if (compare(c, fbs(0x43), fbs(0x44)) != 0) return;
    if (compare(c, draw(4, 0x101dd6e0)) != 0) return;
    const auto rating = [&](std::uint32_t at) { return std::int32_t(c.i16(c.u32(fixture() + at) + 0x1ec)); };
    const auto level = [&](std::int32_t index, std::uint32_t stat_at, std::uint32_t scorer_back, std::uint32_t base) {
        // A goal for the stronger side's index: counts it, picks the scorer, and awards the opposing goalkeepers.
        (void)index; (void)stat_at; (void)scorer_back; (void)base;
    };
    (void)level;
    if (compare(c, rating(0x1c), rating(0x20)) > 0) {
        bump8(match + stat_base - 0x1fc2);
        scorer = pick(weighted_at, 0, overtime, 0x101dd730);
        const auto taker = match + u(s8(scorer) * std::int32_t(player_size)) + players_at;
        t15 = c.u8(taker + 0xc);
        c.put8(taker + 0xc, t15 + 1u);
        add16(taker + 0x36, 0x1db);
        sb(l_sweep_a, 0);
        while (compare(c, s8(lb(l_sweep_a)), 0x14) < 0) {
            const auto keeper = match + u(s8(lb(l_sweep_a)) * std::int32_t(player_size)) + 0x546c;
            const auto found = usable(keeper, 0x101dd788);
            if (compare(c, s8(found)) >= 0) {
                t14 = u(s8(lb(l_sweep_a)));
                c.set(3, match + u(s8(lb(l_sweep_a)) * std::int32_t(player_size)) + 0x546c);
                match_eng::call(c, role_flags_at, 0x101dd7ac);
                if (compare(c, std::int32_t(std::int16_t(c.r(3))), 1) == 0) bump8(keeper + 0xf);
            }
            sb(l_sweep_a, lb(l_sweep_a) + 1u);
        }
        notice(0x1fcc, scorer, 0, 0x101dd80c);
        return;
    }
    if (compare(c, rating(0x1c) + 0xfa, rating(0x20)) >= 0) return;
    bump8(match + stat_base - 0x1f8c);
    scorer = pick(weighted_at, 1, overtime, 0x101dd854);
    const auto taker = match + u(s8(scorer) * std::int32_t(player_size)) + 0x5478;
    c.put8(taker, c.u8(taker) + 1u);
    add16(match + u(s8(scorer) * std::int32_t(player_size)) + 0x54a2, 0x1db);
    sb(l_sweep_b, 0);
    while (compare(c, s8(lb(l_sweep_b)), 0x14) < 0) {
        const auto keeper = match + u(s8(lb(l_sweep_b)) * std::int32_t(player_size)) + players_at;
        const auto found = usable(keeper, 0x101dd8ac);
        if (compare(c, s8(found)) >= 0) {
            c.set(3, keeper);
            match_eng::call(c, role_flags_at, 0x101dd8d4);
            c.set(8, c.r(3));
            if (compare(c, std::int32_t(std::int16_t(c.r(3))), 1) == 0) {
                const auto at = keeper + 0xf;
                t14 = c.u8(at) + 1u;
                c.put8(at, t14);
            }
        }
        sb(l_sweep_b, lb(l_sweep_b) + 1u);
    }
    notice(0x1fcc, scorer, 1, 0x101dd938);
}

// 0x101dd938: after the match each player of a side (unless the fixture's first word is -2) may get
// an injury, a suspension or another note, by a draw weighted by his record's byte and his fitness.
// Both passes use the side in r28 (the pass counter is not a side): quirk of the original.
void Simulation::player_events() {
    if (compare(c, std::int32_t(c.u32(fixture())), -2) == 0) return;
    sb(l_pass, 0);
    while (compare(c, s8(lb(l_pass)), 2) < 0) {
        pass_slot = 0;
        while (compare(c, s8(pass_slot), 0x14) < 0) {
            const auto member = player(own(), s8(pass_slot));
            const auto found = usable(member, 0x101dd97c);
            if (compare(c, s8(found)) >= 0) {
                t14 = u(own() * std::int32_t(side_size));
                const auto record = c.u32(member + 0x70);
                if (compare_logical(c, record) != 0) {
                    t14 = u(s8((c.u8(record + 0x2d) << 1) & 0xfe));
                    sb(l_weight, u(s8(t14 + 5)));
                    const double fitness = c.f32(member + 0x50);
                    fp.fcmp(0, fitness, c.f32(pool + 0x48));
                    if (fitness < c.f32(pool + 0x48)) {
                        sb(l_weight, lb(l_weight) + 0xfu);
                    } else {
                        const double again = c.f32(member + 0x50);
                        fp.fcmp(0, again, c.f32(pool + 0x4c));
                        if (again < c.f32(pool + 0x4c)) sb(l_weight, lb(l_weight) + 5u);
                    }
                    t14 = u(draw(0x190, 0x101dda58));
                    if (compare(c, draw(s8(lb(l_weight)), 0x101dda68), std::int32_t(t14)) >= 0) note_player(member);
                }
            }
            pass_slot = pass_slot + 1;
        }
        sb(l_pass, lb(l_pass) + 1u);
    }
}

void Simulation::note_player(std::uint32_t member) {
    bool narrow = false;  // 0x101ddac4 (draw below 0x66) rather than 0x101ddaac (below 0x5b)
    if (compare_logical(c, c.u32(fixture() + 0x14)) != 0) {
        if (compare_logical(c, c.u32(c.r(27))) == 0) {
            narrow = true;
        } else {
            c.set({{3, c.u32(c.r(27))}, {4, c.u32(fixture() + 0x14)}});
            match_eng::call(c, shoot_out_check_at, 0x101dda9c);
            c.set(4, c.r(3));
            t14 = u(s8(c.r(3)));
            narrow = compare(c, s8(t14)) == 0;
        }
    }
    const auto drawn = draw(narrow ? 0x66 : 0x5b, narrow ? 0x101ddacc : 0x101ddab4);
    if (narrow) c.set(8, u(s8(u(drawn))));
    sb(l_role, u(s8(u(drawn))));
    const auto kind = draw(10, 0x101ddae0);
    const auto act = [&](std::uint32_t code, std::uint32_t back) {
        c.set({{3, match}, {4, code}, {5, lb(l_role)}, {6, member}, {7, 0}});
        match_eng::call(c, action_at, back);
    };
    if (compare(c, kind, 4) < 0) {
        if (compare(c, kind) == 0) {
            c.set(9, u(own() * std::int32_t(side_size)));
            act(1, 0x101ddb40);
        } else if (kind > 0) {
            t14 = u(own() * std::int32_t(side_size) + s8(pass_slot) * std::int32_t(player_size));
            act(3, 0x101ddb74);
        }
        return;
    }
    if (compare(c, kind, 0xa) >= 0) return;
    if (compare(c, kind, 7) >= 0) {
        t14 = u(own() * std::int32_t(side_size));
        act(5, 0x101ddbdc);
    } else {
        c.set(8, u(s8(pass_slot) * std::int32_t(player_size)));
        act(4, 0x101ddba8);
    }
}

// 0x101ddc08: the sides' ratings are rescaled by the match's result, kept in their bounds, and
// the players' condition byte is rebuilt.
void Simulation::ratings() {
    team = 0;
    while (compare(c, s8(team), 2) < 0) {
        column = 0;
        while (compare(c, s8(column), 0x14) < 0) {
            const auto member = player(s8(team), s8(column));
            if (compare(c, std::int32_t(c.u32(fixture())), -2) == 0 && compare(c, s8(column), 0xb) < 0) {
                c.set({{3, match}, {4, team}});
                match_eng::call(c, team_at, 0x101ddc40);
                c.set(8, u(s8(team)));
                const double f2 = to_double(0x70, u(std::int32_t(c.i16(c.r(3) + 0x1ec))));
                const double f3 = fp.multiply_add(0x101ddc68, c.f64(table + 0x140), f2, c.f64(table + 0x148));
                const double scaled = to_double(0x78, u(std::int32_t(c.i16(member + 0x3c))));
                const double offset = fp.fsub(scaled, c.f64(table + 0x138));
                const double blended = fp.multiply_add(0x101ddcb0, c.f64(table + 0x130), offset, f3);
                const double factor = fp.fadd(c.f64(table + 0x58), fp.fdiv(blended, c.f64(table + 0x150)));
                c.set_f(30, factor);
                const double rating = fp.fmul(to_double(0x68, u(std::int32_t(c.i16(member + 0x36)))), factor);
                c.put16(member + 0x36, truncated(0x60, rating));
                // Kept within 0xdac..0x2ee0, through the stack words at sp+0x4c and sp+0x50.
                if (compare(c, c.i16(member + 0x36), 0xdac) < 0) {
                    c.put32(frame.at(0x50), 0xdac);
                } else {
                    if (compare(c, c.i16(member + 0x36), 0x2ee0) > 0) {
                        c.put32(frame.at(0x4c), 0x2ee0);
                    } else {
                        t14 = u(s8(team) * std::int32_t(side_size));
                        t15 = u(s8(column));
                        c.put32(frame.at(0x4c), u(std::int32_t(c.i16(member + 0x36))));
                    }
                    c.put32(frame.at(0x50), c.u32(frame.at(0x4c)));
                }
                c.put16(member + 0x36, c.u32(frame.at(0x50)));
            }
            // The condition byte: (rating + 0x1770) / pool, kept within 1..10.
            const double base = c.f64(table + 0x158);
            const auto shifted = u(std::int32_t(c.i16(member + 0x36)) + 0x1770);
            t15 = shifted ^ 0x80000000;
            const double raw = fp.fsubs(converted(0x60, shifted), magic());
            c.set_f(1, raw);
            const double quotient = fp.frsp(fp.fdiv(raw, c.f64(table + 0x160)));
            const auto level = truncated(0x68, fp.fadd(base, quotient));
            c.put8(member + 0x1b, level);
            t14 = u(s8(team) * std::int32_t(side_size) + s8(column) * std::int32_t(player_size) + 0x3137);
            if (compare(c, s8(c.u8(member + 0x1b)), 10) > 0) c.put8(member + 0x1b, 10);
            if (compare(c, s8(c.u8(member + 0x1b)), 1) < 0) c.put8(member + 0x1b, 1);
            c.put32(member + 0x54, c.u32(member + 0x50));
            column = column + 1;
        }
        team = team + 1;
    }
}

// 0x101ddf34: each side's pair of counters. Below 10 the first becomes 20 and the second 12 plus a draw
// below 6; equal counters lower the second by a draw below 3 plus 1. The scratch registers are those of
// the original's last instructions (a caller's next varargs call spills r4-r10).
void Simulation::final_counts() {
    const auto high = match + stat_base;
    // Left by the last iteration of the ratings loop.
    c.set({{3, u(1 * std::int32_t(side_size) + 19 * std::int32_t(player_size))}, {4, 2}, {5, u(1 * std::int32_t(side_size) + 19 * std::int32_t(player_size))},
           {6, 19}, {7, u(19 * std::int32_t(player_size))}});
    if (compare(c, std::int32_t(c.i16(high - 0x1f9e)), 0xa) < 0) {
        c.set(4, c.u32(high - 0x1f9e));
        c.put16(high - 0x1f9e, 0x14);
        c.set({{5, 0x14}, {4, high}});
        const auto roll = draw(6, 0x101ddf58);
        c.set({{6, u(roll)}, {8, u(roll + 0xc)}, {5, u(std::int32_t(std::int16_t(roll + 0xc)))}, {4, high}});
        c.put16(high - 0x1f9c, u(roll + 0xc));
    } else {
        const auto first = std::int32_t(c.i16(high - 0x1f9e));
        c.set({{4, high}, {7, u(first)}});
        t14 = u(std::int32_t(c.i16(high - 0x1f9c)));
        c.set(7, high);
        if (compare(c, first, std::int32_t(t14)) == 0) {
            const auto roll = draw(3, 0x101ddf94);
            const auto second = std::int32_t(c.i16(high - 0x1f9c));
            const auto lowered = second - (roll + 1);
            c.set({{4, u(lowered)}, {6, high}, {5, u(second)}, {3, u(std::int32_t(std::int16_t(lowered)))}});
            c.put16(high - 0x1f9c, u(lowered));
        }
    }
    c.set(4, u(std::int32_t(c.i16(high - 0x1f68))));
    if (compare(c, std::int32_t(c.i16(high - 0x1f68)), 0xa) < 0) {
        c.put16(high - 0x1f68, 0x14);
        c.set(3, high);
        const auto roll = draw(6, 0x101ddfd0);
        c.set({{3, u(roll + 0xc)}, {4, u(std::int32_t(std::int16_t(roll + 0xc)))}, {5, high}});
        c.put16(high - 0x1f66, u(roll + 0xc));
    } else {
        c.set(3, high);
        t14 = u(std::int32_t(c.i16(high - 0x1f68)));
        c.set({{5, u(std::int32_t(std::int16_t(t14)))}, {4, high}, {6, u(std::int32_t(c.i16(high - 0x1f66)))}});
        if (compare(c, std::int32_t(std::int16_t(t14)), std::int32_t(c.i16(high - 0x1f66))) == 0) {
            const auto roll = draw(3, 0x101de008);
            const auto second = std::int32_t(c.i16(high - 0x1f66));
            const auto lowered = second - (roll + 1);
            c.set({{4, high}, {3, u(std::int32_t(std::int16_t(lowered)))}});
            c.put16(high - 0x1f66, u(lowered));
        }
    }
}
}

void simulate_match_events(Cpu& c, std::uint32_t match) {
    Simulation simulation(c, match);
    simulation.run();
}
}
