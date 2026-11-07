// DATABASE.CPP's text block, part 7: the comma-separated summary of a player's strong positions
// (0x100cc4a0; database_text.hpp).
#include "database_text.hpp"
#include "database_text_detail.hpp"

namespace cm::aot::database {
using namespace text;

namespace {
constexpr std::uint32_t format_entry = 0x10727890, strcat_entry = 0x1077dc60;
constexpr std::uint32_t separator = 0x1932, empty_text = 0xd0, pair_format = 0x190a;
}

// 0x100cc4a0: describes the ratings of a player record (bytes +0xf-+0x1a: goalkeeper, sweeper, defender,
// defensive midfielder, midfielder, attacking midfielder, attacker and the right, left and central
// roles; inferred from database_detail.hpp's names) in `buffer`. Two phrases are built in 0x1070-byte frame
// buffers, one of the positions whose rating reaches a threshold that starts at 15 and falls to 10 while
// the phrase stays empty, one of the sides (rating 15 or more); mode (r6's low byte) 1-4 picks the form of
// each text. `limit` (r5, 16-bit) must be at least 30. Quirk: when the phrase stays empty the
// original also raises the record's +0x13 byte to 15 if +0x14 is 15 or more (a write into the record).
bool player_summary_text(Cpu& c, std::uint32_t record, std::uint32_t buffer, std::uint32_t limit, std::uint32_t mode) {
    constexpr std::uint32_t entry = 0x100cc4a0;
    c.set(0, c.lr());  // mfspr r0,LR
    guest::Frame frame(c, entry, 0x1070, {.first_gpr = 25});
    const auto pool = c.toc(pool_slot), message = c.toc(-0x7ba8), success_flag = c.toc(-0x7bac), scratch = c.toc(-0x7bb8);
    compare(c, std::int32_t(buffer));
    c.set({{29, scratch}, {25, success_flag}, {27, record}, {28, message}, {31, mode}, {30, pool}, {26, buffer}});
    const auto fail = [&](std::uint32_t at, std::int32_t name_at, std::int32_t extension_at, std::uint32_t line) {
        return assertion_failure(c, frame, pool, at, name_at, extension_at, line);
    };
    if (buffer == 0) return fail(0x100cc4e4, 0x90, 0x48, 0x3d95);
    c.set(0, 0);
    compare_logical(c, record);
    c.put8(buffer, 0);
    if (record == 0) return fail(0x100cc54c, 0x70, 0x40, 0x3da0);
    c.set(0, std::uint32_t(extsh(limit)));
    if (compare(c, extsh(limit), 0x1e) < 0) return fail(0x100cc5b0, 0x50, 0x38, 0x3da8);

    const auto phrase = frame.at(0x880), second_phrase = frame.at(0xb0);
    const auto threshold = [&] { return c.r(28) & 0xff; };
    const auto mode_is = [&](std::uint32_t what) {  // rlwinm r0,r31,0,24,31; cmplwi r0,what
        c.set(0, mode & 0xff);
        return compare_logical(c, c.r(0), what) == 0;
    };
    const auto length_nonzero = [&](std::uint32_t text, std::uint32_t back) {
        c.set(3, text);
        c.call(strlen_entry, back);
        return compare_logical(c, c.r(3), 0) != 0;
    };
    const auto add_separator = [&](std::uint32_t text, std::uint32_t back) {
        c.set({{3, text}, {4, pool + separator}});
        c.call(strcat_entry, back);
    };
    const auto format = [&](std::uint32_t offset, std::uint32_t back, bool with_empty) {
        c.set({{3, scratch}, {4, pool + offset}});
        if (with_empty) c.set(5, pool + empty_text);
        c.call_in_place(format_entry, back);
    };
    const auto append = [&](std::uint32_t text, std::uint32_t back) {
        c.set({{4, scratch}, {3, text}});
        c.call(strcat_entry, back);
    };
    // The rating at `at` against the threshold in r28 (signed, as cmpw), or against 15.
    const auto reaches = [&](std::uint32_t at) {
        const auto rating = extsb(c.u8(record + at));
        c.set(3, std::uint32_t(rating));
        c.set(0, threshold());
        return compare(c, rating, std::int32_t(threshold())) >= 0;
    };
    const auto reaches_fifteen = [&](std::uint32_t at) {
        const auto rating = extsb(c.u8(record + at));
        c.set(0, std::uint32_t(rating));
        return compare(c, rating, 0xf) >= 0;
    };
    // One text of the first phrase (blocks of 0xb8 bytes in the original from `s`): modes 1, 2 and 3 add
    // a separator when the phrase is not empty, then the formatted text; other modes only start an empty phrase.
    const auto text = [&](std::uint32_t s, std::uint32_t first, std::uint32_t second, std::uint32_t other,
                          bool second_with_empty = false) {
        const std::uint32_t shift = second_with_empty ? 4 : 0;
        if (mode_is(1)) {
            if (length_nonzero(phrase, s + 0x14)) add_separator(phrase, s + 0x28);
            format(first, s + 0x38, true);
            append(phrase, s + 0x44);
            return;
        }
        if (mode_is(2) || mode_is(3)) {
            if (length_nonzero(phrase, s + 0x68)) add_separator(phrase, s + 0x7c);
            format(second, s + 0x88 + shift, second_with_empty);  // only the 0x1a6c text loads r5 (one more instruction)
            append(phrase, s + 0x94 + shift);
            return;
        }
        if (!length_nonzero(phrase, s + 0xa0 + shift)) {
            c.set({{3, phrase}, {4, pool + other}});
            c.call(strcat_entry, s + 0xb4 + shift);
        }
    };
    // The second phrase's texts (blocks of 0x98 bytes): modes 1, 2 and 4.
    const auto side_text = [&](std::uint32_t s, std::uint32_t first, std::uint32_t second, std::uint32_t other) {
        if (mode_is(1)) {
            format(first, s + 0x1c, true);
            append(second_phrase, s + 0x28);
            return;
        }
        if (mode_is(2)) {
            if (length_nonzero(second_phrase, s + 0x40)) add_separator(second_phrase, s + 0x54);
            format(second, s + 0x60, false);
            append(second_phrase, s + 0x6c);
            return;
        }
        if (mode_is(4) && !length_nonzero(second_phrase, s + 0x84)) {
            c.set({{3, second_phrase}, {4, pool + other}});
            c.call(strcat_entry, s + 0x98);
        }
    };
    const auto clear = [&](std::uint32_t text_at, std::uint32_t back) {
        c.set({{3, text_at}, {4, pool + empty_text}});
        c.call(strcpy_entry, back);
    };

    // First phrase: positions whose rating reaches the threshold; true when it ends the routine.
    const auto first_phrase = [&]() {
        clear(phrase, 0x100cc604);
        if (reaches(0xf)) text(0x100cc618, 0x1934, 0x1952, 0x195d);
        if (reaches(0x10)) text(0x100cc6e0, 0x195f, 0x197a, 0x1982);
        if (reaches(0x11)) text(0x100cc7a8, 0x1984, 0x199f, 0x19a8);
        if (reaches(0x12)) {
            text(0x100cc870, 0x19aa, 0x19d2, 0x19e7);
        } else if (reaches(0x14) && reaches(0x13)) {
            text(0x100cc950, 0x19e9, 0x1a11, 0x1a26);
        } else if (reaches(0x13)) {
            text(0x100cca1c, 0x1a28, 0x1a45, 0x1a50);
        }
        if (reaches(0x15)) {
            const bool sides = reaches(0x14) || reaches(0x17) || reaches(0x18) || reaches(0x1a);
            if (sides) text(0x100ccb34, 0x1a52, 0x1a6c, 0x1a95, true);
            else text(0x100ccbf0, 0x1a97, 0x1ab1, 0x1ab9);
        }
        if (reaches(0xf)) {
            c.set({{3, buffer}, {4, phrase}});
            c.call(strcpy_entry, 0x100cccc4);
            return true;
        }
        return false;
    };
    c.set(28, 0xf);
    for (;;) {
        if (first_phrase()) return guest::result(c, 1) != 0;
        c.set(28, c.r(28) - 1);
        if (length_nonzero(phrase, 0x100cccd8)) break;
        c.set(0, threshold());
        if (compare_logical(c, c.r(0), 0xa) < 0) break;
    }
    c.set(0, std::uint32_t(extsb(c.u8(phrase))));
    if (compare(c, extsb(c.u8(phrase))) == 0) {
        // The phrase is still empty: fall back to the goalkeeper-style text, after capping +0x13.
        c.set(0, std::uint32_t(extsb(c.u8(record + 0x14))));
        if (compare(c, extsb(c.u8(record + 0x14)), 0xf) >= 0) {
            c.set(0, 0xf);
            c.put8(record + 0x13, 0xf);
        }
        if (mode_is(1)) {
            format(0x19e9, 0x100ccd2c, true);
            append(phrase, 0x100ccd38);
        } else if (mode_is(2) || mode_is(3)) {
            if (length_nonzero(phrase, 0x100ccd5c)) add_separator(phrase, 0x100ccd70);
            format(0x1a11, 0x100ccd7c, false);
            append(phrase, 0x100ccd88);
        } else if (!length_nonzero(phrase, 0x100ccd94)) {
            format(0x19e9, 0x100ccdac, true);
            append(phrase, 0x100ccdb8);
        }
    }
    // Second phrase: the side roles at 15 or more.
    c.set(28, 0xf);
    for (;;) {
        clear(second_phrase, 0x100ccdc8);
        if (reaches_fifteen(0x17)) side_text(0x100ccdd8, 0x1abb, 0x1ad3, 0x195d);
        if (reaches_fifteen(0x18)) side_text(0x100cce80, 0x1ad9, 0x1af0, 0x1982);
        if (reaches_fifteen(0x19)) side_text(0x100ccf28, 0x1af5, 0x1b0e, 0x19a8);
        c.set(28, c.r(28) - 1);
        if (length_nonzero(second_phrase, 0x100ccfcc)) break;
        c.set(0, threshold());
        if (compare_logical(c, c.r(0), 0xa) < 0) break;
    }
    if (mode_is(2) && length_nonzero(second_phrase, 0x100ccff4)) {
        c.set({{3, buffer}, {4, pool + 0x1b15}, {5, phrase}, {6, second_phrase}});
        c.call(sprintf_entry, 0x100cd010);
    } else {
        if (length_nonzero(second_phrase, 0x100cd01c)) {
            c.set({{3, buffer}, {4, pool + pair_format}, {5, phrase}, {6, second_phrase}});
            c.call(sprintf_entry, 0x100cd038);
        } else {
            c.set({{3, buffer}, {4, phrase}});
            c.call(strcpy_entry, 0x100cd048);
        }
    }
    return guest::result(c, 1) != 0;
}
}
