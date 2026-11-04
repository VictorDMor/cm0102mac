#include "transfer_manager_schedule.hpp"
#include "guest_values.hpp"

namespace cm::aot::transfer_manager {
namespace {
using namespace guest;
constexpr std::uint32_t date_after_days = 0x10717890, days_since_origin = 0x10718130,
    append_to_list = 0x10738d10;

void date_copy(Cpu& c, std::uint32_t to, std::uint32_t from) {
    const auto year = c.u16(from + 2);
    c.set(0, year);
    c.put16(to + 2, year);
    const auto ordinal = c.i16(from);
    c.set(0, std::uint32_t(ordinal));
    c.put16(to, std::uint32_t(ordinal));
    const auto leap = c.u8(from + 4);
    c.set(0, leap);
    c.put8(to + 4, leap);
}

void report_excessive_delay(Cpu& c, const Frame& frame, std::uint32_t pool, std::uint32_t message) {
    const auto file = frame.at(0x50), extension = frame.at(0x48);
    c.set({{3, pool + 0x37}, {4, 0}, {5, 0}, {6, file}, {7, extension}});
    c.call(0x10739890, 0x103eb51c);
    c.call(0x10768810, 0x103eb520);
    const auto version = c.r(3);
    c.set({{3, message}, {4, pool + 0x6ff}, {5, version}, {6, file}, {7, extension}, {8, 0x18b8}});
    c.call(0x1077b720, 0x103eb53c);
    c.set({{3, pool + 0x70c}, {4, message}, {5, 0}});
    c.call(0x10769b60, 0x103eb54c);
    const auto valid = c.toc(-0x7bac);
    c.set({{0, 0}, {3, valid}, {5, 10}});
    c.put8(valid, 0);
}

void advance(Cpu& c, std::uint32_t to, std::uint32_t from, std::int32_t days, std::uint32_t back) {
    c.set({{3, to}, {4, from}, {5, std::uint32_t(days)}});
    c.call(date_after_days, back);
}
}

void schedule_transfer_offer(Cpu& c, std::uint32_t manager, std::uint32_t offer,
                             std::uint32_t days, std::uint32_t phases) {
    Frame frame(c, 0x103eb4a0, 0x90, {.first_gpr = 25});
    c.set(0, c.lr());
    const Kept<25> today(c, c.toc(-0x7b80));
    const Kept<26> message(c, c.toc(-0x7ba8));
    const Kept<27> pool(c, c.toc(-0x4b9c));
    const Kept<28> transfers(c, manager);
    const Kept<29> scheduled(c, offer);
    const Kept<30> phase_delay(c, phases);
    const Kept<31> current_phase(c, c.toc(-0x79ec));

    const auto origin = transfers + 0x97c;
    c.set(0, c.u16(origin + 2));
    if (!nonzero_logical(c, c.r(0))) {
        date_copy(c, origin, today);
        const auto phase = c.u8(current_phase);
        c.set(0, phase);
        c.put8(transfers + 0x984, phase);
    }

    auto delay = std::int32_t(std::int8_t(days));
    c.set(0, std::uint32_t(delay));
    if (compare(c, delay, 50) >= 0) {
        report_excessive_delay(c, frame, pool, message);
        delay = 10;
    }
    const auto due = scheduled + 0x14;
    const auto phase_field = scheduled + 0x1c;
    advance(c, frame.at(0x40), today, delay, 0x103eb56c);
    date_copy(c, due, frame.at(0x40));
    const auto first_phase = c.u8(current_phase) + std::uint32_t(phase_delay);
    c.set(0, first_phase);
    c.put8(phase_field, first_phase);
    for (;;) {
        const auto phase = c.i8(phase_field);
        c.set(0, std::uint32_t(phase));
        if (compare(c, phase, 2) <= 0) break;
        c.set(0, std::uint32_t(phase - 3));
        c.put8(phase_field, phase - 3);
        advance(c, frame.at(0x38), due, 1, 0x103eb5b0);
        date_copy(c, due, frame.at(0x38));
    }

    c.set({{3, due}, {4, c.u32(origin)}, {5, c.u32(origin + 4)}});
    c.call(days_since_origin, 0x103eb5e8);
    const auto raw_day = std::uint32_t(c.i8(transfers + 0x978)) + c.r(3);
    c.set(0, raw_day);
    auto slot_day = std::int32_t(std::int8_t(raw_day));
    for (;;) {
        c.set({{3, std::uint32_t(slot_day)}, {4, std::uint32_t(slot_day)}});
        if (compare(c, slot_day, 50) < 0) break;
        slot_day -= 50;
    }
    const auto phase_offset = std::uint32_t(c.i8(phase_field)) * 16;
    const auto slot = transfers + std::uint32_t(slot_day) * 48 + phase_offset + 0x18;
    c.set({{0, phase_offset}, {3, slot}, {4, scheduled + 0x28}, {5, 4}});
    c.call(append_to_list, 0x103eb634);
}
}
