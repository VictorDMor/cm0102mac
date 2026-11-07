// cm-database-checks fixtures for DATABASE.CPP's text block (database_checks.hpp): the name
// layouts and sort comparators, then the text tables. Callees that need state the fixtures
// cannot build are stubbed with record_call (the lookup object's two methods, the assertion
// report); the library routines (strlen, strcpy, sprintf, the text comparator) run for real.
#include "database_checks.hpp"
#include "guest_abi.hpp"
#include "localization_format_adapter.hpp"
#include <array>
#include <memory>
#include <optional>
#include <string>

namespace cm::aot::database_checks {
namespace {
using namespace cm::aot::guest;
constexpr std::uint32_t pool_offset = 0xd;  // the assertion format in the string pool (TOC -0x5618)

// Where fixtures keep strings and records.
constexpr std::uint32_t strings = data + 0x4000, record_a = data + 0x1000, record_b = data + 0x2000,
                        pointer_a = data + 0x3000, pointer_b = data + 0x3010, lookup_word = data + 0x3020,
                        lookup_object = data + 0x3030;

void put_string(Cpu& c, std::uint32_t at, const std::string& text) {
    for (std::size_t i = 0; i < text.size(); ++i) c.put8(at + std::uint32_t(i), std::uint8_t(text[i]));
    c.put8(at + std::uint32_t(text.size()), 0);
}

// Entry state of one fixture, plus the stub every case of the text block shares: the
// assertion report (recorded, never opened) and the lookup object's methods.
struct Stubs {
    std::uint32_t has_name = 0, common_name = 0;  // the lookup object's answers
    // 0x100d5de0's callees: 0x10459b50's object, 0x10082f20's primary contract, 0x10303560's f1.
    std::uint32_t object = 0, primary = 0;
    double scaled = 0;
};
bool stub_calls(State& s, const Stubs& t) {
    Cpu c(s);
    const auto pc = s.pc;
    // The generated pages call the title formatter's C++ owner directly; declining it on every route
    // (each run's first dispatch gets here) sends the call through the dispatcher, where the stand-in answers.
    localization_format_diagnostics(s).original_body = 0x10727890;
    if ((pc == 0x1077b720 && c.r(4) != c.toc(-0x5618) + pool_offset) || pc == 0x10727890) {
        // Stand-in for sprintf and for the localization formatter (0x10727890), over the argument registers r5
        // on, recorded through record_call so every argument is compared. It is exact for the C formats the
        // routines pass to sprintf ("%s %s", "%c.%s", "%s, %c", "%c%c", "%c %c", "%c": literals, %s and %c only).
        // For 0x10727890 it is NOT the real formatter: the real one (localization_format.cpp) strips the
        // "<%s - COMMENT ...>" hint, consuming its %s argument (always the empty pool text), and takes one
        // argument word per "{}" or "[]" marker (the club-name formats "{}<%s - nation name>{} ..." of
        // 0x100d0940). Here the hint text is kept, each "%s" takes the next register and a "{}" takes none, so the
        // formatted text in memory differs from the game's; both routes run this same stand-in, and the
        // arguments the routines pass are still compared. A non-text %s argument prints as '#' and its number.
        record_call(c);
        std::string text;
        unsigned next = 5;
        for (auto at = c.r(4); c.u8(at); ++at) {
            if (c.u8(at) != '%') { text += char(c.u8(at)); continue; }
            const char kind = char(c.u8(++at));
            const auto argument = c.r(next++);
            if (kind == 'c') text += char(argument);
            else if ((argument >= data && argument < data + data_size) || (argument >= high && argument < high + high_size) ||
                     (argument >= 0x10000000 && argument < 0x11000000))
                for (auto from = argument; c.u8(from) && text.size() < 0x400; ++from) text += char(c.u8(from));
            else text += '#' + std::to_string(argument);  // not a text: a grammar value
        }
        put_string(c, c.r(3), text);
        c.set(3, std::uint32_t(text.size()));
        s.pc = c.lr();
        return true;
    }
    if (pc == 0x10459b50 || pc == 0x10082f20 || pc == 0x10303560) {
        record_call(c);
        c.set(3, pc == 0x10459b50 ? t.object : pc == 0x10082f20 ? t.primary : 0);
        if (pc == 0x10303560) c.set_f(1, t.scaled);
        s.pc = c.lr();
        return true;
    }
    const auto assertion = pc == 0x10739890 || pc == 0x10768810 || pc == 0x10769b60 ||
                           (pc == 0x1077b720 && c.r(4) == c.toc(-0x5618) + pool_offset);
    if (!assertion && pc != 0x1014bf80 && pc != 0x1014bdc0) return false;
    record_call(c);
    if (pc == 0x1014bf80) c.set(3, t.has_name);
    else if (pc == 0x1014bdc0) c.set(3, t.common_name);
    else c.set(3, 0x2000 + (pc & 0xff));
    s.pc = c.lr();
    return true;
}

using Setup = std::function<void(Cpu&)>;
void add(std::vector<Fixture>& out, std::uint32_t entry, std::string description, Setup setup, Stubs stubs = {},
         bool summary_overflow = false) {
    Fixture f;
    f.entry = entry;
    f.description = std::move(description);
    f.setup = std::move(setup);
    f.stub = [stubs](State& s) { return stub_calls(s, stubs); };
    f.summary_overflow = summary_overflow;
    out.push_back(std::move(f));
}

// The strings a name field can hold: null, empty, short, with a high byte (a negative char).
struct Text {
    const char* label;
    std::optional<std::string> value;
};
const std::array<Text, 5> texts{{{"null", std::nullopt}, {"empty", std::string()}, {"x", std::string("x")},
                                 {"Anna", std::string("Anna")}, {"\xc9ric", std::string("\xc9ric")}}};

std::uint32_t place(Cpu& c, std::uint32_t slot, const Text& t) {
    if (!t.value) return 0;
    const auto at = strings + slot * 0x40;
    put_string(c, at, *t.value);
    return at;
}

// 0x100caeb0: a person's name in every layout, with every combination of the three strings.
void names(std::vector<Fixture>& out) {
    constexpr std::uint32_t entry = 0x100caeb0, person = record_a, buffer = data + 0x5000;
    const auto setup = [](std::uint32_t person_pointer, std::uint32_t buffer_pointer, std::uint32_t limit, std::uint32_t mode,
                          const Text* a, const Text* b, const Text* cc, bool lookup, std::uint32_t common_slot) {
        return [=](Cpu& c) {
            c.put32(c.r(2) - 0x7a54, lookup_word);
            c.put32(lookup_word, lookup ? lookup_object : 0);
            if (a) c.put32(person + 0x30, place(c, 1, *a));
            if (b) c.put32(person + 0x28, place(c, 2, *b));
            if (cc) c.put32(person + 0x2c, place(c, 3, *cc));
            (void)common_slot;
            c.set({{3, person_pointer}, {4, buffer_pointer}, {5, limit}, {6, mode}});
        };
    };
    for (const auto mode : {0u, 1u, 2u, 3u, 4u, 5u, 6u, 7u, 8u, 0x101u, 0x1ffu, 0x80000003u, 0xffffff00u})
        for (const auto& a : texts)
            for (const auto& b : texts)
                for (const auto& cc : texts) {
                    if (mode > 7 && (&b != &texts[3] || &cc != &texts[2])) continue;  // the assertion ignores the strings
                    for (const auto lookup : {0, 1, 2}) {
                        // lookup 1: the object has no name for the record; 2: it answers the first text.
                        if (lookup && (&b != &texts[3] || &cc != &texts[3])) continue;
                        Stubs stubs{lookup == 2 ? 0x100u + 1 : 0u, 0};
                        std::string description = "layout " + std::to_string(mode) + " A=" + a.label + " B=" + b.label +
                                                  " C=" + cc.label + " lookup=" + std::to_string(lookup);
                        if (lookup == 2) {
                            for (const auto has : {0x1u, 0x80u, 0x100u, 0xffffff7fu}) {
                                stubs.has_name = has;
                                stubs.common_name = strings + 0x1000;
                                add(out, entry, description + " has=" + hex_address(has),
                                    [=](Cpu& c) { setup(person, buffer, 0x69, mode, &a, &b, &cc, true, 0)(c);
                                                  put_string(c, strings + 0x1000, "Lookup"); },
                                    stubs);
                            }
                            stubs.common_name = 0;
                            stubs.has_name = 1;
                            add(out, entry, description + " has a null common name",
                                setup(person, buffer, 0x69, mode, &a, &b, &cc, true, 0), stubs);
                        } else {
                            add(out, entry, description, setup(person, buffer, 0x69, mode, &a, &b, &cc, lookup == 1, 0), stubs,
                                (mode & 1) != 0);
                        }
                    }
                }
    // Arguments: a null record or buffer, a limit below 0x69 or whose 16 low bits are.
    for (const auto mode : {1u, 5u})
        for (const auto& [record, buf, limit] : std::array<std::array<std::uint32_t, 3>, 8>{{
                 {0, buffer, 0x69}, {person, 0, 0x69}, {person, buffer, 0x68}, {person, buffer, 0x8000},
                 {person, buffer, 0x10069}, {person, buffer, 0x7fff}, {high + 0x100, buffer, 0x69},
                 {person, high + 0x200, 0x69}}})
            for (const bool overflow : {false, true})
                add(out, entry, "arguments " + hex_address(record) + " " + hex_address(buf) + " " + hex_address(limit),
                    [=](Cpu& c) {
                        for (const auto base : {high + 0x100, person}) { c.put32(base + 0x28, 0); c.put32(base + 0x2c, 0); c.put32(base + 0x30, 0); }
                        setup(record, buf, limit, mode, &texts[3], &texts[3], &texts[4], false, 0)(c);
                    }, {}, overflow);
}

// The comparators over two pointers to records.
void comparators(std::vector<Fixture>& out) {
    const auto pointers = [](std::uint32_t a, std::uint32_t b) {
        return [=](Cpu& c) {
            c.put32(pointer_a, a);
            c.put32(pointer_b, b);
            c.put32(c.r(2) - 0x7a54, lookup_word);
            c.put32(lookup_word, 0);
            c.set({{3, pointer_a}, {4, pointer_b}});
        };
    };
    const auto name_record = [](Cpu& c, std::uint32_t record, const Text& a, const Text& b, const Text& cc) {
        c.put32(record + 0x30, place(c, record == record_a ? 1 : 4, a));
        c.put32(record + 0x28, place(c, record == record_a ? 2 : 5, b));
        c.put32(record + 0x2c, place(c, record == record_a ? 3 : 6, cc));
    };
    const std::array<std::array<const Text*, 3>, 5> name_sets{{
        {&texts[3], &texts[2], &texts[4]}, {&texts[0], &texts[3], &texts[4]}, {&texts[0], &texts[4], &texts[0]},
        {&texts[1], &texts[2], &texts[2]}, {&texts[3], &texts[3], &texts[3]}}};
    for (const auto entry : {0x100cd2a0u, 0x100cd6b0u})
        for (const auto& first : name_sets)
            for (const auto& second : name_sets)
                for (const auto null_mask : {0, 1, 2, 3})
                    for (const bool overflow : {false, true})
                        add(out, entry, "names null_mask=" + std::to_string(null_mask) + " set " + first[0]->label + "," + first[1]->label,
                            [=](Cpu& c) {
                                pointers(null_mask & 1 ? 0 : record_a, null_mask & 2 ? 0 : record_b)(c);
                                name_record(c, record_a, *first[0], *first[1], *first[2]);
                                name_record(c, record_b, *second[0], *second[1], *second[2]);
                            }, {}, overflow);
    // Inline text at +0x221 and +0xe1.
    const std::array<std::string, 4> inline_texts{"", "Anna", "Anne", "anna"};
    for (const auto [entry, offset] : {std::pair{0x100cd410u, 0x221u}, std::pair{0x100cd4a0u, 0xe1u}})
        for (const auto& first : inline_texts)
            for (const auto& second : inline_texts)
                for (const auto kind : {0, 1, 2, 3, 4, 5}) {
                    // kind: both records; a null first; a null second; both null; a record whose
                    // text address wraps to 0 (first); the same (second).
                    add(out, entry, "inline text '" + first + "' '" + second + "' kind " + std::to_string(kind),
                        [=](Cpu& c) {
                            std::uint32_t a = record_a, b = record_b;
                            if (kind == 1 || kind == 3) a = 0;
                            if (kind == 2 || kind == 3) b = 0;
                            if (kind == 4) a = 0u - offset;
                            if (kind == 5) b = 0u - offset;
                            pointers(a, b)(c);
                            put_string(c, record_a + offset, first);
                            put_string(c, record_b + offset, second);
                        }, {}, kind == 3);
                }
}

// 0x100cd530: the date fields.
void dates(std::vector<Fixture>& out) {
    const std::array<std::uint16_t, 5> years{0, 1, 0x7fff, 0x8000, 0xffff};
    const std::array<std::uint16_t, 4> words{0, 1, 5, 0xffff};
    for (const auto year_a : years)
        for (const auto year_b : years)
            for (const auto word_a : words)
                for (const auto word_b : words)
                    for (const auto kind : {0, 1, 2, 3}) {
                        if ((year_a != year_b && word_a != word_b) && kind != 0) continue;
                        add(out, 0x100cd530, "dates", [=](Cpu& c) {
                            c.put32(pointer_a, kind & 1 ? 0 : record_a);
                            c.put32(pointer_b, kind & 2 ? 0 : record_b);
                            c.put32(c.r(2) - 0x7a54, lookup_word);
                            c.put32(lookup_word, 0);
                            c.put16(record_a + 0x34, year_a);
                            c.put16(record_b + 0x34, year_b);
                            c.put16(record_a + 0x36, word_a);
                            c.put16(record_b + 0x36, word_b);
                            c.put8(record_a + 0x38, year_a & 0xff);
                            c.put8(record_b + 0x38, word_b & 0xff);
                            put_string(c, strings + 0x40, "Anna");
                            put_string(c, strings + 0x80, year_a < year_b ? "Anne" : "Anna");
                            c.put32(record_a + 0x30, strings + 0x40);
                            c.put32(record_b + 0x30, strings + 0x80);
                            c.set({{3, pointer_a}, {4, pointer_b}});
                        }, {}, kind == 3);
                    }
}

// 0x100d5ce0 and 0x100c4b00.
void small(std::vector<Fixture>& out) {
    for (const auto& [a, b] : std::array<std::array<std::uint8_t, 2>, 5>{{{1, 2}, {2, 1}, {7, 7}, {0x80, 0x7f}, {0, 0}}})
        for (const auto kind : {0, 1, 2, 3}) {
            add(out, 0x100d5ce0, "ids", [=](Cpu& c) {
                c.put8(record_a, a);
                c.put8(record_b, b);
                c.set({{3, kind & 1 ? 0 : record_a}, {4, kind & 2 ? 0 : record_b}});
            }, {}, kind == 1);
        }
    const std::array<std::optional<std::string>, 13> files{std::nullopt, std::string(""), std::string("a"), std::string("ab"),
                                                           std::string("a.b"), std::string("."), std::string("ab."),
                                                           std::string(".ab"), std::string(".."), std::string("abcdef"),
                                                           std::string("\xc9x"), std::string("\x80"), std::string("a\xe9.")};
    for (const auto& name : files)
        for (const bool overflow : {false, true})
            add(out, 0x100c4b00, name ? "file '" + *name + "'" : "null file", [=](Cpu& c) {
                if (name) put_string(c, strings, *name);
                c.set(3, name ? strings : 0);
            }, {}, overflow);
}
}

// 0x100c6af0 and 0x100c6bf0: every role and position index with every flag combination.
void titles(std::vector<Fixture>& out) {
    constexpr std::uint32_t out_byte = data + 0x5100;
    const auto kind_byte = [](Cpu& c, std::uint8_t kind) { c.put8(c.r(2) - 0x35ac, kind); };
    for (int role = -128; role < 128; ++role)
        for (const auto kind : {std::uint8_t(4), std::uint8_t(0x84), std::uint8_t(0)})
            for (const auto with_out : {false, true}) {
                if (kind != 4 && !with_out) continue;
                add(out, 0x100c6af0, "role " + std::to_string(role) + " kind " + std::to_string(kind), [=](Cpu& c) {
                    kind_byte(c, kind);
                    c.put8(out_byte, 0x77);
                    c.set({{3, (role & 1 ? 0x12345600u : 0u) | std::uint8_t(role)}, {4, with_out ? out_byte : 0}});
                }, {}, role % 5 == 0);
            }
    for (int index = -128; index < 128; ++index)
        for (const auto long_form : {0x0u, 0x1u, 0x100u, 0x80u})
            for (const auto plural : {0x0u, 0x1u, 0x100u, 0xffu})
                for (const auto kind : {std::uint8_t(4), std::uint8_t(3)})
                    for (const auto with_out : {false, true}) {
                        // The index space is only 0-17 plus out-of-range samples; the rest repeat the default.
                        if ((index > 20 && index < 126 && index != 64) || (index < -3 && index > -126 && index != -64)) continue;
                        if ((long_form == 0x100 || long_form == 0x80 || plural == 0x100 || plural == 0xff) && (index < 0 || index > 20)) continue;
                        if (kind == 3 && !with_out) continue;
                        add(out, 0x100c6bf0, "position " + std::to_string(index) + " r4=" + std::to_string(long_form) +
                                                 " r5=" + std::to_string(plural) + " kind " + std::to_string(kind), [=](Cpu& c) {
                            kind_byte(c, kind);
                            c.put8(out_byte, 0x77);
                            c.set({{3, (index & 1 ? 0x12345600u : 0u) | std::uint8_t(index)}, {4, long_form},
                                   {5, plural}, {6, with_out ? out_byte : 0}});
                        }, {}, (index & 7) == 0);
                    }
}

// 0x100cb750: the layouts over three strings given directly.
void parts(std::vector<Fixture>& out) {
    constexpr std::uint32_t buffer = data + 0x5000;
    for (const auto mode : {0u, 1u, 2u, 3u, 4u, 5u, 6u, 7u, 8u, 0x1ffu})
        for (const auto& a : texts)
            for (const auto& b : texts)
                for (const auto& cc : texts) {
                    if (mode > 7 && (&a != &texts[3] || &b != &texts[3])) continue;
                    add(out, 0x100cb750, "parts layout " + std::to_string(mode) + " A=" + a.label + " B=" + b.label + " C=" + cc.label,
                        [=](Cpu& c) {
                            c.set({{3, place(c, 2, b)}, {4, place(c, 3, cc)}, {5, place(c, 1, a)}, {6, buffer}, {7, 0x69}, {8, mode}});
                        }, {}, (mode & 1) != 0);
                }
    for (const auto& [buf, limit] : std::array<std::array<std::uint32_t, 2>, 4>{{{0, 0x69}, {buffer, 0x68}, {buffer, 0x8000}, {buffer, 0x10069}}})
        add(out, 0x100cb750, "parts arguments", [=](Cpu& c) {
            c.set({{3, place(c, 2, texts[3])}, {4, place(c, 3, texts[4])}, {5, 0}, {6, buf}, {7, limit}, {8, 1}});
        });
}

// 0x100cb590: names by table indices; the real 0x100cbde0 matches the lookup table's row.
void indices(std::vector<Fixture>& out) {
    constexpr std::uint32_t entry = 0x100cb590, buffer = data + 0x5000, words = data + 0x6000, table = data + 0x7000,
                            objects = data + 0x7800;
    struct Row { const char* label; std::uint32_t flags14, flags1c; bool a, b, c; std::int32_t da, db, dc; };
    const std::array<Row, 8> rows{{{"match", 1, 0, true, true, true, 0, 0, 0}, {"match 1c", 0, 1, true, true, true, 0, 0, 0},
                                   {"no flags", 0, 0, true, true, true, 0, 0, 0}, {"first differs", 1, 0, true, true, true, 1, 0, 0},
                                   {"surname differs", 1, 0, true, true, true, 0, 1, 0}, {"common differs", 1, 0, true, true, true, 0, 0, 1},
                                   {"no objects", 1, 0, false, false, false, 0, 0, 0}, {"only first", 1, 0, true, false, false, 0, 0, 0}}};
    const std::array<std::array<std::int32_t, 3>, 3> sets{{{1, 2, 0}, {-1, 1, -1}, {0, -1, 2}}};
    for (const auto& row : rows)
        for (const auto& set : sets)
            for (const auto lookup : {std::int32_t(-1), std::int32_t(1)})
                for (const auto has : {0u, 1u, 0x100u})
                    for (const auto mode : {1u, 6u}) {
                        if (lookup < 0 && has != 0) continue;
                        Stubs stubs{has, strings + 0x1000};
                        add(out, entry, std::string("indices row ") + row.label + " set " + std::to_string(set[0]) + "," + std::to_string(set[1]) + "," + std::to_string(set[2]) +
                                             " lookup " + std::to_string(lookup) + " has " + std::to_string(has),
                            [=](Cpu& c) {
                                c.put32(c.r(2) - 0x752c, words);
                                c.put32(c.r(2) - 0x7530, words + 4);
                                c.put32(c.r(2) - 0x7534, words + 8);
                                c.put32(c.r(2) - 0x7be0, words + 12);
                                c.put32(c.r(2) - 0x7a54, lookup_word);
                                c.put32(lookup_word, lookup_object);
                                c.put32(words, data + 0x6100);
                                c.put32(words + 4, data + 0x6400);
                                c.put32(words + 8, data + 0x6700);
                                c.put32(words + 12, table);
                                for (unsigned i = 0; i < 3; ++i) {
                                    put_string(c, data + 0x6100 + i * 0x2c, i == 0 ? "Zed" : "Ann" + std::to_string(i));
                                    put_string(c, data + 0x6400 + i * 0x2c, "Sur" + std::to_string(i));
                                    put_string(c, data + 0x6700 + i * 0x2c, "Com" + std::to_string(i));
                                }
                                put_string(c, strings + 0x1000, "Looked");
                                const auto r = table + 0x70;
                                c.put32(r + 0x14, row.flags14);
                                c.put32(r + 0x1c, row.flags1c);
                                c.put32(r + 0x28, row.a ? objects : 0);
                                c.put32(r + 0x2c, row.b ? objects + 0x40 : 0);
                                c.put32(r + 0x30, row.c ? objects + 0x80 : 0);
                                c.put32(objects + 0x20, std::uint32_t(set[0] + row.da));
                                c.put32(objects + 0x60, std::uint32_t(set[1] + row.db));
                                c.put32(objects + 0xa0, std::uint32_t(set[2] + row.dc));
                                c.set({{3, std::uint32_t(set[0])}, {4, std::uint32_t(set[1])}, {5, std::uint32_t(set[2])}, {6, buffer},
                                       {7, 0x69}, {8, mode}, {9, std::uint32_t(lookup)}});
                            }, stubs, mode == 6 && has == 1);
                    }
    // A null lookup object with a lookup index of 0 or more: the original skips the lookup (0x100cb680).
    for (const auto lookup : {std::int32_t(0), std::int32_t(1), std::int32_t(5)})
        for (const auto mode : {1u, 6u})
            add(out, entry, "indices null lookup object, lookup " + std::to_string(lookup) + " layout " + std::to_string(mode),
                [=](Cpu& c) {
                    c.put32(c.r(2) - 0x752c, words);
                    c.put32(c.r(2) - 0x7530, words + 4);
                    c.put32(c.r(2) - 0x7534, words + 8);
                    c.put32(c.r(2) - 0x7be0, words + 12);
                    c.put32(c.r(2) - 0x7a54, lookup_word);
                    c.put32(lookup_word, 0);
                    c.put32(words, data + 0x6100);
                    c.put32(words + 4, data + 0x6400);
                    c.put32(words + 8, data + 0x6700);
                    c.put32(words + 12, table);
                    put_string(c, data + 0x6100 + 0x2c, "Ann1");
                    put_string(c, data + 0x6400 + 0x2c, "Sur1");
                    put_string(c, data + 0x6700 + 0x2c, "Com1");
                    c.set({{3, 1}, {4, 1}, {5, 1}, {6, buffer}, {7, 0x69}, {8, mode}, {9, std::uint32_t(lookup)}});
                }, Stubs{1, strings + 0x1000}, mode == 6);
    // Arguments: no buffer, a low limit, a limit whose low 16 bits are fine, a layout the formatter rejects,
    // and a table base of 0 so the row is null (0x100cbde0 reports its own assertion).
    for (const auto& [buf, limit, layout, base] : std::array<std::array<std::uint32_t, 4>, 6>{{
             {0, 0x69, 1, 1}, {buffer, 0x68, 1, 1}, {buffer, 0x10069, 1, 1}, {buffer, 0x69, 9, 1}, {buffer, 0x69, 1, 0}, {buffer, 0x7fff, 3, 1}}})
        add(out, entry, "indices arguments " + hex_address(buf) + " " + hex_address(limit) + " layout " + std::to_string(layout) + " base " + std::to_string(base),
            [=](Cpu& c) {
                c.put32(c.r(2) - 0x752c, words);
                c.put32(c.r(2) - 0x7530, words + 4);
                c.put32(c.r(2) - 0x7534, words + 8);
                c.put32(c.r(2) - 0x7be0, words + 12);
                c.put32(c.r(2) - 0x7a54, lookup_word);
                c.put32(lookup_word, lookup_object);
                c.put32(words, data + 0x6100);
                c.put32(words + 4, data + 0x6400);
                c.put32(words + 8, data + 0x6700);
                c.put32(words + 12, base ? table : 0);
                put_string(c, data + 0x6100, "Zed");
                put_string(c, data + 0x6400, "Sur0");
                put_string(c, data + 0x6700, "Com0");
                c.put32(table + 0x14, 0);
                c.put32(table + 0x1c, 0);
                c.set({{3, 0}, {4, 0}, {5, 0}, {6, buf}, {7, limit}, {8, layout}, {9, 0}});
            }, Stubs{1, strings + 0x1000});
}

// 0x100d2720, 0x100d15f0 and 0x100ca140.
void tables(std::vector<Fixture>& out) {
    constexpr std::uint32_t buffer = data + 0x5200;
    for (const auto entry : {0x100d2720u, 0x100d15f0u})
        for (int value = -128; value < 128; ++value) {
            const bool interesting = value < 40 || value > 120 || value == 64;
            if (!interesting) continue;
            for (const auto flag : {0u, 1u, 0x100u, 0x80u, 0xffffff00u})
                for (const auto& [buf, limit] : std::array<std::array<std::uint32_t, 2>, 5>{{{buffer, 0x64}, {0, 0x64}, {buffer, 0x63}, {buffer, 0x8000}, {buffer, 0x10064}}}) {
                    if ((flag == 0x100 || flag == 0x80 || flag == 0xffffff00u) && (buf == 0 || limit != 0x64)) continue;
                    if ((buf == 0 || limit != 0x64) && flag > 1) continue;
                    add(out, entry, "attribute " + std::to_string(value) + " flag " + hex_address(flag) + " limit " + hex_address(limit),
                        [=](Cpu& c) {
                            c.put8(buffer, 0x55);
                            c.set({{3, (value & 1 ? 0x12345600u : 0u) | std::uint8_t(value)}, {4, buf}, {5, limit}, {6, flag}});
                        }, {}, (value & 3) == 0);
                }
        }
    for (int index = -128; index < 128; ++index)
        for (const bool overflow : {false, true}) {
            if (overflow && (index & 7) != 0) continue;
            add(out, 0x100ca140, "language " + std::to_string(index), [=](Cpu& c) {
                c.set(3, (index & 1 ? 0x12345600u : 0u) | std::uint8_t(index));
            }, {}, overflow);
        }
}

// 0x100cbf60: a record's name; the team pointer at +0xc matches the key word or not.
void records(std::vector<Fixture>& out) {
    constexpr std::uint32_t buffer = data + 0x5000, team = data + 0x2800, key_word = data + 0x2900;
    const auto prepare = [=](std::uint32_t record, std::uint32_t buf, std::uint32_t limit, std::uint32_t mode, const Text* b,
                             const Text* cc, int team_kind) {
        return [=](Cpu& c) {
            c.put32(c.r(2) - 0x7a18, key_word);
            c.put32(key_word, 0x1234);
            c.put32(team + 0x68, team_kind == 1 ? 0x1234 : 0x4321);
            c.put32(record_a + 0xc, team_kind == 0 ? 0 : team);
            c.put32(record_a + 4, b ? place(c, 2, *b) : 0);
            c.put32(record_a + 8, cc ? place(c, 3, *cc) : 0);
            c.set({{3, record}, {4, buf}, {5, limit}, {6, mode}});
        };
    };
    for (const auto mode : {0u, 1u, 2u, 3u, 4u, 5u, 6u, 7u, 8u, 0x101u, 0x1ffu, 0xffffff04u})
        for (const auto& b : texts)
            for (const auto& cc : texts)
                for (const auto team_kind : {0, 1, 2}) {
                    if (team_kind == 1 && (mode & 0xff) > 2) continue;
                    if (team_kind == 2 && (&b != &texts[3] && &b != &texts[0])) continue;
                    add(out, 0x100cbf60, std::string("record layout ") + std::to_string(mode) + " B=" + b.label + " C=" + cc.label +
                                         " team " + std::to_string(team_kind),
                        prepare(record_a, buffer, 0x69, mode, &b, &cc, team_kind), {}, (mode & 1) != 0);
                }
    for (const auto& [record, buf, limit] : std::array<std::array<std::uint32_t, 3>, 6>{{
             {0, buffer, 0x69}, {record_a, 0, 0x69}, {record_a, buffer, 0x68}, {record_a, buffer, 0x8000},
             {record_a, buffer, 0x10069}, {high + 0x100, buffer, 0x69}}})
        add(out, 0x100cbf60, "record arguments " + hex_address(record) + " " + hex_address(buf) + " " + hex_address(limit),
            [=](Cpu& c) {
                c.put32(high + 0x104, 0);
                c.put32(high + 0x108, 0);
                c.put32(high + 0x10c, 0);
                prepare(record, buf, limit, 1, &texts[3], &texts[4], 0)(c);
            });
}

// 0x100d6300: every combination of the mask's tested bits with the kinds 0x10727880 can report.
void mask_texts(std::vector<Fixture>& out) {
    constexpr std::uint32_t buffer = data + 0x5300;
    const std::array<std::uint32_t, 10> bits{0x1, 0x2, 0x4, 0x8, 0x10, 0x20, 0x40, 0x80, 0x200, 0x800};
    for (std::uint32_t n = 0; n < 1024; ++n) {
        std::uint32_t mask = 0;
        for (unsigned i = 0; i < 10; ++i) if (n >> i & 1) mask |= bits[i];
        for (const auto kind : {std::uint8_t(0), std::uint8_t(2), std::uint8_t(4), std::uint8_t(5), std::uint8_t(10)}) {
            if (n % 7 != 0 && kind != 2 && kind != 5 && kind != 10 && kind != 4 && kind != 0) continue;
            add(out, 0x100d6300, "mask " + hex_address(mask) + " kind " + std::to_string(kind), [=](Cpu& c) {
                c.put8(c.r(2) - 0x35ac, kind);
                c.set({{3, mask | (n & 1 ? 0xabc00000u : 0u)}, {4, buffer}, {5, 0x1e}, {6, 1}});
            }, {}, (n & 3) == 3);
        }
    }
    for (const auto limit : {0x1du, 0x8000u, 0x7fffu, 0u, 0x1e0001du})
        add(out, 0x100d6300, "mask limit " + hex_address(limit), [=](Cpu& c) {
            c.put8(c.r(2) - 0x35ac, 4);
            c.set({{3, 0x25}, {4, buffer}, {5, limit}, {6, 1}});
        });
}

// 0x100d6af0: the role label; the tested masks of the flag words, the table rows' ratings and the kind.
void roles(std::vector<Fixture>& out) {
    constexpr std::uint32_t buffer = data + 0x5400, words = data + 0x6000, table = data + 0x7000;
    const auto make = [=](std::uint32_t index, std::uint32_t role_bits, std::uint32_t flags, std::uint8_t kind,
                          std::uint32_t limit, std::uint32_t seed) {
        return [=](Cpu& c) {
            c.put8(c.r(2) - 0x35ac, kind);
            c.put32(c.r(2) - 0x7be0, words);
            c.put32(words, table);
            std::uint32_t x = seed;
            for (unsigned row = 0; row < 12; ++row) {
                x = x * 1664525u + 1013904223u;
                static constexpr std::array<std::uint8_t, 8> ratings{0x80, 0x11, 0x13, 0x14, 0x21, 0x22, 0x23, 0x7f};
                c.put16(table + row * 0x70 + 0x5c, 0xab00 | ratings[(x >> 20) & 7]);
                c.put8(table + row * 0x70 + 0x5e, (x >> 12) & 1);
            }
            c.set({{3, index}, {4, role_bits}, {5, flags}, {6, buffer}, {7, limit}, {8, 0x55}});
        };
    };
    const std::array<std::uint32_t, 4> role_masks{0x40, 0x10, 0x80, 0x800};
    const std::array<std::uint32_t, 17> flag_bits{0x40, 0x1, 0x2, 0x4, 0x200, 0x1000, 0x800, 0x10000, 0x40000,
                                                  0x200000, 0x2000, 0x8000, 0x800000, 0x100000, 0x80000, 0x20000, 0x400000};
    std::vector<std::uint32_t> flag_words{0, 0x2c0000, 0x240000, 0xc0000, 0x40000, 0x102000, 0x200800, 0x1200, 0x800200,
                                          0x102100, 0x80000000, 0xffffffff, 0x7fffffff, 0x1200 | 4, 0x200000 | 2, 0x10000 | 2};
    for (const auto bit : flag_bits) {
        flag_words.push_back(bit);
        flag_words.push_back(bit | 0x40000);
        flag_words.push_back(bit | 0x1);
        flag_words.push_back(bit | 0x200000 | 0x40000);
        flag_words.push_back(bit | 0x40);
    }
    const std::array<std::uint32_t, 9> indexes{0xffffffff, 0, 1, 2, 3, 4, 5, 7, 0xfffffff0};
    // Directed flag words for the texts the random mixes rarely reach: 0x22f0 (0xc0040), 0x2314 (0x102040),
    // 0x2324 (0x200840), 0x2348 (0x1240 without 0x800) and, with roles & 0x10 and 0x40 clear, 0x237b (0x840200).
    for (const auto flags : {0xc0040u, 0x102040u, 0x200840u, 0x1240u, 0x840200u})
        for (const auto role_bits : {0u, 0x10u, 0x90u, 0x810u})
            for (const auto index : {0xffffffffu, 1u, 4u})
                for (const std::uint8_t kind : {std::uint8_t(0), std::uint8_t(4), std::uint8_t(5)})
                    add(out, 0x100d6af0, "role directed " + hex_address(flags) + " roles " + hex_address(role_bits) + " index " +
                                             hex_address(index) + " kind " + std::to_string(kind),
                        make(index, role_bits, flags, kind, 0x1e, 11 + kind), {}, kind == 5);
    std::uint32_t seed = 0x1234567;
    unsigned n = 0;
    for (std::uint32_t roles_n = 0; roles_n < 16; ++roles_n) {
        std::uint32_t role_bits = 0;
        for (unsigned i = 0; i < 4; ++i) if (roles_n >> i & 1) role_bits |= role_masks[i];
        for (const auto flags : flag_words)
            for (const auto index : indexes) {
                seed = seed * 1664525u + 1013904223u;
                if (index == 0xfffffff0 && (seed >> 8) % 3) continue;
                if ((seed >> 8) % 2) continue;
                const std::uint8_t kind = std::array<std::uint8_t, 3>{0, 4, 5}[(seed >> 16) % 3];
                add(out, 0x100d6af0, "role " + hex_address(index) + " " + hex_address(role_bits) + " " + hex_address(flags) + " kind " +
                                         std::to_string(kind), make(index, role_bits, flags, kind, 0x1e, seed), {}, (++n & 7) == 0);
            }
    }
    for (const auto limit : {0x1du, 0x8000u, 0x7fffffffu, 0u})
        add(out, 0x100d6af0, "role limit " + hex_address(limit), make(1, 0x40, 0x2c0000, 4, limit, 7));
    // Every rating around both comparisons, for each remainder of the index.
    for (const auto index : indexes)
        for (const auto rating : {0x80u, 0x10u, 0x11u, 0x12u, 0x13u, 0x14u, 0x20u, 0x21u, 0x22u, 0x23u, 0x24u, 0x7fu})
            for (const auto flags : {0u, 0x100000u, 0x1u, 0x200000u})
                for (const auto third : {0u, 1u}) {
                    add(out, 0x100d6af0, "role row rating " + hex_address(rating) + " index " + hex_address(index), [=](Cpu& c) {
                        make(index, 0, flags, 4, 0x1e, 3)(c);
                        for (unsigned row = 0; row < 12; ++row) {
                            c.put16(table + row * 0x70 + 0x5c, 0xab00 | rating);
                            c.put8(table + row * 0x70 + 0x5e, third);
                        }
                        c.put32(table - 0x70 * 0 + 0, 0);  // keep the first row's words defined
                    });
                }
}

// 0x100db410: every subset of the flag bits the tests look at, with the variants and kinds.
void flag_labels(std::vector<Fixture>& out) {
    constexpr std::uint32_t buffer = data + 0x5500;
    const std::array<std::uint8_t, 4> kinds{0, 2, 4, 5};
    for (std::uint32_t n = 0; n < 4096; ++n) {
        std::uint32_t flags = 0;
        for (unsigned i = 0; i < 12; ++i) if (n >> i & 1) flags |= 1u << i;
        for (unsigned rep = 0; rep < 2; ++rep) {
            const std::uint32_t variant = std::array<std::uint32_t, 4>{0, 1, 0x100, 0xffffff80}[(n + rep * 2) % 4];
            const auto kind = kinds[(n / 4 + rep) % 4];
            add(out, 0x100db410, "flags " + hex_address(flags) + " variant " + hex_address(variant) + " kind " + std::to_string(kind),
                [=](Cpu& c) {
                    c.put8(c.r(2) - 0x35ac, kind);
                    c.set({{3, flags | (n % 5 == 0 ? 0xabc00000u : 0u)}, {4, buffer}, {5, variant}});
                }, {}, n % 9 == 0);
        }
    }
    add(out, 0x100db410, "flags null buffer", [=](Cpu& c) { c.set({{3, 0x260}, {4, 0}, {5, 0}}); });
    add(out, 0x100db410, "flags null buffer overflow", [=](Cpu& c) { c.set({{3, 0x260}, {4, 0}, {5, 0}}); }, {}, true);
}

// 0x100d0940: a record's name in the four selector layouts; the two range tests and the table lookup.
void club_names(std::vector<Fixture>& out) {
    constexpr std::uint32_t buffer = data + 0x5600, words = data + 0x6000, table = data + 0x6100, flags_table = data + 0x6200,
                            owner = data + 0x6300;
    const auto make = [=](std::uint32_t record, std::uint32_t long_form, std::uint32_t limit, std::uint32_t selector, std::uint32_t second,
                          std::uint32_t first, std::uint8_t looked, std::uint32_t grammar) {
        return [=](Cpu& c) {
            c.put32(c.r(2) - 0x7b04, words);
            c.put32(c.r(2) - 0x7b50, words + 4);
            c.put32(c.r(2) - 0x7a64, words + 8);
            c.put32(words, 5);
            c.put32(words + 4, 100);
            c.put32(words + 8, table);
            c.put32(table + 0x38, flags_table);
            c.put8(flags_table + 3, looked);
            c.put32(owner + 0x68, 3);
            for (const auto base : {record_a, high + 0x100}) {
                c.put32(base, first);
                c.put32(base + 0x2c, owner);
                put_string(c, base + 0x1ee, "Alpha");
                put_string(c, base + 0x221, "Beta");
                c.put8(base + 0x242, grammar);
                c.put8(base + 0x23b, grammar ^ 1);
            }
            c.set({{3, record}, {4, long_form}, {5, buffer}, {6, limit}, {7, selector}, {8, second}});
        };
    };
    for (const auto selector : {0u, 1u, 2u, 3u, 4u, 5u, 0xffffffffu, 0x103u, 0x80u})
        for (const auto record : {record_a, high + 0x100})  // a null record asserts, then faults on its name in both
            for (const auto long_form : {0u, 1u, 0x100u})
                for (const auto second : {0u, 1u, 0x100u})
                    for (const auto first : {89u, 90u, 91u, 94u, 95u, 96u, 0x80000000u, 0xffffffffu, 0u})
                        for (const auto looked : {std::uint8_t(0), std::uint8_t(1), std::uint8_t(0x80)}) {
                            if (second == 0x100 && long_form != 1) continue;
                            if (looked != 0 && (long_form == 1 || (first != 90 && first != 95 && first != 96))) continue;
                            if (selector > 4 && (first != 90 || long_form != 0 || second != 0 || looked != 0)) continue;
                            add(out, 0x100d0940, "club selector " + hex_address(selector) + " record " + hex_address(record) +
                                                     " long " + std::to_string(long_form) + " second " + std::to_string(second) +
                                                     " first " + std::to_string(first) + " looked " + std::to_string(looked),
                                make(record, long_form, 0x47, selector, second, first, looked, selector & 1 ? 0x85 : 3), {},
                                (first & 1) != 0);
                        }
    // A null record: the assertion, then its name is read at the low addresses, which these cases map.
    for (const auto selector : {1u, 2u, 3u, 4u})
        for (const auto long_form : {0u, 1u})
            add(out, 0x100d0940, "club null record selector " + std::to_string(selector) + " long " + std::to_string(long_form),
                [=](Cpu& c) {
                    c.s.memory.map(0, 0x1000);
                    c.s.memory.copy_in(0, std::vector<std::uint8_t>(0x1000, 0));
                    make(0, long_form, 0x47, selector, 0, 90, 0, 3)(c);
                });
    for (const auto limit : {0x46u, 0u, 0x80000000u, 0x47u, 0x7fffffffu})
        add(out, 0x100d0940, "club limit " + hex_address(limit), make(record_a, 0, limit, 1, 0, 95, 0, 3));
}

// 0x100cc4a0: the summary of a player's ratings; every rating level, single strong ratings, random mixes.
void summaries(std::vector<Fixture>& out) {
    constexpr std::uint32_t buffer = data + 0x5800;
    using Ratings = std::array<std::uint8_t, 12>;  // the bytes +0xf-+0x1a
    const auto make = [=](const Ratings& ratings, std::uint32_t mode, std::uint32_t limit, std::uint32_t record, std::uint32_t buf) {
        return [=](Cpu& c) {
            for (unsigned i = 0; i < ratings.size(); ++i) c.put8(record_a + 0xf + i, ratings[i]);
            c.put8(high + 0x100 + 0xf, 0);
            for (unsigned i = 0; i < ratings.size(); ++i) c.put8(high + 0x100 + 0xf + i, ratings[i]);
            c.set({{3, record}, {4, buf}, {5, limit}, {6, mode}});
        };
    };
    const std::array<std::uint32_t, 7> modes{0, 1, 2, 3, 4, 5, 0x101};
    const auto describe = [](const Ratings& r, std::uint32_t mode) {
        std::string text = "mode " + std::to_string(mode) + " ratings";
        for (const auto v : r) text += ' ' + std::to_string(v);
        return text;
    };
    std::vector<Ratings> sets;
    for (const std::uint8_t level : {0, 9, 10, 11, 14, 15, 16, 20, 0x90, 0xff}) {
        Ratings r;
        r.fill(level);
        sets.push_back(r);
        for (unsigned i = 0; i < r.size(); ++i) {
            Ratings single{};
            single[i] = level;
            sets.push_back(single);
        }
    }
    std::uint32_t seed = 0xdeadbeef;
    for (unsigned n = 0; n < 700; ++n) {
        Ratings r;
        for (auto& v : r) {
            seed = seed * 1664525u + 1013904223u;
            const auto pick = (seed >> 24) & 7;
            v = pick < 3 ? 0 : pick == 3 ? std::uint8_t(9 + ((seed >> 8) & 7)) : pick == 4 ? 15 : pick == 5 ? 20 : std::uint8_t((seed >> 12) & 15);
        }
        sets.push_back(r);
    }
    unsigned n = 0;
    for (const auto& r : sets)
        for (const auto mode : modes) {
            if ((n++ % 3) != 0 && mode > 1 && mode != 2 && mode != 4) continue;
            add(out, 0x100cc4a0, "summary " + describe(r, mode), make(r, mode, 0x1e, record_a, buffer), {}, (n & 7) == 0);
        }
    for (const auto& [record, buf, limit] : std::array<std::array<std::uint32_t, 3>, 6>{{
             {0, buffer, 0x1e}, {record_a, 0, 0x1e}, {record_a, buffer, 0x1d}, {record_a, buffer, 0x8000},
             {record_a, buffer, 0x1001e}, {high + 0x100, buffer, 0x1e}}}) {
        Ratings r;
        r.fill(15);
        add(out, 0x100cc4a0, "summary arguments " + hex_address(record) + " " + hex_address(buf) + " " + hex_address(limit),
            make(r, 1, limit, record, buf));
    }
}

// 0x100d5de0: the scaled rating; random combinations of the records' fields, plus the null cases.
void scaled_ratings(std::vector<Fixture>& out) {
    constexpr std::uint32_t words = data + 0x6000, team_base = data + 0x6100, entry24 = data + 0x6300, club_words = data + 0x6500,
                            club_base_word = data + 0x6510, primary = data + 0x6700, role = data + 0x6800,
                            object_a = data + 0x6900, object459 = data + 0x6a00, scale_word = data + 0x6b00;
    struct Params {
        bool null_sixth, null_entry, null_b, b_zero14, null_primary, club_matches, null_role, null_object;
        std::int16_t rating, object_scale, team_scale;
        std::uint8_t type, level, grade;
        bool same_words;
        int role_match;  // 0: none; 1-3: plus slots; 4-6: minus slots; 7: plus and minus
        double scaled;
    };
    const auto make = [=](const Params& p) {
        return [=](Cpu& c) {
            c.put32(c.r(2) - 0x7b5c, words);
            c.put32(words, team_base);
            c.put32(team_base + 0, p.null_entry ? 0 : entry24);
            c.put32(object_a, 0);  // the team index of a's object
            c.put32(object_a + 0x18, 0);
            c.put16(object_a + 0x18, std::uint16_t(p.object_scale));
            c.put32(record_a + 0x30, p.null_sixth ? 0 : object_a);
            c.put32(record_a + 0x34, p.same_words ? (p.null_sixth ? 0 : object_a) : 0x1234);
            c.put16(record_a + 0x1ec, std::uint16_t(p.rating));
            c.put8(record_a + 0x23c, p.grade);
            put_string(c, record_a + 0x221, "Aa");
            c.put32(entry24 + 4, 3);
            c.put32(entry24 + 8, 0x77);
            c.put16(entry24 + 0x3e, std::uint16_t(p.team_scale));
            c.put8(object459 + 4, p.type);
            c.put8(object459 + 0x76, p.level);
            c.put32(c.r(2) - 0x7b7c, words + 0x20);
            c.put32(words + 0x20, 0x1111);
            c.put32(c.r(2) - 0x7b78, words + 0x24);
            c.put32(c.r(2) - 0x7be8, club_words);
            c.put32(club_words, club_base_word);
            c.put32(c.r(2) - 0x7b3c, scale_word);
            c.put32(scale_word, 0x2222);
            c.put32(c.r(2) - 0x7bb8, data + 0x5c00);  // the null-object message buffer (sprintf destination)
            // The primary contract's club: base + index x 0x248 is a when the club matches.
            c.put32(primary + 4, 2);
            c.put32(club_base_word, p.club_matches ? record_a - 2 * 0x248 : 0x1000);
            c.put32(club_base_word - 4, 0);
            c.put32(record_b + 0x14, p.b_zero14 ? 0 : 0x55);
            c.put32(record_b + 0x18, p.null_role ? 0 : role);
            for (unsigned i = 1; i <= 6; ++i) c.put32(role + i * 4, 0x4000 + i);  // plus slots +4/+8/+0xc, minus slots +0x10/+0x14/+0x18
            if (p.role_match >= 1 && p.role_match <= 3) c.put32(role + p.role_match * 4, record_a);
            if (p.role_match >= 4 && p.role_match <= 6) c.put32(role + 0x10 + (p.role_match - 4) * 4, record_a);
            if (p.role_match == 7) { c.put32(role + 0xc, record_a); c.put32(role + 0x14, record_a); }
            c.set({{3, record_a}, {4, p.null_b ? 0u : record_b}});
        };
    };
    std::uint32_t seed = 0x13579bd;
    const auto pick = [&](std::uint32_t n) {
        seed = seed * 1664525u + 1013904223u;
        return (seed >> 16) % n;
    };
    const std::array<std::int16_t, 7> ratings{0, 100, 1000, -500, 0x7fff, -0x8000, 2999};
    const std::array<std::int16_t, 6> object_scales{0, 1, 2, -1, 6, 0x7fff};
    const std::array<std::int16_t, 6> team_scales{0, 0x14, 10, 40, -5, 1};
    const std::array<std::uint8_t, 7> types{0, 5, 0x13, 0x14, 0x20, 1, 0xff};
    const std::array<std::uint8_t, 6> levels{0, 1, 5, 20, 0xff, 0x80};
    const std::array<std::uint8_t, 8> grades{0, 1, 5, 20, 21, 0x80, 0xff, 3};
    for (unsigned n = 0; n < 6000; ++n) {
        Params p{};
        p.null_sixth = pick(12) == 0;
        p.null_entry = pick(8) == 0;
        p.null_b = pick(8) == 0;
        p.b_zero14 = pick(8) == 0;
        p.null_primary = pick(5) == 0;
        p.club_matches = pick(3) != 0;
        p.null_role = pick(6) == 0;
        p.null_object = pick(10) == 0;
        p.rating = ratings[pick(7)];
        p.object_scale = object_scales[pick(6)];
        p.team_scale = team_scales[pick(6)];
        p.type = types[pick(7)];
        p.level = levels[pick(6)];
        p.grade = grades[pick(8)];
        p.same_words = pick(2) == 0;
        p.role_match = int(pick(9)) > 7 ? 0 : int(pick(8));
        p.scaled = pick(2) ? 0.75 : -3.25;
        Stubs stubs;
        stubs.object = p.null_object ? 0 : object459;
        stubs.primary = p.null_primary ? 0 : primary;
        stubs.scaled = p.scaled;
        add(out, 0x100d5de0, "scaled " + std::to_string(n), make(p), stubs, (n & 7) == 0);
    }
}

std::vector<Fixture> text_fixtures() {
    std::vector<Fixture> found;
    scaled_ratings(found);
    summaries(found);
    club_names(found);
    flag_labels(found);
    roles(found);
    mask_texts(found);
    records(found);
    tables(found);
    indices(found);
    parts(found);
    titles(found);
    names(found);
    comparators(found);
    dates(found);
    small(found);
    return found;
}
}
