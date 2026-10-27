#include "contract_manager_detail.hpp"

namespace cm::aot::contract_manager {
using namespace detail;
namespace {
void publish_contract_news(Cpu& c, std::uint32_t entry, std::uint32_t template_id,
                            std::uint32_t person_address, std::uint32_t club_address) {
    Frame frame(c, entry, 0x130, {.first_gpr = 29});
    Kept<31> news(c, c.toc(-0x7bf0)); Kept<30> club(c, club_address); Kept<29> person(c, person_address);
    Program program(c);
    const auto interested = std::int8_t(program.call(0x102cdbf0, entry + 0x30, {c.u32(news), club}));
    c.set(0, std::uint32_t(std::int32_t(interested)));
    if (!nonzero(c, interested)) return;
    program.call(0x102bdf90, entry + 0x48, {frame.at(0x38), template_id, 0});
    program.call(0x102cb4b0, entry + 0x58, {frame.at(0x38), 0, c.u32(person)});
    for (unsigned slot = 1; slot <= 3; ++slot) {
        const auto competition = c.u32(person + 0x24 + slot * 4);
        program.call(0x102cb4b0, entry + 0x58 + slot * 0x14,
                     {frame.at(0x38), slot, c.u32(competition + 0x20)});
    }
    program.call(0x102cb4b0, entry + 0xa4, {frame.at(0x38), 4, c.u32(club)});
    program.call(0x102cc140, entry + 0xb4, {c.u32(news), frame.at(0x38), club});
}
void publish_pending_news(Cpu& c, std::uint32_t entry, std::uint32_t template_id, std::uint32_t assertion_line,
                           std::uint32_t contract_address, std::uint32_t related_value, bool outcome) {
    Frame frame(c, entry, 0x160, {.first_gpr = 28});
    Kept<31> pool(c, c.toc(slot::pool)); Kept<30> error_buffer(c, c.toc(-0x7ba8));
    Kept<29> related(c, related_value); Kept<28> contract(c, contract_address);
    const bool present = nonzero(c, std::int32_t(contract_address)); Program program(c);
    if (!present) {
        program.call(0x10739890, entry + 0x48, {pool + 0x37, 0, 0, frame.at(0x40), frame.at(0x38)});
        const auto version = program.call(0x10768810, entry + 0x4c);
        program.call(0x1077b720, entry + 0x68, {error_buffer, pool + 0x23b, version, frame.at(0x40), frame.at(0x38), assertion_line});
        program.call(0x10769b60, entry + 0x78, {pool + 0x248, error_buffer, 0});
        const auto enabled = c.toc(-0x7bac); c.set(3, enabled); c.set(0, 0); c.put8(enabled, 0); return;
    }
    program.call(0x102bdf90, entry + 0x98, {frame.at(0x60), template_id, 0});
    const auto club_id = c.u32(contract + 4); c.set(5, club_id);
    const auto clubs_holder = c.toc(-0x7be8); c.set(7, clubs_holder); c.set(4, 0);
    const auto person_id = c.u32(contract); c.set(0, person_id); c.set(6, club_id * 0x248);
    const auto clubs = c.u32(clubs_holder); c.set(7, clubs);
    const auto staff_holder = c.toc(-0x7be0); c.set(5, staff_holder);
    Kept<30> club(c, clubs + club_id * 0x248); c.set(0, person_id * Staff::size);
    const auto staff = c.u32(staff_holder); c.set(6, staff);
    const auto club_number = c.u32(club); c.set(5, club_number);
    Kept<31> person(c, staff + person_id * Staff::size);
    program.call(0x102cb4b0, entry + 0xd0, {frame.at(0x60), 0, club_number});
    program.call(0x102cb4b0, entry + 0xe0, {frame.at(0x60), 2, c.u32(person)});
    for (unsigned argument = 3; argument <= 5; ++argument) {
        const auto competition = c.u32(person + 0x1c + argument * 4);
        program.call(0x102cb4b0, entry + 0xe0 + (argument - 2) * 0x14,
                     {frame.at(0x60), argument, c.u32(competition + 0x20)});
    }
    if (outcome) {
        program.call(0x102cb4b0, entry + 0x12c, {frame.at(0x60), 6, related});
        const auto mood = c.i8(contract + 0x46); c.set(6, std::uint32_t(std::int32_t(mood)));
        const auto bits = std::uint32_t(std::int32_t(mood)) ^ 1u;
        c.s.wr<1>(guest::reg::xer_ca, std::int32_t(bits) < 0 && (bits & 1)); c.set(0, mood > 1);
        program.call(0x102cb4b0, entry + 0x15c, {frame.at(0x60), 7, mood > 1});
        program.call(0x102cc140, entry + 0x170, {c.u32(c.toc(-0x7bf0)), frame.at(0x60), club});
    } else {
        if (nonzero_logical(c, related))
            program.call(0x102cb4b0, entry + 0x134, {frame.at(0x60), 6, c.u32(related)});
        else program.call(0x102cb4b0, entry + 0x148, {frame.at(0x60), 6, 0xffffffff});
        program.call(0x102cc140, entry + 0x15c, {c.u32(c.toc(-0x7bf0)), frame.at(0x60), club});
    }
}
}
void publish_contract_renewal(Cpu& c, std::uint32_t person, std::uint32_t club) {
    publish_contract_news(c, 0x1008bfc0, 0xbc4, person, club);
}
void publish_contract_departure(Cpu& c, std::uint32_t person, std::uint32_t club) {
    publish_contract_news(c, 0x10091320, 0xbbc, person, club);
}
void publish_pending_club(Cpu& c, std::uint32_t contract, std::uint32_t club) {
    publish_pending_news(c, 0x1008c8d0, 0xbbe, 0x25d3, contract, club, false);
}
void publish_pending_outcome(Cpu& c, std::uint32_t contract, std::uint32_t outcome) {
    publish_pending_news(c, 0x1008f620, 0xbc0, 0x2b62, contract, outcome, true);
}

std::uint32_t preferred_contract_role(Cpu& c, std::uint32_t manager_address, std::uint32_t person_address) {
    Frame frame(c, 0x10089cc0, 0x40, {.first_gpr = 30});
    Kept<31> person(c, person_address); Kept<30> object(c, manager_address); Program program(c);
    const auto attributes = c.u32(person + 0x14); c.set(0, attributes);
    if (nonzero_logical(c, attributes)) {
        const auto nonplayer = c.u32(person + 0x1c); c.set(3, nonplayer);
        if (nonzero_logical(c, nonplayer)) {
            const auto first = std::int8_t(program.call(0x100d2f50, 0x10089cfc, {nonplayer, c.u8(nonplayer + 0x30)}));
            c.set(0, std::uint32_t(std::int32_t(first)));
            if (compare(c, first, 10) >= 0) return result(c, 15);
            const auto current = c.u32(person + 0x1c);
            const auto second = std::int8_t(program.call(0x100d2f50, 0x10089d14, {current, c.u8(current + 0x31)}));
            c.set(0, std::uint32_t(std::int32_t(second)));
            if (compare(c, second, 10) >= 0) return result(c, 15);
        }
        return result(c, 11);
    }
    const auto nonplayer = c.u32(person + 0x1c); c.set(0, nonplayer);
    if (!nonzero_logical(c, nonplayer)) return result(c, 9);
    const auto primary = c.u8(person + 0x62); c.set(4, primary);
    c.set(0, std::uint32_t(std::int32_t(std::int8_t(primary))));
    if (compare(c, std::int8_t(primary), 5) == 0 || compare(c, std::int8_t(primary), 12) == 0) return result(c, 6);
    const auto secondary = c.i8(person + 0x60); c.set(0, std::uint32_t(std::int32_t(secondary)));
    if (compare(c, secondary, 5) == 0 || compare(c, secondary, 12) == 0) return result(c, 6);
    const auto employer = c.u32(person + 0x24); c.set(0, employer);
    if (nonzero_logical(c, employer)) {
        const auto available = std::int8_t(program.call(0x1007e4f0, 0x10089d88, {object, primary, person}));
        c.set(0, std::uint32_t(std::int32_t(available)));
        if (nonzero(c, available)) return result(c, c.u8(person + 0x62));
    }
    const auto other_employer = c.u32(person + 0x20); c.set(0, other_employer);
    if (nonzero_logical(c, other_employer)) {
        const auto available = std::int8_t(program.call(0x1007e4f0, 0x10089db4, {object, c.u8(person + 0x60), person}));
        c.set(0, std::uint32_t(std::int32_t(available)));
        if (nonzero(c, available)) return result(c, c.u8(person + 0x60));
    }
    const auto available = std::int8_t(program.call(0x1007e4f0, 0x10089dd4, {object, c.u8(person + 0x62), person}));
    c.set(0, std::uint32_t(std::int32_t(available)));
    if (nonzero(c, available)) return result(c, c.u8(person + 0x62));
    const auto current = c.u32(person + 0x1c);
    const auto first = std::int8_t(program.call(0x100d2f50, 0x10089df0, {current, c.u8(current + 0x30)}));
    c.set(0, std::uint32_t(std::int32_t(first)));
    if (compare(c, first, 15) >= 0) return result(c, 8);
    const auto repeated = c.u32(person + 0x1c);
    const auto second = std::int8_t(program.call(0x100d2f50, 0x10089e08, {repeated, c.u8(repeated + 0x31)}));
    c.set(0, std::uint32_t(std::int32_t(second)));
    return result(c, compare(c, second, 15) >= 0 ? 8 : 9);
}
}
