// DATABASE.CPP's tables block, part 4: 0x100a1950, which decides which staff of the staff table stay in the
// database (database_tables.hpp). It is the staff counterpart of 0x100a34e0; its random tests draw from the sample
// pool inline (cursor, salt and wrap through rng_core as every pool user does).
#include "database_generation_detail.hpp"
#include "database_people_extra.hpp"
#include "database_tables.hpp"
#include "database_tables_detail.hpp"

namespace cm::aot::database {
namespace {
using namespace tables;
using guest::Frame;

constexpr std::uint32_t entry = 0x100a1950;
constexpr std::uint32_t allocate_cleared = 0x10775200, release = 0x10775170, quit = 0x1076f7d0,
                        compare_text = 0x1077dc90;

// A staff record, 0x70 bytes (offsets inferred from the tests): +0x0 its id, +0xc the club's linked record (+0x68
// the nation number, +0x120 a flag byte), +0x10, +0x14 and +0x1c records of ratings (halfwords), +0x20, +0x24 the
// staff's club (+0x1ec its rating, +0x2c its linked record, +0x30 a pointer to a word), +0x28 and +0x2c names,
// +0x5e a flag byte, +0x60 a code byte, +0x6b the role.
namespace staff {
constexpr std::uint32_t stride = 0x70, id = 0x0, linked = 0xc, link_b = 0x10, ratings_b = 0x14, ratings_a = 0x1c,
                        club_slot = 0x20, club = 0x24, name_a = 0x28, name_b = 0x2c, flag_5e = 0x5e, code = 0x60,
                        role = 0x6b;
}
// A group of kept staff per nation, 0x1c8 bytes: a list of ids, its count (+0x28) and the position of the weakest
// (+0x2c); then a second list at +0x30 with count +0x1c0 and weakest +0x1c4. List A is ranked by halfword +0x24
// of the staff's +0x1c record, list B by halfword +0x4 of its +0x14 record.
namespace group {
constexpr std::uint32_t stride = 0x1c8, count_a = 0x28, weakest_a = 0x2c, ids_b = 0x30, count_b = 0x1c0, weakest_b = 0x1c4;
}
constexpr std::int32_t rating_gate = 0x1d4c;  // 7500: a club rated at least this

// The registers the original keeps and the TOC cells it reads.
struct Run {
    Cpu& c;
    const Frame& frame;
    std::uint32_t out, mode;
    std::uint32_t pool, message, staff_cell, count_cell, groups_cell, salt_at, start, end_at, cursor_at, id_cell;
    std::uint32_t marks = 0, groups = 0, last = 0;
    bool small = false;

    std::int8_t size_mode() const { return std::int8_t(mode); }
    std::uint32_t staff_base() const { return c.u32(staff_cell); }

    void assertion(std::uint32_t at, std::uint32_t line, std::uint32_t name, std::uint32_t extension) {
        guest::report_assertion(c, {.at = at, .file = pool, .format = pool + 0xd, .title = pool + 0x1a,
                                    .name = frame.at(name), .extension = frame.at(extension), .line = line});
    }
    void after_assertion(std::uint32_t result) {
        c.set(4, c.toc(slot::success));
        c.set(0, 0);
        c.set(3, result);
    }

    // The volatile registers (r0, r3-r10), cr0 and XER[CA] are dead at every call of this pass: the callees (the
    // string compare, the random generator, the release) take their arguments in r3/r4 and read no other
    // volatile register (the checker's stub clobbers them after each call). So the tests are computed with C++
    // values; only the non-volatile registers the callees save into the stack image (r13-r31: r19 the walker of
    // the new table, r14 the person) are kept in step.
    std::int32_t half(std::uint32_t record, std::uint32_t at) { return c.i16(record + at); }
    bool present(std::uint32_t at) { return c.u32(at) != 0; }
    std::uint32_t row_of(std::uint32_t id) { return staff_base() + id * staff::stride; }

    // A staff's size class from the linked record's halfword +0xac: clamped to 5..15, scaled by `factor` and divided
    // by 15 (what the compiler's multiply-high division computes): the number of staff of the nation's group the
    // list may hold.
    std::int32_t room(std::uint32_t person, std::int32_t factor) {
        const auto size_class = std::clamp(half(c.u32(person + staff::linked), 0xac), 5, 15);
        return size_class * factor / 15;
    }
    std::int32_t key_a(std::uint32_t row) { return half(c.u32(row + staff::ratings_a), 0x24); }
    std::int32_t key_b(std::uint32_t row) { return half(c.u32(row + staff::ratings_b), 4); }

    // List A of a group (roles 1 and 3), ranked by the halfword +0x24 of the person's +0x1c record: while the
    // group has room the person joins (tracking the weakest); else it replaces the weakest when ranked higher, and
    // the weakest is found again (the first entry always counts as the new weakest, later ones when ranked lower).
    void place_in_list_a(std::uint32_t person, std::uint32_t g) {
        const auto weakest_row = [&] { return row_of(c.u32(g + (c.u32(g + group::weakest_a) << 2))); };
        const auto count = c.u32(g + group::count_a);
        const auto limit = room(person, 10);
        const auto id = c.u32(person + staff::id);
        if (std::int32_t(count) < limit) {
            c.put32(g + (count << 2), id);
            bool lowest = true;
            if (count != 0) lowest = key_a(person) < key_a(weakest_row());
            if (lowest) c.put32(g + group::weakest_a, c.u32(g + group::count_a));
            c.put32(g + group::count_a, c.u32(g + group::count_a) + 1);
            return;
        }
        if (key_a(person) <= key_a(weakest_row())) return;
        c.put32(g + (c.u32(g + group::weakest_a) << 2), id);
        for (std::int32_t i = 0; i < std::int32_t(c.u32(g + group::count_a)); ++i)
            if (i == 0 || key_a(row_of(c.u32(g + (std::uint32_t(i) << 2)))) < key_a(weakest_row()))
                c.put32(g + group::weakest_a, std::uint32_t(i));
    }

    // List B (roles 2 and 3), ranked by the halfword +0x4 of the +0x14 record; its ids start at +0x30.
    void place_in_list_b(std::uint32_t person, std::uint32_t g) {
        const auto weakest_row = [&] { return row_of(c.u32(g + group::ids_b + (c.u32(g + group::weakest_b) << 2))); };
        const auto count = c.u32(g + group::count_b);
        const auto limit = room(person, 100);
        const auto id = c.u32(person + staff::id);
        if (std::int32_t(count) < limit) {
            c.put32(g + group::ids_b + (count << 2), id);
            bool lowest = true;
            if (count != 0) lowest = key_b(person) < key_b(weakest_row());
            if (lowest) c.put32(g + group::weakest_b, c.u32(g + group::count_b));
            c.put32(g + group::count_b, c.u32(g + group::count_b) + 1);
            return;
        }
        if (key_b(person) <= key_b(weakest_row())) return;
        c.put32(g + group::ids_b + (c.u32(g + group::weakest_b) << 2), id);
        for (std::int32_t i = 0; i < std::int32_t(c.u32(g + group::count_b)); ++i)
            if (i == 0 || key_b(row_of(c.u32(g + group::ids_b + (std::uint32_t(i) << 2)))) < key_b(weakest_row()))
                c.put32(g + group::weakest_b, std::uint32_t(i));
    }

    // One staff of the grouping pass: roles 1 and 3 feed list A, roles 2 and 3 list B, of the nation's group.
    void group_person(std::uint32_t person) {
        const auto linked = c.u32(person + staff::linked);
        if (linked == 0) return;
        const auto role = c.u8(person + staff::role);
        const auto g = groups + c.u32(linked + nation::number) * group::stride;
        if ((role == 1 || role == 3) && present(person + staff::ratings_a)) place_in_list_a(person, g);
        if ((role == 2 || role == 3) && present(person + staff::ratings_b)) place_in_list_b(person, g);
    }

    // The kept staff of every group are marked.
    void mark_group_members() {
        c.set(4, 0);
        for (;;) {
            load32(c, 0, groups_cell);
            if (compare(c, std::int32_t(c.r(4)), std::int32_t(c.r(0))) >= 0) break;
            c.set(0, c.r(4) * group::stride);
            c.set(5, 0);
            c.set(3, groups + c.r(0));
            for (;;) {
                load32(c, 0, c.r(3) + group::count_a);
                if (compare(c, std::int32_t(c.r(5)), std::int32_t(c.r(0))) >= 0) break;
                c.set(0, c.r(5) << 2);
                c.set(6, 1);
                load32(c, 0, c.r(3) + c.r(0));
                c.set(5, c.r(5) + 1);
                c.put8(marks + c.r(0), c.r(6));
            }
            c.set(5, 0);
            for (;;) {
                load32(c, 0, c.r(3) + group::count_b);
                if (compare(c, std::int32_t(c.r(5)), std::int32_t(c.r(0))) >= 0) break;
                c.set(0, c.r(5) << 2);
                c.set(7, 1);
                c.set(6, c.r(3) + c.r(0));
                c.set(5, c.r(5) + 1);
                load32(c, 0, c.r(6) + group::ids_b);
                c.put8(marks + c.r(0), c.r(7));
            }
            c.set(4, c.r(4) + 1);
        }
    }

    // The inline draw "(sample + salt) % modulus == 0": the pool cursor moves on (a wrap draws a new salt through
    // rng_core at `back`), then the sample plus salt is tested (the compiler's multiply-high division is exact).
    bool draw_is_zero(std::int32_t modulus, std::uint32_t back) {
        generation::advance_pool(c, {3, 0}, cursor_at, end_at, salt_at, start, back);
        const auto sum = std::uint32_t(c.u16(c.u32(cursor_at))) + c.u32(salt_at);
        return std::int32_t(sum) % modulus == 0;
    }

    // r19 walks the new records; `keep` stores the walker's address for the id and advances it, `drop` stores 0.
    void keep(std::uint32_t person) {
        c.put32(out + (c.u32(person + staff::id) << 2), c.r(19));
        c.set(19, c.r(19) + staff::stride);
    }
    void drop(std::uint32_t person) { c.put32(out + (c.u32(person + staff::id) << 2), 0); }
    bool flagged(std::uint32_t record) { return std::int8_t(c.u8(record + nation::flag)) != 0; }
    bool small_mode() const { return std::int8_t(c.r(17)) != 0; }
    bool mode_is(std::int32_t value) const { return size_mode() == value; }
    bool role_is(std::uint32_t person, std::int32_t a, std::int32_t b = -1) {
        const auto role = c.u8(person + staff::role);
        return role == a || (b >= 0 && role == b);
    }
    // The tests of 0x100a2034: both names matched against two pairs of pool strings (a pair matches when both do).
    bool names_match(std::uint32_t person) {
        const auto equal = [&](std::uint32_t field, std::uint32_t text, std::uint32_t back) {
            c.set(3, c.u32(person + field));
            c.set(4, pool + text);
            c.call(compare_text, back);
            return c.r(3) == 0;
        };
        if (equal(staff::name_a, 0x64, entry + 0x708) && equal(staff::name_b, 0x6b, entry + 0x71c)) return true;
        return equal(staff::name_a, 0x72, entry + 0x730) && equal(staff::name_b, 0x78, entry + 0x744);
    }
    // The staff's record of ratings at `field` has the halfword at `at` of at least `limit`.
    bool rated_at_least(std::uint32_t person, std::uint32_t field, std::uint32_t at, std::int32_t limit) {
        return half(c.u32(person + field), at) >= limit;
    }
    // The halfword at least 0x96, or -1 or -2.
    bool rated_high_or_marker(std::uint32_t person, std::uint32_t field, std::uint32_t at) {
        const auto value = half(c.u32(person + field), at);
        return value >= 0x96 || value == -1 || value == -2;
    }
    // The staff's club has a +0x30 pointer to a word that differs from the shared id word.
    bool club_word_differs(std::uint32_t person) {
        return c.u32(c.u32(c.u32(person + staff::club) + 0x30)) != c.u32(id_cell);
    }
    // The club (non-null) has a linked record (+0x2c) whose flag byte is set.
    bool club_flagged(std::uint32_t person) {
        const auto club_record = c.u32(person + staff::club);
        return present(club_record + 0x2c) && flagged(c.u32(club_record + 0x2c));
    }

    // One staff of the keeping pass: the first test that applies decides (keep = a record in the new table,
    // drop = 0). The tests follow the original's order, which also fixes the order of the pool draws.
    void filter_person(std::uint32_t p) {
        if (c.u8(p + staff::role) == 6) return drop(p);
        const auto id = c.u32(p + staff::id);
        if (std::int8_t(c.u8(marks + id)) != 0) return keep(p);
        if (std::int32_t(c.u32(p + staff::id)) >= std::int32_t(c.u32(count_cell)) - 0x10) return keep(p);
        if (present(p + staff::name_a) && present(p + staff::name_b) && names_match(p)) return keep(p);
        if (!small_mode() && present(p + staff::linked) && flagged(c.u32(p + staff::linked))) return keep(p);
        if (small_mode() && present(p + staff::linked) && flagged(c.u32(p + staff::linked))) {
            if (present(p + staff::club) && present(c.u32(p + staff::club) + 0x30) && club_word_differs(p)) return keep(p);
            if (draw_is_zero(20, entry + 0x818)) return keep(p);
        }
        if (!small_mode() && present(p + staff::link_b) && flagged(c.u32(p + staff::link_b))) return keep(p);
        if (small_mode() && present(p + staff::link_b) && flagged(c.u32(p + staff::link_b)) && present(p + staff::club) &&
            present(c.u32(p + staff::club) + 0x30) && club_word_differs(p) && draw_is_zero(50, entry + 0x920))
            return keep(p);
        if (!small_mode() && present(p + staff::club) && club_flagged(p)) return keep(p);
        if (small_mode() && present(p + staff::club) && club_flagged(p)) {
            if (present(c.u32(p + staff::club) + 0x30) && club_word_differs(p)) return keep(p);
            if (draw_is_zero(35, entry + 0xa44)) return keep(p);
        }
        if (present(p + staff::club_slot) && (c.u8(p + staff::code) == 5 || c.u8(p + staff::code) == 0xc)) return keep(p);
        if (present(p + staff::club) && half(c.u32(p + staff::club), club::rating) >= rating_gate && !small_mode())
            return keep(p);
        if (small_mode() && present(p + staff::club) && present(c.u32(p + staff::club) + 0x2c) &&
            std::int8_t(c.u8(c.u32(c.u32(p + staff::club) + 0x2c) + 0x11f)) > 10 &&
            half(c.u32(p + staff::club), club::rating) >= rating_gate && draw_is_zero(3, entry + 0xb80))
            return keep(p);
        if (c.u8(p + staff::flag_5e) != 0 && !small_mode()) return keep(p);
        if (small_mode() && present(p + staff::club) && club_flagged(p) && c.u8(p + staff::role) == 1) return keep(p);
        const auto strong_a = [&](std::uint32_t at, std::int32_t limit) {
            return role_is(p, 1, 3) && present(p + staff::ratings_a) && rated_at_least(p, staff::ratings_a, at, limit);
        };
        if (!small_mode() && strong_a(0x24, 0x96)) return keep(p);
        if (!small_mode() && role_is(p, 1, 3) && present(p + staff::ratings_a) &&
            rated_high_or_marker(p, staff::ratings_a, 0x26))
            return keep(p);
        if (!small_mode() && strong_a(0x28, 0xa0)) return keep(p);
        if (!small_mode() && strong_a(0x2a, 0x9b)) return keep(p);
        if (strong_a(0x2c, 0x96)) return keep(p);
        const auto strong_b = [&](std::uint32_t at, std::int32_t limit) {
            return role_is(p, 2, 3) && present(p + staff::ratings_b) && rated_at_least(p, staff::ratings_b, at, limit);
        };
        if (strong_b(0x4, 0x96)) return keep(p);
        if (!mode_is(2) && role_is(p, 2, 3) && present(p + staff::ratings_b) &&
            rated_high_or_marker(p, staff::ratings_b, 0x6))
            return keep(p);
        if (!mode_is(2) && strong_b(0x8, 0xa0)) return keep(p);
        if (!mode_is(2) && strong_b(0xa, 0x9b)) return keep(p);
        if (strong_b(0xc, 0x96)) return keep(p);
        // From here the walker (the next free record of the new table: its old content is read) decides some drops.
        if (small_mode() && present(c.r(19) + staff::linked) &&
            std::int8_t(c.u8(c.u32(c.r(19) + staff::linked) + 0x11f)) < 6)
            return drop(p);
        if ((mode_is(2) || mode_is(1)) && present(p + staff::linked)) {
            const auto number = c.u32(c.u32(p + staff::linked) + nation::number);
            bool listed = false;
            for (const std::int32_t cell : {-0x74d8, -0x74dc, -0x74e0, -0x74e4, -0x74e8})
                if (!listed) listed = number == c.u32(c.toc(cell));
            for (const unsigned reg : {13u, 22u, 21u})
                if (!listed) listed = c.u32(c.u32(p + staff::linked) + nation::number) == c.u32(c.r(reg));
            if (listed && !draw_is_zero(20, entry + 0x1108)) return drop(p);
        }
        if (mode_is(2) && present(c.r(19) + staff::linked) &&
            std::int8_t(c.u8(c.u32(c.r(19) + staff::linked) + 0x11f)) < 10 && present(p + staff::ratings_b)) {
            if (half(c.u32(p + staff::ratings_b), 6) < 0x28) return drop(p);
            if (!draw_is_zero(15, entry + 0x11c0)) return drop(p);
        }
        if ((mode_is(2) || mode_is(1)) && present(c.r(19) + staff::ratings_b) &&
            half(c.u32(c.r(19) + staff::ratings_b), 6) < 0x50 && !draw_is_zero(12, entry + 0x1268))
            return drop(p);
        if (mode_is(4) && draw_is_zero(30, entry + 0x12e4)) return keep(p);
        if (mode_is(1) && draw_is_zero(75, entry + 0x1350)) return keep(p);
        if (mode_is(2) && draw_is_zero(100, entry + 0x13b8)) return keep(p);
        drop(p);
    }
};
}

// 0x100a1950. A frame of 0x130 bytes saving r13-r31 and LR. `out` receives, for each staff id, the address its
// record will have in the compacted staff table, or 0 for a dropped staff (the new addresses advance by 0x70
// from the table's start); `mode` (its low byte: 1, 2 and 4 are special) changes the quotas and the random tests.
// First it generates the extra staff (0x100a3db0); a full mode (not 1 or 2) then groups the staff by nation, keeping
// the best by the ratings (the count follows the nation's size class) and marking them, and a pass of tests keeps
// or drops the rest. A failed allocation asserts and calls 0x1076f7d0, which ends the game. Returns 1, or 0 for
// a null count cell or `out`.
std::uint32_t choose_kept_staff(Cpu& c, std::uint32_t out, std::uint32_t mode) {
    c.set(0, c.lr());  // mfspr r0,LR
    Frame frame(c, entry, 0x130, {.first_gpr = 13});
    const auto first_cell = c.toc(-0x7b98);
    c.set(5, first_cell);
    guest::keep(c, {{15, out}, {21, c.toc(-0x74f4)}, {16, mode}, {22, c.toc(-0x74f0)}, {13, c.toc(-0x74ec)},
                    {23, c.toc(-0x7a0c)}, {24, c.toc(-0x7b04)}, {25, c.toc(-0x7b90)}, {26, c.toc(-0x7b8c)},
                    {27, c.toc(-0x7b88)}, {28, c.toc(-0x7b84)}, {14, c.toc(slot::message)},
                    {29, c.toc(slot::text_pool)}, {30, c.toc(-0x7be0)}, {31, c.toc(-0x7be4)}});
    Run x{c, frame, out, mode, c.r(29), c.r(14), c.r(30), c.r(31), c.r(24), c.r(25), c.r(26), c.r(27), c.r(28), c.r(23)};
    c.set(0, c.u32(first_cell));
    if (compare_logical(c, c.r(0)) == 0) {
        x.assertion(entry + 0x70, 0x277, 0xb8, 0x50);
        x.after_assertion(0);
        return 0;
    }
    if (compare_logical(c, out) == 0) {
        x.assertion(entry + 0xd4, 0x27e, 0x98, 0x48);
        x.after_assertion(0);
        return 0;
    }
    c.set(0, extsb(mode));
    bool small = compare(c, x.size_mode(), 2) == 0;
    if (!small) {
        c.set(0, extsb(mode));
        small = compare(c, x.size_mode(), 1) == 0;
    }
    c.set(17, small);
    guest::enter(c, entry + 0x144);
    generate_extra_staff(c);
    c.set(3, c.u32(x.count_cell));
    c.set(4, 1);
    c.call(allocate_cleared, entry + 0x150);
    c.set(20, c.r(3));
    compare(c, std::int32_t(c.r(3)));
    if (c.r(20) == 0) {
        x.assertion(entry + 0x16c, 0x297, 0x78, 0x40);
        c.set(4, c.toc(slot::success));
        c.set(0, 0);
        c.set(3, 0xffffffff);
        c.call(quit, entry + 0x1b4);
    }
    x.marks = c.r(20);
    load32(c, 3, x.count_cell);
    c.set(0, extsb(c.r(17)));
    compare(c, std::int32_t(c.r(0)));
    load32(c, 4, x.staff_cell);
    c.set(0, c.r(3) - 1);
    c.set(0, c.r(0) * staff::stride);
    c.set(18, c.r(4) + c.r(0));
    x.last = c.r(18);
    if (c.r(17) == 0) {
        load32(c, 3, x.groups_cell);
        c.set(4, group::stride);
        c.call(allocate_cleared, entry + 0x1dc);
        c.set(19, c.r(3));
        compare(c, std::int32_t(c.r(3)));
        if (c.r(19) == 0) {
            x.assertion(entry + 0x1f8, 0x2a2, 0x58, 0x38);
            c.set(4, c.toc(slot::success));
            c.set(0, 0);
            c.set(3, 0xffffffff);
            c.call(quit, entry + 0x240);
        }
        x.groups = c.r(19);
        load32(c, 14, x.staff_cell);
        for (; compare_logical(c, c.r(14), x.last) <= 0; c.set(14, c.r(14) + staff::stride)) x.group_person(c.r(14));
        x.mark_group_members();
        c.set(3, c.r(19));
        c.call(release, entry + 0x664);
    }
    load32(c, 19, x.staff_cell);
    load32(c, 14, x.staff_cell);
    for (; compare_logical(c, c.r(14), x.last) <= 0; c.set(14, c.r(14) + staff::stride)) x.filter_person(c.r(14));
    c.set(3, c.r(20));
    c.call(release, entry + 0x1430);
    c.set(3, 1);
    return 1;
}
}
