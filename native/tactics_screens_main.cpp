#include "tactics_screens_main.hpp"

namespace cm::aot::tactics_screens::main {
using guest::Cpu;
using guest::Frame;
namespace {
// TOC slots of the globals these routines read.
constexpr std::int32_t pool_slot = -0x4c90;      // TACTICS_SCREENS.CPP's string pool
constexpr std::int32_t message_slot = -0x7ba8;   // -> assertion message buffer
constexpr std::int32_t success_slot = -0x7bac;   // -> shared success flag
constexpr std::int32_t root_slot = -0x7bb0;      // -> the game root (UI pages, current human)
constexpr std::int32_t managers_slot = -0x7a54;  // -> -> human manager table
constexpr std::int32_t tactics_slot = -0x7a6c;   // -> -> CTACTICS (tactics.hpp)
constexpr std::int32_t shirts_slot = -0x7a98;    // -> -> the object 0x104ae570 asks for shirt numbers
constexpr std::int32_t text_slot = -0x7bb8;      // -> shared text buffer

// The game's assert macro at `at` (its first call); r3 is left at the
// success flag, as the original's last load leaves it.
void fail(Cpu& c, const Frame& frame, std::uint32_t at, std::int32_t name, std::int32_t extension, std::uint32_t line) {
    const auto pool = c.toc(pool_slot);
    guest::report_assertion(c, {at, pool, pool + 0x14, pool + 0x21, frame.at(name), frame.at(extension), line});
    c.set(3, c.toc(success_slot));
}
// A char or short register as the original sign-extends it.
std::int32_t byte(std::uint32_t value) { return std::int8_t(value); }
std::int32_t half(std::uint32_t value) { return std::int16_t(value); }
// A char argument within [low, high], tested low then high into cr0 as the original.
bool in_range(Cpu& c, std::uint32_t value, std::int32_t low, std::int32_t high) {
    c.compare(0, byte(value), low);
    if (byte(value) < low) return false;
    c.compare(0, byte(value), high);
    return byte(value) <= high;
}

std::uint32_t entry(std::uint32_t screen, std::uint32_t index) {
    return screen + std::uint32_t(byte(index)) * field::entry_size + field::players;
}
// The tests below compare into cr0 as the original does: a routine leaves
// every CR field as the original leaves it (docs/cpp-rewrites.md).
bool read_only(Cpu& c, std::uint32_t screen) {
    c.compare(0, c.i8(screen + field::read_only), 0);
    return c.i8(screen + field::read_only) != 0;
}
bool view_is(Cpu& c, std::uint32_t screen, std::int32_t view) {
    c.compare(0, c.i8(screen + field::view), view);
    return c.i8(screen + field::view) == view;
}
// 1 in the with-the-ball view: the ball-state argument of the formation's spot routines.
std::uint32_t with_ball(Cpu& c, std::uint32_t screen) { return view_is(c, screen, 2) ? 1 : 0; }
// 0x103baae0 (a leaf): copies a cell, as two halfwords.
void copy_cell_value(Cpu& c, std::uint32_t to, std::uint32_t from) {
    c.put16(to, c.u16(from));
    c.put16(to + 2, c.u16(from + 2));
}
// Before an edit: the formation into the undo copy (0x103c1340 copies a
// formation record), and undo becomes possible.
void save_undo(Cpu& c, std::uint32_t screen, std::uint32_t back) {
    c.set({{3, screen + field::undo}, {4, screen + field::formation}});
    c.call(0x103c1340, back);
    c.put8(screen + field::can_undo, 1);
}
void mark_changed(Cpu& c, std::uint32_t screen) { c.put8(screen + field::changed, 1); }
bool clipboard_empty(Cpu& c, std::uint32_t screen) {
    c.compare(0, c.i8(screen + field::clipboard), 0);
    return !c.i8(screen + field::clipboard);
}
// The original's test of a match pointer (cmplwi), into cr0.
bool in_match(Cpu& c, std::uint32_t screen) {
    c.compare_logical(0, c.u32(screen + field::match), 0);
    return c.u32(screen + field::match) != 0;
}

// The control `index` of the UI page the root's current human shows.
std::uint32_t current_page(Cpu& c, std::uint32_t root) {
    const auto human = std::uint32_t(half(c.u16(root + field::root_human)));
    return c.u32(root + human * field::root_human_size + field::root_page);
}
std::uint32_t control(std::uint32_t page, std::uint32_t index) {
    return page + std::uint32_t(half(index)) * field::control_size + field::page_controls;
}
// The root-level wrapper: spills the index, finds the page, and calls the
// page-level reader with the sign-extended index.
template <std::uint32_t (*Read)(Cpu&, std::uint32_t, std::uint32_t)>
std::uint32_t root_control(Cpu& c, std::uint32_t entry, std::uint32_t back, std::uint32_t root, std::uint32_t index) {
    Frame frame(c, entry, 0x40, {.first_gpr = 31});
    c.put16(frame.at(0x5e), index);
    c.set(31, root);
    const auto page = current_page(c, root);
    c.set({{3, page}, {4, std::uint32_t(half(index))}});
    c.link(back);
    const auto value = Read(c, page, std::uint32_t(half(index)));
    c.set(0, c.lr());
    return value;
}
// The page-level reader: a control word, or -1 for no control.
std::uint32_t page_control_word(Cpu& c, std::uint32_t entry, std::uint32_t page, std::uint32_t index,
                                std::uint32_t offset) {
    Frame frame(c, entry, 0x40, {.first_gpr = 30});
    c.put32(frame.at(0x58), page);
    c.compare(0, half(index), -1);
    if (half(index) == -1) return 0xffffffffu;
    c.set(4, page);
    return c.u32(control(page, index) + offset);
}
}

std::uint32_t open(Cpu& c, std::uint32_t club, std::uint32_t opened_from, std::uint32_t match, std::uint32_t in_match,
                   std::uint32_t preset, std::uint32_t national) {
    Frame frame(c, 0x103b3830, 0xc0, {.first_gpr = 19});
    const auto root = c.toc(root_slot);
    c.set({{21, club}, {22, opened_from}, {19, match}, {23, in_match}, {20, preset}, {24, national}, {31, root},
           {28, c.toc(message_slot)}, {30, c.toc(pool_slot)}});
    c.compare_logical(0, club, 0);
    if (!club) {
        fail(c, frame, 0x103b3880, 0x61, 0x3d, 0x17c);
        return c.r(3);
    }
    c.compare(0, byte(in_match), 0);
    if (byte(in_match)) c.compare(0, byte(national), 0);
    if (byte(in_match) && byte(national)) {
        fail(c, frame, 0x103b38f0, 0x42, 0x38, 0x183);
        return c.r(3);
    }
    c.set(3, field::size);
    c.call(0x10773520, 0x103b3940);
    const auto screen = c.r(3);
    c.set(25, screen);
    c.compare_logical(0, screen, 0);
    if (screen) {
        c.set({{3, screen}, {4, club}, {5, match}, {6, opened_from}, {7, in_match}, {8, preset}, {9, national}});
        c.call(0x103c4b10, 0x103b396c);  // constructs the state
    }
    // Pushes the page: built by 0x103baec0 (or 0x103b3c50 when not opened
    // from elsewhere), handled by 0x103baee0.
    c.compare(0, byte(opened_from), 0);
    const auto builder = c.toc(byte(opened_from) ? -0x4c94 : -0x4c98);
    c.set({{29, screen}, {27, builder}});
    const bool locked = read_only(c, screen);
    if (locked)
        c.set({{3, root}, {4, builder}, {5, c.toc(-0x4c9c)}, {6, 0}, {7, c.toc(-0x4ca0)}, {8, 0}});
    else
        c.set({{3, root}, {4, builder}, {5, c.toc(-0x4c9c)}, {6, 1}, {7, 0}, {8, c.toc(-0x4ca4)}});
    c.call(0x1073e4b0, locked ? 0x103b39b8 : 0x103b39f0);
    c.compare(0, byte(c.r(3)), 0);
    if (!byte(c.r(3))) {
        c.set({{3, screen}, {4, 1}});
        c.call(0x103b3b50, locked ? 0x103b39d0 : 0x103b3a08);  // destroys the state
        return c.r(3);
    }
    c.set(3, screen);
    c.call(0x103b3b20, 0x103b3a14);
    c.compare(0, byte(c.r(3)), 0);
    const std::uint32_t mode = byte(c.r(3)) ? 6 : 2;
    c.set(26, mode);
    const auto signed_byte = [](std::uint32_t value) { return std::uint32_t(byte(value)); };
    const std::uint32_t parameters[] = {screen, club, match, signed_byte(opened_from), signed_byte(in_match), 0, 0,
                                        preset, signed_byte(national), 0xffffffffu, mode};
    for (std::uint32_t n = 0; n < std::size(parameters); ++n) {
        c.set({{3, root}, {4, n}, {5, parameters[n]}, {6, 0}});
        c.call(0x1073f1e0, 0x103b3a40 + n * 0x14);  // sets page parameter n
    }
    return c.r(3);
}

std::uint32_t match_changes(Cpu& c, std::uint32_t screen, std::uint32_t text) {
    Frame frame(c, 0x103c64c0, 0x38e0, {.first_gpr = 17});
    const auto shirts = c.toc(shirts_slot), buffer = c.toc(text_slot), pool = c.toc(pool_slot);
    c.set({{30, screen}, {24, text}, {29, shirts}, {27, buffer}, {31, pool}, {3, text}});
    c.call(0x1074c570, 0x103c64ec);  // clears the text
    if (!in_match(c, screen)) return 0;
    const auto message = frame.at(0x1190), name = frame.at(0x128), other_name = frame.at(0x60);
    c.put8(message, 0);
    // Appends the shared buffer to the message, after a blank line.
    const auto append = [&](std::uint32_t separator_back, std::uint32_t back) {
        c.compare(0, c.i8(message), 0);
        if (c.i8(message)) {
            c.set({{3, message}, {4, pool + 0xeb4}});
            c.call(0x1077dc60, separator_back);
        }
        c.set({{3, message}, {4, buffer}});
        c.call(0x1077dc60, back);
    };
    // A match player's name into `into`, and a link to his staff record in
    // the text (kind 0x7dc when `test` holds, else 0x7d7). The call sites
    // share one layout from `back` (the return of the first call).
    const auto name_player = [&](std::uint32_t player, std::uint32_t into, std::uint32_t back, auto test) {
        const auto staff = [&](std::uint32_t at) {
            c.set(3, player);
            c.call(0x101e5070, at);
            return c.r(3);
        };
        c.set({{3, staff(back)}, {4, into}, {5, 0xc8}, {6, 1}});
        c.call(0x100caeb0, back + 0x10);
        const bool kind = test(back + 0x18);
        const auto record = staff(back + (kind ? 0x2c : 0x4c));
        c.set({{6, record}, {3, text}, {4, into}, {5, kind ? 0x7dcu : 0x7d7u}});
        c.call(0x1074bab0, back + (kind ? 0x40 : 0x60));
    };
    const auto screen_test = [&](std::uint32_t at) {
        c.set(3, screen);
        c.call(0x103baca0, at);
        return byte(c.r(3)) != 0;
    };
    const auto count = screen + field::substitutions;
    if (c.i8(count) > 3) {
        c.set({{3, std::uint32_t(std::int32_t(c.i8(count)))}, {4, name}, {5, 0xc8}, {6, 1}});
        c.call(0x10746a70, 0x103c6528);  // the number as text
        c.set({{3, buffer}, {4, pool + 0xe89}, {5, name}});
        c.call(0x10727890, 0x103c6538);
        append(0x103c6554, 0x103c6560);
    } else {
        // One sentence per substitution, with shirt numbers when the club's
        // are known (0x104ae570), else the entries' numbers.
        c.set(28, 0);
        for (std::uint32_t i = 0; byte(i) < c.i8(count); ++i) {
            c.set(28, i);
            const auto shirt = [&](std::uint32_t list, std::uint32_t player, std::uint32_t back) {
                if (!c.i8(screen + field::opened_from)) {
                    c.set({{3, c.u32(shirts)}, {4, c.u32(screen + field::club)}, {5, 0}});
                    c.call(0x104ae570, back);
                    if (byte(c.r(3))) {
                        c.set(3, player);
                        c.call(0x101e5070, back + 0x14);
                        c.set({{4, c.r(3)}, {3, c.u32(shirts)}, {5, c.u32(screen + field::club)}});
                        c.call(0x104add10, back + 0x24);
                        c.set(3, std::uint32_t(byte(c.r(3))));
                        c.call(0x10775540, back + 0x2c);
                        return std::uint32_t(byte(c.r(3)));
                    }
                }
                return std::uint32_t(byte(c.u8(list + std::uint32_t(byte(i))) + 1u));
            };
            const auto on = entry(screen, c.u8(screen + field::brought_on + std::uint32_t(byte(i))));
            c.set(26, on);
            const auto on_shirt = shirt(screen + field::brought_on, on, 0x103c65a8);
            c.set(19, on_shirt);
            name_player(on, name, 0x103c65f8, screen_test);
            const auto off = entry(screen, c.u8(screen + field::taken_off + std::uint32_t(byte(i))));
            c.set(26, off);
            const auto off_shirt = shirt(screen + field::taken_off, off, 0x103c6694);
            c.set(18, off_shirt);
            name_player(off, other_name, 0x103c66e4, screen_test);
            c.set({{3, buffer}, {4, pool + 0xeb7}, {5, on_shirt}, {6, name}, {7, off_shirt}, {8, other_name}});
            c.call(0x10727890, 0x103c6760);
            append(0x103c677c, 0x103c6788);
            c.set(28, i + 1);
        }
    }
    // Without a captain on the pitch, the best player will be captain.
    c.set(3, screen);
    c.link(0x103c67a8);
    const auto selected = captain_selected(c, screen);
    c.set({{3, selected}, {0, c.lr()}});
    if (!byte(selected)) {
        c.set(3, screen);
        c.link(0x103c67bc);
        const auto best = best_player(c, screen);
        c.set({{17, best}, {3, best}, {0, c.lr()}});
        if (byte(best) > -1) {
            const auto player = entry(screen, best);
            c.set(26, player);
            name_player(player, name, 0x103c67e4, [&](std::uint32_t at) {
                c.set(3, c.u32(screen + field::club));
                c.call(0x100c7d00, at);
                return byte(c.r(3)) != 0;
            });
            c.set({{3, buffer}, {4, pool + 0xf11}, {5, name}});
            c.call(0x10727890, 0x103c6854);
            append(0x103c6870, 0x103c687c);
        }
    }
    // Set-piece takers who are not on the pitch, each named once (at most 10).
    const auto takers = frame.at(0x38);
    std::uint32_t listed = 0;
    c.set({{25, 0}, {28, 0}});
    for (std::uint32_t index = 0; byte(index) < 20; ++index) {
        c.set(28, index);
        c.set({{3, screen}, {4, index}});
        c.call(0x103c9720, 0x103c6894);
        if (!byte(c.r(3))) continue;
        c.set({{3, screen}, {4, index}, {5, 0}, {6, 0}});
        c.call(0x103c9810, 0x103c68b4);
        if (byte(c.r(3))) continue;
        c.set({{3, screen}, {4, index}, {5, 0}});
        c.call(0x103c9970, 0x103c68d0);
        if (byte(c.r(3))) continue;
        c.set({{3, screen}, {4, index}, {5, 0}});
        c.call(0x103ca4e0, 0x103c68ec);
        const auto staff = c.r(3);
        c.set(20, staff);
        if (!staff) continue;
        for (std::uint32_t kind = 0; byte(kind) < 5; ++kind) {
            for (std::uint32_t order = 0; byte(order) < 2; ++order) {
                const auto taker = screen + std::uint32_t(byte(kind)) * 8 + std::uint32_t(byte(order)) * 4 + field::takers;
                if (c.u32(taker) != staff) continue;
                std::uint32_t seen = 0;
                while (byte(seen) < byte(listed) && c.u32(takers + std::uint32_t(byte(seen)) * 4) != staff) ++seen;
                c.set(23, seen);
                if (byte(seen) == byte(listed)) c.put32(takers + std::uint32_t(byte(listed++)) * 4, staff);
            }
            c.set(21, 2);
        }
        c.set({{22, 5}, {25, listed}});
    }
    c.set(28, 20);  // the loop's counter at its end
    if (byte(listed) > 0) {
        const auto list = frame.at(0x1f0), sentence = frame.at(0x9c0);
        for (std::uint32_t i = 0; byte(i) < byte(listed); ++i) {
            c.set(28, i);
            const auto staff = c.u32(takers + std::uint32_t(byte(i)) * 4);
            c.set({{3, staff}, {4, name}, {5, 0xc8}, {6, 1}});
            c.call(0x100caeb0, 0x103c69ec);
            if (byte(i) == 0) {
                c.set({{3, sentence}, {4, name}});
                c.call(0x1077dbf0, 0x103c6a04);
            } else {
                const bool last = byte(i) == byte(listed) - 1;
                const auto format = last && byte(listed) == 2 ? 0xf5du : last ? 0xf81u : 0xfa8u;
                c.set({{3, sentence}, {4, pool + format}, {5, list}, {6, name}});
                c.call(0x10727890, last && byte(listed) == 2 ? 0x103c6a3c : last ? 0x103c6a68 : 0x103c6a80);
            }
            c.set({{3, list}, {4, sentence}});
            c.call(0x1077dbf0, 0x103c6a8c);
            const bool kind = screen_test(0x103c6a94);
            c.set({{3, text}, {4, name}, {5, kind ? 0x7dcu : 0x7d7u}, {6, c.u32(takers + std::uint32_t(byte(i)) * 4)}});
            c.call(0x1074bab0, kind ? 0x103c6ac0 : 0x103c6ae4);
            c.set(28, i + 1);
        }
        const bool several = byte(listed) > 1;
        c.set({{3, buffer}, {4, pool + (several ? 0xfcbu : 0x1028u)}, {5, list}});
        c.call(0x10727890, several ? 0x103c6b14 : 0x103c6b28);
        append(0x103c6b44, 0x103c6b50);
    }
    c.compare(0, c.i8(message), 0);
    if (!c.i8(message)) return 0;
    c.compare(0, c.i8(screen + field::in_match), 0);
    if (!c.i8(screen + field::in_match)) return 0;
    c.set({{3, buffer}, {4, pool + 0x107e}});
    c.call(0x10727890, 0x103c6b7c);
    append(0x103c6b98, 0x103c6ba4);
    c.set({{3, text}, {4, message}});
    c.call(0x1074b830, 0x103c6bb0);  // the text shown
    return 1;
}

// Controls of the current page.

std::uint32_t page_control_1c(Cpu& c, std::uint32_t page, std::uint32_t index) {
    return page_control_word(c, 0x103be4e0, page, index, 0x1c);
}
std::uint32_t control_1c(Cpu& c, std::uint32_t root, std::uint32_t index) {
    return root_control<page_control_1c>(c, 0x103be490, 0x103be4c0, root, index);
}
std::uint32_t page_control_18(Cpu& c, std::uint32_t page, std::uint32_t index) {
    return page_control_word(c, 0x103be5b0, page, index, 0x18);
}
std::uint32_t control_18(Cpu& c, std::uint32_t root, std::uint32_t index) {
    return root_control<page_control_18>(c, 0x103be560, 0x103be590, root, index);
}
std::uint32_t page_control_14(Cpu& c, std::uint32_t page, std::uint32_t index) {
    return page_control_word(c, 0x103be680, page, index, 0x14);
}
std::uint32_t control_14(Cpu& c, std::uint32_t root, std::uint32_t index) {
    return root_control<page_control_14>(c, 0x103be630, 0x103be660, root, index);
}
std::uint32_t page_control_10(Cpu& c, std::uint32_t page, std::uint32_t index) {
    return page_control_word(c, 0x103be750, page, index, 0x10);
}
std::uint32_t control_10(Cpu& c, std::uint32_t root, std::uint32_t index) {
    return root_control<page_control_10>(c, 0x103be700, 0x103be730, root, index);
}
std::uint32_t page_control_76(Cpu& c, std::uint32_t page, std::uint32_t index) {
    Frame frame(c, 0x103be910, 0x40, {.first_gpr = 30});
    c.put32(frame.at(0x58), page);
    c.compare(0, half(index), -1);
    if (half(index) == -1) return 0xffffffffu;
    c.set(4, page);
    return std::uint32_t(std::int32_t(c.i16(control(page, index) + 0x76)));
}
std::uint32_t control_76(Cpu& c, std::uint32_t root, std::uint32_t index) {
    return root_control<page_control_76>(c, 0x103be8c0, 0x103be8f0, root, index);
}

std::uint32_t copy_control_60(Cpu& c, std::uint32_t target, std::uint32_t source) {
    Frame frame(c, 0x103be880, 0x40, {});
    c.put32(frame.at(0x58), target);
    c.put32(frame.at(0x5c), source);
    c.set({{3, target + 0x60}, {4, source}, {5, 8}});
    c.call(0x1077fc28, 0x103be8a8);  // the import glue saves r2 at sp+0x14
    c.set(2, c.u32(frame.at(0x14)));
    return c.r(3);
}
std::uint32_t set_page_control_60(Cpu& c, std::uint32_t page, std::uint32_t index, std::uint32_t source) {
    Frame frame(c, 0x103be820, 0x40, {.first_gpr = 31});
    c.put32(frame.at(0x58), page);
    c.set(31, index);
    c.put32(frame.at(0x60), source);
    c.compare(0, half(index), -1);
    if (half(index) == -1) return page;  // r3 untouched
    c.link(0x103be868);
    const auto result = copy_control_60(c, control(page, index), source);
    c.set(0, c.lr());
    return result;
}
std::uint32_t set_control_60(Cpu& c, std::uint32_t root, std::uint32_t index, std::uint32_t source) {
    Frame frame(c, 0x103be7d0, 0x40, {.first_gpr = 31});
    c.put16(frame.at(0x5e), index);
    c.put32(frame.at(0x60), source);
    c.set(31, root);
    const auto page = current_page(c, root);
    c.set({{3, page}, {4, std::uint32_t(half(index))}, {5, source}});
    c.link(0x103be808);
    const auto result = set_page_control_60(c, page, std::uint32_t(half(index)), source);
    c.set({{3, result}, {0, c.lr()}});
    return result;
}

// The state object.

std::uint32_t player_at_spot(Cpu& c, std::uint32_t screen, std::uint32_t spot) {
    Frame frame(c, 0x103be420, 0x40, {.first_gpr = 30});
    c.put32(frame.at(0x5c), spot);
    const auto ball = with_ball(c, screen);
    c.set({{31, screen}, {30, ball}});
    c.set({{3, screen + field::formation}, {4, ball}, {5, c.u32(screen + field::cell)}, {6, spot}});
    c.call(0x10112d20, 0x103be46c);
    return c.r(3);
}

std::uint32_t formation_changed(Cpu& c, std::uint32_t screen) {
    Frame frame(c, 0x103be9c0, 0x40, {});
    c.put32(frame.at(0x58), screen);
    return c.u8(screen + field::formation + field::formation_changed);
}

std::uint32_t captain_selected(Cpu& c, std::uint32_t screen) {
    Frame frame(c, 0x103bea00, 0x40, {.first_gpr = 31});
    c.set({{31, screen}, {3, screen}});
    c.call(0x103c6180, 0x103bea1c);  // the entry holding the captain
    c.set({{4, c.r(3)}, {3, screen}, {5, 0}, {6, 0}});
    c.call(0x103c9810, 0x103bea30);
    return c.r(3);
}

std::uint32_t save_preset(Cpu& c, std::uint32_t screen, std::uint32_t path) {
    Frame frame(c, 0x103bea50, 0x40, {});
    c.put32(frame.at(0x58), screen);
    c.put32(frame.at(0x5c), path);
    c.set({{3, screen + field::formation}, {4, path}, {5, 1}, {6, 1}});
    c.call(0x10115cd0, 0x103bea7c);
    return c.r(3);
}

std::uint64_t club_event(Cpu& c, std::uint32_t screen, std::uint32_t a, std::uint32_t b) {
    Frame frame(c, 0x103bea90, 0x40, {.first_gpr = 31});
    c.set(31, screen);
    c.put32(frame.at(0x5c), a);
    c.put32(frame.at(0x60), b);
    c.set({{3, c.u32(screen + field::club)}, {4, c.u8(screen + field::team_kind)}, {5, a}, {6, b}});
    c.call(0x100c7a50, 0x103beac4);
    return std::uint64_t(c.r(3)) << 32 | c.r(4);
}

void open_team_instructions(Cpu& c, std::uint32_t screen) {
    Frame frame(c, 0x103bebb0, 0x50, {.first_gpr = 28});
    const auto root = c.toc(root_slot);
    c.set({{28, screen}, {31, root}, {3, field::size}});
    c.call(0x10773520, 0x103bebe0);
    const auto copy = c.r(3);
    c.set(29, copy);
    if (copy) {
        c.set({{3, copy}, {4, screen}});
        c.link(0x103bebf8);
        c.set(3, copy_construct(c, copy, screen));
        c.set(0, c.lr());
    }
    c.set(30, copy);
    // Built by 0x103becb0, or by 0x103c0cf0 when the formation cannot be edited.
    const bool locked = read_only(c, copy);
    c.set({{3, root}, {4, c.toc(locked ? -0x4cc4 : -0x4cbc)}, {5, c.toc(-0x4cc0)}, {6, 1}, {7, 0}, {8, 0}});
    c.call(0x1073e4b0, locked ? 0x103bec58 : 0x103bec2c);
    c.compare(0, byte(c.r(3)), 0);
    if (!byte(c.r(3))) return;
    if (locked) c.set(28, 0);
    c.set({{3, root}, {4, 0}, {5, copy}, {6, 0}});
    c.call(0x1073f1e0, 0x103bec7c);
    c.set({{3, root}, {4, 1}, {5, c.r(28)}, {6, 0}});
    c.call(0x1073f1e0, 0x103bec90);
}

void open_player_instructions(Cpu& c, std::uint32_t screen, std::uint32_t flag) {
    Frame frame(c, 0x103c1f30, 0x50, {.first_gpr = 28});
    c.put8(frame.at(0x6f), flag);
    const auto root = c.toc(root_slot);
    c.set({{28, screen}, {31, root}, {3, field::size}});
    c.call(0x10773520, 0x103c1f64);
    const auto copy = c.r(3);
    c.set(30, copy);
    if (copy) {
        c.set({{3, copy}, {4, screen}});
        c.link(0x103c1f7c);
        c.set(3, copy_construct(c, copy, screen));
        c.set(0, c.lr());
    }
    c.set(29, copy);
    c.set({{3, root}, {4, c.toc(-0x4cd4)}, {5, c.toc(-0x4cd8)}, {6, 1}, {7, 0}, {8, 0}});
    c.call(0x1073e4b0, 0x103c1f9c);
    c.compare(0, byte(c.r(3)), 0);
    if (!byte(c.r(3))) return;
    c.set({{3, root}, {4, 0}, {5, screen}, {6, 0}});
    c.call(0x1073f1e0, 0x103c1fbc);
    c.set({{3, root}, {4, 1}, {5, std::uint32_t(byte(c.u8(frame.at(0x6f))))}, {6, 0}});
    c.call(0x1073f1e0, 0x103c1fd4);
    c.set({{3, root}, {4, 2}, {5, copy}, {6, 0}});
    c.call(0x1073f1e0, 0x103c1fe8);
}

std::uint32_t copy_construct(Cpu& c, std::uint32_t screen, std::uint32_t source) {
    Frame frame(c, 0x103c5a90, 0x70, {.first_gpr = 30});
    c.put32(frame.at(0x88), screen);
    c.set({{30, source}, {31, c.toc(pool_slot)}});
    c.set(3, screen + 0x14);
    c.call(0x103c5a40, 0x103c5abc);
    // The three formation records: the edited one, the undo copy, and one at +0x87e4.
    c.set(3, screen + field::formation);
    c.call(0x103c59f0, 0x103c5ac8);
    c.set(3, screen + field::undo);
    c.call(0x103c59f0, 0x103c5ad4);
    c.set(3, screen + 0x87e4);
    c.call(0x103c59f0, 0x103c5ae4);
    c.compare_logical(0, source, 0);
    if (!source) {
        fail(c, frame, 0x103c5b00, 0x3d, 0x38, 0x138c);
        return screen;
    }
    c.set({{3, screen}, {4, source}});
    c.call(0x103c11b0, 0x103c5b54);  // copies the state
    return screen;
}

std::uint32_t set_view(Cpu& c, std::uint32_t screen, std::uint32_t view) {
    Frame frame(c, 0x103c60a0, 0x70, {.first_gpr = 29});
    c.set({{31, screen}, {30, view}, {29, c.toc(pool_slot)}});
    // The original tests 1, 2 then 3 in turn.
    bool valid = false;
    for (std::int32_t allowed = 1; allowed <= 3 && !valid; ++allowed) {
        c.compare(0, byte(view), allowed);
        valid = byte(view) == allowed;
    }
    if (!valid) {
        fail(c, frame, 0x103c60fc, 0x3d, 0x38, 0x144d);
        return 0;
    }
    c.put8(screen + field::previous_view, c.u8(screen + field::view));
    c.put8(screen + field::view, view);
    copy_cell_value(c, screen + field::previous_cell, screen + field::cell);
    c.set(4, screen + field::cell);  // as the leaf's arguments leave it
    return 1;
}

std::uint32_t set_captain(Cpu& c, std::uint32_t screen, std::uint32_t staff) {
    Frame frame(c, 0x103c6210, 0x70, {.first_gpr = 29});
    c.set({{31, screen}, {30, staff}, {29, c.toc(pool_slot)}});
    c.compare_logical(0, staff, 0);
    if (!staff) {
        fail(c, frame, 0x103c6250, 0x3d, 0x38, 0x1488);
        return 0;
    }
    if (read_only(c, screen)) return 0;
    c.compare(0, c.i32(staff), c.i32(screen + field::captain));
    if (c.u32(staff) != c.u32(screen + field::captain)) {
        save_undo(c, screen, 0x103c62cc);
        c.put32(screen + field::captain, c.u32(staff));
        mark_changed(c, screen);
    }
    return 1;
}

std::uint32_t choose_captain(Cpu& c, std::uint32_t screen) {
    Frame frame(c, 0x103c6310, 0x70, {.first_gpr = 29});
    c.set({{31, screen}, {29, c.toc(pool_slot)}});
    if (read_only(c, screen)) return 0;
    c.set(3, screen);
    c.link(0x103c6350);
    const auto best = best_player(c, screen);
    c.set({{0, c.lr()}, {3, best}, {30, best}});
    c.compare(0, byte(best), -1);
    if (byte(best) <= -1) return 0;
    const auto staff = entry(screen, best) + field::entry_staff;
    c.compare_logical(0, c.u32(staff), 0);
    if (!c.u32(staff)) {
        fail(c, frame, 0x103c638c, 0x3d, 0x38, 0x14ba);
        return 0;
    }
    c.set(3, screen);
    c.call(0x103c6180, 0x103c63dc);  // the entry holding the captain
    c.set(3, std::uint32_t(byte(c.r(3))));
    c.compare(0, byte(best), byte(c.r(3)));
    if (byte(c.r(3)) != byte(best)) {
        save_undo(c, screen, 0x103c63f8);
        c.put32(screen + field::captain, c.u32(c.u32(staff)));
        mark_changed(c, screen);
    }
    return 1;
}

std::uint32_t set_cell(Cpu& c, std::uint32_t screen, std::uint32_t cell) {
    Frame frame(c, 0x103c6cb0, 0x70, {.first_gpr = 30});
    c.set(31, screen);
    c.put32(frame.at(0x8c), cell);
    c.set(30, c.toc(pool_slot));
    c.compare_logical(0, c.u16(frame.at(0x8c)), 2);
    if (c.u16(frame.at(0x8c)) <= 2) c.compare_logical(0, c.u16(frame.at(0x8e)), 3);
    if (c.u16(frame.at(0x8c)) > 2 || c.u16(frame.at(0x8e)) > 3) {
        fail(c, frame, 0x103c6cfc, 0x3d, 0x38, 0x15cd);
        return 0;
    }
    copy_cell_value(c, screen + field::previous_cell, screen + field::cell);
    copy_cell_value(c, screen + field::cell, frame.at(0x8c));
    c.set(4, frame.at(0x8c));  // as the leaf's arguments leave it
    c.put8(screen + field::previous_view, c.u8(screen + field::view));
    return 1;
}

std::uint32_t undo(Cpu& c, std::uint32_t screen) {
    Frame frame(c, 0x103c7260, 0x40, {.first_gpr = 31});
    c.set(31, screen);
    c.compare(0, c.i8(screen + field::can_undo), 0);
    if (!c.i8(screen + field::can_undo)) return 0;
    c.set({{3, screen + field::formation}, {4, screen + field::undo}});
    c.call(0x103c1340, 0x103c7294);
    c.put8(screen + field::can_undo, 0);
    return 1;
}

std::uint32_t set_lineup(Cpu& c, std::uint32_t screen, std::uint32_t place, std::uint32_t entry_index) {
    Frame frame(c, 0x103c7380, 0xa0, {.first_gpr = 26});
    c.set({{31, screen}, {27, place}, {28, entry_index}, {26, c.toc(message_slot)}, {30, c.toc(pool_slot)}});
    if (!in_range(c, place, 0, 10)) {
        fail(c, frame, 0x103c73d0, 0x61, 0x3d, 0x16a3);
        return 0;
    }
    if (!in_range(c, entry_index, 0, 0x13)) {
        fail(c, frame, 0x103c7444, 0x42, 0x38, 0x16aa);
        return 0;
    }
    if (read_only(c, screen)) return 0;
    const auto lineup = screen + field::lineup;
    const auto at = lineup + std::uint32_t(byte(place));
    c.compare(0, byte(entry_index), c.i8(at));
    if (byte(entry_index) == c.i8(at)) {
        c.set(4, std::uint32_t(byte(entry_index)));  // as the original's comparison leaves it
        return 1;
    }
    save_undo(c, screen, 0x103c74cc);
    // The place that held the entry gets this place's entry.
    for (std::int8_t other = 0; other < 11; ++other)
        if (c.i8(lineup + std::uint32_t(other)) == byte(entry_index)) {
            c.set(4, c.u8(at));
            c.put8(lineup + std::uint32_t(other), c.u8(at));
        }
    c.compare(0, 11, 11);  // the loop's last test
    c.put8(at, entry_index);
    mark_changed(c, screen);
    return 1;
}

std::uint32_t swap_lineup(Cpu& c, std::uint32_t screen, std::uint32_t first, std::uint32_t second) {
    Frame frame(c, 0x103c7550, 0x80, {.first_gpr = 27});
    c.set({{31, screen}, {29, first}, {30, second}, {28, c.toc(pool_slot)}});
    if (!in_range(c, first, 0, 10) || !in_range(c, second, 0, 10)) {
        fail(c, frame, 0x103c75b4, 0x3d, 0x38, 0x16d7);
        return 0;
    }
    if (read_only(c, screen)) return 0;
    c.compare(0, byte(first), byte(second));
    if (byte(first) == byte(second)) return 1;
    save_undo(c, screen, 0x103c7630);
    const auto a = screen + field::lineup + std::uint32_t(byte(first));
    const auto b = screen + field::lineup + std::uint32_t(byte(second));
    const auto held = c.u8(a);
    c.set(4, c.u8(b));
    c.put8(a, c.u8(b));
    c.put8(b, held);
    mark_changed(c, screen);
    return 1;
}

std::uint32_t load(Cpu& c, std::uint32_t screen, std::uint32_t path, std::uint32_t preset) {
    Frame frame(c, 0x103c7ad0, 0x80, {.first_gpr = 27});
    c.set({{31, screen}, {29, path}, {27, preset}, {30, c.toc(pool_slot)}});
    c.compare_logical(0, path, 0);
    if (!path) {
        fail(c, frame, 0x103c7b0c, 0x3d, 0x38, 0x178c);
        return 0;
    }
    if (read_only(c, screen)) return 0;
    save_undo(c, screen, 0x103c7b78);
    c.set({{3, screen + field::formation}, {4, path}, {5, 1}, {6, preset}});
    c.call(0x10115000, 0x103c7b98);  // reads the file
    c.set(28, c.r(3));
    c.compare(0, byte(c.r(3)), 0);
    if (!byte(c.r(3))) {
        c.set({{3, screen + field::formation}, {4, screen + field::undo}});
        c.call(0x103c1340, 0x103c7bb4);
        c.put8(screen + field::can_undo, 0);
        return 0;
    }
    mark_changed(c, screen);
    c.compare(0, byte(preset), 0);
    if (!byte(preset)) {
        c.set({{3, c.u32(c.toc(managers_slot))}, {4, path}, {5, 0}, {6, 1}});
        c.call(0x101476e0, 0x103c7bf4);  // remembers the file
    }
    return 1;
}

std::uint32_t save(Cpu& c, std::uint32_t screen, std::uint32_t path, std::uint32_t named) {
    Frame frame(c, 0x103c7c10, 0xb0, {.first_gpr = 25});
    const auto managers = c.toc(managers_slot);
    c.set({{31, screen}, {28, path}, {25, named}, {30, managers}, {27, c.toc(pool_slot)}});
    c.compare_logical(0, path, 0);
    if (!path) {
        fail(c, frame, 0x103c7c50, 0x3d, 0x38, 0x17c4);
        return 0;
    }
    c.set(3, c.toc(root_slot));
    c.call(0x10324090, 0x103c7ca0);
    c.set({{4, screen + 0xf29}, {5, 0xc8}, {6, 1}});
    c.call(0x100caeb0, 0x103c7cb0);
    const auto description = screen + field::formation + field::formation_description;
    std::uint32_t saved = 0;
    if (named) {
        // Saved under the named manager's name, keeping the description.
        const auto kept = frame.at(0x5c);
        c.set({{3, kept}, {4, description}});
        c.call(0x1077dbf0, 0x103c7cc4);
        c.set({{3, c.u32(managers)}, {4, named}});
        c.call(0x10148eb0, 0x103c7cd0);
        c.set({{4, c.r(3)}, {3, description}});
        c.call(0x1077dbf0, 0x103c7cdc);
        c.set({{3, screen + field::formation}, {4, path}, {5, 1}, {6, 0}});
        c.call(0x10115cd0, 0x103c7cf0);
        saved = c.r(3);
        c.set({{29, saved}, {3, description}, {4, kept}});
        c.call(0x1077dbf0, 0x103c7d00);
    } else {
        c.set({{3, screen + field::formation}, {4, path}, {5, 1}, {6, 0}});
        c.call(0x10115cd0, 0x103c7d18);
        saved = c.r(3);
        c.set(29, saved);
        if (byte(saved) && c.u32(screen + field::match)) {
            c.set({{3, c.u32(screen + field::club)}, {4, 0}, {5, 1}});
            c.call(0x1014c490, 0x103c7d44);
            if (byte(c.r(3))) {
                // During a match the human's formation follows the saved one.
                c.set({{3, c.u32(managers)}, {4, c.u8(screen + field::opened_from)}, {5, c.u32(screen + field::club)},
                       {6, 0}});
                c.call(0x10146830, 0x103c7d64);
                c.set({{3, c.u32(managers)}, {4, c.u32(c.u32(screen + field::club) + 0x90)}});
                c.call(0x101466c0, 0x103c7d74);
                const auto current = c.r(3);
                c.set({{26, current}, {3, screen + field::formation}, {4, current}});
                c.call(0x1077dc90, 0x103c7d84);
                if (c.r(3) == 0) {
                    c.set({{3, current}, {4, screen + field::formation}});
                    c.call(0x103c1980, 0x103c7d98);
                }
            }
        }
    }
    c.compare(0, byte(saved), 0);
    if (!byte(saved)) return 0;
    c.put8(screen + field::can_undo, 0);
    c.set({{3, c.u32(managers)}, {4, path}, {5, 0}, {6, 1}});
    c.call(0x101476e0, 0x103c7dcc);
    return 1;
}

std::uint32_t set_default(Cpu& c, std::uint32_t screen, std::uint32_t index) {
    Frame frame(c, 0x103c7df0, 0x70, {.first_gpr = 28});
    const auto tactics = c.toc(tactics_slot);
    const auto pool = c.toc(pool_slot);
    c.set({{30, screen}, {29, index}, {28, tactics}, {31, pool}});
    c.compare(0, half(index), 0);
    bool valid = half(index) >= 0;
    if (valid) {
        c.set({{3, c.u32(tactics)}, {4, 1}});
        c.call(0x103bae90, 0x103c7e34);  // the number of formations
        c.compare(0, half(index), std::int32_t(c.r(3)));
        valid = half(index) < std::int32_t(c.r(3));
    }
    if (!valid) {
        fail(c, frame, 0x103c7e54, 0x3d, 0x38, 0x1818);
        return 0;
    }
    if (read_only(c, screen)) return 0;
    save_undo(c, screen, 0x103c7ec0);
    const auto formation = screen + field::formation;
    const auto default_formation = [&](std::uint32_t back) {
        c.set({{3, c.u32(tactics)}, {4, index}, {5, 1}});
        c.call(0x103af480, back);
        return c.r(3);
    };
    c.set({{4, default_formation(0x103c7edc)}, {3, formation}});
    c.call(0x103c1980, 0x103c7ee8);
    // The built-in shapes keep their own name; any other gets the default's.
    constexpr std::uint32_t names[] = {0x5f, 0x6f, 0x7f, 0x8d, 0x9d, 0xad, 0xbd, 0xcd};
    bool named = false;
    for (std::uint32_t i = 0; i < std::size(names) && !named; ++i) {
        const auto block = 0x30 * i;
        c.set({{3, default_formation(0x103c7ef8 + block)}, {4, pool + names[i]}});
        c.call(0x107393f0, 0x103c7f00 + block);
        c.compare(0, std::int32_t(c.r(3)), 0);
        if (c.r(3) != 0) continue;
        c.set({{3, formation}, {4, pool + names[i]}});
        c.call(0x10727890, 0x103c7f14 + block);
        named = true;
    }
    if (!named) {
        c.set({{4, default_formation(0x103c8078)}, {3, formation}});
        c.call(0x1077dbf0, 0x103c8084);
    }
    mark_changed(c, screen);
    return 1;
}

std::uint32_t unlock(Cpu& c, std::uint32_t screen) {
    Frame frame(c, 0x103c8160, 0x40, {.first_gpr = 31});
    c.set(31, screen);
    if (read_only(c, screen)) return 0;
    c.set(3, screen);
    c.call(0x103bade0, 0x103c8194);  // locked
    c.compare(0, byte(c.r(3)), 0);
    if (byte(c.r(3))) {
        save_undo(c, screen, 0x103c81ac);
        c.put8(screen + field::formation + field::formation_description, 0);
        mark_changed(c, screen);
    }
    return 1;
}

std::uint32_t lock(Cpu& c, std::uint32_t screen) {
    Frame frame(c, 0x103c81e0, 0x40, {.first_gpr = 31});
    c.set(31, screen);
    if (read_only(c, screen)) return 0;
    c.set(3, screen);
    c.call(0x103bade0, 0x103c8214);  // locked
    c.compare(0, byte(c.r(3)), 0);
    if (byte(c.r(3))) return 1;
    const auto manager_name = [&](std::uint32_t back) {
        c.set({{3, c.u32(c.toc(managers_slot))}, {4, 0}});
        c.call(0x10148eb0, back);
        return c.r(3);
    };
    const auto name = manager_name(0x103c8230);
    c.compare(0, c.i8(name), 0);
    if (!c.i8(name)) return 0;
    save_undo(c, screen, 0x103c8254);
    c.set({{4, manager_name(0x103c8270)}, {3, screen + field::formation + field::formation_description}});
    c.call(0x1077dbf0, 0x103c827c);
    mark_changed(c, screen);
    return 1;
}

std::uint32_t copy_cell(Cpu& c, std::uint32_t screen) {
    Frame frame(c, 0x103c8360, 0x40, {.first_gpr = 30});
    c.set(31, screen);
    if (view_is(c, screen, 1)) return 0;
    const auto ball = with_ball(c, screen);
    c.set({{30, ball}, {3, screen + field::formation}, {4, ball}, {5, c.u32(screen + field::cell)},
           {6, screen + field::cell_clipboard}});
    c.call(0x10112850, 0x103c83c4);
    c.put8(screen + field::clipboard, 1);
    return 1;
}

std::uint32_t copy_all_cells(Cpu& c, std::uint32_t screen) {
    Frame frame(c, 0x103c83f0, 0x40, {.first_gpr = 30});
    c.set(31, screen);
    if (view_is(c, screen, 1)) return 0;
    const auto ball = with_ball(c, screen);
    c.set({{30, ball}, {3, screen + field::formation}, {4, ball}, {5, screen + field::all_clipboard}});
    c.call(0x10112b90, 0x103c8450);
    c.put8(screen + field::clipboard, 2);
    return 1;
}

std::uint32_t paste(Cpu& c, std::uint32_t screen) {
    Frame frame(c, 0x103c8480, 0x50, {.first_gpr = 29});
    c.set(31, screen);
    if (read_only(c, screen) || view_is(c, screen, 1) || clipboard_empty(c, screen)) return 0;
    save_undo(c, screen, 0x103c84f4);
    const auto ball = with_ball(c, screen);
    c.compare(0, c.i8(screen + field::clipboard), 2);
    if (c.i8(screen + field::clipboard) == 2) {
        c.set({{30, ball}, {3, screen + field::formation}, {4, ball}, {5, screen + field::all_clipboard}});
        c.call(0x10112a00, 0x103c8544);
    } else {
        c.set({{29, ball}, {3, screen + field::formation}, {4, ball}, {5, c.u32(screen + field::cell)},
               {6, screen + field::cell_clipboard}});
        c.call(0x10112690, 0x103c857c);
    }
    mark_changed(c, screen);
    return 1;
}

std::uint32_t mirror_cell(Cpu& c, std::uint32_t screen) {
    Frame frame(c, 0x103c85b0, 0x40, {.first_gpr = 30});
    c.set(31, screen);
    if (read_only(c, screen) || view_is(c, screen, 1)) return 0;
    save_undo(c, screen, 0x103c8604);
    const auto ball = with_ball(c, screen);
    c.set({{30, ball}, {3, screen + field::formation}, {4, ball}, {5, c.u32(screen + field::cell)}});
    c.call(0x10113160, 0x103c863c);
    c.s.wr<1>(guest::reg::xer_ca, c.u16(screen + field::cell) <= 2);  // subfic's carry
    c.put16(screen + field::cell, 2 - c.u16(screen + field::cell));  // the opposite row
    mark_changed(c, screen);
    return 1;
}

std::uint32_t swap_players(Cpu& c, std::uint32_t screen, std::uint32_t first, std::uint32_t second) {
    Frame frame(c, 0x103c8680, 0x70, {.first_gpr = 28});
    c.set({{31, screen}, {28, first}, {29, second}, {30, c.toc(pool_slot)}});
    if (!in_range(c, first, 0, 10) || !in_range(c, second, 0, 10)) {
        fail(c, frame, 0x103c86f0, 0x3d, 0x38, 0x194a);
        return 0;
    }
    if (read_only(c, screen)) return 0;
    save_undo(c, screen, 0x103c875c);
    c.set({{3, screen + field::formation}, {4, first}, {5, second}});
    c.call(0x10111890, 0x103c8778);
    mark_changed(c, screen);
    return 0;  // the original returns 0 here too
}

std::uint32_t set_position(Cpu& c, std::uint32_t screen, std::uint32_t player, std::uint32_t position,
                           std::uint32_t with_ball_flag) {
    Frame frame(c, 0x103c87b0, 0x70, {.first_gpr = 29});
    c.set({{31, screen}, {29, player}});
    c.put16(frame.at(0x92), position);
    c.put8(frame.at(0x97), with_ball_flag);
    c.set(30, c.toc(pool_slot));
    if (!in_range(c, player, 0, 10)) {
        fail(c, frame, 0x103c8808, 0x3d, 0x38, 0x196f);
        return 0;
    }
    if (read_only(c, screen)) return 0;
    save_undo(c, screen, 0x103c8874);
    c.set({{3, screen + field::formation}, {4, player}, {5, std::uint32_t(half(c.u16(frame.at(0x92))))},
           {6, c.u8(frame.at(0x97))}});
    c.call(0x10111310, 0x103c8894);
    mark_changed(c, screen);
    return 1;
}

std::uint32_t swap_spots(Cpu& c, std::uint32_t screen, std::uint32_t first, std::uint32_t second) {
    Frame frame(c, 0x103c88c0, 0x80, {.first_gpr = 27});
    c.set({{31, screen}, {27, first}, {28, second}, {30, c.toc(pool_slot)}});
    if (!in_range(c, first, 0, 10) || !in_range(c, second, 0, 10)) {
        fail(c, frame, 0x103c8924, 0x3d, 0x38, 0x1993);
        return 0;
    }
    if (read_only(c, screen) || view_is(c, screen, 1)) return 0;
    save_undo(c, screen, 0x103c89a8);
    const auto ball = with_ball(c, screen);
    c.set({{29, ball}, {3, screen + field::formation}, {4, first}, {5, second}, {6, ball},
           {7, c.u32(screen + field::cell)}});
    c.call(0x10112f00, 0x103c89e8);
    mark_changed(c, screen);
    return 0;  // the original returns 0 here too
}

std::uint32_t move_spot(Cpu& c, std::uint32_t screen, std::uint32_t player, std::uint32_t spot) {
    Frame frame(c, 0x103c8a10, 0x70, {.first_gpr = 28});
    c.set({{31, screen}, {28, player}});
    c.put32(frame.at(0x90), spot);
    c.set(30, c.toc(pool_slot));
    if (!in_range(c, player, 0, 10)) {
        fail(c, frame, 0x103c8a68, 0x3d, 0x38, 0x19ba);
        return 0;
    }
    if (read_only(c, screen) || view_is(c, screen, 1)) return 0;
    save_undo(c, screen, 0x103c8aec);
    const auto ball = with_ball(c, screen);
    c.set({{29, ball}, {3, screen + field::formation}, {4, player}, {5, ball}, {6, c.u32(screen + field::cell)},
           {7, c.u32(frame.at(0x90))}});
    c.call(0x10111e60, 0x103c8b2c);
    mark_changed(c, screen);
    return 1;
}

namespace {
// 0x101d3b50: a match player's place code (>= 0 on the pitch, -1 on the
// bench, -2 ...), as the int8 the callers test; the whole r3 is returned.
std::uint32_t place_code(Cpu& c, std::uint32_t player, std::uint32_t back) {
    c.set(3, player);
    c.call(0x101d3b50, back);
    return c.r(3);
}
}

std::uint32_t swap_entries(Cpu& c, std::uint32_t screen, std::uint32_t first, std::uint32_t second,
                           std::uint32_t from_match) {
    Frame frame(c, 0x103ca8d0, 0x90, {.first_gpr = 23});
    c.set({{31, screen}, {27, first}, {28, second}, {23, from_match}, {26, c.toc(pool_slot)}});
    if (!in_range(c, first, 0, 0x13) || !in_range(c, second, 0, 0x13)) {
        fail(c, frame, 0x103ca938, 0x3d, 0x38, 0x1e4c);
        return 0;
    }
    c.compare(0, byte(from_match), 0);
    if (byte(from_match)) {
        c.compare_logical(0, c.u32(screen + 0xe64), 0);
        if (!c.u32(screen + 0xe64)) return 0;
    }
    if (!in_match(c, screen)) {
        c.compare(0, byte(first), 11);
        if (byte(first) >= 11) return 0;
        c.compare(0, byte(second), 11);
        if (byte(second) >= 11) return 0;
        save_undo(c, screen, 0x103cabc8);
        c.set({{3, screen + field::formation}, {4, first}, {5, second}});
        c.call(0x10111890, 0x103cabe4);
        mark_changed(c, screen);
        return 1;
    }
    const auto player = [&](std::uint32_t index, std::uint32_t back) {
        if (!byte(from_match)) return entry(screen, index);
        c.set({{3, c.u32(screen + field::match)}, {4, c.u8(screen + field::own_side)}, {5, index}});
        c.call(0x101f9440, back);
        return c.r(3);
    };
    const auto a = player(first, 0x103ca9c8);
    c.set(30, a);
    const auto b = player(second, 0x103ca9fc);
    c.set(29, b);
    const auto substitute_on = [&](std::uint32_t on, std::uint32_t off, std::uint32_t back) {
        c.set({{3, screen}, {4, on}, {5, off}});
        c.link(back);
        c.set(3, substitute(c, screen, on, off));
        c.set(0, c.lr());
        return 1u;
    };
    // Each test of a place code compares into cr0, as the original.
    const auto is = [&](std::uint32_t who, std::uint32_t back, std::int32_t value) {
        const auto code = byte(place_code(c, who, back));
        c.compare(0, code, value);
        return code == value;
    };
    const auto on_pitch = [&](std::uint32_t who, std::uint32_t back) {
        const auto code = byte(place_code(c, who, back));
        c.compare(0, code, 0);
        return code >= 0;
    };
    // A bench player (-1) with one on the pitch: a substitution.
    if (is(a, 0x103caa1c, -1)) {
        if (on_pitch(b, 0x103caa30) || is(b, 0x103caa44, -2)) return substitute_on(first, second, 0x103caa60);
    }
    if (on_pitch(a, 0x103caa70) || is(a, 0x103caa84, -2)) {
        if (is(b, 0x103caa98, -1)) return substitute_on(second, first, 0x103caab4);
    }
    // Two players on the pitch: they swap places.
    if (!(on_pitch(a, 0x103caac4) || is(a, 0x103caad8, -2))) return 0;
    if (!(on_pitch(b, 0x103caaec) || is(b, 0x103cab00, -2))) return 0;
    save_undo(c, screen, 0x103cab18);
    const auto slot = [&](std::uint32_t who, std::uint32_t test, std::uint32_t substitute_back, std::uint32_t back) {
        if (!is(who, test, -2)) return place_code(c, who, back);
        c.set(3, who);
        c.call(0x101e3f00, substitute_back);
        return c.r(3);
    };
    const auto second_slot = slot(b, 0x103cab2c, 0x103cab40, 0x103cab50);
    c.set(25, second_slot);
    const auto first_slot = slot(a, 0x103cab5c, 0x103cab70, 0x103cab80);
    c.set({{24, first_slot}, {3, screen + field::formation}, {4, first_slot}, {5, second_slot}});
    c.call(0x10111890, 0x103cab94);
    mark_changed(c, screen);
    return 1;
}

std::uint32_t substitute(Cpu& c, std::uint32_t screen, std::uint32_t on, std::uint32_t off) {
    Frame frame(c, 0x103cb3c0, 0x90, {.first_gpr = 20});
    c.set({{31, screen}, {26, on}, {27, off}, {25, c.toc(pool_slot)}});
    if (!in_range(c, on, 0, 0x13) || !in_range(c, off, 0, 0x13)) {
        fail(c, frame, 0x103cb424, 0x3d, 0x38, 0x1fb1);
        return 0;
    }
    if (!in_match(c, screen)) return 0;
    save_undo(c, screen, 0x103cb48c);
    // Taking off a player this screen brought on replaces that substitution:
    // both entries come back as the match has them, and the newcomer
    // replaces the player taken off then.
    const auto count = screen + field::substitutions;
    std::uint32_t undone = 0, i = 0;
    for (; byte(i) < c.i8(count); ++i)
        if (byte(off) == c.i8(screen + field::brought_on + std::uint32_t(byte(i)))) {
            undone = 1;
            break;
        }
    c.set({{24, undone}, {29, i}});
    const auto slot = std::uint32_t(byte(i));
    if (undone) {
        const auto restore = [&](std::uint32_t list, std::uint32_t fetch_back, std::uint32_t copy_back) {
            const auto index = c.u8(list + slot);
            c.set({{3, c.u32(screen + field::match)}, {4, c.u8(screen + field::side)}, {5, index}});
            c.call(0x101f9440, fetch_back);
            c.set({{4, c.r(3)}, {3, entry(screen, index)}});
            c.call(0x103c14a0, copy_back);  // the entry as the match has it
        };
        restore(screen + field::brought_on, 0x103cb504, 0x103cb528);
        restore(screen + field::taken_off, 0x103cb540, 0x103cb564);
        c.put8(screen + field::brought_on + slot, on);
        off = c.u8(screen + field::taken_off + slot);
        c.set(27, off);
    } else {
        c.put8(screen + field::brought_on + std::uint32_t(c.i8(count)), on);
        c.put8(screen + field::taken_off + std::uint32_t(c.i8(count)), off);
        c.put8(count, c.u8(count) + 1u);
    }
    const auto coming = entry(screen, on), leaving = entry(screen, off);
    c.set({{30, coming}, {28, leaving}, {3, c.u32(screen + field::match)}});
    c.call(0x101e8920, 0x103cb5d4);  // the match time, in elevenths of a minute
    const auto time = half(c.r(3));
    const auto high = std::int32_t((std::int64_t(0x2e8ba2e9) * time) >> 32);
    c.s.wr<1>(guest::reg::xer_ca, high < 0 && (high & 1));  // srawi's carry
    auto minute = high >> 1;  // time / 11
    minute += std::int32_t(std::uint32_t(minute) >> 31);
    c.put8(coming + field::entry_on_minute, std::uint32_t(minute));
    c.set(3, c.u32(screen + field::match));
    c.call(0x101e8900, 0x103cb600);  // the end of the current period
    // At most the period's last minute; each case keeps it in its own register.
    const auto cap = [&](unsigned reg, std::int32_t last) {
        const auto value = c.i8(coming + field::entry_on_minute);
        const auto capped = value >= last ? std::uint32_t(last) : std::uint32_t(std::int32_t(value));
        c.set(reg, capped);
        c.put8(coming + field::entry_on_minute, capped);
    };
    switch (half(c.r(3))) {
    case 0x1ef: cap(23, 45); break;
    case 0x3de: cap(22, 90); break;
    case 0x483: cap(21, 105); break;
    case 0x528: cap(20, 120); break;
    default: break;
    }
    // The newcomer takes the leaving player's place; he goes off (-4).
    std::uint32_t place;
    const auto leaving_code = byte(place_code(c, leaving, 0x103cb6e8));
    c.compare(0, leaving_code, -2);
    if (leaving_code == -2) {
        c.set(3, leaving);
        c.call(0x101e3f00, 0x103cb6fc);
        place = c.r(3);
        c.set({{4, place}, {3, coming}});
        c.call(0x101f1870, 0x103cb708);
    } else {
        place = place_code(c, leaving, 0x103cb714);
        c.set({{4, place}, {3, coming}});
        c.call(0x101f1870, 0x103cb720);
    }
    c.put8(leaving + field::entry_off_minute, c.u8(coming + field::entry_on_minute));
    c.set({{3, leaving}, {4, 0xfffffffcu}});
    c.call(0x101f1870, 0x103cb734);
    c.put8(leaving + field::entry_off, 1);
    mark_changed(c, screen);
    return 1;
}

std::uint32_t best_player(Cpu& c, std::uint32_t screen) {
    Frame frame(c, 0x103cba70, 0x50, {.first_gpr = 26});
    c.set({{30, screen}, {29, 0xffffffffu}});
    if (!in_match(c, screen)) return 0xffffffffu;
    std::uint32_t best = 0xffffffffu, best_rating = 0;
    c.set({{26, 0}, {31, 0}});
    for (std::uint32_t index = 0;; ++index) {
        c.compare(0, byte(index), 20);
        if (byte(index) >= 20) break;
        const auto player = entry(screen, index);
        c.set({{31, index}, {28, player}});
        if (byte(place_code(c, player, 0x103cbab8)) < 0) continue;
        c.set({{3, 0}, {4, player}, {5, c.u32(screen + field::club)}});
        c.call(0x10210800, 0x103cbad4);  // the player's rating
        const auto rating = c.r(3);
        c.set(27, rating);
        if (byte(best) == -1 || half(rating) > half(best_rating)) {
            best = std::uint32_t(byte(index));
            best_rating = rating;
            c.set({{29, best}, {26, best_rating}});
        }
    }
    return best;
}

std::uint64_t instruction_bit(std::uint32_t index) {
    const auto bit = byte(index);
    if (bit < 0 || bit > 47) return 0;
    return std::uint64_t(1) << bit;
}

void instruction_bit_registers(Cpu& c) {
    const auto index = byte(c.r(3));
    const auto bit = instruction_bit(c.r(3));
    c.compare(0, index, index >= 0 && index <= 47 ? index : 47);
    c.set({{0, std::uint32_t(index)}, {3, std::uint32_t(bit >> 32)}, {4, std::uint32_t(bit)}});
}
}
