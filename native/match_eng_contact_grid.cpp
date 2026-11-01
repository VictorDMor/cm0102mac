// MATCH_ENG.CPP, contact: the grid of players around the ball (0x101f7c10). The pitch is cut into 9 rows by 12
// columns of cells holding the players standing there (match +0xa08, 0x450 per row, 0x5c per cell: 22 player
// pointers, 11 per side, and the counts at +0x58). Each cell's players get an entry in the caller's grid.
#include "match_eng_contact.hpp"
#include "match_eng_shots.hpp"

namespace cm::aot::match_eng {
using namespace guest;
using namespace shots_detail;

namespace {
constexpr std::uint32_t flags_of = 0x10289560, memset_of = 0x10778300;
std::int32_t mulhw(std::int32_t a, std::int32_t b) { return std::int32_t(std::int64_t(a) * b >> 32); }
// A division written as mulhw and a shift: the shifted product and the sign bit that rounds it toward zero.
struct Quotient { std::int32_t shifted, value; };
Quotient divide_by_magic(std::int32_t value, std::int32_t magic, unsigned shift) {
    const auto shifted = mulhw(magic, value) >> shift;
    return {shifted, shifted + std::int32_t(std::uint32_t(shifted) >> 31)};
}
}

// 0x101f7c10: `grid` gets the ball's cell (bytes 0 and 1), and for each side and position of a player standing in a cell
// the cell's centre (bytes +2, +3 of the entry at +8 + side*0x58 + position*8), his record's first byte (+4), his flags
// halfword (+6) and a present flag (+8). The player holding the ball takes the middle of his cell; the others a random
// ninth of it, retried while that ninth is crowded.
void fill_contact_grid(Cpu& c, std::uint32_t match, std::uint32_t grid) {
    Frame frame(c, 0x101f7c10, 0x110, {.first_gpr = 16});
    Kept<26> match_kept(c, match);
    Kept<28> grid_kept(c, grid);
    const auto success = c.toc(-0x7bac), buffer = c.toc(-0x7ba8), pool = c.toc(-0x52cc);
    keep(c, {{29, success}, {30, buffer}, {31, pool}});
    const auto assertion = [&](std::uint32_t at, std::uint32_t name, std::uint32_t extension, std::uint32_t line) {
        report_assertion(c, {.at = at, .file = pool + 0x45, .format = pool + 0x53, .title = pool + 0x60, .name = name,
                             .extension = extension, .line = line});
    };
    if (compare_logical(c, grid) == 0) assertion(0x101f7c50, frame.at(0xa2), frame.at(0x56), 0x431c);
    c.set(3, u(s8(c.u8(match + 0x791d))));
    c.put8(grid, c.r(3));
    c.set(3, u(s8(c.u8(match + 0x791e))));
    c.put8(grid + 1, c.r(3));
    for (Kept<17> side(c, 0); compare(c, s8(side), 2) < 0; side = side + 1) {
        c.put32(frame.at(0x48), match + u(s8(side) * std::int32_t(side_size)) + players_at);
        for (Kept<16> slot(c, 0); compare(c, s8(slot), 0xb) < 0; slot = slot + 1) {
            c.put8(grid + 8 + u(s8(side) * 0x58) + u(s8(slot) * 8), 0);
            const auto walked = c.u32(frame.at(0x48));
            c.put32(frame.at(0x48), walked + player_size);
            c.put32(frame.at(0x44), walked);
        }
    }
    const auto entry = [&](std::uint32_t side, std::uint32_t position) { return grid + 8 + u(s8(side) * 0x58) + u(s8(position) * 8); };
    const auto holder = [&] { return c.u32(match + 0xe000); };
    const auto sub_cells = frame.at(0x5b);
    const auto roll_bytes = [&](std::uint32_t a_slot, std::uint32_t b_slot) {
        return sub_cells + u(s8(c.u8(a_slot)) * 3) + u(s8(c.u8(b_slot)));
    };
    for (Kept<19> row_k(c, 0); compare(c, s8(row_k), 9) < 0; row_k = row_k + 1) {
        for (Kept<18> column_k(c, 0); (c.set(3, u(s8(column_k))), compare(c, s8(column_k), 12) < 0); column_k = column_k + 1) {
            // The cell's first sub-cell: three to a cell.
            c.put8(frame.at(0x3d), u(divide_by_magic(s8(row_k) * 27, 0x38e38e39, 1).value + 1));
            c.put8(frame.at(0x3c), u(divide_by_magic(s8(column_k) * 36, 0x2aaaaaab, 1).value + 1));
            const auto cell = match + u(s8(row_k) * 0x450) + u(s8(column_k) * 0x5c) + 0xa08;
            keep(c, {{20, cell}});
            c.set({{3, sub_cells}, {4, 0}, {5, 9}});
            c.call(memset_of, 0x101f7dac);
            Kept<27> first(c, u(s8(draw(c, 2, 0x101f7db4))));
            const auto count_of = [&](std::uint32_t side) { return s8(c.u8(cell + 0x58 + u(s8(side)))); };
            const auto other = [&] { return u(s8(first) == 0); };
            // The side drawn first.
            for (Kept<23> index(c, 0); (c.set(3, u(count_of(first))), compare(c, s8(index), count_of(first)) < 0); index = index + 1) {
                c.set({{5, u(s8(first))}, {4, u(s8(index))}});
                Kept<22> player(c, c.u32(cell + u(s8(first) * 0x2c) + u(s8(index) * 4)));
                c.set(3, player);
                const auto found = position(c, player);
                if (compare(c, s8(found), 0) < 0) {
                    assertion(0x101f7e00, frame.at(0x83), frame.at(0x51), 0x4340);
                    continue;
                }
                Kept<25> where(c, found);
                const auto base = entry(first, where) - grid - 8;
                c.set(5, match + 0x10000);
                if (player == holder()) {
                    c.put8(grid + base + 2, c.u8(frame.at(0x3d)));
                    c.put8(grid + base + 3, c.u8(frame.at(0x3c)));
                    c.put8(grid + base + 4, c.u8(player));
                    c.set({{3, player}, {4, u(s8(first) * 0x58)}, {5, c.u8(player)}, {6, base + 4}});
                    c.call(flags_of, 0x101f7ec4);
                    c.put16(grid + base + 6, c.r(3));
                    c.set({{4, 1}, {5, u(s8(first))}, {3, base}});
                    c.put8(grid + base + 8, 1);
                    c.put8(frame.at(0x5f), 1);
                    continue;
                }
                c.set(3, 0);
                c.put16(frame.at(0x40), 0);
                for (;;) {
                    c.set(5, u(s8(draw(c, 3, 0x101f7f1c))));
                    c.put8(frame.at(0x3b), c.r(5));
                    c.put8(frame.at(0x3a), u(s8(draw(c, 3, 0x101f7f2c))));
                    c.set(3, u(c.i16(frame.at(0x40))));
                    c.set(4, c.r(3) + 1);
                    c.put16(frame.at(0x40), c.r(4));
                    c.set(3, match + 0x10000);
                    // Retry while the roll hits the middle sub-cell of the ball holder's own cell.
                    bool retry = false;
                    if (compare_logical(c, holder()) != 0) {
                        c.set(3, holder());
                        c.set(5, u(s8(row(c, holder()))));
                        if (compare(c, s8(row_k), s8(c.r(5))) == 0) {
                            c.set(3, holder());
                            c.set(4, u(s8(column(c, holder()))));
                            if (compare(c, s8(column_k), s8(c.r(4))) == 0 && compare(c, s8(c.u8(frame.at(0x3b))), 1) == 0) {
                                c.set(4, c.u8(frame.at(0x3a)));
                                retry = compare(c, s8(c.u8(frame.at(0x3a))), 1) == 0;
                            }
                        }
                    }
                    if (retry) continue;
                    c.set({{3, u(s8(c.u8(frame.at(0x3a))))}, {5, sub_cells}});
                    c.set(4, u(s8(c.u8(roll_bytes(frame.at(0x3b), frame.at(0x3a))))));
                    const auto tries = divide_by_magic(c.i16(frame.at(0x40)), 0x51eb851f, 4);
                    c.set(3, u(tries.shifted < 0));
                    // A sub-cell already taken more often than the tries divided by fifty is rejected.
                    if (compare(c, s8(c.r(4)), tries.value) > 0) continue;
                    break;
                }
                c.set(4, u(s8(c.u8(frame.at(0x3d)) + c.u8(frame.at(0x3b)) - 1)));
                c.put8(grid + base + 2, c.r(4));
                c.set(4, u(s8(c.u8(frame.at(0x3c)) + c.u8(frame.at(0x3a)) - 1)));
                c.put8(grid + base + 3, c.r(4));
                c.set(4, c.u8(player));
                c.put8(grid + base + 4, c.r(4));
                c.set(3, player);
                c.call(flags_of, 0x101f8078);
                c.put16(grid + base + 6, c.r(3));
                c.put8(grid + base + 8, 1);
                c.put8(roll_bytes(frame.at(0x3b), frame.at(0x3a)), 1);
                c.set({{3, 1}, {4, u(s8(c.u8(frame.at(0x3a))))}, {5, sub_cells}});
            }
            // The other side.
            for (Kept<23> index(c, 0);; index = index + 1) {
                c.set(4, u(s8(index)));
                c.set(5, c.u8(cell + 0x58 + other()));
                c.set(3, u(s8(c.r(5))));
                if (!(compare(c, s8(index), s8(c.r(5))) < 0)) break;
                Kept<21> player(c, c.u32(cell + other() * 0x2c + u(s8(index) * 4)));
                c.set(3, player);
                const auto found = position(c, player);
                if (compare(c, s8(found), 0) < 0) {
                    assertion(0x101f8148, frame.at(0x64), frame.at(0x4c), 0x4369);
                    continue;
                }
                Kept<24> where(c, found);
                const auto base = entry(other(), where) - grid - 8;
                c.set(3, match + 0x10000);
                if (player == holder()) {
                    c.put8(grid + base + 2, c.u8(frame.at(0x3d)));
                    c.put8(grid + base + 3, c.u8(frame.at(0x3c)));
                    c.set(4, c.u8(player));
                    c.put8(grid + base + 4, c.r(4));
                    c.set(3, player);
                    c.call(flags_of, 0x101f8224);
                    c.set(5, c.r(3));
                    c.put16(grid + base + 6, c.r(5));
                    c.set(5, 1);
                    c.set(4, other());
                    c.set(3, base);
                    c.put8(grid + base + 8, 1);
                    c.set(4, 1);
                    c.put8(frame.at(0x5f), 1);
                    continue;
                }
                c.set(4, 0);
                c.put16(frame.at(0x3e), 0);
                for (;;) {
                    c.put8(frame.at(0x39), u(s8(draw(c, 3, 0x101f8290))));
                    c.put8(frame.at(0x38), u(s8(draw(c, 3, 0x101f82a0))));
                    c.set(3, u(c.i16(frame.at(0x3e))));
                    c.put16(frame.at(0x3e), c.r(3) + 1);
                    c.set(3, match + 0x10000);
                    bool retry = false;
                    if (compare_logical(c, holder()) != 0) {
                        c.set(4, match + 0x10000);
                        c.set(3, holder());
                        const auto holder_row = s8(row(c, holder()));
                        if (compare(c, s8(row_k), holder_row) == 0) {
                            c.set(3, holder());
                            const auto holder_column = s8(column(c, holder()));
                            c.set({{3, u(holder_column)}, {4, u(s8(column_k))}});
                            if (compare(c, s8(column_k), holder_column) == 0 && compare(c, s8(c.u8(frame.at(0x39))), 1) == 0)
                                retry = compare(c, s8(c.u8(frame.at(0x38))), 1) == 0;
                        }
                    }
                    if (retry) continue;
                    c.set(4, sub_cells);
                    const auto seen = s8(c.u8(roll_bytes(frame.at(0x39), frame.at(0x38))));
                    c.set({{3, u(seen)}, {5, u(seen)}});
                    c.set(4, 0x51eb851f);
                    const auto tries = divide_by_magic(c.i16(frame.at(0x3e)), 0x51eb851f, 4);
                    c.set(3, u(tries.shifted < 0));
                    if (compare(c, seen, tries.value) > 0) continue;
                    break;
                }
                c.set(5, u(s8(c.u8(frame.at(0x3d)) + c.u8(frame.at(0x39)) - 1)));
                c.put8(grid + base + 2, c.r(5));
                c.set(6, u(s8(c.u8(frame.at(0x3c)) + c.u8(frame.at(0x38)) - 1)));
                c.put8(grid + base + 3, c.r(6));
                c.set(5, u(std::countl_zero(std::uint32_t(s8(first)))));
                c.set(6, c.u8(player));
                c.put8(grid + base + 4, c.r(6));
                c.set(5, other() * 0x58);
                c.set(4, base + 4);
                c.set(3, player);
                c.call(flags_of, 0x101f8404);
                c.set(5, c.r(3));
                c.put16(grid + base + 6, c.r(5));
                c.set(5, 1);
                c.put8(grid + base + 8, 1);
                c.set(5, 1);
                c.put8(roll_bytes(frame.at(0x39), frame.at(0x38)), 1);
                c.set({{3, u(s8(c.u8(frame.at(0x39))) * 3)}, {4, sub_cells}});
            }
        }
    }
}
}
