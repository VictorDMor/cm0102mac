#include "shortlist_manager_detail.hpp"
#include "shortlist_manager_random.hpp"

// The category lists: their rebuild from every person (reset_storage), the
// random range draw it and the rest of the module use (sample_range), the
// ratings the lists are sorted by (rate_categories), and the manager's
// destruction (destroy_storage, which frees them as reset_storage does).
namespace cm::aot::shortlist_manager {
using namespace detail;
using namespace storage;
namespace {
namespace routine {
constexpr std::uint32_t fatal_exit = 0x1076f7d0;
constexpr std::uint32_t eligible = 0x10365d10;          // (manager, person, category): int8, the policy group's
constexpr std::uint32_t primary_contract = 0x10082f20;  // (contracts, person): the person's contract (club index at +4)
constexpr std::uint32_t club_contract = 0x10082f90;     // (contracts, person, club): the contract with that club
constexpr std::uint32_t expiring = 0x10380710;          // (contract): int8, the contract's expiry band (inferred)
constexpr std::uint32_t human_club = 0x1014c2e0;        // (club, 0, 1, 0, 0): int8, the club is in a human's hands
constexpr std::uint32_t person_test = 0x100d3040;       // (person, 0): int8 (meaning not established)
}
constexpr std::int32_t nations_slot = -0x7b98;       // the nations (Nation, 0x128 bytes each), the TOC word a pointer to them
constexpr std::int32_t nation_count_slot = -0x7b04;  // int32*: the number of nations
constexpr std::int32_t contract_scratch = -0x7b78;   // the last primary contract looked up (the game's shared scratch)
constexpr std::uint32_t person_size = Person::size, club_size = Club::size;

// sample_range inlines its recursion four levels deep in one frame: each
// level keeps its range and draw in registers of its own and its
// temporaries in the frame's slots (the remainder's always at +0x40); the
// fourth calls sample_range for its refinement. Quirk: range * 10 wraps at
// 32 bits, so every refinement range is within +-32768 and only the first
// level can be wide; the deeper levels' wide paths and the recursive call
// are unreachable (kept as the original has them, never run).
struct Level {
    unsigned range, part;
    std::int32_t range_at, result_at;
    std::uint32_t rand_back, division;  // after the rand call; the narrow path's divw
};
constexpr Level levels[] = {{30, 31, 0x48, 0x38, 0x10356e94, 0x1035738c},
                            {22, 24, 0x38, 0x48, 0x10356f80, 0x10357328},
                            {21, 23, 0x38, 0x48, 0x1035706c, 0x103572c8},
                            {20, 25, 0x38, 0x48, 0x10357158, 0x10357268}};

std::int32_t draw_level(Cpu& c, const Frame& frame, const RandomTable& random, unsigned depth) {
    const auto& level = levels[depth];
    const auto range = std::int32_t(c.r(level.range));
    random.advance(level.rand_back);
    if (within_table(c, range)) {
        const auto value = narrow_below(c, random, level.division, range);
        if (depth == 0) return std::int32_t(result(c, std::uint32_t(value)));
        c.set(level.part, std::uint32_t(value));
        return value;
    }
    Fpu fpu{c};
    const auto d = wide_draw(c, fpu, random, range, frame.at(level.range_at), frame.at(0x40), frame.at(level.result_at));
    c.set(level.part, std::uint32_t(d.part));
    std::int32_t refined = 0;
    if (depth < 3) {
        c.set({{3, random.position()}, {4, d.coarse.sign}, {5, d.multiple}, {6, d.whole.sign}, {7, d.x}, {8, d.entry},
               {9, 0x43300000}});
        const auto& next = levels[depth + 1];
        c.set(next.range, std::uint32_t(d.coarse.value));
        if (nonzero(c, d.coarse.value)) refined = draw_level(c, frame, random, depth + 1);
        else c.set(next.part, 0);
    } else {
        d.leave_b(c);
        enter(c, 0x10357220, {std::uint32_t(d.coarse.value)});
        refined = sample_range(c, d.coarse.value);
    }
    const auto part = wide_total(c, d.part, refined, range, depth != 0);
    c.set(level.part, std::uint32_t(part));
    return depth == 0 ? std::int32_t(result(c, std::uint32_t(part))) : part;
}

// Frees both banks' lists, storing null (held in register Zero) back; the
// loop's registers are the caller's (reset_storage and destroy_storage
// differ only in them).
template <unsigned Zero, unsigned BankOffset, unsigned Bank, unsigned Category, unsigned CategoryOffset, unsigned Slot>
void free_lists(Cpu& c, std::uint32_t address, std::uint32_t back) {
    const Kept<Zero> zero{c, 0};
    Kept<BankOffset> bank_offset{c, 0};
    Kept<Bank> bank{c, 0};
    for (; compare(c, bank, 2) < 0; bank = bank + 1, bank_offset = bank_offset + 0x40) {
        Kept<Category> category{c, 0};
        Kept<CategoryOffset> category_offset{c, 0};
        for (; compare(c, category, 16) < 0; category = category + 1, category_offset = category_offset + 4) {
            const Kept<Slot> offset{c, bank_offset + category_offset + 0x18};
            const Field<std::uint32_t> list{c, address + offset};
            if (nonzero_logical(c, result(c, list))) {
                Program{c}.free(list, back);
                list = zero;
            }
        }
    }
}
}

std::int32_t sample_range(Cpu& c, std::int32_t range) {
    Frame frame(c, 0x10356e40, 0x80, {.first_gpr = 20});
    const RandomTable random{c};
    c.set(30, std::uint32_t(range));
    keep(c, {{25, c.toc(slot::constants)}, {26, random.salt}, {27, random.table}, {28, random.end}, {29, random.cursor}});
    if (!nonzero(c, range)) return std::int32_t(result(c, 0));
    return draw_level(c, frame, random, 0);
}

std::uint32_t destroy_storage(Cpu& c, std::uint32_t address, std::uint32_t flags) {
    Frame frame(c, 0x103567b0, 0x60, {.first_gpr = 24});
    Program program{c};
    const Manager manager{c, address};
    const Kept<30> object{c, address};
    const bool present = nonzero(c, std::int32_t(address));
    const auto club_count = c.toc(slot::club_count);
    keep(c, {{28, club_count}, {31, flags}});
    if (!present) return result(c, address);
    if (nonzero_logical(c, manager.clubs())) {
        // Each club's records (null or not), then the table.
        Kept<27> offset{c, 0};
        Kept<26> club{c, 0};
        const Kept<29> zero{c, 0};
        for (; compare(c, club, c.i32(club_count)) < 0; club = club + 1, offset = offset + ClubEntry::size) {
            const ClubEntry entry{c, manager.clubs() + offset};
            program.free(entry.rows(), 0x103567fc);
            ClubEntry{c, manager.clubs() + offset}.rows() = zero;
        }
        program.free(manager.clubs(), 0x10356820);
        manager.clubs() = 0;
    }
    free_lists<29, 28, 25, 24, 27, 26>(c, address, 0x10356858);
    if (nonzero_logical(c, result(c, manager.links_by_staff()))) {
        program.free(manager.links_by_staff(), 0x1035688c);
        manager.links_by_staff() = 0;
    }
    if (nonzero_logical(c, result(c, manager.links_by_club()))) {
        program.free(manager.links_by_club(), 0x103568a4);
        manager.links_by_club() = 0;
    }
    if (compare(c, std::int16_t(flags)) > 0) program.operator_delete(address, 0x103568bc);
    return result(c, address);
}

// Each list's capacity is the most persons one category admits, counted over
// every person; the lists are then filled from all but the last sixteen
// persons (quirk: `index < person_count - 16`). The draw of a missing nation
// inlines sample_range's first level.
void reset_storage(Cpu& c, std::uint32_t address) {
    Frame frame(c, 0x103568e0, 0xf0, {.first_gpr = 13});
    Program program{c};
    const Manager manager{c, address};
    const RandomTable random{c};
    const auto contracts = c.toc(slot::contracts), scratch = c.toc(contract_scratch), nations = c.toc(nations_slot);
    const auto person_count = c.toc(slot::person_count), pool = c.toc(slot::pool);
    keep(c, {{23, contracts}, {24, scratch}, {15, address}, {14, nations}, {25, c.toc(slot::constants)},
             {26, random.salt}, {13, random.table}, {27, random.cursor}, {28, person_count},
             {29, c.toc(slot::success)}, {30, c.toc(slot::message)}, {31, pool}});
    Kept<22> persons{c, c.toc(slot::persons)};  // later the nation count

    // Both banks freed (r16 holds the null stored back).
    free_lists<16, 18, 21, 20, 17, 19>(c, address, 0x10356950);

    // The lists' capacity.
    manager.capacity() = 0;
    {
        Kept<18> category{c, 0};
        for (; compare(c, std::int8_t(category), 16) < 0; category = category + 1) {
            Kept<16> person{c, c.u32(persons)};
            Kept<17> admitted{c, 0};
            Kept<19> index{c, 0};
            for (; compare(c, index, c.i32(person_count)) < 0; index = index + 1, person = person + person_size)
                if (nonzero(c, std::int8_t(program.call(routine::eligible, 0x103569a4, {address, person, category}))))
                    admitted = admitted + 1;
            if (compare(c, admitted, manager.capacity()) > 0) manager.capacity() = admitted;
        }
    }

    // Both banks at that capacity.
    {
        Kept<18> bank{c, 0};
        Kept<17> bank_offset{c, 0};
        for (; compare(c, bank, 2) < 0; bank = bank + 1, bank_offset = bank_offset + 0x40) {
            Kept<19> category{c, 0};
            Kept<16> category_offset{c, 0};
            for (; compare(c, std::int8_t(category), 16) < 0; category_offset = category_offset + 4, category = category + 1) {
                const Field<std::uint32_t> list{c, address + bank_offset + category_offset + 0x18};
                list = program.malloc(std::uint32_t(manager.capacity()) * CategoryEntry::size, 0x10356a04);
                if (!nonzero_logical(c, list)) {
                    report(c, pool, 0x10356a30, frame.at(0x68), frame.at(0x40), 0x109, false);
                    program.call(routine::fatal_exit, 0x10356a74, {std::uint32_t(-1)});
                }
            }
        }
    }
    program.memset(address + 0x98, 0, 0x40, 0x10356aa8);

    // Every person (but the last sixteen) into the lists of the categories
    // that admit them.
    Kept<18> person_address{c, c.u32(persons)};
    Kept<19> index{c, 0};
    Kept<17> nation_index{c, c.r(17)};
    const auto more_persons = [&] {
        const auto count = c.i32(person_count);
        c.set(3, std::uint32_t(count));
        return compare(c, index, std::int32_t(std::uint32_t(count) - 16)) < 0;
    };
    for (; more_persons(); index = index + 1, person_address = person_address + person_size) {
        const Person person{c, person_address};
        Kept<16> category{c, 0};
        Kept<21> category_offset{c, 0};
        for (; compare(c, std::int8_t(category), 16) < 0; category_offset = category_offset + 4, category = category + 1) {
            if (!nonzero(c, std::int8_t(program.call(routine::eligible, 0x10356ad4, {address, person_address, category}))))
                continue;
            const Kept<20> lists{c, address + category_offset};  // list(0, k) at +0x18, list(1, k) at +0x58, count(k) at +0x98
            const Field<std::int32_t> count{c, lists + 0x98};
            if (compare(c, count, manager.capacity()) >= 0) {
                report(c, pool, 0x10356b04, frame.at(0x48), frame.at(0x38), 0x11e, false);
                program.call(routine::fatal_exit, 0x10356b48, {std::uint32_t(-1)});
            }
            const auto entry = [&](std::uint32_t bank) {
                return CategoryEntry{c, c.u32(lists + 0x18 + bank * 0x40) + std::uint32_t(count.get()) * CategoryEntry::size};
            };
            entry(0).person() = std::int32_t(std::uint32_t(index));
            entry(1).person() = std::int32_t(std::uint32_t(index));
            if (!nonzero_logical(c, result(c, person.nation()))) {
                // A person without a nation gets one drawn from all of them.
                const auto count_at = c.toc(nation_count_slot);
                c.set(3, count_at);
                persons = c.u32(count_at);
                const DrawSite site{0x10356be8, 0x10356cf8, 0x10356cb0, frame.at(0x98), frame.at(0x90), frame.at(0x88),
                                    false, {.previous_in_r4 = true}};
                Fpu fpu{c};
                draw_below(c, fpu, random, site, std::int32_t(std::uint32_t(persons)), 17,
                           [&](const WideDraw& d) { d.leave_a(c); });
                person.nation() = c.u32(nations) + nation_index * Nation::size;
            }
            entry(0).nation_id() = person.nation_record().id().get();
            entry(1).nation_id() = person.nation_record().id().get();
            entry(0).rating() = person.value().get();
            c.set({{5, c.u32(lists + 0x18)}, {6, std::uint32_t(person.value().get())}});
            const auto contract = program.call(routine::primary_contract, 0x10356d7c, {c.u32(contracts), person_address});
            c.put32(scratch, contract);
            std::uint32_t club = 0;
            if (nonzero_logical(c, contract)) club = c.u32(c.toc(slot::clubs)) + c.u32(contract + 4) * club_size;
            const auto with_club = program.call(routine::club_contract, 0x10356db0, {c.u32(contracts), person_address, club});
            const std::int32_t rating = nonzero_logical(c, with_club) ? std::int32_t(c.i8(with_club + 0x44)) : 100;
            entry(1).rating() = rating;
            c.set({{3, std::uint32_t(count.get())}, {4, c.u32(lists + 0x58)}, {5, std::uint32_t(rating)}});
            count = count + 1;
        }
    }
}

// Bank 0 rates by the person's value (category 15 only so, and without
// touching bank 1); a contract in its expiry band (inferred) or none rates
// 0, and a human's club's person passing 0x100d3040 loses 1,500,000 down to
// a third of the value. Bank 1 rates by the contract's byte +0x44, 2000 for
// persons without a contract, club or league (or a club below 6000), -1000
// with the contract's bit 2, 2000 with bits 1 or 8. Each list is then sorted
// (compare_by_value_then_key: rating, then person).
void rate_categories(Cpu& c, std::uint32_t address) {
    Frame frame(c, 0x10357d30, 0x70, {.first_gpr = 18});
    Program program{c};
    const auto by_rating_1 = c.toc(slot::compare_10358f50_tv), by_rating_0 = c.toc(slot::compare_10358f20_tv);
    const auto competitions = c.toc(slot::competitions), contracts = c.toc(slot::contracts);
    const auto scratch = c.toc(contract_scratch), persons = c.toc(slot::persons), clubs = c.toc(slot::clubs);
    keep(c, {{25, by_rating_1}, {18, address}, {26, by_rating_0}, {27, competitions}, {28, contracts}, {29, scratch},
             {30, persons}, {31, clubs}});
    Kept<21> person_address{c, c.r(21)};
    // The two lookups both banks make: the person's contract (into the game's
    // scratch) and the contract with its club. Returns the latter.
    const auto contract_of = [&](std::uint32_t list, std::uint32_t offset, std::uint32_t back) {
        person_address = c.u32(persons) + c.u32(list + offset) * person_size;
        const auto contract = program.call(routine::primary_contract, back, {c.u32(contracts), person_address});
        c.put32(scratch, contract);
        std::uint32_t club = 0;
        if (nonzero_logical(c, contract)) club = c.u32(clubs) + c.u32(contract + 4) * club_size;
        return program.call(routine::club_contract, back + 0x30, {c.u32(contracts), person_address, club});
    };
    Kept<20> category{c, 0};
    Kept<23> category_offset{c, 0};
    for (; compare(c, category, 16) < 0; category = category + 1, category_offset = category_offset + 4) {
        const Kept<22> lists{c, address + category_offset};
        const Field<std::uint32_t> first{c, lists + 0x18}, second{c, lists + 0x58};
        const Field<std::int32_t> count{c, lists + 0x98};
        Kept<19> index{c, 0};
        Kept<24> offset{c, 0};
        const auto more = [&] {
            c.set(4, std::uint32_t(count.get()));
            return compare(c, index, count) < 0;
        };
        for (; more(); offset = offset + CategoryEntry::size, index = index + 1) {
            const auto contract = contract_of(first, offset, 0x10357d98);
            const Person person{c, person_address};
            const Field<std::int32_t> rating_0{c, first + offset + 8};
            if (compare(c, category, 15) == 0) {
                rating_0 = person.value().get();
                c.set({{3, first}, {4, std::uint32_t(person.value().get())}});
                continue;
            }
            if (!nonzero_logical(c, contract) ||
                nonzero(c, std::int8_t(program.call(routine::expiring, 0x10357df0)))) {
                c.set(4, 0);
                rating_0 = 0;
            } else {
                c.set(4, std::uint32_t(person.value().get()));
                rating_0 = person.value().get();
                if (nonzero_logical(c, person.club()) &&
                    nonzero(c, std::int8_t(program.call(routine::human_club, 0x10357e28, {person.club(), 0, 1, 0, 0}))) &&
                    nonzero(c, std::int8_t(program.call(routine::person_test, 0x10357e3c, {person_address, 0})))) {
                    rating_0 = std::int32_t(std::uint32_t(rating_0.get()) - 1500000);
                    const auto high = multiply_high(0x55555556, person.value());
                    const auto third = std::int32_t(std::uint32_t(high) + (std::uint32_t(high) >> 31));
                    c.set({{3, std::uint32_t(high)}, {4, std::uint32_t(rating_0.get())}, {5, first}, {6, offset + 8}});
                    if (compare(c, rating_0, third) < 0) rating_0 = third;
                }
            }

            const auto with_club = contract_of(second, offset, 0x10357ebc);
            const Person rated{c, person_address};
            const Field<std::int32_t> rating_1{c, second + offset + 8};
            std::int32_t rating = 2000;
            bool in_league = false;
            if (nonzero_logical(c, rated.player())) {
                const auto club = rated.club().get();
                c.set(5, club);
                if (nonzero_logical(c, club)) {
                    const auto league = c.u32(club + 0x30);
                    c.set(4, league);
                    if (nonzero_logical(c, league)) {
                        const auto competition = c.u32(c.u32(competitions) + c.u32(league) * 4);
                        c.set(4, c.u32(competitions));
                        in_league = nonzero_logical(c, competition) || compare(c, c.i16(club + 0x1ec), 6000) >= 0;
                    }
                }
            }
            if (in_league && nonzero_logical(c, with_club)) {
                const auto bits = std::int32_t(c.i8(with_club + 0x5c));
                if (nonzero(c, bits & 2)) rating = -1000;
                else if (nonzero(c, bits & 1) || nonzero(c, bits & 8)) rating = 2000;
                else rating = c.i8(with_club + 0x44);
            }
            c.set({{3, second}, {4, std::uint32_t(rating)}});
            rating_1 = rating;
        }
        c.set(4, std::uint32_t(count.get()));
        program.qsort(first, std::uint32_t(count.get()), CategoryEntry::size, by_rating_0, 0x10357fe4);
        program.qsort(second, std::uint32_t(count.get()), CategoryEntry::size, by_rating_1, 0x10357ff8);
    }
}
}
