// MATCH_ENG.CPP, match setup: preparing a fixture's match object (0x101d1760).
#include "match_eng_setup.hpp"

namespace cm::aot::match_eng {
using namespace guest;

namespace {
// Fixture fields (+0x3118 of the match).
constexpr std::uint32_t fx_competition = 0x0, fx_home = 0x1c, fx_away = 0x20, fx_options = 0x4e, fx_mode = 0x4f;

// Stack slots of the routine (from its new stack pointer): the caller's argument slots.
constexpr std::int32_t st_fixture = 0x36c, st_inactive = 0x373, st_target = 0x374, st_silent = 0x37b;

std::int32_t s32(std::uint32_t value) { return std::int32_t(value); }

class SetUp {
public:
    SetUp(Cpu& cpu, std::uint32_t match, std::uint32_t fixture)
        : c(cpu), frame(cpu, 0x101d1760, 0x350, {.first_gpr = 15, .first_fpr = 18}), fp(cpu), match(match),
          fixture_arg(fixture), hi(match + 0x10000) {}

    std::uint32_t run(std::uint32_t inactive, std::uint32_t target, std::uint32_t silent);

private:
    Cpu& c;
    Frame frame;
    Fpu fp;
    const std::uint32_t match, fixture_arg, hi;

    std::uint32_t fixture() const { return c.u32(match + fixture_at); }
    std::uint32_t toc_at(std::int32_t offset) const { return c.r(2) + u(offset); }
    double pool(std::uint32_t offset) const { return c.f64(c.r(31) + offset); }
    float single(std::uint32_t offset) const { return c.f32(c.r(28) + offset); }
    // An integer through the stack as the original converts it: high word 0x43300000, low word biased.
    double widen(std::int32_t at, std::uint32_t value) {
        c.put32(frame.at(at + 4), value ^ 0x80000000);
        c.put32(frame.at(at), 0x43300000);
        return c.f64(frame.at(at));
    }
    std::int32_t cmp(std::int32_t a, std::int32_t b = 0) { return compare(c, a, b); }
    std::int32_t cmpu(std::uint32_t a, std::uint32_t b = 0) { return compare_logical(c, a, b); }
    std::int32_t sb(std::uint32_t at) const { return s8(c.u8(at)); }
    // Loads that also leave the value in the register the original used (it is the next call's input).
    std::int32_t sbyte(unsigned to, std::uint32_t at, unsigned raw = 0) {
        const auto value = c.u8(at);
        c.set(raw, value);
        c.set(to, u(s8(value)));
        return s8(value);
    }
    std::uint32_t ubyte(unsigned to, std::uint32_t at) { return c.set(to, c.u8(at)), c.u8(at); }
    std::int32_t shalf(unsigned to, std::uint32_t at) { return c.set(to, u(std::int32_t(c.i16(at)))), c.i16(at); }
    std::uint32_t word(unsigned to, std::uint32_t at) { return c.set(to, c.u32(at)), c.u32(at); }
    std::int32_t half(std::int32_t at) const { return c.i16(frame.at(at)); }
    void memory_set(std::uint32_t at, std::uint32_t count, std::uint32_t back) {
        c.set({{3, at}, {4, 0}, {5, count}});
        c.call(0x10778300, back);
    }
    std::uint32_t team_of(std::uint32_t side) {
        c.set({{3, match}, {4, side}});
        return fixture_team(c, match, side);
    }

    void begin();
    void fixture_options();
    void angle_table();
    void clear_counters();
    void clear_sides();
    void attach_world();
    void rating_limits();
    void count_lineups();
    void report_shortage(bool away, std::uint32_t count);

    bool message_object();
    bool prepare_players();
    void run_players();
    bool assess_player();
    void assess_transfer(std::uint32_t record, std::uint32_t team, std::uint32_t object);
    void assess_form(std::uint32_t record, std::uint32_t team, std::uint32_t object);
    void assess_career(std::uint32_t record, std::uint32_t team, std::uint32_t object);
    void kickoff();
    void post_event(std::uint32_t player, std::uint32_t team, std::uint32_t argument, std::uint32_t flag,
                    std::uint32_t slot, std::uint32_t kind, std::uint32_t back);
};

// --- The fixture's own fields and the match's first values ---
void SetUp::begin() {
    c.set(30, match);
    keep(c, {{24, c.toc(-0x7b80)}, {29, c.toc(-0x7b3c)}, {25, c.toc(-0x7b7c)}, {16, c.toc(-0x52cc)},
             {28, c.toc(-0x52c8)}, {26, c.toc(-0x641c)}, {31, c.toc(-0x52c4)}});
    // The bearing table is built once per program run (flags in the program's globals).
    if (cmp(sb(toc_at(-0x3912))) == 0) {
        c.put8(toc_at(-0x3911), 1);
        c.put8(toc_at(-0x3912), 1);
    }
    c.put32(match + fixture_at, c.u32(frame.at(st_fixture)));
    c.set(15, 0);
    c.put8(match + shootout_stage, 0);
    c.put8(match + 0x792a, 0);
    memory_set(match + 0x238, 0x7d0, 0x101d17f0);
    c.put8(match + 0x791b, 0);
    c.set(15, 0);
    c.put8(match + 0x1bc, 0);
    c.put8(match + 0x1e5, 0);
    c.put8(match + 0x1e4, 0);
    // The two teams' ratings.
    c.put16(match + 0x1f0, c.u16(c.u32(fixture() + fx_home) + 0x1ec));
    c.put16(match + 0x1f2, c.u16(c.u32(fixture() + fx_away) + 0x1ec));
    c.put8(match + 0xaf, 0);
    c.put8(match + 0xb0, 0);
    c.put32(hi - 0x1fec, 0);
    c.put32(hi - 0x1fe8, 0);
    c.put16(match + 0x77c2, 0xffff);
    c.put8(match + 0x77c4, 0);
    c.put16(match + 0x77c0, 0xffff);
}

// The fixture's option nibbles (+0x4e) are limited to 9; the world is asked about the match.
void SetUp::fixture_options() {
    const auto set_mode = [&](std::uint32_t value) {
        const auto record = fixture();
        c.put8(record + fx_mode, (c.u8(record + fx_mode) & ~0x0cu) | ((value << 2) & 0x0c));
    };
    const auto high = (std::uint32_t(c.u8(c.u32(frame.at(st_fixture)) + fx_options)) >> 4) & 0xf;
    if (cmpu(high, 9) > 0) {
        const auto record = fixture();
        c.put8(record + fx_options, (c.u8(record + fx_options) & ~0xf0u) | (9u << 4));
        c.set(7, 9);
    }
    const auto low = c.u8(c.u32(frame.at(st_fixture)) + fx_options) & 0xfu;
    c.set(6, low);
    if (cmpu(low, 9) > 0) {
        c.set(6, 9);
        const auto record = fixture();
        c.set(9, record);
        const auto byte = (c.u8(record + fx_options) & ~0xfu) | 9u;
        c.set(10, byte);
        c.put8(record + fx_options, byte);
    }
    c.set({{3, c.u32(c.toc(-0x7aac))}, {4, match + 0x30d8}, {5, fixture()}});
    c.call(0x10433410, 0x101d18d8);
    c.set({{3, c.u32(c.toc(-0x7aac))}, {4, match + 0x30d8}, {5, fixture()}});
    c.call(0x10434ad0, 0x101d18ec);
    c.put8(match + 1, c.r(3));
    c.set({{3, c.u32(c.toc(-0x7aac))}, {4, fixture()}});
    c.call(0x10433da0, 0x101d1900);
    set_mode(c.r(3) & 0xff);
    const auto mode = (std::uint32_t(c.u8(fixture() + fx_mode)) >> 2) & 3;
    c.set(6, mode & 0x12);
    if (cmp(std::int32_t(mode & 0x12)) == 0) {
        set_mode(0);
        c.put8(hi - 0x1f48, 0xff);
    } else {
        // Mode 2 or 3: the byte at +0xe0b8 is drawn, 15..39 or 50..79.
        enter(c, 0x101d1938, {2});
        const auto coin = random_below(c, 2);
        c.set(8, u(coin));
        if (cmp(coin) != 0) {
            enter(c, 0x101d194c, {0x19});
            const auto draw = u(random_below(c, 0x19));
            c.set(7, draw);
            c.set(3, draw + 0xf);
            c.put8(hi - 0x1f48, u(s8(draw + 0xf)));
        } else {
            enter(c, 0x101d196c, {0x1e});
            const auto draw = u(random_below(c, 0x1e));
            c.set(7, draw + 0x32);
            c.put8(hi - 0x1f48, u(s8(draw + 0x32)));
        }
        c.set(6, fixture());
        c.set(7, (std::uint32_t(c.u8(fixture() + fx_mode)) >> 2) & 3);
        c.set(9, c.r(7) & 0x10);
        // Quirk: the original tests bit 4 of a two-bit field, which is never set.
        if (cmp(std::int32_t(c.r(7) & 0x10)) == 0) {
            if (cmp(sb(match + 1)) == 0 || cmp(sb(match + 1), 2) == 0 || cmp(sb(match + 1), 4) == 0) {
                set_mode(0);
                c.put8(hi - 0x1f48, 0xff);
            }
        }
    }
    // Pointers to areas inside the match object.
    c.put32(hi - 0x2014, hi - 0x7da4);
    c.put32(hi - 0x2010, hi - 0x5b7c);
    c.put32(match + 0x7934, match + 0x7d9a);
    c.put32(match + 0x7938, hi - 0x6986);
    c.put32(match + 0x793c, match + 0x7dbc);
    c.put32(match + 0x7940, hi - 0x6964);
    leave(c, {{3, hi}, {4, hi}, {5, hi}, {6, match + 0x7dbc}, {7, hi - 0x6964}});
}

// --- The table of bearings (degrees) between cells, built once per program run ---
void SetUp::angle_table() {
    if (cmp(sb(toc_at(-0x3911))) == 0) return;
    // atan(|numerator / denominator|) in degrees, optionally added to a pool constant.
    const auto bearing = [&](std::int32_t numerator_at, std::int32_t numerator, std::int32_t denominator_at,
                             std::int32_t denominator, std::uint32_t back, std::int32_t offset_pool,
                             std::int32_t result_at) {
        const double magic = pool(0x60);
        c.set_f(1, magic);
        const double top = fp.fsub(widen(numerator_at, u(numerator)), magic);
        c.set_f(1, magic);
        const double bottom = fp.fsub(widen(denominator_at, u(denominator)), magic);
        const double ratio = fp.fdiv(top, bottom);
        c.set_f(1, ratio);
        absolute(c, ratio);
        c.call(0x107810e0, back);
        c.set(2, c.u32(frame.at(0x14)));
        const double product = fp.fmul(pool(0), c.f(1));
        c.set_f(1, product);
        double degrees = fp.fdiv(product, pool(8));
        if (offset_pool >= 0) {
            c.set_f(1, degrees);
            degrees = fp.fadd(pool(u(offset_pool)), degrees);
        }
        c.put64(frame.at(result_at - 4), fp.fctiwz(0, degrees));
        return std::int32_t(c.u32(frame.at(result_at)));
    };
    for (c.set(20, u(-8)); cmp(s32(c.r(20)), 9) < 0; c.set(20, c.r(20) + 1)) {
        for (c.set(19, u(-11)); cmp(s32(c.r(19)), 12) < 0; c.set(19, c.r(19) + 1)) {
            const std::int32_t row = s32(c.r(20)), column = s32(c.r(19));
            std::int32_t angle = 0;
            if (cmp(row) == 0 && cmp(column) > 0) angle = 0;
            else if (cmp(row) == 0 && cmp(column) < 0) angle = 0xb4;
            else if (cmp(row) == 0 && cmp(column) == 0) angle = 0;
            else if (cmp(row) > 0 && cmp(column) == 0) angle = 0x5a;
            else if (cmp(row) < 0 && cmp(column) == 0) angle = 0x10e;
            else if (cmp(row) > 0 && cmp(column) > 0) {
                angle = bearing(0x288, row, 0x280, column, 0x101d1b54, -1, 0x27c);
                // Quirk: this quadrant keeps the unscaled product in f1.
                c.set(8, u(angle));
            } else if (cmp(row) > 0 && cmp(column) < 0) {
                angle = bearing(0x278, column, 0x280, row, 0x101d1bd0, 0x10, 0x28c);
                c.set(3, u(angle));
            } else if (cmp(row) < 0 && cmp(column) < 0) {
                angle = bearing(0x278, row, 0x280, column, 0x101d1c54, 0x18, 0x28c);
                c.set(3, u(angle));
            } else if (cmp(row) < 0 && cmp(column) > 0) {
                angle = bearing(0x278, column, 0x280, row, 0x101d1cd8, 0x20, 0x28c);
                c.set(15, u(angle));
            }
            c.put16(frame.at(0x50), u(angle));
            if (cmp(half(0x50), 0x167) > 0) c.put16(frame.at(0x50), 0);
            if (cmp(half(0x50)) < 0) c.put16(frame.at(0x50), u(half(0x50) + 0x168));
            c.put16(c.r(26) + u((row + 9) * 0x30) + u((column + 12) * 2), u(half(0x50)));
        }
    }
    c.put8(toc_at(-0x3911), 0);
}

// --- The match's counters, per-side tables and per-side statistics ---
void SetUp::clear_counters() {
    c.put16(hi - 0x1f46, 0);
    c.put8(match + 0x7930, 0);
    c.put16(match + clock_at, 0);
    c.put8(match + added_time, 0);
    c.put8(match + 0x792b, 0);
    c.put16(match + period_end, 0x1ef);
    c.put16(match + 0x794a, 0);
    c.put8(match + extra_time_flag, 0);
    c.put8(match + 0x1c2, 0);
    c.put8(match + 0x1c3, 0);
    c.put16(match + 0x7944, 0);
    c.put16(match + 0x7946, 0);
    c.put8(match + 4, 0xb);
    c.put8(match + 5, 0xb);
    c.put16(match + 0x1ec, 0x2710);
    c.put16(match + 0x1ee, 0x2710);
    c.put32(match + 0x3100, u(-1));
    c.put8(match + 0x1bd, 0);
    c.put8(match + 0x1e6, 0);
    c.put8(match + 8, 0);
    c.put8(match + 9, 0);
    c.put8(match + 6, 0);
    c.put8(match + 7, 0);
    c.put8(match + 0x1be, 0);
    c.put8(match + 0x1bf, 0);
    c.put32(match + 0x310c, 0);
    c.put32(match + 0x3108, 0);
    c.put8(match + 0x1c4, 0xff);
    c.put8(match + 0x1c5, 0xff);
    c.put_f32(match + 0x30f0, single(0));
    c.put_f32(match + 0x30f4, single(0));
    c.put8(match + 0x1de, 0);
    c.put8(match + 0x1df, 0);
    c.put8(match + 0x1e0, 0);
    c.put32(match + 0x3104, 0);
    c.put8(match + 0x7927, 0);
    c.put8(match + 0x1c0, 0);
    c.put8(match + 0x77be, 0);
    c.put8(match + 0x77bf, 0);
    c.set(3, match);
    call(c, 0x101d6930, 0x101d1eac);
    c.put32(match + 0x3110, 0);
    c.put8(match + 2, 0xff);
    c.put8(match + 3, 0xff);
    // Five slots of per-position tables.
    c.put8(frame.at(0x45), 0);
    while (cmp(sb(frame.at(0x45)), 5) < 0) {
        const auto slot = sb(frame.at(0x45));
        const auto word = u(slot) * 4, halfword = u(slot) * 2;
        c.put32(match + 0x797c + word, 0);
        c.put32(match + 0x79cc + word, 0);
        c.put32(match + 0x79a4 + word, 0);
        c.put32(match + 0x7990 + word, 0);
        c.put32(match + 0x79e0 + word, 0);
        c.put32(match + 0x79b8 + word, 0);
        c.put16(match + 0x210 + halfword, 0);
        c.put16(match + 0x21a + halfword, 0);
        c.put16(match + 0x224 + halfword, 0);
        c.put16(match + 0x22e + halfword, 0);
        c.set(15, u(slot));
        c.put8(frame.at(0x45), u(slot + 1));
    }
    static constexpr std::uint32_t cleared[] = {0xa, 0x41, 0x78, 0x77c7, 0x77fe, 0x7835, 0x786c, 0x78a3, 0x78da};
    static constexpr std::uint32_t backs[] = {0x101d1fec, 0x101d1ffc, 0x101d200c, 0x101d201c, 0x101d2030,
                                              0x101d2040, 0x101d2050, 0x101d2060, 0x101d2074};
    for (unsigned n = 0; n < 9; ++n) {
        memory_set(match + cleared[n], 0x37, backs[n]);
        if (n == 3 || n == 7) c.set(15, c.r(3));
    }
    c.put8(match + 0x7911, 0);
    c.put8(match + last_touch_side, 0xff);
    c.put16(match + 0x7950, 0xb4);
    c.put8(match + 0x7920, 0);
    c.put32(hi - 0x2000, 0);
    c.put8(match + shot_state, 0);
    c.put32(hi - 0x200c, 0);
    c.put32(hi - 0x2008, 0);
    c.put32(hi - 0x2004, 0);
    c.put32(hi - 0x1fd4, 0);
    c.put32(match + 0x7c68, 0);
    c.put32(hi - 0x1ffc, 0);
    c.put32(hi - 0x1ff8, 0);
    c.put32(hi - 0x1fdc, 0);
    c.put32(hi - 0x1fd8, 0);
    c.put32(hi - 0x1ff4, 0);
    c.put32(hi - 0x1ff0, 0);
    memory_set(match + 0xa08, 0x26d0, 0x101d2138);
    c.set(10, c.r(3));
    c.put32(hi - 0x1fe4, 0);
    c.put32(hi - 0x1fe0, 0);
    clear_sides();
}

// Each side's statistics block (0x36 bytes apart at +0xe03c...).
void SetUp::clear_sides() {
    static constexpr std::int32_t bytes_a[] = {-0x1fc4, -0x1fc3, -0x1fc2, -0x1fc1};
    static constexpr std::int32_t halves_a[] = {-0x1f9e, -0x1f9c, -0x1f9a, -0x1f98, -0x1f96, -0x1f94, -0x1f92};
    static constexpr std::int32_t bytes_b[] = {-0x1fbd, -0x1fbc, -0x1fbb, -0x1fba, -0x1fb9, -0x1fb8, -0x1fc0};
    static constexpr std::int32_t bytes_c[] = {-0x1fb7, -0x1fb6};
    static constexpr std::int32_t bytes_d[] = {-0x1fb5, -0x1fb4, -0x1fbe};
    static constexpr std::int32_t per_slot[] = {-0x1fb2, -0x1fad, -0x1fa8, -0x1fa3};
    for (c.set(22, 0); cmp(s8(c.r(22)), 2) < 0; c.set(22, c.r(22) + 1)) {
        const auto base = hi + u(s8(c.r(22)) * 0x36);
        for (const auto at : bytes_a) c.put8(base + u(at), 0);
        for (const auto at : halves_a) c.put16(base + u(at), 0);
        for (const auto at : bytes_b) c.put8(base + u(at), 0);
        c.set(15, base - match + u(-0x1fbf));
        c.put8(base + u(-0x1fbf), 0);
        for (const auto at : bytes_c) c.put8(base + u(at), 0);
        c.put16(base + u(-0x1f90), 0);
        for (const auto at : bytes_d) c.put8(base + u(at), 0);
        c.put8(frame.at(0x44), 0);
        while (cmp(sb(frame.at(0x44)), 5) < 0) {
            const auto slot = sb(frame.at(0x44));
            for (const auto at : per_slot) c.put8(base + u(slot) + u(at), 0);
            c.put8(frame.at(0x44), u(slot + 1));
        }
        c.put8(match + 0x77bc + u(s8(c.r(22))), 0);
    }
}

// The competition's structure is attached to the match (0x1021b020) and its rating limits drawn.
void SetUp::attach_world() {
    c.put_f32(hi - 0x1f58, single(4));
    c.put_f32(hi - 0x1f54, single(4));
    c.put_f32(hi - 0x1f50, single(8));
    c.put8(match + 0, c.u8(frame.at(st_inactive)));
    if (cmp(sb(c.toc(-0x642c))) == 0) {
        c.put8(match + 0, 1);
        c.put8(match + 0x7913, 0);
    }
    c.put8(match + 0x7931, c.u8(frame.at(st_silent)));
    // The two sides' lineup codes through the teams' tactics routine (0x101fd6b0).
    const auto team_b = team_of(1);
    c.set(15, team_b);
    const auto team_a = team_of(0);
    c.set({{3, team_a}, {4, team_b}, {5, 1}});
    call(c, 0x101fd6b0, 0x101d2510);
    c.put8(match + 0x77c5, c.r(3));
    const auto first = team_of(0);
    c.set(15, first);
    const auto second = team_of(1);
    c.set({{3, second}, {4, first}, {5, 0}});
    call(c, 0x101fd6b0, 0x101d2548);
    c.set(4, c.r(3));
    c.put8(match + 0x77c6, c.r(4));
    const auto attach = [&](std::uint32_t entry, std::uint32_t back) {
        c.set({{3, hi - 0x1fd0}, {4, match}, {5, entry}});
        c.call(0x1021b020, back);
    };
    const auto linked = c.u32(c.toc(-0x7428));
    if (cmpu(linked) != 0 && cmp(s32(c.u32(c.u32(frame.at(st_fixture)))), -2) != 0) {
        const auto index = c.i16(c.u32(frame.at(st_fixture)) + 0x2e);
        if (cmp(index, -1) > 0) {
            const auto table = c.u32(c.toc(-0x7ab4));
            attach(table + u(index * 0x2c), 0x101d25a8);
        } else {
            attach(0, 0x101d25c0);
        }
    } else {
        attach(0, 0x101d25d8);
    }
    if (cmp(s32(c.u32(fixture())), -2) != 0 && cmp(s32(c.u32(c.u32(fixture() + 0x14))), s32(c.u32(c.toc(-0x7aa8)))) != 0) {
        c.set({{3, c.u32(c.toc(-0x742c))}, {4, fixture()}, {5, match + 0x7c5c}});
        c.call(0x104b0b90, 0x101d2618);
        c.put32(hi - 0x1f4c, c.r(3));
    } else {
        c.put32(hi - 0x1f4c, 0);
        c.put32(match + 0x7c5c, 0);
        c.put32(match + 0x7c60, 0);
        c.put32(match + 0x7c64, 0);
    }
    if (cmp(s32(c.u32(match + 0x7c5c))) == 0) {
        c.put32(match + 0x7c5c, 100);
        c.put32(match + 0x7c60, 90);
        c.put32(match + 0x7c64, 10);
    }
}

// The two byte limits at +0x1c6 / +0x1c7 from the competition's weights (none for statistics fixtures).
void SetUp::rating_limits() {
    c.set(15, c.u32(fixture() + fx_competition));
    if (cmp(s32(c.r(15)), -2) == 0) {
        memory_set(match + 0x1c6, 2, 0x101d268c);
        return;
    }
    const double magic = pool(0x60);
    c.set_f(1, magic);
    c.set(15, 0x43300000);
    const double weight_b = fp.fsubs(widen(0x278, c.u32(match + 0x7c60)), magic);
    const double a = fp.fmul(pool(0x28), weight_b);
    c.set_f(1, magic);
    const double weight_a = fp.fsubs(widen(0x280, c.u32(match + 0x7c5c)), magic);
    const double f28 = fp.frsp(fp.fdiv(a, weight_a));
    c.set_f(28, f28);
    const double f20 = fp.frsp(fp.fsub(pool(0x28), f28));
    c.set_f(20, f20);
    c.set_f(1, magic);
    const double offset = fp.fsubs(widen(0x288, c.u32(match + 0x7c64)), magic);
    c.set_f(1, offset);
    const double low = fp.fadd(pool(0x30), fp.fdiv(offset, pool(0x38)));
    c.set_f(1, low);
    const double f31 = fp.frsp(fp.fsub(low, fp.fdiv(f28, pool(0x40))));
    c.set_f(31, f31);
    // min(f31, 0x48 constant) -> f27; then the larger of that bound and constant 0x50 -> f26.
    fp.fcmp(0, pool(0x48), f31);
    if (c.cr(0) & 8) c.set_f(27, pool(0x48));
    else c.set_f(27, f31);
    fp.fcmp(0, pool(0x50), c.f(27));
    if (c.cr(0) & 4) {
        c.set_f(26, pool(0x50));
    } else {
        fp.fcmp(0, pool(0x48), f31);
        if (c.cr(0) & 8) c.set_f(25, pool(0x48));
        else c.set_f(25, f31);
        c.set_f(26, c.f(25));
    }
    c.put64(frame.at(0x278), fp.fctiwz(0, c.f(26)));
    c.put8(match + 0x1c6, c.u32(frame.at(0x27c)));
    const auto comp = fixture();
    const auto nation = c.u32(c.u32(comp + fx_home) + 0x3c);
    double bonus;
    if (cmpu(nation, c.u32(comp + 0x18)) != 0 && cmpu(c.u32(c.u32(comp + fx_home) + 0x3c), c.u32(c.u32(comp + fx_away) + 0x3c)) != 0)
        bonus = single(0xc);
    else
        bonus = single(0x10);
    c.set_f(30, bonus);
    c.set_f(1, magic);
    const double offset2 = fp.fsubs(widen(0x278, c.u32(match + 0x7c60)), magic);
    c.set_f(1, offset2);
    const double sum = fp.fadd(pool(0x30), fp.fdiv(offset2, pool(0x38)));
    c.set_f(1, sum);
    const double g31 = fp.frsp(fp.fsub(sum, fp.fdiv(f20, pool(0x40))));
    c.set_f(31, g31);
    fp.fcmp(0, pool(0x48), g31);
    if (c.cr(0) & 8) c.set_f(24, pool(0x48));
    else c.set_f(24, g31);
    fp.fcmp(0, bonus, c.f(24));
    if (c.cr(0) & 4) {
        c.set_f(23, bonus);
    } else {
        fp.fcmp(0, pool(0x48), g31);
        if (c.cr(0) & 8) c.set_f(22, pool(0x48));
        else c.set_f(22, g31);
        c.set_f(23, c.f(22));
    }
    c.put64(frame.at(0x278), fp.fctiwz(0, c.f(23)));
    c.put8(match + 0x1c7, c.u32(frame.at(0x27c)));
}

// A club that cannot field eleven eligible players is reported through the assertion dialog.
void SetUp::report_shortage(bool away, std::uint32_t count) {
    const auto at = frame.at(away ? 0x74 : 0x84);
    const auto name_buffer = frame.at(away ? 0xe6 : 0x1ae);
    c.set({{3, at}, {4, u(c.i16(fixture() + 0x2a))}, {5, c.u16(fixture() + 0x28)}});
    c.call(0x107156b0, away ? 0x101d2a90 : 0x101d28dc);
    c.set({{3, at}, {4, name_buffer}, {5, 0xc8}, {6, 0xff}});
    c.call(0x107164f0, away ? 0x101d2aa4 : 0x101d28f0);
    const auto record = fixture();
    const auto id = s32(c.u32(record));
    const auto after_minus_one = cmp(id, -1);
    // The competition's name: a pool text for the two statistics ids (-1, -2), else the competition's own.
    std::uint32_t text, back;
    if (after_minus_one == 0) { text = c.r(16) + 0x3a; back = away ? 0x101d2b34 : 0x101d2980; }
    else if (after_minus_one > 0 || cmp(id, -2) < 0) { text = c.u32(record + 0x14) + 0x4d; back = away ? 0x101d2b74 : 0x101d29c0; }
    else { text = c.r(16) + 0x2a; back = away ? 0x101d2afc : 0x101d2948; }
    c.set({{3, c.toc(-0x7bb8)}, {4, away ? c.r(16) + 0x66 : c.r(16)}, {5, count},
           {6, c.u32(record + fx_home) + 0x221}, {7, c.u32(record + fx_away) + 0x221}, {8, text}, {9, name_buffer}});
    c.call(0x1077b720, back);
    Assertion assertion{away ? 0x101d2b88u : 0x101d29d4u, c.r(16) + 0x45, c.r(16) + 0x53, c.r(16) + 0x60,
                        frame.at(away ? 0xa8 : 0xc7), frame.at(away ? 0x6c : 0x7c), away ? 0x28cu : 0x264u};
    report_assertion(c, assertion);
    if (away) c.set(3, c.toc(-0x7bac));
    else c.set(4, c.toc(-0x7bac));
}

// Counts each club's eligible players (0x102065f0); too few is an error.
void SetUp::count_lineups() {
    c.put32(frame.at(0x38), fixture());
    c.put32(frame.at(0x3c), frame.at(0x47));
    c.set({{3, match + side_stats_at}, {4, match}, {5, c.u32(c.u32(fixture() + fx_home) + 0x90)},
           {6, c.u32(fixture() + fx_home) + 0x98}, {7, match + players_at}, {8, 0},
           {9, c.u32(fixture() + fx_home)}, {10, c.u32(fixture() + fx_away)}});
    c.call(0x102065f0, 0x101d28ac);
    c.s.pc = 0x101d28ac;  // the line-up boundary watches this one-byte store
    c.put8(frame.at(0x43), c.r(3));
    c.s.pc = 0x101d28b0;
    if (cmp(sb(frame.at(0x43)), 0xb) < 0) report_shortage(false, u(sb(frame.at(0x43))));
    c.put32(frame.at(0x38), fixture());
    c.put32(frame.at(0x3c), frame.at(0x46));
    c.set({{3, match + side_stats_at + side_stats_size}, {4, match}, {5, c.u32(c.u32(fixture() + fx_away) + 0x90)},
           {6, c.u32(fixture() + fx_away) + 0x98}, {7, match + players_at + side_size}, {8, 1},
           {9, c.u32(fixture() + fx_away)}, {10, c.u32(fixture() + fx_home)}});
    c.call(0x102065f0, 0x101d2a60);
    c.s.pc = 0x101d2a60;  // and this one
    c.put8(frame.at(0x42), c.r(3));
    c.s.pc = 0x101d2a64;
    if (cmp(sbyte(3, frame.at(0x42), 6), 0xb) < 0) report_shortage(true, u(sb(frame.at(0x42))));
    if (cmp(sb(frame.at(0x47)) + sb(frame.at(0x46)), 5) < 0 && cmp(sb(frame.at(st_silent))) == 0) c.put8(match, 1);
}

// The message object the match reports through: the caller's, or a new one.
bool SetUp::message_object() {
    c.set({{3, hi - 0x51d4}, {4, match + team_tactics}, {5, 0x31c0}});
    c.call(0x10739ef0, 0x101d2c10);
    c.set(15, 0x6e);
    c.put16(match + 0x1e8, 0x6e);
    c.set(5, 0x6e);
    c.put16(match + 0x1ea, 0x6e);
    c.set(3, 0);
    c.put32(match + message_target, 0);
    if (cmp(sb(match + 0x7930)) != 0) {
        c.put8(match, 1);
        c.put8(match + 0x7913, 0);
        leave(c, {{3, 0}, {4, u(sb(match + 0x7930)) }, {5, 0}});
        return false;
    }
    if (cmpu(c.u32(frame.at(st_target))) != 0) {
        c.put32(match + message_target, c.u32(frame.at(st_target)));
        c.put8(match + 0x7913, 0);
    } else {
        c.set(3, 0x4074);
        c.call(0x10773520, 0x101d2c78);
        c.put32(frame.at(0x68), c.r(3));
        if (cmpu(c.r(3)) != 0) {
            c.set({{3, c.u32(frame.at(0x68))}, {4, c.u8(match + 0x7931)}});
            c.call(0x101fef60, 0x101d2c90);
        }
        c.set(3, c.u32(frame.at(0x68)));
        c.put32(match + message_target, c.r(3));
        c.put8(match + 0x7913, 1);
        const auto object = c.u32(match + message_target);
        c.set({{3, object}, {4, 1}});
        set_byte_0(c, object, 1);
    }
    c.set({{3, match}, {4, 0}});
    call(c, 0x101d6a90, 0x101d2cb8);
    c.set({{3, match}, {4, 1}});
    call(c, 0x101d6a90, 0x101d2cc4);
    c.set(3, match);
    call(c, 0x101d6f90, 0x101d2ccc);
    c.put8(match + 0x791b, 1);
    return true;
}

// The first player of each side leaves its position code at +0xe04d / +0xe083.
bool SetUp::prepare_players() {
    const auto first_position = [&](std::uint32_t side, std::uint32_t back_test, std::uint32_t back_get, std::uint32_t at) {
        enter(c, back_test, {match, side});
        if (cmpu(side_player(c, match, side)) != 0) {
            enter(c, back_get, {match, side});
            const auto player = side_player(c, match, side);
            c.set(3, u(sb(player)));
            c.put8(at, c.r(3));
        } else {
            c.put8(at, 0xff);
        }
    };
    first_position(0, 0x101d2ce0, 0x101d2cf4, hi - 0x1fb3);
    first_position(1, 0x101d2d20, 0x101d2d34, hi - 0x1f7d);
    c.set(3, 0x10);
    c.call(0x10773520, 0x101d2d5c);
    c.put32(frame.at(0x64), c.r(3));
    if (cmpu(c.r(3)) != 0) {
        c.set(3, c.u32(frame.at(0x64)));
        c.call(0x10738c00, 0x101d2d70);
    }
    c.put32(match + 0x3114, c.u32(frame.at(0x64)));
    if (cmpu(c.u32(match + 0x3114)) == 0) return false;
    c.set(3, c.u32(fixture() + fx_home));
    c.call(0x100c7d00, 0x101d2d90);
    if (cmp(s8(c.r(3))) != 0) return false;
    if (cmp(sbyte(3, match, 3)) != 0) return false;
    c.set(3, c.u32(frame.at(st_fixture)));
    if (cmp(sbyte(5, c.r(3) + 0x40, 3)) == 0) return false;
    c.set(3, fixture());
    if (cmpu(word(0, c.r(3) + 0x14)) == 0) return false;
    c.set(3, c.u32(c.toc(-0x7b54)));
    if (cmpu(c.r(3)) != 0) {
        c.set({{3, c.u32(c.toc(-0x7b54))}, {4, c.u32(fixture() + 0x14)}});
        c.call(0x10494d20, 0x101d2df4);
        if (cmp(s8(c.r(3))) != 0) return false;
    }
    return true;
}

// Posts the 0x1c-byte event record built on the stack to the match's message object.
void SetUp::post_event(std::uint32_t player, std::uint32_t team, std::uint32_t argument, std::uint32_t flag,
                       std::uint32_t slot, std::uint32_t kind, std::uint32_t back) {
    c.put8(frame.at(0x9a), 0);
    c.put32(frame.at(0x9c), u(-1));
    c.put32(frame.at(0x90), team);
    c.put8(frame.at(0x98), flag);
    c.put32(frame.at(0x8c), player);
    c.put8(frame.at(0x99), slot);
    c.put16(frame.at(0xa0), kind);
    c.put32(frame.at(0x94), argument);
    c.set({{3, c.u32(match + 0x3114)}, {4, frame.at(0x8c)}, {5, 0x1c}});
    c.call(0x10738d10, back);
}

// Both sides' players, one at a time; a side is abandoned when its team has no club record.
void SetUp::run_players() {
    c.set(4, u(-1));
    c.put32(frame.at(0xa4), u(-1));
    c.put8(frame.at(0x41), 0);
    for (;;) {
        const auto side_byte = sbyte(7, frame.at(0x41), 3);
        if (cmp(side_byte, 2) >= 0) break;
        const auto first = u(sbyte(4, frame.at(0x41)) * 0x2350) + 0x311c;
        c.set(4, first);
        c.set(18, match + first);
        c.set(4, 0);
        c.put32(frame.at(0x60), 0);
        while (cmp(s32(word(3, frame.at(0x60))), 0x14) < 0) {
            if (!assess_player()) break;
            const auto index = word(4, frame.at(0x60));
            c.put32(frame.at(0x60), index + 1);
            c.set(3, c.r(18));
            c.set(18, c.r(18) + 0x1c4);
            c.put32(frame.at(0x54), c.r(3));
        }
        const auto side = ubyte(4, frame.at(0x41));
        c.set(3, side + 1);
        c.put8(frame.at(0x41), side + 1);
    }
}

// One player (r18): is there a record, a team and a club object to judge it by? False abandons the side.
bool SetUp::assess_player() {
    const auto me = c.r(18);
    c.set(3, me);
    if (cmp(s8(position(c, me))) < 0) return true;
    const auto record = word(27, me + 0x6c);
    c.set(3, match);
    const auto side = ubyte(4, frame.at(0x41));
    const auto team = fixture_team(c, match, side);
    c.set(21, team);
    c.set(3, team);
    c.call(0x100c7d00, 0x101d2e64);
    c.set(4, u(s8(c.r(3))));
    if (cmp(s8(c.r(3))) != 0) return false;
    if (cmpu(word(0, team + 0x2c)) == 0) return false;
    if (cmpu(word(3, record + 0x14)) == 0) return true;
    c.set({{3, c.u32(c.r(25))}, {4, record}, {5, team}});
    c.call(0x10082f90, 0x101d2e98);
    c.put32(frame.at(0x5c), c.r(3));
    const auto object = word(3, frame.at(0x5c));
    if (cmpu(object) == 0) return true;
    c.set(3, record);
    c.call(0x100db1b0, 0x101d2eb0);
    c.put16(frame.at(0x4e), c.r(3));
    c.set(3, me);
    c.call(0x10289550, 0x101d2ec0);
    c.put16(frame.at(0x4a), c.r(3));
    c.set(3, me);
    c.call(0x102895e0, 0x101d2ed0);
    c.set(4, c.r(3));
    c.put16(frame.at(0x4c), c.r(3));
    // The first event: the object is flagged (0x40 of its first status or 0x02000000 of its word at
    // +0x50), the player has a first halfword and the three halfwords disagree.
    bool event = false;
    if (cmpu(word(0, frame.at(0x5c))) != 0) {
        c.set(3, object);
        c.call(0x1037fa70, 0x101d2eec);
        bool flagged = cmp(s32(c.r(3) & 0x40)) != 0;
        if (!flagged) {
            c.set(3, object);
            const auto status = word_50(c, object);
            c.set(4, status);
            c.set(4, status & 0x02000000);
            flagged = cmp(s32(status & 0x02000000)) != 0;
        }
        if (flagged && cmp(shalf(15, frame.at(0x4e))) != 0) {
            const auto a = shalf(3, frame.at(0x4c)), b = shalf(0, frame.at(0x4e));
            shalf(15, frame.at(0x4c));
            c.set(4, u(b & a));
            if (cmp(a, b & a) == 0) {
                event = true;
            } else {
                const auto d = shalf(4, frame.at(0x4a));
                shalf(5, frame.at(0x4e));
                shalf(0, frame.at(0x4a));
                c.set(3, u(b & d));
                event = cmp(d, b & d) != 0;
            }
        }
    }
    if (event) {
        const auto index = word(5, frame.at(0x60));
        c.set(5, u(s8(index)));
        post_event(c.u32(record), c.u32(team), 0, 0, u(s8(index)), 0x13, 0x101d2fa0);
    } else {
        assess_transfer(record, team, object);
    }
    // A strong player of a weak club: a chance event.
    if (cmpu(team) != 0) {
        enter(c, 0x101d342c, {7});
        const auto roll = random_below(c, 7);
        const auto club = word(3, me + 0x70);
        bool hit = cmp(sbyte(3, club + 0x2f, 3), roll + 6) > 0;
        if (hit) {
            const auto club4 = word(4, me + 0x70);
            hit = cmp(shalf(0, club4 + 0xa), 0xea6) > 0;
        }
        if (hit) hit = cmp(sbyte(3, record + 0x67, 3), 7) < 0;
        if (hit) {
            enter(c, 0x101d346c, {0xf});
            const auto roll2 = random_below(c, 0xf);
            c.set(4, u(roll2));
            hit = cmp(sbyte(3, record + 0x67, 0), roll2) < 0;
        }
        if (hit) {
            enter(c, 0x101d3488, {u(shalf(3, team + 0x1ec))});
            const auto own = random_below(c, c.i16(team + 0x1ec));
            c.set(15, u(own));
            const auto club5 = word(5, me + 0x70);
            enter(c, 0x101d3498, {u(shalf(3, club5 + 0xa))});
            const auto other = random_below(c, c.i16(club5 + 0xa));
            hit = cmp(other, own) > 0;
        }
        if (hit) {
            c.set(3, word(3, frame.at(0x5c)));
            const auto status = word_50(c, c.r(3));
            c.set(4, 0x2f7a0b);
            hit = cmp(s32(status & 0x2f7a0b)) != 0;
        }
        if (hit) {
            // The record carries the object's status word and whether this is the side's first player.
            c.set(3, word(3, frame.at(0x5c)));
            const auto argument = word_50(c, c.r(3));
            c.set(4, argument);
            const auto side_now = ubyte(4, frame.at(0x41));
            enter(c, 0x101d3508, {match, side_now});
            const auto first = side_player(c, match, side_now);
            post_event(c.u32(record), c.u32(team), argument, compare_logical(c, me, first) == 0, side_now, 0x1e,
                       0x101d3538);
        }
    }
    assess_form(record, team, object);
    return true;
}

// The weighing of a transfer-minded player against his club (0x2fa4...).
void SetUp::assess_transfer(std::uint32_t record, std::uint32_t team, std::uint32_t object) {
    const auto add = [&](std::int32_t amount) { c.set(23, c.r(23) + u(amount)); };
    if (cmp(sbyte(3, record + 0x62, 3), 0xc) == 0) return;
    if (cmp(shalf(0, frame.at(0x4e))) == 0) return;
    {
        const auto a = shalf(5, frame.at(0x4c)), b = shalf(3, frame.at(0x4e)), d = shalf(4, frame.at(0x4c));
        c.set(3, u(b & d));
        if (cmp(a, b & d) == 0) return;
    }
    {
        const auto a = shalf(4, frame.at(0x4a)), b = shalf(0, frame.at(0x4e)), d = shalf(3, frame.at(0x4a));
        c.set(3, u(b & d));
        if (cmp(a, b & d) == 0) return;
    }
    std::int32_t pace;
    if (cmp(shalf(0, frame.at(0x4e)), 1) == 0) {
        c.set(3, 6);
        pace = 6;
    } else {
        // Quirk: `x | 4` is never zero, so these tests always pass (and the later ones never fire).
        bool two = false;
        const auto h = shalf(3, frame.at(0x4c)) | 4;
        c.set(3, u(h));
        if (cmp(h) != 0) {
            two = cmp(sbyte(3, word(3, record + 0x14) + 0x11, 3), 0xa) < 0;
            if (two) {
                const auto attributes = word(15, record + 0x14);
                two = cmp(sbyte(3, attributes + 0x12, 3), 0xa) < 0;
            }
        }
        if (two) {
            c.set(3, 2);
            pace = 2;
        } else {
            const auto g = shalf(15, frame.at(0x4c)) | 4;
            c.set(3, u(g));
            cmp(g);
            const auto k = shalf(3, frame.at(0x4e)) | 4;
            c.set(3, u(k));
            cmp(k);
            pace = -1;
        }
    }
    c.put16(frame.at(0x48), u(pace));
    if (cmpu(team) == 0) return;
    if (cmp(shalf(3, word(3, record + 0x14) + 0xa), 0xcb2) <= 0) return;
    {
        const auto attributes = word(4, record + 0x14);
        if (cmp(shalf(3, attributes + 0xa), 0x1d4c) <= 0) {
            const auto attributes3 = word(3, record + 0x14);
            const auto rating = shalf(7, attributes3 + 0xa);
            const auto pace_now = shalf(3, frame.at(0x48));
            c.set(4, u(pace_now * 0x1f4));
            ubyte(3, record + 0x63);
            const auto physical = sbyte(5, record + 0x63, 3);
            c.set(5, u(physical * 0x19));
            c.set(6, u(pace_now * 0x1f4 + physical * 0x19));
            c.set(6, u(rating + pace_now * 0x1f4 + physical * 0x19));
            const auto total = std::int16_t(rating + pace_now * 0x1f4 + physical * 0x19);
            c.set(9, u(std::int32_t(total)));
            if (cmp(total, shalf(10, team + 0x1ec)) <= 0) return;
        }
    }
    if (cmp(sbyte(4, record + 0x67, 3), 0xf) >= 0) return;
    if (cmp(sbyte(4, record + 0x63, 3), 5) <= 0) return;
    {
        const auto a = sbyte(5, record + 0x67, 0);
        const auto b = sbyte(4, record + 0x69, 3);
        c.set(3, u(a + b));
        if (cmp(a + b, 0x19) >= 0) return;
    }
    c.set(23, 0);
    if (cmp(shalf(0, frame.at(0x48)), 6) == 0) {
        add(0x19);
    } else if (cmp(shalf(6, frame.at(0x48)), 2) == 0) {
        add(5);
    } else if (cmp(shalf(6, frame.at(0x48)), 0) == 0) {
        add(-3);
    }
    if (cmp(sbyte(7, record + 0x67, 8), 1) == 0) {
        add(0x14);
    } else if (cmp(sbyte(6, record + 0x67, 3), 6) < 0) {
        add(0xa);
    } else if (cmp(sbyte(7, record + 0x67, 8), 0xf) < 0) {
        add(5);
    } else if (cmp(sbyte(3, record + 0x67, 7), 0x14) < 0) {
        add(3);
    } else {
        add(1);
    }
    {
        const auto h = shalf(8, frame.at(0x4e));
        c.set(6, u(h | 4));
        if (cmp(h | 4) != 0) add(-5);  // the same quirk
    }
    if (cmp(sbyte(7, record + 0x63, 7), 1) == 0) {
        add(-3);
    } else if (cmp(sbyte(8, record + 0x63, 9), 6) < 0) {
        add(-1);
    }
    {
        const auto attributes = word(3, record + 0x14);
        if (cmp(shalf(5, attributes + 0xa), 0x186a) > 0) {
            if (cmp(sbyte(0, record + 0x63, 3), 0xf) > 0 && cmp(sbyte(4, record + 0x67, 4), 6) < 0) {
                add(5);
            } else if (cmp(sbyte(5, record + 0x67, 3), 0xf) > 0) {
                add(1);
            } else {
                add(3);
            }
        }
    }
    {
        bool more = cmp(sbyte(6, record + 0x66, 5), 0xa) > 0;
        if (more) {
            const auto form = shalf(6, record + 0x5c);
            c.set(9, u(s8(u(form))));
            more = cmp(s8(u(form)), 0x18) < 0;
        }
        if (more) more = cmp(sbyte(6, record + 0x63, 3), 0xf) > 0;
        if (more) add(5);
    }
    if (cmp(sbyte(5, record + 0x66, 3), 1) == 0) {
        add(-0xa);
    } else if (cmp(sbyte(5, record + 0x66, 4), 6) < 0) {
        add(-5);
    }
    c.set(3, object);
    if (cmp(s32(word_50(c, object))) != 0) {
        add(5);
    } else {
        c.set(3, object);
        c.call(0x1037fa70, 0x101d3324);
        c.set(4, c.r(3));
        if (cmp(s32(c.r(3))) != 0) add(1);
        else add(-3);
    }
    c.set(3, object);
    c.call(0x1037fa70, 0x101d3344);
    c.set(3, c.r(3) & 0x40);
    if (cmp(s32(c.r(3))) != 0) add(5);
    c.set({{3, record}, {4, team}});
    c.call(0x100da0b0, 0x101d3360);
    c.set(6, c.r(3));
    c.set(6, u(s8(c.r(6))));
    if (cmp(s8(c.r(6))) != 0) add(-3);
    if (cmpu(word(7, team + 0x90)) != 0) {
        c.set({{3, record}, {4, c.u32(team + 0x90)}});
        c.call(0x100d9520, 0x101d338c);
        c.set(7, c.r(3));
        c.set(3, u(s8(c.r(7))));
        if (cmp(s8(c.r(7))) != 0) add(-5);
    }
    c.set(6, u(std::int32_t(std::int16_t(c.r(23)))));
    if (cmp(std::int16_t(c.r(23)), 5) <= 0) return;
    enter(c, 0x101d33b4, {0x1e});
    const auto roll = random_below(c, 0x1e);
    c.set(6, u(roll));
    c.set(8, u(std::int32_t(std::int16_t(c.r(23)))));
    if (cmp(std::int16_t(c.r(23)), roll) <= 0) return;
    // The event record's scratch registers stay behind: the team and player ids, the index, the kind.
    const auto index = c.u32(frame.at(0x60));
    c.set(6, 0);
    c.set(7, index);
    c.set(8, 0x13);
    post_event(c.u32(record), c.u32(team), 0x40, 0, u(s8(index)), 0x13, 0x101d3418);
    c.set(5, c.r(3));
}

// The player's form (r17) against the club's ratings: a poor or a strong run is reported.
void SetUp::assess_form(std::uint32_t record, std::uint32_t team, std::uint32_t object) {
    const auto me = c.r(18);
    const auto adjust = [&](std::int32_t amount) { c.set(17, c.r(17) + u(amount)); };
    // Whether this player is the side's first.
    const auto first_flag = [&](std::uint32_t back) {
        const auto side_now = ubyte(4, frame.at(0x41));
        enter(c, back, {match, side_now});
        const auto first = side_player(c, match, side_now);
        return std::uint32_t(compare_logical(c, me, first) == 0);
    };
    // The lookup (0x10303560) of one of the attribute's grades, in the pool's single.
    const auto grade = [&](std::uint32_t kind, std::uint32_t back) {
        c.set({{3, c.u32(c.r(29))}, {4, c.u32(record + 0x14)}, {5, 6}, {6, kind}});
        c.set_f(1, single(0x14));
        c.call(0x10303560, back);
        return c.f(1);
    };
    // An attribute byte as the original loads it: the attribute pointer in `at`, the byte
    // in `raw` and sign-extended in `signed_to`.
    const auto attribute = [&](unsigned at, unsigned signed_to, unsigned raw, std::uint32_t offset) {
        const auto attributes = word(at, record + 0x14);
        return sbyte(signed_to, attributes + offset, raw);
    };
    shalf(0, record + 0x5c);
    c.set(17, u(s8(u(c.i16(record + 0x5c)))));
    if (cmpu(word(3, record + 0x14)) != 0) {
        if (cmp(attribute(4, 4, 3, 0xf), 0x14) == 0) adjust(-2);
        else if (cmp(attribute(3, 3, 3, 0x11), 0x14) == 0) adjust(-1);
        else if (cmp(attribute(3, 0, 5, 0x15), 0x14) == 0 || cmp(attribute(4, 3, 3, 0x14), 0x14) == 0) adjust(1);
        if (cmp(attribute(3, 4, 3, 0xf), 0x14) != 0 && cmp(attribute(3, 3, 0, 0x3c), 0xf) > 0) adjust(-2);
        else if (cmp(attribute(3, 3, 3, 0x3c), 0xa) > 0) adjust(-1);
        else if (cmp(attribute(5, 0, 3, 0xf), 0x14) != 0 && cmp(attribute(3, 3, 3, 0x3c), 6) < 0) adjust(1);
    }
    c.set(4, u(s8(c.r(17))));
    if (cmp(s8(c.r(17)), 0x17) < 0) {
        c.set_f(19, grade(1, 0x101d364c));
        c.set_f(18, grade(0x11, 0x101d3668));
        const auto raw67 = ubyte(3, record + 0x67);
        const auto form = shalf(0, record + 0x5c);
        c.set(4, u(s8(u(form))));
        const auto sum = u(s8(u(form))) + c.u8(record + 0x66) + raw67 - 0x1e;
        c.set(4, sum);
        c.set(4, u(s8(sum)));
        c.put8(frame.at(0x40), sum);
        const auto mark = sbyte(0, frame.at(0x40), 3);
        if (cmp(mark, 4) < 0) {
            c.set(3, 4);
            c.put8(frame.at(0x40), 4);
        } else if (c.set(3, u(s8(c.r(17)))), cmp(s8(c.r(17)), 0x12) < 0 && cmp(sbyte(0, frame.at(0x40), 4), 0x19) > 0) {
            if (cmp(attribute(3, 3, 3, 0x3c), 0xa) < 0) {
                const auto attributes = word(3, record + 0x14);
                const auto grade_byte = ubyte(0, attributes + 0x3c);
                c.set(0, 0x19 - grade_byte);
                c.set(4, u(s8(0x19 - grade_byte)));
                c.put8(frame.at(0x40), 0x19 - grade_byte);
            } else {
                c.put8(frame.at(0x40), 0x19);
            }
        }
        bool chance = cmpu(word(3, record + 0x14)) != 0 && cmpu(ubyte(0, record + 0x5e)) == 0;
        if (chance) {
            const auto attributes = word(4, record + 0x14);
            chance = cmp(shalf(4, attributes + 0xa), 0x1b58) < 0;
        }
        if (chance) {
            const auto rating = shalf(15, team + 0x1ec);
            chance = cmp(rating, 0xcb2) > 0;
        }
        if (chance) {
            const auto limit = sbyte(0, frame.at(0x40), 3);
            c.set_f(1, pool(0x60));
            c.set(15, u(limit) ^ 0x80000000);
            c.set(4, 0x43300000);
            const double bound = fp.fsubs(widen(0x278, u(limit)), pool(0x60));
            fp.fcmp(0, c.f(19), bound);
            chance = (c.cr(0) & 4) != 0;
        }
        if (chance) {
            const auto arg = word(4, frame.at(st_fixture));
            chance = cmp(sbyte(0, arg + 0x40, 5)) != 0;
        }
        if (chance) {
            const auto limit = sbyte(3, frame.at(0x40), 3);
            enter(c, 0x101d377c, {u(limit)});
            chance = cmp(random_below(c, limit)) == 0;
        }
        if (chance) {
            c.set({{3, c.u32(c.r(29))}, {4, 0}});
            const double rate = side_rate(c, c.r(3), 0);
            fp.fcmp(0, c.f(18), rate);
            chance = (c.cr(0) & 8) != 0;
        }
        if (chance) {
            c.set(3, object);
            const auto argument = word_50(c, object);
            const auto flag = first_flag(0x101d37e0);
            c.set(5, c.r(3));
            post_event(c.u32(record), c.u32(team), argument, flag, c.u8(frame.at(0x41)), 5, 0x101d3810);
        }
    } else if (c.set(3, u(s8(c.r(17)))), cmp(s8(c.r(17)), 0x20) > 0) {
        const double high = grade(1, 0x101d3838);
        c.set_f(29, high);
        const double low = grade(0x11, 0x101d3854);
        c.put_f32(frame.at(0x58), float(low));
        fp.fcmp(0, high, single(0x18));
        if (c.cr(0) & 4) {
            const double ratio = fp.fdiv(high, pool(0x48));
            c.set_f(1, ratio);
            fp.fcmp(0, ratio, pool(0x58));
            if (c.cr(0) & 8) c.set_f(21, fp.fdiv(high, pool(0x48)));
            else c.set_f(21, pool(0x58));
            c.set({{3, c.u32(c.r(29))}, {4, 0}});
            const double rate = side_rate(c, c.r(3), 0);
            const double gap = fp.fsub(rate, pool(0x58));
            const double reach = fp.fadd(c.f(21), gap);
            c.set_f(1, reach);
            fp.fcmp(0, c.f32(frame.at(0x58)), reach);
            if (c.cr(0) & 8) {
                c.set(3, object);
                const auto argument = word_50(c, object);
                const auto flag = first_flag(0x101d38f4);
                post_event(c.u32(record), c.u32(team), argument, flag, c.u8(frame.at(0x41)), 4, 0x101d3920);
            }
        }
    }
    assess_career(record, team, object);
}

// Last check of each player: a late-career player of rare skill is reported.
void SetUp::assess_career(std::uint32_t record, std::uint32_t team, std::uint32_t object) {
    (void)object;
    const auto weight = sbyte(3, record + 0x69, 3);
    enter(c, 0x101d392c, {u(weight)});
    if (cmp(random_below(c, weight)) != 0) return;
    const auto attributes = word(3, record + 0x14);
    if (cmp(sbyte(3, attributes + 0x25, 3), 0xf) <= 0) return;
    enter(c, 0x101d3950, {0xc8});
    const auto roll = u(random_below(c, 0xc8)) + 0x32;
    c.set(15, roll);
    c.set({{3, c.r(24)}, {4, word(4, record + 0x4c)}, {5, word(5, record + 0x50)}});
    c.call(0x10718130, 0x101d3964);
    if (cmp(s32(c.r(3)), s32(roll)) <= 0) return;
    c.set(3, u(s8(c.r(17))));
    if (cmp(s8(c.r(17)), 0x17) <= 0) return;
    enter(c, 0x101d3980, {4});
    const auto need = u(random_below(c, 4)) + 2;
    c.set(15, need);
    c.set(3, word(3, frame.at(0x5c)));
    const auto level = signed_byte_46(c, c.r(3));
    c.set(3, level);
    if (cmp(s8(level), s32(need)) <= 0) return;
    c.set(3, word(3, frame.at(0x5c)));
    const auto argument = word_50(c, c.r(3));
    const auto side = ubyte(3, frame.at(0x41));
    c.set(4, 0);
    c.set(3, u(-1));
    c.set(4, 6);
    enter(c, 0x101d39e0, {match, side});
    const auto first = side_player(c, match, side);
    const bool same = compare_logical(c, c.r(18), first) == 0;
    if (same) c.set(4, 1);
    else c.set(5, 0);
    post_event(c.u32(record), c.u32(team), argument, same, side, 6, 0x101d3a0c);
}

// The coin toss, unless the match is not played here.
void SetUp::kickoff() {
    const auto inactive = sbyte(4, match, 3);
    if (cmp(inactive) != 0) return;
    enter(c, 0x101d3a64, {2});
    if (cmp(random_below(c, 2)) != 0) {
        c.set(5, 1);
        c.put8(match + 0x792c, 1);
        c.set(5, 0);
        c.put8(match + possession_side, 0);
    } else {
        c.set(6, 0);
        c.put8(match + 0x792c, 0);
        c.set(6, 1);
        c.put8(match + possession_side, 1);
    }
    c.set(9, 8);
    c.put8(match + play_state, 8);
    c.set(10, 0);
    c.put8(match + 0x1e3, 0);
}

std::uint32_t SetUp::run(std::uint32_t inactive, std::uint32_t target, std::uint32_t silent) {
    c.put32(frame.at(st_fixture), fixture_arg);
    c.put8(frame.at(st_inactive), inactive);
    c.put32(frame.at(st_target), target);
    c.put8(frame.at(st_silent), silent);
    begin();
    fixture_options();
    angle_table();
    clear_counters();
    attach_world();
    rating_limits();
    count_lineups();
    if (!message_object()) return 0;
    if (prepare_players()) run_players();
    kickoff();
    return 1;
}
}  // namespace

std::uint32_t set_up_match(Cpu& c, std::uint32_t match, std::uint32_t fixture, std::uint32_t inactive,
                           std::uint32_t target, std::uint32_t silent) {
    SetUp setup(c, match, fixture);
    const auto done = setup.run(inactive, target, silent);
    if (done) c.set(3, 1);
    return done;
}
}
