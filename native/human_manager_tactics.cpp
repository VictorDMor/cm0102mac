// HUMAN_MANAGER.CPP: a human's squad lists, the names of the files he used last, and the flags that
// pick a team's squad.
#include "human_manager_tactics.hpp"
#include "formation.hpp"
#include "human_manager_detail.hpp"
#include "tactics.hpp"

namespace cm::aot::human_manager {
namespace {
// A tactic slot's squad: 50 staff ids (-1 for none) ending the slot.
constexpr std::uint32_t squad = 0x1dec, squad_size = 50;
// The recent file names: four texts of 0x100 bytes, at +0x19eb in the current tactic slot (tactics),
// at 0xa140 in the record (training) and at 40000 (search, print).
constexpr std::uint32_t name_size = 0x100, name_count = 4, tactic_names = 0x19eb, training_names = 0xa140,
                        other_names = 40000;
constexpr std::uint32_t staff_team = 0x24;         // word of a staff record: the team the squad list is read from
// Pool offsets of the extensions (5 bytes each) and of the empty one.
constexpr std::uint32_t tactics_suffix = 0x7a, training_suffix = 0x7f, search_suffix = 0x84, print_suffix = 0x89;

// The three checks every squad routine starts with: a staff record (given or the current human's)
// and one inside the table; the third (an entry index or a pointer) is each routine's own.
struct Sites { Site no_human, outside, argument; };
std::optional<std::uint32_t> human_or_report(Cpu& c, const Frame& frame, std::uint32_t humans, std::uint32_t staff,
                                             const Sites& sites) {
    staff = staff_or_current(c, staff);
    if (!staff) {
        report(c, frame, sites.no_human);
        return std::nullopt;
    }
    const auto index = index_of(c, humans, staff);
    if (!index) {
        report(c, frame, sites.outside);
        return std::nullopt;
    }
    return record_at(c, humans, *index);
}
// The current slot's squad. The slot byte is read as unsigned and 1 counts as 0 (0x1014c890 and
// its kin), then sign-extended: any other byte reaches wherever the arithmetic lands.
std::uint32_t squad_of(Cpu& c, std::uint32_t human) {
    auto slot = c.u8(human + record::current_tactic);
    if (slot == 1) slot = 0;
    return human + std::uint32_t(std::int32_t(std::int8_t(slot))) * record::tactic_size + squad;
}
bool in_squad(std::int8_t which) { return which >= 0 && which < std::int8_t(squad_size); }

// The list of recent names of a kind in a record. Kind 1 sits in the current slot (the byte
// sign-extended, with no special case for 1), 3 is training, anything else search/print.
struct Names {
    std::uint32_t base;
    std::uint32_t at(unsigned index) const { return base + index * name_size; }
    std::uint32_t last_byte(unsigned index) const { return at(index) + name_size - 1; }
};
Names names_of(Cpu& c, std::uint32_t human, std::int8_t kind) {
    if (kind == 1)
        return {human + std::uint32_t(std::int32_t(c.i8(human + record::current_tactic))) * record::tactic_size + tactic_names};
    return {human + (kind == 3 ? training_names : other_names)};
}
}

std::uint32_t squad_player(Cpu& c, std::uint32_t humans, std::int8_t which, std::uint32_t staff) {
    const Opening frame(c, 0x1014c890, Opening::Order::success_first, 0xc0);
    const Sites sites{{0x1014c8ec, 0x90, 0x48, 0xbd9}, {0x1014c96c, 0x70, 0x40, 0xbe5}, {0x1014c9d8, 0x50, 0x38, 0xbec}};
    const auto human = human_or_report(c, frame, humans, staff, sites);
    if (!human) return 0;
    if (!in_squad(which)) {
        report(c, frame, sites.argument);
        return 0;
    }
    const auto id = c.u32(squad_of(c, *human) + std::uint32_t(std::int32_t(which)) * 4);
    return id == 0xffffffffu ? 0 : c.u32(c.toc(toc::people)) + id * 0x70;
}

void set_squad_player(Cpu& c, std::uint32_t humans, std::int8_t which, std::uint32_t player, std::uint32_t staff) {
    const Opening frame(c, 0x1014caa0, Opening::Order::success_first, 0xf0);
    const Sites sites{{0x1014cafc, 0xb8, 0x50, 0xc19}, {0x1014cb78, 0x98, 0x48, 0xc25}, {0x1014cbe0, 0x78, 0x40, 0xc2c}};
    const auto human = human_or_report(c, frame, humans, staff, sites);
    if (!human) return;
    if (!in_squad(which)) {
        report(c, frame, sites.argument);
        return;
    }
    if (!player) {
        report(c, frame, {0x1014cc3c, 0x58, 0x38, 0xc33});
        return;
    }
    c.put32(squad_of(c, *human) + std::uint32_t(std::int32_t(which)) * 4, c.u32(player));
}

void remove_squad_player(Cpu& c, std::uint32_t humans, std::uint32_t player, std::uint32_t staff) {
    const Opening frame(c, 0x1014ccf0, Opening::Order::message_first, 0xc0);
    const auto human = human_or_report(c, frame, humans, staff,
                                       {{0x1014cd4c, 0x90, 0x48, 0xc58}, {0x1014cdc8, 0x70, 0x40, 0xc64}, {}});
    if (!human) return;
    if (!player) {
        report(c, frame, {0x1014ce24, 0x50, 0x38, 0xc6b});
        return;
    }
    const auto list = squad_of(c, *human);
    for (std::uint32_t entry = 0; entry < squad_size; ++entry)
        if (c.u32(player) == c.u32(list + entry * 4)) {
            c.put32(list + entry * 4, 0xffffffffu);
            return;
        }
}

void clear_squad_slot(Cpu& c, std::uint32_t humans, std::int8_t which, std::uint32_t staff) {
    const Opening frame(c, 0x1014cf00, Opening::Order::success_first, 0xc0);
    const Sites sites{{0x1014cf5c, 0x90, 0x48, 0xc98}, {0x1014cfd8, 0x70, 0x40, 0xca4}, {0x1014d040, 0x50, 0x38, 0xcab}};
    const auto human = human_or_report(c, frame, humans, staff, sites);
    if (!human) return;
    if (!in_squad(which)) {
        report(c, frame, sites.argument);
        return;
    }
    c.put32(squad_of(c, *human) + std::uint32_t(std::int32_t(which)) * 4, 0xffffffffu);
}

void swap_squad_slots(Cpu& c, std::uint32_t humans, std::int8_t first, std::int8_t second, std::uint32_t staff) {
    const Opening frame(c, 0x1014d0f0, Opening::Order::success_first, 0xc0);
    const Sites sites{{0x1014d14c, 0x90, 0x48, 0xcd2}, {0x1014d1c8, 0x70, 0x40, 0xcde}, {0x1014d244, 0x50, 0x38, 0xce6}};
    const auto human = human_or_report(c, frame, humans, staff, sites);
    if (!human) return;
    if (!in_squad(first) || !in_squad(second)) {
        report(c, frame, sites.argument);
        return;
    }
    const auto list = squad_of(c, *human);
    const auto a = list + std::uint32_t(std::int32_t(first)) * 4, b = list + std::uint32_t(std::int32_t(second)) * 4;
    const auto kept = c.u32(a), other = c.u32(b);
    c.put32(a, other);
    c.put32(b, kept);
}

std::int32_t squad_slot_of(Cpu& c, std::uint32_t humans, std::uint32_t player, std::uint32_t staff) {
    const Opening frame(c, 0x1014d300, Opening::Order::message_first, 0xc0);
    const auto human = human_or_report(c, frame, humans, staff,
                                       {{0x1014d35c, 0x90, 0x48, 0xd0d}, {0x1014d3dc, 0x70, 0x40, 0xd19}, {}});
    if (!human) return -1;
    if (!player) {
        report(c, frame, {0x1014d43c, 0x50, 0x38, 0xd20});
        return -1;
    }
    const auto list = squad_of(c, *human);
    for (std::uint32_t entry = 0; entry < squad_size; ++entry)
        if (c.u32(player) == c.u32(list + entry * 4)) return std::int32_t(entry);
    return -1;
}

std::uint32_t recent_file_path(Cpu& c, std::uint32_t humans, std::int8_t which, std::uint32_t buffer, std::int32_t size,
                               std::uint32_t bare, std::uint32_t staff, std::uint32_t kind_word) {
    // r26 keeps the buffer, r27 `bare` (then the extension) and r28 `kind` as words.
    const Frame frame(c, 0x10147310, 0x240, {.first_gpr = 26});
    const auto pool = c.toc(toc::pool);
    c.set({{26, buffer}, {27, bare}, {28, kind_word}, {29, c.toc(toc::success)}, {30, c.toc(toc::message)}, {31, pool},
           {10, c.toc(toc::game)}});  // r10 keeps the game record (the prologue loads it); the callees read it
    const auto kind = std::int8_t(kind_word);
    if (!buffer || size < 0x100) {
        report(c, frame, {0x1014735c, 0x100, 0x58, 0x286});
        return 0;
    }
    c.put8(buffer, 0);
    if (which < 0 || which > 3) {
        report(c, frame, {0x101473d0, 0xe0, 0x50, 0x291});
        return 0;
    }
    staff = staff_or_current(c, staff);
    if (!staff) {
        report(c, frame, {0x10147448, 0xc0, 0x48, 0x29c});
        return 0;
    }
    if (kind == 4) {
        report(c, frame, {0x101474ac, 0xa0, 0x40, 0x2a5});
        return 0;
    }
    const auto index = index_of(c, humans, staff);
    if (!index) {
        report(c, frame, {0x1014752c, 0x80, 0x38, 0x2b0});
        return 0;
    }
    // What the checks leave in r5-r8: the table's count, the kind, the index, the staff.
    c.set({{5, c.u32(humans + table::count)}, {6, std::uint32_t(std::int32_t(kind))}, {7, *index}, {8, staff}});
    const auto names = names_of(c, record_at(c, humans, *index), kind);
    invoke(c, 0x1077dbf0, kind == 1 ? 0x101475bc : kind == 3 ? 0x101475f8 : 0x1014762c, {buffer, names.at(std::uint32_t(which))});
    if (std::int8_t(bare) != 0) return 1;
    // The folder (a tactics, training or search folder: the original's compare ladder) and the extension;
    // any other kind has neither.
    const auto local = frame.at(0x120), copy = frame.at(0x60);
    std::uint32_t suffix = pool + print_suffix;
    if (kind == 1 || kind == 2 || kind == 3) {
        const auto [folder_name, back, extension] =
            kind == 2 ? std::tuple{text::search, 0x10147690u, search_suffix}
            : kind == 1 ? std::tuple{text::tactics, 0x10147668u, tactics_suffix}
                        : std::tuple{text::training, 0x1014767cu, training_suffix};
        invoke(c, 0x10746300, back, {pool + folder_name, local});
        suffix = pool + extension;
    } else {
        c.put8(local, 0);
    }
    c.set(27, suffix);
    invoke(c, 0x1077dbf0, 0x101476b0, {copy, buffer});
    invoke(c, 0x10739b40, 0x101476c8, {buffer, 0, local, copy, suffix});
    return 1;
}

std::uint32_t remember_file(Cpu& c, std::uint32_t humans, std::uint32_t name, std::uint32_t staff, std::uint32_t kind_word) {
    const Frame frame(c, 0x101476e0, 0x100, {.first_gpr = 23});
    const auto local = frame.at(0xb0), game = c.toc(toc::game), success = c.toc(toc::success);
    c.set({{23, staff}, {24, game}, {25, success}, {26, c.toc(toc::message)}, {27, c.toc(toc::pool)}, {29, humans}, {30, kind_word}});
    // The file name part of `name` into the local buffer, which holds it for the rest of the routine.
    // This site is the one the audit marks odd: five arguments, the last one null (r28 and r31 are the caller's here).
    invoke(c, 0x10739890, 0x10147724, {name, 0, 0, local, 0});
    c.set(31, local);
    staff = staff_or_current(c, staff);
    c.set(23, staff);
    if (!staff) {
        report(c, frame, {0x1014775c, 0x90, 0x48, 0x333});
        return 0;
    }
    const auto kind = std::int8_t(kind_word);
    c.set(28, std::uint32_t(std::int32_t(kind)));
    if (kind == 4) {
        report(c, frame, {0x101477c0, 0x70, 0x40, 0x33c});
        return 0;
    }
    // 0x10147814 `addic.` leaves the index in r24 before the range test.
    const auto id = c.u32(staff + staff::id), total = c.u32(c.toc(toc::staff_count));
    const auto index = std::int32_t(id - total + std::uint32_t(human_slots));
    c.set(24, std::uint32_t(index));
    if (index < 0 || index > std::int32_t(c.u32(humans + table::count) - 1)) {
        report(c, frame, {0x10147840, 0x50, 0x38, 0x347});
        return 0;
    }
    const auto human = record_at(c, humans, std::uint32_t(index));
    const auto names = names_of(c, human, kind);
    const std::uint32_t compare_back = kind == 1 ? 0x101478d8 : kind == 3 ? 0x1014790c : 0x10147938,
                        shift_back = kind == 1 ? 0x101479bc : kind == 3 ? 0x10147a2c : 0x10147a84,
                        store_back = kind == 1 ? 0x10147af4 : kind == 3 ? 0x10147b50 : 0x10147b90;
    // Where does the name stand? r25 and r26 hold the offset and the index, r27 the answer (3 when it is not found).
    c.set({{27, 3}, {26, 0}, {25, 0}});
    for (std::uint32_t entry = 0; entry < name_count;) {
        if (invoke(c, 0x107393f0, compare_back, {names.at(entry), local}) == 0) {
            c.set(27, std::uint32_t(std::int32_t(std::int8_t(entry))));
            break;
        }
        ++entry;
        c.set({{26, entry}, {25, entry * name_size}});
    }
    c.set({{26, std::uint32_t(std::int32_t(std::int8_t(c.r(27))))}, {28, std::uint32_t(std::int32_t(kind))}});
    c.set(25, c.r(26) << 8);
    const auto record_offset = std::uint32_t(index) * record::size;  // r24 and r27 after the original's mullw
    for (auto entry = std::int32_t(c.r(26)); entry > 0; --entry) {
        c.set(27, record_offset);
        invoke(c, 0x1077dc10, shift_back, {names.at(std::uint32_t(entry)), names.at(std::uint32_t(entry - 1)), name_size});
        c.put8(names.last_byte(std::uint32_t(entry)), 0);
        c.set({{25, c.r(25) - name_size}, {26, c.r(26) - 1}});
    }
    c.set(24, record_offset);
    invoke(c, 0x1077dc10, store_back, {names.at(0), local, name_size});
    c.put8(names.last_byte(0), 0);
    return 1;
}

void set_flag(Cpu& c, std::uint32_t humans, std::int8_t which, std::uint32_t value, std::uint32_t staff) {
    const Frame frame(c, 0x10149f50, 0xb0, {.first_gpr = 25});
    const auto pool = c.toc(toc::pool), message = c.toc(toc::message), team_cell = c.toc(toc::tactics_templates);
    // r29 is not loaded before it is set below: the caller's value stays at the checks.
    c.set({{25, humans}, {26, staff}, {27, pool}, {28, message}, {30, team_cell}, {31, c.toc(toc::success)}});
    staff = staff_or_current(c, staff);
    c.set(26, staff);
    if (!staff) {
        report(c, frame, {0x10149fac, 0x68, 0x40, 0x7a2});
        return;
    }
    const auto id = c.u32(staff + staff::id), total = c.u32(c.toc(toc::staff_count));
    const auto index = std::int32_t(id - total + std::uint32_t(human_slots));
    if (index < 0 || index > std::int32_t(c.u32(humans + table::count) - 1)) {
        report(c, frame, {0x1014a028, 0x48, 0x38, 0x7ae});
        return;
    }
    const auto human = record_at(c, humans, std::uint32_t(index));
    c.set(29, std::uint32_t(index) * record::size);
    c.put8(human + record::flags + std::uint32_t(std::int32_t(which)), value);
    if (which != 3 || std::int8_t(value) == 0 || c.u32(staff + staff_team) == 0) return;
    // The staff's team: its formation, then its 16 players, each with the position the formation gives
    // him (the first 11) or -1.
    c.link(0x1014a0b4);
    const auto formation = tactics::team_formation(c, c.u32(team_cell), c.u32(staff + staff_team), 0);
    c.set({{27, formation}, {28, 0}, {31, 0}});
    for (std::uint32_t player = 0; std::int8_t(player) < 16; ++player) {
        const auto entry = human + record::list + player * 8;
        c.link(0x1014a0d8);
        const auto member = tactics::squad_player(c, c.u32(team_cell), c.u32(staff + staff_team), player, 0);
        if (member) {
            c.put32(entry, c.u32(member));
            if (std::int8_t(player) < 0xb) {
                c.link(0x1014a114);
                c.put32(entry + 4, std::uint32_t(std::int32_t(std::int16_t(formation::position(c, formation, std::int8_t(player), 0)))));
            } else {
                c.put32(entry + 4, 0xffffffffu);
            }
        } else {
            c.put32(entry, 0xffffffffu);
            c.put32(entry + 4, 0xffffffffu);
        }
        c.set({{31, (player + 1) * 8}, {28, player + 1}});
    }
}
}
