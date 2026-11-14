// INJURY.CPP: questions about a person's status row and current injury, the manager's entry list, and the
// injury choice (injury_unit_status.hpp). The daily update is in injury_unit_status_update.cpp.
#include "injury_unit_status.hpp"
#include "injury_unit_status_detail.hpp"
#include <stdexcept>

namespace cm::aot::injury_unit {
using guest::Cpu;
using guest::Frame;
using namespace status_detail;

namespace {
constexpr CheckSite current_injury_without_person{0x1018c384, 0x40, 0x38, 0xe23};
constexpr CheckSite stage_without_person{0x101876c4, 0x48, 0x40, 0x53f};
constexpr CheckSite close_without_person{0x1018ba2c, 0x40, 0x38, 0xc70};
// The manager's entry list (+0x28): a list object with its cursor, the entries 8 bytes (key word, two bytes).
constexpr std::uint32_t entry_list = 0x28, list_head = 4, list_cursor = 0xc;
namespace callee_list {
constexpr std::uint32_t construct = 0x10738c00, next = 0x10738e00, append = 0x10738d10, remove = 0x10738e60;
}
// The unit's float pool constants this file uses.
namespace pool {
constexpr std::uint32_t scale_a = 0x10, scale_b = 0x18, rating_factor = 0x20, cap = 0x28;
}
}

std::int32_t condition_of(Cpu& c, std::uint32_t manager_object, std::int32_t index) {
    if (index >= c.i32(c.toc(toc::people_count))) return 0;
    return c.i16(c.u32(manager_object + manager::status_rows) + std::uint32_t(index) * status::size + status::condition);
}

void set_status_flag(Cpu& c, std::uint32_t manager_object, std::uint32_t person, std::uint32_t value) {
    c.put8(status_row(c, manager_object, person) + status::flag, std::uint8_t(value));
}

std::uint32_t type_class(Cpu& c, std::int8_t type) {
    return c.u8(c.toc(toc::injury_types) + std::uint32_t(std::int32_t(type)) * injury_type::size + 1);
}

std::uint32_t current_injury(Cpu& c, std::uint32_t manager_object, std::uint32_t person) {
    Frame frame(c, 0x1018c340, 0x70, {.first_gpr = 28});
    c.set({{31, c.toc(toc::pool)}, {30, c.toc(toc::message)}, {29, person}, {28, manager_object}});
    if (person == 0) {
        report_check(c, frame, current_injury_without_person);
        return 0;
    }
    if (c.i32(person) >= c.i32(c.toc(toc::people_count))) return 0;
    c.link(0x1018c3f8);
    update_status(c, manager_object, person);
    const auto index = c.u32(status_row(c, manager_object, person) + status::injury);
    return index == no_injury ? 0 : injury_record(c, manager_object, index);
}

std::uint32_t injury_stage(Cpu& c, std::uint32_t manager_object, std::uint32_t person) {
    Frame frame(c, 0x10187680, 0x80, {.first_gpr = 28});
    c.set({{31, c.toc(toc::pool)}, {30, c.toc(toc::message)}, {29, person}, {28, manager_object}});
    if (person == 0) {
        report_check(c, frame, stage_without_person);
        return 0;
    }
    if (c.i32(person) >= c.i32(c.toc(toc::people_count))) return 0;
    c.link(0x10187738);
    update_status(c, manager_object, person);
    const auto index = c.u32(status_row(c, manager_object, person) + status::injury);
    if (index == no_injury) return 0;
    const auto record = injury_record(c, manager_object, index);
    c.set(30, record);
    invoke(c, callee::days_between, 0x10187774,
           {frame.at(0x38), c.toc(toc::today), c.u32(record + injury::started), c.u32(record + injury::started + 4)});
    const std::int32_t since = days_of(c, frame.at(0x38));
    if (since < c.i16(record + injury::days_unfit)) return 2;
    return halfword_greater(c.i16(record + injury::days_out), since) ? 1 : 0;
}

std::uint32_t sharpness_of(Cpu& c, std::uint32_t manager_object, std::uint32_t person, std::uint32_t percent) {
    Frame frame(c, 0x101877e0, 0x60, {.first_gpr = 29});
    c.set({{31, percent}, {30, person}, {29, manager_object}});
    if (person == 0) return 0;
    if (c.i32(person) >= c.i32(c.toc(toc::people_count))) return std::int8_t(percent) == 0 ? 10000 : 100;
    c.link(0x10187848);
    update_status(c, manager_object, person);
    if (std::int8_t(percent) == 0)
        return std::uint32_t(std::int32_t(c.i16(status_row(c, manager_object, person) + row::sharpness)));
    guest::Fpu fpu(c);
    const auto floats = c.toc(toc::floats);
    const std::int32_t sharpness = c.i16(status_row(c, manager_object, person) + row::sharpness);
    c.put32(frame.at(0x40), 0x43300000);
    c.put32(frame.at(0x44), std::uint32_t(sharpness) ^ 0x80000000u);
    const double shifted = fpu.fsubs(magic(sharpness), c.f64(floats + 8));
    const double divisor = double(c.f32(c.toc(cell::float_one)));
    const auto bits = fpu.fctiwz(0, fpu.fdivs(shifted, divisor));
    c.put64(frame.at(0x38), bits);
    return std::uint32_t(bits);
}

std::uint32_t match_fitness_of(Cpu& c, std::uint32_t manager_object, std::uint32_t person, std::uint32_t percent) {
    Frame frame(c, 0x101878d0, 0x60, {.first_gpr = 29});
    c.set({{31, percent}, {30, person}, {29, manager_object}});
    if (person == 0) return 0;
    if (c.i32(person) >= c.i32(c.toc(toc::people_count))) return std::int8_t(percent) == 0 ? 10000 : 100;
    c.link(0x10187938);
    const auto stage = std::int8_t(injury_stage(c, manager_object, person));
    const auto row_address = status_row(c, manager_object, person);
    const std::int32_t product = c.i16(row_address + row::fitness) * c.i16(row_address + row::sharpness);
    std::int32_t value = std::int16_t(product / 10000);
    if (stage == 1) value = std::int16_t(value >= 0x2328 ? 0x2328 : value);
    std::int32_t result = std::int16_t(value > 100 ? value : 100);
    if (std::int8_t(percent) == 0) return std::uint32_t(result);
    guest::Fpu fpu(c);
    const auto floats = c.toc(toc::floats);
    c.put32(frame.at(0x44), std::uint32_t(result) ^ 0x80000000u);
    c.put32(frame.at(0x40), 0x43300000);
    const double shifted = fpu.fsubs(magic(result), c.f64(floats + 8));
    const double divided = fpu.fdivs(shifted, double(c.f32(c.toc(cell::float_one))));
    const auto bits = fpu.fctiwz(0, fpu.fadd(c.f64(floats + pool::cap), divided));
    c.put64(frame.at(0x38), bits);
    const auto low = std::uint32_t(bits);
    return std::int16_t(low) > 100 ? 100 : low;
}

void wear_condition(Cpu& c, std::uint32_t manager_object, std::int32_t index) {
    Frame frame(c, 0x10187450, 0x60, {.first_gpr = 24});
    if (index >= c.i32(c.toc(toc::people_count))) return;
    const std::uint32_t by_status = std::uint32_t(index) * status::size, by_person = std::uint32_t(index) * person_size;
    const auto hold = [&] {
        c.set({{24, manager_object}, {25, by_person}, {26, by_status}, {27, c.toc(sample_table::base)},
               {28, c.toc(sample_table::first)}, {29, c.toc(sample_table::last)}, {30, c.toc(sample_table::cursor)},
               {31, c.toc(toc::people)}});
    };
    const auto row_address = [&] { return c.u32(manager_object + manager::status_rows) + by_status; };
    const auto person_address = [&] { return c.u32(c.toc(toc::people)) + by_person; };
    const auto trait = [&] { return std::int8_t(c.u16(person_address() + person::form)); };
    const std::int32_t first = c.i16(row_address() + row::condition);
    if (first < -0x3e8) c.put16(row_address() + row::condition, std::uint32_t(first - 1));
    bool settled = false;
    if (trait() < 0x13) {
        const auto address = row_address() + row::condition;
        if (c.i16(address) < 0) {
            c.put16(address, 0);
            settled = true;
        }
    }
    if (!settled) {
        bool second_draw = true;
        if (c.i16(row_address() + row::condition) >= -0x1f4) {
            const auto sample = draw_sample(c, 0x10187518, hold);
            const auto rest = std::int32_t(sample) - std::int32_t(sample) / 25 * 25;
            if (trait() >= rest) {
                const auto address = row_address() + row::condition;
                c.put16(address, std::uint32_t(c.i16(address) - 0xd));
                second_draw = false;
            }
        }
        if (second_draw) {
            const auto sample = draw_sample(c, 0x10187588, hold);
            const auto address = row_address() + row::condition;
            const std::int32_t rest = std::int16_t(std::int32_t(sample) - std::int32_t(sample) / 13 * 13);
            c.put16(address, std::uint32_t(c.i16(address) - rest));
        }
    }
    const auto profile = c.u32(person_address() + person::profile);
    if (profile != 0 && c.u8(profile + 0xf) == 0x14) {
        const auto address = row_address() + row::condition;
        const std::int32_t now = c.i16(address);
        c.put16(address, std::uint32_t(now < 0x41a ? now : 0x41a));
    }
}

void set_sharpness(Cpu& c, std::uint32_t manager_object, std::uint32_t person, std::uint32_t sharpness, std::uint32_t change) {
    Frame frame(c, 0x10186a20, 0x70, {.first_gpr = 26});
    if (person == 0) return;
    if (c.i32(person) >= c.i32(c.toc(toc::people_count))) return;
    const auto floats = c.toc(toc::floats);
    c.put16(status_row(c, manager_object, person) + row::sharpness, sharpness);
    const auto row_address = status_row(c, manager_object, person);
    const std::int32_t condition = c.i16(row_address + row::condition);
    const std::int32_t delta = std::int16_t(change);
    if (condition < -1000) {
        guest::Fpu fpu(c);
        const double scaled = fpu.fsub(magic(condition), c.f64(floats + 8));
        const double rate = fpu.multiply_add(0x10186ac8, c.f64(floats + pool::scale_a), scaled, c.f64(floats + pool::scale_b));
        const double step = fpu.fsub(magic(delta), c.f64(floats + 8));
        const auto bits = fpu.fctiwz(0, fpu.fmul(rate, step));
        c.put16(row_address + row::condition, std::uint32_t(condition) + std::uint32_t(bits));
        return;
    }
    const auto profile = c.u32(person + person::profile);
    const std::int32_t trait_sum = std::int16_t(c.i8(profile + 0x44) + c.i8(person + person::temper) + c.i8(profile + 0x3c));
    if (condition < 0x4b0 - trait_sum) {
        c.put16(row_address + row::condition, std::uint32_t(condition + delta));
        return;
    }
    if (condition >= 0x4e2) return;
    std::int32_t roll = 0;
    if (delta != 0) {
        const auto sample = draw_sample(c, 0x10186b70, [&] {
            c.set({{26, manager_object}, {27, person}, {28, std::uint32_t(delta)}, {29, floats},
                   {30, c.toc(sample_table::base)}, {31, c.toc(sample_table::cursor)}});
        });
        // 0x10186b98..0x10186c68 draw over a range past 0xffff: the range is a halfword, so never.
        roll = remainder_in(sample, delta);
    }
    const auto again = status_row(c, manager_object, person) + row::condition;
    c.put16(again, std::uint32_t(c.i16(again) + std::int16_t(roll + 1)));
}

void reset_status(Cpu& c, std::uint32_t manager_object, std::uint32_t index) {
    Frame frame(c, 0x10186ea0, 0x50, {.first_gpr = 27});
    c.set({{27, manager_object}, {28, index}, {30, c.toc(toc::people)}, {31, c.toc(toc::today)}});
    if (const auto list = c.u32(manager_object + entry_list); list != 0) {
        c.put32(list + list_cursor, c.u32(list + list_head));
        for (;;) {
            const auto entry = invoke(c, callee_list::next, 0x10186ef4, {c.u32(manager_object + entry_list)});
            if (entry == 0) break;
            if (c.u32(entry) == index)
                invoke(c, callee_list::remove, 0x10186eec, {c.u32(manager_object + entry_list), entry});
        }
    }
    const auto row_address = c.u32(manager_object + manager::status_rows) + index * status::size;
    c.set(29, row_address);
    const auto person = c.u32(c.toc(toc::people)) + index * person_size;
    if (c.u32(row_address + status::injury) != no_injury) {
        c.link(0x10186f2c);
        close_injury(c, manager_object, person, 0);
    }
    invoke_unit(c, 0x1018fdf0, 0x10186f40, {manager_object, person});
    const auto today = c.toc(toc::today);
    c.put16(row_address + 2, c.u16(today + date::year_part));
    c.put16(row_address, std::uint32_t(c.i16(today + date::day_part)));
    c.put8(row_address + 4, c.u8(today + date::byte_part));
    c.put16(row_address + row::fitness, 10000);
    c.put16(row_address + row::sharpness, 10000);
    c.put16(row_address + row::condition, 0);
    c.put32(row_address + status::injury, no_injury);
    c.put8(row_address + 0x14, 0xff);
    c.put32(row_address + 0x18, no_injury);
    c.put32(row_address + 0x1c, no_injury);
    c.put8(row_address + 0x21, 0xff);
    const auto flag = c.u32(c.toc(cell::person_stats)) + index * stats::size + stats::flags;
    c.put8(flag, c.u8(flag) & 0x7f);
}

void close_injury(Cpu& c, std::uint32_t manager_object, std::uint32_t person, std::uint32_t report) {
    Frame frame(c, 0x1018b9f0, 0x80, {.first_gpr = 27});
    c.set({{27, c.toc(toc::message)}, {28, c.toc(toc::pool)}, {29, manager_object}, {30, person}, {31, report}});
    if (person == 0) {
        report_check(c, frame, close_without_person);
        return;
    }
    if (c.i32(person) >= c.i32(c.toc(toc::people_count))) return;
    const auto row_address = status_row(c, manager_object, person);
    const auto index = c.u32(row_address + status::injury);
    if (index == no_injury) return;
    const auto record = injury_record(c, manager_object, index);
    c.set(27, record);
    if (c.u8(record + record::kind) != 0x51)
        invoke(c, 0x10748ce0, 0x1018bad0, {c.u32(manager_object + 0xc), row_address + 0x18, row_address + 0x1c, record});
    if (std::int8_t(report) != 0) {
        if (const auto club = c.u32(person + person::club); club != 0) {
            if (std::int8_t(invoke(c, 0x1014c2e0, 0x1018baf8, {club, 0, 1, 0, 0})) != 0)
                invoke_unit(c, 0x1018dcb0, 0x1018bb1c, {manager_object, person, 1, c.u8(record + record::kind), 0, 0});
        }
    }
    c.put8(record + record::kind, 0xff);
    c.put32(status_row(c, manager_object, person) + status::injury, no_injury);
}

void note_entry(Cpu& c, std::uint32_t manager_object, std::uint32_t key, std::uint32_t kind_cell, std::uint32_t strength_cell) {
    Frame frame(c, 0x10186cd0, 0x60, {.first_gpr = 27});
    c.set({{28, manager_object}, {29, key}, {30, kind_cell}, {31, strength_cell}});
    if (c.u32(manager_object + entry_list) == 0) {
        const auto object = invoke(c, callee::allocate_object, 0x10186d04, {0x10});
        c.set(27, object);
        if (object != 0) invoke(c, callee_list::construct, 0x10186d10, {object});
        c.put32(manager_object + entry_list, object);
    }
    const auto list = c.u32(manager_object + entry_list);
    c.put32(list + list_cursor, c.u32(list + list_head));
    for (;;) {
        const auto entry = invoke(c, callee_list::next, 0x10186dc8, {c.u32(manager_object + entry_list)});
        if (entry == 0) {
            // The new entry is built in the frame and copied as 8 bytes; the last two are never written.
            c.put32(frame.at(0x38), c.u32(key));
            c.put8(frame.at(0x3d), c.u8(strength_cell));
            c.put8(frame.at(0x3c), c.u8(kind_cell));
            invoke(c, callee_list::append, 0x10186df8, {c.u32(manager_object + entry_list), frame.at(0x38), 8});
            return;
        }
        if (c.u32(entry) != c.u32(key)) continue;
        if (c.i8(entry + 4) == c.i8(kind_cell)) {
            const std::int32_t sum = c.i8(entry + 5) + c.i8(strength_cell);
            if (sum > 100) c.put8(entry + 5, 100);
            else c.put8(entry + 5, c.u8(entry + 5) + std::uint32_t(std::int32_t(c.i8(strength_cell))));
        } else {
            const auto strength = c.u8(strength_cell);
            if (std::int8_t(strength) > c.i8(entry + 5)) {
                c.put8(entry + 5, strength);
                c.put8(entry + 4, c.u8(kind_cell));
            }
        }
        c.put8(kind_cell, c.u8(entry + 4));
        c.put8(strength_cell, c.u8(entry + 5));
        return;
    }
}

std::uint32_t find_entry(Cpu& c, std::uint32_t manager_object, std::uint32_t key) {
    Frame frame(c, 0x10186e10, 0x40, {.first_gpr = 30});
    const auto list = c.u32(manager_object + entry_list);
    if (list == 0) return 0;
    c.set({{30, manager_object}, {31, key}});
    c.put32(list + list_cursor, c.u32(list + list_head));
    for (;;) {
        const auto entry = invoke(c, callee_list::next, 0x10186e70, {c.u32(manager_object + entry_list)});
        if (entry == 0) return 0;
        if (c.u32(entry) == c.u32(key)) return entry;
    }
}

std::uint32_t pick_injury_type(Cpu& c, std::uint32_t manager_object, std::uint32_t kind, std::uint32_t severity,
                               std::uint32_t person) {
    Frame frame(c, 0x10188930, 0xa0, {.first_gpr = 15});
    Registers reg(c);
    reg[15] = person; reg[16] = kind; reg[17] = manager_object; reg[19] = severity;
    reg[21] = no_injury; reg[22] = no_injury; reg[23] = 100;
    reg[25] = c.toc(toc::injury_types); reg[26] = c.toc(toc::type_ranges); reg[27] = c.toc(toc::floats);
    reg[28] = c.toc(sample_table::base); reg[29] = c.toc(sample_table::first);
    reg[30] = c.toc(sample_table::last); reg[31] = c.toc(sample_table::cursor);
    reg.sync();
    c.link(0x10188980);
    if (const auto current = current_injury(c, manager_object, person); current != 0) {
        const auto type = c.u8(current);
        // The minor kinds go on to choose again; any other current injury keeps its type.
        const bool minor = type == 0 || type == 1 || type == 2 || type == 3 || type == 4 || type == 0x4c ||
                           type == 0x4d || type == 0x4f || type == 0x4e || type == 0x50 || type == 0x51;
        if (!minor) return type;
    }
    if (person != 0) {
        const auto raw = c.u8(status_row(c, manager_object, person) + 0x14);
        if (std::int8_t(raw) != -1) {
            c.link(0x10188a1c);
            if (std::int8_t(kind) == std::int8_t(type_class(c, std::int8_t(raw))))
                reg[21] = c.u8(status_row(c, manager_object, person) + 0x14);
        }
    }
    const auto ranges = c.toc(toc::type_ranges), types = c.toc(toc::injury_types);
    const std::int32_t kind8 = std::int8_t(kind), third8 = std::int8_t(severity);
    const auto first_type = c.u8(ranges + std::uint32_t(kind8 * 3) + 1);
    const auto last_at = ranges + std::uint32_t(kind8 * 3 + 2);
    reg[17] = std::uint32_t(third8);
    reg[18] = std::uint32_t(kind8 * 3 + 2);
    reg[20] = first_type;
    std::uint32_t offset = std::uint32_t(std::int32_t(std::int8_t(first_type)) * 12);
    reg[24] = offset;
    std::uint32_t type = first_type, best = no_injury;
    std::int32_t best_value = 100;
    while (std::int8_t(type) <= c.i8(last_at)) {
        // The allowed-kind byte is read only when the severity is not -1 (0x10188a68).
        bool fits = third8 == -1;
        if (!fits) {
            const std::int32_t allowed = c.i8(types + offset + 10);
            fits = third8 == allowed || allowed == -1;
        }
        if (fits && third8 != 3) {
            std::int32_t value;
            const auto hold = [&] { reg.sync(); };
            if (std::int8_t(type) == std::int8_t(reg[21])) {
                const auto sample = draw_sample(c, 0x10188ac8, hold);
                value = std::int8_t(std::int32_t(sample) % 3 - 1);
            } else {
                const std::int32_t range = c.i8(types + offset + 2);
                reg[16] = std::uint32_t(range);
                std::int32_t roll = 0;
                if (range != 0) {
                    const auto sample = draw_sample(c, 0x10188b44, hold);
                    // 0x10188b6c..0x10188c3c draw over a range past 0xffff: the range is a byte, so never.
                    roll = remainder_in(sample, range);
                }
                reg[15] = std::uint32_t(roll);
                value = std::int8_t(roll);
            }
            if (value < best_value) {
                best_value = value;
                reg[23] = std::uint32_t(value);
                best = type;
                reg[22] = type;
            }
        }
        offset += 12;
        reg[24] = offset;
        ++type;
        reg[20] = type;
    }
    return best;
}

void match_injuries(Cpu& c, std::uint32_t manager_object, std::uint32_t options, std::uint32_t match) {
    Frame frame(c, 0x10186fc0, 0xb0, {.first_gpr = 13});
    Registers reg(c);  // r18 is read before it is written (0x1018735c): it starts as the caller's
    guest::Fpu fpu(c);
    const auto floats = c.toc(toc::floats);
    reg[26] = floats; reg[27] = c.toc(sample_table::base); reg[28] = c.toc(sample_table::first);
    reg[29] = c.toc(sample_table::last); reg[30] = c.toc(sample_table::cursor);
    reg[13] = c.toc(toc::people); reg[31] = c.toc(cell::match_date); reg[14] = c.toc(toc::today);
    reg[15] = manager_object; reg[16] = options; reg[17] = match;
    const auto hold = [&] { reg.sync(); };
    std::uint32_t side_offset = 0;
    reg[24] = 0;
    c.put8(frame.at(0x58), 0);
    for (std::int32_t side = 0; side < 2; ++side) {
        std::uint32_t slot_offset = 0;
        std::uint32_t slot = 0;
        reg[22] = 0;
        reg[25] = 0;
        for (; std::int8_t(slot) < 20; ++slot, slot_offset += 0x4c) {
            reg[22] = slot;
            reg[25] = slot_offset;
            const auto entry = match + side_offset + slot_offset;
            reg[23] = entry;
            const auto person_index = c.u32(entry + 0x194);
            reg[20] = person_index;
            if (person_index == no_injury) continue;
            const auto person = c.u32(c.toc(toc::people)) + person_index * person_size;
            reg[19] = person;
            if (person != 0 && c.i32(person) >= c.i32(c.toc(toc::people_count))) continue;
            const std::int32_t slot8 = std::int8_t(slot);
            std::int32_t base;
            if (c.i8(match + 0x4c) != 0 && slot8 < 11) {
                base = 0x5a;
            } else if (slot8 < 11) {
                if (const auto x = c.i8(entry + 0x1c8); x > -1) base = x;
                else if (const auto y = c.i8(entry + 0x1c4); y > -1) base = y;
                else base = std::int8_t(c.i16(match + 0x4e));
            } else {
                const auto z = c.u8(entry + 0x1c6);
                if (std::int8_t(z) == -1) base = 0;
                else if (const auto x = c.u8(entry + 0x1c8); std::int8_t(x) > -1) base = std::int8_t(x - z);
                else if (const auto y = c.u8(entry + 0x1c4); std::int8_t(y) > -1) base = std::int8_t(y - z);
                else base = std::int8_t(std::uint32_t(std::int32_t(c.i16(match + 0x4e))) - z);
            }
            const std::int32_t stamina = c.i16(entry + 0x1ca);
            const bool full_time = c.i8(options + 0x40) != 0;
            std::int32_t rating = std::int16_t(std::int8_t(base) + (10000 - stamina) / 100);
            reg[21] = std::uint32_t(rating);
            if (!full_time) {
                rating = std::int16_t(rating / 2);
            } else {
                const auto sample = draw_sample(c, 0x1018717c, hold);
                const std::int32_t trait = std::int8_t(c.u16(person + person::form));
                if (trait > std::int32_t(sample) % 3 + 0x21) {
                    const double scaled = fpu.fmul(c.f64(floats + pool::rating_factor), to_double(fpu, c, floats, rating));
                    rating = std::int32_t(std::uint32_t(fpu.fctiwz(0, scaled)));
                } else {
                    const auto again = draw_sample(c, 0x10187214, hold);
                    const std::int32_t now = std::int8_t(c.u16(person + person::form));
                    if (now < 0x17 - std::int32_t(again) % 5) rating = std::int16_t(std::uint32_t(rating) << 1);
                }
            }
            // Before the match's date the rating counts for nothing.
            const auto today = c.toc(toc::today), match_date = c.toc(cell::match_date);
            const auto today_a = c.u32(today);
            const std::uint32_t today_day = std::uint16_t(today_a), match_day = c.u16(match_date + date::year_part);
            bool before;
            if (today_day < match_day) before = true;
            else if (today_day > match_day) before = false;
            else before = halfword_greater(c.i16(match_date + date::day_part), std::int16_t(today_a >> 16));
            c.put32(frame.at(0x40), today_a);
            c.put32(frame.at(0x44), c.u32(today + 4));
            if (before) rating = 0;
            reg[21] = std::uint32_t(rating);
            reg.sync();
            c.link(0x101872dc);
            set_sharpness(c, manager_object, person, std::uint32_t(stamina), std::uint32_t(rating));
            if (c.i8(entry + 0x1b3) == 0) continue;
            const auto list = c.u32(manager_object + entry_list);
            if (list == 0) continue;
            c.put32(list + list_cursor, c.u32(list + list_head));
            for (;;) {
                reg.sync();
                const auto found = invoke(c, callee_list::next, 0x10187400, {c.u32(manager_object + entry_list)});
                if (found == 0) break;
                if (c.u32(found) != person_index) continue;
                const auto kind = c.u8(found + 4);
                if (kind == 7 && c.i8(c.u32(person + person::profile) + 0xf) < 15) continue;
                const std::int32_t strength = c.i8(found + 5);
                if (strength > 0x5f) reg[18] = 2;
                else if (strength > 0x41) reg[18] = 1;
                else if (strength > 0x32) reg[18] = 0;
                reg.sync();
                c.link(0x10187370);
                const auto type = pick_injury_type(c, manager_object, kind, reg[18], person);
                reg[21] = type;
                if (std::int8_t(type) <= -1) continue;
                reg.sync();
                c.link(0x10187390);
                close_injury(c, manager_object, person, 0);
                const bool no_option = c.i32(options) == -2;
                c.put32(frame.at(0x38), 0);
                invoke_unit(c, 0x10188cc0, no_option ? 0x101873c8 : 0x101873f8,
                            {manager_object, type, 0, person, no_option ? 0 : options, 1, 0xffffffff, 0});
            }
        }
        side_offset += 0x664;
        reg[24] = side_offset;
        c.put8(frame.at(0x58), std::uint32_t(side + 1));
    }
}

std::int32_t random_below(Cpu& c, std::int32_t n) {
    Frame frame(c, 0x101864a0, 0x80, {.first_gpr = 20});
    if (n == 0) return 0;
    guest::Fpu fpu(c);
    const auto pool_address = c.toc(toc::floats);
    struct Level { std::int32_t range = 0, partial = 0; } level[4];
    constexpr std::uint32_t redraw_back[4] = {0x101864f4, 0x101865e0, 0x101866cc, 0x101867b8};
    // The registers the original holds when a draw at depth d redraws.
    const auto hold = [&](int depth) {
        c.set({{25, pool_address}, {26, c.toc(sample_table::base)}, {27, c.toc(sample_table::first)},
               {28, c.toc(sample_table::last)}, {29, c.toc(sample_table::cursor)}, {30, std::uint32_t(n)}});
        if (depth >= 1) c.set({{22, std::uint32_t(level[1].range)}, {31, std::uint32_t(level[0].partial)}});
    };
    std::int32_t range = n, result = 0;
    int depth = 0;
    for (;; ++depth) {
        // The original has three more inline levels (0x101865c4..0x10186878) and a call of itself (0x1018687c). None
        // can run: a level goes on past its draw only for a range outside +-0xffff, and the next range is
        // `range * 10 / 0xffff` with the product wrapping in a word, which lies in -32768..32768 for every word
        // (a product within +-2^31 over 65535 is at most 32768 in size). So the second level always draws once.
        if (depth == 2) throw std::logic_error("random below: a range past 0xffff after two levels");
        level[depth].range = range;
        if (range == 0) {
            result = 0;
            break;
        }
        const auto sample = draw_sample(c, redraw_back[depth], [&] { hold(depth); });
        if (fits_one_draw(range)) {
            result = remainder_in(sample, range);
            break;
        }
        const double scale = fpu.fdiv(to_double(fpu, c, pool_address, range), c.f64(pool_address));
        const std::int32_t rest = std::int32_t(sample) % 0xffff;
        const double part = fpu.fmul(scale, to_double(fpu, c, pool_address, rest));
        level[depth].partial = std::int32_t(std::uint32_t(fpu.fctiwz(0, part)));
        range = std::int32_t(std::uint32_t(range) * 10u) / 0xffff;
    }
    for (int d = depth - 1; d >= 0; --d) {
        std::int32_t part = std::int32_t(std::uint32_t(level[d].partial) +
                                         std::uint32_t(std::int32_t(std::uint32_t(result) + 5u) / 10));
        const std::int32_t limit = std::int32_t(std::uint32_t(level[d].range) - 1u);
        if (part > limit) part = limit;
        result = part;
    }
    return result;
}
}
