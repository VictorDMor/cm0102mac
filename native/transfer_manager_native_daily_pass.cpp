#include "transfer_manager_native_daily.hpp"
#include "transfer_manager_native_detail.hpp"

namespace cm::aot::transfer_manager_native {
using guest::Cpu;
using guest::Frame;
namespace {
constexpr std::uint32_t call_vector = 0x10781320, nation_dates = 0x103ec0f0, due_offers = 0x103f3100, set_stage = 0x103eb4a0,
                        recipients = 0x103e64e0, process_one = 0x103eb650, date_step = 0x10717890, month_of = 0x10718360,
                        month_rollover = 0x103f2250, year_start = 0x103e92b0, weekly = 0x103ecc20, date_at = 0x107156b0,
                        person_closed = 0x100ca6a0, list_next = 0x10738e00, list_reset = 0x10738ca0;
// The manager's clock: the date of the pass last run (high halfword +0x97c, low +0x97e, byte +0x980) and its phase +0x984.
constexpr std::uint32_t clock_hi = 0x97c, clock_lo = 0x97e, clock_byte = 0x980, clock_phase = 0x984, clock_slot = 0x978;
constexpr std::uint32_t offers_count = 4, offer_hi = 0x14, offer_lo = 0x16, offer_phase = 0x1c, offer_stage = 0x34;
constexpr std::uint32_t person_hi = 0x4c, person_lo = 0x4e, person_link = 0x24, person_target = 0x10;
constexpr std::uint32_t nation_objects = 0x9e0, nation_count = 27;
}

void run_daily_pass(Cpu& c, std::uint32_t manager, std::uint32_t date, std::uint32_t phase_register) {
    Frame frame(c, 0x103ea840, 0xc0, {.first_gpr = 20});
    keep(c, 20, manager);
    keep(c, 21, date);
    keep(c, 22, phase_register);
    const auto before_cell = c.toc(-0x758c), cell_a = c.toc(-0x7594), cell_b = c.toc(-0x7a14), cell_c = c.toc(-0x79fc),
               cell_d = c.toc(-0x7a18), cell_e = c.toc(-0x7a70), people_count = c.toc(-0x7be4);
    keep(c, 25, before_cell);
    keep(c, 26, cell_a);
    keep(c, 27, cell_b);
    keep(c, 28, cell_c);
    keep(c, 29, cell_d);
    keep(c, 30, cell_e);
    keep(c, 31, people_count);
    const auto phase = std::int32_t(std::int8_t(phase_register));
    // The first pass ever stamps the clock with this date and phase.
    if (c.u16(manager + clock_lo) == 0) {
        c.put16(manager + clock_lo, c.u16(date + 2));
        c.put16(manager + clock_hi, c.u16(date));
        c.put8(manager + clock_byte, c.u8(date + 4));
        c.put8(manager + clock_phase, phase_register);
    }
    // The original copies the date into frame slots, and later routines read leftovers there.
    c.put32(frame.at(0x60), c.u32(date));
    c.put32(frame.at(0x64), c.u32(date + 4));
    // Only the pass the clock stands at runs.
    if (!(c.u16(date + 2) == c.u16(manager + clock_lo) && c.i16(date) == c.i16(manager + clock_hi))) return;
    if (std::int32_t(std::int8_t(c.u8(manager + clock_phase))) != phase) return;
    if (phase == 0) invoke_native(c, nation_dates, 0x103ea90c, {manager, date});
    const auto on_a_week = [&] { return c.i16(date) % 7 == 0; };
    if (on_a_week()) {
        // Offers whose own date has passed and are not closed, waiting or answered start the next stage.
        std::uint32_t index = 0;
        keep(c, 23, 0);
        keep(c, 24, 0);
        for (; std::int32_t(index) < c.i32(manager + offers_count); ++index) {
            keep(c, 23, index);
            keep(c, 24, index * 4);
            const auto offer = c.u32(c.u32(manager) + index * 4);
            if (offer == 0) continue;
            c.put32(frame.at(0x58), c.u32(date));
            c.put32(frame.at(0x5c), c.u32(date + 4));
            if (!daily::before(c.i16(offer + offer_hi), c.u16(offer + offer_lo), c.i16(date), c.u16(date + 2))) continue;
            const auto stage = std::int32_t(std::int8_t(c.u8(offer + offer_stage)));
            if (stage == 0x13 || stage == 7 || stage == 8 || stage == 0xa) continue;
            invoke(c, set_stage, 0x103ea9e8, {manager, offer, 1, 0});
        }
        keep(c, 23, index);
        keep(c, 24, index * 4);
    }
    c.put8(manager + clock_phase, c.u8(manager + clock_phase) + 1);
    invoke_native(c, recipients, 0x103eaa10, {manager});
    const auto shift = std::uint32_t(phase) << 4;
    keep(c, 24, shift);
    // The offer lists of this phase (16 bytes each, three per slot): [+4] the start, [+0xc] the cursor.
    const auto list_of = [&] {
        return manager + 0x18 + std::uint32_t(std::int32_t(std::int8_t(c.u8(manager + clock_slot)))) * 0x30 + shift;
    };
    c.put32(list_of() + 0xc, c.u32(list_of() + 4));
    for (;;) {
        const auto entry = invoke(c, list_next, 0x103eaae8, {list_of()});
        if (entry == 0) break;
        const auto number = c.i32(entry);
        if (number >= c.i32(manager + offers_count) || number < 0) continue;
        const auto slot = c.u32(manager) + std::uint32_t(number) * 4;
        const auto offer = c.u32(slot);
        if (offer == 0) continue;
        c.put32(frame.at(0x50), c.u32(date));
        c.put32(frame.at(0x54), c.u32(date + 4));
        if (!(c.u16(date + 2) == c.u16(offer + offer_lo) && c.i16(date) == c.i16(offer + offer_hi))) continue;
        if (std::int32_t(std::int8_t(c.u8(offer + offer_phase))) != phase) continue;
        invoke_native(c, process_one, 0x103eaacc, {manager, slot});
    }
    invoke(c, list_reset, 0x103eab0c, {list_of()});
    if (phase == 2) {
        invoke(c, date_step, 0x103eab28, {frame.at(0x70), manager + clock_hi, 1});
        c.put16(manager + clock_lo, c.u16(frame.at(0x72)));
        c.put16(manager + clock_hi, c.u16(frame.at(0x70)));
        c.put8(manager + clock_byte, c.u8(frame.at(0x74)));
        c.put8(manager + clock_phase, 0);
        c.put8(manager + clock_slot, c.u8(manager + clock_slot) + 1);
        if (std::int32_t(std::int8_t(c.u8(manager + clock_slot))) >= 0x32) c.put8(manager + clock_slot, 0);
        invoke(c, date_step, 0x103eab78, {frame.at(0x68), date, 1});
        const auto month_then = invoke(c, month_of, 0x103eab80, {frame.at(0x68)});
        keep(c, 23, std::uint32_t(std::int32_t(std::int8_t(month_then))));
        const auto month_now = invoke(c, month_of, 0x103eab8c, {date});
        if (std::int8_t(month_now) != std::int8_t(month_then)) invoke(c, month_rollover, 0x103eaba0, {manager});
        if (c.i16(date) == 0) {
            invoke(c, year_start, 0x103eabb4, {manager});
            invoke(c, weekly, 0x103eabbc, {manager});
        }
        keep(c, 24, 0);
        keep(c, 23, 0);
        for (std::uint32_t k = 0; std::int32_t(std::int8_t(k)) < std::int32_t(nation_count); ++k) {
            keep(c, 23, k * 4);
            keep(c, 24, k);
            const auto object = c.u32(manager + nation_objects + k * 4);
            if (object == 0) continue;
            c.set({{3, object}, {4, date}});
            c.set(12, c.u32(c.u32(object) + 8));
            c.call(call_vector, 0x103eabec);
            c.set(2, c.u32(frame.at(0x14)));
        }
        keep(c, 23, nation_count * 4);
        keep(c, 24, nation_count);
        if (on_a_week()) invoke(c, weekly, 0x103eac38, {manager});
    }
    if (phase == 0) invoke_native(c, due_offers, 0x103eac48, {manager});
    if (c.i16(date) == 100 && phase == 2) {
        const auto lo = std::uint32_t(c.u16(date + 2));
        invoke(c, date_at, 0x103eac78, {frame.at(0x88), 100, (lo - 5) & 0xffff});
        invoke(c, date_at, 0x103eac90, {frame.at(0x80), 100, (lo - 2) & 0xffff});
        invoke(c, date_at, 0x103eaca8, {frame.at(0x78), 100, (lo - 10) & 0xffff});
        auto person = c.u32(c.toc(-0x7be0));
        keep(c, 24, 0);
        keep(c, 23, person);
        const auto moment_before = [&](std::uint32_t at) {
            return daily::before(c.i16(person + person_hi), c.u16(person + person_lo), c.i16(at), c.u16(at + 2));
        };
        // Each test copies a date buffer's two words into a frame slot first.
        const auto copy_then_before = [&](std::uint32_t buffer, std::uint32_t slot) {
            c.put32(frame.at(slot), c.u32(frame.at(buffer)));
            c.put32(frame.at(slot + 4), c.u32(frame.at(buffer + 4)));
            return moment_before(frame.at(buffer));
        };
        for (std::uint32_t n = 0; std::int32_t(n) < c.i32(people_count); ++n, person += person_size) {
            keep(c, 24, n);
            keep(c, 23, person);
            c.put32(frame.at(0x48), c.u32(frame.at(0x80)));
            c.put32(frame.at(0x4c), c.u32(frame.at(0x84)));
            if (!moment_before(frame.at(0x80))) continue;
            const auto link = c.u32(person + person_link);
            if (link == 0) continue;
            const auto nation = c.u32(link + 0x2c);
            if (nation == 0) continue;
            const auto record = c.u32(nation + 0x74);
            if (record == 0) continue;
            if (c.i32(record + 8) != c.i32(cell_e)) continue;
            if (std::int8_t(invoke(c, person_closed, 0x103ead50, {person, nation})) != 0) continue;
            const auto target = c.u32(c.u32(person + person_link) + 0x2c);
            const auto kind = c.i32(target + 0x68);
            if (kind == c.i32(cell_d) || kind == c.i32(cell_c)) {
                c.put32(person + person_target, target);
            } else if (copy_then_before(0x88, 0x40) && kind != c.i32(cell_b) && kind != c.i32(cell_a) &&
                       kind != c.i32(before_cell)) {
                c.put32(person + person_target, target);
            } else if (copy_then_before(0x78, 0x38)) {
                c.put32(person + person_target, target);
            }
        }
    }
}
}
