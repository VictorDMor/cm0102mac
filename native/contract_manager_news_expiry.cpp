#include "contract_manager_detail.hpp"

namespace cm::aot::contract_manager {
using namespace detail;

void publish_expiring_contract(Cpu& c, std::uint32_t contract_address, std::uint32_t period_value,
                               std::uint32_t bosman_value, std::uint32_t distribute_value) {
    c.set(0, c.lr());
    Frame frame(c, 0x1008c270, 0x180, {.first_gpr = 24});
    Kept<24> contract(c, contract_address); const bool present = nonzero(c, std::int32_t(contract_address));
    Kept<30> news(c, c.toc(-0x7bf0)); Kept<31> clubs(c, c.toc(-0x7be8));
    Kept<26> period(c, period_value); Kept<25> person(c, c.toc(-0x7ba8));
    Kept<27> bosman(c, bosman_value); Kept<29> club(c, c.toc(slot::pool));
    Kept<28> distribute(c, distribute_value); Program program(c);
    if (!present) {
        program.call(0x10739890, 0x1008c2bc, {club + 0x37, 0, 0, frame.at(0x50), frame.at(0x38)});
        const auto version = program.call(0x10768810, 0x1008c2c0);
        program.call(0x1077b720, 0x1008c2dc,
            {person, club + 0x23b, version, frame.at(0x50), frame.at(0x38), 0x24b2});
        program.call(0x10769b60, 0x1008c2ec, {club + 0x248, person, 0});
        const auto success = c.toc(-0x7bac); c.set(3, success); c.set(0, 0); c.put8(success, 0);
        return;
    }
    program.call(0x102bdf90, 0x1008c30c, {frame.at(0x70), 0xbbd, 0});
    const auto club_id = c.u32(contract + 4); c.set(5, club_id);
    const auto person_id = c.u32(contract); c.set(0, person_id); c.set(4, 0);
    const auto club_offset = club_id * 0x248; c.set(6, club_offset);
    const auto club_records = c.u32(clubs); c.set(7, club_records);
    const auto staff_holder = c.toc(-0x7be0); c.set(5, staff_holder);
    club = club_records + club_offset;
    const auto staff_records = c.u32(staff_holder); c.set(6, staff_records);
    const auto person_offset = person_id * Staff::size; c.set(0, person_offset);
    const auto club_number = c.u32(club); c.set(5, club_number); person = staff_records + person_offset;
    program.call(0x102cb4b0, 0x1008c340, {frame.at(0x70), 0, club_number});
    program.call(0x102cb4b0, 0x1008c350, {frame.at(0x70), 2, c.u32(person)});
    for (unsigned slot = 3; slot <= 5; ++slot) {
        const auto competition = c.u32(person + 0x1c + slot * 4);
        program.call(0x102cb4b0, 0x1008c350 + (slot - 2) * 0x14,
            {frame.at(0x70), slot, c.u32(competition + 0x20)});
    }
    program.call(0x102cb4b0, 0x1008c39c,
        {frame.at(0x70), 6, std::uint32_t(std::int32_t(std::int8_t(period_value)))});
    program.call(0x102cb4b0, 0x1008c3ac,
        {frame.at(0x70), 7, std::uint32_t(std::int32_t(std::int8_t(bosman_value)))});
    const auto attributes = c.u32(person + 0x14); c.set(5, attributes);
    c.set(0, (0u - attributes) | attributes);
    program.call(0x102cb4b0, 0x1008c3c8, {frame.at(0x70), 8, attributes != 0});
    const auto division = c.u32(club + 0x2c); c.set(3, division);
    if (nonzero_logical(c, division)) {
        const auto allowed = std::int8_t(program.call(0x100d33d0, 0x1008c3d8));
        c.set(0, std::uint32_t(std::int32_t(allowed)));
        if (!nonzero(c, allowed)) distribute = 0;
    }
    program.call(0x102cc140, 0x1008c3f4, {c.u32(news), frame.at(0x70), club});
    const auto spread = std::int8_t(std::uint32_t(distribute)); c.set(0, std::uint32_t(std::int32_t(spread)));
    if (!nonzero(c, spread)) return;
    program.call(0x10738c00, 0x1008c404, {frame.at(0x40)});
    const auto registry_holder = c.toc(-0x7b40); c.set(3, registry_holder);
    const auto registry = c.u32(registry_holder); c.set(3, registry);
    const auto shortlist = c.u32(registry + 0x988);
    program.call(0x10362ef0, 0x1008c428, {shortlist, person, frame.at(0x40), 1, 1, 0});
    const auto head = c.u32(frame.at(0x44)); c.set(0, head); c.put32(frame.at(0x4c), head);
    for (;;) {
        const auto row = program.call(0x10738e00, 0x1008c48c, {frame.at(0x40)});
        period = row;
        if (!nonzero(c, std::int32_t(row))) break;
        const auto target_id = c.u32(row + 4); c.set(3, target_id);
        const auto current_id = c.u32(club); c.set(0, current_id);
        if (compare(c, std::int32_t(target_id), std::int32_t(current_id)) == 0) continue;
        const auto offset = target_id * 0x248; c.set(3, offset);
        const auto records = c.u32(clubs); c.set(4, records); c.set(0, offset + 0x2c);
        const auto target_division = c.u32(records + offset + 0x2c); c.set(3, target_division);
        if (!nonzero_logical(c, target_division)) continue;
        const auto allowed = std::int8_t(program.call(0x100d33d0, 0x1008c460));
        c.set(0, std::uint32_t(std::int32_t(allowed)));
        if (!nonzero(c, allowed)) continue;
        const auto latest_id = c.u32(period + 4); c.set(0, latest_id);
        const auto latest_clubs = c.u32(clubs); c.set(5, latest_clubs);
        c.set(0, latest_id * 0x248);
        program.call(0x102cc140, 0x1008c484, {c.u32(news), frame.at(0x70), latest_clubs + latest_id * 0x248});
    }
    program.call(0x10738c20, 0x1008c4a0, {frame.at(0x40), 0xffffffff});
}
}
