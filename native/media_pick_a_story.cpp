// Story 0xbd0: an event of `kind` about a player and his club. The kind is first refined by draws
// against the player's state (some hand the story to 0xbd9 or change the kind), then a draw by kind
// picks a variant; the item names the persons around the player, goes to the club's listeners, is
// logged, and the player's morale (+0x4c of his standing) moves.
#include "media_pick_a.hpp"
#include "media_pick_a_detail.hpp"
#include "guest_fpu.hpp"
#include "media_pick_b.hpp"

namespace cm::aot::media {
using namespace guest;
using namespace detail;
using namespace values;
using namespace detail::pick;

namespace {
constexpr std::int32_t famous = 0x186a;  // a profile rating above this
inline std::uint32_t ext(std::int32_t value) { return std::uint32_t(byte_word(std::uint32_t(value))); }

class Story {
public:
    Story(Cpu& cpu, std::uint32_t self_, std::uint32_t player_, std::uint32_t club_, std::uint32_t kind_register,
          std::uint32_t other_, std::uint32_t third_)
        : c(cpu), frame(cpu, 0x10649fb0, 0x270, {.first_gpr = 13, .gpr_offset = -0x5c, .first_fpr = 31, .fpr_offset = -8}),
          fpu(cpu), rng(Stream::world(cpu)), program{cpu}, item{cpu, frame.at(0x110)}, counter{cpu, frame.at(0x38)},
          numbered{item, counter}, self(self_), player(player_), club(club_), other(other_), third(third_), person{cpu, player_},
          team{cpu, club_},
          spare(cpu, cpu.toc(-0x7ba0)), roll(cpu, cpu.r(14)), scratch(cpu, cpu.r(15)), score(cpu, cpu.r(16)),
          kind(cpu, kind_register), rival(cpu, cpu.r(28)), manager(cpu, cpu.r(29)), contract(cpu, cpu.r(30)),
          last(cpu, cpu.toc(-0x7b7c)) {
        keep(c, {{17, c.toc(slot::floats)}, {18, rng.seed}, {19, rng.start}, {20, rng.end}, {21, rng.cursor},
                 {22, self}, {23, player}, {24, club}, {26, other}, {27, third}});
    }

    void run() {
        if (!admit()) return;
        score_player();
        if (early_story()) return;
        refine_kind();
        if (known_story()) return;
        pick_variant();
        describe();
        if (handed_over()) return;
        name_persons();
        notify_and_log();
        settle();
    }

private:
    // ---- small views
    std::int32_t half(std::uint32_t v) const { return half_word(v); }
    std::int32_t kind_value() const { return half_word(kind); }
    Contract standing() const { return {c, contract}; }
    std::uint32_t profile() const { return person.player(); }
    std::int32_t rating() const { return person.player_record().reputation(); }
    std::int32_t age() const { return person.age(); }
    std::uint32_t persons_table() const { return c.u32(c.toc(-0x7be0)); }
    std::uint32_t rival_row() const { return persons_table() + std::uint32_t(rival) * player_size; }
    // The press table's sample, in line.
    std::int32_t draw(std::uint32_t back) const {
        rng.advance(back);
        return std::int32_t(rng.sample());
    }
    // A draw in [0, n) for the byte-sized n the compiler expanded in line (the float form for n
    // beyond 16 bits never runs): 0 without drawing when n is 0. The sample, its table cursor and
    // n stay in r4, r5 and r3.
    std::int32_t draw_below(std::int32_t n, std::uint32_t back) const {
        if (!nonzero(c, n)) return 0;
        rng.advance(back);
        const auto value = narrow_draw(rng, n);
        leave(c, {{3, std::uint32_t(n)}, {4, rng.sample()}, {5, c.u32(rng.cursor)}});
        compare(c, n);
        return value;
    }
    // 0x10718130: the distance between the world's date and a person's.
    std::int32_t distance(std::uint32_t back) const {
        return std::int32_t(program.call(0x10718130, back, {c.toc(-0x7b80), person.joined_first(), person.joined_second()}));
    }
    bool kind_in(std::initializer_list<std::int32_t> kinds) const {
        for (const auto k : kinds) if (compare(c, kind_value(), k) == 0) return true;
        return false;
    }
    // Hands the story on as a 0xbd9 request of the given kind, built at `at` in the frame.
    void hand_over(std::uint32_t at, std::uint32_t request_kind, std::uint32_t back) {
        c.put32(at + 0x10, 0xffffffffu);
        c.put32(at + 4, c.u32(club));
        c.put8(at + 0xc, 0);
        c.put8(at + 0xe, 0);
        c.put32(at, c.u32(player));
        c.put16(at + 0x14, request_kind);
        enter(c, back, {self, at, 0, c.u32(player), 0, c.u32(club)});
        post_item_bd9(c, self, at, 0);
    }

    // ---- 0x10649fb0 .. 0x1064a21c
    bool admit() {
        program.call(0x10715880, 0x1064a000, {frame.at(0x74)});
        if (!nonzero_logical(c, player)) return false;
        if (!nonzero_logical(c, profile())) return false;
        c.set(3, profile());
        if (compare(c, rating(), 0xdac) < 0) return false;
        if (!nonzero_logical(c, person.nation())) return false;
        if (!nonzero_logical(c, club)) return false;
        if (!nonzero_logical(c, team.nation())) return false;
        if (!nonzero_logical(c, team.ground())) return false;
        c.set(3, team.ground());
        if (!nonzero_logical(c, team.ground_record().location())) return false;
        if (!nonzero_logical(c, team.manager())) return false;
        item.begin(0xbd0, 0x1064a07c);
        put_person(c, item, 0, player, {0x1064a08c, 0x1064a0a0, 0x1064a0b4, 0x1064a0c8});
        // The club is nonnull here, so the original's -1 for field 4 never runs.
        compare_logical(c, club);
        item.put(4, c.u32(club), 0x1064a0e0);
        // The standing: by the player's key when he has one (r30 keeps its value when he has none).
        if (nonzero_logical(c, person.club()))
            contract = program.call(0x10082f90, 0x1064a110, {c.u32(last), player, person.club()});
        if (!nonzero_logical(c, contract)) {
            contract = program.call(0x10082f90, 0x1064a12c, {c.u32(last), player, club});
            if (!nonzero(c, std::int32_t(contract))) return false;
        }
        manager = team.manager();
        rival = program.module_call(0x106487d0, 0x1064a144, {self, club});
        return true;
    }

    // r16: how settled the player is, from his standing's flags and his bytes.
    void score_player() {
        // The standing is there (admit checked).
        if (nonzero(c, std::int32_t(standing().flags() & 0x30321))) {
            score = 0;
            if (compare(c, unsigned_byte(person.sportsmanship()), 1) == 0) score = score + 3;
            else if (compare(c, person.sportsmanship().get(), 6) < 0) score = score + 2;
            else if (compare(c, person.sportsmanship().get(), 0xf) < 0) score = score + 1;
            if (compare(c, person.ambition().get(), 0xf) > 0) score = ext(std::int32_t(score + 1));
            if (compare(c, unsigned_byte(person.trait_69()), 1) == 0) score = ext(std::int32_t(score + 2));
            if (compare(c, person.trait_69().get(), 6) < 0) score = ext(std::int32_t(score + 1));
            if (compare(c, person.professionalism().get(), 0xa) < 0) score = score + 1;
        } else {
            score = compare(c, unsigned_byte(person.sportsmanship()), 1) == 0 ? 1 : 0;
        }
    }

    // ---- 0x1064a21c .. 0x1064a894: a young player on a bad run may turn the story into another.
    // True when 0xbd9 took it over.
    bool early_story() {
        if (!(compare(c, kind_value()) == 0 || kind_in({2, 0x23, 1}))) return false;
        if (compare(c, person.sportsmanship().get(), byte_word(score) + 6) >= 0) return false;
        if (compare(c, person.loyalty().get(), 0xf) >= 0) return false;
        if (compare(c, person.trait_69().get(), 0xa) >= 0) return false;
        roll = unsigned_byte(standing().morale());
        const auto morale = byte_word(roll);
        scratch = std::uint32_t(draw_below(morale, 0x1064a2b4));
        // A draw below the negated remainder of 75 (so only a negative morale can pass).
        const auto limit = std::int32_t(0u - std::uint32_t(draw(0x1064a42c) % 75));
        if (compare(c, std::int32_t(scratch), limit) >= 0) return false;
        scratch = std::uint32_t(person.sportsmanship().get());
        roll = std::uint32_t(draw_below(std::int32_t(scratch), 0x1064a4a4));
        scratch = std::uint32_t(draw_below(byte_word(score) + 0xa, 0x1064a614));
        if (compare(c, std::int32_t(roll), std::int32_t(scratch)) >= 0) return false;
        if (draw(0x1064a7ac) % 3 == 0 && compare(c, kind_value(), 2) == 0) { kind = 0x1e; return false; }
        if (draw(0x1064a818) % 4 == 0) { kind = 0x20; return false; }
        hand_over(frame.at(0x58), 0x1e, 0x1064a890);
        return true;
    }

    // ---- 0x1064a894 .. 0x1064b2d0: refinements of the kind
    void refine_kind() {
        program.module_call(0x10658370, 0x1064a8a0, {self, player});
        if (compare(c, byte_word(c.r(3))) != 0) score = ext(std::int32_t(score + 5));
        // A young player who is seldom written about turns an ordinary story into 0x21, or into 0x22 when
        // the manager's standing is far away.
        if (compare(c, person.loyalty().get(), 0xf) < 0 && compare(c, person.trait_69().get(), 0xa) < 0 &&
            compare(c, person.sportsmanship().get(), byte_word(score) + 6) < 0) {
            scratch = std::uint32_t(person.sportsmanship().get());
            roll = std::uint32_t(draw_below(std::int32_t(scratch), 0x1064a91c));
            scratch = std::uint32_t(draw_below(byte_word(score) + 7, 0x1064aa8c));
            if (compare(c, std::int32_t(roll), std::int32_t(scratch)) < 0) {
                if (kind_in({0, 2, 0xd})) {
                    kind = 0x21;
                } else if (compare(c, byte_word(program.module_call(0x10658370, 0x1064ac38, {self, player}))) != 0) {
                    // The club has a manager (admit checked), so the original's test for one never fails.
                    const auto far = std::int32_t(program.call(0x10718130, 0x1064ac60,
                        {c.toc(-0x7b80), team.manager_record().joined_first(), team.manager_record().joined_second()}));
                    if (compare(c, far, 0x2d) > 0) kind = 0x22;
                }
            }
        }
        // An older player of low rating turns an ordinary story into 7 when the standing says he is on the move.
        if (compare(c, kind_value()) == 0 || compare(c, kind_value(), 2) == 0) {
            if (compare(c, age(), draw(0x1064aca0) % 2 + 0x12) < 0) {
                const auto chance = draw(0x1064acfc) % 7000;
                if (compare(c, rating(), chance) < 0) {
                    const auto standing = [&](std::uint32_t back, bool first) {
                        if (first) leave(c, {{6, c.u16(c.u32(rng.cursor))}});
                        return byte_word(program.call(0x1007c140, back, {c.u32(last), player, contract}));
                    };
                    if (compare(c, standing(0x1064ad54, true), 1) == 0 || compare(c, standing(0x1064ad70, false), 2) == 0 ||
                        compare(c, standing(0x1064ad8c, false), 3) == 0)
                        kind = 7;
                }
            }
        }
        // A player past a profile-dependent age may turn an ordinary story into 4.
        if (compare(c, kind_value()) == 0 || compare(c, kind_value(), 2) == 0) {
            const auto prof = profile();
            const std::int32_t threshold = c.u8(prof + 0xf) == 0x14 ? 0x22 : c.u8(prof + 0x11) == 0x14 ? 0x21 : 0x20;
            if (compare(c, age(), threshold) > 0 && draw(0x1064ae50) % 3 == 0) kind = 4;
        }
        // Standing flags 0x2000 and 8: a draw against the player's +0x67 byte turns the story into 0x21.
        if (nonzero(c, std::int32_t(standing().flags() & 0x2000)) || nonzero(c, std::int32_t(standing().flags() & 8))) {
            scratch = std::uint32_t(person.sportsmanship().get());
            roll = std::uint32_t(draw_below(std::int32_t(scratch), 0x1064aed8));
            if (compare(c, std::int32_t(roll), draw(0x1064b034) % 10 + 5) < 0) kind = 0x21;
        }
        // A big club (+0xc above 7500, reputation class 1 to 3 in +0x60) may turn a kind-2 story into 9.
        if (compare(c, kind_value(), 2) == 0 && compare(c, standing().wage(), 0x1d4c) > 0) {
            const auto classes = standing().squad_status() & 0xf;
            if (compare(c, std::int32_t(classes), 3) == 0 || compare(c, std::int32_t(classes), 1) == 0 || compare(c, std::int32_t(classes), 2) == 0) {
                scratch = ext(std::int32_t(c.u8(profile() + 0x21)));
                roll = std::uint32_t(draw_below(std::int32_t(scratch), 0x1064b0fc));
                if (compare(c, std::int32_t(roll), draw(0x1064b258) % 25) < 0) kind = 9;
            }
        }
        if (compare(c, std::int32_t(c.u8(profile() + 0xf)), 0x14) == 0 && (kind_in({0, 2, 0xd}))) kind = 0xe;
    }

    // ---- 0x1064b2d0: a story already told is dropped
    bool known_story() {
        return compare(c, byte_word(program.module_call(0x1069e700, 0x1064b2ec, {self, player, club, 0xbd0, kind, 0}))) != 0;
    }

    // ---- 0x1064b2f4 .. 0x1064be90: the variant of the story, r14, by kind (a jump table for kinds 0 to 0x25)
    void pick_variant() {
        if (compare_logical(c, std::uint32_t(kind_value()), 0x25) > 0) { roll = 0; return; }
        switch (kind_value()) {
        case 0: roll = std::uint32_t(draw(0x1064ba10) % 12); break;
        case 1:
            if (compare(c, distance(0x1064bb40), 0x16d) < 0) {
                roll = std::uint32_t(draw(0x1064bb68) % 4);
            } else if (compare(c, age(), 0x20) > 0) {
                roll = ext(draw(0x1064bbcc) % 3 + 4);
            } else {
                roll = ext(draw(0x1064bc30) % 2 + 4);
            }
            break;
        case 2: draw(0x1064bc88); roll = 0; break;
        case 3:
            if (nonzero(c, std::int32_t(c.u8(player + 0x5e))) && compare(c, age(), 0x20) < 0 && compare(c, rating(), famous) > 0)
                roll = std::uint32_t(draw(0x1064ba9c) % 5);
            else
                roll = std::uint32_t(draw(0x1064bafc) % 4 + 1);
            break;
        case 4: case 32: roll = std::uint32_t(draw(kind_value() == 4 ? 0x1064bd14 : 0x1064bcb8) % 3); break;
        case 5: roll = std::uint32_t(draw(0x1064bd70) % 2); break;
        case 7: roll = std::uint32_t(draw(0x1064b8c8) % 2); break;
        case 8: roll = std::uint32_t(draw(0x1064bdc8) % 2); break;
        case 34: roll = std::uint32_t(draw(0x1064b9c0) % 2); break;
        case 13: roll = std::uint32_t(draw(0x1064b918) % 4); break;
        case 33: roll = std::uint32_t(draw(0x1064b96c) % 4); break;
        case 9: {
            if (compare(c, std::int32_t(rival), -1) == 0) {
                roll = ext(draw(0x1064b560) % 4 + 2);
            } else {
                scratch = ext(unsigned_byte(Person{c, rival_row()}.trait_69()));
                roll = std::uint32_t(draw_below(std::int32_t(scratch), 0x1064b5e0));
                // Quirk: a low draw gives the raw remainder, the other the byte-sized one.
                if (compare(c, std::int32_t(roll), draw(0x1064b73c) % 20) < 0) roll = std::uint32_t(draw(0x1064b79c) % 2);
                else roll = ext(draw(0x1064b7ec) % 6);
            }
            if (compare(c, std::int32_t(roll), 3) == 0 && compare(c, age(), 0x15) < 0 && compare(c, rating(), famous) > 0)
                roll = roll + ext(draw(0x1064b870) % 2 + 1);
            break;
        }
        case 14:
            if (compare(c, byte_word(c.u8(profile() + 0x21)), 6) < 0) {
                if (compare(c, distance(0x1064b338), 0x16d) > 0) roll = ext(draw(0x1064b360) % 5);
                else roll = ext(draw(0x1064b3c4) % 4);
            } else if (compare(c, distance(0x1064b40c), 0x16d) > 0) {
                roll = ext(draw(0x1064b434) % 3 + 2);
            } else {
                roll = ext(draw(0x1064b498) % 2 + 2);
            }
            break;
        case 17: roll = ext(draw(0x1064b4f0) % 3); break;
        case 35:
            if (compare(c, age(), 0x17) > 0 || compare(c, rating(), famous) > 0) roll = std::uint32_t(draw(0x1064be40) % 3);
            else roll = 3;
            break;
        default: roll = 0; break;  // 6, 10, 12, 19, 37 and the unlisted kinds
        }
    }

    // ---- 0x1064be90 .. 0x1064c568: the item's description of the story
    void describe() {
        item.put(5, roll, 0x1064bea0);
        last = program.module_call(0x1069eb10, 0x1064bea8, {player});
        item.put(6, last, 0x1064bec0);
        item.put(7, person.nation_record().id(), 0x1064bed4);
        item.put(8, c.u32(team.ground_record().location() + 0x10), 0x1064beec);
        item.put(9, std::uint32_t(kind_value()), 0x1064befc);
        if (compare(c, byte_word(program.call(0x1014c2e0, 0x1064bf14, {club, 0, 1, 0, 0}))) != 0) c.set(3, 0);
        else program.module_call(0x106584d0, 0x1064bf38, {self, manager, player, kind});
        roll = c.r(3);
        item.put(0xa, roll, 0x1064bf4c);
        if (compare(c, std::int32_t(rival), -1) == 0) {
            put_no_person(item, 0xb, {0x1064bf64, 0x1064bf74, 0x1064bf84, 0x1064bf94});
        } else {
            item.put(0xb, rival, 0x1064bfa8);
            put_names(c, Run{item, 0xc}, rival_row(), {0x1064bfcc, 0x1064bff0, 0x1064c014});
        }
        // Field 0xf: whether the standing's byte +0x45 is at most 1 (the compiler's bit trick).
        const auto settled = std::int32_t(standing().missed_run().get());
        leave(c, {{6, 0}, {7, std::uint32_t(settled)}});
        item.put(0xf, settled <= 1 ? 1 : 0, 0x1064c044);
        item.put(0x10, standing().squad_status() & 0xf, 0x1064c058);
        item.put(0x11, std::uint32_t(age()), 0x1064c06c);
        scratch = program.call(0x100d8260, 0x1064c078, {player, club});
        // The player's form, as a float from 0x10303560; at most 20 (the pool's second float: bit 8 is
        // dropped from the flags when the form is below the pool's 6.5... see the compare below).
        const auto limits = c.toc(slot::doubles);
        c.set_f(1, double(c.f32(limits)));
        program.call(0x10303560, 0x1064c09c, {c.u32(c.toc(-0x7b3c)), profile(), 6, 1});
        if (nonzero(c, std::int32_t(scratch & 8))) {
            fpu.fcmp(0, c.f(1), double(c.f32(limits + 4)));
            if (c.cr(0) & 8) scratch = scratch ^ 8;
        }
        item.put(0x12, scratch, 0x1064c0c8);
    }

    // ---- 0x1064c0c8 .. 0x1064c568: some stories turn into 0x52 (bd9, or the club's 0x106891b0)
    bool handed_over() {
        const auto flags = std::uint32_t(scratch);
        c.set(3, flags & 0x200200);
        if ((flags & 0x200200) != 0x200200 || nonzero(c, std::int32_t(flags & 0x180))) return false;
        const auto x = draw(0x1064c108);
        scratch = std::uint32_t(x % 4);
        const auto reaction = byte_word(program.call(0x100d1cb0, 0x1064c140, {player, 0xc, std::uint32_t(x) >> 31}));
        const auto bound = std::int32_t(std::uint32_t(team.reputation().get() / 1250) + scratch + 8);
        if (compare(c, reaction, bound) <= 0) return false;
        if (!nonzero_logical(c, team.division())) return false;
        const auto listeners = c.u32(c.toc(-0x7b5c));
        const auto division_id = [&] { return c.u32(team.division()); };
        if (!nonzero_logical(c, c.u32(listeners + division_id() * 4))) return false;
        if (compare(c, person.loyalty().get(), 0xf) < 0 && compare(c, person.sportsmanship().get(), byte_word(score) + 5) < 0) {
            // A figure from the standing's morale: a tenth of it plus the player's two bytes.
            const auto pool = c.toc(slot::floats);
            const auto stacked = stacked_integer(c, frame.at(0x1f4), signed_word(standing().morale().get()));
            c.set_f(1, c.f64(pool + 8));
            c.put64(frame.at(0x1fc), fpu.fctiwz(0, fpu.fmul(c.f64(pool + 0x10), fpu.fsub(stacked, c.f64(pool + 8)))));
            scratch = c.u32(frame.at(0x200)) + std::uint32_t(person.sportsmanship().get());
            scratch = std::uint32_t(person.loyalty().get()) + scratch;
            score = std::uint32_t(draw_below(std::int32_t(scratch), 0x1064c240));
            if (compare(c, std::int32_t(score), draw(0x1064c39c) % 50) < 0) {
                hand_over(frame.at(0x3c), 0x52, 0x1064c41c);
                return true;
            }
        }
        const auto entry = c.u32(listeners + division_id() * 4);
        scratch = c.u32(entry + 8);
        if (!nonzero_logical(c, scratch)) return false;
        const auto limits = c.toc(slot::doubles);
        program.call(0x10457d00, 0x1064c454, {scratch, club, 3});
        fpu.fcmp(0, c.f(1), double(c.f32(limits + 8)));
        // cror eq,lt,eq
        const bool at_most = (c.cr(0) & 0xa) != 0;
        c.set_cr(0, std::uint8_t((c.cr(0) & ~2) | (at_most ? 2 : 0)));
        if (!at_most) return false;
        program.call(0x10457d00, 0x1064c478, {scratch, club, 1});
        c.set_f(31, fpu.fmul(c.f64(c.toc(slot::floats) + 0x18), c.f(1)));
        program.call(0x10457d00, 0x1064c490, {scratch, club, 0xa});
        fpu.fcmp(0, c.f(1), c.f(31));
        if (!(c.cr(0) & 8)) return false;
        if (draw(0x1064c4b8) % 4 != 0) return false;
        const auto radar = c.u32(c.toc(-0x7a6c));
        scratch = program.call(0x103b05c0, 0x1064c500, {radar, club, player, 0});
        if (!(compare(c, byte_word(scratch), -1) > 0 && compare(c, byte_word(scratch), 0xb) < 0)) return false;
        const auto kinds = program.call(0x103af570, 0x1064c530, {radar, club, 0});
        const auto flags_of = program.call(0x10111240, 0x1064c53c, {kinds, scratch, 0});
        if (!nonzero(c, std::int32_t(flags_of & 0x40))) return false;
        typed_call(c, 0x1064c564, post_item_bdf, self, club, 0x52, player, 0, 0, none);
        return true;
    }

    // ---- 0x1064c568 .. 0x1064c9cc: the persons around the story
    void name_persons() {
        put_person(c, item, 0x17, manager, {0x1064c578, 0x1064c58c, 0x1064c5a0, 0x1064c5b4});
        if (compare(c, std::int32_t(rival), -1) > 0 && nonzero_logical(c, Person{c, rival_row()}.club()))
            item.put(0x1b, c.u32(Person{c, rival_row()}.club()), 0x1064c5fc);
        else
            item.put(0x1b, none, 0x1064c610);
        // The club's +0x2c record is there (admit checked), so the original's -1 for the fourth argument never runs.
        compare_logical(c, team.nation());
        scratch = program.module_call(0x106a95c0, 0x1064c638, {c.u32(c.toc(-0x7570)), club, ext(std::int32_t(last)), c.u32(team.nation() + 0x68)});
        if (nonzero_logical(c, other) && compare(c, kind_value(), 0x13) != 0) {
            item.put(0x1c, 1, 0x1064c67c);
            program.call(0x1030d4a0, 0x1064c68c, {other, club, frame.at(0x74)});
            c.put8(counter.at, 0x1d);
            program.call(0x1030dcd0, 0x1064c6a4, {frame.at(0x74), item.at, counter.at});
            put_result(c, numbered, Fixture{c, other}, club, {0x1064c6cc, 0x1064c70c, 0x1064c728, 0x1064c768, 0x1064c784, 0});
            // The original calls first and steps the counter after (0x1064c790, 0x1064c7ac).
            const auto fame = program.call(0x100d71a0, 0x1064c790, {player, club});
            numbered.put(fame, 0x1064c7a8);
            const auto grade = std::uint32_t(half(program.call(0x10117490, 0x1064c7b0, {profile()})));
            numbered.put(grade, 0x1064c7c8);
            numbered.put(scratch, 0x1064c7e0);
            return;
        }
        item.put(0x1c, 0, 0x1064c7f4);
        const auto fame = program.call(0x100d71a0, 0x1064c800, {player, club});
        item.put(0x1d, fame, 0x1064c810);
        const auto grade = std::uint32_t(half(program.call(0x10117490, 0x1064c818, {profile()})));
        item.put(0x1e, grade, 0x1064c82c);
        item.put(0x1f, scratch, 0x1064c83c);
        const auto region = program.call(0x100db1b0, 0x1064c844, {player});
        item.put(0x20, region, 0x1064c854);
        scratch = 0;
        c.put8(counter.at, 0);
        for (; compare(c, byte_word(c.u8(counter.at)), 0x32) < 0; c.put8(counter.at, c.u8(counter.at) + 1)) {
            if (!nonzero_logical(c, c.u32(club + std::uint32_t(byte_word(c.u8(counter.at))) * 4 + 0x98))) continue;
            if (compare(c, byte_word(program.call(0x10187680, 0x1064c88c, {c.u32(spare), player})), 2) == 0) scratch = scratch + 1;
        }
        item.put(0x21, ext(std::int32_t(scratch)), 0x1064c8c8);
        spare = 0;
        if (compare(c, byte_word(program.call(0x1014c2e0, 0x1064c8e4, {club, 0, 1, 0, 0}))) == 0 &&
            nonzero_logical(c, team.manager_record().non_player()) && compare(c, byte_word(c.u8(team.manager_record().non_player() + 0x39)), 0xa) < 0)
            spare = 1;
        item.put(0x22, ext(std::int32_t(spare)), 0x1064c934);
        if (nonzero_logical(c, third)) put_person(c, item, 0x23, third, {0x1064c94c, 0x1064c960, 0x1064c974, 0x1064c988});
        else put_no_person(item, 0x23, {0x1064c99c, 0x1064c9ac, 0x1064c9bc, 0x1064c9cc});
    }

    // ---- 0x1064c9cc .. 0x1064ca70: the item goes to every listener of the player, and the story is logged
    void notify_and_log() {
        const auto listed = c.toc(-0x7b40);
        const auto list = frame.at(0x98);
        if (nonzero_logical(c, c.u32(listed))) program.call(0x103fc710, 0x1064c9f8, {c.u32(listed), player, 0x2da, list, 0x1e});
        else c.set(3, 0);
        spare = c.r(3);
        for (c.put8(counter.at, 0); compare(c, byte_word(c.u8(counter.at)), byte_word(spare)) < 0; c.put8(counter.at, c.u8(counter.at) + 1))
            item.post(c.u32(list + std::uint32_t(byte_word(c.u8(counter.at))) * 4), 0x1064ca34);
        program.module_call(0x1069e190, 0x1064ca70, {self, player, club, 0xbd0, ext(half_word(kind)), ext(byte_word(roll))});
    }

    // ---- 0x1064ca70 .. : the player's morale moves, and the club hears of it
    void settle() {
        if (compare(c, kind_value(), 0x13) != 0) {
            scratch = ext(person.professionalism().get());
            spare = std::uint32_t(draw_below(std::int32_t(scratch), 0x1064cab0));
            if (compare(c, std::int32_t(spare), draw(0x1064cc0c) % 5) < 0) program.call(0x1037fa80, 0x1064cc7c, {contract, 0x40000, 0});
            else shaken();
        } else {
            shaken();
        }
        scratch = std::uint32_t(person.professionalism().get() - 0x1e);
        compare(c, std::int32_t(scratch));
        spare = std::uint32_t(draw_below(std::int32_t(scratch), 0x1064ccb4));
        const auto morale = standing().morale();
        if (compare(c, morale.get() + byte_word(spare), 0x64) > 0) morale = 0x64;
        else if (compare(c, morale.get() + byte_word(spare), -0x64) < 0) morale = -0x64;
        else morale = std::int8_t(unsigned_byte(morale) + spare);
        if (kind_in({0x1e, 0x20})) {
            enter(c, 0x1064ce78, {self, kind, player, club, 0, none});
            weigh_story_targets(c, kind, player, club, 0, none);
        } else if (nonzero_logical(c, c.u32(club + 0x80)) && nonzero_logical(c, c.u32(c.u32(club + 0x80) + 0x1c))) {
            spare = c.u32(club + 0x80);
            const auto x = draw(0x1064cebc) % 150;
            if (compare(c, byte_word(c.u8(c.u32(spare + 0x1c) + 0x36)), x) > 0 && nonzero(c, std::int32_t(standing().flags().get()))) {
                leave(c, {{6, c.u16(c.u32(rng.cursor))}});
                program.call(0x100d9b80, 0x1064cf24, {spare, player, 1, c.u16(c.u32(rng.cursor))});
            }
        }
        if (compare(c, byte_word(program.call(0x1014c2e0, 0x1064cf3c, {club, 0, 1, 0, 0}))) == 0)
            program.module_call(0x10677d60, 0x1064cf5c, {self, player, club, kind, ext(byte_word(roll))});
    }
    // The morale check shared by both ends of the reaction draw.
    void shaken() {
        if (compare(c, standing().morale().get(), -0x46 - person.sportsmanship().get()) < 0)
            program.call(0x1037fa80, 0x1064cc7c, {contract, 0x40000, 0});
    }

    Cpu& c;
    Frame frame;
    Fpu fpu;
    Stream rng;
    Program program;
    Item item;
    Counter counter;
    Fields numbered;
    const std::uint32_t self, player, club, other, third;
    const Person person;  // the player
    const Club team;      // his club
    // r13 starts as the world's persons-by-listener table, later a flag or a count; r14 the variant;
    // r15 and r16 scratch and the player's score; r25 the kind (a halfword in a register);
    // r28 the other person's index, r29 the club's manager, r30 the player's standing, r31 the story roll.
    Kept<13> spare;
    Kept<14> roll;
    Kept<15> scratch;
    Kept<16> score;
    Kept<25> kind;
    Kept<28> rival;
    Kept<29> manager;
    Kept<30> contract;
    Kept<31> last;
};
}

void post_item_bd0(Cpu& c, std::uint32_t self, std::uint32_t player, std::uint32_t club, std::uint32_t kind, std::uint32_t other,
                   std::uint32_t third) {
    Story(c, self, player, club, kind, other, third).run();
}
}
