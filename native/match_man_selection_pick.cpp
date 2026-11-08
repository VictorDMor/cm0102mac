// MATCH_MAN.CPP selection block, 0x10210c50 (match_man_selection.hpp): fills the places of a side from a
// list of people, then improves the picks by swapping.
//
// The original keeps its loop variables in the stack frame and in r13-r31 across calls. A callee's prologue
// saves r13-r31, its link register and condition register into this frame's image, and the game reads
// leftover stack, so each register keeps the value the original holds there (Kept) and each stack local
// is written where the original writes it (Field). Variable names say what the code does with them;
// the offsets and registers are the original's (the comments give the register of each Kept).
#include "match_man_selection.hpp"
#include "match_man_selection_detail.hpp"
#include "match_man_adapter.hpp"
#include "guest_values.hpp"

namespace cm::aot::match_man {
namespace {
using guest::compare;
using guest::compare_logical;
using guest::Field;
using guest::Frame;
using guest::Kept;
using guest::nonzero_logical;

constexpr std::int32_t s8(std::uint32_t value) { return std::int8_t(value); }
constexpr std::uint32_t reg(std::int32_t value) { return std::uint32_t(value); }

constexpr std::uint32_t entry_point = 0x10210c50;
// 0x10211e70 rates a person for a place (a later block's routine).
constexpr std::uint32_t place_rating_entry = 0x10211e70;
constexpr std::uint32_t mode_judge_bit = 0x00400000;  // mode bit 9 (counting from the high bit): look up the human manager
constexpr std::uint32_t mode_recursion_bit = 0x200;   // set by the call that places the secondary places

using Word = Field<std::uint32_t>;
using Byte = Field<std::uint8_t>;

class PlaceSelection {
public:
    PlaceSelection(Cpu& cpu, Frame& stack) : c(cpu), frame(stack) {}
    std::uint32_t run(std::uint32_t people_argument, std::uint32_t count_argument, std::uint32_t flags_argument,
                      std::uint32_t primary_argument, std::uint32_t secondary_argument, std::uint32_t chosen_argument,
                      std::uint32_t mode_argument, std::uint32_t club_argument);

private:
    std::uint32_t at(std::uint32_t offset) const { return frame.at(std::int32_t(offset)); }
    // The runtime's allocator (count, size), release and fill (pointer, byte, size).
    std::uint32_t allocate(std::uint32_t number, std::uint32_t size, std::uint32_t back);
    void release(std::uint32_t pointer, std::uint32_t back);
    void zero_fill(std::uint32_t pointer, std::uint32_t size, std::uint32_t back);
    void release_own(std::uint32_t role_back, std::uint32_t slot_back, std::uint32_t rating_back);
    std::uint32_t make_array(std::uint32_t size, std::uint32_t back);
    std::int32_t rate(std::uint32_t back, std::uint32_t person, std::uint32_t data, std::uint32_t resolved_record,
                      std::uint32_t flag_at, std::uint32_t place_at, std::uint32_t role_at, std::uint32_t rating_at);
    std::uint32_t side_record_of(std::uint32_t person);
    void resolve_people();
    void find_human_manager();
    void fill_by_rating();
    void drop_misfits();
    void assign_best_per_place();
    void resolve_duplicates();
    void place_secondary();
    void improve_by_swaps();

    Cpu& c;
    Frame& frame;
    // Every non-volatile register holds its entry value until the original sets it.
    Kept<13> r13{c, c.r(13)};  // the people count, then a pointer into the resolved records
    Kept<14> r14{c, c.r(14)};
    Kept<15> r15{c, c.r(15)};  // the used flags, a byte per person
    Kept<16> r16{c, c.r(16)};
    Kept<17> r17{c, c.r(17)};
    Kept<18> r18{c, c.r(18)};
    Kept<19> r19{c, c.r(19)};
    Kept<20> r20{c, c.r(20)};
    Kept<21> r21{c, c.r(21)};
    Kept<22> r22{c, c.r(22)};
    Kept<23> r23{c, c.r(23)};
    Kept<24> r24{c, c.r(24)};
    Kept<25> r25{c, c.r(25)};
    Kept<26> r26{c, c.r(26)};
    Kept<27> r27{c, c.r(27)};
    Kept<28> club{c, c.r(28)};
    Kept<29> side{c, c.r(29)};   // the match side, or 0
    Kept<30> roles{c, c.r(30)};  // the role byte of each place
    Kept<31> human{c, c.r(31)};  // the human manager found for the club's judge, or 0

    // The arguments as the original stores them (registers into the caller's parameter area) and the stack words
    // the caller passed.
    Word people{c, at(0x238)}, flags{c, at(0x240)}, chosen{c, at(0x24c)}, mode{c, at(0x250)};
    Byte count_byte{c, at(0x23f)}, primary_byte{c, at(0x247)}, secondary_byte{c, at(0x24b)};
    Word side_argument{c, at(0x258)}, weights{c, at(0x25c)}, slots{c, at(0x264)}, role_argument{c, at(0x268)},
        extra{c, at(0x26c)}, ratings{c, at(0x270)};
    // The locals.
    Word side_player{c, at(0x120)}, resolved{c, at(0x128)}, pass_a{c, at(0x12c)}, candidate_data{c, at(0x130)},
        best_loss{c, at(0x13c)}, best_score{c, at(0x140)}, replacement{c, at(0x144)}, pass_c{c, at(0x14c)},
        pass_e{c, at(0x150)}, total_places{c, at(0x158)};
    Byte candidate_index{c, at(0x124)}, ratings_made{c, at(0x125)}, slots_made{c, at(0x126)}, roles_made{c, at(0x127)},
        swap_candidate{c, at(0x134)}, swap_place{c, at(0x135)}, replacement_index{c, at(0x136)}, best_index{c, at(0x137)},
        duplicates{c, at(0x138)}, duplicate_index{c, at(0x148)}, place_b{c, at(0x154)}, place_a{c, at(0x155)};
    // Spills of loop bounds, pointers and scores.
    Word spill_15c{c, at(0x15c)}, spill_160{c, at(0x160)}, spill_164{c, at(0x164)}, spill_168{c, at(0x168)},
        spill_16c{c, at(0x16c)}, spill_170{c, at(0x170)}, spill_174{c, at(0x174)}, spill_178{c, at(0x178)},
        spill_17c{c, at(0x17c)}, spill_180{c, at(0x180)}, spill_184{c, at(0x184)}, spill_188{c, at(0x188)},
        spill_18c{c, at(0x18c)}, spill_190{c, at(0x190)}, spill_194{c, at(0x194)}, spill_198{c, at(0x198)},
        spill_19c{c, at(0x19c)}, spill_1a0{c, at(0x1a0)}, spill_1a4{c, at(0x1a4)}, spill_1a8{c, at(0x1a8)},
        spill_1ac{c, at(0x1ac)}, spill_1b0{c, at(0x1b0)}, spill_1b4{c, at(0x1b4)}, spill_1b8{c, at(0x1b8)},
        spill_1bc{c, at(0x1bc)}, spill_1c0{c, at(0x1c0)}, spill_1c4{c, at(0x1c4)};
};

std::uint32_t PlaceSelection::allocate(std::uint32_t number, std::uint32_t size, std::uint32_t back) {
    c.set({{3, number}, {4, size}});
    c.call(selection_calls::allocate, back);
    return c.r(3);
}
void PlaceSelection::release(std::uint32_t pointer, std::uint32_t back) {
    c.set(3, pointer);
    c.call(selection_calls::release, back);
}
void PlaceSelection::zero_fill(std::uint32_t pointer, std::uint32_t size, std::uint32_t back) {
    c.set({{3, pointer}, {4, 0}, {5, size}});
    c.call(selection_calls::fill, back);
}
// What the call allocated itself (a flag byte each), in the order the original releases it.
void PlaceSelection::release_own(std::uint32_t role_back, std::uint32_t slot_back, std::uint32_t rating_back) {
    if (compare(c, s8(roles_made)) != 0 && nonzero_logical(c, roles)) release(roles, role_back);
    if (compare(c, s8(slots_made)) != 0) {
        const std::uint32_t pointer = slots;
        if (nonzero_logical(c, pointer)) release(pointer, slot_back);
    }
    if (compare(c, s8(ratings_made)) != 0) {
        const std::uint32_t pointer = ratings;
        if (nonzero_logical(c, pointer)) release(pointer, rating_back);
    }
}
// An array for the places the caller didn't give: one entry of `size` bytes per place (primary and secondary
// together), none when there are no places (then r3 is 0).
std::uint32_t PlaceSelection::make_array(std::uint32_t size, std::uint32_t back) {
    const std::int32_t places = s8(primary_byte) + s8(secondary_byte);
    compare(c, places);
    if (places == 0) return guest::result(c, 0);
    return allocate(reg(places), size, back);
}
// The rating of `person` for one place (0x10211e70): r3-r10 and seven stack words, all the original's.
// `resolved_record` is the person's record on the side (r5), `flag_at` the place's halfword in the flags
// list, `place_at` the place's entry (two words) in the slots, `role_at` its role byte and `rating_at` its
// halfword in the ratings.
std::int32_t PlaceSelection::rate(std::uint32_t back, std::uint32_t person, std::uint32_t data, std::uint32_t resolved_record,
                                  std::uint32_t flag_at, std::uint32_t place_at, std::uint32_t role_at, std::uint32_t rating_at) {
    c.put32(at(0x38), 0xffffffffu);
    const auto first = c.u32(place_at);
    const auto second = c.u32(place_at + 4);
    c.put32(at(0x40), second);
    c.put32(at(0x3c), first);
    c.put32(at(0x44), c.u8(role_at));
    c.put32(at(0x48), extra);
    c.put32(at(0x4c), reg(c.i16(rating_at)));
    c.put32(at(0x50), human);
    c.set({{3, person}, {4, data}, {5, resolved_record}, {6, reg(c.i16(flag_at))}, {7, mode}, {8, club}, {9, side}, {10, weights}});
    match_man::call(c, place_rating_entry, back);
    return std::int32_t(c.r(3));
}
// The record of `person` among the side's 20 players (side + 0x28, 0x1c4 each, compared on +0x6c); 0 if absent
// or when there is no side.
std::uint32_t PlaceSelection::side_record_of(std::uint32_t person) {
    std::uint32_t record = c.u32(side + selection_side::players);
    std::uint32_t passed = 0;
    for (;;) {
        if (compare(c, s8(passed), match_side::player_count) >= 0) { record = 0; break; }
        const auto who = c.u32(record + match_player::person);
        compare_logical(c, who, person);
        if (who == person) break;
        ++passed;
        record += match_player::size;
    }
    c.set({{3, passed}, {5, record}});
    return record;
}

// The side's record of each person goes into the resolved list (0 when the person isn't on the side).
void PlaceSelection::resolve_people() {
    std::uint32_t out = resolved, list = people;
    std::uint32_t index = 0;
    const std::int32_t number = s8(count_byte);
    c.set({{9, 0}, {4, people}});
    while (compare(c, s8(index), number) < 0) {
        const auto person = c.u32(list);
        c.set({{8, person}, {6, 0}, {7, c.u32(side + selection_side::players)}});
        const auto record = side_record_of(person);
        c.set({{6, c.r(3)}, {7, record}});
        c.put32(out, record);
        out += 4;
        list += 4;
        ++index;
        c.set({{3, out}, {4, list}, {9, index}});
    }
    c.set(5, reg(s8(index)));  // the loop test leaves the count in r5
}

// A club whose judge is a human manager's record (an id among the last 16 people with a flag at +0x6b) asks the
// human manager module for it.
void PlaceSelection::find_human_manager() {
    compare(c, std::int32_t(mode.get() & mode_judge_bit));
    if ((mode.get() & mode_judge_bit) == 0) return;
    if (!nonzero_logical(c, club)) return;
    const auto judge = c.u32(club + selection_club::judge);
    c.set(4, judge);
    if (!nonzero_logical(c, judge)) return;
    const auto id = c.u32(judge + selection_person::id);
    const auto people_count = c.u32(c.toc(-0x7be4));
    c.set({{5, id}, {3, people_count}});
    bool is_human = false;
    if (compare(c, std::int32_t(id), std::int32_t(people_count) - 0x10) >= 0 && compare(c, std::int32_t(id), std::int32_t(people_count)) < 0 &&
        compare(c, s8(c.u8(judge + selection_person::human_flag))) != 0)
        is_human = true;
    if (compare(c, is_human ? 1 : 0) == 0) return;
    c.set(3, c.u32(c.toc(-0x7a54)));
    c.call(selection_calls::human_manager, 0x10210f1c);
    human = c.r(3);
}


// Phase one: with at least as many places as people, each person goes to the place where it rates best,
// displacing a worse occupant, until everyone is placed (or 10000 passes).
void PlaceSelection::fill_by_rating() {
    // Quirk: the best score (0x140) is not reset for each person, so a person only takes a place that rates above the
    // best any earlier person found, unless the place is the first it tries.
    r25 = 0;  // the people placed so far
    spill_18c = reg(s8(count_byte));
    spill_190 = reg(s8(primary_byte));
    pass_a = 0;
    for (;;) {
        if (compare(c, std::int32_t(spill_18c.get()), std::int32_t(spill_190.get())) >= 0) return;
        if (compare(c, s8(r25), std::int32_t(spill_18c.get())) >= 0) return;
        if (compare(c, std::int32_t(pass_a.get()), 0x2710) >= 0) return;
        spill_1b0 = people.get();
        pass_a = pass_a + 1;
        r13 = resolved.get();
        candidate_index = 0;
        spill_180 = 0;
        while (compare(c, s8(candidate_index), std::int32_t(spill_18c.get())) < 0) {
            spill_184 = reg(s8(candidate_index));
            if (compare(c, s8(c.u8(r15 + spill_184.get()))) == 0) {
                r26 = reg(-1);  // the best place so far
                r27 = c.u32(spill_1b0.get());
                if (nonzero_logical(c, r27)) {
                    const auto data = c.u32(r27 + person::player);
                    candidate_data = data;
                    if (nonzero_logical(c, data)) {
                        r23 = 0;
                        r17 = chosen.get();
                        r19 = r23;
                        r16 = ratings.get();
                        r14 = flags.get();
                        r20 = r23;
                        spill_188 = reg(s8(primary_byte));
                        r21 = r23;
                        while (compare(c, s8(r23), std::int32_t(spill_188.get())) < 0) {
                            r18 = reg(s8(r23));
                            r22 = slots.get() + r20;
                            const auto score = rate(0x10211108, r27, candidate_data.get(), c.u32(r13), r14, r22, roles + r18, r16);
                            r24 = reg(score);
                            // Better than the best so far, and better than what the place holds now (judged with
                            // this candidate's record, as the original does).
                            if (compare(c, s8(r26), -1) == 0 || compare(c, score, std::int32_t(best_score.get())) > 0) {
                                bool better = true;
                                const auto occupant = c.u32(r17);
                                if (nonzero_logical(c, occupant)) {
                                    const auto kept = rate(0x10211184, occupant, c.u32(occupant + person::player), c.u32(r13), r14, r22, roles + r18, r16);
                                    better = compare(c, kept, score) < 0;
                                }
                                if (better) {
                                    r26 = r23;
                                    best_score = r24;
                                }
                            }
                            r19 = r19 + 2;
                            r14 = r14 + 2;
                            r16 = r16 + 2;
                            r20 = r20 + 8;
                            r21 = r21 + 4;
                            r17 = r17 + 4;
                            r23 = r23 + 1;
                        }
                        if (compare(c, s8(r26), -1) > 0) {
                            const std::uint32_t place = reg(s8(r26)) << 2;
                            c.set(7, place);
                            if (nonzero_logical(c, c.u32(chosen.get() + place))) {
                                // The displaced person is free again (the loop leaves r4-r9 as below).
                                const std::int32_t number = s8(count_byte);
                                std::uint32_t listed = people.get(), offset = 0;
                                c.set({{8, 0}, {4, listed}, {5, 0}, {9, 0}});
                                for (std::uint32_t index = 0; compare(c, s8(index), number) < 0; ++index, listed += 4, offset += 4) {
                                    c.set(6, c.u32(listed));
                                    compare_logical(c, c.u32(listed), c.u32(chosen.get() + place));
                                    if (c.u32(listed) == c.u32(chosen.get() + place)) {
                                        r25 = r25 - 1;
                                        c.put8(r15 + reg(s8(index)), 0);
                                    }
                                    c.set({{8, offset + 4}, {4, listed + 4}, {9, index + 1}});
                                }
                            }
                            r25 = r25 + 1;
                            c.put32(chosen.get() + place, r27);
                            c.put8(r15 + spill_184.get(), 1);
                        }
                    }
                }
            }
            spill_180 = spill_180 + 4;
            r13 = r13 + 4;
            spill_1b0 = spill_1b0 + 4;
            candidate_index = candidate_index + 1;
        }
    }
}

// Fewer people than places: a goalkeeper place keeps only a natural goalkeeper (rating 10 or more), any other
// place gives up a natural goalkeeper (rating 20).
void PlaceSelection::drop_misfits() {
    std::uint32_t index = 0, offset = 0;
    std::uint32_t r6_flags = flags.get();
    c.set({{4, 0}, {6, r6_flags}});
    while (compare(c, s8(index), s8(r19)) < 0) {
        std::uint32_t holder = c.u32(chosen.get() + offset);
        c.set(5, holder);
        if (nonzero_logical(c, holder)) {
            const bool keeper_place = compare(c, std::int32_t(c.i16(r6_flags)) & 1) != 0;
            const auto data = c.u32(holder + person::player);
            c.set(5, data);
            bool drop;
            if (keeper_place) drop = compare(c, s8(c.u8(data + selection_data::goalkeeper)), 10) < 0;
            else drop = compare(c, c.u8(data + selection_data::goalkeeper), 0x14) == 0;
            if (drop) {
                // Everyone equal to the holder is free again (the loop leaves r7-r10 as below, r5 the list at the end).
                const std::int32_t number = s8(count_byte);
                std::uint32_t listed = people.get(), advanced = 0;
                c.set({{7, listed}, {10, 0}, {8, 0}});
                for (std::uint32_t walk = 0; compare(c, s8(walk), number) < 0; ++walk, listed += 4, advanced += 4) {
                    c.set(9, c.u32(listed));
                    compare_logical(c, c.u32(listed), c.u32(chosen.get() + offset));
                    if (c.u32(listed) == c.u32(chosen.get() + offset)) c.put8(r15 + reg(s8(walk)), 0);
                    c.set({{10, advanced + 4}, {7, listed + 4}});
                }
                c.set(5, chosen.get());
                c.put32(chosen.get() + offset, 0);
            }
        }
        offset += 4;
        r6_flags += 2;
        ++index;
        c.set({{4, index}, {6, r6_flags}});
    }
}

// With people for every primary place: each place in turn takes the person who rates best for it (the best
// score carries over from place to place, as in the original), and a person taken twice is counted.
void PlaceSelection::assign_best_per_place() {
    r23 = 0;
    r16 = chosen.get();
    r14 = ratings.get();
    r22 = r23;
    r13 = flags.get();
    while (compare(c, s8(r23), s8(r19)) < 0) {
        r24 = 0;
        r18 = people.get();
        r17 = resolved.get();
        r21 = r24;
        r26 = 0;
        while (compare(c, s8(r24), s8(r20)) < 0) {
            r25 = c.u32(r18);
            if (nonzero_logical(c, r25) && nonzero_logical(c, c.u32(r25 + person::player))) {
                const auto score = rate(0x102114a8, r25, c.u32(r25 + person::player), c.u32(r17), r13, slots.get() + r22, roles + reg(s8(r23)), r14);
                if (nonzero_logical(c, r26) == false || compare(c, score, std::int32_t(best_score.get())) > 0) {
                    best_score = reg(score);
                    r26 = r25;
                    best_index = r24;
                }
            }
            r21 = r21 + 4;
            r17 = r17 + 4;
            r18 = r18 + 4;
            r24 = r24 + 1;
        }
        if (nonzero_logical(c, r26)) {
            const std::uint32_t at_index = reg(s8(best_index));
            c.set(4, at_index);
            c.put8(r15 + at_index, c.u8(r15 + at_index) + 1);
            if (compare(c, s8(c.u8(r15 + at_index)), 1) > 0) duplicates = duplicates + 1;
        }
        c.put32(r16, r26);
        r13 = r13 + 2;
        r14 = r14 + 2;
        r22 = r22 + 8;
        r16 = r16 + 4;
        r23 = r23 + 1;
    }
}

// A person taken for more than one place keeps the place where leaving costs most; each other place takes
// the best person it can without losing more than the best replacement found so far (up to 10000 passes).
void PlaceSelection::resolve_duplicates() {
    spill_194 = reg(s8(primary_byte));
    spill_19c = reg(s8(count_byte));
    pass_c = 0;
    while (compare(c, s8(duplicates)) > 0 && compare(c, std::int32_t(pass_c.get()), 0x2710) < 0) {
        duplicate_index = 0;
        pass_c = pass_c + 1;
        spill_1b8 = people.get();
        spill_170 = duplicate_index;
        spill_1bc = resolved.get();
        swap_place = 0xff;
        swap_candidate = 0xff;
        while (compare(c, s8(duplicate_index), std::int32_t(spill_19c.get())) < 0) {
            place_b = 0;
            r18 = c.u32(spill_1b8.get());
            spill_17c = place_b;
            r26 = ratings.get();
            spill_178 = place_b;
            r27 = flags.get();
            spill_174 = place_b;
            spill_1b4 = chosen.get();
            spill_198 = reg(s8(duplicate_index));
            replacement = 0;
            while (compare(c, s8(place_b), std::int32_t(spill_194.get())) < 0 && nonzero_logical(c, r18) &&
                   compare(c, s8(c.u8(r15 + spill_198.get())), 1) > 0) {
                const auto holder = c.u32(spill_1b4.get());
                compare_logical(c, r18, holder);
                if (r18 == holder) {
                    // The person holds this place too: its rating here, then the best other person for it.
                    r20 = slots.get() + spill_174.get();
                    r22 = reg(s8(place_b));
                    const auto held = rate(0x1021166c, r18, c.u32(r18 + person::player), c.u32(spill_1bc.get()), r27, r20, roles + r22, r26);
                    r16 = 0;
                    r24 = people.get();
                    r23 = reg(held);
                    r25 = resolved.get();
                    r19 = r16;
                    r17 = r16;
                    r21 = r16;
                    r13 = reg(s8(c.u8(at(0x23f))));
                    while (compare(c, s8(r17), s8(r13)) < 0) {
                        r14 = c.u32(r24);
                        if (nonzero_logical(c, r14) && nonzero_logical(c, c.u32(r14 + person::player)) && compare_logical(c, r14, r18) != 0) {
                            const auto score = rate(0x1021170c, r14, c.u32(r14 + person::player), c.u32(r25), r27, r20, roles + r22, r26);
                            bool skip = false;
                            if (nonzero_logical(c, r16) && compare(c, score, std::int32_t(r19)) <= 0) skip = true;
                            if (!skip) {
                                const bool free_person = compare(c, score, std::int32_t(r23)) >= 0;
                                if (free_person && compare(c, s8(c.u8(r15 + reg(s8(r17))))) != 0) skip = true;
                            }
                            if (!skip) {
                                replacement_index = r17;
                                r16 = r14;
                                r19 = reg(score);
                            }
                        }
                        r21 = r21 + 4;
                        r25 = r25 + 4;
                        r24 = r24 + 4;
                        r17 = r17 + 1;
                    }
                    bool better = true;
                    if (nonzero_logical(c, replacement.get())) better = compare(c, std::int32_t(r23 - r19), std::int32_t(best_loss.get())) < 0;
                    if (better) {
                        replacement = r16;
                        best_loss = reg(std::int16_t(r23 - r19));
                        swap_place = place_b.get();
                        swap_candidate = replacement_index.get();
                    }
                }
                r27 = r27 + 2;
                r26 = r26 + 2;
                spill_17c = spill_17c + 4;
                spill_1b4 = spill_1b4 + 4;
                spill_178 = spill_178 + 2;
                spill_174 = spill_174 + 8;
                place_b = place_b + 1;
            }
            if (nonzero_logical(c, replacement.get())) {
                const std::uint32_t used_at = reg(s8(swap_candidate));
                c.set(4, used_at);
                c.put8(r15 + used_at, c.u8(r15 + used_at) + 1);
                if (compare(c, s8(c.u8(r15 + used_at)), 1) > 0) duplicates = reg(s8(duplicates + 1));
                c.put32(chosen.get() + (reg(s8(swap_place)) << 2), replacement.get());
                const std::uint32_t left_at = reg(s8(duplicate_index));
                c.set({{5, reg(s8(swap_place)) << 2}, {4, left_at}});
                c.put8(r15 + left_at, c.u8(r15 + left_at) - 1);
                duplicates = duplicates - 1;
            }
            spill_170 = spill_170 + 4;
            spill_1bc = spill_1bc + 4;
            spill_1b8 = spill_1b8 + 4;
            duplicate_index = duplicate_index + 1;
        }
    }
}

// The secondary places are filled by this routine again, from the people left unused (and with the places'
// own flags, ratings and chosen entries moved on past the primary places).
void PlaceSelection::place_secondary() {
    if (compare(c, s8(secondary_byte)) == 0) return;
    r13 = 0;
    const auto leftovers = at(0x58);
    zero_fill(leftovers, 0xc8, 0x10211900);
    const std::int32_t number = s8(count_byte);
    std::uint32_t listed = people.get();
    for (std::uint32_t index = 0; compare(c, s8(index), number) < 0; ++index, listed += 4) {
        if (compare(c, s8(c.u8(r15 + reg(s8(index))))) == 0) {
            c.put32(leftovers + (reg(s8(r13)) << 2), c.u32(listed));
            r13 = r13 + 1;
        }
    }
    // The places after the primary ones: flags, chosen and ratings are moved on past them; the slots and roles are
    // passed on unmoved, so the secondary places read the first entries (the original's choice).
    const std::int32_t primary = s8(primary_byte);
    const std::uint32_t half = reg(primary) << 1, quad = reg(primary) << 2;
    c.put32(at(0x38), side);
    c.put32(at(0x3c), weights);
    c.put32(at(0x40), 0);
    c.put32(at(0x44), slots);
    c.put32(at(0x48), roles);
    c.put32(at(0x4c), extra);
    c.put32(at(0x50), ratings.get() + half);
    c.set({{3, leftovers}, {4, r13}, {5, flags.get() + half}, {6, secondary_byte}, {7, 0}, {8, chosen.get() + quad},
           {9, mode.get() | mode_recursion_bit}, {10, club}});
    match_man::call(c, entry_point, 0x102119c4);
}

// Swaps two places' holders while that raises the total rating, for up to 100 passes.
void PlaceSelection::improve_by_swaps() {
    pass_e = 0;
    spill_1ac = reg(s8(primary_byte));
    for (;;) {
        r13 = chosen.get();
        place_a = 0;
        r23 = 0;  // a swap was made
        spill_1c0 = ratings.get();
        spill_164 = place_a;
        spill_1c4 = flags.get();
        spill_160 = place_a;
        spill_15c = place_a;
        while (compare(c, s8(place_a), std::int32_t(spill_1ac.get())) < 0 && compare(c, s8(r23)) == 0) {
            r24 = c.u32(r13);
            if (nonzero_logical(c, r24) && nonzero_logical(c, c.u32(r24 + person::player))) {
                // The holder's record on the side.
                if (nonzero_logical(c, side)) side_player = side_record_of(r24);
                else side_player = 0;
                r27 = slots.get() + spill_15c.get();
                spill_1a0 = reg(s8(place_a));
                const auto here = rate(0x10211af0, r24, c.u32(r24 + person::player), side_player.get(), spill_1c4.get(), r27,
                                       roles + spill_1a0.get(), spill_1c0.get());
                spill_1a4 = reg(here);
                c.set(4, place_a.get());  // lbz r4 (the place), left in r4 after the rating
                r22 = reg(s8(c.u8(at(0x155)) + 1));
                spill_168 = r22 << 1;
                spill_16c = r22 << 2;
                r20 = r22 << 3;
                r17 = ratings.get() + spill_168.get();
                r16 = flags.get() + spill_168.get();
                r14 = chosen.get() + spill_16c.get();
                while (compare(c, s8(r22), std::int32_t(total_places.get())) < 0 && compare(c, s8(r23)) == 0) {
                    r25 = c.u32(r14);
                    if (nonzero_logical(c, r25) && nonzero_logical(c, c.u32(r25 + person::player))) {
                        if (nonzero_logical(c, side)) r26 = side_record_of(r25);
                        else r26 = 0;
                        const auto there = rate(0x10211bf8, r25, c.u32(r25 + person::player), r26, spill_1c4.get(), r27,
                                                roles + spill_1a0.get(), spill_1c0.get());
                        r18 = reg(s8(r22));
                        r19 = reg(there);
                        if (compare(c, std::int32_t(r18), s8(primary_byte)) >= 0) {
                            // A secondary place: swap when the holder of place a rates higher there than here.
                            if (compare(c, std::int32_t(spill_1a4.get()), std::int32_t(r19)) < 0) r23 = 1;
                        } else {
                            r21 = slots.get() + r20;
                            const auto a_there = rate(0x10211c84, r24, c.u32(r24 + person::player), side_player.get(), r16, r21, roles + r18, r17);
                            spill_1a8 = reg(a_there);
                            const auto b_there = rate(0x10211ce0, r25, c.u32(r25 + person::player), r26, r16, r21, roles + r18, r17);
                            const auto together = std::int32_t(std::uint32_t(spill_1a4.get()) + std::uint32_t(b_there));  // wraps as add does
                            const auto crossed = std::int32_t(std::uint32_t(spill_1a8.get()) + std::uint32_t(r19));
                            if (compare(c, together, crossed) < 0) r23 = 1;
                        }
                        if (compare(c, s8(r23)) != 0) {
                            const auto first = c.u32(r13), second = c.u32(r14);
                            c.put32(r13, second);
                            c.put32(r14, first);
                        }
                    }
                    spill_16c = spill_16c + 4;
                    r14 = r14 + 4;
                    r16 = r16 + 2;
                    r17 = r17 + 2;
                    r20 = r20 + 8;
                    r22 = r22 + 1;
                    spill_168 = spill_168 + 2;
                }
            }
            spill_164 = spill_164 + 4;
            r13 = r13 + 4;
            spill_160 = spill_160 + 2;
            spill_1c4 = spill_1c4 + 2;
            spill_1c0 = spill_1c0 + 2;
            spill_15c = spill_15c + 8;
            place_a = place_a + 1;
        }
        const std::uint32_t passes = pass_e.get() + 1;
        const bool swapped = compare(c, s8(r23)) != 0;
        pass_e = passes;
        if (!swapped) break;
        if (compare(c, std::int32_t(passes), 100) >= 0) break;
    }
}

std::uint32_t PlaceSelection::run(std::uint32_t people_argument, std::uint32_t count_argument, std::uint32_t flags_argument,
                                  std::uint32_t primary_argument, std::uint32_t secondary_argument, std::uint32_t chosen_argument,
                                  std::uint32_t mode_argument, std::uint32_t club_argument) {
    // ---- The arguments are stored and the locals started.
    club = club_argument;
    human = 0;
    const std::uint32_t slots_given = slots.get();
    compare_logical(c, slots_given, 0);
    side = side_argument.get();
    roles = role_argument.get();
    people = people_argument;
    best_score = 0;
    best_loss = 0x2710;
    duplicates = 0;
    roles_made = 0;
    slots_made = 0;
    count_byte = count_argument;
    flags = flags_argument;
    primary_byte = primary_argument;
    secondary_byte = secondary_argument;
    chosen = chosen_argument;
    mode = mode_argument;
    ratings_made = 0;
    // Each array the caller didn't give has one entry per place; the arrays are released at the end.
    if (slots_given == 0) {
        slots = make_array(8, 0x10210cf4);
        slots_made = 1;
    }
    if (!nonzero_logical(c, roles)) {
        roles = make_array(1, 0x10210d30);
        roles_made = 1;
    }
    if (compare_logical(c, ratings.get(), 0) == 0) {
        ratings = make_array(2, 0x10210d70);
        ratings_made = 1;
    }

    // ---- No people: the places are cleared and nothing is placed.
    r13 = reg(s8(count_byte));
    if (compare(c, s8(r13), 1) < 0) {
        const std::int32_t places = s8(primary_byte) + s8(secondary_byte);
        compare(c, places);
        if (places != 0) {
            c.set({{3, chosen.get()}, {5, reg(places) << 2}, {4, 0}});
            c.call(selection_calls::fill, 0x10210dbc);
        }
        release_own(0x10210dd8, 0x10210df8, 0x10210e18);
        return guest::result(c, 0);
    }

    // ---- The side's record of each person, and the human manager of the club's judge.
    resolved = allocate(r13, 4, 0x10210e2c);
    compare_logical(c, side, 0);
    if (side != 0) resolve_people();
    find_human_manager();
    r15 = allocate(r13, 1, 0x10210f2c);
    c.set(4, reg(s8(primary_byte)));  // extsb r4: the primary count, until the fill sets r4
    const std::int32_t places = s8(primary_byte) + s8(secondary_byte);
    compare(c, places);
    total_places = reg(places);
    if (places != 0) zero_fill(chosen.get(), reg(places) << 2, 0x10210f5c);
    if (compare(c, s8(primary_byte)) == 0 && compare(c, s8(secondary_byte)) == 0) {
        release(r15, 0x10210f7c);
        release_own(0x10210f98, 0x10210fb8, 0x10210fd8);
        release(resolved.get(), 0x10210fe0);
        return guest::result(c, 0);
    }

    fill_by_rating();
    r20 = reg(s8(count_byte));
    r19 = reg(s8(primary_byte));
    if (compare(c, s8(r20), s8(r19)) < 0) {
        drop_misfits();
    } else {
        assign_best_per_place();
        resolve_duplicates();
    }
    place_secondary();
    improve_by_swaps();
    release(r15, 0x10211de8);
    release(resolved.get(), 0x10211df0);
    release_own(0x10211e0c, 0x10211e2c, 0x10211e4c);
    return guest::result(c, 1);
}
}

std::uint32_t select_places(Cpu& c, std::uint32_t people, std::uint32_t count, std::uint32_t flags, std::uint32_t primary,
                            std::uint32_t secondary, std::uint32_t chosen, std::uint32_t mode, std::uint32_t club) {
    Frame frame(c, entry_point, 0x220, {.first_gpr = 13});
    PlaceSelection selection(c, frame);
    return selection.run(people, count, flags, primary, secondary, chosen, mode, club);
}
}
