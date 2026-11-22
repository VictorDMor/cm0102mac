// PLAYER_REGEN.CPP, block `regen_pending_processor`: 0x102dabb0, which turns the regeneration queue (list B) into new players and then
// gives the spare staff of the pool (list C) the abilities of a player.
//
// 1. The queue. While the first entry of list B is due (its year halfword at +6 is before the game's year, or equal with its day
//    halfword at +4 below the game's day) and the player pool (list D) is not empty, its person is regenerated (0x102e4530, the newgen
//    player) and the entry is removed (0x102df6d0). An id outside 0..the people count (the test accepts the count) reports the game's
//    assertion ("Invalid staff_id for regen: %ld", line 0x36e) and ends the routine. The bound of the loop is the queue's length when the
//    routine began.
// 2. The pool. While list C holds more than 2 entries, up to its length less 2 times, the best candidate of the nation the first entry
//    names (0x102dbf00) gets that entry's staff record (the table of 0x44-byte records at TOC -0x79f8, indexed by the entry's first word)
//    and then:
//      - a person without a player record draws three abilities (+0x28 and +0x2a below 1500 plus 1, +0x2c below 500 plus 1);
//      - a person with one: when his position byte +0xf is 20 and the staff record's signed bytes +0x30 > +0x31, the bytes swap and one
//        draw below 4 (zero) is followed by a draw below 50 plus 50 into +0x31; the database routine 0x100c3870 runs; a person with a
//        club that is not a human's (0x1014c2e0) is taken on as that club's assistant manager (+0x94, job 0xd) or coach (+0x160.. five
//        slots, job 0xf: every empty slot gets him) by the club's reputation against the player's halfword +0xa (0.5 and 0.75 of it);
//        then the abilities +0x2a, +0x28, +0x2c of the staff record come from the player's halfwords +0xa, +8, +0xc: each is
//        trunc(min(cap, factor * max(pow(role byte, 2), halfword))) (the third compares 0.75 * pow, not pow), with factor and cap
//        0.9 / 7500 for a halfword +0xa above 7500, else 0.75 / 6000 after a draw below the byte +0x63 (none when it is 0) that is
//        nonzero, 0.5 / 4500 when it is zero.
//    The entry is then removed from C (0x102deb70).
//
// Fidelity. The floating-point operations run in the original's order through guest::Fpu (the FPSCR it leaves is compared): each
// pow result is computed again where the original recomputes it (the maximum's two arms and the capped product). pow is the host's
// std::pow, as the CarbonLib bridge computes it. Constants come from the table at TOC -0x50a8: +0 the bias of a signed integer, +0x70
// the 2^52 of an unsigned one, +0x20 2.0, +0x28 0.9, +0x30 7500, +0x38 0.75, +0x48 6000, +0x58 0.5, +0x60 4500. The callee-saved
// registers r17..r31 are mirrored (`Run::held`): the original's loop counter, bounds, the person, and r20/r21 (the date's address, then
// the byte +0x63 and the draw it makes, which persist across iterations), set before every call and draw so a callee's prologue saves
// the original's. The other pooled draws are inline copies at the refill return addresses below.
//
// Not written, with the proof: the scaled form of the draw below the byte +0x63 (a signed byte: its magnitude is at most 128, below
// 0x10000, so the original's call of 0x102da630 after the scaled arithmetic cannot run).
//
// Registers. The caller (0x101198f0 in GAME.CPP) reads nothing after the call (tools/register_contract.py --after 0x102dabb0): the
// routine leaves r4..r10 and r3 as its last callee did.
#include "player_regen_detail.hpp"
#include <array>
#include <cmath>

namespace cm::aot::player_regen {
namespace {
namespace p {
constexpr std::uint32_t player = 0x14, staff = 0x1c, club = 0x24, role = 0x5e, rating = 0x63, job = 0x62;
}
namespace pl {  // the player record
constexpr std::uint32_t value_8 = 0x08, value_a = 0x0a, value_c = 0x0c, position = 0x0f;
}
namespace st {  // the staff record (0x44 bytes)
constexpr std::uint32_t ability_28 = 0x28, ability_2a = 0x2a, ability_2c = 0x2c, byte_30 = 0x30, byte_31 = 0x31;
}
namespace cl {  // the club record
constexpr std::uint32_t reputation = 0x1ec, manager = 0x94, coaches = 0x160;
}
namespace k {  // the table of doubles at TOC -0x50a8
constexpr std::uint32_t bias = 0x00, square = 0x20, high_factor = 0x28, high_cap = 0x30, weight = 0x38, mid_cap = 0x48, half = 0x58,
                        low_cap = 0x60, unsigned_bias = 0x70;
}
namespace fn {
constexpr std::uint32_t build_person = 0x100c3870, club_is_human = 0x1014c2e0, club_busy = 0x100c9440, club_free = 0x100c9bb0;
}
constexpr std::int32_t staff_table_cell = -0x79f8, message_cell = -0x7bb8, buffer_cell = -0x7ba8;

// One run of the routine: r17..r31 as the original holds them, set before every call and draw.
struct Run {
    Cpu& c;
    Pool pool;
    std::array<std::uint32_t, 32> held{};
    explicit Run(Cpu& cpu) : c(cpu), pool(cpu) {}
    void sync() { for (unsigned n = 17; n < 32; ++n) c.set(n, held[n]); }
    std::uint32_t call(std::uint32_t target, std::uint32_t back, std::initializer_list<std::uint32_t> arguments) {
        sync();
        return invoke(c, target, back, arguments);
    }
    // A routine of this module called as C++: the registers the original sets for the call (the callee, or a callee of its own, reads
    // them as leftovers: 0x102e4530's first callee reads r4), then the call with the original's return address as its link register
    // (the callee saves it).
    template <class F> auto direct(std::uint32_t back, std::initializer_list<std::pair<unsigned, std::uint32_t>> registers, F&& f) {
        sync();
        c.set(registers);
        const auto link = c.lr();
        c.set_lr(back);
        struct Restore { Cpu& c; std::uint32_t link; ~Restore() { c.set_lr(link); } } restore{c, link};
        return f();
    }
    void advance(std::uint32_t back) { sync(); pool.advance(back); }
    std::int32_t draw(std::int32_t bound, std::uint32_t back) { sync(); return pool.draw(bound, back, refill_site::random_first); }
};

// The unsigned byte as the original converts it: the high word 0x43300000 beside the byte (a double of 2^52 + byte).
double unsigned_biased(std::uint32_t value) { return std::bit_cast<double>((std::uint64_t(conversion_high) << 32) | value); }

// The abilities of one person's staff record from his player record's halfwords.
struct Abilities {
    Cpu& c;
    Fpu fp;
    std::uint32_t person, constants;
    Abilities(Cpu& cpu, std::uint32_t who) : c(cpu), fp(cpu), person(who), constants(cpu.toc(toc::constants)) {}
    double constant(std::uint32_t offset) const { return c.f64(constants + offset); }
    std::uint32_t record() const { return c.u32(person + p::player); }
    // pow(role byte, 2): the byte converted through the 2^52 bias, the table's 2.0 as the exponent.
    double square() { return std::pow(fp.fsub(unsigned_biased(c.u8(person + p::role)), constant(k::unsigned_bias)), constant(k::square)); }
    double value(std::uint32_t field) { return fp.fsub(biased(sext16(c.u16(record() + field))), constant(k::bias)); }
    // The larger of the (weighted) square and the halfword: the original compares them (fcmpo) and takes the square again, or the halfword
    // again, converting and subtracting a second time.
    double term(bool weighted) {
        auto s = square();
        if (weighted) s = fp.fmul(constant(k::weight), s);
        return s;
    }
    double larger(std::uint32_t field, bool weighted) {
        const auto first = term(weighted);
        const auto halfword = value(field);
        fp.fcmp(0, first, halfword);
        if (!(first > halfword)) return value(field);
        return term(weighted);
    }
    // trunc(min(cap, factor * larger)): the cap when the product is not below it, else the product of a second evaluation.
    std::int32_t capped(std::uint32_t field, bool weighted, std::uint32_t factor_at, std::uint32_t cap_at) {
        const auto factor = constant(factor_at), cap = constant(cap_at);
        const auto product = fp.fmul(factor, larger(field, weighted));
        fp.fcmp(0, product, cap);
        auto result = cap;
        if (product < cap) result = fp.fmul(factor, larger(field, weighted));
        return std::int32_t(std::uint32_t(fp.fctiwz(0, result)));
    }
    void derive(std::uint32_t factor_at, std::uint32_t cap_at) {
        c.put16(c.u32(person + p::staff) + st::ability_2a, std::uint32_t(capped(pl::value_a, false, factor_at, cap_at)));
        c.put16(c.u32(person + p::staff) + st::ability_28, std::uint32_t(capped(pl::value_8, false, factor_at, cap_at)));
        c.put16(c.u32(person + p::staff) + st::ability_2c, std::uint32_t(capped(pl::value_c, true, factor_at, cap_at)));
    }
};

// The club's coaches or assistant manager take the person on, by the club's reputation against the player's halfword +0xa.
void join_club_staff(Run& run, std::uint32_t person) {
    auto& c = run.c;
    Fpu fp(c);
    const auto constants = c.toc(toc::constants);
    const auto constant = [&](std::uint32_t offset) { return c.f64(constants + offset); };
    const auto threshold_test = [&](std::uint32_t factor_at) {
        const auto club = c.u32(person + p::club);
        const auto reputation = fp.fsub(biased(sext16(c.u16(club + cl::reputation))), constant(k::bias));
        const auto threshold = fp.fmul(constant(factor_at), reputation);
        const auto value = fp.fsub(biased(sext16(c.u16(c.u32(person + p::player) + pl::value_a))), constant(k::bias));
        fp.fcmp(0, value, threshold);
        return value < threshold;
    };
    const auto make_manager = [&](std::uint32_t back) {
        if (run.call(fn::club_free, back, {c.u32(person + p::club)}) == 0) {
            c.put32(c.u32(person + p::club) + cl::manager, person);
            c.put8(person + p::job, 0xd);
        }
    };
    bool second = true;  // the second test (0.75) is the one every path but the first's fall-through takes
    if (threshold_test(k::half)) {
        if (sext8(run.call(fn::club_busy, 0x102daed4, {c.u32(person + p::club)})) < 5) {
            second = false;
            for (std::int32_t slot = 0; slot < 5; ++slot) {
                const auto at = c.u32(person + p::club) + cl::coaches + std::uint32_t(slot) * 4;
                if (c.u32(at) == 0) {
                    c.put32(at, person);
                    c.put8(person + p::job, 0xf);
                }
            }
            if (c.u8(person + p::job) != 0xf) make_manager(0x102daf38);
        }
    }
    if (second && threshold_test(k::weight)) make_manager(0x102dafa8);
}

// The report of an id outside the people table: "Invalid staff_id for regen: %ld", then the game's assertion (line 0x36e).
void report_invalid_id(Run& run, const Frame& frame, std::uint32_t id) {
    auto& c = run.c;
    const auto pool = c.toc(toc::pool);
    run.sync();
    c.set({{3, c.toc(message_cell)}, {4, pool + 0xd8}, {5, id}});
    invoke(c, 0x1077b720, 0x102dac90);
    guest::report_assertion(c, {0x102daca4, pool + 0x37, pool + 0x48, pool + 0x55, frame.at(0x48), frame.at(0x40), 0x36e},
                            [&](std::uint32_t entry, std::uint32_t back) { run.sync(); invoke(c, entry, back); });
}
}

void process_regen_queue(Cpu& c, std::uint32_t regen) {
    save_link(c);
    Frame frame(c, 0x102dabb0, 0xc0, {.first_gpr = 17});
    Run run(c);
    auto& h = run.held;
    const auto date = c.toc(toc::year_date), count_cell = c.toc(toc::person_count), people_cell = c.toc(toc::people);
    h[17] = 0; h[25] = regen; h[26] = c.toc(staff_table_cell); h[27] = c.toc(toc::rng_salt); h[28] = c.toc(toc::rng_start);
    h[29] = c.toc(toc::rng_end); h[30] = c.toc(toc::rng_cursor); h[31] = c.toc(toc::constants); h[20] = date; h[21] = count_cell;
    h[22] = people_cell; h[23] = c.toc(buffer_cell); h[24] = c.toc(toc::pool);
    const auto queue = c.u32(regen + queue_list.entries);
    h[19] = queue;
    h[18] = c.u32(regen + queue_list.count);

    // 1. The due entries of the regeneration queue.
    for (; std::int32_t(h[17]) < std::int32_t(h[18]); ++h[17]) {
        c.put32(frame.at(0x38), c.u32(date));
        c.put32(frame.at(0x3c), c.u32(date + 4));
        const auto entry_year = c.u16(queue + queue_entry::rank_high), year = c.u16(frame.at(0x3a));
        bool due;
        if (year > entry_year) due = true;
        else if (year < entry_year) due = false;
        else {
            const auto day = sext16(c.u16(frame.at(0x38))), entry_day = sext16(c.u16(queue + queue_entry::rank_low));
            const auto difference = day ^ entry_day;
            due = ((std::uint32_t((difference >> 1) - (difference & day))) >> 31) != 0;
        }
        if (!due || c.u32(regen + player_pool_list.count) == 0) break;
        const auto id = c.i32(queue + queue_entry::id);
        if (id < 0 || id > c.i32(count_cell)) {
            report_invalid_id(run, frame, std::uint32_t(id));
            return;
        }
        const auto who = c.u32(people_cell) + std::uint32_t(id) * person::size;
        run.direct(0x102dad00, {{3, regen}, {4, who}}, [&] { regenerate_player(c, regen, who); });
        run.direct(0x102dad0c, {{3, regen}, {4, queue}}, [&] { pop_queue_entry(c, regen); });
    }

    // 2. The spare staff of the pool take a player's abilities.
    const auto pool_count = c.i32(regen + staff_pool_list.count);
    if (pool_count <= 2) return;
    h[18] = std::uint32_t(pool_count - 2);
    h[17] = 0;
    for (; std::int32_t(h[17]) < std::int32_t(h[18]); ++h[17]) {
        const auto first_entry = [&] { return c.u32(regen + staff_pool_list.entries); };
        const auto key = c.u32(first_entry() + 4);
        const auto person = run.direct(0x102dad40, {{3, regen}, {4, key}}, [&] { return best_candidate(c, key); });
        h[19] = person;
        if (person == 0) return;
        c.put32(person + p::staff, c.u32(c.toc(staff_table_cell)) + c.u32(first_entry()) * 0x44);
        const auto staff = [&] { return c.u32(person + p::staff); };
        if (c.u32(person + p::player) == 0) {
            c.put16(staff() + st::ability_28, std::uint32_t(run.draw(0x5dc, 0x102dbd48) + 1));
            c.put16(staff() + st::ability_2a, std::uint32_t(run.draw(0x5dc, 0x102dbdac) + 1));
            c.put16(staff() + st::ability_2c, std::uint32_t(run.draw(0x1f4, 0x102dbe10) + 1));
        } else {
            const auto player = [&] { return c.u32(person + p::player); };
            if (c.u8(player() + pl::position) == 0x14) {
                const auto low = c.u8(staff() + st::byte_30), high = c.u8(staff() + st::byte_31);
                if (sext8(low) > sext8(high)) {
                    c.put8(staff() + st::byte_31, low);
                    c.put8(staff() + st::byte_30, high);
                    run.advance(0x102dadc0);
                    if ((run.pool.sum() & 3) == 0) c.put8(staff() + st::byte_31, std::uint32_t(run.draw(0x32, 0x102dae0c) + 0x32));
                }
            }
            run.call(fn::build_person, 0x102dae58, {person});
            if (c.u32(person + p::club) != 0 && sext8(run.call(fn::club_is_human, 0x102dae78, {c.u32(person + p::club), 1, 1, 0, 0})) == 0)
                join_club_staff(run, person);
            Abilities abilities(c, person);
            if (sext16(c.u16(player() + pl::value_a)) > 0x1d4c) {
                abilities.derive(k::high_factor, k::high_cap);
            } else {
                h[20] = std::uint32_t(sext8(c.u8(person + p::rating)));
                // r21 keeps its old value through the draw's refill and takes the draw (0 when the byte is 0) after it.
                if (std::int32_t(h[20]) != 0) h[21] = std::uint32_t(run.draw(std::int32_t(h[20]), 0x102db400));
                else h[21] = 0;
                if (std::int32_t(h[21]) != 0) abilities.derive(k::weight, k::mid_cap);
                else abilities.derive(k::half, k::low_cap);
            }
        }
        const auto first_key = c.u32(first_entry());
        const auto before_removal = c.u32(regen + staff_pool_list.count);
        run.direct(0x102dbe64, {{3, regen}, {4, first_key}}, [&] { remove_staff_pool_entry(c, regen, std::int32_t(first_key)); });
        // The removal (exempt: its callers read r3 only) leaves its index counter in r7: the entry is the first, so it ends at count - 1. The caller of this
        // routine reads r7 and r8 (player_regen_volatile_read.inc), so they are set here as the original's callee leaves them.
        c.set(7, before_removal - 1);
    }
}
}
