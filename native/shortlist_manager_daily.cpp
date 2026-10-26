// SHORTLIST_MANAGER.CPP: one club's budget share-out over its shortlist rows
// (0x10359020) and the two small entry points around it: the club refresh
// (0x10357cb0) and the approach reset for one person (0x1036f100).
#include "shortlist_manager_detail.hpp"
#include "shortlist_manager_random.hpp"

// The group's code lives in namespace daily, so its record views take
// precedence over other groups' views of the same names.
namespace cm::aot::shortlist_manager::daily {
using namespace detail;

void share_out_budget(Cpu& c, std::uint32_t manager, std::uint32_t club_address) {
    Frame frame(c, 0x10359020, 0xc0, {.first_gpr = 13});
    Program program{c};
    // The prologue's registers, kept across the calls as the original keeps them.
    const Kept<16> club{c, club_address};
    const Kept<15> manager_reg{c, manager};
    const Kept<25> transfers{c, c.toc(game_slot::transfers)};
    const Kept<26> contracts{c, c.toc(slot::contracts)};
    Kept<21> total_weight{c, 0};  // the open rows' weights; later the person examined
    const Kept<27> constants{c, c.toc(slot::constants)};
    Kept<23> open_rows{c, 0};  // the open rows; later the row index, then his contract
    const Kept<28> salt{c, c.toc(slot::random_salt)};
    Kept<20> players{c, 0};  // player rows given a share; later the row index
    const Kept<14> table{c, c.toc(slot::random_table)};
    Kept<19> allocated{c, 0};  // the staff and player shares; later the row offset
    const Kept<13> table_end{c, c.toc(slot::random_end)};
    Kept<18> end{c, 8};  // rows considered; later a random digit
    const Kept<29> cursor{c, c.toc(slot::random_cursor)};
    const Kept<30> persons{c, c.toc(slot::persons)};
    const Club owner{c, club_address};

    if (nonzero(c, std::int8_t(program.call(0x1014c2e0, 0x10359084, {club_address, 0, 1, 0, 0})))) return;
    if (nonzero(c, std::int8_t(program.call(0x100c7d00, 0x10359094, {club_address})))) return;
    const std::uint32_t finances = program.call(0x10103510, 0x103590ac, {c.u32(c.toc(game_slot::finances)), club_address});
    if (!nonzero_logical(c, finances)) return;
    // The club's budget for the shortlist: 0x100f5770's second result.
    program.call(0x100f5770, 0x103590cc, {finances, 0, 0, 0, 1, 0});
    Kept<17> remaining{c, c.r(4)};
    if (compare(c, std::int32_t(remaining)) < 0) remaining = 0;
    const std::uint32_t entries = Manager{c, manager}.clubs().get();
    c.set(3, entries);
    const Kept<22> entry_address{c, entries + std::uint32_t(owner.id()) * ClubEntry::size};
    const ClubEntry entry{c, entry_address};
    if (compare(c, entry.status(), 1) != 0) return;
    program.qsort(entry.rows(), Row::per_club, Row::size, c.toc(slot::compare_10358f80_tv), 0x10359108);

    // The open rows (no target) among the first eight, which end after the
    // fourth open player row: their count and weights.
    {
        std::int8_t player_rows = 0;
        std::uint32_t offset = 0;
        for (std::int8_t index = 0; index < std::int8_t(std::uint32_t(end)); ++index, offset += Row::size) {
            const Row row{c, entry.rows() + offset};
            if (row.target() != -1) continue;
            if (row.kind() == Row::player && ++player_rows == 4) end = extended8(std::uint32_t(index) + 1);
            open_rows = open_rows + 1;
            total_weight = total_weight + std::uint32_t(row.weight().get());
        }
    }
    const std::int32_t rows_end = std::int8_t(std::uint32_t(end));
    // A first share of the budget for each open row by its kind: players by
    // category, staff roles (no previous holder) only at a club with
    // +0x23d == 1.
    {
        const std::int32_t open = std::int8_t(std::uint32_t(open_rows));
        std::uint32_t offset = 0;
        for (std::int32_t index = 0; index < rows_end; ++index, offset += Row::size) {
            const Row row{c, entry.rows() + offset};
            c.set(4, row.address);
            const std::int32_t target = row.target();
            c.set(6, std::uint32_t(target));
            if (target != -1) continue;
            const std::uint8_t kind_byte = row.kind();
            c.set(6, kind_byte);
            const std::int32_t kind = std::int8_t(kind_byte);
            c.set(7, std::uint32_t(kind));
            // Each kind has a divw of its own (open is 0 when no row is
            // open, and then none of these is reached).
            std::int32_t divisor = 0;
            std::uint32_t division = 0;
            if (kind == Row::player) {
                const std::uint32_t category = row.category();
                c.set(6, category);
                if (category == 2) divisor = open * 2, division = 0x103591e0;
                else if (category == 3) divisor = open * 2, division = 0x103591f8;
                else if (category == 4) divisor = open * 4, division = 0x10359210;
                else if (category == 5) divisor = open * 10, division = 0x10359228;
                else continue;
            } else {
                const std::int32_t previous = row.previous();
                c.set(6, std::uint32_t(previous));
                if (previous != -1) continue;
                if (kind == 6) divisor = open * 10, division = 0x10359284;
                else if (kind == 7) divisor = open * 1000, division = 0x103592b0;
                else if (kind == 8) divisor = open * 10, division = 0x103592dc;
                else if (kind == 9) divisor = open * 20, division = 0x10359308;
                else if (kind == 10) divisor = open * 20, division = 0x10359334;
                else continue;
                const std::uint8_t staffed = owner.status();
                c.set(6, staffed);
                if (staffed != 1) {
                    c.set(6, 0);
                    row.budget() = 0;
                    continue;
                }
            }
            c.set(6, std::uint32_t(divisor));
            const std::uint32_t share = std::uint32_t(divide(division, std::int32_t(remaining), divisor));
            c.set(6, share);
            row.budget() = std::int32_t(share);
            allocated = allocated + share;
        }
        c.set({{3, std::uint32_t(rows_end) * Row::size}, {4, std::uint32_t(rows_end)}, {5, std::uint32_t(rows_end)}});
    }
    // Each open row's final budget from what is left, then its candidate
    // scan (0x1035e9f0); a found target's value is set aside.
    open_rows = 0;
    remaining = remaining - allocated;
    Kept<24> offset{c, 0};
    const Kept<31> last{c, std::uint32_t(rows_end)};
    for (; std::int32_t(std::int8_t(std::uint32_t(open_rows))) < std::int32_t(last);
         offset = offset + Row::size, open_rows = open_rows + 1) {
        const Row row{c, entry.rows() + offset};
        c.set(3, row.address);
        const std::uint8_t kind_byte = row.kind();
        c.set(4, kind_byte);
        if (kind_byte != Row::player && row.previous() > -1) continue;
        const std::int32_t target = row.target();
        if (target > -1) continue;
        const std::int32_t kind = std::int8_t(kind_byte);
        c.set(5, std::uint32_t(kind));
        if (kind == 7) continue;
        const std::int32_t approach = row.approach();
        c.set(4, std::uint32_t(approach));
        if (approach > -1) {
            // An approach without a target: dropped, or (an index below -1)
            // its value set aside.
            if (target == -1) {
                row.approach() = -1;
            } else if (kind == Row::player) {
                const std::uint32_t found = program.call(0x103eb440, 0x103593dc, {c.u32(transfers)});
                const std::uint32_t bid = c.u32(found);
                c.set(3, bid);
                if (bid) remaining = remaining - c.u32(bid + 0x20);
            } else {
                const std::uint32_t at = std::uint32_t(target) * Person::size, base = c.u32(persons);
                c.set({{3, at}, {4, base}});
                remaining = remaining - c.u32(base + at + 8);
            }
            continue;
        }
        Fpu fpu{c};
        const auto integer = [&](std::int32_t at, std::uint32_t value) {
            return fpu.fsub(stacked_integer(c, frame.at(at), signed_word(std::int32_t(value))), k::signed_bias);
        };
        if (owner.status() == 1 || kind == Row::player) {
            // Topped up by half the rest in proportion to the first share.
            const std::uint32_t budget = std::uint32_t(row.budget().get());
            c.set({{6, budget}, {5, 0x43300000}});
            const double rest = integer(0x60, remaining);
            c.set(4, signed_word(std::int32_t(budget)));
            const double first = integer(0x58, budget);
            const std::uint32_t shares = std::int32_t(allocated) < 1000 ? 1000 : std::uint32_t(allocated);
            c.set(4, signed_word(std::int32_t(shares)));
            const double ratio = fpu.fdiv(first, integer(0x50, shares));
            c.set_f(1, k::signed_bias);  // the original loads the bias into f1 (lfd f1)
            const auto bits = fpu.fctiwz(0, fpu.fmul(constant::half, fpu.fmul(rest, ratio)));
            c.put64(frame.at(0x48), bits);
            row.budget() = std::int32_t(budget + std::uint32_t(bits));
        } else {
            row.budget() = 0;
        }
        {
            const Row again{c, entry.rows() + offset};
            c.set(3, again.address);
            const std::uint32_t category = again.category();
            c.set(4, category);
            // The rest by weight: in full to the first three player rows of
            // categories 0-3, by half to higher categories.
            const bool full = category <= 3;
            if (!full || std::int8_t(std::uint32_t(players)) < 3) {
                const std::int32_t weight = again.weight();
                const std::uint32_t by = weight < 1 ? 1 : std::uint32_t(weight);
                c.set(6, by);
                if (full) players = players + 1;
                c.set(4, 0x43300000);
                const double rest = integer(0x48, remaining);
                const std::uint32_t budget = std::uint32_t(again.budget().get());
                c.set(5, budget);
                const std::uint32_t weights = std::int32_t(total_weight) < 1 ? 1 : std::uint32_t(total_weight);
                const double ratio = fpu.fdiv(rest, integer(0x50, weights));
                c.set_f(1, ratio);  // the original computes it into f1
                double share = fpu.fmul(ratio, integer(0x58, by));
                if (!full) share = fpu.fmul(constant::half, share);
                const auto bits = fpu.fctiwz(0, share);
                c.put64(frame.at(0x60), bits);
                again.budget() = std::int32_t(budget + std::uint32_t(bits));
            }
        }
        // The candidate scan for the row, its bit 7 cleared first.
        const std::uint32_t rows = entry.rows();
        c.set({{7, rows}, {4, offset + 0x28}});
        c.put8(rows + offset + 0x28, c.u8(rows + offset + 0x28) & 0x7f);
        program.call(0x1035e9f0, 0x1035963c, {manager, entry.rows() + offset, club_address, 0});
        const Row scanned{c, entry.rows() + offset};
        c.set(3, scanned.address);
        const std::int32_t found = scanned.target();
        c.set(4, std::uint32_t(found));
        if (found > -1 && !(scanned.flags() & 0x80)) {
            // A target found: 1.3 times his value (+8) is set aside.
            const std::uint32_t at = std::uint32_t(found) * Person::size, base = c.u32(persons);
            c.set({{4, at}, {5, base}});
            const double value = integer(0x48, c.u32(base + at + 8));
            c.set_f(1, k::signed_bias);  // the original loads the bias into f1 (lfd f1)
            const auto bits = fpu.fctiwz(0, fpu.fmul(constant::one_point_three, value));
            c.put64(frame.at(0x50), bits);
            remaining = remaining - std::uint32_t(bits);
        }
        total_weight = total_weight - std::uint32_t(scanned.weight().get());
    }

    // With money left, the rows after them (up to the fortieth): the first
    // player row whose target is at the club under another club's contract
    // (inferred: a loanee it wants to keep) takes the rest as its budget.
    Kept<19> at{c, std::uint32_t(rows_end) * Row::size};
    Kept<20> index{c, end};
    for (; compare(c, std::int8_t(std::uint32_t(index)), 40) < 0 && compare(c, std::int32_t(remaining)) > 0;
         at = at + Row::size, index = index + 1) {
        const Row row{c, entry.rows() + at};
        c.set(4, row.address);
        if (row.kind() != Row::player) continue;
        const std::int32_t target = row.target();
        c.set(3, std::uint32_t(target));
        if (target <= -1) continue;
        if (row.approach() != -1) continue;
        const std::uint32_t base = c.u32(persons);
        c.set(3, base);
        total_weight = base + std::uint32_t(target) * Person::size;
        const Kept<21>& person = total_weight;
        if (c.u32(person + 0x24) != club_address) continue;
        // His contract's club (the contract lookup also sets the shared
        // contract slot).
        const std::uint32_t contract = program.call(0x10082f20, 0x1035971c, {c.u32(contracts), person});
        c.set(4, c.toc(game_slot::last_contract));
        c.put32(c.toc(game_slot::last_contract), contract);
        std::uint32_t contract_club = 0;
        if (nonzero_logical(c, contract)) {
            const std::uint32_t clubs = c.u32(c.toc(slot::clubs));
            c.set(3, clubs);
            contract_club = clubs + c.u32(contract + 4) * Club::size;
        }
        if (compare_logical(c, contract_club, club_address) == 0) continue;
        if (nonzero(c, std::int8_t(program.call(0x100d3230, 0x1035975c, {person, 0})))) continue;
        open_rows = program.call(0x10082f90, 0x10359774, {c.u32(contracts), person, club_address});
        const Kept<23>& own = open_rows;  // his contract with the club (0: none)
        const std::uint32_t status = nonzero_logical(c, own) ? c.u8(own + 0x60) & 0xf : 4;
        program.call(0x10369080, 0x103597b0, {manager, person, club_address, 1, 0, extended8(status), 0});
        {
            Fpu fpu{c};
            fpu.fcmp(0, c.f(1), constant::interest_threshold);
            const std::uint8_t cr = c.cr(0);
            c.set_cr(0, std::uint8_t((cr & ~2u) | (((cr >> 2) | (cr >> 1)) & 1) << 1));  // cror eq,gt,eq
            if (!(c.cr(0) & 2)) continue;
        }
        bool sell = false;
        if (nonzero_logical(c, own)) {
            // A random digit against 20 - 0x10374d10's result.
            const RandomTable random{c, cursor, table_end, table, salt};
            random.advance(0x103597e8);
            const std::int32_t drawn = std::int32_t(random.value());
            end = std::uint32_t(drawn % 10);
            c.set({{4, std::uint32_t(drawn < 0)}, {5, std::uint32_t(drawn)}});
            const std::int32_t value = std::int16_t(program.call(0x10374d10, 0x10359828, {own}));
            c.set(3, std::uint32_t(value));
            sell = compare(c, value, 20 - std::int32_t(end)) < 0;
        }
        if (!sell) {
            sell = compare(c, std::int32_t(program.call(0x103eaea0, 0x1035984c, {c.u32(transfers), person, 0, 0}))) != 0;
            if (!sell) {
                if (!nonzero_logical(c, own)) continue;
                // His contract ends before a random day 30-129 days ahead.
                const RandomTable random{c, cursor, table_end, table, salt};
                random.advance(0x1035987c);
                const std::int32_t drawn = std::int32_t(random.value());
                const std::uint32_t days = extended(std::int16_t(drawn % 100 + 30));
                c.set(6, std::uint32_t(drawn));
                program.call(0x10717b20, 0x103598c8, {frame.at(0x40), c.toc(slot::today), days});
                c.put32(frame.at(0x38), c.u32(frame.at(0x40)));
                c.put32(frame.at(0x3c), c.u32(frame.at(0x44)));
                const std::uint32_t contract_year = c.u16(own + 0x2a), year = c.u16(frame.at(0x3a));
                c.set({{3, contract_year}, {4, year}});
                std::int32_t later;
                if (compare_logical(c, year, contract_year) > 0) {
                    later = 1;
                } else if (year < contract_year) {
                    later = 0;
                } else {
                    // The original's signed day compare, as compiled.
                    const std::int32_t day = c.i16(frame.at(0x38)), contract_day = c.i16(own + 0x28);
                    const std::int32_t both = day ^ contract_day, half = both >> 1;
                    c.set({{3, std::uint32_t(half)}, {4, std::uint32_t(day)}});
                    later = std::int8_t((std::uint32_t(half - (both & day)) >> 31) & 1);
                }
                if (!nonzero(c, std::int8_t(later))) continue;
            }
        }
        const std::uint32_t chosen = std::uint32_t(std::int32_t(std::int8_t(std::uint32_t(index)))) * Row::size;
        const Row taken{c, entry.rows() + chosen};
        taken.weight() = taken.level().get();
        Row{c, entry.rows() + chosen}.budget() = std::int32_t(remaining);
        c.set({{3, entry.rows() + chosen}, {4, chosen}});
        return;
    }
}

void refresh_club_shortlist(Cpu& c, std::uint32_t manager, std::uint32_t club) {
    Frame frame(c, 0x10357cb0, 0x40, {.first_gpr = 30});
    Program program{c};
    const Kept<31> club_reg{c, club};
    const Kept<30> manager_reg{c, manager};
    const auto entry = [&] { return Manager{c, manager}.clubs().get() + std::uint32_t(Club{c, club}.id()) * ClubEntry::size; };
    program.call(0x10366250, 0x10357ce0, {manager, club, entry()});
    program.call(0x1035a9f0, 0x10357d04, {manager, entry(), club, 1, 0});
    enter_direct(c, 0x10357d10, {manager, club});
    share_out_budget(c, manager, club);
}

void forget_approaches(Cpu& c, std::uint32_t manager, std::uint32_t club, std::uint32_t person) {
    // Frameless: everything in volatile registers.
    const std::uint32_t entries = Manager{c, manager}.clubs().get();
    c.set(3, entries);
    const ClubEntry entry{c, entries + std::uint32_t(Club{c, club}.id()) * ClubEntry::size};
    c.set(6, entry.address);
    if (compare(c, entry.status(), 1) != 0) return;
    std::uint32_t offset = 0, wanted = 0, row = 0;
    for (unsigned index = 0; index < Row::per_club; ++index, offset += Row::size) {
        wanted = c.u32(person);
        row = entry.rows() + offset;
        if (std::int32_t(wanted) == Row{c, row}.target()) Row{c, row}.approach() = -1;
    }
    c.set({{3, std::uint32_t(-1)}, {4, wanted}, {7, offset}, {8, row}, {9, Row::per_club}});
    compare(c, Row::per_club, 40);
}
}
