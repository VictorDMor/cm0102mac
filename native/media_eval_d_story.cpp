// 0x10677d60: the press evaluates a story about a player and his club. Four families of stories (by
// kind): 0x25 (a verdict that moves the club's two ratings and the player's traits), 0x1f (the same
// verdict spread over the club's 50 persons), 0x20 (rating changes through the club-rating routines)
// and the rest, which only post. Each ends in an item posted by 0x106620b0.
#include "media_eval_d.hpp"
#include "media_records.hpp"
#include "media_values_detail.hpp"

namespace cm::aot::media {
using namespace guest;
using namespace detail;
using records::Club;
using records::Person;

namespace {
constexpr std::uint32_t evaluate_entry = 0x10677d60;
constexpr std::uint32_t post_item = 0x106620b0;
// Yes/no questions about a person and a club (or a second person), from the club-records module.
constexpr std::uint32_t query_a = 0x100d9480, query_b = 0x100d94d0, query_c = 0x100d9520, query_d = 0x100d9580;
constexpr std::uint32_t move_morale = 0x100d3570;

// A club rating row's columns (halfwords of the 76-byte rows).
constexpr unsigned column_a = 8, column_b = 6;

// x mod 2^bits for a signed x, by the compiler's rotate sequence.
std::int32_t power_remainder(std::uint32_t x, unsigned bits, std::uint32_t sign) {
    const auto low = (x & ((1u << bits) - 1)) << (32 - bits);
    const auto t = low - sign;
    return std::int32_t(((t << bits) | (t >> (32 - bits))) + sign);
}

class Evaluation {
public:
    Evaluation(Cpu& cpu, std::uint32_t reporter_, std::uint32_t player_, std::uint32_t club_, std::uint32_t kind_, std::uint32_t side_)
        : c(cpu), frame(cpu, evaluate_entry, 0xb0, {.first_gpr = 13, .first_fpr = 30}), fpu(cpu), rng(Stream::world(cpu)), program{cpu},
          pool(cpu.toc(slot::floats)), ratings(cpu.toc(-0x79f0)), reporter(reporter_), player(player_), club(club_), kind(kind_), side(side_) {
        // r13 holds the pool of single floats until the first draw, r14 the table of standings.
        keep(c, {{13, c.toc(-0x4200)}, {14, c.toc(-0x7b5c)}, {15, reporter}, {16, player}, {17, club}, {18, kind}, {19, side}, {20, 0}, {21, 0},
                 {24, c.toc(-0x7484)}, {25, ratings}, {26, pool}, {27, rng.seed}, {28, rng.start}, {29, rng.end}, {30, rng.cursor},
                 {31, c.toc(-0x7be4)}});
        c.set_f(30, c.f64(pool + 0x70));
    }

    void run() {
        if (!admit()) return;
        prepare();
        const auto verdict = half_word(kind);
        if (compare(c, verdict, 0x25) == 0) {
            by_side_25();
        } else if (compare(c, verdict, 0x1f) == 0) {
            by_side_1f();
        } else if (compare(c, verdict, 0x20) == 0) {
            by_side_20();
        } else {
            by_side_other();
        }
    }

private:
    // ---- views
    // The player's trait bytes (-128..127) the stories read and age.
    Person me() const { return {c, player}; }
    std::int32_t ambition() const { return me().ambition().get(); }
    std::int32_t loyalty() const { return me().loyalty().get(); }
    std::int32_t professionalism() const { return me().professionalism().get(); }
    std::int32_t sportsmanship() const { return me().sportsmanship().get(); }
    std::int32_t trait_69() const { return me().trait_69().get(); }
    double pooled(std::uint32_t offset) const { return c.f64(pool + offset); }
    std::uint32_t standing() const { return c.r(22); }
    std::uint32_t manager() const { return c.r(23); }
    // (The club's +0x80 record is a person; the queries take it as the other party.)
    std::uint32_t rival() const { return c.u32(club + 0x80); }
    // The traits live as bytes; a story nudges them by one.
    static void nudge(const Field<std::int8_t>& trait, int by) { trait = std::int8_t(std::uint8_t(trait.get()) + by); }
    bool asks(std::uint32_t query, std::uint32_t back, std::uint32_t about, std::uint32_t other) const {
        return nonzero(c, byte_word(program.call(query, back, {about, other})));
    }
    void morale(std::uint32_t who, std::int32_t by, std::uint32_t back) const { program.call(move_morale, back, {who, std::uint32_t(by), 1}); }

    // ---- the table's sample, in line
    // A draw in [0, n) for a signed-byte n (the float form for n beyond 16 bits never runs), n in `bound`
    // and the result in `result`; none for n = 0.
    std::int32_t roll(unsigned bound, unsigned result, std::int32_t n, std::uint32_t back) {
        c.set(bound, std::uint32_t(n));
        return draw_narrow(c, rng, n, back, result);
    }
    // sample mod a constant. `rest_in_r3` is the schedule of the first use (rest, sign, sample in r3-r5);
    // the others leave the sign in r3 and the sample in r4.
    std::int32_t reduce(std::uint32_t back, const Reciprocal& by, bool rest_in_r3 = false) {
        rng.advance(back);
        const auto r = reduced_by(rng.sample(), by);
        if (rest_in_r3) leave(c, {{3, std::uint32_t(r.rest)}, {4, r.sign}, {5, r.sample}});
        else leave(c, {{3, r.sign}, {4, r.sample}});
        return r.rest;
    }
    std::int32_t by_twenty(std::uint32_t back) { return reduce(back, divisors::by_20); }
    std::int32_t by_twenty_five(std::uint32_t back) { return reduce(back, divisors::by_25); }
    // sample mod 4 (signed), leaving the sample's sign in r3; the caller tests it (add. -> cr0).
    std::int32_t by_four(std::uint32_t back) {
        rng.advance(back);
        const auto x = rng.sample();
        c.set(3, x >> 31);
        return power_remainder(x, 2, x >> 31);
    }
    // (The second advance of two draws the compiler overlapped happens after the first sample was reduced, with that
    // reduction's scratch registers still in r3-r6: Stream::advance with the leftovers.)
    // random(5) - random(5), as the byte a morale move takes; the first remainder waits in r15 (r6 ends
    // holding the second sample).
    std::int32_t five_apart(std::uint32_t first_back, std::uint32_t second_back) {
        rng.advance(first_back);
        const auto first = reduced_by(rng.sample(), divisors::by_5);
        const auto entry = c.u16(c.u32(rng.cursor));
        c.set(15, std::uint32_t(first.rest));
        rng.advance(second_back, {{3, first.sign}, {4, std::uint32_t(first.high)}, {5, first.sample}, {6, entry}});
        const auto second = reduced_by(rng.sample(), divisors::by_5);
        c.set(6, second.sample);
        return byte_word(std::uint32_t(first.rest - second.rest));
    }
    // random(8) - random(8), likewise (the scratch of the first sample is its sign, its low bits shifted up and itself).
    std::int32_t eight_apart(std::uint32_t first_back, std::uint32_t second_back) {
        rng.advance(first_back);
        const auto x = rng.sample(), sign = x >> 31;
        const auto first = power_remainder(x, 3, sign);
        c.set(15, std::uint32_t(first));
        rng.advance(second_back, {{3, sign}, {4, (x & 7) << 29}, {5, x}});
        const auto y = rng.sample();
        const auto second = power_remainder(y, 3, y >> 31);
        return byte_word(std::uint32_t(first - second));
    }

    // ---- club ratings
    // The club's rating row column moves by `delta`, kept within 1..10000 (see move_club_rating).
    void rate(unsigned column, std::int32_t delta, unsigned holder = 3) const { move_club_rating(c, club, column, delta, holder); }

    // ---- the posting routines of other blocks
    void change_a(std::uint32_t back, std::int32_t change) {
        enter(c, back, {reporter, club, player, kind, std::uint32_t(change), side});
        change_rating_a(c, club, player, kind, std::uint32_t(change), side);
    }
    void change_b(std::uint32_t back, std::uint32_t change) {
        enter(c, back, {reporter, club, player, kind, change, side, 0});
        change_rating_b(c, club, player, kind, change, side, 0);
    }
    // The story's own item, with the code of the reaction (0x100 ...).
    void post(std::uint32_t code, std::uint32_t back) {
        c.set_f(1, c.f(31));
        c.set({{3, reporter}, {4, club}, {5, kind}, {6, code}, {8, side}, {9, 0}, {10, player}});
        call(c, post_item, back);
    }

    // ---- start
    bool admit() {
        compare(c, std::int32_t(club));  // or. r17,r5,r5
        if (club == 0) return false;
        if (!nonzero_logical(c, player)) return false;
        c.set(23, Club{c, club}.manager());
        if (!nonzero_logical(c, manager())) return false;
        if (!nonzero_logical(c, Person{c, manager()}.non_player())) return false;
        const auto found = program.call(0x10082f20, 0x10677df8, {c.u32(c.toc(-0x7b7c)), player});
        c.set(22, found);
        if (!nonzero(c, std::int32_t(found))) return false;
        // The club's second table entry: its group, from the standings table in r14.
        const auto table = [&] { return c.u32(c.u32(club + 0x30)) << 2; };
        if (!nonzero(c, byte_word(program.call(0x100c7d00, 0x10677e08, {club})))) {
            if (!nonzero_logical(c, c.u32(club + 0x30))) return false;
            leave(c, {{3, c.u32(club + 0x30)}, {4, c.u32(c.r(14))}});
            if (!nonzero_logical(c, c.u32(c.u32(c.r(14)) + table()))) return false;
        }
        if (!nonzero(c, byte_word(program.call(0x100c7d00, 0x10677e44, {club})))) {
            leave(c, {{3, c.u32(c.u32(c.r(14)) + table())}, {4, c.u32(c.r(14))}});
            c.set(21, c.u32(c.r(3) + 8));
            if (!nonzero_logical(c, c.r(21))) return false;
        }
        return true;
    }

    // The player's age, the rival's pull on f30 and the club group's scale in f31, then the stack word
    // 0x40 that the original keeps from f31.
    void prepare() {
        if (nonzero_logical(c, player)) c.set(20, std::uint32_t(me().age()));
        if (nonzero_logical(c, rival())) {
            c.put32(frame.at(0x48), 0x43300000);  // stored before the load, as the original does
            const auto level = Person{c, rival()}.ambition().get();
            leave(c, {{3, std::uint32_t(level)}});
            const double bias = pooled(8);
            const auto top = fpu.fsub(stacked_integer(c, frame.at(0x48), signed_word(level - 0xa)), bias);
            c.set_f(1, top);
            c.set_f(30, fpu.fadd(pooled(0x38), fpu.fdiv(top, pooled(0x48))));
        }
        if (nonzero_logical(c, c.r(21))) {
            c.set({{3, c.r(21)}, {4, club}, {5, 3}});
            c.call(0x10457d00, 0x10677ee4);
        } else {
            c.set_f(1, double(c.f32(c.r(13) + 8)));
        }
        c.set_f(31, c.f(1));
        const auto scaled = fpu.fctiwz(0, fpu.fmuls(double(c.f32(c.r(13) + 0x54)), c.f(31)));
        c.put64(frame.at(0x40), scaled);
    }

    // ================= kind 0x25 =================
    void by_side_25() {
        const auto s = byte_word(side);
        if (compare(c, s, 2) == 0) {
            verdict_second();
        } else if (compare(c, s, 2) > 0) {
            if (compare(c, s, 4) < 0) verdict_third();
        } else if (compare(c, s, 1) >= 0) {
            verdict_first();
        }
    }

    // Side 1: a leader club gains rating, any other loses a little; a young player's traits may slip and
    // his morale and the club's manager take the news.
    void verdict_first() {
        if (asks(query_a, 0x10677f40, player, club)) rate(column_a, 0xc8);
        else rate(column_a, -0x32);
        if (compare(c, sportsmanship(), 0xa) < 0) {
            roll(14, 13, sportsmanship(), 0x106780c0);
            if (compare(c, std::int32_t(c.r(13)), 5) < 0) {
                if (compare(c, sportsmanship(), 1) > 0) {
                    roll(14, 13, sportsmanship(), 0x10678248);
                    if (nonzero(c, std::int32_t(c.r(13)))) nudge(me().sportsmanship(), -1);
                }
                if (compare(c, trait_69(), 1) > 0) {
                    roll(14, 13, trait_69(), 0x106783dc);
                    if (nonzero(c, std::int32_t(c.r(13)))) nudge(me().trait_69(), -1);
                }
            }
        }
        morale(player, 0xa, 0x1067853c);
        roll(14, 13, trait_69(), 0x10678570);
        if (nonzero(c, std::int32_t(c.r(13))) && asks(query_d, 0x106786c0, player, manager()))
            program.call(0x100d9e90, 0x106786d4, {player, manager()});
        const auto chance = roll(13, 14, professionalism(), 0x10678708);
        if (compare(c, chance, 0) == 0) return;
        const auto flags = program.call(0x1037fa70, 0x10678854, {standing()});
        if (compare(c, std::int32_t(flags & 0x800)) == 0) return;
        program.call(0x1037fa50, 0x10678868, {standing(), 0x800});
    }

    // Side 2: the reverse for a leader, then the player's traits and standing react to a drawn outcome.
    void verdict_second() {
        c.set(22, program.call(0x10082f20, 0x1067887c, {c.u32(c.toc(-0x7b7c)), player}));
        if (asks(query_a, 0x10678890, player, club)) rate(column_a, -0x32);
        else rate(column_a, 0xc8);
        if (compare(c, sportsmanship(), 0xa) > 0) {
            const auto limit = reduce(0x106789fc, divisors::by_20, true);
            if (compare(c, sportsmanship(), limit) > 0) {
                enter(c, 0x10678a58, {reporter, player, club, kind});
                post_item_bdb(c, player, club, kind);
                return;
            }
        }
        if (compare(c, sportsmanship(), 6) >= 0) return;
        bool settle_now = false;
        if (nonzero_logical(c, standing())) {
            const auto chance = roll(14, 13, professionalism(), 0x10678aa8);
            const auto limit = reduce(0x10678c04, divisors::by_10);
            if (compare(c, chance, limit) < 0) {
                program.call(0x1037f8a0, 0x10678c50, {standing(), 0x800});
                settle_now = true;
            }
        }
        if (!settle_now) {
            // Traits that grow by one when the draw comes up 0.
            const auto grow = [&](const Field<std::int8_t>& trait, std::uint32_t back) {
                if (compare(c, trait.get(), 0x14) >= 0) return;
                roll(14, 13, trait.get(), back);
                if (!nonzero(c, std::int32_t(c.r(13)))) nudge(trait, 1);
            };
            grow(me().professionalism(), 0x10678c98);
            grow(me().trait_64(), 0x10678e2c);
            grow(me().player_record().morale(), 0x10678fc8);
        }
        morale(player, -0xa, 0x1067912c);
        if (!nonzero(c, byte_word(program.call(0x1037f110, 0x10679134, {standing()})))) return;
        if (compare(c, loyalty(), 1) > 0) {
            const auto chance = roll(14, 13, loyalty(), 0x10679180);
            if (compare(c, chance, by_twenty_five(0x106792dc)) < 0) nudge(me().loyalty(), -1);
        }
        const auto chance = roll(14, 13, loyalty(), 0x1067935c);
        if (nonzero(c, chance)) return;
        program.call(0x100d9b80, 0x106794b0, {player, manager(), 1});
        const auto pick = roll(13, 14, professionalism(), 0x106794e4);
        if (nonzero(c, pick)) {
            program.call(0x1037fa80, 0x1067965c, {standing(), 8, 0});
        } else {
            program.call(0x1037fa80, 0x10679638, {standing(), 8, 0});
            program.call(0x1037fa80, 0x10679648, {standing(), 0x2000, 0});
        }
    }

    // Side 3: the club loses rating in both columns.
    void verdict_third() {
        rate(column_a, -0x64, 5);
        rate(column_b, -0x32, 5);
    }

    // ================= kind 0x1f: the club's 50 persons =================
    // The persons that count: not the player himself, with a profile and an index below the world's count.
    bool bystander(std::uint32_t person) {
        if (!nonzero_logical(c, person)) return false;
        if (!nonzero_logical(c, Person{c, person}.player())) return false;
        nonzero_logical(c, person);
        return compare(c, c.i32(person), c.i32(c.r(31))) < 0;
    }
    template <class Body>
    void each_person(Body&& body) {
        c.set(13, 0);
        while (compare(c, std::int32_t(c.r(13)), 0x32) < 0) {
            c.set(3, club + 4 * c.r(13));
            const auto person = c.u32(c.r(3) + 0x98);
            c.set(14, person);
            body(person);
            c.set(13, c.r(13) + 1);
        }
    }

    void by_side_1f() {
        const auto s = byte_word(side);
        if (compare(c, s, 2) == 0) persons_second();
        else if (compare(c, s, 2) > 0) persons_other();
        else if (compare(c, s, 1) >= 0) persons_first();
        else persons_other();
    }

    void persons_first() {
        each_person([&](std::uint32_t person) {
            if (compare_logical(c, person, player) == 0) {
                if (asks(query_a, 0x106797e8, player, club)) {
                    rate(column_a, 0xc8);
                    rate(column_b, 0x64);
                } else if (asks(query_b, 0x10679930, player, club)) {
                    rate(column_a, -0x64);
                    rate(column_b, -0x64);
                }
                if (asks(query_c, 0x10679a74, player, rival())) rate(column_b, 0xfa);
                else if (asks(query_d, 0x10679b24, player, rival())) rate(column_b, -0xfa);
                if (!nonzero(c, byte_word(program.call(0x1037f110, 0x10679bcc, {standing()})))) {
                    morale(player, 0xa, 0x1067a118);
                    return;
                }
                morale(player, -0xa, 0x10679be4);
                const auto first = roll(15, 14, loyalty(), 0x10679c18);
                if (compare(c, first, by_twenty_five(0x10679d74)) >= 0) return;
                if (compare(c, loyalty(), 1) > 0) {
                    const auto again = roll(15, 14, loyalty(), 0x10679df8);
                    if (!nonzero(c, again)) nudge(me().loyalty(), -1);
                }
                program.call(0x100d9b80, 0x10679f58, {player, manager(), 1});
                const auto pick = roll(15, 14, professionalism(), 0x10679f8c);
                if (nonzero(c, pick)) {
                    program.call(0x1037fa80, 0x1067a104, {standing(), 8, 0});
                } else {
                    program.call(0x1037fa80, 0x1067a0e0, {standing(), 8, 0});
                    program.call(0x1037fa80, 0x1067a0f0, {standing(), 0x2000, 0});
                }
                return;
            }
            if (!bystander(person)) return;
            if (asks(query_c, 0x1067a154, player, person)) {
                morale(person, 0xa, 0x1067a16c);
            } else if (asks(query_d, 0x1067a17c, player, person)) {
                morale(person, -5, 0x1067a194);
            } else {
                const auto apart = five_apart(0x1067a1b8, 0x1067a210);
                program.call(move_morale, 0x1067a25c, {person, std::uint32_t(apart), 1});
            }
        });
    }

    void persons_second() {
        each_person([&](std::uint32_t person) {
            if (compare_logical(c, person, player) == 0) {
                if (asks(query_a, 0x1067a294, player, club)) {
                    rate(column_a, -0x1f4);
                    rate(column_b, -0xc8);
                } else if (asks(query_b, 0x1067a3dc, player, club)) {
                    rate(column_a, 0x1f4);
                    rate(column_b, 0xc8);
                }
                if (asks(query_c, 0x1067a520, player, rival())) rate(column_b, -0x1f4);
                else if (asks(query_d, 0x1067a5d0, player, rival())) rate(column_b, 0xc8);
                if (nonzero(c, byte_word(program.call(0x1037f110, 0x1067a678, {standing()})))) {
                    morale(player, 0xa, 0x1067aa18);
                    return;
                }
                morale(player, -0xa, 0x1067a690);
                const auto first = roll(15, 14, loyalty(), 0x1067a6c4);
                if (compare(c, first, by_twenty_five(0x1067a820)) >= 0) return;
                if (compare(c, loyalty(), 1) > 0) {
                    const auto again = roll(15, 14, loyalty(), 0x1067a8a4);
                    if (!nonzero(c, again)) nudge(me().loyalty(), -1);
                }
                program.call(0x100d9b80, 0x1067aa04, {player, manager(), 1});
                return;
            }
            if (!bystander(person)) return;
            if (asks(query_c, 0x1067aa54, player, person)) {
                // Half of his +0x67 trait (rounded toward zero), less 15.
                const auto half = (Person{c, person}.sportsmanship().get() + std::int32_t(std::uint32_t(Person{c, person}.sportsmanship().get()) >> 31)) >> 1;
                morale(person, byte_word(std::uint32_t(half - 0xf)), 0x1067aa84);
                return;
            }
            if (asks(query_d, 0x1067aa94, player, person)) {
                morale(person, 0xa, 0x1067aaac);
                return;
            }
            const auto first = roll(18, 15, Person{c, person}.ambition().get(), 0x1067aae4);
            if (compare(c, first, by_twenty(0x1067ac40)) <= 0) return;
            const auto second = roll(18, 15, Person{c, person}.sportsmanship().get(), 0x1067acb4);
            if (compare(c, second, by_twenty(0x1067ae10)) >= 0) return;
            morale(person, -5, 0x1067ae60);
        });
    }

    void persons_other() {
        each_person([&](std::uint32_t person) {
            if (compare_logical(c, person, player) == 0) {
                const auto apart = eight_apart(0x1067aeac, 0x1067aef8);
                program.call(move_morale, 0x1067af38, {person, std::uint32_t(apart), 1});
                return;
            }
            if (!bystander(person)) return;
            const auto apart = five_apart(0x1067af88, 0x1067afe0);
            program.call(move_morale, 0x1067b02c, {person, std::uint32_t(apart), 1});
        });
    }

    // ================= kind 0x20: the club-rating routines =================
    void by_side_20() {
        const auto s = byte_word(side);
        if (compare(c, s, 2) == 0) {
            ratings_second();
        } else if (compare(c, s, 2) < 0 && compare(c, s, 1) >= 0) {
            ratings_first();
        }
    }
    void ratings_first() {
        if (asks(query_a, 0x1067b070, player, club)) change_a(0x1067b094, 0x1f4);
        else if (asks(query_b, 0x1067b0a4, player, club)) change_a(0x1067b0c8, -0x5dc);
        else change_a(0x1067b0e8, -0x1f4);
        const auto scaled = [&](std::uint32_t figure_at, std::uint32_t site, std::uint32_t back) {
            const auto f1 = pooled(figure_at);
            c.set_f(1, f1);
            const auto bits = fpu.fctiwz(0, fpu.multiply_add(site, f1, c.f(30), pooled(0xf0)));
            c.put64(frame.at(0x38), bits);
            change_b(back, c.u32(frame.at(0x3c)));
        };
        if (asks(query_c, 0x1067b0f4, player, rival())) scaled(0x50, 0x1067b114, 0x1067b130);
        else if (asks(query_d, 0x1067b140, player, rival())) scaled(0x58, 0x1067b160, 0x1067b17c);
        else change_b(0x1067b1a0, std::uint32_t(-0x7d0));
        post(0x100, 0x1067b1c4);
    }
    void ratings_second() {
        if (asks(query_b, 0x1067b1d4, player, club)) change_a(0x1067b1f8, 0xbb8);
        if (asks(query_d, 0x1067b204, player, rival())) {
            const auto bits = fpu.fctiwz(0, fpu.fmul(pooled(0xf8), c.f(30)));
            c.put64(frame.at(0x38), bits);
            change_b(0x1067b23c, c.u32(frame.at(0x3c)));
        }
        post(0x80, 0x1067b260);
    }

    // ================= every other kind =================
    void by_side_other() {
        const auto s = byte_word(side);
        if (compare(c, s, 2) == 0) {
            by_kind();
        } else if (compare(c, s, 2) > 0) {
            if (compare(c, s, 4) < 0) post(0x404, 0x1067b8f8);
        } else if (compare(c, s, 1) >= 0) {
            post(0x500, 0x1067b2b0);
        }
    }
    // The story kinds 0..0x23 by the game's jump table: one of two reactions, or just the posting.
    void by_kind() {
        const auto code = half_word(kind);
        std::uint32_t body = 0x1067b8ac;
        if (compare_logical(c, std::uint32_t(code), 0x23) <= 0) body = c.u32(c.toc(-0x4234) + std::uint32_t(code) * 4);
        if (body == 0x1067b2d4) reaction(false);
        else if (body == 0x1067b5c4) reaction(true);
        post(0x480, 0x1067b8d0);
    }
    // A player of the right age and standing may draw the club's attention (1070c8f0): `known` is the
    // variant for a player already at his peak (profile figure 0xc at most 0xea6 only with a flag of 0x64).
    // Either way it posts afterwards.
    void reaction(bool known) {
        if (!nonzero_logical(c, player)) return;
        if (known && !nonzero_logical(c, standing())) return;
        if (!nonzero_logical(c, me().player())) return;
        const auto profile = [&] { return me().player().get(); };
        c.set(3, profile());
        if (compare(c, c.i16(profile() + 0xa), 0x1a5e) <= 0) return;
        c.set(3, profile());
        if (compare(c, c.i16(profile() + 0xc), 0xea6) <= 0) {
            if (!known) return;
            if (compare(c, c.u8(standing() + 0x44), 0x64) != 0) return;
        }
        c.set(3, profile());
        if (compare(c, c.i16(profile() + 4), 0x78) <= 0) return;
        const auto age = byte_word(c.r(20));
        if (compare(c, age, 0x13) <= 0) return;
        if (compare(c, byte_word(c.r(20)), 0x22) >= 0) return;
        if (compare(c, ambition(), 0xa) <= 0) return;
        if (known) {
            if (compare(c, sportsmanship(), 0xa) >= 0) return;
            if (compare(c, loyalty(), 6) >= 0) return;
            if (compare(c, trait_69(), 0xa) >= 0) return;
        } else {
            if (compare(c, sportsmanship(), 6) >= 0) return;
            if (compare(c, professionalism(), 0xa) <= 0) return;
            if (compare(c, loyalty(), 6) >= 0 && compare(c, trait_69(), 6) >= 0) return;
        }
        if (!known && !nonzero_logical(c, standing())) return;
        const auto flags = program.call(0x1037fa70, known ? 0x1067b67c : 0x1067b390, {standing()});
        if (compare(c, std::int32_t(flags)) == 0 && compare(c, c.i32(standing() + 0x50)) == 0) return;
        if (compare(c, byte_word(c.u8(standing() + 0x4c)), -0x19) >= 0) return;
        if (!nonzero(c, byte_word(program.call(0x1037f110, known ? 0x1067b6a8 : 0x1067b3bc, {standing()})))) return;
        const auto chance = roll(14, 13, loyalty(), known ? 0x1067b6e4 : 0x1067b3f8);
        if (nonzero(c, chance)) return;
        const auto rest = by_four(known ? 0x1067b848 : 0x1067b55c);
        const bool zero = !nonzero(c, rest);
        const auto back = known ? (zero ? 0x1067b8ac : 0x1067b890) : (zero ? 0x1067b5c0 : 0x1067b5a4);
        program.call(0x1070c8f0, back, {c.u32(c.r(24)), player, club, 4, zero ? 1u : 0u});
    }

    Cpu& c;
    Frame frame;
    Fpu fpu;
    Stream rng;
    Program program;
    std::uint32_t pool, ratings;
    std::uint32_t reporter, player, club, kind, side;
};
}

void evaluate_story(Cpu& c, std::uint32_t reporter, std::uint32_t player, std::uint32_t club, std::uint32_t kind, std::uint32_t side) {
    Evaluation(c, reporter, player, club, kind, side).run();
}
}
