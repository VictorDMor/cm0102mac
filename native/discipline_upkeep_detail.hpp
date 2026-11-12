#pragma once
// DISCIPLINE.CPP upkeep block: the records and code outside the module its routines reach.
#include "discipline_table_detail.hpp"

namespace cm::aot::discipline {
namespace upkeep {
namespace toc {
inline constexpr std::int32_t clubs = -0x7be8, database = -0x7b7c, database_result = -0x7b78, news = -0x7bf0,
                              today = -0x7b80, news_source = -0x7b40, assert_flag = -0x7bac, pool = -0x5608;
}
namespace callee {
inline constexpr std::uint32_t find_entry = 0x105bc2b0,       // RULING_BODY.CPP (body, person, 0): the person's entry or 0
    list_append = 0x10738d10, list_next = 0x10738e00, list_remove = 0x10738e60, fill = 0x10778300,
    database_person = 0x10082f20, database_club = 0x10083110;
}
inline constexpr std::uint32_t club_size = 0x248, club_key_source = 0x2c, key_source_key = 0x68, key_source_nation = 0x74,
                               nation_id = 8;
inline constexpr std::uint32_t body_club_scope = 0x18, body_nation_scope = 0x1c;
inline constexpr std::uint32_t entry_ban_index = 6;  // the lookup's entry: the ban record's index (signed halfword)
// A club's ruling-body scope against a ruling body: `unscoped` when the body has neither scope.
enum class Cover { unscoped, covered, uncovered };
inline Cover body_cover(const guest::Cpu& c, std::uint32_t body, std::uint32_t club) {
    const auto source = [&] { return club ? c.u32(club + club_key_source) : 0u; };
    if (c.i32(body + body_club_scope) != -1) {
        const auto at = source();
        return at && c.i32(at + key_source_key) == c.i32(body + body_club_scope) ? Cover::covered : Cover::uncovered;
    }
    const auto nation = c.i32(body + body_nation_scope);
    if (nation == -1) return Cover::unscoped;
    const auto at = source();
    const auto record = at ? c.u32(at + key_source_nation) : 0u;
    return record && c.i32(record + nation_id) == nation ? Cover::covered : Cover::uncovered;
}
inline std::uint32_t club_record(const guest::Cpu& c, std::uint32_t index) {
    return c.u32(c.toc(toc::clubs)) + index * club_size;
}
inline std::uint32_t body_at(const guest::Cpu& c, std::uint32_t manager_object, std::int8_t index) {
    return body_object(c, manager_object, std::uint32_t(std::int32_t(index)));
}
inline std::uint32_t slot_at(std::uint32_t row, std::int32_t slot) { return row + std::uint32_t(slot) * row::slot_size; }
}
}
