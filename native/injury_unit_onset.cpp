// INJURY.CPP, block "onset": the daily pass (0x10185410) and the pending-injury settling (0x1018b6e0).
#include "injury_unit_onset.hpp"
#include "injury_unit_onset_detail.hpp"
#include <stdexcept>

namespace cm::aot::injury_unit {
using namespace onset;

namespace {
std::uint32_t extended(std::int32_t value) { return std::uint32_t(value); }

constexpr std::uint32_t type_class_routine = 0x1018c180, owner_check = 0x10188490, pick_type = 0x10188930,
                        settle_class = 0x1018c2f0, news_refresh_a = 0x1018b2b0, news_refresh_b = 0x1018bdd0,
                        refresh_person = 0x10187a40;
constexpr std::uint32_t month_of = 0x10715b20, league_check = 0x10003dc0, club_check = 0x1014c2e0;
constexpr std::uint32_t season_boundary_day = 0xb4, summer_months_a = 6, summer_months_b = 5;

// What a person row of the daily pass is, kept the way the original keeps it in registers where a callee
// can see it (bookkeeping beside the calls; the arithmetic is native).
class DailyPass {
public:
    DailyPass(Cpu& cpu, std::uint32_t manager_object)
        : c(cpu), frame(cpu, 0x10185410, 0xb0, {.first_gpr = 13}), manager(manager_object) {}

    void run() {
        today_cell = c.toc(toc::today);
        c.set({{15, manager}, {24, c.toc(toc::type_ranges)}, {25, c.toc(toc::floats)},
               {26, c.toc(sample_table::base)}, {27, c.toc(sample_table::first)}, {28, c.toc(sample_table::last)},
               {29, c.toc(sample_table::cursor)}, {30, c.toc(toc::people)}, {13, c.toc(toc::people_count)},
               {14, today_cell}});
        drop_news_list();
        count_injuries_down();
        if (season_boundary()) refresh_everybody();
        if (month(0x1018553c) != summer_months_a && month(0x10185550) != summer_months_b) sample_people();
        invoke_unit(c, news_refresh_a, 0x10186484, {manager});
        invoke_unit(c, news_refresh_b, 0x1018648c, {manager});
    }

private:
    Cpu& c;
    Frame frame;
    std::uint32_t manager, today_cell = 0;
    std::uint32_t people_base() { return c.u32(c.toc(toc::people)); }
    std::int32_t people_count() { return c.i32(c.toc(toc::people_count)); }
    std::uint32_t rows() { return c.u32(manager + manager::status_rows); }
    std::int16_t month(std::uint32_t back) { return std::int16_t(invoke(c, month_of, back, {today_cell})); }

    // The news list of the last pass is destroyed (0x10738c20 deleting) and forgotten. No case reaches the destroy: the list
    // comes from 0x10186cd0 and the destructor frees it with the game's allocator (a scratch list faults there on both sides).
    void drop_news_list() {
        const auto list = c.u32(manager + 0x28);
        if (list == 0) return;
        invoke(c, callee::list_destroy, 0x1018545c, {list, 1});
        c.put32(manager + 0x28, 0);
    }
    // Both countdowns of every injury record lose a day (halfwords, no floor).
    void count_injuries_down() {
        auto at = c.u32(manager + manager::injuries);
        for (std::int32_t i = 0; i < c.i32(manager + manager::injury_count); ++i, at += injury::size) {
            c.put16(at + record::countdown_a, std::uint32_t(c.i16(at + record::countdown_a)) - 1u);
            c.put16(at + record::countdown_b, std::uint32_t(c.i16(at + record::countdown_b)) - 1u);
        }
    }
    // The season date (two words) is copied into the frame, where later readers read it, and compared with
    // today: whether it lies after today (year first, then the day's sign trick).
    bool season_boundary() {
        const auto season = c.toc(toc_cell::season_date);
        const auto first = c.u32(season), second = c.u32(season + 4);
        c.put32(frame.at(0x40), first);
        c.put32(frame.at(0x44), second);
        const std::uint32_t year = first & 0xffff, today_year = c.u16(today_cell + 2);
        bool later;
        if (year > today_year) later = true;
        else if (year < today_year) later = false;
        else {
            const auto a = std::int32_t(std::int16_t(first >> 16)), b = std::int32_t(c.i16(today_cell));
            const auto x = a ^ b;
            later = ((x >> 1) - (x & a)) < 0;
        }
        return later && c.u16(today_cell) == season_boundary_day;
    }
    // At the season boundary every person's status row is brought up to date.
    void refresh_everybody() {
        std::int32_t index = 0;
        std::uint32_t person = people_base();
        c.set({{16, 0}, {17, person}});
        while (index < people_count()) {
            invoke_unit(c, refresh_person, 0x10185520, {manager, person});
            ++index;
            person += person_size;
            c.set({{16, std::uint32_t(index)}, {17, person}});
        }
    }

    // ---- the sample of people ----
    enum class Step { next, retry };

    void sample_people() {
        const auto people = std::uint32_t(people_count());
        std::int32_t share = std::int32_t(people) / 0x96;
        share = std::int16_t(share);
        if (share < 10) share = 10;
        const auto rounds = std::int32_t(std::int16_t(share));
        const auto early = std::int32_t(std::int16_t(rounds / 14));
        rounds_register = rounds;
        c.set({{20, std::uint32_t(rounds)}, {19, 0}, {14, std::uint32_t(early)}});
        std::int32_t turn = 0;
        early_rounds = early;
        while (turn < rounds) {
            this->turn = turn;
            if (sample_person() == Step::retry) --turn;
            ++turn;
            c.set(19, std::uint32_t(turn));
        }
    }

    std::int32_t rounds_register = 0, early_rounds = 0, turn = 0;
    // The registers the original keeps its current person in.
    std::uint32_t person = 0, offset = 0, row_offset = 0, class_register = 0, result_register = 0;

    std::uint32_t row() { return rows() + row_offset; }
    std::uint32_t profile_of_person() { return c.u32(person + person::profile); }
    std::uint32_t ask_class(std::uint32_t row_byte, std::uint32_t back) {
        return invoke_unit(c, type_class_routine, back, {manager, row_byte});
    }
    void wear(std::uint32_t field, std::uint32_t by) {
        const auto profile_address = profile_of_person();
        c.put8(profile_address + field, c.u8(profile_address + field) - by);
    }

    Step sample_person() {
        const auto count = std::uint32_t(people_count());
        c.set(16, count);
        const auto pick = bounded_draw(c, std::int32_t(count), {0x101855e8, 0x101856b0, 17});
        offset = std::uint32_t(pick) * person_size;
        c.set(21, offset);
        person = people_base() + offset;
        c.set(18, person);
        if (profile_of_person() == 0) return Step::retry;
        const auto job = c.i8(person + person::job);
        if (job != 0xb && job != 0xf && job != 0xd) return Step::next;
        if (std::int8_t(invoke(c, league_check, 0x10185764, {c.u32(c.toc(toc_cell::league_object)), person, 0})) != 0)
            return Step::next;
        if (c.u8(profile_of_person() + profile::level) == 0x14) {
            const auto first = std::int32_t(draw(c, 0x1018579c)) % 5;
            c.set(16, first);
            const auto second = draw(c, 0x101857f0) & 1;
            if (std::int32_t(first) > std::int32_t(second)) return Step::next;
        }
        row_offset = std::uint32_t(pick) * status::size;
        c.set(22, row_offset);
        if (c.i32(row() + row::injury) != -1) return Step::next;
        if (!proneness_roll()) return Step::next;
        if (std::int32_t(draw(c, 0x10185990)) % 3 != 0) return Step::next;
        if (c.i8(row() + row::form) < 0x14) return Step::next;
        choose_type();
        return after_choice();
    }
    // The person must be fit enough to be the sort that gets hurt: the draw against the form and proneness.
    bool proneness_roll() {
        if (c.i8(row() + row::pending_type) > -1) {
            const auto roll = std::int32_t(draw(c, 0x10185868)) % 100;
            const auto sum = std::int32_t(c.i8(row() + row::pending_count)) +
                             std::int32_t(c.i8(profile_of_person() + profile::proneness)) * 5 + std::int32_t(c.i8(row() + row::form));
            return std::int32_t(roll) <= sum * 2 - 0x1e;
        }
        const auto roll = std::int32_t(draw(c, 0x10185904)) % 160;
        const auto sum = std::int32_t(c.i8(row() + row::form)) + std::int32_t(c.i8(profile_of_person() + profile::proneness)) * 5;
        return std::int32_t(roll) <= sum * 2 - 0x1e;
    }
    // The class of injury: the pending one often, otherwise one drawn from the twelve with ranges.
    void choose_type() {
        if (c.i8(row() + row::pending_type) > -1) {
            const auto range = std::int32_t(c.i8(row() + row::pending_count)) + 2;
            c.set(16, std::uint32_t(range));
            const auto again = bounded_draw(c, range, {0x10185a24, 0x10185aec, 17});
            const auto roll = std::int32_t(draw(c, 0x10185b6c)) % 3;
            if (again > roll) {
                const auto answer = ask_class(c.u8(rows() + row_offset + row::pending_type), 0x10185bbc);
                class_register = extended(std::int8_t(answer));
                c.set(16, class_register);
                return;
            }
        }
        c.set(31, 0x2aaaaaab);
        for (;;) {
            const auto klass = std::int8_t(std::int32_t(draw(c, 0x10185bf0)) % 12);
            class_register = extended(klass);
            c.set(16, class_register);
            if (klass == 0 && std::int32_t(draw(c, 0x10185c44)) % 100 != 0) continue;
            const auto ranges = c.toc(toc::type_ranges) + std::uint32_t(klass) * 3;
            const auto range = std::int32_t(c.i8(ranges + 2)) + 2 - std::int32_t(c.i8(ranges + 1));
            c.set(23, std::uint32_t(range));
            if (bounded_draw(c, range, {0x10185ccc, 0x10185d94, 17}) != 0) break;
        }
    }
    Step after_choice() {
        if (c.i8(profile_of_person() + profile::level) < 0xf && std::int8_t(class_register) == 7) return Step::retry;
        if (std::int8_t(invoke_unit(c, owner_check, 0x10185e30, {manager, person})) != 0) return Step::next;
        const bool early = std::int32_t(turn) < early_rounds;
        const auto answer = invoke_unit(c, pick_type, early ? 0x10185e58u : 0x10185e74u,
                                        {manager, class_register, early ? 0xffffffffu : 0u, people_base() + offset});
        result_register = answer;
        c.set(17, answer);
        const auto index = extended(std::int8_t(answer)) - 0xc;
        if (index > 0x30) return register_chosen();
        const auto target = c.u32(c.toc(toc_cell::jump_table_pick) + index * 4);
        switch (target) {
            case 0x10185f48: wear_both_fitness(); return register_chosen();
            case 0x10186440: return register_chosen();
            case 0x10186470: return Step::next;
            case 0x10185e9c: wear_class_match(); return register_chosen();
            case 0x101862bc: wear_c(); return register_chosen();
            case 0x10186350: wear_d(); return register_chosen();
            // Every slot of the table at TOC -0x5440 holds one of the six targets above (jump-tables.txt); the table is the
            // game's constant data, so no other target is reachable.
            default: throw std::logic_error("A jump table target the original's table does not hold");
        }
    }
    Step register_chosen() {
        c.link(0x10186470);
        register_injury(c, {manager, result_register, 0, people_base() + offset, 0, 1, 0xffffffff, 0, 0});
        return Step::next;
    }
    std::uint32_t row_byte() { return c.u8(rows() + row_offset + row::pending_type); }

    // 0x10185f48: wears the two fitness bytes, harder when the pending injury is of the drawn class.
    void wear_both_fitness() {
        const auto first = profile_of_person();
        if (c.i8(first + profile::fitness_a) == 0x14) {
            const auto byte = row_byte();
            if (std::int8_t(byte) > -1 && std::int8_t(ask_class(byte, 0x10185f7c)) == std::int8_t(class_register)) {
                wear(profile::fitness_a, std::int32_t(draw(c, 0x10185fac)) % 3 + 1);
            } else if (std::int32_t(draw(c, 0x1018601c)) % 3 != 0) {
                wear(profile::fitness_a, (draw(c, 0x10186070) & 1) + 1);
            }
        } else if (c.i8(first + profile::fitness_a) > 0xf) {
            if (std::int32_t(draw(c, 0x101860dc)) % 3 == 0) wear(profile::fitness_a, 1);
        }
        if (c.u8(profile_of_person() + profile::fitness_c) == 0x14) {
            const auto byte = row_byte();
            if (std::int8_t(byte) > -1 && std::int8_t(ask_class(byte, 0x10186154)) == std::int8_t(class_register)) {
                wear(profile::fitness_c, std::int32_t(draw(c, 0x10186184)) % 3 + 1);
            } else if (std::int32_t(draw(c, 0x101861f4)) % 3 != 0) {
                wear(profile::fitness_c, 1);
            }
        } else if (c.i8(profile_of_person() + profile::fitness_a) > 0x11) {
            if (std::int32_t(draw(c, 0x10186270)) % 3 == 0) wear(profile::fitness_a, 1);
        }
    }
    // 0x10185e9c: only when the pending injury is of the drawn class and the first byte is at 0x14.
    void wear_class_match() {
        const auto byte = row_byte();
        if (std::int8_t(byte) <= -1) return;
        if (std::int8_t(ask_class(byte, 0x10185ebc)) != std::int8_t(class_register)) return;
        if (c.u8(profile_of_person() + profile::fitness_a) != 0x14) return;
        if (std::int32_t(draw(c, 0x10185efc)) % 3 == 0) wear(profile::fitness_a, 1);
    }
    // 0x101862bc.
    void wear_c() {
        const auto value = c.u8(profile_of_person() + profile::fitness_b);
        if (std::int8_t(value) == 0x14) {
            c.put8(profile_of_person() + profile::fitness_b, value - 1u);
        } else if (std::int8_t(value) > 0xf) {
            if (std::int32_t(draw(c, 0x10186304)) % 3 == 0) wear(profile::fitness_b, 1);
        }
    }
    // 0x10186350.
    void wear_d() {
        const auto value = c.i8(profile_of_person() + profile::fitness_b);
        if (value == 0x14) {
            if (std::int32_t(draw(c, 0x10186384)) % 3 != 0) wear(profile::fitness_b, 1);
        } else if (value > 0xf) {
            if (std::int32_t(draw(c, 0x101863f8)) % 3 == 0) wear(profile::fitness_b, 1);
        }
    }
};
}

void daily_injury_pass(Cpu& c, std::uint32_t manager) {
    DailyPass pass(c, manager);
    pass.run();
}

void settle_pending_injury(Cpu& c, std::uint32_t manager, std::uint32_t status_row, std::uint32_t injury_record) {
    Frame frame(c, 0x1018b6e0, 0x80, {.first_gpr = 18});
    c.set({{18, 0}, {19, injury_record}, {20, 0}, {22, 0}, {23, status_row}, {25, c.toc(sample_table::base)},
           {26, c.toc(sample_table::first)}, {27, c.toc(sample_table::last)}, {28, c.toc(sample_table::cursor)},
           {29, c.toc(toc::people)}, {30, c.toc(toc::people_count)}, {31, manager}});
    if (c.i8(status_row + row::pending_type) != -1) return;
    const auto kind = std::int32_t(c.i8(injury_record + record::type));
    if (c.i8(c.toc(toc::injury_types) + std::uint32_t(kind) * injury_type::size + type_row::pending) == 0) return;
    if (std::int32_t(draw(c, 0x1018b770)) % 20 != 0) return;
    c.put8(status_row + row::pending_type, c.u8(injury_record + record::type));
    c.put8(status_row + row::pending_count, 0);

    // The person with the best chance of the pending type: a sample of each pending row against the
    // number of times it was announced and the person's proneness, stopping at 95 or more.
    std::uint32_t best_score = 0, best_row = 0, best_person = 0;
    std::int32_t index = 0;
    std::uint32_t offset = 0, row_at = c.u32(manager + manager::status_rows);
    c.set({{21, 0}, {24, 0}, {23, row_at}});
    while (index < c.i32(c.toc(toc::people_count))) {
        if (c.i8(row_at + row::pending_type) != -1) {
            const auto person_at = c.u32(c.toc(toc::people)) + offset;
            c.set(19, person_at);
            const auto sample = draw(c, 0x1018b804);
            const auto announced = extended(std::int8_t(c.u8(row_at + row::pending_count) * 5u));
            const auto profile_address = c.u32(person_at + person::profile);
            std::uint32_t score = extended(std::int8_t(sample % 101)) - announced;
            if (profile_address != 0) score = extended(std::int8_t(score - c.u8(profile_address + profile::proneness)));
            else score = 100;
            if (std::int8_t(score) > std::int8_t(best_score)) {
                best_score = score;
                best_row = row_at;
                best_person = person_at;
                c.set({{20, best_score}, {22, best_row}, {18, best_person}});
            }
            if (std::int8_t(best_score) >= 0x5f) break;
        }
        offset += person_size;
        ++index;
        row_at += status::size;
        c.set({{24, offset}, {21, std::uint32_t(index)}, {23, row_at}});
    }
    if (best_row != 0) {
        const auto club = c.u32(best_person + person::club);
        if (c.u32(best_person + person::profile) != 0 && club != 0 && c.i16(club + 0x1ec) > 0x109a &&
            std::int8_t(invoke(c, club_check, 0x1018b8f0, {club, 0, 1, 0, 0})) == 0 &&
            std::int32_t(draw(c, 0x1018b918)) % 3 == 0) {
            const auto group = std::int8_t(invoke_unit(c, settle_class, 0x1018b964, {manager, best_person}));
            const auto slot = extended(group) - 0xb;
            std::uint32_t type = 0x4d;
            if (slot <= 0x41) {
                switch (c.u32(c.toc(toc_cell::jump_table_news) + slot * 4)) {
                    case 0x1018b988: type = 0x4e; break;
                    case 0x1018b990: type = 0x4f; break;
                    case 0x1018b998: type = 0x4d; break;
                    // Every slot of the table at TOC -0x545c holds one of the three targets above (jump-tables.txt);
                    // the table is the game's constant data, so no other target is reachable.
                    default: throw std::logic_error("A jump table target the original's table does not hold");
                }
            }
            c.link(0x1018b9c4);
            register_injury(c, {manager, type, 0, best_person, 0, 1, 0xffffffff, 0, 0});
            return;
        }
        c.put8(best_row + row::pending_type, 0xff);
        c.put8(best_row + row::pending_count, 0);
    }
}
}
