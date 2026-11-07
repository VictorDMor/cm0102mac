// DATABASE.CPP's init block: "Player Initialisation" (0x100b8cf0), the loop and the helpers its phases share.
#include "database_generation.hpp"
#include "database_init.hpp"
#include "database_records.hpp"
#include "database_init_detail.hpp"
#include "database_people_attributes.hpp"

namespace cm::aot::database {
namespace init {
Gen::Gen(Cpu& cpu, guest::Frame& frame, std::uint32_t date_in) : c(cpu), f(frame), fpu(cpu) {
    for (unsigned n = 13; n < 32; ++n) g[n] = c.r(n);
    count_word = c.toc(-0x7be4);
    pool = generation::load_pool(c);
    date = date_in;
    g[13] = c.toc(-0x7ba8);
    g[14] = count_word;
    g[17] = pool.salt_at;
    g[18] = pool.start;
    g[19] = pool.end_at;
    g[20] = pool.cursor_at;
    g[21] = pool.constants;
    g[22] = date;
    g[29] = u32(-0x7961);  // the smallest attribute distance so far
}

void Gen::sync() {
    for (unsigned n = 13; n < 32; ++n) c.set(n, g[n]);
}
void Gen::call(std::uint32_t target, std::uint32_t back) {
    sync();
    c.call_in_place(target, back);
}
void Gen::enter(std::uint32_t back, std::initializer_list<std::uint32_t> arguments) {
    sync();
    guest::enter(c, back, arguments);
}

std::int32_t Gen::next() {
    generation::advance(c, pool, 0x100b8fb0);  // a wrap draws from rng_core, a leaf: the site does not matter
    return std::int32_t(c.u16(c.u32(pool.cursor_at)) + c.u32(pool.salt_at));
}

std::int32_t Gen::below(std::int32_t bound) {
    if (bound == 0) return 0;
    generation::advance(c, pool, 0x100b8fb0);
    return generation::draw_below(c, pool, bound, 3);  // every caller's bound is narrow (|bound| <= 0xffff)
}

double Gen::converted(std::uint32_t slot_offset, std::int32_t value) {
    c.put32(at(std::int32_t(slot_offset)), 0x43300000);
    c.put32(at(std::int32_t(slot_offset)) + 4, std::uint32_t(value) ^ 0x80000000u);
    return c.f64(at(std::int32_t(slot_offset)));
}

std::int32_t Gen::scaled(std::int32_t value, std::uint32_t k_offset, std::uint32_t in, std::uint32_t out,
                         bool constant_in_f1) {
    const auto magic = constant(0x18), k = constant(k_offset);
    const auto x = converted(in, value);
    // f1 holds the conversion constant and f2 the factor, except where the factor replaces the constant in f1.
    c.set_f(1, magic);
    if (!constant_in_f1) c.set_f(2, k);
    const auto minus = fpu.fsub(x, magic);
    c.set_f(0, minus);
    if (constant_in_f1) c.set_f(1, k);
    const auto product = fpu.fmul(k, minus);
    const auto bits = fpu.fctiwz(0, product);
    c.put64(at(std::int32_t(out)), bits);
    return std::int32_t(std::uint32_t(bits));
}

// The wide branch of the inline draw (a bound beyond +-65535): the sample reduced modulo 65535 scales the
// bound in floating point, 0x100a0d80 draws a bias from a tenth of the bound's scaling, and the sum is
// clamped to bound - 1. The result sits in r13 across the call.
std::int32_t Gen::below_wide(std::int32_t bound, const WideSlots& slots, std::uint32_t back) {
    if (bound == 0) return 0;
    const auto sample = next();
    if (bound <= 0xffff && bound >= -0xffff) {
        const auto remainder = rem_s(sample, bound);
        return bound > 0 ? remainder : std::int32_t(0u - std::uint32_t(remainder));
    }
    // lhz r8,0(r7); lwz r7,0(r17); add r7,r8,r7: the sample and its sum stay in r8 and r7 across the call.
    c.set({{8, c.u16(c.u32(pool.cursor_at))}, {7, u32(sample)}});
    const auto magic = constant(0x18), divisor = constant(0x10);
    const auto bound_double = converted(slots.first, bound);
    c.set_f(2, magic);
    c.set_f(0, divisor);
    const auto minus = fpu.fsub(bound_double, magic);
    c.set_f(1, minus);
    const auto ratio = fpu.fdiv(minus, divisor);
    c.set_f(2, ratio);
    const auto v = rem_s(sample, 65535);
    c.set_f(1, magic);
    const auto v_double = converted(slots.second, v);
    const auto v_minus = fpu.fsub(v_double, magic);
    c.set_f(0, v_minus);
    const auto product = fpu.fmul(ratio, v_minus);
    const auto bits = fpu.fctiwz(0, product);
    c.put64(at(std::int32_t(slots.result)), bits);
    g[13] = std::uint32_t(bits);
    // The registers the original holds at the call (range_sample leaves most of them as it finds them): r4 the
    // residue flipped for the conversion, r5 0xffff, r6 the sign of the sample's quotient by 65535, r0 the
    // scaled bound before its sign is added.
    const auto high = [](std::int32_t a, std::int32_t b) { return std::int32_t((std::int64_t(a) * b) >> 32); };
    const auto reciprocal = std::int32_t(0x80008001u);
    const auto quotient = (high(reciprocal, sample) + sample) >> 15;
    const auto times_ten = std::int32_t(std::uint32_t(bound) * 10u);
    const auto tenth_before = (high(reciprocal, times_ten) + times_ten) >> 15;
    const auto tenth = tenth_before + std::int32_t(std::uint32_t(tenth_before) >> 31);
    c.set({{4, u32(v) ^ 0x80000000u}, {5, 0xffff}, {6, std::uint32_t(quotient) >> 31}, {0, u32(tenth_before)}});
    enter(back, {u32(tenth)});
    const auto bias = std::int32_t(range_sample(c, tenth));
    auto value = std::int32_t(g[13]) + div_s(bias + 5, 10);
    if (value > bound - 1) value = bound - 1;
    g[13] = u32(value);
    return value;
}

// The loop over the person table.
void Gen::loop() {
    g[31] = c.u32(person_table_slot());
    c.put32(at(0x314), 0);
    while (std::int32_t(c.u32(at(0x314))) < std::int32_t(c.u32(count_word))) {
        const auto p = g[31];
        person_names(p);
        person_dates(p);
        person_progress();
        person_ratings(p);
        person_nation(p);
        const auto player = c.u32(p + person::player);
        g[30] = player;
        if (player != 0) player_abilities(p, player);
        else staff_only(p);
        staff_record(p);
        // 100c24c8: the age again, then the next row.
        enter(0x100c24d4, {p, date});
        const auto age = database::date_difference(c, p, date);
        c.put16(p + person::age, u32(s8(std::uint32_t(age))));
        g[31] = p + 0x70;
        c.put32(at(0x314), c.u32(at(0x314)) + 1);
    }
}
}

void player_initialisation(Cpu& c, std::uint32_t date) {
    using namespace init;
    c.set(0, c.lr());  // mfspr r0,LR
    guest::Frame frame(c, entry, 0x370, {.first_gpr = 13});
    Gen gen(c, frame, date);
    c.put8(gen.at(0x310), 0);
    c.put32(gen.at(0x308), 0);
    c.put8(gen.at(0x304), 1);
    if (std::int32_t(c.u32(gen.count_word)) == 0) {
        // The assert macro (line 0x2194), then the exit call whose answer the original ignores.
        const auto pool = c.toc(-0x5618), message = c.toc(-0x7ba8);
        gen.sync();
        c.set({{3, pool}, {4, 0}, {5, 0}, {6, gen.at(0x94)}, {7, gen.at(0x8c)}});
        c.call_in_place(0x10739890, 0x100b8d60);
        c.call_in_place(0x10768810, 0x100b8d64);
        c.set({{5, c.r(3)}, {3, message}, {4, pool + 0xd}, {6, gen.at(0x94)}, {7, gen.at(0x8c)}, {8, 0x2194}});
        c.call_in_place(0x1077b720, 0x100b8d84);
        c.set({{4, message}, {3, pool + 0x1a}, {5, 0}});
        c.call_in_place(0x10769b60, 0x100b8d98);
        c.put8(c.toc(-0x7bac), 0);
        c.set({{4, c.toc(-0x7bac)}, {3, 0xffffffff}});
        c.call_in_place(0x1076f7d0, 0x100b8dac);
    }
    // 100b8dac: a progress message and the screen refresh every count / 100 rows (the count is read again: the
    // exit call may return).
    const auto count = std::int32_t(c.u32(gen.count_word));
    const auto high = std::int32_t((std::int64_t(0x51eb851f) * count) >> 32) >> 5;  // mulhw, srawi 5
    const auto step = high + std::int32_t(std::uint32_t(high) >> 31);
    c.put32(gen.at(0x30c), u32(step));
    c.set({{3, c.toc(-0x7bb8)}, {4, c.toc(-0x5618) + 0x895}, {5, std::uint32_t(high) >> 31}});
    gen.call(0x10727890, 0x100b8ddc);
    c.set({{3, c.toc(-0x7bb0)}, {4, c.toc(-0x7bb8)}, {5, 0}, {6, 0x64}, {7, 0xffffffff}});
    gen.call(0x10743120, 0x100b8df4);
    // 100b8df4: the two 63-slot arrays on the stack start out as terminators.
    for (std::uint32_t i = 0; i < 0x3f; ++i) {
        c.put16(gen.at(0x1b0) + i * 4, 0xff81);  // sthx
        c.put16(gen.at(0xb4) + i * 4, 0xff81);
        c.put8(gen.at(0x1b2) + i * 4, 0x81);  // stbx
        c.put8(gen.at(0xb6) + i * 4, 0x81);
    }
    // The loop's last pass leaves its scratch registers (r0 and r3-r9 as the compiler used them).
    c.set({{0, 248}, {3, gen.at(0xb6)}, {4, 0xffffff81}, {5, 0xffffff81}, {6, gen.at(0x1b2)}, {7, 248}, {8, 0xffffff81}, {9, 0x3f}});
    gen.loop();
    // 100c2500: the rescale of every person.
    gen.sync();
    guest::enter(c, 0x100c2504);
    rescale_people_attributes(c);
}
}
