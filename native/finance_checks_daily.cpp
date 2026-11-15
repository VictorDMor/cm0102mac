// FINANCE.CPP checker cases, block daily: the daily pass (0x100fa6e0) and the season roll (0x100fcfd0).
//
// The finance manager, the accounts, the clubs and the sample table are the career's own. A case sets the
// fields its branch reads: a club's records (nation, division, ground row, assistant, squad) are built in the
// scratch area and the club record is pointed at them; the sample table is steered by writing the halfwords
// the next draws read (base 0), so each draw's value is the case's. Routines of other modules (the date
// functions, the list, the club tests, the unit's other blocks) are answered where a case needs one outcome
// and run for real where the pass's own arithmetic is what is compared.
#include "finance_checks.hpp"
#include <algorithm>
#include <limits>
#include <string>

namespace cm::aot::finance_checks {
namespace {
constexpr std::uint32_t daily_pass = 0x100fa6e0, season_roll = 0x100fcfd0;
constexpr std::uint32_t club_inactive = 0x100c7d00, weekday_of = 0x10715b20, date_maker = 0x107156f0, next_day = 0x10717890,
                        month_of = 0x10718360, refresh_account = 0x100f4f80,
                        pending_deal = 0x100f9d80, settle_deal = 0x100f7270, unscheduled = 0x10101400,
                        place_club = 0x101030c0, wealth_of = 0x100f55b0, club_of_code = 0x100ce320,
                        list_start = 0x10738c00, list_next = 0x10738e00, list_end = 0x10738c20, list_find = 0x1048b180,
                        list_remove = 0x1048a140, list_insert = 0x104888f0, squad_places = 0x10688e80,
                        find_contract = 0x10082f90, retired = 0x100d3150, dislikes = 0x100d9580, likes = 0x100d9520,
                        mark_sale = 0x1037fa80, club_class = 0x10191fd0;
constexpr std::uint32_t S = scratch_base + 0xa0000;
constexpr std::uint32_t date_at = S, fixed_date_at = S + 0x10, division_table = S + 0x100, rows_at = S + 0x1000,
                        element_a = S + 0x1400, element_b = S + 0x1440, contract_at = S + 0x1480, club_area = S + 0x2000;
constexpr std::uint32_t club_area_size = 0x2000;

struct Memory {
    std::vector<Write> writes;
    void put(std::uint32_t at, std::uint32_t value, unsigned size = 4) { writes.push_back({size, at, value}); }
    void put64(std::uint32_t at, std::uint64_t value) {
        put(at, std::uint32_t(value >> 32));
        put(at + 4, std::uint32_t(value));
    }
};

struct PlayerSpec {
    std::int32_t age = 25, skill = 20, trait_65 = 10, trait_66 = 3, trait_67 = 8, reputed = 3000;
    bool has_player_record = true;
};
struct ClubSpec {
    bool nation = true, division = true, ground = true;
    std::int32_t flag = 1;                 // the nation's byte at +0x120
    std::uint32_t nation_68 = 0x49;        // the game country word at +0x68 (0x49: the real cell's value)
    std::uint32_t division_word = 7;       // against the cell at TOC -0x7a0c (0x165)
    std::int32_t reputation = 6000;
    std::uint32_t status = 1, on_row = 0, flag_241 = 0;
    std::int32_t tier = 5;
    std::uint32_t attendance = 20000;
    std::uint32_t ground_index = 5, ground_money = 30000, flag_4c = 0, flag_4d = 0;
    int assistant = 2;                     // 0: none, 1: a person without staff record, 2: with one
    std::int32_t skill = 10;
    std::int64_t balance = 0;
    std::uint32_t account_class = 1, pending = 1000;
    std::vector<PlayerSpec> players = {};
};
using Answers = std::vector<std::pair<std::uint32_t, std::uint32_t>>;

void answer(Answers& answers, std::uint32_t entry, std::uint32_t value) {
    for (auto& a : answers) if (a.first == entry) { a.second = value; return; }
    answers.emplace_back(entry, value);
}
// Every routine outside the pass answered with its quiet outcome: no club inactive, no weekday 6, no
// deal, no list entry. Two are left to run: a call inside the original's own generated page is not
// dispatched, so an answer would reach the rewrite only (0x100fa1f0 from 0x100fafa4, 0x100fd210 from
// 0x100fd134).
Answers quiet() {
    return {{club_inactive, 0}, {weekday_of, 0}, {refresh_account, 0}, {pending_deal, 0}, {settle_deal, 0},
            {unscheduled, 0}, {place_club, 0}, {wealth_of, 0}, {club_of_code, 0}, {list_start, 0}, {list_find, 0},
            {list_end, 0}, {list_remove, 0}, {list_insert, 0}, {squad_places, 0}, {find_contract, 0}, {retired, 0},
            {dislikes, 0}, {likes, 0}, {mark_sale, 0}, {club_class, 0}};
}

struct Rig {
    const Scenario& sc;
    Memory m;
    Answers answers = quiet();
    std::vector<Case::NthAnswer> nth = {};
    std::vector<Case::Poke> pokes = {};
    std::uint32_t manager = sc.global(-0x7ab8);
    explicit Rig(const Scenario& scenario) : sc(scenario) {}
    std::uint32_t accounts() const { return sc.r32(manager); }
    std::uint32_t account(unsigned index) const { return accounts() + index * 0x168; }
    void date(std::int32_t day, std::uint32_t year, std::uint32_t second = 0) {
        m.put(date_at, (std::uint32_t(day) << 16) | (year & 0xffff));
        m.put(date_at + 4, second);
    }
    void clubs(std::uint32_t count) { m.put(sc.toc(-0x7b50), count); }
    // The next draws' values (each a halfword; the base is 0).
    void draws(const std::vector<std::uint32_t>& values) {
        m.put(sc.toc(-0x7b90), 0);
        m.put(sc.toc(-0x7b84), sc.toc(-0x7b8c));
        for (std::size_t k = 0; k < values.size(); ++k) m.put(sc.toc(-0x7b8c) + 2 + 2 * std::uint32_t(k), values[k], 2);
    }
    // The cursor k halfwords before the end of the table: the (k+1)-th draw is the one that wraps (a generator call).
    void wrap_at(unsigned k) { m.put(sc.toc(-0x7b84), sc.global(-0x7b88) - 2 * k); }
    // Months: the month of tomorrow and of today (the date functions are answered), or the real ones.
    void months(int tomorrow, int today) {
        answer(answers, next_day, 0);
        nth.push_back({month_of, 1, std::uint32_t(tomorrow)});
        nth.push_back({month_of, 2, std::uint32_t(today)});
    }
    void club(unsigned slot, const ClubSpec& s) {
        const auto record = sc.club(slot), base = club_area + slot * club_area_size;
        const auto nation = base, division = base + 0x100, ground = base + 0x200, person = base + 0x300, staff = base + 0x400;
        m.put(record + 0x2c, s.nation ? nation : 0);
        m.put(nation + 0x120, std::uint32_t(s.flag), 1);
        m.put(nation + 0x68, s.nation_68);
        m.put(record + 0x30, s.division ? division : 0);
        m.put(division, s.division_word);
        m.put(record + 0x3c, s.ground ? ground : 0);
        m.put(ground, s.ground_index);
        m.put(ground + 4, s.ground_money);
        m.put(ground + 0x4c, s.flag_4c, 1);
        m.put(ground + 0x4d, s.flag_4d, 1);
        m.put(record + 0x80, s.assistant == 0 ? 0 : person);
        m.put(person + 0x1c, s.assistant == 2 ? staff : 0);
        m.put(staff + 0x2f, std::uint32_t(s.skill), 1);
        m.put(record + 0x1ec, std::uint32_t(s.reputation), 2);
        m.put(record + 0x23d, s.status, 1);
        m.put(record + 0x23e, s.on_row, 1);
        m.put(record + 0x241, s.flag_241, 1);
        m.put(record + 0x240, std::uint32_t(s.tier), 1);
        m.put(record + 8, s.attendance);
        for (unsigned k = 0; k < 50; ++k) {
            if (k >= s.players.size()) { m.put(record + 0x98 + 4 * k, 0); continue; }
            const auto& p = s.players[k];
            const auto at = base + 0x600 + k * 0x100;
            m.put(record + 0x98 + 4 * k, at);
            m.put(at + 0x14, p.has_player_record ? at + 0x80 : 0);
            m.put(at + 0x80 + 0xa, std::uint32_t(p.reputed), 2);
            m.put(at + 0x5d, std::uint32_t(p.age), 1);
            m.put(at + 0x63, std::uint32_t(p.skill), 1);
            m.put(at + 0x65, std::uint32_t(p.trait_65), 1);
            m.put(at + 0x66, std::uint32_t(p.trait_66), 1);
            m.put(at + 0x67, std::uint32_t(p.trait_67), 1);
        }
        const auto acct = account(slot);
        m.put64(acct, std::uint64_t(s.balance));
        m.put(acct + 0x165, s.account_class, 1);
        m.put(acct + 0xc, s.pending);
    }
    Case finish(std::uint32_t entry, const std::string& label, std::uint32_t second = date_at) {
        Case item;
        item.entry = entry;
        item.registers = {{3, manager}, {4, second}};
        item.memory = m.writes;
        item.label = label;
        item.answers = answers;
        item.nth_answers = nth;
        item.pokes = pokes;
        return item;
    }
};
}

FINANCE_CASES("daily") {
    if (scenario.match) return;
    const auto first_day = scenario.toc(-0x7a9c);
    const auto fixed_date = fixed_date_at;
    const auto add = [&](Rig& rig, const std::string& label) { cases.push_back(rig.finish(daily_pass, label)); };

    // ---- the gate: the game's first day (day 354 of 2001 in the snapshot) against the date ----
    {
        struct Gate { std::uint32_t first_word; std::int32_t day; std::uint32_t year; };
        const Gate gates[] = {{(354u << 16) | 2001, 354, 2001}, {(354u << 16) | 2001, 353, 2001}, {(354u << 16) | 2001, 355, 2001},
                              {(354u << 16) | 2001, 354, 2000}, {(354u << 16) | 2001, 354, 2002}, {(354u << 16) | 2001, 100, 2002},
                              {(0xffffu << 16) | 2001, 0, 2001}, {(0u << 16) | 2001, -1, 2001}, {(0x7fffu << 16) | 2001, -32768, 2001},
                              {(0x8000u << 16) | 2001, 32767, 2001}, {(0x8000u << 16) | 2001, -32768, 2001}, {(0x7fffu << 16) | 2001, 32767, 2001},
                              {(1u << 16) | 0xffff, 5, 0xffff}, {(1u << 16) | 0, 5, 0}};
        for (const auto& g : gates) {
            Rig rig(scenario);
            rig.date(g.day, g.year);
            rig.clubs(0);
            rig.m.put(first_day, g.first_word);
            add(rig, "gate first " + std::to_string(g.first_word >> 16) + "/" + std::to_string(g.first_word & 0xffff) + " date " +
                         std::to_string(g.day) + "/" + std::to_string(g.year));
        }
        // The real date functions on real dates: an ordinary day and the last day of the year (months differ).
        for (const std::int32_t day : {360, 365}) {
            Rig rig(scenario);
            rig.date(day, 2001);
            rig.clubs(2);
            add(rig, "real date functions, day " + std::to_string(day));
        }
    }

    // ---- the fixed day: two clubs move onto rows named by the pool's texts ----
    {
        const auto pool = scenario.toc(-0x5598);
        const auto name1_a = scenario.r32(pool + 0x162), name1_b = scenario.r32(pool + 0x166) >> 16;
        const auto name2_a = scenario.r32(pool + 0x169), name2_b = scenario.r32(pool + 0x16d);
        struct Fixed {
            bool same_day = true;
            std::uint32_t first = 3, second = 4;          // the clubs' indices (the cells at TOC -0x74a8, -0x7494)
            std::uint32_t rep1 = 800, rep2 = 800;
            std::uint32_t attendance1 = 100, attendance2 = 100;
            bool row1_named = true, row2_named = true;
            std::uint32_t ground1 = 99, ground2 = 99;     // the clubs' current ground index (a row's number match ends the sweep)
            std::int32_t row_money2 = -20000, new_row_money = 5000, old_row_money = 1234;
            std::uint32_t walk = 0;                       // 0: no entry, 1: a matching entry, 2: a foreign one, then a matching one
        };
        const auto run = [&](const Fixed& f, const std::string& label) {
            Rig rig(scenario);
            rig.date(360, 2001);
            rig.clubs(2);
            rig.m.put(scenario.toc(-0x7568), 3);
            rig.m.put(scenario.toc(-0x7548), rows_at);
            for (unsigned r = 0; r < 3; ++r) {
                rig.m.put(rows_at + r * 0x50, r);
                rig.m.put(rows_at + r * 0x50 + 0x18, 0, 4);
                rig.m.put(rows_at + r * 0x50 + 0x1c, 0, 4);
                rig.m.put(rows_at + r * 0x50 + 0x20, 0, 4);
                rig.m.put(rows_at + r * 0x50 + 4, std::uint32_t(r == 2 ? f.row_money2 : (r == 1 ? f.new_row_money : 7)));
            }
            if (f.row1_named) { rig.m.put(rows_at + 1 * 0x50 + 0x18, name1_a); rig.m.put(rows_at + 1 * 0x50 + 0x1c, name1_b, 2); }
            if (f.row2_named) { rig.m.put(rows_at + 2 * 0x50 + 0x18, name2_a); rig.m.put(rows_at + 2 * 0x50 + 0x1c, name2_b); }
            rig.m.put(scenario.toc(-0x74a8), f.first);
            rig.m.put(scenario.toc(-0x7494), f.second);
            rig.m.put(fixed_date, f.same_day ? (360u << 16 | 2001u) : 0x12340001u);
            answer(rig.answers, date_maker, fixed_date);
            ClubSpec one, two;
            one.reputation = std::int32_t(f.rep1); one.attendance = f.attendance1; one.ground_index = f.ground1;
            two.reputation = std::int32_t(f.rep2); two.attendance = f.attendance2; two.ground_index = f.ground2;
            // The clubs 3 and 4 are named by the cells; their records take the same scratch shape (slots 0 and 1 of the
            // area are used by the sweep over the first two clubs).
            const auto first_record = scenario.club(f.first), second_record = scenario.club(f.second);
            const auto base1 = club_area + 0x6000, base2 = club_area + 0x7000;
            rig.m.put(first_record + 0x3c, base1); rig.m.put(base1, f.ground1); rig.m.put(base1 + 4, 77);
            rig.m.put(second_record + 0x3c, base2); rig.m.put(base2, f.ground2); rig.m.put(base2 + 4, std::uint32_t(f.old_row_money));
            rig.m.put(first_record + 0x1ec, f.rep1, 2); rig.m.put(second_record + 0x1ec, f.rep2, 2);
            rig.m.put(first_record + 8, f.attendance1); rig.m.put(second_record + 8, f.attendance2);
            if (f.walk) {
                // walk 1, 2: the first club's list; 3: the second club's list only; 4: both lists, a foreign entry
                // then the club's entry in each. (Dispatches of the list-next call count 1, 2, ... over the pass.)
                const auto foreign_a = S + 0x14c0, club_c = S + 0x1500, foreign_b = S + 0x1540;
                rig.m.put(foreign_a + 0x1c, second_record + 0x10);
                rig.m.put(club_c + 0x1c, second_record);
                rig.m.put(foreign_b + 0x1c, first_record + 0x20);
                rig.m.put(element_a + 0x1c, f.walk == 2 ? first_record + 0x10 : first_record);
                rig.m.put(element_b + 0x1c, first_record);
                std::vector<std::uint32_t> sequence;
                if (f.walk == 1) sequence = {element_a, 0, 0};
                else if (f.walk == 2) sequence = {element_a, element_b, 0, 0};
                else if (f.walk == 3) sequence = {0, foreign_a, club_c, 0};
                else sequence = {foreign_b, element_b, 0, foreign_a, club_c, 0};
                for (std::size_t n = 0; n < sequence.size(); ++n) rig.nth.push_back({list_next, unsigned(n + 1), sequence[n]});
            }
            add(rig, label);
        };
        run({.same_day = false}, "fixed day not today");
        run({.first = 0xffffffff}, "fixed day, first club unnamed");
        run({.second = 0xffffffff}, "fixed day, second club unnamed");
        // The first club's row: no named row, the club already on it, tiers by reputation and floors of the attendance.
        run({.row1_named = false, .row2_named = false}, "fixed day, no named rows");
        run({.ground1 = 1, .ground2 = 2}, "fixed day, clubs already on their rows");
        for (const std::uint32_t rep : {0u, 1500u, 1501u, 3000u, 3001u, 32767u}) {
            for (const std::uint32_t attendance : {100u, 5000u, 20000u, 0x7fffffffu, 0x80000000u})
                run({.rep1 = rep, .rep2 = rep, .attendance1 = attendance, .attendance2 = attendance}, "fixed day, reputation " + std::to_string(rep) + " attendance " + std::to_string(attendance));
        }
        for (const std::uint32_t rep : {0u, 5000u, 5001u, 5250u, 5251u, 6000u, 6750u, 6751u, 6788u, 6789u, 9000u})
            run({.rep1 = rep, .rep2 = rep}, "fixed day, second club reputation " + std::to_string(rep));
        for (const std::int32_t money : {-20000, -10001, -10000, 0, 5000})
            run({.row_money2 = money}, "fixed day, row set aside money " + std::to_string(money));
        for (const std::int32_t money : {-5000, 0, 5000, std::numeric_limits<std::int32_t>::min()})
            run({.new_row_money = money}, "fixed day, first row money " + std::to_string(money));
        run({.walk = 1}, "fixed day, a list entry of the club");
        run({.walk = 2}, "fixed day, a foreign entry then the club's");
        run({.walk = 1, .rep1 = 4000, .rep2 = 9000}, "fixed day, entries, other tiers");
        run({.walk = 3}, "fixed day, the second club's list: a foreign entry then its own");
        run({.walk = 4}, "fixed day, both lists: a foreign entry then the club's in each");
        run({.walk = 3, .rep2 = 6000}, "fixed day, the second club's list, middle tier");
    }

    // ---- the sweep: one club per case at slot 0 (clubs 1 and 2 follow as plain ones), then an ordinary day ----
    {
        const auto sweep = [&](const ClubSpec& spec, const std::string& label, std::uint32_t weekday = 0,
                               const std::vector<std::uint32_t>& draws = {}) {
            Rig rig(scenario);
            rig.date(360, 2001);
            rig.clubs(2);
            rig.months(4, 4);
            ClubSpec other;
            other.division = false;
            other.nation = false;
            rig.club(0, spec);
            rig.club(1, other);
            answer(rig.answers, weekday_of, weekday);
            rig.draws(draws.empty() ? std::vector<std::uint32_t>{0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12} : draws);
            add(rig, label);
        };
        const std::int64_t lows[] = {-3'000'000, -2'250'001, -2'250'000, -100, 0, 100, 2'250'000, 2'250'001, 3'000'000,
                                     -300'000, -250'001, -250'000, 250'000, 250'001, -25'001, -25'000, 25'000, 25'001,
                                     -10'001, -10'000, 10'000, 10'001, std::int64_t(1) << 40, -(std::int64_t(1) << 40)};
        for (const std::uint32_t status : {3u, 2u, 1u, 0u, 4u, 255u})
            for (const std::int32_t rep : {100, 2500, 2501, 4999})
                for (const auto balance : lows) {
                    ClubSpec spec;
                    spec.flag = 0;                 // reputation below 5000 confines
                    spec.status = status;
                    spec.reputation = rep;
                    spec.balance = balance;
                    sweep(spec, "sweep status " + std::to_string(status) + " rep " + std::to_string(rep) + " balance " + std::to_string(balance));
                }
        // The drift: one time in five the correction flips (sample % 5 == 0).
        for (const std::int64_t balance : {std::int64_t(-3'000'000), std::int64_t(3'000'000)})
            for (const std::uint32_t sample : {0u, 1u, 5u, 7u, 10u}) {
                ClubSpec spec;
                spec.flag = 0;
                spec.status = 1;
                spec.reputation = 100;
                spec.balance = balance;
                sweep(spec, "drift balance " + std::to_string(balance) + " sample " + std::to_string(sample), 0, {sample});
            }
        for (unsigned k = 0; k < 3; ++k)
            for (const std::int64_t balance : {std::int64_t(-3'000'000), std::int64_t(3'000'000)}) {
                ClubSpec spec;
                spec.flag = 0;
                spec.status = 1;
                spec.reputation = 100;
                spec.balance = balance;
                Rig rig(scenario);
                rig.date(360, 2001);
                rig.clubs(1);
                rig.months(4, 4);
                rig.club(0, spec);
                rig.draws({0, 1, 2});
                rig.wrap_at(k);
                add(rig, "drift, wrap at draw " + std::to_string(k) + " balance " + std::to_string(balance));
            }
        // Which clubs are confined: the division word, the nation's flag, reputation 5000.
        for (const bool division : {false, true})
            for (const std::int32_t flag : {0, 1, -1})
                for (const std::uint32_t word : {7u, 0x165u})
                    for (const std::int32_t rep : {4999, 5000, 6000}) {
                        ClubSpec spec;
                        spec.division = division;
                        spec.flag = flag;
                        spec.division_word = word;
                        spec.reputation = rep;
                        spec.status = 3;
                        spec.balance = 500'000;
                        sweep(spec, "which clubs division " + std::to_string(division) + " flag " + std::to_string(flag) + " word " + std::to_string(word) + " rep " + std::to_string(rep));
                    }
        // Saturday: the pending charge, the deal outcome (the byte at frame +0x41 is the callee's), and the other weekdays.
        const auto frame_flag = std::uint32_t(scenario.s.r<4>(4)) - 0x120 + 0x41;
        for (const std::uint32_t weekday : {0u, 5u, 6u, 7u, 0x10006u, 0xffffu})
            for (const std::uint32_t deal : {0u, 1u})
                for (const std::uint32_t pending : {0u, 1000u, 0x80000000u, 0xffffffffu})
                    for (const std::int64_t balance : {std::int64_t(0), std::int64_t(100), std::int64_t(-1)}) {
                        ClubSpec spec;
                        spec.flag = 1;
                        spec.reputation = 6000;
                        spec.pending = pending;
                        spec.balance = balance;
                        Rig rig(scenario);
                        rig.date(360, 2001);
                        rig.clubs(1);
                        rig.months(4, 4);
                        rig.club(0, spec);
                        answer(rig.answers, weekday_of, weekday);
                        answer(rig.answers, pending_deal, deal);
                        rig.pokes.push_back({pending_deal, {{1, frame_flag, 9}}});
                        add(rig, "saturday weekday " + std::to_string(weekday) + " deal " + std::to_string(deal) + " pending " + std::to_string(pending) + " balance " + std::to_string(balance));
                    }
        {
            // The nation vanishes while the weekday routine runs: the pass goes on to the next club.
            ClubSpec spec;
            Rig rig(scenario);
            rig.date(360, 2001);
            rig.clubs(1);
            rig.months(4, 4);
            rig.club(0, spec);
            answer(rig.answers, weekday_of, 6);
            rig.pokes.push_back({weekday_of, {{4, scenario.club(0) + 0x2c, 0}}});
            add(rig, "saturday, the nation goes away");
        }
        // The inactive club ends the sweep (and, on an ordinary day, the pass).
        for (const unsigned nth : {1u, 2u}) {
            Rig rig(scenario);
            rig.date(360, 2001);
            rig.clubs(3);
            rig.months(4, 4);
            ClubSpec spec;
            spec.flag = 0;
            spec.reputation = 100;
            spec.status = 3;
            spec.balance = 99999;
            for (unsigned slot = 0; slot < 3; ++slot) rig.club(slot, spec);
            rig.nth.push_back({club_inactive, nth, 1});
            add(rig, "inactive club number " + std::to_string(nth));
        }
        // No nation: the club is skipped without a test.
        {
            ClubSpec spec;
            spec.nation = false;
            sweep(spec, "sweep, club without nation");
        }
    }

    // ---- an ordinary day: the unscheduled events ----
    {
        struct Day {
            std::int32_t reputation = 6000, flag = 0, account_class = 1;
            std::uint32_t division_word = 3;     // the division's number: an index into the table at TOC -0x7b5c
            bool table = true;
            std::uint32_t entry = 0, count = 20;
            std::vector<std::uint32_t> draws = {0};
            int wrap = -1;
        };
        const auto run = [&](const Day& d, const std::string& label) {
            Rig rig(scenario);
            rig.date(360, 2001);
            rig.clubs(2);
            rig.months(4, 4);
            ClubSpec spec;
            spec.reputation = d.reputation;
            spec.flag = d.flag;
            spec.account_class = std::uint32_t(d.account_class);
            spec.division_word = d.division_word;
            spec.status = 3;
            spec.balance = 100;  // the sweep leaves a small balance alone
            rig.club(0, spec);
            ClubSpec second = spec;
            second.division_word = 5;
            second.reputation = 6000;
            rig.club(1, second);
            rig.m.put(scenario.toc(-0x7b5c), d.table ? division_table : 0);
            rig.m.put(scenario.toc(-0x7b64), d.count);
            for (unsigned n = 0; n < 24; ++n) rig.m.put(division_table + 4 * n, 0x30000000 + n * 0x100);
            if (d.division_word < 24) rig.m.put(division_table + 4 * d.division_word, d.entry);
            rig.draws(d.draws);
            if (d.wrap >= 0) rig.wrap_at(unsigned(d.wrap));
            add(rig, label);
        };
        run({}, "ordinary day, table entry empty");
        run({.entry = 0x30000100}, "ordinary day, table entry present");
        for (int k = 0; k < 2; ++k) run({.entry = 0x30000100, .wrap = k}, "ordinary day, wrap at draw " + std::to_string(k));
        run({.table = false}, "ordinary day, no table");
        for (const std::uint32_t word : {0u, 1u, 18u, 19u, 20u, 23u})
            run({.division_word = word}, "ordinary day, division number " + std::to_string(word));
        for (const std::int32_t rep : {5000, 4249, 4250, 4251, 20000, -32768, 32767, 0})
            run({.reputation = rep, .flag = rep >= 5000 ? 0 : 1, .division_word = 3}, "ordinary day, reputation " + std::to_string(rep));
        for (const std::int32_t account_class : {0, 1, 2, 3, -1})
            run({.account_class = account_class}, "ordinary day, account class " + std::to_string(account_class));
        for (const std::int32_t rep : {-32768, -20000, 0, 4000, 9000, 13000, 20000, 32767})
            for (const std::uint32_t draw : {0u, 1u, 7u, 100u, 65535u})
                run({.reputation = rep, .flag = rep >= 5000 ? 0 : 1, .entry = 0x30000100, .draws = {draw}},
                    "ordinary day, reputation " + std::to_string(rep) + " draw " + std::to_string(draw));
    }

    // ---- the last day of a month: one club at slot 0, the second skipped (no nation) ----
    {
        const auto frame_base = std::uint32_t(scenario.s.r<4>(4)) - 0x120;
        const std::vector<std::uint32_t> zeros(40, 0), sevens(40, 7), ramp = [] {
            std::vector<std::uint32_t> v;
            for (std::uint32_t n = 1; n <= 40; ++n) v.push_back(n * 37);
            return v;
        }();
        struct Month {
            ClubSpec club = {};
            int wealth = 0;                         // the band the wealth routine answers
            std::uint32_t stands_for = 0, stands_byte = 1;
            std::uint32_t contract = 0, contract_kind = 1;
            std::uint32_t retired = 0, dislikes = 0, likes = 0;
            const std::vector<std::uint32_t>* draws = nullptr;
            std::vector<std::uint32_t> own_draws = {};
            std::uint32_t entry = 0x30000000;        // the division table's entry for the club's division
            bool table = true;
            int wrap = -1;                           // >= 0: the draw that wraps the sample table
            bool nation_goes = false;                // the wealth routine's dispatch clears the club's nation
        };
        const auto run = [&](Month mo, const std::string& label) {
            Rig rig(scenario);
            rig.date(365, 2001);
            rig.clubs(2);
            rig.months(5, 4);
            rig.club(0, mo.club);
            ClubSpec skipped;
            skipped.nation = false;
            rig.club(1, skipped);
            answer(rig.answers, wealth_of, std::uint32_t(mo.wealth));
            answer(rig.answers, club_of_code, mo.stands_for);
            rig.pokes.push_back({club_of_code, {{1, frame_base + 0x40, mo.stands_byte}}});
            answer(rig.answers, find_contract, mo.contract ? contract_at : 0);
            rig.m.put(contract_at + 0x60, 0x50 | mo.contract_kind, 1);
            answer(rig.answers, retired, mo.retired);
            answer(rig.answers, dislikes, mo.dislikes);
            answer(rig.answers, likes, mo.likes);
            rig.m.put(scenario.toc(-0x7b5c), mo.table ? division_table : 0);
            rig.m.put(scenario.toc(-0x7b64), 20);
            for (unsigned n = 0; n < 24; ++n) rig.m.put(division_table + 4 * n, 0x30000000);
            rig.m.put(division_table + 4 * mo.club.division_word, mo.entry);
            rig.draws(!mo.own_draws.empty() ? mo.own_draws : mo.draws ? *mo.draws : zeros);
            if (mo.wrap >= 0) rig.wrap_at(unsigned(mo.wrap));
            if (mo.nation_goes) rig.pokes.push_back({wealth_of, {{4, scenario.club(0) + 0x2c, 0}}});
            add(rig, label);
        };
        const auto plain = [&] {
            Month mo;
            mo.club.flag = 1;
            mo.club.division_word = 3;
            mo.club.balance = 1000;
            return mo;
        };
        run(plain(), "month, plain club");
        for (const auto* draws : {&zeros, &sevens, &ramp}) {
            auto mo = plain();
            mo.draws = draws;
            run(mo, std::string("month, plain club draws ") + (draws == &zeros ? "zeros" : draws == &sevens ? "sevens" : "ramp"));
        }
        // The sample table wraps at each draw of the month: the redraw is a generator call between the pass's own steps.
        for (unsigned k = 0; k < 10; ++k) {
            for (const int assistant : {0, 2}) {
                auto mo = plain();
                mo.club.assistant = assistant; mo.wrap = int(k); mo.draws = &ramp;
                run(mo, "month, wrap at draw " + std::to_string(k) + " assistant " + std::to_string(assistant));
            }
            auto mo = plain();
            mo.wealth = -2; mo.club.reputation = 6000; mo.club.players = {PlayerSpec{}, PlayerSpec{.age = 19}};
            mo.contract = 1; mo.wrap = int(k);
            mo.own_draws = {800, 0, 4, 1, 3, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12};
            run(mo, "month, wrap at draw " + std::to_string(k) + " in trouble");
        }
        // Which clubs take part, and a record standing for another club.
        for (const bool division : {false, true})
            for (const bool nation : {false, true})
                for (const std::int32_t flag : {0, 1})
                    for (const std::int32_t rep : {4999, 5000})
                        for (const std::uint32_t word : {3u, 0x165u}) {
                            auto mo = plain();
                            mo.club.division = division; mo.club.nation = nation; mo.club.flag = flag; mo.club.reputation = rep;
                            mo.club.division_word = word;
                            run(mo, "month, takes part " + std::to_string(division) + std::to_string(nation) + std::to_string(flag) + " rep " + std::to_string(rep) + " word " + std::to_string(word));
                        }
        for (const std::uint32_t stands : {0u, 0x22299360u})
            for (const std::uint32_t byte : {0u, 1u, 0x80u}) {
                auto mo = plain();
                mo.stands_for = stands;
                mo.stands_byte = byte;
                run(mo, "month, stands for " + std::to_string(stands) + " byte " + std::to_string(byte));
            }
        // A club in trouble: the chance and the squad. Draws: the chance, the test draw, then per player.
        for (const std::int32_t rep : {0, 1500, -1500, 32767, -32768, 7000}) {
            for (const std::uint32_t chance : {0u, 899u, 900u, 1u, 30000u}) {
                for (const std::uint32_t test : {0u, 5u, 899u, 900u, 19999u}) {
                    auto mo = plain();
                    mo.wealth = -2;
                    mo.club.reputation = rep;
                    mo.club.flag = 1;
                    mo.club.players = {PlayerSpec{}, PlayerSpec{.age = 19}};
                    mo.contract = 1;
                    mo.own_draws = {chance, test, 3, 0, 5, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12};
                    run(mo, "month, trouble reputation " + std::to_string(rep) + " chance " + std::to_string(chance) + " test " + std::to_string(test));
                }
            }
        }
        // The squad players: age bands, contract kinds, the player's standing, the manager's feelings.
        const std::int32_t ages[] = {19, 20, 21, 23, 24, 30, 32, 33, 40, -5};
        const std::int32_t reputeds[] = {0, 0xcb2, 0xcb3, 0x128d, 0x128e, 3000, 6000, 0x1676, 0x1677, 9000};
        for (const auto age : ages)
            for (const std::uint32_t kind : {0u, 1u, 2u, 3u, 7u, 15u})
                for (const auto reputed : reputeds) {
                    auto mo = plain();
                    mo.wealth = -2;
                    mo.club.reputation = 6000;
                    mo.club.players = {PlayerSpec{.age = age, .skill = 20, .reputed = reputed}};
                    mo.contract = 1; mo.contract_kind = kind;
                    mo.own_draws = {800, 0, 4, 1, 3, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
                    run(mo, "month, player age " + std::to_string(age) + " kind " + std::to_string(kind) + " reputed " + std::to_string(reputed));
                }
        for (const std::int32_t skill : {0, 10, 15, 16, 30, -20, 127})
            for (const std::int32_t t65 : {-20, 0, 10, 40, 127})
                for (const std::int32_t t66 : {-5, 0, 3, 5, 10, 30})
                    for (const std::uint32_t feelings : {0u, 1u, 2u, 3u}) {
                        auto mo = plain();
                        mo.wealth = -2;
                        mo.club.reputation = 6000;
                        mo.club.players = {PlayerSpec{.age = 27, .skill = skill, .trait_65 = t65, .trait_66 = t66, .trait_67 = 9, .reputed = 6000}};
                        mo.contract = 1; mo.dislikes = feelings & 1; mo.likes = feelings >> 1;
                        mo.own_draws = {800, 0, 5, 6, 3, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
                        run(mo, "month, player skill " + std::to_string(skill) + " traits " + std::to_string(t65) + "/" + std::to_string(t66) + " feelings " + std::to_string(feelings));
                    }
        for (const std::uint32_t reputed_case : {0u, 1u})
            for (const std::int32_t skill : {5, 20}) {
                auto mo = plain();
                mo.wealth = -2;
                mo.club.reputation = 6000;
                mo.club.players = {PlayerSpec{.age = 27, .skill = skill, .reputed = reputed_case ? 0x2000 : 100}, PlayerSpec{.has_player_record = false}};
                mo.club.players.push_back(PlayerSpec{});
                mo.contract = 1;
                mo.own_draws = {800, 0, 6, 10, 3, 4, 7, 12, 3, 2, 1, 5, 9, 8, 11, 0};
                run(mo, "month, squad of three " + std::to_string(reputed_case) + "/" + std::to_string(skill));
            }
        for (const std::uint32_t retired_case : {0u, 1u})
            for (const std::uint32_t contract : {0u, 1u}) {
                auto mo = plain();
                mo.wealth = -2;
                mo.club.reputation = 6000;
                mo.club.players = {PlayerSpec{}, PlayerSpec{}};
                mo.contract = contract; mo.retired = retired_case;
                mo.own_draws = {800, 0, 3, 5, 6, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11};
                run(mo, "month, retired " + std::to_string(retired_case) + " contract " + std::to_string(contract));
            }
        for (const std::int32_t wealth : {-1, 0, 1, 2, -3, 127, 254})
            { auto mo = plain(); mo.wealth = wealth; run(mo, "month, wealth " + std::to_string(wealth)); }
        // Costs: the ground's flags, the tier, the division table.
        for (const bool ground : {false, true})
            for (const std::uint32_t f4c : {0u, 1u, 0x80u})
                for (const std::uint32_t f4d : {0u, 1u})
                    for (const std::int32_t rep : {4500, 4501, 8000}) {
                        auto mo = plain();
                        mo.club.ground = ground; mo.club.flag_4c = f4c; mo.club.flag_4d = f4d; mo.club.reputation = rep;
                        run(mo, "month, ground " + std::to_string(ground) + " flags " + std::to_string(f4c) + std::to_string(f4d) + " rep " + std::to_string(rep));
                    }
        for (const std::int32_t tier : {-128, -1, 0, 3, 5, 6, 7, 15, 16, 19, 20, 21, 30, 127})
            for (const std::int32_t rep : {0x1869, 0x186a, 20000}) {
                auto mo = plain();
                mo.club.tier = tier; mo.club.reputation = rep;
                run(mo, "month, tier " + std::to_string(tier) + " rep " + std::to_string(rep));
            }
        // (A null table cell faults on both sides at address 0xc: the game never has one.)
        for (const bool table : {true})
            for (const std::uint32_t entry : {0u, 0x30000000u})
                for (const std::uint32_t attendance : {0u, 1u, 30000u, 0x80000000u, 0xffffffffu})
                    for (const std::int32_t rep : {-32768, -1, 0, 7000, 32767}) {
                        auto mo = plain();
                        mo.table = table; mo.entry = entry; mo.club.attendance = attendance; mo.club.reputation = rep;
                        mo.club.flag = 1;
                        run(mo, "month, table " + std::to_string(table) + " entry " + std::to_string(entry) + " attendance " + std::to_string(attendance) + " rep " + std::to_string(rep));
                    }
        // Interest on the balance, by its depth against the reputation.
        for (const std::int32_t rep : {-1000, 0, 1, 6000, 32767, -32768})
            for (const std::int64_t balance : {std::int64_t(1), std::int64_t(0), std::int64_t(-1), std::int64_t(-5'999'999), std::int64_t(-6'000'000),
                                                std::int64_t(-6'000'001), std::int64_t(-14'999'999), std::int64_t(-15'000'000), std::int64_t(-15'000'001),
                                                std::int64_t(-40'000'000), std::int64_t(40'000'000), std::int64_t(1) << 40, -(std::int64_t(1) << 40), 2'000'000'000LL, -2'000'000'000LL}) {
                auto mo = plain();
                mo.club.reputation = rep; mo.club.flag = 1; mo.club.balance = balance;
                run(mo, "month, interest rep " + std::to_string(rep) + " balance " + std::to_string(balance));
            }
        // The assistant: none, no staff record, skills including the ones that make a limit 0; draws by presets.
        for (const int assistant : {0, 1, 2})
            for (const std::int32_t skill : {0, 10, 31, 30, 32, -1, -2, 127, -128, 5})
                for (const auto* draws : {&zeros, &sevens, &ramp}) {
                    auto mo = plain();
                    mo.club.assistant = assistant; mo.club.skill = skill; mo.draws = draws;
                    run(mo, "month, assistant " + std::to_string(assistant) + " skill " + std::to_string(skill) + (draws == &zeros ? " zeros" : draws == &sevens ? " sevens" : " ramp"));
                }
        // The earnings by status, reputation and flag (+0x241); the draws.
        for (const std::uint32_t status : {1u, 0u, 3u})
            for (const std::int32_t rep : {-300, 0, 3000, 4999, 5000, 5001, 8000})
                for (const std::uint32_t flag : {0u, 1u})
                    for (const auto* draws : {&zeros, &ramp}) {
                        auto mo = plain();
                        mo.club.status = status; mo.club.reputation = rep; mo.club.flag_241 = flag; mo.draws = draws;
                        mo.club.flag = rep >= 5000 ? 0 : 1;
                        run(mo, "month, earnings status " + std::to_string(status) + " rep " + std::to_string(rep) + " flag " + std::to_string(flag) + (draws == &zeros ? " zeros" : " ramp"));
                    }
        for (const std::int32_t skill : {0, 1, 10, -3})
            for (const std::uint32_t status : {1u, 3u}) {
                auto mo = plain();
                mo.club.skill = skill; mo.club.status = status; mo.club.flag_241 = 1;
                run(mo, "month, earnings skill " + std::to_string(skill) + " status " + std::to_string(status));
            }
        // The running cost: the ground row, the account's class, the money ladder, the nation, the reputation ladder.
        for (const std::uint32_t status : {1u, 0u})
            for (const std::uint32_t on_row : {0u, 1u})
                for (const bool ground : {false, true})
                    for (const std::uint32_t account_class : {0u, 1u, 2u, 255u})
                        for (const std::uint32_t money : {0u, 5000u, 10000u, 10001u, 21000u, 21001u, 27500u, 27501u, 50000u, 50001u, 70000u, 70001u, 100000u, 0x80000000u, 0xfffff000u, 750u, 751u, 2000u}) {
                            auto mo = plain();
                            mo.club.status = status; mo.club.on_row = on_row; mo.club.ground = ground;
                            mo.club.account_class = account_class; mo.club.ground_money = money;
                            run(mo, "month, running status " + std::to_string(status) + " row " + std::to_string(on_row) + " ground " + std::to_string(ground) +
                                        " class " + std::to_string(account_class) + " money " + std::to_string(money));
                        }
        for (const std::uint32_t nation_68 : {0x49u, 0x4au, 0u})
            for (const std::int32_t rep : {0x4e2, 0x4e3, 0x109a, 0x109b, 0x1676, 0x1677, 0x1c52, 0x1c53, 20000, 5000})
                for (const bool has_nation_effect : {false, true}) {
                    auto mo = plain();
                    mo.club.status = 1; mo.club.on_row = 1; mo.club.nation_68 = nation_68; mo.club.reputation = rep;
                    mo.club.flag = rep >= 5000 || has_nation_effect ? 1 : 1;
                    mo.club.ground_money = 30000;
                    mo.club.account_class = has_nation_effect ? 0 : 2;
                    run(mo, "month, ladder nation " + std::to_string(nation_68) + " rep " + std::to_string(rep) + " " + std::to_string(has_nation_effect));
                }
        // The nation goes away between the club test and the running cost's nation test (0x100fc7e8).
        for (const std::uint32_t account_class : {0u, 2u}) {
            auto mo = plain();
            mo.club.status = 1; mo.club.on_row = 1; mo.club.account_class = account_class; mo.club.ground_money = 30000;
            mo.nation_goes = true;
            run(mo, "month, the nation goes away, class " + std::to_string(account_class));
        }
        for (const std::uint32_t account_class : {1u, 0u})
            for (const std::uint32_t money : {0u, 100u, 750u, 751u, 5000u, 1000000u, 0x7fffffffu}) {
                auto mo = plain();
                mo.club.status = 0; mo.club.on_row = 1; mo.club.account_class = account_class; mo.club.ground_money = money;
                run(mo, "month, running other class " + std::to_string(account_class) + " money " + std::to_string(money));
            }
    }

    // ---- the season roll ----
    {
        const auto types = scenario.toc(-0x7b44);
        const auto roll = [&](std::uint32_t slot, std::uint32_t type, const ClubSpec& spec, const std::string& label,
                              unsigned inactive_nth = 0) {
            Rig rig(scenario);
            rig.clubs(2);
            rig.club(0, spec);
            ClubSpec second = spec;
            second.reputation = 6000;
            second.flag = 1;
            rig.club(1, second);
            answer(rig.answers, club_class, type);
            if (inactive_nth) rig.nth.push_back({club_inactive, inactive_nth, 1});
            cases.push_back(rig.finish(season_roll, label, slot));
        };
        ClubSpec plain_club;
        plain_club.flag = 1;
        roll(0, 0, plain_club, "season roll, no slot");
        for (const std::uint32_t type : {0u, 1u, 3u, 0xffu, 0x80u}) {
            roll(types + std::uint32_t(std::int32_t(std::int8_t(type))) * 0x4c, type, plain_club, "season roll, matching slot, type " + std::to_string(type));
            roll(types + std::uint32_t(std::int32_t(std::int8_t(type))) * 0x4c + 4, type, plain_club, "season roll, other slot, type " + std::to_string(type));
        }
        for (const bool division : {false, true})
            for (const bool nation : {false, true})
                for (const std::int32_t flag : {0, 1})
                    for (const std::int32_t rep : {4999, 5000}) {
                        ClubSpec spec;
                        spec.division = division; spec.nation = nation; spec.flag = flag; spec.reputation = rep;
                        roll(types, 0, spec, "season roll, club " + std::to_string(division) + std::to_string(nation) + std::to_string(flag) + " rep " + std::to_string(rep));
                    }
        for (const unsigned nth : {1u, 2u}) roll(types, 0, plain_club, "season roll, inactive club " + std::to_string(nth), nth);
        // The game's first day after today: the pass is skipped.
        for (const std::uint32_t today : {353u, 354u, 355u}) {
            Rig rig(scenario);
            rig.clubs(2);
            rig.club(0, plain_club);
            rig.m.put(scenario.toc(-0x7b80), (today << 16) | 2001);
            answer(rig.answers, club_class, 0);
            cases.push_back(rig.finish(season_roll, "season roll, today " + std::to_string(today), types));
        }
    }

    // ---- whole passes over every club of the career, with the real routines ----
    {
        Rig rig(scenario);
        rig.answers.clear();
        cases.push_back(rig.finish(daily_pass, "whole pass, today", scenario.toc(-0x7b80)));
        Rig month(scenario);
        month.answers.clear();
        month.date(365, 2001);
        cases.push_back(month.finish(daily_pass, "whole pass, last day of the year"));
        Rig roll(scenario);
        roll.answers.clear();
        cases.push_back(roll.finish(season_roll, "whole season roll", scenario.toc(-0x7b44)));
    }

    // The cases that reach the rarest outcomes run first, so a run cut short (a slow instrumented build) still
    // reaches the list relinking, the attendance tiers, the Saturday deal, the squad-sale valuation, the negative
    // draw limit and the null nation. Exact labels from the families above.
    {
        static const char* const first[] = {
            "fixed day, a list entry of the club", "fixed day, a foreign entry then the club's", "fixed day, the second club's list: a foreign entry then its own", "fixed day, both lists: a foreign entry then the club's in each", "fixed day, the second club's list, middle tier", "fixed day, entries, other tiers",
            "fixed day, reputation 32767 attendance 100", "fixed day, reputation 3001 attendance 100", "fixed day, reputation 0 attendance 100",
            "fixed day, second club reputation 6000", "fixed day, second club reputation 5251", "fixed day, second club reputation 6789",
            "fixed day, second club reputation 5000",
            "saturday weekday 6 deal 1 pending 1000 balance 0", "saturday weekday 6 deal 0 pending 1000 balance 0",
            "saturday, the nation goes away", "month, the nation goes away, class 0", "month, the nation goes away, class 2",
            "month, assistant 2 skill 32 zeros", "month, assistant 1 skill -1 zeros", "month, plain club", "month, interest rep -1000 balance 1",
            "month, trouble reputation -1500 chance 900 test 0", "month, trouble reputation 1500 chance 900 test 0",
            "month, player age 19 kind 1 reputed 3000", "month, player age 21 kind 1 reputed 3000", "month, player age 33 kind 1 reputed 3000",
            "month, player age 30 kind 0 reputed 3000", "month, player age 30 kind 2 reputed 3000", "month, player age 30 kind 3 reputed 3251",
            "month, player age 30 kind 3 reputed 9000", "month, player age 30 kind 3 reputed 4750",
            "month, player skill 30 traits 10/3 feelings 3", "month, player skill 10 traits 10/10 feelings 0",
            "month, player skill 0 traits 0/3 feelings 0", "month, retired 1 contract 1", "month, retired 0 contract 0"};
        std::stable_partition(cases.begin(), cases.end(), [&](const Case& item) {
            return std::any_of(std::begin(first), std::end(first), [&](const char* label) { return item.label == label; });
        });
    }
}
}
