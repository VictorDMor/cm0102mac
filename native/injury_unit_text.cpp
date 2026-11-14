// INJURY.CPP: the names and descriptions of injuries, the texts of screens and the news of a recovery
// or a match (injury_unit_text.hpp).
//
// The game builds its texts with the varargs formatter 0x10727890 (it reads r3-r10), so each call
// below sets the registers the original sets and leaves the others as the previous callee left them.
#include "injury_unit_text.hpp"
#include "injury_unit_detail.hpp"

namespace cm::aot::injury_unit {
using guest::Cpu;
using guest::Frame;

namespace {
#include "injury_unit_text_tables.inc"

namespace routine {
constexpr std::uint32_t prepare_status_row = 0x10187a40;
}
namespace callee_text {
constexpr std::uint32_t format = 0x10727890,       // (buffer, format, ...)
                        format_wide = 0x1077b720,  // the same with the call convention of the report
                        language = 0x10727880,     // the game's language
                        clock_reading = 0x10749fd0, clock_copy = 0x10749fc0, exit_game = 0x1076f7d0,
                        field_of_news = 0x102cb570,   // (news record, field index)
                        make_date = 0x107156b0,       // (date, first halfword, second halfword)
                        person_text = 0x100cb590,     // (a, b, c, buffer, 0x99, mode, person)
                        person_allowed = 0x100cbde0,  // (person record, a, b, c, date)
                        sink_line = 0x1074b830, sink_append = 0x1074b960, sink_link = 0x1074bab0,
                        format_days = 0x1030e880, date_apart = 0x10717dd0, match_check = 0x104abf40,
                        match_link = 0x1029eb20;
}
namespace toc_text {
// Words of the table of contents that this file alone uses.
constexpr std::int32_t competitions = -0x7bbc,  // the cell holding the competition table (0x6c bytes each)
                       news_buffer = -0x7bb8,   // the text buffer of the news
                       manager_cell = -0x7ba0,  // the cell holding the injury manager
                       match_state = -0x7b34, match_context = -0x7b4c;
}
namespace pool_text {
constexpr std::uint32_t no_injury = 0xd7, empty = 0xf6;
}
namespace news_record {
constexpr std::uint32_t competition = 0x0c, first_club = 0x18, second_club = 0x1c, first_score = 0x20, second_score = 0x21,
                        link = 0x24;
}
namespace club_row {
constexpr std::uint32_t name = 0x221, tag = 0x23b;
}

constexpr CheckSite row_date_reading_without_person{0x1018c1d8, 0x40, 0x38, 0xd71};
constexpr CheckSite injury_name_without_person{0x1018c494, 0x40, 0x38, 0xe4c};
constexpr CheckSite name_text_small_buffer{0x1018c5a4, 0x90, 0x48, 0xe76};
constexpr CheckSite name_text_plain_past_table{0x1018cb50, 0x70, 0x40, 0xed5};
constexpr CheckSite name_text_alternate_past_table{0x1018d0f0, 0x50, 0x38, 0xf32};
constexpr CheckSite time_left_without_person{0x1018d998, 0x48, 0x40, 0x1056};
constexpr CheckSite form_text_small_buffer{0x1018daf0, 0x68, 0x40, 0x10b9};
constexpr CheckSite form_text_past_table{0x1018dc38, 0x48, 0x38, 0x10d0};
constexpr CheckSite has_injury_without_person{0x1018f388, 0x40, 0x38, 0xe23};
constexpr CheckSite record_date_first_without_person{0x1018fe2c, 0x68, 0x40, 0x1462};
constexpr CheckSite record_date_second_without_person{0x1018fe8c, 0x48, 0x38, 0x1466};

std::uint32_t sign_extended(std::uint32_t byte) { return std::uint32_t(std::int32_t(std::int8_t(byte))); }

// A call to a routine of this file: the arguments in r3.. as the original's call has them (the
// formatters below read them) and the return address its prologue saves.
void enter(Cpu& c, std::uint32_t back, std::initializer_list<std::uint32_t> arguments) {
    unsigned n = 3;
    for (const auto argument : arguments) c.set(n++, argument);
    c.link(back);
}
}

std::uint32_t row_date_reading(Cpu& c, std::uint32_t manager_object, std::uint32_t person) {
    Frame frame(c, 0x1018c1a0, 0x70, {.first_gpr = 30});
    c.set({{31, c.toc(toc::pool)}, {30, c.toc(toc::message)}});
    if (person == 0) {
        report_check(c, frame, row_date_reading_without_person);
        return 0;
    }
    const auto clock = c.u32(manager_object + 0xc);
    if (clock == 0) return 0;
    const auto row = status_row(c, manager_object, person);
    return invoke(c, callee_text::clock_reading, 0x1018c24c, {clock, c.u32(row + 0x18), c.u32(row + 0x1c)});
}

std::int32_t status_degree_of(Cpu& c, std::uint32_t manager_object, std::uint32_t person) {
    // The original's second zero test (0x1018c278) repeats the first one's condition, which cannot hold.
    if (person == 0) return 0;
    if (c.i32(person) >= c.i32(c.toc(toc::people_count))) return 0;
    if (c.u32(person + 0x14) == 0) return 0;
    const auto row = status_row(c, manager_object, person);
    if (c.i8(row + 0x14) < 0) return 0;
    const auto degree = c.i8(row + 0x15);
    return degree > 1 ? degree : 1;
}

std::uint32_t status_kind_of(Cpu& c, std::uint32_t manager_object, std::uint32_t person) {
    if (person == 0) return 0xffffffff;
    if (c.i32(person) >= c.i32(c.toc(toc::people_count))) return 0xffffffff;
    if (c.u32(person + 0x14) == 0) return 0xffffffff;
    return c.u8(status_row(c, manager_object, person) + 0x14);
}

void injury_name_of_person(Cpu& c, std::uint32_t manager_object, std::uint32_t person, std::uint32_t buffer,
                           std::uint32_t size, std::uint32_t style) {
    Frame frame(c, 0x1018c450, 0x80, {.first_gpr = 25});
    const auto pool = c.toc(toc::pool);
    c.set({{26, person}, {30, c.toc(toc::message)}, {31, pool}, {25, manager_object}, {27, buffer}, {28, size}, {29, style}});
    if (person == 0) {
        report_check(c, frame, injury_name_without_person);
        return;
    }
    if (c.i32(person) >= c.i32(c.toc(toc::people_count))) return;
    invoke_unit(c, routine::prepare_status_row, 0x1018c4fc, {manager_object, person});
    const auto index = c.u32(status_row(c, manager_object, person) + status::injury);
    if (index == no_injury) {
        invoke(c, callee_text::format, 0x1018c528, {buffer, pool + pool_text::no_injury, pool + pool_text::empty});
        return;
    }
    const auto type = c.u8(c.u32(manager_object + manager::injuries) + index * injury::size);
    enter(c, 0x1018c54c, {manager_object, type, buffer, size, style});
    injury_name_text(c, manager_object, type, buffer, std::int32_t(size), style);
}

void injury_name_text(Cpu& c, std::uint32_t manager_object, std::uint32_t type, std::uint32_t buffer,
                      std::int32_t size, std::uint32_t style) {
    (void)manager_object;
    Frame frame(c, 0x1018c560, 0xd0, {.first_gpr = 26});
    const auto pool = c.toc(toc::pool);
    c.set({{26, type}, {27, buffer}, {28, style}, {29, c.toc(toc::message)}, {30, pool}, {31, c.toc(toc::success)}});
    if (size < 30) {
        report_check(c, frame, name_text_small_buffer);
        return;
    }
    const auto style_byte = std::int8_t(style);
    bool plain = true;
    if (style_byte != 0) plain = std::int8_t(invoke(c, callee_text::language, 0x1018c5f0)) == 3;
    const auto index = sign_extended(type);
    if (plain) {
        if (index > 0x51) {
            report_check(c, frame, name_text_plain_past_table);
            return;
        }
        invoke(c, callee_text::format, 0x1018c628 + 0x10 * index, {buffer, pool + plain_name_offsets[index]});
    } else if (style_byte == 1) {
        if (index > 0x51) {
            report_check(c, frame, name_text_alternate_past_table);
            return;
        }
        invoke(c, callee_text::format, 0x1018cbc8 + 0x10 * index, {buffer, pool + alternate_name_offsets[index]});
    }
}

std::int32_t language_flag(Cpu& c, std::uint32_t manager_object, std::uint32_t type) {
    (void)manager_object;
    Frame frame(c, 0x1018d140, 0x40, {.first_gpr = 31});
    c.set(31, type);
    const auto index = sign_extended(type);
    if (std::int8_t(invoke(c, callee_text::language, 0x1018d158)) == 9) return index < 0x4d ? language_9_values[index] : 1;
    if (std::int8_t(invoke(c, callee_text::language, 0x1018d3f8)) == 3) return index < 0x4d ? language_3_values[index] : 1;
    if (std::int8_t(invoke(c, callee_text::language, 0x1018d698)) == 5) return index < 0x4d ? language_5_values[index] : 1;
    return 0;
}

std::uint32_t time_left_text(Cpu& c, std::uint32_t manager_object, std::uint32_t person, std::uint32_t out,
                             std::uint32_t first, std::uint32_t which, std::uint32_t second) {
    Frame frame(c, 0x1018d950, 0x90, {.first_gpr = 24});
    c.set({{24, manager_object}, {25, person}, {26, c.toc(toc::message)}, {27, c.toc(toc::pool)}, {28, out},
           {29, first}, {30, which}, {31, second}});
    if (person == 0) {
        report_check(c, frame, time_left_without_person);
        return 0;
    }
    if (c.i32(person) >= c.i32(c.toc(toc::people_count))) return 0;
    invoke_unit(c, routine::prepare_status_row, 0x1018da0c, {manager_object, person});
    const auto index = c.u32(status_row(c, manager_object, person) + status::injury);
    if (index == no_injury) {
        c.put8(out, 0);
        return 0;
    }
    const auto record = injury_record(c, manager_object, index);
    c.set(26, record);
    invoke(c, callee_text::date_apart, 0x1018da58, {frame.at(0x38), c.toc(toc::today), c.u32(record + injury::started),
                                                    c.u32(record + injury::started + 4)});
    // The date is a halfword of days and a halfword of years, which the game counts as 365 days each.
    const std::uint32_t since = std::uint32_t(std::int32_t(c.i16(frame.at(0x38)))) + std::uint32_t(c.u16(frame.at(0x3a))) * 0x16d;
    const std::uint32_t length = std::int8_t(which) != 0 ? std::uint32_t(std::int32_t(c.i16(record + injury::days_out)))
                                                         : std::uint32_t(std::int32_t(c.i16(record + injury::days_unfit)));
    return invoke(c, callee_text::format_days, 0x1018da94, {length - since, out, first, second});
}

std::uint32_t form_text(Cpu& c, std::uint32_t manager_object, std::uint32_t form, std::uint32_t buffer,
                        std::int32_t size) {
    (void)manager_object;
    Frame frame(c, 0x1018dab0, 0xa0, {.first_gpr = 29});
    const auto pool = c.toc(toc::pool);
    c.set({{31, pool}, {30, c.toc(toc::message)}, {29, c.toc(toc::success)}});
    if (size < 20) {
        report_check(c, frame, form_text_small_buffer);
        return 0;
    }
    const auto index = sign_extended(form);
    if (index <= 12)
        invoke(c, callee_text::format, 0x1018db60 + 0x10 * index, {buffer, pool + form_name_offsets[index]});
    else
        report_check(c, frame, form_text_past_table);
    return std::int8_t(invoke(c, callee_text::language, 0x1018dc78)) == 3 ? 2 : 0;
}

std::uint32_t has_injury_81(Cpu& c, std::uint32_t person) {
    Frame frame(c, 0x1018f340, 0x70, {.first_gpr = 28});
    const auto pool = c.toc(toc::pool);
    const auto manager_object = c.u32(c.toc(toc_text::manager_cell));
    c.set({{31, pool}, {30, c.toc(toc::message)}, {29, manager_object}, {28, person}});
    std::uint32_t record = 0;
    if (person == 0) {
        report_check(c, frame, has_injury_without_person);
    } else if (c.i32(person) < c.i32(c.toc(toc::people_count))) {
        invoke_unit(c, routine::prepare_status_row, 0x1018f3f8, {manager_object, person});
        const auto index = c.u32(status_row(c, manager_object, person) + status::injury);
        if (index != no_injury) record = injury_record(c, manager_object, index);
    }
    return record != 0 && c.u8(record) == 0x51 ? 1 : 0;
}

void match_result_news(Cpu& c, std::uint32_t news, std::uint32_t sink) {
    Frame frame(c, 0x1018fbe0, 0x70, {.first_gpr = 25});
    const auto competitions_cell = c.toc(toc_text::competitions), message = c.toc(toc_text::news_buffer), pool = c.toc(toc::pool);
    const auto clubs = c.u32(c.toc(toc::clubs));
    const auto second_club = clubs + c.u32(news + news_record::second_club) * club_size;
    const auto first_club = clubs + c.u32(news + news_record::first_club) * club_size;
    const auto first_score = c.i8(news + news_record::first_score), second_score = c.i8(news + news_record::second_score);
    c.set({{25, news}, {26, sink}, {27, second_club}, {28, first_club}, {29, competitions_cell}, {30, message}, {31, pool}});
    const auto line = frame.at(0x40);
    // The score line names the higher score first; a draw writes the same format for both.
    std::uint32_t site = 0x1018fce0, format_one = 0x1bef, format_two = 0x1cfe, two_site = 0x1018fd14;
    std::uint32_t high = std::uint32_t(std::int32_t(first_score)), low = std::uint32_t(std::int32_t(second_score));
    if (first_score > second_score) {
        site = 0x1018fc48, format_two = 0x1c17, two_site = 0x1018fc7c;
    } else if (first_score < second_score) {
        site = 0x1018fc98, format_two = 0x1c87, two_site = 0x1018fccc;
        std::swap(high, low);
    }
    invoke(c, callee_text::format, site + 4, {line, pool + format_one, high, low, c.u8(news + news_record::second_score),
                                              c.u8(news + news_record::first_score)});
    const auto first_tag = sign_extended(c.u8(first_club + club_row::tag)), second_tag = sign_extended(c.u8(second_club + club_row::tag));
    c.put32(frame.at(0x38), second_club + club_row::name);
    invoke(c, callee_text::format, two_site + 4,
           {message, pool + format_two, first_tag, first_tag, second_tag, second_tag, first_club + club_row::name, line});
    invoke(c, callee_text::sink_line, 0x1018fd24, {sink, message});
    invoke(c, callee_text::sink_link, 0x1018fd38, {sink, first_club + club_row::name, 0x7d5, first_club});
    invoke(c, callee_text::sink_link, 0x1018fd4c, {sink, second_club + club_row::name, 0x7d5, second_club});
    constexpr std::uint32_t competition_size = 0x6c;
    const auto competition_index = c.u32(news + news_record::competition);
    const auto allowed = invoke(c, callee_text::match_check, 0x1018fd68,
                                {c.u32(c.toc(toc_text::match_state)), c.u32(competitions_cell) + competition_index * competition_size});
    if (std::int8_t(allowed) != 0) {
        const auto competitions = c.u32(competitions_cell);
        invoke(c, callee_text::format, 0x1018fd90,
               {message, pool + 0x1d70, competitions + c.u32(news + news_record::competition) * competition_size + 0x4d, competitions});
        invoke(c, callee_text::sink_append, 0x1018fd9c, {sink, message});
    }
    const auto linked = invoke(c, callee_text::match_link, 0x1018fdc0,
                               {c.u32(c.toc(toc_text::match_context)), c.u32(news + news_record::link),
                                c.u32(news + news_record::first_club), c.u32(news + news_record::second_club),
                                c.u16(news + 4), c.u16(news + 6)});
    if (std::int8_t(linked) != 0)
        invoke(c, callee_text::sink_link, 0x1018fddc, {sink, line, 0x7d3, c.u32(news + news_record::link)});
}

void record_row_date(Cpu& c, std::uint32_t manager_object, std::uint32_t person) {
    Frame frame(c, 0x1018fdf0, 0xa0, {.first_gpr = 27});
    c.set({{27, manager_object}, {28, person}, {29, c.toc(toc::success)}, {30, c.toc(toc::message)}, {31, c.toc(toc::pool)}});
    // A null person is reported twice, each time followed by the exit routine; the original then goes
    // on to read through the null pointer.
    if (person == 0) {
        report_check(c, frame, record_date_first_without_person);
        invoke(c, callee_text::exit_game, 0x1018fe70, {0xffffffff});
    }
    if (person == 0) {
        report_check(c, frame, record_date_second_without_person);
        invoke(c, callee_text::exit_game, 0x1018fed0, {0xffffffff});
    }
    const auto row = c.u32(manager_object) + c.u32(person) * status::size;
    invoke(c, callee_text::clock_copy, 0x1018feec, {c.u32(manager_object + 0xc), row + 0x18, row + 0x1c});
}

void recovery_news(Cpu& c, std::uint32_t manager_object, std::uint32_t news, std::uint32_t style, std::uint32_t sink) {
    Frame frame(c, 0x1018f470, 0x260, {.first_gpr = 16});
    const auto pool = c.toc(toc::pool), message = c.toc(toc_text::news_buffer);
    const auto people_cell = c.toc(toc::people), types = c.toc(toc::injury_types);
    c.set({{16, news}, {22, manager_object}, {30, message}, {17, style}, {19, types}, {25, sink}, {24, people_cell}, {31, pool}});
    const auto text = frame.at(0x178), date = frame.at(0x38), days_text = frame.at(0x74), first_name = frame.at(0xa8),
               second_name = frame.at(0xdc), short_name = frame.at(0x40);

    // The record's eleven fields, read in the original's order.
    const auto field = [&](unsigned index, std::uint32_t at) {
        return invoke(c, callee_text::field_of_news, at + 4, {news, index});
    };
    const auto outcome = std::int8_t(field(0, 0x1018f4a8));
    c.set(21, std::uint32_t(std::int32_t(outcome)));
    const auto type = sign_extended(field(1, 0x1018f4b8));
    c.set(18, type);
    const auto person = field(2, 0x1018f4c8);
    c.set(29, person);
    const auto key_a = field(3, 0x1018f4dc);
    c.set(28, key_a);
    const auto key_b = field(4, 0x1018f4f0);
    c.set(27, key_b);
    const auto key_c = field(5, 0x1018f504);
    c.set(26, key_c);
    const auto year_word = field(7, 0x1018f518) & 0xffff;
    c.set(20, year_word);
    const auto day_word = field(6, 0x1018f528) & 0xffff;
    invoke(c, callee_text::make_date, 0x1018f540, {date, std::uint32_t(std::int32_t(std::int16_t(day_word))), year_word});
    const auto days = std::uint32_t(std::int32_t(std::int16_t(field(8, 0x1018f548))));
    c.set(23, days);
    const auto detail = std::int32_t(field(9, 0x1018f558));
    c.set(20, std::uint32_t(detail));
    const auto result = std::int32_t(field(10, 0x1018f568));
    c.set(21, std::uint32_t(result));

    const bool full = std::int8_t(style) != 0;
    const auto type_value = std::int32_t(type);
    const auto class_byte = c.u8(types + type * 12 + 1);
    const auto class_value = std::int8_t(class_byte);

    // The formatter writes into the news text; `more` are r5 onwards.
    const auto say = [&](std::uint32_t at, std::uint32_t offset, std::initializer_list<std::uint32_t> more) {
        unsigned n = 5;
        c.set({{3, message}, {4, pool + offset}});
        for (const auto argument : more) c.set(n++, argument);
        c.call(callee_text::format, at + 4);
    };
    const auto person_text = [&](std::uint32_t at, std::uint32_t buffer, std::uint32_t mode) {
        invoke(c, callee_text::person_text, at + 4, {key_a, key_b, key_c, buffer, 0x99, mode, person});
    };
    const auto line = [&](std::uint32_t at) { invoke(c, callee_text::sink_line, at + 4, {sink, message}); };
    // The name of the person, attached when the game allows it.
    const auto attach_name = [&](std::uint32_t check_at, std::uint32_t link_at) {
        const auto offset = person * person_size;
        c.set(16, offset);
        const auto allowed = invoke(c, callee_text::person_allowed, check_at + 4,
                                    {c.u32(people_cell) + offset, key_a, key_b, key_c, date});
        if (std::int8_t(allowed) != 0)
            invoke(c, callee_text::sink_link, link_at + 4, {sink, text, 0x7d7, c.u32(people_cell) + offset});
    };
    // The text of the injury's name and, for a form of its own, the flag the line is built with.
    const auto named_line = [&](std::uint32_t buffer, std::uint32_t form_at, std::uint32_t name_at, std::uint32_t flag_at,
                                std::uint32_t say_at, std::uint32_t offset) {
        std::uint32_t flag;
        if (class_value != 6) {
            enter(c, form_at + 4, {manager_object, class_byte, buffer, 0x32});
            flag = form_text(c, manager_object, class_byte, buffer, 0x32);
        } else {
            enter(c, name_at + 4, {manager_object, type, buffer, 0x32, 0});
            injury_name_text(c, manager_object, type, buffer, 0x32, 0);
            enter(c, flag_at + 4, {manager_object, type});
            flag = std::uint32_t(language_flag(c, manager_object, type));
        }
        const auto flag_value = sign_extended(flag);
        say(say_at, offset, {flag_value, flag_value, text, buffer});
    };

    if (outcome == 0) {
        if (!full) {
            person_text(0x1018f59c, text, 5);
            say(0x1018f5ac, 0x9e0, {text});
            line(0x1018f5b8);
            attach_name(0x1018f5d8, 0x1018f5f8);
            return;
        }
        person_text(0x1018f61c, text, 1);
        if (class_value == 12) {
            if (type_value == 0x51) say(0x1018f64c, 0xa1b, {text});
            else if (type_value == 0x50) say(0x1018f668, 0xa83, {text});
            else if (type_value == 0x4d) say(0x1018f684, 0xb7e, {text});
            else if (type_value == 0x4e) say(0x1018f6a0, 0xbf4, {text});
            else if (type_value == 0x4f) say(0x1018f6bc, 0xc79, {text});
            else invoke(c, callee_text::format_wide, 0x1018f6d4, {message, pool + 0xd2e, text});
        } else if (class_value == 0) {
            if (type_value == 0) say(0x1018f6f4, 0xd91, {text});
            else if (type_value == 1) say(0x1018f710, 0xde5, {text});
            else if (type_value == 2) say(0x1018f72c, 0xe40, {text});
            else say(0x1018f740, 0xe9f, {text});
        } else {
            named_line(first_name, 0x1018f75c, 0x1018f778, 0x1018f784, 0x1018f7a0, 0xf05);
        }
        line(0x1018f7ac);
        invoke(c, callee_text::format_days, 0x1018f7c4, {days, days_text, 0x32, 1});
        say(0x1018f7d0, 0xf7c, {days_text});
        invoke(c, callee_text::sink_append, 0x1018f7e0, {sink, message});
        attach_name(0x1018f7fc, 0x1018f81c);
        return;
    }

    if (!full) {
        person_text(0x1018f848, text, 5);
        if (type_value == 0x51 && result == 3) say(0x1018f868, 0xfd2, {text});
        else say(0x1018f87c, 0x101d, {text});
        line(0x1018f888);
        attach_name(0x1018f8a8, 0x1018f8c8);
        return;
    }
    person_text(0x1018f8ec, text, 1);
    person_text(0x1018f90c, second_name, 5);
    if (class_value == 12) {
        if (type_value == 0x51) {
            if (result == 2) say(0x1018f944, 0x1057, {text});
            else if (result == 3) say(0x1018f960, 0x110a, {text});
            else say(0x1018f974, 0xa1b, {text});
        } else if (type_value == 0x4d) {
            if (result == -1) say(0x1018f998, 0x115e, {text});
            else if (result == 1) say(0x1018f9b4, 0x1277, {text});
            else if (detail == 0) say(0x1018f9d0, 0x139e, {text});
            else say(0x1018f9e4, 0x1467, {text});
        } else if (type_value == 0x50) {
            say(0x1018fa00, 0x1522, {text});
        } else if (type_value == 0x4e) {
            if (detail == 0) say(0x1018fa24, 0x1617, {text});
            else say(0x1018fa3c, 0x1696, {text, second_name});
        } else if (type_value == 0x4f) {
            if (detail == 2) {
                if (result == 1) say(0x1018fa68, 0x1796, {text});
                else say(0x1018fa7c, 0x1866, {text});
            } else {
                say(0x1018fa90, 0x18cc, {text});
            }
        } else {
            say(0x1018faa4, 0x1994, {text});
        }
    } else if (class_value == 0) {
        if (type_value == 0) say(0x1018fac8, 0x1a18, {text});
        else if (type_value == 1) say(0x1018fae4, 0x1a68, {text});
        else if (type_value == 2) say(0x1018fb00, 0x1abf, {text});
        else say(0x1018fb14, 0x1b1a, {text});
    } else {
        named_line(short_name, 0x1018fb30, 0x1018fb4c, 0x1018fb58, 0x1018fb74, 0x1b7c);
    }
    line(0x1018fb80);
    attach_name(0x1018fba0, 0x1018fbc0);
}
}
