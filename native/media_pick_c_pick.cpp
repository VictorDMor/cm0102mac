// 0x1069f270: what the press and the people around a signing make of it. A person (r4) is linked to a club (r5):
// the pass settles his standing with a rival the press history names, then with the club's own squad, may lift the
// club's rating, and picks the best-placed member of the squad (and of a second club, r6) to post item 0xbe5 / 0xbe4
// for. The squad scans draw against each member's traits; the one-in-three draws pick which stories run.
#include "media_pick_c.hpp"
#include "media_pick_c_detail.hpp"

namespace cm::aot::media {
using namespace guest;
using namespace detail;
using namespace detail::pick_c;

namespace {
constexpr int persons_slot = -0x7be0, today_slot = -0x7b80, tiers_slot = -0x7b5c;
constexpr std::uint32_t squad_first = 0x98, squad_slots = 0x32, level_at = 0x50;
// The 0x68 words of the second club's +0x2c record that put it in the top group (seven pool slots).
constexpr int top_group[] = {-0x7598, -0x7514, -0x757c, -0x7594, -0x7574, -0x7578, -0x7650};
constexpr std::uint32_t modify_mask = 0x80000000, forget = 0x20000000;

struct PickPass {
    Cpu& c;
    const Frame& frame;
    const Stream rng;
    const Program program;
    // r22: the press (self); r23: the person; r24: his club; r25: the second club. The pass reuses a few registers for
    // unrelated values in turn, so each is one Kept with a name per role (the references below share it):
    // r13: the rival's story mark, then the squad index, the second club's level bar, and where a draw lands (draws
    // are made over r15 or r16 with their result in r13 or r14/r15/r16, as the site's register arguments say);
    // r14: whether anything happened (the best draw so far while a squad is searched); r15: the region id of the other
    // person, the best member found; r16: the rival (a person), a candidate's score and draw; r26: today's date, then the
    // search index and the pull a second-club member feels; r27: the second club's squad index; r28: the member looked
    // at; r29: the club's level; r30: its region's tier; r31: the member's standing at the club.
    std::uint32_t self, person, club, other;
    Kept<13> r13;
    Kept<13>& rival_mark = r13;
    Kept<13>& squad_index = r13;
    Kept<13>& level_bar = r13;
    Kept<13>& drawn = r13;
    Kept<14> acted;
    Kept<14>& best_draw = acted;
    Kept<15> r15;
    Kept<15>& other_region = r15;
    Kept<15>& best_member = r15;
    Kept<16> r16;
    Kept<16>& rival_person = r16;
    Kept<16>& candidate = r16;
    Kept<26> r26;
    Kept<26>& today = r26;
    Kept<26>& search_index = r26;
    Kept<26>& pull = r26;
    Kept<27> other_index;
    Kept<28> member;
    Kept<29> level;
    Kept<30> tier;
    Kept<31> standing;

    PickPass(Cpu& cpu, const Frame& f, std::uint32_t self, std::uint32_t person, std::uint32_t club, std::uint32_t other)
        : c(cpu), frame(f), rng(Stream::world(cpu)), program{cpu}, self(self), person(person), club(club), other(other),
          r13(cpu, cpu.r(13)), acted(cpu, cpu.r(14)), r15(cpu, cpu.r(15)), r16(cpu, cpu.r(16)), r26(cpu, cpu.toc(today_slot)),
          other_index(cpu, cpu.r(27)), member(cpu, cpu.r(28)), level(cpu, cpu.r(29)), tier(cpu, cpu.r(30)), standing(cpu, cpu.r(31)) {}

    Person who(std::uint32_t at) const { return {c, at}; }
    Club team(std::uint32_t at) const { return {c, at}; }
    // The person's player record (the original calls it his contract).
    std::uint32_t contract(std::uint32_t of) const { return who(of).player(); }
    std::int32_t rating(std::uint32_t of) const { return who(of).player_record().reputation(); }
    std::int32_t club_points(std::uint32_t of) const { return team(of).reputation(); }
    std::uint32_t slot_standings() const { return c.u32(c.toc(slots::standings)); }
    std::uint32_t threshold_at() const { return frame.at(level_at); }
    bool flag_test(std::uint32_t back, std::uint32_t mask) {
        return compare(c, std::int32_t(program.call(callee::flags_of, back, {standing}) & mask)) != 0;
    }
    void modify(std::uint32_t mask, std::uint32_t who, std::uint32_t back) { program.call(callee::modify, back, {standing, mask, who}); }
    void set_flag(std::uint32_t mask, std::uint32_t back) { program.call(callee::set_flag, back, {standing, mask}); }
    void clear_flag(std::uint32_t mask, std::uint32_t back) { program.call(callee::clear_flag, back, {standing, mask}); }
    void clear_bits(std::uint32_t mask, std::uint32_t back) { program.call(callee::clear_bits, back, {standing, mask}); }
    // Whether the person and `of` belong to the same region (two calls, compared as the original does).
    bool same_region(std::uint32_t of, std::uint32_t back_of, std::uint32_t back_person) {
        other_region = program.call(callee::region_of, back_of, {of});
        return compare(c, std::int32_t(program.call(callee::region_of, back_person, {person})), std::int32_t(std::uint32_t(other_region))) == 0;
    }
    // The stream's next sample reduced by a constant: the registers the compiler's sequence leaves.
    Reduced reduce(const Reciprocal& by, std::uint32_t refill_back) {
        rng.advance(refill_back);
        return reduced_by(rng.sample(), by);
    }

    // ------------------------------------------------------------------------------------------------------------
    // The rival the press history names: the first history row (the original's search loop never advances) about
    // story 0xbd0 ...
    void meet_rival() {
        acted = 0;
        std::int32_t rival_row = -1;
        const auto count = c.i32(self + 8);
        c.set(3, std::uint32_t(count));
        if (compare(c, 0, count) < 0) {
            const auto history = c.u32(self);
            c.set(4, history);
            c.set(3, c.u32(history + 0x14));
            if (compare(c, c.i32(history + 0x14), 0xbd0) == 0) {
                c.set(3, extended(c.i16(history + 0x12)));
                if (compare(c, c.i16(history + 0x12), 0x25) == 0) {
                    const auto club_id = c.u32(club);
                    c.set(5, club_id);
                    c.set(3, extended(c.i16(history + 0x12)));
                    if (compare(c, std::int32_t(club_id), c.i16(history + 0x12)) == 0) {
                        rival_mark = c.u8(history + 0x10);
                        leave(c, {{3, history}, {4, history}, {5, history}});
                        rival_row = c.i32(history + 8);
                    }
                }
            }
        }
        if (compare(c, rival_row, -1) == 0) return;
        c.set(3, c.u32(c.toc(persons_slot)));
        const auto rival = c.u32(c.toc(persons_slot)) + std::uint32_t(rival_row) * 0x70;
        rival_person = rival;
        if (!same_region(rival, 0x1069f3a0, 0x1069f3ac)) return;
        c.set(3, contract(person));
        if (compare(c, rating(person), 0x128e) <= 0) {
            leave(c, {{4, extended(std::int16_t(rating(person)))}});
            c.set(3, contract(rival));
            c.set(3, std::uint32_t(rating(person) - 0x5dc));
            if (compare(c, rating(person) - 0x5dc, rating(rival)) <= 0) return;
        }
        standing = program.call(callee::standing_of, 0x1069f3f4, {slot_standings(), rival, club});
        if (compare(c, std::int32_t(standing)) == 0) return;
        acted = 1;
        if (compare(c, byte_word(rival_mark), 1) == 0) {
            modify(0x8000000, 0, 0x1069f420);
            modify(modify_mask, person, 0x1069f434);
        } else if (compare(c, byte_word(rival_mark), 2) == 0) {
            if (flag_test(0x1069f44c, 0x800)) {
                set_flag(0x200, 0x1069f460);
                modify(modify_mask, person, 0x1069f474);
                set_flag(0x80, 0x1069f480);
            }
        } else {
            const auto draw = draw_byte<15>(c, rng, raw_byte(who(rival).sportsmanship()), 0x1069f4b8, 13);
            const auto s = reduce(divisors::by_10, 0x1069f614);
            leave(c, {{3, s.sign}, {4, s.sample}});
            if (compare(c, draw, s.rest) < 0) {
                set_flag(0x80, 0x1069f660);
                if (compare(c, draw_byte<15>(c, rng, raw_byte(who(rival).professionalism()), 0x1069f694, 13)) == 0) modify(modify_mask, person, 0x1069f7ec);
            }
        }
        if (flag_test(0x1069f7f4, 0x800)) clear_flag(0x800, 0x1069f808);
    }

    // ------------------------------------------------------------------------------------------------------------
    // The club's own squad: members who know the person's region and are unsettled, or whose standing the person
    // changes.
    void scan_squad() {
        level = program.call(callee::club_level, 0x1069f818, {club, 0, 0});
        std::uint32_t bar;
        if (compare(c, club_points(club), 0x9c4) < 0) bar = 0x14;
        else if (compare(c, club_points(club), 0x1482) < 0) bar = 0x19;
        else if (compare(c, club_points(club), 0x1c52) < 0) bar = 0x1e;
        else bar = 0x20;
        c.put8(threshold_at(), bar);
        const auto region = program.call(callee::region_of, 0x1069f874, {person});
        tier = program.call(callee::region_tier, 0x1069f880, {club, region});
        for (squad_index = 0; compare(c, std::int32_t(std::uint32_t(squad_index)), std::int32_t(squad_slots)) < 0; squad_index = std::uint32_t(squad_index) + 1)
            visit_squad_member();
    }

    void visit_squad_member() {
        const auto slot_at = club + std::uint32_t(squad_index) * 4;
        c.set(3, slot_at);
        member = c.u32(slot_at + squad_first);
        if (compare_logical(c, member) == 0) return;
        if (compare_logical(c, who(member).player()) == 0) return;
        if (compare_logical(c, member, person) == 0) return;
        const auto since = std::int32_t(program.call(callee::since, 0x1069f8c4, {today, who(member).joined_first(), who(member).joined_second()}));
        c.set(4, extended(std::int16_t(who(member).professionalism().get())));
        if (compare(c, since, who(member).professionalism().get() + 0xf) <= 0) return;
        standing = program.call(callee::standing_of, 0x1069f8ec, {slot_standings(), member, club});
        if (compare(c, std::int32_t(standing)) == 0) return;
        if (flag_test(0x1069f8fc, 0x800) && same_region(member, 0x1069f90c, 0x1069f918)) {
            acted = 1;
            clear_flag(0x800, 0x1069f930);
            modify(modify_mask, person, 0x1069f944);
            if (compare(c, draw_byte<16>(c, rng, raw_byte(who(member).professionalism()), 0x1069f978, 15)) == 0) set_flag(0x200, 0x1069fac8);
            if (compare(c, draw_byte<16>(c, rng, raw_byte(who(member).sportsmanship()), 0x1069fafc, 15)) == 0) set_flag(0x80, 0x1069fc4c);
        }
        if (flag_test(0x1069fc54, 0x400)) {
            c.set(3, std::uint32_t(byte_word(level)));
            if (compare(c, byte_word(level), byte_word(c.u8(threshold_at()))) >= 0) clear_flag(0x400, 0x1069fc7c);
        }
        if (compare(c, byte_word(tier), 2) > 0 && !flag_test(0x1069fc90, 0x400)) unsettle_member();
        free_member();
    }

    // A member the person does not know yet but who shares his region and club level: a draw (weighed by his
    // traits and the tier) may settle him with the person.
    void unsettle_member() {
        c.set(3, contract(member));
        if (compare(c, rating(member), 0x109a) <= 0) return;
        if (!same_region(member, 0x1069fcb0, 0x1069fcbc)) return;
        c.set(3, contract(person));
        if (compare(c, rating(person), 0x128e) <= 0) {
            c.set(3, contract(person));
            c.set(3, std::uint32_t(rating(person) - 0x5dc));
            if (compare(c, rating(person) - 0x5dc, club_points(club)) <= 0) return;
        }
        const auto professionalism = who(member).professionalism(), ambition = who(member).ambition();
        if (compare(c, professionalism.get(), 0xf) >= 0) return;
        if (compare(c, who(member).sportsmanship().get(), 0xf) >= 0) return;
        c.set(4, raw_byte(professionalism));
        c.set(3, std::uint32_t(std::int32_t(professionalism.get())));
        if (compare(c, professionalism.get(), byte_word(tier) + 4) >= 0) return;
        c.set(4, raw_byte(ambition));
        c.set(3, std::uint32_t(std::int32_t(ambition.get())));
        if (compare(c, ambition.get(), byte_word(tier) + 8) >= 0) return;
        const auto draw = draw_byte<16>(c, rng, raw_byte(professionalism), 0x1069fd70, 15);
        const auto s = reduce(divisors::by_6, 0x1069fecc);
        leave(c, {{3, s.shifted}, {4, s.sample}});
        if (compare(c, draw, s.rest) >= 0) return;
        acted = 1;
        if (compare(c, byte_word(program.call(callee::stance, 0x1069ff20, {slot_standings(), member, standing})), 1) != 0 &&
            compare(c, byte_word(program.call(callee::stance, 0x1069ff40, {slot_standings(), member, standing})), 2) != 0)
            set_flag(0x200, 0x1069ff70);
        else
            modify(modify_mask, person, 0x1069ff60);
    }

    // A member whose standing carries the 0x20000000 bit and who is not far above the person's rating may lose it.
    void free_member() {
        if (compare(c, std::int32_t(c.u32(standing + 0x50) & forget)) == 0) return;
        c.set(3, contract(person));
        if (compare(c, rating(person), 0x1482) <= 0) {
            c.set(3, std::uint32_t(rating(person) - 0x4e2));
            if (compare(c, rating(person) - 0x4e2, club_points(club)) <= 0) return;
        }
        acted = 1;
        if (compare(c, byte_word(level), 0x28) > 0) {
            clear_bits(forget, 0x1069ffc4);
            return;
        }
        if (compare(c, draw_byte<16>(c, rng, raw_byte(who(member).professionalism()), 0x1069fffc, 15)) != 0) clear_bits(forget, 0x106a0150);
    }

    // ------------------------------------------------------------------------------------------------------------
    // A strong, famous person lifts his club's rating by 500 (the row of the ratings table, clamped to 1..10000) and
    // the press gets item 0xbe5, kind 1.
    void lift_club() {
        c.set(3, contract(person));
        if (compare(c, c.i16(c.r(3) + 0xc), 0x186a) <= 0) return;
        c.set(3, contract(person));
        c.set(3, std::uint32_t(rating(person) + 0x2ee));
        if (compare(c, rating(person) + 0x2ee, club_points(club)) <= 0) {
            c.set(3, contract(person));
            if (compare(c, rating(person), 0x222e) <= 0) return;
        }
        move_club_rating(c, club, 8, 0x1f4);  // (the original tests the club for zero again; it cannot be)
        enter(c, 0x106a024c, {self, person, club, 1, 1, 0});
        post_item_be5(c, self, person, club, 1, 1, 0);
    }

    // The best-placed member of `squad`'s club (a draw weighed by his rating, traits and age).
    std::uint32_t best_of(std::uint32_t squad, std::uint32_t refill_back) {
        best_draw = 0;
        best_member = 0;
        for (search_index = 0; compare(c, std::int32_t(std::uint32_t(search_index)), std::int32_t(squad_slots)) < 0; search_index = std::uint32_t(search_index) + 1) {
            const auto slot_at = squad + std::uint32_t(search_index) * 4;
            c.set(3, slot_at);
            member = c.u32(slot_at + squad_first);
            if (compare_logical(c, member) == 0) continue;
            if (compare_logical(c, who(member).player()) == 0) continue;
            if (compare_logical(c, member, person) == 0) continue;
            c.set(3, contract(member));
            if (compare(c, rating(member), 0x109a) <= 0) continue;
            const std::int32_t trait_a = who(member).sportsmanship().get(), trait_b = who(member).loyalty().get();
            const auto build = who(member).age();
            leave(c, {{4, std::uint32_t(trait_a) * 0x14}, {5, contract(member)}});
            c.set(3, std::uint32_t(trait_b) * 0x14);
            // The member's score: rating and traits, less for the young.
            std::uint32_t score = std::uint32_t(rating(member)) + std::uint32_t(trait_a) * 0x14 + std::uint32_t(trait_b) * 0x14;
            if (compare(c, build, 0x13) < 0) score -= 0x5dc;
            else if (compare(c, build, 0x17) < 0) score -= 0x2ee;
            else if (compare(c, build, 0x1c) < 0) score -= 0xfa;
            else if (compare(c, build, 0x22) < 0) score += 0xfa;
            candidate = score;
            roll(c, rng, refill_back, 13, std::int32_t(score));
            candidate = std::uint32_t(drawn);
            if (compare(c, std::int32_t(std::uint32_t(candidate)), std::int32_t(std::uint32_t(best_draw))) <= 0) continue;
            best_draw = std::uint32_t(candidate);
            best_member = member;
        }
        return best_member;
    }

    // The second club's top-group test and the thresholds it sets (r13 and the byte at sp+0x50).
    void second_club_thresholds() {
        bool top = false;
        if (compare_logical(c, team(other).nation()) != 0) {
            for (const auto slot_offset : top_group) {
                c.set(4, team(other).nation_record().id());
                const auto global = c.toc(slot_offset);
                c.set(3, global);
                if (compare(c, std::int32_t(c.r(4)), c.i32(global)) == 0) {
                    top = true;
                    break;
                }
            }
        }
        if (top) {
            level_bar = 0x2d;
            c.put8(threshold_at(), 0x19);
        } else if (compare(c, club_points(other), 0x1482) < 0) {
            level_bar = 0x2d;
            c.put8(threshold_at(), 0x19);
        } else if (compare(c, club_points(other), 0x1c52) < 0) {
            level_bar = 0x2d;
            c.put8(threshold_at(), 0x1e);
        } else {
            level_bar = 0x2e;
            c.put8(threshold_at(), 0x23);
        }
    }

    // ------------------------------------------------------------------------------------------------------------
    // After the squad scan and the rating lift: when something happened to a strong person, the press looks for the
    // best-placed member of his club (item 0xbe5, kind 2).
    void pick_from_club() {
        if (compare(c, byte_word(acted)) == 0) return;
        c.set(3, contract(person));
        if (compare(c, c.i16(c.r(3) + 0xc), 0x186a) <= 0) return;
        c.set(3, contract(person));
        c.set(3, std::uint32_t(rating(person) + 0x2ee));
        if (compare(c, rating(person) + 0x2ee, club_points(club)) <= 0) return;
        if (compare(c, who(person).sportsmanship().get(), 0xa) <= 0) return;
        if (compare(c, who(person).loyalty().get(), 6) <= 0) return;
        const auto s = reduce(divisors::by_3, 0x106a02bc);
        leave(c, {{3, s.shifted}, {4, s.sample}});
        if (compare(c, s.rest) == 0) return;
        const auto best = best_of(club, 0x106a03f0);
        if (compare_logical(c, best) == 0) return;
        enter(c, 0x106a0570, {self, person, club, 2, 1, best});
        post_item_be5(c, self, person, club, 2, 1, best);
    }

    // ------------------------------------------------------------------------------------------------------------
    // The second club's squad: members whose standing the person's arrival changes.
    void judge_second_club() {
        compare_logical(c, other);
        acted = 0;
        if (other == 0) return;
        if (compare_logical(c, team(other).division()) == 0) return;
        const auto table_slot = c.toc(tiers_slot);
        const auto table = c.u32(table_slot);
        const auto entry = c.u32(table + c.u32(team(other).division()) * 4);
        leave(c, {{3, table}, {4, table_slot}});
        if (compare_logical(c, entry) == 0) return;
        level = program.call(callee::club_level, 0x106a05b8, {other, 0, 0});
        second_club_thresholds();
        const auto region = program.call(callee::region_of, 0x106a06d4, {person});
        tier = program.call(callee::region_tier, 0x106a06e0, {other, region});
        for (other_index = 0; compare(c, std::int32_t(std::uint32_t(other_index)), std::int32_t(squad_slots)) < 0; other_index = std::uint32_t(other_index) + 1)
            visit_other_member();
    }

    void bump_score(std::uint32_t amount) {
        pull = extended(std::int16_t(std::int8_t(std::uint32_t(pull) + amount)));
    }

    void visit_other_member() {
        const auto slot_at = other + std::uint32_t(other_index) * 4;
        c.set(3, slot_at);
        member = c.u32(slot_at + squad_first);
        if (compare_logical(c, member) == 0) return;
        if (compare_logical(c, who(member).player()) == 0) return;
        if (compare_logical(c, member, person) == 0) return;
        standing = program.call(callee::standing_of, 0x106a0728, {slot_standings(), member, other});
        if (compare(c, std::int32_t(standing)) == 0) return;
        pull = 0;
        if (compare(c, std::int32_t(c.u32(standing + 0x50))) != 0) bump_score(3);
        if (compare(c, std::int32_t(program.call(callee::flags_of, 0x106a0750, {standing}))) != 0) bump_score(1);
        if (compare(c, byte_word(c.u8(standing + 0x4c)), -0x19) < 0) bump_score(1);
        if (!first_story()) second_story();
        settle_member();
    }

    // The member's first story: a rumour that moves him to the person's side; true when it ran.
    bool first_story() {
        if (compare(c, byte_word(c.u8(frame.at(0xdb)))) == 0) return false;
        if (flag_test(0x106a078c, 0x200)) return false;
        if (flag_test(0x106a079c, 0x800)) return false;
        if (compare(c, std::int32_t(c.u32(standing + 0x50) & 0x80000000)) != 0) return false;
        if (compare(c, byte_word(level), 0x28) >= 0 && compare(c, byte_word(tier), 6) >= 0) return false;
        if (same_region(member, 0x106a07d0, 0x106a07dc) && compare(c, byte_word(tier), 2) >= 0) return false;
        c.set(3, contract(person));
        if (compare(c, rating(person), 0x1e46) <= 0) {
            leave(c, {{3, std::uint32_t(rating(person))}, {4, std::uint32_t(rating(person))}});
            // Both sides are halfwords: the club's points less 750, cut to 16 bits.
            if (compare(c, rating(person), half_word(std::uint32_t(club_points(club)) - 0x2ee)) <= 0) return false;
        }
        const auto s = reduce(divisors::by_6, 0x106a0840);
        const auto lhs = std::int32_t(who(member).ambition().get()) + byte_word(pull);
        leave(c, {{3, std::uint32_t(lhs)}, {4, std::uint32_t(s.rest)}, {5, s.sample}, {6, entry_of(c, rng)}});
        if (compare(c, lhs, s.rest + 10) <= 0) return false;
        if (compare(c, byte_word(c.u8(standing + 0x44)), 0x19) <= 0) return false;
        c.set(3, contract(member));
        if (compare(c, rating(member), 0x109a) <= 0) return false;
        const auto first = draw_byte<15>(c, rng, raw_byte(who(member).sportsmanship()), 0x106a08e4, 16);
        c.set(3, std::uint32_t(byte_word(pull)));
        if (compare(c, first, byte_word(pull) + 5) >= 0) return false;
        if (compare(c, draw_byte<16>(c, rng, raw_byte(who(member).professionalism()), 0x106a0a64, 15)) != 0) return false;
        if (compare(c, who(person).age(), 0x18) < 0) {
            c.set(3, contract(person));
            if (compare(c, c.i16(contract(person) + 0xc), 0x128e) < 0) {
                modify(0x40000000, person, 0x106a0bdc);
                acted = 1;
                return true;
            }
        }
        modify(forget, person, 0x106a0bf8);
        acted = 1;
        return true;
    }

    // The second story: an older person's pull on a member who is not settled with a region-mate.
    void second_story() {
        if (compare(c, who(person).age(), 0x17) <= 0) return;
        if (compare(c, byte_word(c.u8(frame.at(0xdb)))) == 0) return;
        if (compare(c, byte_word(program.call(callee::rival_check, 0x106a0c2c, {member, club, person}))) == 0) return;
        if (same_region(member, 0x106a0c3c, 0x106a0c48)) return;
        const auto draw = draw_byte<16>(c, rng, raw_byte(who(member).sportsmanship()), 0x106a0c84, 15);
        c.set(3, std::uint32_t(byte_word(pull)));
        if (compare(c, draw, byte_word(pull) + 5) >= 0) return;
        c.set(3, contract(person));
        if (compare(c, rating(person), 0x1e46) <= 0) {
            leave(c, {{3, std::uint32_t(rating(person))}, {4, std::uint32_t(rating(person))}});
            if (compare(c, rating(person), half_word(std::uint32_t(club_points(club)) - 0x2ee)) <= 0) {
                shift_score(c, standing + 0x4c, -0x19);
                return;
            }
        }
        modify(forget, person, 0x106a0e14);
        acted = 1;
    }

    // The member's mood and flags after the stories.
    void settle_member() {
        if (compare(c, who(member).age(), 0x17) > 0 && !flag_test(0x106a0e80, 0x400) &&
            compare(c, who(member).ambition().get(), 0xf) > 0) {
            const auto s = reduce(divisors::by_10, 0x106a0eb8);
            const std::int32_t trait = who(member).trait_64().get();
            leave(c, {{3, std::uint32_t(trait)}, {4, std::uint32_t(s.rest)}, {5, s.sample}, {6, entry_of(c, rng)}});
            if (compare(c, trait, byte_word(std::uint32_t(s.rest) + 0xa)) > 0 && compare(c, who(member).sportsmanship().get(), 0xa) > 0) {
                c.set(3, contract(person));
                if (compare(c, rating(person), 0x109a) > 0) {
                    c.set(3, contract(member));
                    if (compare(c, rating(member), 0x109a) > 0) {
                        c.set(3, std::uint32_t(byte_word(level)));
                        if (compare(c, byte_word(level), byte_word(c.u8(threshold_at()))) < 0) set_flag(0x400, 0x106a0f58);
                    }
                }
            }
        }
        c.set(3, std::uint32_t(byte_word(level)));
        if (compare(c, byte_word(level), byte_word(level_bar)) < 0 || compare(c, byte_word(tier), 2) < 0) {
            if (same_region(member, 0x106a0f7c, 0x106a0f88) && flag_test(0x106a0f98, 0x200)) clear_flag(0x200, 0x106a0fac);
        }
        bool settled = false;
        c.set(3, std::uint32_t(byte_word(level)));
        if (compare(c, byte_word(level), byte_word(level_bar)) < 0) {
            const auto draw = draw_byte<16>(c, rng, raw_byte(who(person).professionalism()), 0x106a0ff0, 15);
            const auto s = reduce(divisors::by_10, 0x106a114c);
            leave(c, {{3, s.sign}, {4, s.sample}});
            if (compare(c, draw, s.rest) > 0) settled = same_region(member, 0x106a1194, 0x106a11a0);
        }
        if (settled || compare(c, byte_word(tier), 2) < 0) {
            if (flag_test(0x106a11bc, 0x800)) clear_flag(0x800, 0x106a11d0);
        }
        bool shared = false;
        c.set(3, std::uint32_t(byte_word(level)));
        if (compare(c, byte_word(level), byte_word(level_bar)) < 0) shared = same_region(member, 0x106a11e8, 0x106a11f4);
        if (shared || compare(c, byte_word(tier), 2) < 0) {
            if (compare(c, std::int32_t(c.u32(standing + 0x50) & 0x80000000)) != 0) clear_bits(modify_mask, 0x106a1224);
        }
        if (flag_test(0x106a122c, 0x400)) {
            if (compare(c, std::int32_t(c.u32(standing + 0x50) & 0x80000000)) != 0) clear_bits(modify_mask, 0x106a1250);
            if (flag_test(0x106a1258, 0x800)) clear_flag(0x800, 0x106a126c);
            if (flag_test(0x106a1274, 0x200)) clear_flag(0x200, 0x106a1288);
        }
    }

    // ------------------------------------------------------------------------------------------------------------
    // Last: when something happened at the second club to a strong person, the press picks the best-placed member of it
    // (item 0xbe4).
    void pick_from_second_club() {
        if (compare(c, byte_word(acted)) == 0) return;
        // (Then the second club cannot be zero: judge_second_club clears `acted` without it.)
        compare_logical(c, other);
        if (compare(c, who(person).age(), 0x23) >= 0) return;
        c.set(3, contract(person));
        if (compare(c, c.i16(c.r(3) + 4), 0x64) <= 0) return;
        c.set(3, contract(person));
        if (compare(c, c.i16(c.r(3) + 0xc), 0x186a) <= 0) return;
        c.set(3, contract(person));
        c.set(3, std::uint32_t(rating(person) + 0x2ee));
        if (compare(c, rating(person) + 0x2ee, club_points(other)) <= 0) return;
        if (compare(c, who(person).sportsmanship().get(), 0xa) <= 0) return;
        if (compare(c, who(person).loyalty().get(), 6) <= 0) return;
        const auto s = reduce(divisors::by_3, 0x106a132c);
        leave(c, {{3, s.shifted}, {4, s.sample}});
        if (compare(c, s.rest) == 0) return;
        const auto best = best_of(other, 0x106a1460);
        if (compare_logical(c, best) == 0) return;
        enter(c, 0x106a15dc, {self, other, club, person, best});
        post_item_be4(c, self, other, club, person, best);
    }
};
}

void pick_reactions(Cpu& c, std::uint32_t self, std::uint32_t person, std::uint32_t club, std::uint32_t other, std::uint32_t flag) {
    Frame frame(c, 0x1069f270, 0xb0, {.first_gpr = 13});
    const auto rng = Stream::world(c);
    keep(c, {{17, c.toc(slot::floats)}, {18, rng.seed}, {19, rng.start}, {20, rng.end}, {21, rng.cursor}, {22, self}, {23, person}, {24, club}, {25, other}});
    PickPass pass(c, frame, self, person, club, other);
    c.put8(frame.at(0xdb), flag);
    if (compare(c, std::int32_t(person)) == 0) return;
    if (compare_logical(c, club) == 0) return;
    if (compare_logical(c, pass.who(person).player()) == 0) return;
    if (compare(c, pass.who(person).age(), 0x13) < 0) return;
    c.set(3, pass.contract(person));
    if (compare(c, pass.rating(person), 0x109a) < 0) return;
    pass.meet_rival();
    pass.scan_squad();
    pass.lift_club();
    pass.pick_from_club();
    pass.judge_second_club();
    pass.pick_from_second_club();
}
}
