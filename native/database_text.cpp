// DATABASE.CPP's text block, part 1: a person's name in the seven layouts and the
// comparators that sort records by their text (database_text.hpp).
#include "database_text.hpp"
#include "database_records.hpp"
#include "database_text_detail.hpp"
#include <array>
#include <initializer_list>

namespace cm::aot::database {
using namespace text;

namespace {
// One alternative of a layout: the first of its strings that exist and are not empty.
// A string is "present" when its pointer is not null and strlen answers nonzero.
// `first` (and `second` for a pair) pick among the common name (A, r29), the first
// name (B, r30) and the surname (C, r31). A step copies one string, or formats one or
// two of them with `format` (an offset in the pool); `char5`/`char6` pass the first
// character of a string instead of the string. `check` addresses are the cmplwi of
// each presence test, `back` is the address after the original's call.
enum class Name { A, B, C, None };
struct Step {
    Name first, second;
    std::uint32_t check, check_second, back;
    std::uint32_t format = 0;  // 0: strcpy
    Name arg5 = Name::None, arg6 = Name::None;
    bool char5 = false, char6 = false;
};
using Steps = std::initializer_list<Step>;
constexpr std::uint32_t format_pair = 0x190a, format_initial_surname = 0x1910, format_surname_comma = 0x1916,
                        format_comma_initial = 0x191d, format_initial = 0x1924, format_two_initials = 0x1927,
                        format_two_initials_spaced = 0x192c;

const Steps layout1{  // "Forename Surname"
    {Name::A, Name::None, 0x100cafbc, 0, 0x100cafe0},
    {Name::B, Name::C, 0x100cafe4, 0x100caffc, 0x100cb028, format_pair, Name::B, Name::C},
    {Name::C, Name::None, 0x100cb02c, 0, 0x100cb050},
    {Name::A, Name::None, 0x100cb054, 0, 0x100cb078},
    {Name::B, Name::None, 0x100cb07c, 0, 0x100cb0a0}};
const Steps layout2{  // "F.Surname"
    {Name::A, Name::None, 0x100cb0a8, 0, 0x100cb0cc},
    {Name::B, Name::C, 0x100cb0d0, 0x100cb0e8, 0x100cb118, format_initial_surname, Name::B, Name::C, true},
    {Name::C, Name::None, 0x100cb11c, 0, 0x100cb140},
    {Name::B, Name::None, 0x100cb144, 0, 0x100cb168}};
const Steps layout3{  // "Surname, Forename"
    {Name::A, Name::None, 0x100cb170, 0, 0x100cb194},
    {Name::B, Name::C, 0x100cb198, 0x100cb1b0, 0x100cb1dc, format_surname_comma, Name::C, Name::B},
    {Name::C, Name::None, 0x100cb1e0, 0, 0x100cb204},
    {Name::B, Name::None, 0x100cb208, 0, 0x100cb22c}};
const Steps layout4{  // "Surname, F"
    {Name::A, Name::None, 0x100cb234, 0, 0x100cb258},
    {Name::B, Name::C, 0x100cb25c, 0x100cb274, 0x100cb2a4, format_comma_initial, Name::C, Name::B, false, true},
    {Name::C, Name::None, 0x100cb2a8, 0, 0x100cb2cc},
    {Name::B, Name::None, 0x100cb2d0, 0, 0x100cb2f4}};
const Steps layout5{  // the common name or the surname or the forename
    {Name::A, Name::None, 0x100cb2fc, 0, 0x100cb320},
    {Name::C, Name::None, 0x100cb324, 0, 0x100cb348},
    {Name::B, Name::None, 0x100cb34c, 0, 0x100cb370}};
const Steps layout6{  // initials
    {Name::A, Name::None, 0x100cb43c, 0, 0x100cb468, format_initial, Name::A, Name::None, true},
    {Name::B, Name::C, 0x100cb46c, 0x100cb484, 0x100cb4b8, format_two_initials, Name::B, Name::C, true, true},
    {Name::C, Name::None, 0x100cb4bc, 0, 0x100cb4e8, format_initial, Name::C, Name::None, true},
    {Name::B, Name::None, 0x100cb4ec, 0, 0x100cb518, format_initial, Name::B, Name::None, true}};
const Steps layout7{  // "Forename Surname", the common name last
    {Name::B, Name::C, 0x100cb378, 0x100cb390, 0x100cb3bc, format_pair, Name::B, Name::C},
    {Name::C, Name::None, 0x100cb3c0, 0, 0x100cb3e4},
    {Name::A, Name::None, 0x100cb3e8, 0, 0x100cb40c},
    {Name::B, Name::None, 0x100cb410, 0, 0x100cb434}};

// `cmplwi rX,0; beq; or r3,rX,rX; bl strlen; cmplwi r3,0; beq`
bool present(Cpu& c, std::uint32_t string, std::uint32_t check) {
    if (compare_logical(c, string) == 0) return false;
    c.set(3, string);
    c.call(strlen_entry, check + 0x10);
    return compare_logical(c, c.r(3)) != 0;
}
const Steps second_layout1{
    {Name::A, Name::None, 0x100cb80c, 0, 0x100cb830},
    {Name::B, Name::C, 0x100cb834, 0x100cb84c, 0x100cb878, format_pair, Name::B, Name::C, false, false},
    {Name::C, Name::None, 0x100cb87c, 0, 0x100cb8a0},
    {Name::B, Name::None, 0x100cb8a4, 0, 0x100cb8c8}};
const Steps second_layout2{
    {Name::A, Name::None, 0x100cb8d0, 0, 0x100cb8f4},
    {Name::B, Name::C, 0x100cb8f8, 0x100cb910, 0x100cb940, format_initial_surname, Name::B, Name::C, true, false},
    {Name::C, Name::None, 0x100cb944, 0, 0x100cb968},
    {Name::B, Name::None, 0x100cb96c, 0, 0x100cb990}};
const Steps second_layout3{
    {Name::A, Name::None, 0x100cb998, 0, 0x100cb9bc},
    {Name::B, Name::C, 0x100cb9c0, 0x100cb9d8, 0x100cba04, format_surname_comma, Name::C, Name::B, false, false},
    {Name::C, Name::None, 0x100cba08, 0, 0x100cba2c},
    {Name::B, Name::None, 0x100cba30, 0, 0x100cba54}};
const Steps second_layout4{
    {Name::A, Name::None, 0x100cba5c, 0, 0x100cba80},
    {Name::B, Name::C, 0x100cba84, 0x100cba9c, 0x100cbacc, format_comma_initial, Name::C, Name::B, false, true},
    {Name::C, Name::None, 0x100cbad0, 0, 0x100cbaf4},
    {Name::B, Name::None, 0x100cbaf8, 0, 0x100cbb1c}};
const Steps second_layout5{
    {Name::A, Name::None, 0x100cbb24, 0, 0x100cbb48},
    {Name::C, Name::None, 0x100cbb4c, 0, 0x100cbb70},
    {Name::B, Name::None, 0x100cbb74, 0, 0x100cbb98}};
const Steps second_layout6{
    {Name::A, Name::None, 0x100cbc8c, 0x0, 0x100cbcb8, format_initial, Name::A, Name::None, true, false},
    {Name::B, Name::C, 0x100cbcbc, 0x100cbcd4, 0x100cbd08, format_two_initials_spaced, Name::B, Name::C, true, true},
    {Name::C, Name::None, 0x100cbd0c, 0x0, 0x100cbd38, format_initial, Name::C, Name::None, true, false},
    {Name::B, Name::None, 0x100cbd3c, 0x0, 0x100cbd68, format_initial, Name::B, Name::None, true, false}};
const Steps second_layout7{
    {Name::B, Name::C, 0x100cbba0, 0x100cbbb8, 0x100cbbe4, format_pair, Name::B, Name::C, false, false},
    {Name::A, Name::None, 0x100cbbe8, 0, 0x100cbc0c},
    {Name::C, Name::None, 0x100cbc10, 0, 0x100cbc34},
    {Name::A, Name::None, 0x100cbc38, 0, 0x100cbc5c},
    {Name::B, Name::None, 0x100cbc60, 0, 0x100cbc84}};
const std::array<const Steps*, 8> layouts_second{nullptr, &second_layout1, &second_layout2, &second_layout3, &second_layout4, &second_layout5, &second_layout6, &second_layout7};

// The layouts' steps: the first of the alternatives whose strings are present.
void apply_layout(Cpu& c, const Steps& steps, std::uint32_t common, std::uint32_t first_name, std::uint32_t surname,
                  std::uint32_t buffer, std::uint32_t pool) {
    const auto value = [&](Name name) { return name == Name::A ? common : name == Name::B ? first_name : surname; };
    for (const auto& step : steps) {
        if (!present(c, value(step.first), step.check)) continue;
        if (step.second != Name::None && !present(c, value(step.second), step.check_second)) continue;
        if (step.format == 0) {
            c.set({{3, buffer}, {4, value(step.first)}});
        } else {
            const auto argument = [&](Name name, bool character) {
                return character ? std::uint32_t(extsb(c.u8(value(name)))) : value(name);
            };
            c.set(5, argument(step.arg5, step.char5));
            if (step.arg6 != Name::None) c.set(6, argument(step.arg6, step.char6));
            c.set({{3, buffer}, {4, pool + step.format}});
        }
        c.call(step.format ? sprintf_entry : strcpy_entry, step.back);
        break;
    }
    c.set(3, 1);
}
const std::array<const Steps*, 8> layouts_first{nullptr, &layout1, &layout2, &layout3, &layout4, &layout5, &layout6, &layout7};
}

bool person_name(Cpu& c, std::uint32_t person, std::uint32_t buffer, std::uint32_t limit, std::uint32_t mode) {
    constexpr std::uint32_t entry = 0x100caeb0;
    c.set(0, c.lr());  // mfspr r0,LR
    guest::Frame frame(c, entry, 0xb0, {.first_gpr = 23});
    const auto pool = c.toc(pool_slot);
    const auto success_flag = c.toc(-0x7bac);
    c.set(23, person);
    compare(c, std::int32_t(person));
    c.set(29, c.toc(-0x7a54));  // the address of the lookup object's word
    c.set(25, success_flag);
    c.set(28, buffer);
    c.set(26, c.toc(-0x7ba8));
    c.set(24, mode);
    c.set(27, pool);
    const auto fail = [&](std::uint32_t at, std::int32_t first_name, std::int32_t second_name, std::uint32_t line) {
        return assertion_failure(c, frame, pool, at, first_name, second_name, line);
    };
    bool valid = person != 0;
    if (valid) {
        valid = compare_logical(c, buffer) != 0;
        if (valid) {
            c.set(0, std::uint32_t(extsh(limit)));
            valid = compare(c, extsh(limit), 0x69) >= 0;
        }
    }
    if (!valid) return fail(0x100caf08, 0x68, 0x40, 0x3b8c);

    // The common name: the lookup object's answer for this record, when it has one.
    const auto lookup_word = c.toc(-0x7a54);
    const auto lookup = c.u32(lookup_word);
    c.set(0, lookup);
    const auto first_name = c.u32(person + 0x28);
    c.set(30, first_name);
    compare_logical(c, lookup);
    const auto surname = c.u32(person + 0x2c);
    c.set(31, surname);
    std::uint32_t common = 0;
    bool found = false;
    if (lookup != 0) {
        c.set({{3, c.u32(lookup_word)}, {4, person}, {5, 0}});
        c.call(0x1014bf80, entry + 0xc0);
        const auto has_name = extsb(c.r(3));
        c.set(0, std::uint32_t(has_name));
        if (compare(c, has_name) != 0) {
            c.set({{3, c.u32(lookup_word)}, {4, person}, {5, 0}});
            c.call(0x1014bdc0, entry + 0xd8);
            common = c.r(3);
            found = true;
        }
    }
    if (!found) common = c.u32(person + 0x30);
    c.set(29, common);

    const auto layout = std::uint32_t(mode & 0xff);
    c.set(0, layout);
    c.set(3, 0);
    compare_logical(c, layout, 7);
    c.put8(buffer, 0);
    if (layout > 7) return fail(0x100cb534, 0x48, 0x38, 0x3c07);
    const auto table = c.toc(-0x567c);
    c.set(0, layout * 4);
    const auto target = c.u32(table + layout * 4);
    c.set(3, target);
    c.s.wr<4>(guest::reg::ctr, target);
    if (layout == 0) return fail(0x100cb534, 0x48, 0x38, 0x3c07);  // the table sends 0 to the assertion

    apply_layout(c, *layouts_first[layout], common, first_name, surname, buffer, pool);
    return true;
}

// 0x100cb750: the same layouts over three strings passed in (first name, surname, common name), for callers
// that have no record. Differences from 0x100caeb0's: layout 6 spaces its two initials ("%c %c"); layout 1
// tests the common name once (0x100caeb0 tests it again after the surname); layout 7 tests it twice, after
// the pair and again after the surname (0x100caeb0 once).
bool name_from_parts(Cpu& c, std::uint32_t first_name, std::uint32_t surname, std::uint32_t common,
                     std::uint32_t buffer, std::uint32_t limit, std::uint32_t mode) {
    constexpr std::uint32_t entry = 0x100cb750;
    c.set(0, c.lr());  // mfspr r0,LR
    guest::Frame frame(c, entry, 0xb0, {.first_gpr = 25});
    const auto pool = c.toc(pool_slot);
    const auto success_flag = c.toc(-0x7bac);
    c.set(29, buffer);
    compare(c, std::int32_t(buffer));
    c.set(26, success_flag);
    c.set(25, c.toc(-0x7ba8));
    c.set(30, first_name);
    c.set(27, pool);
    c.set(31, surname);
    c.set(28, common);
    const auto fail = [&](std::uint32_t at, std::int32_t name_at, std::int32_t extension_at, std::uint32_t line) {
        return assertion_failure(c, frame, pool, at, name_at, extension_at, line);
    };
    bool valid = buffer != 0;
    if (valid) {
        c.set(0, std::uint32_t(extsh(limit)));
        valid = compare(c, extsh(limit), 0x69) >= 0;
    }
    if (!valid) return fail(0x100cb7a0, 0x68, 0x40, 0x3c4c);
    const auto layout = std::uint32_t(mode & 0xff);
    c.set(0, layout);
    c.set(3, 0);
    compare_logical(c, layout, 7);
    c.put8(buffer, 0);
    if (layout > 7) return fail(0x100cbd84, 0x48, 0x38, 0x3cb9);
    const auto table = c.toc(-0x5680);
    c.set(0, layout * 4);
    const auto target = c.u32(table + layout * 4);
    c.set(3, target);
    c.s.wr<4>(guest::reg::ctr, target);
    if (layout == 0) return fail(0x100cbd84, 0x48, 0x38, 0x3cb9);
    apply_layout(c, *layouts_second[layout], common, first_name, surname, buffer, pool);
    return true;
}

// 0x100cb590: the name of a person given by indices into three tables of 0x2c-byte text records
// (first names, surnames, common names; -1 or below for none) and, optionally, an index into the
// 0x70-byte records of the lookup object's table (TOC -0x7be0), which supplies the common name
// when the lookup object accepts it. Formats through 0x100cb750.
bool name_from_indices(Cpu& c, std::uint32_t first_index, std::uint32_t surname_index, std::uint32_t common_index,
                       std::uint32_t buffer, std::uint32_t limit, std::uint32_t mode, std::uint32_t lookup_index) {
    constexpr std::uint32_t entry = 0x100cb590;
    c.set(0, c.lr());  // mfspr r0,LR
    guest::Frame frame(c, entry, 0x90, {.first_gpr = 21});
    const auto pool = c.toc(pool_slot);
    c.set(24, buffer);
    compare(c, std::int32_t(buffer));
    const auto lookup_word = c.toc(-0x7a54);
    c.set(30, lookup_word);
    const auto message = c.toc(-0x7ba8);
    c.set(29, message);
    c.set(21, first_index);
    c.set(28, pool);
    c.set(22, surname_index);
    const auto lookup_table = c.toc(-0x7be0);
    c.set(31, lookup_table);
    c.set(23, common_index);
    c.set(25, limit);
    c.set(26, mode);
    c.set(27, lookup_index);
    bool valid = buffer != 0;
    if (valid) {
        c.set(0, std::uint32_t(extsh(limit)));
        valid = compare(c, extsh(limit), 0x69) >= 0;
    }
    if (!valid) {
        return assertion_failure(c, frame, pool, 0x100cb5f0, 0x40, 0x38, 0x3c21, true);
    }
    // The text record of an index: a table's base word plus index x 0x2c, or null for -1 and below.
    const auto record = [&](std::int32_t slot, std::uint32_t index, unsigned kept) {
        if (compare(c, std::int32_t(index), -1) <= 0) { c.set(kept, 0); return; }
        c.set(3, c.toc(slot));
        c.set(0, index * 0x2c);
        c.set(3, c.u32(c.r(3)));
        c.set(kept, c.r(3) + c.r(0));
    };
    record(-0x752c, first_index, 29);
    record(-0x7530, surname_index, 28);
    std::uint32_t common = 0;
    bool from_lookup = false;
    if (compare(c, std::int32_t(lookup_index), -1) > 0) {
        c.set(0, c.u32(lookup_word));
        if (compare_logical(c, c.r(0)) != 0) {
            const auto row = lookup_index * 0x70;
            c.set(0, row);
            c.set(4, c.u32(lookup_table) + row);
            c.set(3, c.u32(lookup_word));
            c.set(5, 0);
            c.call(0x1014bf80, entry + 0x114);
            c.set(0, std::uint32_t(extsb(c.r(3))));
            if (compare(c, extsb(c.r(3))) != 0) {
                c.set(0, row);
                const auto row_record = c.u32(lookup_table) + row;
                guest::enter(c, entry + 0x13c, {row_record, first_index, surname_index, common_index, 0});
                record_matches(c, row_record, first_index, surname_index, common_index, 0);  // the records block's C++
                c.set(0, std::uint32_t(extsb(c.r(3))));
                if (compare(c, extsb(c.r(3))) != 0) {
                    c.set(0, row);
                    c.set(4, c.u32(lookup_table) + row);
                    c.set(3, c.u32(lookup_word));
                    c.set(5, 0);
                    c.call(0x1014bdc0, entry + 0x15c);
                    c.set(5, c.r(3));
                    common = c.r(3);
                    from_lookup = true;
                }
            }
        }
    }
    if (!from_lookup) {
        if (compare(c, std::int32_t(common_index), -1) > 0) {
            c.set(3, c.toc(-0x7534));
            c.set(0, common_index * 0x2c);
            c.set(3, c.u32(c.r(3)));
            common = c.r(3) + c.r(0);
        } else {
            common = 0;
        }
        c.set(5, common);
    }
    c.set({{3, c.r(29)}, {4, c.r(28)}, {6, buffer}, {7, limit}, {8, mode}, {9, 0xffffffff}});
    guest::enter(c, entry + 0x1a0, {});
    return name_from_parts(c, c.r(3), c.r(4), common, buffer, limit, mode);
}

namespace {
// 0x100cbf60's layouts: the record has no common name, so only the first name (B) and the surname (C).
const Steps record_preferred{  // when the record's team matches: the surname, else the first name
    {Name::C, Name::None, 0x100cc028, 0, 0x100cc04c},
    {Name::B, Name::None, 0x100cc050, 0, 0x100cc074}};
const Steps record_layout1{
    {Name::B, Name::C, 0x100cc0b4, 0x100cc0cc, 0x100cc0f8, format_pair, Name::B, Name::C},
    {Name::C, Name::None, 0x100cc0fc, 0, 0x100cc120},
    {Name::B, Name::None, 0x100cc124, 0, 0x100cc148}};
const Steps record_layout2{
    {Name::B, Name::C, 0x100cc150, 0x100cc168, 0x100cc198, format_initial_surname, Name::B, Name::C, true},
    {Name::C, Name::None, 0x100cc19c, 0, 0x100cc1c0},
    {Name::B, Name::None, 0x100cc1c4, 0, 0x100cc1e8}};
const Steps record_layout3{
    {Name::B, Name::C, 0x100cc1f0, 0x100cc208, 0x100cc234, format_surname_comma, Name::C, Name::B},
    {Name::C, Name::None, 0x100cc238, 0, 0x100cc25c},
    {Name::B, Name::None, 0x100cc260, 0, 0x100cc284}};
const Steps record_layout4{
    {Name::B, Name::C, 0x100cc28c, 0x100cc2a4, 0x100cc2d4, format_comma_initial, Name::C, Name::B, false, true},
    {Name::C, Name::None, 0x100cc2d8, 0, 0x100cc2fc},
    {Name::B, Name::None, 0x100cc300, 0, 0x100cc324}};
const Steps record_layout5{
    {Name::C, Name::None, 0x100cc32c, 0, 0x100cc350},
    {Name::B, Name::None, 0x100cc354, 0, 0x100cc378}};
const Steps record_layout6{
    {Name::B, Name::C, 0x100cc380, 0x100cc398, 0x100cc3cc, format_two_initials_spaced, Name::B, Name::C, true, true},
    {Name::C, Name::None, 0x100cc3d0, 0, 0x100cc3fc, format_initial, Name::C, Name::None, true},
    {Name::B, Name::None, 0x100cc400, 0, 0x100cc42c, format_initial, Name::B, Name::None, true}};
const std::array<const Steps*, 7> record_layouts{nullptr, &record_layout1, &record_layout2, &record_layout3,
                                                 &record_layout4, &record_layout5, &record_layout6};
}

// 0x100cbf60: a record's name (a record of the person's club or team: first name at +4, surname
// at +8, a pointer at +0xc) in one of layouts 1-6. When +0xc points to a record whose +0x68 equals the
// word the TOC -0x7a18 points at, the surname (else the first name) is used whatever the layout.
// Layouts 0 and 7 up report an assertion.
bool record_name(Cpu& c, std::uint32_t record, std::uint32_t buffer, std::uint32_t limit, std::uint32_t mode) {
    constexpr std::uint32_t entry = 0x100cbf60;
    c.set(0, c.lr());  // mfspr r0,LR
    guest::Frame frame(c, entry, 0xa0, {.first_gpr = 26});
    const auto success_flag = c.toc(-0x7bac), message = c.toc(-0x7ba8), pool = c.toc(pool_slot);
    compare_logical(c, record);
    c.set({{26, success_flag}, {27, message}, {29, buffer}, {28, pool}});
    const auto fail = [&](std::uint32_t at, std::int32_t name_at, std::int32_t extension_at, std::uint32_t line) {
        return assertion_failure(c, frame, pool, at, name_at, extension_at, line);
    };
    bool valid = record != 0;
    if (valid) {
        valid = compare_logical(c, buffer) != 0;
        if (valid) {
            c.set(0, std::uint32_t(extsh(limit)));
            valid = compare(c, extsh(limit), 0x69) >= 0;
        }
    }
    if (!valid) return fail(0x100cbfb0, 0x68, 0x40, 0x3d08);
    const auto first_name = c.u32(record + 4);
    c.set(30, first_name);
    c.set(0, 0);
    const auto surname = c.u32(record + 8);
    c.set(31, surname);
    c.put8(buffer, 0);
    const auto team = c.u32(record + 0xc);
    c.set(0, team);
    if (compare_logical(c, team) != 0) {
        c.set(4, team);
        c.set(3, c.toc(-0x7a18));
        const auto team_key = c.u32(team + 0x68);
        c.set(4, team_key);
        const auto wanted = c.u32(c.r(3));
        c.set(0, wanted);
        if (compare(c, std::int32_t(team_key), std::int32_t(wanted)) == 0) {
            apply_layout(c, record_preferred, 0, first_name, surname, buffer, pool);
            return true;
        }
    }
    // The layout number: the original compares it with 4, 6, 2 and 1 in a tree.
    const auto layout = std::uint32_t(mode & 0xff);
    c.set(0, layout);
    const auto against = [&](std::int32_t value) { return compare(c, std::int32_t(layout), value); };
    std::uint32_t chosen;  // the layout the tree reaches (0: the assertion)
    if (const auto four = against(4); four == 0) {
        chosen = 4;
    } else if (four > 0) {
        const auto six = against(6);
        chosen = six == 0 ? 6 : six > 0 ? 0 : 5;
    } else if (const auto two = against(2); two == 0) {
        chosen = 2;
    } else if (two > 0) {
        chosen = 3;
    } else {
        chosen = against(1) >= 0 ? 1 : 0;
    }
    if (chosen == 0) return fail(0x100cc448, 0x48, 0x38, 0x3d6a);
    apply_layout(c, *record_layouts[chosen], 0, first_name, surname, buffer, pool);
    return true;
}
}
