// DATABASE.CPP's loading block, part "language": the language file's load into the language object
// (0x100d4970), the seven accessors of its arrays and the class lookup (database_loading.hpp).
#include "database_loading.hpp"
#include "database_loading_detail.hpp"
#include "guest_values.hpp"
#include <array>

namespace cm::aot::database {
namespace {
using guest::compare;
using guest::compare_logical;
using guest::extended;
namespace callee = loading::callee;
namespace toc = loading::toc;

std::uint32_t byte_swapped(std::uint32_t word) { return std::byteswap(word); }

namespace layout = loading::object;
}

// 0x100d4970: frame of 0xa0 saving r26-r31. Held across the calls: r26 the byte-swap flag, r27 the success
// flag's address, r28 the message buffer, r29 the string pool, r30 the file, r31 the object.
bool load_language_object(Cpu& c, std::uint32_t object) {
    constexpr std::uint32_t entry = 0x100d4970, signature = 0xf1fffff1;
    c.set(0, c.lr());
    guest::Frame frame(c, entry, 0xa0, {.first_gpr = 26});
    const auto pool = c.toc(toc::string_pool);
    c.set({{29, pool}, {31, object}});
    c.set(27, c.toc(toc::success_flag));
    c.set(28, c.toc(toc::message_buffer));
    const auto fail = [&](std::uint32_t at, std::uint32_t name, std::uint32_t extension, std::uint32_t line) {
        guest::report_assertion(c, {at, pool, pool + 0xd, pool + 0x1a, frame.at(name), frame.at(extension), line});
        c.set({{0, 0}, {3, 0}});
        return false;
    };
    c.set({{3, pool + loading::pool::folder}, {4, pool + 0x846}});
    c.call(callee::data_folder_path, entry + 0x2c);
    c.set(4, pool + loading::pool::mode_read);
    c.call(callee::open_stream, entry + 0x34);
    const auto file = c.r(3);
    c.set(30, file);
    if (compare(c, std::int32_t(file)) == 0) return fail(0x100d49c0, 0x68, 0x40, 0x4cf9);

    // The file's signature check: the marker is compared against a constant; a different one means the
    // header is in the other byte order.
    c.set(3, file);
    c.call(callee::read_file_marker, entry + 0x9c);
    const auto marker = c.r(3);
    const auto different = ((signature - marker) | (marker - signature)) >> 31;
    c.set(26, different);  // extsb of 0 or 1
    c.set({{0, different}, {3, object}, {4, 0x20}, {5, 1}, {6, file}});
    c.call(callee::read_block, entry + 0xd0);
    const bool swap = compare(c, std::int32_t(c.r(26))) != 0;  // extsb. r0,r26
    if (swap) {
        std::uint32_t last = 0;
        for (std::uint32_t at = 0; at < 0x20; at += 4) {
            last = c.u32(object + at);
            c.put32(object + at, byte_swapped(last));
        }
        c.set({{0, byte_swapped(last)}, {3, last}, {4, last}, {5, last}});
    }

    // The header's counts against the loaded tables' counts (TOC words point at them).
    const auto matches = [&] {
        const auto word = [&](std::uint32_t offset) { return c.u32(object + offset); };
        const auto against = [&](std::int32_t slot, std::uint32_t offset) {
            const auto expected = c.toc(slot);
            c.set({{3, expected}, {4, word(offset)}});
            c.set(0, c.u32(expected));
            c.compare(0, std::int32_t(c.r(4)), std::int32_t(c.r(0)));
            return c.r(4) == c.r(0);
        };
        if (!against(-0x7524, 0x4)) return false;
        if (!against(-0x7b04, 0xc)) return false;
        if (!against(-0x7558, 0x10)) return false;
        const auto other = c.toc(-0x7b64);
        c.set(4, other);
        c.set(3, word(0x14));
        c.set(4, c.u32(c.r(4)));
        c.set(0, word(0x8));
        c.set(4, c.r(4) - 0x7f);
        c.set(0, c.r(3) + c.r(0));
        c.compare(0, std::int32_t(c.r(4)), std::int32_t(c.r(0)));
        if (c.r(4) != c.r(0)) return false;
        return against(-0x7568, 0x18) && against(-0x755c, 0x1c);
    }();
    if (!matches) {
        c.set(3, file);
        c.call(callee::close_stream, entry + 0x284);
        return fail(0x100d4c08, 0x48, 0x38, 0x4d18);
    }

    // The eight arrays: allocate count * size and, when the allocation answers, read them from the file.
    const loading::LanguageObject view(c, object);
    for (std::uint32_t n = 0; n < layout::in_file_order.size(); ++n) {
        const auto& table = layout::in_file_order[n];
        const auto count = view.count(table);
        c.set({{0, count}, {3, count * table.record_size}});
        c.call(callee::allocate, entry + 0x2e8 + 0x30 * n);
        view.set_array(table, c.r(3));
        const auto block = view.array(table);
        c.set(0, block);
        if (compare_logical(c, block) == 0) continue;
        c.set({{0, block}, {3, block}, {6, file}, {5, view.count(table)}, {4, table.record_size}});
        c.call(callee::read_block, entry + 0x30c + 0x30 * n);
    }
    c.set(3, file);
    c.call(callee::close_stream, entry + 0x464);
    c.set(3, 1);
    return true;
}

namespace {
// The head every accessor shares: its frame (saving r30-r31, or r29-r31 where r29 is the object's
// register and r30 holds the table word at TOC -0x74f8), the language byte test, and the shared object's
// creation on first use. `new_back` and `init_back` are the return addresses of the two calls.
struct Accessor {
    std::uint32_t entry, frame_size, first_gpr, inline_offset, new_back, init_back;
    bool mapped;  // r30 holds the TOC word at -0x74f8
    unsigned object_register() const { return first_gpr; }
};

template <class Compute>
std::uint32_t language_accessor(Cpu& c, const Accessor& a, std::uint32_t argument, Compute&& compute) {
    c.set(0, c.lr());
    guest::Frame frame(c, a.entry, a.frame_size, {.first_gpr = a.first_gpr});
    const auto mode_at = c.toc(toc::language_byte);
    c.set({{4, mode_at}, {31, argument}});
    if (a.mapped) c.set(30, c.toc(-0x74f8));
    const auto mode = c.u8(mode_at);
    const auto extended_mode = extended(std::int16_t(std::int8_t(mode)));
    c.set(0, extended_mode);
    bool inside_record = compare(c, std::int32_t(extended_mode)) == 0;
    if (!inside_record) {
        c.set(0, mode);
        inside_record = compare(c, std::int32_t(mode), 1) == 0;
    }
    if (inside_record) {
        c.set(3, argument + a.inline_offset);
        return c.r(3);
    }
    const auto object_slot = c.r(2) + std::uint32_t(toc::language_object);
    const auto existing = c.u32(object_slot);
    c.set(0, existing);
    if (compare_logical(c, existing) == 0) {
        c.set(3, loading::object::size);
        c.call(callee::operator_new, a.entry + a.new_back);
        const auto created = c.r(3);
        c.set(a.object_register(), created);
        if (compare(c, std::int32_t(created)) != 0) {
            loading::LanguageObject(c, created).clear_arrays();
            c.set({{3, created}, {4, 0}, {5, 0}, {6, 0}, {7, 0}, {0, 0}});
            guest::enter(c, a.entry + a.init_back, {created});
            load_language_object(c, created);
        }
        c.put32(object_slot, c.r(a.object_register()));
    }
    const auto object = c.u32(object_slot);
    c.set(0, object);
    if (compare_logical(c, object) == 0) {
        c.set(3, 0);
        return 0;
    }
    return compute(object);
}
}

// 0x100d4df0 and 0x100d4ee0: the 0x6d-byte rows (rows_6d_to_128), indexed by the record's word +0x68;
// the second text starts 0x4f bytes into the row.
std::uint32_t language_text_6d_first(Cpu& c, std::uint32_t record) {
    return language_accessor(c, {0x100d4df0, 0x40, 30, 0xae, 0x50, 0xa0, false}, record, [&](std::uint32_t object) {
        c.set(0, c.u32(record + 0x68));
        c.set(3, c.u32(c.r(2) + std::uint32_t(toc::language_object)));
        c.set(0, c.r(0) * 0x6d);
        c.set(3, loading::LanguageObject(c, c.r(3)).array(layout::rows_6d_to_128));
        c.set(3, c.r(3) + c.r(0));
        (void)object;
        return c.r(3);
    });
}
std::uint32_t language_text_6d_second(Cpu& c, std::uint32_t record) {
    return language_accessor(c, {0x100d4ee0, 0x40, 30, 0xa4, 0x50, 0xa0, false}, record, [&](std::uint32_t object) {
        c.set(0, c.u32(record + 0x68));
        c.set(4, object);
        c.set(3, c.r(0) * 0x6d);
        c.set(0, loading::LanguageObject(c, object).array(layout::rows_6d_to_128));
        c.set(3, c.r(3) + 0x4f);
        c.set(3, c.r(0) + c.r(3));
        return c.r(3);
    });
}

// 0x100d4fd0: indices below people_53's count read the 0x53-byte rows of people_53; the others
// those of people_53_appended from index minus that count.
std::uint32_t language_text_53(Cpu& c, std::uint32_t index_word) {
    return language_accessor(c, {0x100d4fd0, 0x40, 30, 0x1a, 0x50, 0xa0, false}, index_word, [&](std::uint32_t object) {
        c.set(4, object);
        c.set(3, c.u32(index_word));
        const loading::LanguageObject view(c, object);
        c.set(0, view.count(layout::people_53));
        if (compare(c, std::int32_t(c.r(3)), std::int32_t(c.r(0))) < 0) {
            c.set(0, c.u32(index_word));
            c.set(3, view.array(layout::people_53));
            c.set(0, c.r(0) * 0x53);
            c.set(3, c.r(3) + c.r(0));
        } else {
            c.set(3, view.count(layout::people_53));
            c.set(0, c.u32(index_word));
            c.set(4, view.array(layout::people_53_appended));
            c.set(0, c.r(0) - c.r(3));
            c.set(0, c.r(0) * 0x53);
            c.set(3, c.r(4) + c.r(0));
        }
        return c.r(3);
    });
}

// 0x100d50e0: the 0x4f-byte rows of rows_4f_to_68.
std::uint32_t language_text_4f_3c(Cpu& c, std::uint32_t index_word) {
    return language_accessor(c, {0x100d50e0, 0x40, 30, 0x30, 0x50, 0xa0, false}, index_word, [&](std::uint32_t) {
        c.set(0, c.u32(index_word));
        c.set(3, c.u32(c.r(2) + std::uint32_t(toc::language_object)));
        c.set(0, c.r(0) * 0x4f);
        c.set(3, loading::LanguageObject(c, c.r(3)).array(layout::rows_4f_to_68));
        c.set(3, c.r(3) + c.r(0));
        return c.r(3);
    });
}

namespace {
// 0x100d51d0 and 0x100d5310: the 0x4f-byte rows of clubs_4f. When the table word at TOC -0x74f8
// points at a map of 32-bit row numbers the index goes through it; otherwise it must be below the count at
// clubs_4f's count (else null). The first variant's rows start 0x34 bytes in.
std::uint32_t language_text_4f_20_with(Cpu& c, const Accessor& a, std::uint32_t index_word, bool row_offset) {
    return language_accessor(c, a, index_word, [&](std::uint32_t object) {
        const loading::LanguageObject view(c, object);
        c.set(0, c.u32(c.r(30)));
        c.set(4, c.u32(c.r(2) + std::uint32_t(toc::language_object)));
        if (compare_logical(c, c.r(0)) != 0) {
            c.set(0, c.u32(index_word));
            c.set(3, c.u32(c.r(30)));
            c.set(0, c.r(0) << 2);
            c.set(4, view.array(layout::clubs_4f));
            c.set(0, c.u32(c.r(3) + c.r(0)));
            if (row_offset) {
                c.set(3, c.r(0) * 0x4f);
                c.set(3, c.r(3) + 0x34);
                c.set(3, c.r(4) + c.r(3));
            } else {
                c.set(0, c.r(0) * 0x4f);
                c.set(3, c.r(4) + c.r(0));
            }
            return c.r(3);
        }
        c.set(3, c.u32(index_word));
        c.set(0, view.count(layout::clubs_4f));
        if (compare(c, std::int32_t(c.r(3)), std::int32_t(c.r(0))) >= 0) {
            c.set(3, 0);
            return std::uint32_t(0);
        }
        c.set(0, c.u32(index_word));
        if (row_offset) {
            c.set(4, view.array(layout::clubs_4f));
            c.set(3, c.r(0) * 0x4f);
            c.set(3, c.r(3) + 0x34);
            c.set(3, c.r(4) + c.r(3));
        } else {
            c.set(3, view.array(layout::clubs_4f));
            c.set(0, c.r(0) * 0x4f);
            c.set(3, c.r(3) + c.r(0));
        }
        return c.r(3);
    });
}
}
std::uint32_t language_text_4f_20_mapped(Cpu& c, std::uint32_t index_word) {
    return language_text_4f_20_with(c, {0x100d51d0, 0x50, 29, 0x221, 0x58, 0xa8, true}, index_word, true);
}
std::uint32_t language_text_4f_20(Cpu& c, std::uint32_t index_word) {
    return language_text_4f_20_with(c, {0x100d5310, 0x50, 29, 0x1ee, 0x58, 0xa8, true}, index_word, false);
}

// 0x100d5450: the 0x34-byte rows of rows_34_to_50.
std::uint32_t language_text_34(Cpu& c, std::uint32_t index_word) {
    return language_accessor(c, {0x100d5450, 0x40, 30, 0x18, 0x50, 0xa0, false}, index_word, [&](std::uint32_t) {
        c.set(0, c.u32(index_word));
        c.set(3, c.u32(c.r(2) + std::uint32_t(toc::language_object)));
        c.set(0, c.r(0) * 0x34);
        c.set(3, loading::LanguageObject(c, c.r(3)).array(layout::rows_34_to_50));
        c.set(3, c.r(3) + c.r(0));
        return c.r(3);
    });
}

namespace {
// The switch tables of 0x100d5540, one result per index 0..0x23 (index 0 is the default, -1). Tables
// at TOC -0x5694 (kinds 10, 6 and 8), -0x5698 (3), -0x56a0 (7), -0x56a4 (4), -0x56a8 (5) and -0x56ac (9);
// the table at -0x569c (kind 6 again) is unreachable, kind 6 being caught by the first test.
using ClassTable = std::array<std::int8_t, 36>;
constexpr ClassTable twelve_and_thirteen{-1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0,
                                         0,  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
constexpr ClassTable kind_4{-1, 2, 3, 0, 2, 2, 3, 0, 0, 2, 0, 0, 0, 1, 1, 0, 1, 2,
                            3,  0, 3, 0, 0, 0, 2, 0, 0, 1, 2, 0, 0, 0, 0, 0, 0, 0};
constexpr ClassTable kind_5{-1, 2, 3, 2, 2, 1, 3, 2, 1, 2, 2, 3, 1, 3, 3, 1, 3, 2,
                            3,  2, 4, 1, 1, 1, 2, 2, 1, 3, 2, 1, 1, 1, 1, 3, 2, 2};
constexpr ClassTable kind_9{-1, 1, 2, 1, 1, 0, 1, 1, 1, 1, 1, 2, 0, 2, 3, 1, 2, 2,
                            3,  1, 3, 0, 0, 0, 1, 1, 0, 2, 1, 0, 0, 0, 0, 1, 1, 0};
struct KindTable {
    std::int32_t slot;
    const ClassTable& classes;
};
}

// 0x100d5540: frameless. r5 holds the sign-extended kind and r0 the answer; a kind with a table loads the
// handler address (the switch's target) into r4 and CTR before its `li r0`.
std::int32_t language_class(Cpu& c, std::int32_t index, std::int32_t kind) {
    c.set(5, extended(std::int16_t(std::int8_t(kind))));
    c.set(0, 0xffffffff);
    const auto kind_number = std::int8_t(kind);
    const auto step = [&] { c.set(5, extended(std::int16_t(std::int8_t(c.r(4))))); return std::int32_t(c.r(5)); };
    const auto is = [&](std::int32_t number) {
        step();
        return compare(c, std::int32_t(c.r(5)), number) == 0;
    };
    const KindTable* table = nullptr;
    static constexpr KindTable first{-0x5694, twelve_and_thirteen}, three{-0x5698, twelve_and_thirteen},
        seven{-0x56a0, twelve_and_thirteen}, four{-0x56a4, kind_4}, five{-0x56a8, kind_5}, nine{-0x56ac, kind_9};
    if (is(10) || is(6) || is(8)) table = &first;
    else if (is(3)) table = &three;
    else if (is(6)) table = &first;  // never taken: kind 6 left through the first test
    else if (is(7)) table = &seven;
    else if (is(4)) table = &four;
    else if (is(5)) table = &five;
    else {
        c.set(4, extended(std::int16_t(kind_number)));
        if (compare(c, std::int32_t(c.r(4)), 9) == 0) table = &nine;
    }
    if (table) {
        const auto position = extended(std::int16_t(std::int8_t(index)));
        c.set(3, position);
        if (compare_logical(c, position, 0x23) <= 0) {
            c.set(4, c.toc(table->slot));
            c.set(3, position << 2);
            c.set(4, c.u32(c.r(4) + c.r(3)));
            c.s.wr<4>(guest::reg::ctr, c.r(4));
            c.set(0, std::uint32_t(std::int32_t(table->classes[position])));
        }
    }
    c.set(3, c.r(0));
    return std::int32_t(c.r(3));
}
}
