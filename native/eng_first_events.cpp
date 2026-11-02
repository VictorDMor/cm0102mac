#include "eng_first_detail.hpp"
#include "english_competition_rules.hpp"

namespace cm::aot::eng_first {
using namespace detail;

std::uint32_t club_event(Cpu& c, std::uint32_t address, std::uint32_t club,
                         std::uint32_t event, std::uint32_t stage, std::uint32_t changed_out) {
    Frame f(c, 0x10529470, 0x60, {.first_gpr = 25});
    Kept<31> news(c, c.toc(-0x7b48)); Kept<25> object(c, address);
    Kept<26> selected_club(c, club); Kept<27> kind(c, event); Kept<28> output(c, changed_out);
    Program p{c}; const Competition league{c, address};
    if (compare(c, std::int8_t(stage), -1) == 0) {
        c.set(3, std::uint32_t(std::int32_t(std::int8_t(std::uint32_t(kind)))));
        if (compare(c, c.r(3), 5) == 0)
            p.call(0x10381b70, 0x105294b8, {c.u32(news), selected_club, league.definition()});
        else if (compare(c, std::int8_t(std::uint32_t(kind))) == 0)
            p.call(0x103822e0, 0x105294d4, {c.u32(news), selected_club, league.definition(), 100});
        else if (compare(c, c.r(3), 1) == 0)
            p.call(0x10384500, 0x105294f8, {c.u32(news), selected_club, league.definition(), 160, 130, 30});
        else if (compare(c, c.r(3), 3) == 0)
            p.call(0x10382e50, 0x10529510, {c.u32(news), selected_club, league.definition()});
    } else {
        Kept<29> index(c, 0); Kept<30> offset(c, 0);
        while (compare(c, index, league.club_count()) < 0) {
            c.set(5, league.standings());
            if (compare_logical(c, c.u32(c.r(5) + std::uint32_t(offset)), selected_club) == 0) {
                c.set(3, std::uint32_t(std::int32_t(std::int8_t(std::uint32_t(kind)))));
                if (compare(c, c.r(3), 1) == 0) {
                    p.call(0x103822e0, 0x10529550, {c.u32(news), selected_club, league.definition(), 50});
                    c.set({{4, league.standings()}, {5, 0}});
                    c.set(3, c.r(4) + std::uint32_t(index) * 0x44);
                    c.put8(c.r(3) + 0x40, 0); c.put8(output, 1);
                    break;
                }
                if (compare(c, std::int8_t(std::uint32_t(kind))) == 0)
                    p.call(0x10384500, 0x10529598, {c.u32(news), selected_club, league.definition(), 160, 150, 15});
                else if (compare(c, c.r(3), -1) == 0 || compare(c, c.r(3), 2) == 0) {
                    c.set({{4, 0xfffffffe}, {3, c.r(5) + std::uint32_t(index) * 0x44}});
                    c.put8(c.r(3) + 0x40, c.r(4));
                    break;
                }
            }
            offset = std::uint32_t(offset) + 0x44; index = std::uint32_t(index) + 1;
        }
    }
    return 0;
}

void describe_fixture(Cpu& c, std::uint32_t address, std::uint32_t club_index,
                       std::uint32_t kind, std::uint32_t flags) {
    Frame f(c, 0x105295f0, 0x60, {.first_gpr = 28});
    Kept<30> clubs(c, c.toc(slot::clubs)); Kept<29> buffer(c, c.toc(-0x7bb8));
    const auto option = c.u8(f.caller() + 0x3b);
    const auto detail = c.u8(f.caller() + 0x3f);
    Kept<31> message(c, c.u32(f.caller() + 0x40));
    c.set(10, detail); Program p{c};
    if (compare(c, std::int8_t(kind), 1) == 0 && compare(c, std::int8_t(flags)) == 0 &&
        compare(c, std::int8_t(detail)) == 0) {
        Kept<28> club_offset(c, club_index * 0x248);
        const auto club = c.u32(clubs) + std::uint32_t(club_offset);
        const auto text_kind = std::uint32_t(std::int32_t(c.i8(club + 0x23b)));
        p.call(0x10727890, 0x1052966c, {buffer, c.toc(slot::pool) + 0x21, text_kind, text_kind, club + 0x221});
        p.call(0x1074b830, 0x10529678, {message, buffer});
        const auto current_club = c.u32(clubs) + std::uint32_t(club_offset);
        p.call(0x1074bab0, 0x10529690, {message, current_club + 0x221, 2005, current_club});
    } else {
        c.put32(f.at(0x38), option); c.put32(f.at(0x3c), detail); c.put32(f.at(0x40), message);
        c.set(10, 0);
        p.call(0x10450030, 0x105296a8, {address, club_index, kind, flags});
    }
}

void set_fixture_places(Cpu& c, std::uint32_t address, std::uint32_t fixture,
                        std::uint32_t stage, std::uint32_t first,
                        std::uint32_t second, std::uint32_t third) {
    Frame f(c, 0x105296d0, 0x50, {.first_gpr = 28});
    Kept<28> saved_stage(c, stage);
    Kept<29> first_place(c, first);
    Kept<30> second_place(c, second);
    Kept<31> third_place(c, third);
    Program p{c};
    const auto record = p.call(0x10459b50, 0x10529704, {c.u32(address + 8), fixture});
    if (english_rules::expanded_season(c, address)) {
        if (std::int8_t(std::uint32_t(saved_stage)) == -1) {
            const auto in_playoff = [](unsigned place) { return place >= 4 && place <= 11; };
            if (in_playoff(std::uint8_t(std::uint32_t(second_place)))) second_place = 4;
            if (in_playoff(std::uint8_t(std::uint32_t(third_place)))) third_place = 11;
        } else if (std::int8_t(std::uint32_t(saved_stage)) == 0) {
            const auto remap = [](auto& place) {
                const auto rank = std::uint8_t(std::uint32_t(place));
                if (rank >= 1 && rank <= 8) place = rank + english_rules::expanded.automatic;
            };
            remap(first_place); remap(second_place); remap(third_place);
        }
        c.put8(record + 0x76, first_place);
        c.put8(record + 0x77, second_place);
        c.put8(record + 0x78, third_place);
        return;
    }
    if (compare(c, std::int8_t(std::uint32_t(saved_stage)), -1) == 0) {
        const auto second_byte = std::uint8_t(std::uint32_t(second_place));
        if (compare(c, second_byte, 7) < 0 && compare(c, second_byte, 3) >= 0) second_place = 3;
        const auto third_byte = std::uint8_t(std::uint32_t(third_place));
        if (compare(c, third_byte, 7) < 0 && compare(c, third_byte, 3) >= 0) third_place = 6;
    } else if (compare(c, std::int8_t(std::uint32_t(saved_stage))) == 0) {
        // Playoff ranks 1..3 become league ranks 3..5; all other bytes survive.
        const auto remap = [&](auto& place) {
            const auto rank = std::uint8_t(std::uint32_t(place));
            if (compare(c, rank, 2) == 0) place = 4;
            else if (rank < 2) { if (compare(c, rank, 1) >= 0) place = 3; }
            else if (compare(c, rank, 4) < 0) place = 5;
        };
        remap(second_place); remap(third_place); remap(first_place);
    }
    c.put8(record + 0x76, first_place);
    c.put8(record + 0x77, second_place);
    c.put8(record + 0x78, third_place);
}
}
