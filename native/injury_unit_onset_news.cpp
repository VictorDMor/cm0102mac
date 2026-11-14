// INJURY.CPP, block "onset": the news item of a new injury (0x1018aed0).
#include "injury_unit_onset.hpp"
#include "injury_unit_onset_detail.hpp"

namespace cm::aot::injury_unit {
using namespace onset;

namespace {
namespace callee_news {
inline constexpr std::uint32_t people_involved = 0x103fc710,  // (object, person, 0x5a, list, 0x1e): how many it listed
    item_clear = 0x102bdf90, item_put = 0x102cb4b0, item_header = 0x1030dcd0, news_add = 0x102cc140,
    news_add_all = 0x102cc8e0, news_accepts = 0x102cdbf0, owner_of = 0x10082f20;
}
constexpr std::uint32_t item_size = 2000, item_marker = 0x420e9;
std::uint32_t extended(std::int32_t value) { return std::uint32_t(value); }
}

// The item is a bag of 16 numbered values at frame+0xb4. A value that stands for a person is that
// person's id (+0x20), or 0 without one.
void post_injury_news(Cpu& c, std::uint32_t, std::uint32_t person, std::uint32_t record, std::uint32_t news_argument) {
    Frame frame(c, 0x1018aed0, 0x1c0, {.first_gpr = 24});
    const auto item = frame.at(0xb4), people_list = frame.at(0x3c), header_size = frame.at(0x38);
    c.set({{24, person}, {25, record}, {26, news_argument}, {28, c.toc(toc_cell::owner_object)},
           {29, c.toc(toc_cell::owner_cell)}, {30, c.toc(toc_cell::news_object)}, {31, c.toc(toc_cell::clubs_cell)}});
    const auto news = [&] { return c.u32(c.toc(toc_cell::news_object)); };

    // The people the injury concerns (the career object lists them), then the injured person's own id.
    const auto career = c.u32(c.toc(toc_cell::career_object));
    std::uint32_t listed = 0;
    if (career != 0) listed = invoke(c, callee_news::people_involved, 0x1018af1c, {career, person, 0x5a, people_list, 0x1e});
    c.set(27, listed);
    if (c.u32(person + person::news_person) != 0 && std::int8_t(listed) < 0x1e) {
        c.put32(people_list + (extended(std::int8_t(listed)) << 2), c.u32(person + person::news_person));
        listed += 1;
        c.set(27, listed);
    }

    invoke(c, callee_news::item_clear, 0x1018af60, {item, item_size, 0});
    const auto put = [&](unsigned slot, std::uint32_t value, std::uint32_t after) {
        invoke(c, callee_news::item_put, after, {item, slot, value});
    };
    const auto id_of = [&](std::uint32_t link) { return link == 0 ? 0u : c.u32(link + 0x20); };
    const auto relative_id = [&](std::uint32_t field) { return id_of(c.u32(person + field)); };
    put(0, c.u32(person), 0x1018af70);
    put(1, relative_id(person::relative_a), 0x1018af94);
    put(2, relative_id(person::relative_b), 0x1018afb8);
    put(3, relative_id(person::relative_c), 0x1018afdc);
    put(4, extended(c.i8(record + record::type)), 0x1018aff0);
    put(5, extended(c.i16(record + record::started)), 0x1018b000);
    put(6, c.u16(record + record::started + 2), 0x1018b010);
    put(7, extended(c.i16(record + record::days_out)), 0x1018b020);
    put(8, extended(c.i16(person + person::news_a)), 0x1018b030);
    put(9, c.u16(person + person::news_b), 0x1018b040);
    if (c.i8(record + record::match_found) == -1) {
        // The injury did not happen in a match: the three match values are -1.
        put(10, 0xffffffff, 0x1018b094);
        put(11, 0xffffffff, 0x1018b0a4);
        put(12, 0xffffffff, 0x1018b0b4);
    } else {
        put(10, c.u32(record + 0x18), 0x1018b060);
        put(11, c.u32(record + 0x1c), 0x1018b070);
        put(12, c.u32(record + 0x0c), 0x1018b080);
    }
    put(13, news_argument, 0x1018b0c4);
    const auto club = c.u32(person + person::club);
    put(14, club == 0 ? 0xffffffffu : c.u32(club), club == 0 ? 0x1018b0f4u : 0x1018b0e0u);
    put(15, item_marker, 0x1018b108);
    c.put8(header_size, 0x10);
    invoke(c, callee_news::item_header, 0x1018b120, {record + record::match_snapshot, item, header_size});

    if (c.i16(record + record::days_out) <= 0x64) {
        // A short injury goes to the club's news and, when the owner's club differs, to the owner's too.
        const auto club_now = c.u32(person + person::club);
        // person == 0 cannot happen here: the person's id link was read at the start (a null person faults there).
        if (person == 0 || club_now == 0) return;
        if (std::int8_t(invoke(c, callee_news::news_accepts, 0x1018b1bc, {news(), club_now})) == 0) return;
        invoke(c, callee_news::news_add, 0x1018b1d4, {news(), item, c.u32(person + person::club)});
        const auto club_of_owner = [&](std::uint32_t after) {
            const auto owner = invoke(c, callee_news::owner_of, after, {c.u32(c.toc(toc_cell::owner_object)), person});
            c.put32(c.toc(toc_cell::owner_cell), owner);
            return owner == 0 ? 0u : c.u32(c.toc(toc_cell::clubs_cell)) + c.u32(owner + 4) * 0x248;
        };
        if (club_of_owner(0x1018b1e0) != 0 && club_of_owner(0x1018b218) != c.u32(person + person::club))
            invoke(c, callee_news::news_add, 0x1018b284, {news(), item, club_of_owner(0x1018b254)});
        const auto link = c.u32(person + person::news_person);
        if (link != 0) invoke(c, callee_news::news_add, 0x1018b29c, {news(), item, link});
        return;
    }
    const auto profile_address = c.u32(person + person::profile);
    if (profile_address != 0 && c.i16(profile_address + profile::standing) > 0x1f40) {
        // A star's long injury goes to everybody who follows him.
        invoke(c, callee_news::news_add_all, 0x1018b150, {news(), item});
        return;
    }
    c.set({{25, people_list}, {24, extended(std::int8_t(listed))}});
    for (std::int32_t i = 0; i < std::int8_t(listed); ++i)
        invoke(c, callee_news::news_add, 0x1018b180, {news(), item, c.u32(people_list + (std::uint32_t(i) << 2))});
}
}
