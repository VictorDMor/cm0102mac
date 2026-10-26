// SHORTLIST_MANAGER.CPP: the shortlists of a new game (0x103573c0) and one
// club's shortlist rebuilt (0x1036ec30).
#include "shortlist_manager_detail.hpp"

namespace cm::aot::shortlist_manager::daily {
using namespace detail;
namespace {
// The game's routines these call (names by use).
constexpr std::uint32_t list_init = 0x10738c00;     // a list header (16 bytes) emptied
constexpr std::uint32_t list_destroy = 0x10738c20;  // (list, -1): its nodes freed
constexpr std::uint32_t list_next = 0x10738e00;     // the item at the list's cursor (+0xc), which moves on; 0 at the end
constexpr std::uint32_t contract_of = 0x10082f20;   // (contracts, person): his current contract
constexpr std::uint32_t contract_with = 0x10082f90; // (contracts, person, club): his contract with the club
constexpr std::uint32_t person_value = 0x10084be0;  // (contracts, person, club, 0, -1, flag): his value (person+8)
constexpr std::uint32_t club_flag = 0x1014c2e0;     // (club, which, 1, 0, 0): a club predicate (signed byte)

// More nations' ids (int32 TOC words) the contract valuation compares the
// club's nation with, named by slot.
namespace nation_slot {
constexpr std::int32_t n_7af4 = -0x7af4, n_7a00 = -0x7a00, n_7a1c = -0x7a1c, n_7a28 = -0x7a28, n_757c = -0x757c,
                       n_7594 = -0x7594, n_7574 = -0x7574, n_7578 = -0x7578, n_79fc = -0x79fc, n_7a10 = -0x7a10,
                       n_7a14 = -0x7a14, n_7a18 = -0x7a18, n_7a24 = -0x7a24, n_7a20 = -0x7a20;
}
// The thresholds and slopes of the contract valuation (slot::constants +offset).
namespace valuation {
constexpr double t20000 = 20000.0, t250 = 250.0, t500 = 500.0, t100 = 100.0, t200 = 200.0, t350 = 350.0,
                 t25000 = 25000.0;                                   // +0x10, +0x20, +0x28, +0x30, +0x40, +0x50, +0x60
constexpr double quarter = 0.25, s015 = 0.15, tenth = 0.1, fifth = 0.2;  // +0x18, +0x38, +0x48, +0x58
}

// Above `threshold`, a contract's value keeps `slope` of the excess: the
// integer through the stack temporary at `integer_at`, the result's fctiwz
// stored at `bits_at`; `site` is the original's fmadd.
void damp(Cpu& c, Frame& frame, const Contract& contract, double threshold, double slope, std::uint32_t site,
          std::int32_t integer_at, std::int32_t bits_at) {
    Fpu fpu{c};
    const std::uint32_t word = signed_word(contract.wage());
    c.set(3, word);
    const double value = fpu.fsub(stacked_integer(c, frame.at(integer_at), word), k::signed_bias);
    fpu.fcmp(0, value, threshold);
    if (!(c.cr(0) & 4)) return;  // ble
    const double again = fpu.fsub(stacked_integer(c, frame.at(integer_at), word), k::signed_bias);
    c.set_f(1, slope);  // the original loads the slope into f1 (lfd f1)
    const auto bits = fpu.fctiwz(0, fpu.multiply_add(site, slope, fpu.fsub(again, threshold), threshold));
    c.put64(frame.at(bits_at), bits);
    contract.wage() = std::int32_t(std::uint32_t(bits));
}
}

void create_shortlists(Cpu& c, std::uint32_t manager) {
    Frame frame(c, 0x103573c0, 0x100, {.first_gpr = 13});
    Program program{c};
    Kept<31> pool{c, c.toc(slot::pool)};  // later the contract valued
    const Kept<15> manager_reg{c, manager};
    const Kept<19> n_7a20{c, c.toc(nation_slot::n_7a20)};
    const Kept<20> n_7a24{c, c.toc(nation_slot::n_7a24)};
    const Kept<21> n_7a18{c, c.toc(nation_slot::n_7a18)};
    const Kept<22> n_7a14{c, c.toc(nation_slot::n_7a14)};
    const Kept<23> n_7a10{c, c.toc(nation_slot::n_7a10)};
    const Kept<24> n_79fc{c, c.toc(nation_slot::n_79fc)};
    Kept<14> regional_4{c, c.toc(slot::regional_nation_4)};  // later the club order's offset
    Kept<13> regional_2{c, c.toc(slot::regional_nation_2)};  // later the /100 multiplier
    const Kept<25> order{c, c.toc(game_slot::club_order)};
    const Kept<26> game{c, c.toc(slot::game)};
    const Kept<27> contracts{c, c.toc(slot::contracts)};
    const Kept<28> constants{c, c.toc(slot::constants)};
    const Kept<29> clubs{c, c.toc(slot::clubs)};
    const Kept<30> club_count{c, c.toc(slot::club_count)};
    const std::uint32_t progress = frame.at(0x3c);
    // The loops' test, `lwz r6,0(r30); cmpw`: r6 keeps the club count,
    // which the varargs 0x10727890 spills into this frame.
    const auto count = [&] {
        const std::uint32_t clubs_now = c.u32(club_count);
        c.set(6, clubs_now);
        return std::int32_t(clubs_now);
    };
    // The progress bar's step every hundred clubs (0x10743120: game, bar,
    // done, total, -1); the original keeps the /100 multiplier in a register.
    const auto step = [&](std::uint32_t done, std::uint32_t back) {
        if (std::int32_t(done) % 100 == 0) program.call(0x10743120, back, {game, progress, done, std::uint32_t(count()), std::uint32_t(-1)});
    };
    program.call(0x10727890, 0x1035741c, {progress, pool + text::assessing_squads});
    {
        Kept<18> index{c, 0};
        Kept<16> by_hundred{c, 0x51eb851f};
        Kept<17> at{c, 0};
        for (; compare(c, std::int32_t(index), count()) < 0; at = at + 4, index = index + 1) {
            step(index, 0x10357464);
            const std::uint32_t id = c.u32(c.u32(order) + at);
            program.call(0x1035a9f0, 0x10357494,
                         {manager, Manager{c, manager}.clubs().get() + id * ClubEntry::size, c.u32(clubs) + id * Club::size, 0, 1});
        }
    }
    program.call(0x10727890, 0x103574b4, {progress, pool + text::creating_shortlists});
    // Each person's contract with his club valued, then his value.
    {
        Kept<18> index{c, 0};
        Kept<17> person{c, c.u32(c.toc(slot::persons))};
        for (; std::int32_t(index) < c.i32(c.toc(slot::person_count)); index = index + 1, person = person + Person::size) {
            const Person who{c, person};
            Kept<16> club{c, who.club()};
            if (nonzero_logical(c, club)) {
                // 0x100ce320 may give the club to use instead (inferred: a
                // reserve side's parent), its flag byte zero.
                const std::uint32_t other = program.call(0x100ce320, 0x103574e4, {club, frame.at(0x38), 1});
                if (nonzero_logical(c, other) && !nonzero(c, c.i8(frame.at(0x38)))) club = other;
            }
            pool = program.call(contract_with, 0x1035750c, {c.u32(contracts), person, club});
            const Contract contract{c, pool};
            const auto value_person = [&] {
                if (!nonzero_logical(c, c.u32(person + 0x14))) return;  // no player record
                std::uint32_t flag = 0;
                if (nonzero_logical(c, club) && nonzero(c, std::int8_t(program.call(club_flag, 0x10357ba0, {club, 0, 1, 0, 0}))))
                    flag = 1;
                const std::uint32_t value = program.call(person_value, flag ? 0x10357bc4 : 0x10357be8,
                                                         {c.u32(contracts), person, club, 0, std::uint32_t(-1), flag});
                c.put32(person + 8, value);
            };
            if (!nonzero_logical(c, club) || !nonzero_logical(c, contract.address) || contract.wage() != -1) {
                value_person();
                continue;
            }
            program.call(0x10378da0, 0x1035754c,
                         {contract.address, person, club, 1, contract.squad_status() & 0xfu, 11, 0});
            {
                Fpu fpu{c};
                const auto bits = fpu.fctiwz(0, c.f(1));
                c.put64(frame.at(0xa8), bits);
                contract.wage() = std::int32_t(std::uint32_t(bits));
            }
            const std::uint32_t nation = c.u32(club + 0x2c);
            c.set(5, nation);
            if (!nonzero_logical(c, nation) || c.u32(person + 0xc) == nation) {
                value_person();
                continue;
            }
            // A player from abroad (inferred): his contract's value damped by
            // the club's nation, and by its reputation in two nations.
            const std::int32_t id = c.i32(nation + 0x68);
            const auto is = [&](std::uint32_t slot_address) { return id == c.i32(slot_address); };
            const auto is_toc = [&](std::int32_t slot) { return is(c.toc(slot)); };
            const std::int32_t reputation = Club{c, club}.reputation();
            const std::int32_t level = std::int8_t(std::uint32_t(reputation / 500 + 1));
            c.set(4, std::uint32_t(level));
            const bool staff = Club{c, club}.status() == 1;
            using namespace valuation;
            if (is_toc(nation_slot::n_7af4) || is_toc(nation_slot::n_7a00) || is_toc(nation_slot::n_7a1c) ||
                is_toc(nation_slot::n_7a28)) {
                damp(c, frame, contract, t20000, quarter, 0x10357620, 0xa8, 0xa0);
            } else if (is_toc(slot::regional_nation_1)) {
                if (staff) damp(c, frame, contract, t500, quarter, 0x103576f4, 0xa0, 0xa8);
                else damp(c, frame, contract, t250, quarter, 0x10357698, 0xa0, 0xa8);
            } else if (is_toc(nation_slot::n_757c) || is_toc(nation_slot::n_7594) || is_toc(nation_slot::n_7574) ||
                       is_toc(nation_slot::n_7578)) {
                if (staff) damp(c, frame, contract, t200, s015, 0x103577f8, 0xa0, 0xa8);
                else damp(c, frame, contract, t100, s015, 0x1035779c, 0xa0, 0xa8);
            } else if (is_toc(slot::regional_nation_3) || is(regional_2)) {
                if (staff) damp(c, frame, contract, t350, tenth, 0x103578d8, 0xa0, 0xa8);
                else damp(c, frame, contract, t250, tenth, 0x1035787c, 0xa0, 0xa8);
            } else if (is(regional_4) || is(n_79fc)) {
                // A floor of 5 x level squared first (single precision).
                const std::uint32_t floor = std::uint32_t(level) * std::uint32_t(level) * 5;
                Fpu fpu{c};
                const double value =
                    fpu.fsubs(stacked_integer(c, frame.at(0xa0), signed_word(contract.wage())), k::signed_bias);
                c.set_f(1, value);  // the original computes it into f1
                const double least = fpu.fsubs(stacked_integer(c, frame.at(0xa8), signed_word(std::int32_t(floor))), k::signed_bias);
                fpu.fcmp(0, value, least);
                if (c.cr(0) & 8) contract.wage() = std::int32_t(floor);
                if (staff) damp(c, frame, contract, t500, fifth, 0x103579fc, 0xa0, 0xa8);
                else damp(c, frame, contract, t250, fifth, 0x103579a0, 0xa0, 0xa8);
            } else if (is_toc(nation_slot::n_7594)) {
                // Quirk: never taken. The original tests a nation it already
                // tested above (the second of the 100/200 group), kept here.
                if (staff) damp(c, frame, contract, t500, fifth, 0x10357ac8, 0xa0, 0xa8);
                else damp(c, frame, contract, t250, fifth, 0x10357a6c, 0xa0, 0xa8);
            } else if (!is(n_7a10) && !is(n_7a14) && !is(n_7a18) && !is(n_7a24) && !is(n_7a20)) {
                damp(c, frame, contract, t25000, quarter, 0x10357b60, 0xa0, 0xa8);
            }
            value_person();
        }
    }
    program.call(0x10357d30, 0x10357c0c, {manager});
    // Every club's budget shared out, in the club order.
    Kept<16> index{c, 0};
    regional_4 = 0;
    Kept<14>& at = regional_4;
    regional_2 = 0x51eb851f;
    for (; compare(c, std::int32_t(index), count()) < 0; at = at + 4, index = index + 1) {
        const Kept<17> id{c, c.u32(c.u32(order) + at)};
        step(index, 0x10357c58);
        const std::uint32_t club = c.u32(clubs) + id * Club::size;
        enter(c, 0x10357c6c, {manager, club});
        share_out_budget(c, manager, club);
    }
    program.call(0x10743120, 0x10357c94, {game, progress, std::uint32_t(count()), std::uint32_t(count()), std::uint32_t(-1)});
}

void rebuild_club_shortlist(Cpu& c, std::uint32_t manager, std::uint32_t club_address) {
    Frame frame(c, 0x1036ec30, 0x90, {.first_gpr = 16});
    Program program{c};
    Kept<19> by_staff{c, c.toc(slot::compare_10366410_tv)};  // later a squad member
    const Kept<30> manager_reg{c, manager};
    const Kept<20> humans{c, c.toc(game_slot::humans)};
    const Kept<31> club{c, club_address};
    const Kept<21> by_club{c, c.toc(slot::compare_103663b0_tv)};
    const Kept<22> contracts{c, c.toc(slot::contracts)};
    const Kept<23> last_contract{c, c.toc(game_slot::last_contract)};
    const Kept<24> person_count{c, c.toc(slot::person_count)};
    const Kept<25> persons{c, c.toc(slot::persons)};
    const Kept<26> clubs{c, c.toc(slot::clubs)};
    const std::uint32_t links = frame.at(0x38);  // a list of the club's links
    program.call(list_init, 0x1036ec70, {links});
    if (nonzero(c, std::int8_t(program.call(0x100c7d00, 0x1036ec78, {club_address})))) {
        program.call(list_destroy, 0x1036ec8c, {links, std::uint32_t(-1)});
        return;
    }
    // Targets that are no person are dropped; for the others moved to
    // another club (his contract's, unless its type field is 5) the link
    // goes (0x10365590 mode 3) unless an approach is under way.
    {
        Kept<18> index{c, 0};
        const Kept<27> entries{c, Manager{c, manager}.clubs().get()};
        Kept<16> at{c, 0};
        const Kept<28> entry_at{c, std::uint32_t(Club{c, club_address}.id()) * ClubEntry::size};
        for (; std::int8_t(std::uint32_t(index)) < 40; at = at + Row::size, index = index + 1) {
            const auto row = [&] { return Row{c, c.u32(entries + entry_at) + at}; };
            if (row().target() > c.i32(person_count) - 1) row().target() = -1;
            const std::int32_t target = row().target();
            if (target <= -1) continue;
            Kept<29> moved{c, 0};
            const Kept<17> person{c, c.u32(persons) + std::uint32_t(target) * Person::size};
            const std::uint32_t contract = program.call(contract_of, 0x1036ed00, {c.u32(contracts), person});
            c.put32(last_contract, contract);
            if (nonzero_logical(c, contract) && std::int8_t(c.u8(contract + 0x38)) >> 2 != 5) moved = 1;
            const std::uint32_t contract_club =
                moved ? c.u32(clubs) + c.u32(c.u32(last_contract) + 4) * Club::size : 0;
            if (contract_club == club_address || c.u32(person + 0x24) == club_address) continue;
            if (row().approach() != -1) continue;
            const std::int32_t kind = std::int8_t(row().kind());
            c.set(8, std::uint32_t(kind));
            program.call(0x10365590, 0x1036eda4, {manager, person, club_address, 3, kind != Row::player});
        }
    }
    // Every link of the club removed from both of the manager's indexes
    // (8-byte pairs: by staff at +0, count +0x10; by club at +4, count +0x14).
    program.call(0x10362e00, 0x1036edc8, {manager, club_address, links});
    c.put32(links + 0xc, c.u32(links + 4));
    const auto remove = [&](std::uint32_t pair, std::uint32_t array, std::uint32_t used, std::uint32_t compare_tv,
                            std::uint32_t back) {
        const std::uint32_t found = program.bsearch(pair, c.u32(manager + array), c.u32(manager + used), 8, compare_tv, back);
        if (!nonzero_logical(c, found)) return;
        // The pairs after it move down one, the last two words are cleared.
        const std::uint32_t from = found - c.u32(manager + array);
        for (std::uint32_t index = from >> 3, offset = from & ~7u;
             std::int32_t(index) < std::int32_t(c.u32(manager + used)) - 1; ++index, offset += 8) {
            const std::uint32_t base = c.u32(manager + array), next = base + (index + 1) * 8;
            c.put32(base + offset, c.u32(next));
            c.put32(base + offset + 4, c.u32(next + 4));
        }
        c.put32(c.u32(manager + array) + c.u32(manager + used) * 8 - 4, 0);
        c.put32(c.u32(manager + array) + c.u32(manager + used) * 8 - 8, 0);
        c.put32(manager + used, c.u32(manager + used) - 1);
    };
    for (;;) {
        const Kept<16> pair{c, program.call(list_next, 0x1036ef2c, {links})};
        if (!nonzero_logical(c, pair)) break;
        remove(pair, 0, 0x10, by_staff, 0x1036edec);
        remove(pair, 4, 0x14, by_club, 0x1036ee94);
    }
    const std::int32_t id = Club{c, club_address}.id();
    program.call(0x1035a9f0, 0x1036ef60,
                 {manager, Manager{c, manager}.clubs().get() + std::uint32_t(id) * ClubEntry::size,
                  c.u32(clubs) + std::uint32_t(id) * Club::size, 0, 1});
    if (!nonzero(c, std::int8_t(program.call(club_flag, 0x1036ef78, {club_address, 0, 1, 0, 0})))) {
        enter(c, 0x1036ef8c, {manager, club_address});
        share_out_budget(c, manager, club_address);
    }
    // The values of the club's squad (its fifty person pointers at +0x98).
    const auto value = [&](std::uint32_t person, std::uint32_t flag, std::uint32_t back) {
        c.put32(person + 8, program.call(person_value, back, {c.u32(contracts), person, club_address, 0, std::uint32_t(-1), flag}));
    };
    const auto member = [&](std::uint32_t person) {
        // Quirk: the original's second test of the null pointer (a repeated
        // beq on the same compare) can never branch.
        return nonzero_logical(c, person) && compare(c, c.i32(person), c.i32(person_count)) < 0;
    };
    if (nonzero(c, std::int8_t(program.call(club_flag, 0x1036efa4, {club_address, 1, 1, 0, 0})))) {
        Kept<16> index{c, 0};
        Kept<18> at{c, 0};
        for (; std::int8_t(std::uint32_t(index)) < 50; at = at + 4, index = index + 1) {
            by_staff = c.u32(club_address + 0x98 + at);
            const Kept<19>& person = by_staff;
            if (!member(person)) continue;
            const Kept<17> contract{c, program.call(contract_of, 0x1036efec, {c.u32(contracts), person})};
            // Kept as it is when both of the human manager's options hold
            // (0x10147e70, 0x10149df0) and the contract's bit 2 at +0x5c is
            // clear (inferred: a contract the human fixed).
            bool skip = false;
            const std::uint32_t first = program.call(0x100c96e0, 0x1036eff4, {club_address});
            if (nonzero(c, std::int8_t(program.call(0x10147e70, 0x1036f004, {c.u32(humans), first})))) {
                const std::uint32_t second = program.call(0x100c96e0, 0x1036f014, {club_address});
                skip = nonzero(c, std::int8_t(program.call(0x10149df0, 0x1036f024, {c.u32(humans), 0, second}))) &&
                       nonzero_logical(c, contract) && !nonzero(c, std::int32_t(c.u8(contract + 0x5c) & 4));
            }
            if (!skip) value(person, 1, 0x1036f05c);
        }
    } else {
        Kept<17> index{c, 0};
        Kept<16> at{c, 0};
        for (; std::int8_t(std::uint32_t(index)) < 50; at = at + 4, index = index + 1) {
            const Kept<18> person{c, c.u32(club_address + 0x98 + at)};
            if (!member(person)) continue;
            value(person, 0, 0x1036f0c4);
        }
    }
    program.call(list_destroy, 0x1036f0e8, {links, std::uint32_t(-1)});
}
}
