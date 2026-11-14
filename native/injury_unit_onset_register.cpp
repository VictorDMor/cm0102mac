// INJURY.CPP, block "onset": registering a new injury for a person (0x10188cc0), in named steps.
#include "injury_unit_onset.hpp"
#include "injury_unit_onset_detail.hpp"
#include <stdexcept>

namespace cm::aot::injury_unit {
using namespace onset;

namespace {
std::uint32_t extended(std::int32_t value) { return std::uint32_t(value); }

namespace callee_register {
inline constexpr std::uint32_t refresh_person = 0x10187a40, report_news = 0x1018bb50,
    tell_messages = 0x103d0990, unlink_news = 0x10748ce0, resize_records = 0x107751b0, allocate_records = 0x10775130,
    copy_match = 0x1030d4a0, skill_of = 0x100d2f50, club_check = 0x1014c2e0, loan_check = 0x100d3040,
    tell_manager = 0x10363b80, add_days = 0x10717b20, tell_press = 0x102b8460, tell_alert = 0x102de470,
    format_message = 0x1077b720;
}
// Return addresses of the bounded draws (redraw, nested scale) and the register that holds the result.
constexpr Site pending_site{0x10188f20, 0x10188fe8, 14}, reduce_site{0x10189488, 0x10189550, 22},
    reduce_again_site{0x1018963c, 0x10189704, 22}, morale_site{0x101897fc, 0x101898c4, 22},
    proneness_site{0x10189974, 0x10189a3c, 22}, health_site{0x10189b00, 0x10189bc8, 22},
    start_site{0x10189e7c, 0x10189f44, 22}, chance_site{0x1018a0ac, 0x1018a174, 22},
    chance_again_site{0x1018a228, 0x1018a2f0, 14}, repeat_site{0x1018a4c0, 0x1018a588, 14},
    repeat_limit_site{0x1018a618, 0x1018a6e0, 13}, alert_q_site{0x1018a7d4, 0x1018a89c, 14},
    alert_r_site{0x1018a9a0, 0x1018aa68, 14}, alert_s_site{0x1018ab5c, 0x1018ac24, 14},
    alert_t_site{0x1018ad10, 0x1018add8, 14};

constexpr CheckSite without_person{0x10188d38, 0xc0, 0x58, 0x872}, without_profile{0x10188dbc, 0xa0, 0x50, 0x87f},
    type_out_of_range{0x10188e38, 0x80, 0x48, 0x887}, no_room_for_records{0x101891f0, 0x60, 0x40, 0x8f2};

class Registrar {
public:
    Registrar(Cpu& cpu, const Registration& arguments)
        : c(cpu), frame(cpu, 0x10188cc0, 0x150, {.first_gpr = 13}), a(arguments),
          type(std::int8_t(arguments.type)), chance(std::int8_t(arguments.chance)) {}

    std::int32_t run() {
        c.set({{13, 0}, {14, a.match}, {15, a.manager}, {16, a.type}, {17, a.person}, {18, a.chance},
               {19, c.toc(toc::pool)}, {20, c.toc(toc::message)}, {21, 0xffffffff}, {22, c.toc(toc::success)},
               {23, a.pending_only}, {26, c.toc(toc_cell::fitness_rows)}, {27, c.toc(toc::floats)},
               {28, c.toc(sample_table::base)}, {29, c.toc(sample_table::first)}, {30, c.toc(sample_table::last)},
               {31, c.toc(sample_table::cursor)}});
        if (a.person == 0) return fail(without_person);
        if (c.i32(a.person) >= c.i32(c.toc(toc::people_count))) return -1;
        if (c.u32(a.person + person::profile) == 0) return fail(without_profile);
        if (type < 0 || type >= 0x52) {
            c.set({{6, a.person}, {7, a.match}, {8, a.report}, {9, a.news_argument}, {10, a.chance}});
            invoke(c, callee_register::format_message, 0x10188e24,
                   {c.toc(toc_cell::spare_value), c.toc(toc::pool) + 0xc1, extended(type)});
            return fail(type_out_of_range);
        }
        invoke_unit(c, callee_register::refresh_person, 0x10188e88, {a.manager, a.person});
        if (const auto object = c.u32(c.toc(toc_cell::messages_object)); object != 0)
            invoke(c, callee_register::tell_messages, 0x10188ea4, {object, a.person, 0});
        if (std::int8_t(a.pending_only) != 0) return note_pending();

        if (!release_current_injury()) return -1;
        c.put8(row_address() + row::last_type, a.type);
        if (!take_record_slot()) return -1;
        link_match();
        choose_length();
        wear_the_player();
        record_lengths();
        set_start_date();
        invoke_unit(c, callee_register::refresh_person, 0x1018a014, {a.manager, a.person});
        c.link(0x1018a02c);
        post_injury_news(c, a.manager, a.person, record_address(), a.news_argument);
        c.put32(row_address() + row::injury, extended(slot));
        pending_chance();
        reactions();
        if (std::int8_t(a.report) != 0 && (std::uint8_t(a.type) < 2 || type == 2)) {
            if (const auto club = c.u32(a.person + person::club); club != 0)
                invoke_unit(c, callee_register::report_news, 0x1018aeac,
                            {a.manager, c.toc(toc::today), a.type, c.u32(club), 0});
        }
        return slot;
    }

private:
    Cpu& c;
    Frame frame;
    Registration a;
    std::int8_t type, chance;
    std::int32_t slot = -1;                // the record index (r21)
    std::uint32_t record_offset = 0;       // slot * 0x38 (r24)
    std::int32_t carried = 0;              // the countdown of the injury this one replaces (r13)
    std::uint32_t days = 0;                // the length (r20), a full word like the register
    std::int32_t skill = 0;                // the best skill of the club's three staff slots (r19)
    std::uint32_t type_offset = 0;         // type * 0xc (r14)
    std::uint32_t type_row_size_address = 0;  // the type row's +4 (r13 after the length)
    std::int32_t morale = 0;               // the person's morale byte (r19 after the length)

    std::uint32_t row_address() { return c.u32(a.manager + manager::status_rows) + c.u32(a.person) * status::size; }
    std::uint32_t record_address() { return c.u32(a.manager + manager::injuries) + record_offset; }
    std::uint32_t type_row() { return c.toc(toc::injury_types) + type_offset; }
    std::int32_t fail(const CheckSite& site) {
        report_check(c, frame, site);
        return -1;
    }
    std::uint32_t fitness_byte_address() {
        return c.u32(c.toc(toc_cell::fitness_rows)) + c.u32(a.person) * fitness_row_size + fitness_injuries;
    }

    // The injury waits: the status row notes the type and how many times it was announced.
    std::int32_t note_pending() {
        if (chance == 0) return -1;
        if (c.i8(c.toc(toc::injury_types) + extended(type) * injury_type::size + type_row::pending) == 0) return -1;
        if (chance < 0) {
            const std::int32_t range = chance / 10;
            c.set(13, extended(range));
            if (bounded_draw(c, range, pending_site) == 0) {
                c.put8(row_address() + row::pending_type, a.type);
                c.put8(row_address() + row::pending_count, 1);
                return -1;
            }
        }
        if (chance > 0) {
            c.put8(row_address() + row::pending_type, a.type);
            c.put8(row_address() + row::pending_count, a.chance);
        }
        return -1;
    }

    // A person with an injury record gets a new one only for the types 0x4d..0x4f or 0x51; the old record
    // is unlinked and freed, its countdown carried.
    bool release_current_injury() {
        const auto row = row_address();
        if (c.i32(row + row::injury) == -1) return true;
        if (std::uint8_t(a.type - 0x4d) > 2 && type != 0x51) return false;
        const auto old = c.u32(a.manager + manager::injuries) + c.u32(row + row::injury) * injury::size;
        carried = c.i16(old + record::countdown_a);
        c.set({{13, extended(carried)}, {23, old}});
        invoke(c, callee_register::unlink_news, 0x1018911c,
               {c.u32(a.manager + 0xc), row + row::news_list, row + row::news_list_end, old});
        c.put8(old, 0xff);
        c.put32(row_address() + row::injury, 0xffffffff);
        return true;
    }

    // The first free record, or the record table grown by a hundred.
    bool take_record_slot() {
        const auto records = c.u32(a.manager + manager::injuries);
        if (records != 0) {
            std::uint32_t at = records;
            for (std::int32_t i = 0; i < c.i32(a.manager + manager::injury_count); ++i, at += injury::size)
                if (c.i8(at) == -1) {
                    slot = i;
                    c.set(21, extended(slot));
                    break;
                }
        }
        if (slot == -1) {
            const auto old_count = c.i32(a.manager + manager::injury_count);
            const auto new_count = std::int32_t(std::uint32_t(old_count) + 100u);
            c.set(23, extended(new_count));
            const auto bytes = std::uint32_t(new_count) * injury::size;
            const auto grown = records != 0 ? invoke(c, callee_register::resize_records, 0x101891bc, {records, bytes})
                                            : invoke(c, callee_register::allocate_records, 0x101891cc, {bytes});
            c.put32(a.manager + manager::injuries, grown);
            if (c.u32(a.manager + manager::injuries) == 0) {
                fail(no_room_for_records);
                return false;
            }
            for (auto i = old_count; i < new_count; ++i)
                c.put8(c.u32(a.manager + manager::injuries) + std::uint32_t(i) * injury::size, 0xff);
            slot = c.i32(a.manager + manager::injury_count);
            c.set(21, extended(slot));
            c.put32(a.manager + manager::injury_count, extended(new_count));
        }
        record_offset = std::uint32_t(slot) * injury::size;
        c.set(24, record_offset);
        c.put8(c.u32(a.manager + manager::injuries) + record_offset, a.type);
        return true;
    }

    // The match the injury happened in: the snapshot of the match's record of this person, else -1.
    void link_match() {
        if (a.match == 0) {
            c.put8(record_address() + record::match_found, 0xff);
            return;
        }
        std::uint32_t found = 0, step = 0;
        for (std::int8_t i = 0; i < 0x32; ++i, step += 4) {
            auto side = c.u32(a.match + 0x1c);
            if (c.u32(side + step + 0x98) == a.person) { found = side; break; }
            side = c.u32(a.match + 0x20);
            if (c.u32(side + step + 0x98) == a.person) { found = side; break; }
        }
        if (found == 0) c.put8(record_address() + record::match_found, 0xff);
        else invoke(c, callee_register::copy_match, 0x10189384, {a.match, found, record_address() + record::match_snapshot});
    }

    // The best skill among the club's three staff slots (0x100d2f50 of the staff member's record).
    std::int32_t best_staff_skill() {
        c.set(19, 0);
        const auto club = c.u32(a.person + person::club);
        if (club != 0) {
            c.set({{20, 0}, {14, 0}});
            std::uint32_t at = 0;
            for (std::int8_t i = 0; i < 3; ++i, at += 4) {
                const auto holder = c.u32(c.u32(a.person + person::club) + at + 0x190);
                if (holder != 0) {
                    const auto member = c.u32(holder + 0x1c);
                    if (member != 0) {
                        const auto first = invoke(c, callee_register::skill_of, 0x101893dc, {member, c.u8(member + 0x3e)});
                        if (std::int8_t(first) > std::int8_t(c.r(19))) {
                            const auto again = c.u32(c.u32(c.u32(a.person + person::club) + at + 0x190) + 0x1c);
                            c.set(19, invoke(c, callee_register::skill_of, 0x10189404, {again, c.u8(again + 0x3e)}));
                        }
                    }
                }
                c.set({{14, at + 4}, {20, std::uint32_t(i) + 1}});
            }
        }
        type_offset = extended(type) * injury_type::size;
        c.set(14, type_offset);
        const auto best = std::int32_t(std::int8_t(c.r(19)));
        c.set(19, extended(best));
        return best;
    }

    void add_days(std::int32_t by) {
        days += extended(by);
        c.set(20, days);
    }
    void subtract_days(std::int32_t by) {
        days -= extended(by);
        c.set(20, days);
    }

    // The length of the injury: from the type's spread and the skill, shortened by a draw, then lengthened by
    // the person's morale and proneness, the type's base, and the carried countdown; then each day may cost the
    // person a point of the fitness table's injury count.
    void choose_length() {
        skill = best_staff_skill();
        const std::int32_t product = (0x23 - skill) * std::int32_t(c.i16(type_row() + type_row::spread));
        std::int32_t spread = std::int16_t(product / 20);
        days = extended(spread);
        c.set(20, days);
        const auto range = skill + 5;
        c.set(22, extended(range));
        const auto reduction = bounded_draw(c, range, reduce_site);
        if (reduction > std::int32_t(draw(c, 0x101895d0)) % 20) {
            c.set(19, extended(std::int16_t(days)));
            const auto less = bounded_draw(c, std::int16_t(days), reduce_again_site);
            subtract_days(std::int16_t(less));
        }
        if (type == 0x51) {
            days = 0;
            c.set(20, 0);
        }
        add_days(carried);
        morale = std::int8_t(c.i16(a.person + person::morale));
        c.set(19, extended(morale));
        if (type != 0x51 && c.i16(type_row() + type_row::base_days) > 10) {
            if (morale > 0x1e) {
                const auto sum = std::int32_t(std::int8_t(c.u8(c.u32(a.person + person::profile) + profile::proneness))) + (morale - 0x23);
                if (std::int16_t(sum) > 0) {
                    c.set(13, extended(std::int16_t(sum)));
                    add_days(std::int16_t(bounded_draw(c, std::int16_t(sum), morale_site)));
                }
            }
            const auto prone = std::int32_t(c.i8(c.u32(a.person + person::profile) + profile::proneness));
            if (prone > 10) {
                const auto extra = std::int16_t(prone - 10);
                c.set(13, extended(extra));
                add_days(std::int16_t(bounded_draw(c, extra, proneness_site)));
            }
        }
        type_row_size_address = type_row() + type_row::base_days;
        c.set({{23, 0}, {13, type_row_size_address}});
        days = extended(std::int16_t(days + std::uint32_t(c.i16(type_row_size_address))));
        c.set(20, days);
        std::int32_t counted = 0;
        while (counted < std::int32_t(days) && c.u8(fitness_byte_address()) < 100) {
            const auto resilience = std::int32_t(c.i8(c.u32(a.person + person::profile) + profile::resilience));
            const auto limit = resilience * 5 + 12;
            c.set(25, extended(limit));
            if (bounded_draw(c, limit, health_site) == 0) c.put8(fitness_byte_address(), c.u8(fitness_byte_address()) + 1u);
            ++counted;
            c.set(23, extended(counted));
        }
    }

    // A long injury of a player at a club that can lose him tells the career object, and a long one may ask
    // the club's manager to react.
    void wear_the_player() {
        if (std::int8_t(a.starts_later) != 0) return;
        const auto club = c.u32(a.person + person::club);
        if (club != 0 && std::int8_t(invoke(c, callee_register::club_check, 0x10189ca0, {club, 0, 1, 0, 0})) == 0 &&
            std::int32_t(days) >= 100 &&
            std::int8_t(invoke(c, callee_register::loan_check, 0x10189cbc, {a.person, 0})) == 0) {
            if (const auto career = c.u32(c.toc(toc_cell::career_object)); career != 0) {
                const auto state = c.u32(career + 0x988);
                c.put8(c.u32(state + 0xdc) + c.u32(c.u32(a.person + person::club)) * 12 + 9, 100);
            }
        }
        const auto roll = std::int32_t(draw(c, 0x10189d20)) % 50;
        if (std::int32_t(days) >= std::int32_t(roll) + 0x1e) {
            if (const auto career = c.u32(c.toc(toc_cell::career_object)); career != 0)
                if (const auto state = c.u32(career + 0x988); state != 0)
                    invoke(c, callee_register::tell_manager, 0x10189d8c, {state, a.person, 0});
        }
    }

    // The record's two lengths: the days out, and the days unfit (a float share of the length, at most 60 less).
    void record_lengths() {
        const auto record = record_address();
        c.put16(record + record::days_out, days);
        c.put16(record + record::countdown_a, days);
        Fpu fpu(c);
        const auto doubles = c.toc(toc::floats);
        const auto to_double = [&](std::uint32_t value) {
            const auto bits = (std::uint64_t(0x43300000) << 32) | (value ^ 0x80000000u);
            return std::bit_cast<double>(bits);
        };
        const auto magic = c.f64(doubles + 8), divisor = c.f64(doubles + 0x70);
        const auto length = fpu.fsubs(to_double(days), magic);
        const auto ratio_byte = extended(c.i8(c.toc(toc::injury_types) + type_offset + type_row::unfit_ratio));
        auto ratio = fpu.fsub(to_double(ratio_byte), magic);
        ratio = fpu.fdiv(ratio, divisor);
        ratio = fpu.frsp(ratio);
        const auto unfit = fpu.fmuls(length, ratio);
        c.put16(record_address() + record::days_unfit, std::uint32_t(fpu.fctiwz(0, unfit)));
        const auto at = record_address();
        if (c.i16(at + record::days_unfit) + 0x3c < c.i16(at + record::days_out))
            c.put16(at + record::days_unfit, extended(c.i16(at + record::days_out) - 0x3c));
        c.put16(record_address() + record::countdown_b, extended(c.i16(record_address() + record::days_unfit)));
    }

    // Today for an injury that happens now; a few days ahead for one that starts later.
    void set_start_date() {
        if (std::int8_t(a.starts_later) != 0) {
            const auto ahead = bounded_draw(c, std::int32_t(days), start_site);
            const auto out = frame.at(0x38);
            invoke(c, callee_register::add_days, 0x10189fb8,
                   {out, c.toc(toc::today), extended(std::int16_t(extended(ahead + 1)))});
            const auto at = record_address() + record::started;
            c.put16(at + 2, c.u16(out + 2));
            c.put16(at, extended(c.i16(out)));
            c.put8(at + 4, c.u8(out + 4));
        } else {
            const auto today = c.toc(toc::today);
            const auto at = record_address() + record::started;
            c.put16(at + 2, c.u16(today + 2));
            c.put16(at, extended(c.i16(today)));
            c.put8(at + 4, c.u8(today + 4));
        }
    }

    // What the pending chance does once the injury is registered (the rule of a pending-only call, with a
    // second draw for the announcement count), then the press and the settling of other people's pending marks.
    void pending_chance() {
        if (chance != 0 && c.i8(type_row() + type_row::pending) != 0) {
            bool announced = false;
            if (chance < 0) {
                const auto chance_word = extended(chance);
                c.set(20, chance_word);
                const std::int32_t range = chance / 10;
                c.set(14, extended(range));
                if (bounded_draw(c, range, chance_site) == 0) {
                    c.put8(row_address() + row::pending_type, a.type);
                    if (bounded_draw(c, chance, chance_again_site) == 0) c.put8(row_address() + row::pending_count, 1);
                    announced = true;
                }
            }
            if (!announced && chance > 0) {
                c.put8(row_address() + row::pending_type, a.type);
                c.put8(row_address() + row::pending_count, a.chance);
            }
        }
        if (c.u32(a.person + person::news_person) != 0)
            if (const auto object = c.u32(c.toc(toc_cell::press_object)); object != 0)
                invoke(c, callee_register::tell_press, 0x1018a3d4,
                       {object, a.person, extended(c.i16(record_address() + record::days_out))});
        c.put8(row_address() + row::cleared, 0);
        c.link(0x1018a40c);
        settle_pending_injury(c, a.manager, row_address(), record_address());
    }

    void alert(std::uint32_t back) {
        if (const auto object = c.u32(c.toc(toc_cell::alert_object)); object != 0)
            invoke(c, callee_register::tell_alert, back, {object, a.person, extended(c.i16(record_address() + record::days_unfit))});
    }

    // Whether the injury repeats a type already pending, and whether the person's morale and proneness make
    // the length alarming: each tells the alert object (0x102de470), the first matching one ending the pass.
    void reactions() {
        repeat_alert();
        alarm_rules();
    }
    void repeat_alert() {
        const auto row = row_address();
        if (!(type == c.i8(row + row::pending_type) && chance >= 0)) return;
        c.put8(row + row::pending_count, c.u8(row + row::pending_count) + 1u);
        const auto count = c.u8(row_address() + row::pending_count);
        if (std::int8_t(count) <= 1) return;
        const std::int32_t reduced = 200 - c.i16(type_row_size_address);
        std::int32_t limit = reduced > 5 ? reduced : 5;
        c.set(20, extended(limit));
        if (chance > 0) limit /= chance;
        c.set(20, extended(limit));
        const auto times = std::int32_t(std::int8_t(count));
        c.set(13, extended(times));
        const auto first = bounded_draw(c, times, repeat_site);
        const auto second = bounded_draw(c, limit, repeat_limit_site);
        if (first > second) alert(0x1018a76c);
    }
    void alarm_rules() {
        const auto unfit = [&] { return std::int32_t(c.i16(record_address() + record::days_unfit)); };
        const auto morale_now = [&] { return std::int32_t(std::int8_t(c.i16(a.person + person::morale))); };
        const auto prone = [&] { return std::int32_t(c.i8(c.u32(a.person + person::profile) + profile::proneness)); };
        if (morale > 0x24) {
            const auto limit = 0x78 - (morale_now() + prone());
            const auto length = unfit();
            c.set(13, extended(length));
            if (length > limit) {
                const auto drawn = bounded_draw(c, length, alert_q_site);
                if (drawn > 0x5a - morale_now()) { alert(0x1018a934); return; }
            }
        }
        if (morale > 0x21) {
            const auto limit = 0x96 - (morale_now() + prone());
            const auto length = unfit();
            c.set(13, extended(length));
            if (length > limit) {
                const auto drawn = bounded_draw(c, length, alert_r_site);
                if (drawn > 0x5a - morale_now()) { alert(0x1018ab00); return; }
            }
        }
        if (morale > 0x1f) {
            const auto limit = 0x12c - morale_now();
            const auto length = unfit();
            c.set(13, extended(length));
            if (length > limit) {
                const auto drawn = bounded_draw(c, length, alert_s_site);
                if (drawn > 200 - morale_now()) { alert(0x1018acbc); return; }
            }
        }
        if (unfit() > 500) {
            const auto range = std::int32_t(c.i8(c.u32(a.person + person::profile) + profile::resilience)) + 5;
            c.set(13, extended(range));
            if (bounded_draw(c, range, alert_t_site) == 0) alert(0x1018ae64);
        }
    }
};
}

std::int32_t register_injury(Cpu& c, const Registration& arguments) {
    Registrar registrar(c, arguments);
    return registrar.run();
}
}
