#include "contract_screens.hpp"
#include "contract_screens_controls.hpp"
#include "screen_manager_detail.hpp"
#include <array>

namespace cm::aot::contract_screens {
using namespace guest;

namespace {
using screen_manager::detail::Manager;
using screen_manager::detail::Program;
constexpr auto history = 0x1073ef70u, primary_contract = 0x10082f20u, current_draft = 0x101487d0u,
               format = 0x1077b720u, localized = 0x10727890u, add_grid = 0x107198c0u;
constexpr auto leave_negotiation = std::uint32_t(-10), close_details = std::uint32_t(-9), redraw = std::uint32_t(-11);

int signed_field(Cpu& c, std::uint8_t byte, unsigned shift) {
    c.s.wr<1>(reg::xer_ca, (byte & 0x80) != 0 && (byte & (1u << (shift - 1))) != 0);
    const auto value = std::int8_t(byte) >> shift;
    c.set(0, std::uint32_t(value));
    return value;
}

struct ContractLookup {
    Cpu& c;
    Program program;
    std::uint32_t holder, cache, person;

    std::uint32_t primary(std::uint32_t back) const {
        return program.call(primary_contract, back, {c.u32(holder), person});
    }

    int kind(std::uint32_t record) const {
        return signed_field(c, c.u8(record + 0x38), 2);
    }

    std::uint32_t employer(std::uint32_t back, unsigned kept, bool keep_table_holder,
                           unsigned club_register, std::uint32_t club_table) const {
        c.set(kept, 0);
        const auto record = primary(back);
        const bool found = compare_logical(c, record) != 0;
        c.put32(cache, record);
        const bool has_club = found && compare(c, kind(record), 5) != 0;
        c.set(kept, has_club);
        compare(c, has_club);
        std::uint32_t club = 0;
        if (has_club) {
            const auto saved = c.u32(cache), offset = c.u32(saved + 4) * 0x248;
            const auto clubs = c.u32(club_table);
            if (keep_table_holder) c.set({{3, clubs}, {4, club_table}, {0, offset}});
            else c.set({{3, saved}, {4, clubs}, {0, offset}});
            club = clubs + offset;
        }
        c.set(club_register, club);
        return club;
    }
};

struct DetailCommand {
    Cpu& c;
    const Frame& frame;
    Program program;
    std::uint32_t screens, contracts, offers, cache, clubs, today;
    std::uint32_t draft = 0, person = 0, instance = 0, link = 0;
    std::uint32_t offer_id = 0xffffffff, revision = 0xffffffff;
    int role = 0, mode = 0, option = 0;

    std::uint32_t read(std::uint32_t back, unsigned slot) const {
        return program.call(history, back, {screens, slot});
    }

    std::uint32_t destination() const { return c.u32(instance + 0x24); }

    bool refresh();
    void prepare_proposal(const ContractLookup& lookup);
    void mark_participant() const;
};

bool DetailCommand::refresh() {
    const auto existing = c.u32(link);
    c.set(16, existing);
    offer_id = read(0x1009e3d8, 1);
    c.set({{0, offer_id}, {22, offer_id}});
    revision = read(0x1009e3ec, 2);
    c.set(21, revision);
    const auto category = c.u8(draft + 0x60) & 0xf;
    if (category >= 2 && category < 6)
        c.put8(draft + 0x44, category == 2 ? 75 : category == 3 ? 50 : 0);
    if (!detail::offer_changed(c, existing, revision)) return false;
    program.call(0x10411f10, 0x1009e46c, {c.u32(link), 1});
    c.set(0, 0);
    c.put32(link, 0);
    program.call(0x10422570, 0x1009e480, {existing, revision});
    return true;
}

void DetailCommand::prepare_proposal(const ContractLookup& lookup) {
    const auto employer = lookup.employer(0x1009e498, 30, false, 0, clubs);
    const auto club = destination();
    c.set(5, club);
    if (compare_logical(c, club, employer) == 0) return;

    const auto date = frame.at(0x38), proposal = frame.at(0x40), flags = frame.at(0x78);
    const auto initial_year = c.u16(today), initial_day = c.u16(today + 2);
    const auto initial_flags = c.u8(today + 4);
    c.put16(date, initial_year);
    c.put16(date + 2, initial_day);
    c.put8(date + 4, initial_flags);
    c.set({{0, initial_flags}, {9, initial_day}});
    program.call(0x10411ea0, 0x1009e520, {proposal, person, club, 0, 0xffffffff, 1});
    c.put8(frame.at(0x51), std::uint32_t(role));

    const auto player = std::int8_t(program.call(0x100c7a20, 0x1009e52c, {c.u8(person + 0x62)}));
    c.set(0, std::uint32_t(player));
    if (compare(c, player) != 0 && compare_logical(c, lookup.employer(0x1009e544, 21, false, 0, clubs)) != 0) {
        c.set(0, std::uint32_t(mode));
        const bool has_mode = compare(c, mode) != 0;
        if (has_mode) c.set(0, std::uint32_t(option));
        if (has_mode && compare(c, option) == 0) {
            const auto value = program.call(0x103fac70, 0x1009e5bc, {c.u32(offers), person, destination(), 0});
            const auto changed = (c.u8(flags) & 0xf) | 0x50u;
            c.set({{0, changed}, {4, 5}});
            c.put32(frame.at(0x60), value);
            c.put8(flags, changed);
        } else if (compare(c, lookup.kind(lookup.primary(0x1009e5e0)), 1) != 0) {
            const auto changed = (c.u8(flags) & 0xf) | 0x30u;
            c.set(0, changed);
            c.put8(flags, changed);
            const auto primary = lookup.primary(0x1009e614);
            c.put16(date + 2, c.u16(primary + 0x32));
            c.put16(date, c.u16(primary + 0x30));
            const auto date_flags = c.u8(primary + 0x34);
            c.set(0, date_flags);
            c.put8(date + 4, date_flags);
        }
    }

    const auto profile = c.u32(person + 0x14);
    c.set(0, profile);
    if (compare_logical(c, profile) != 0 && compare(c, signed_field(c, c.u8(flags), 4), 5) != 0 &&
        (compare(c, role, 11) == 0 || compare(c, role, 13) == 0 || compare(c, role, 15) == 0)) {
        const auto player_club = lookup.employer(0x1009e678, 18, false, 7, clubs);
        const auto protected_player = std::int8_t(program.call(0x103f8820, 0x1009e6e0,
            {c.u32(offers), destination(), date, person, player_club, 0, 0}));
        c.set(0, std::uint32_t(protected_player));
        if (compare(c, protected_player) == 0) {
            const auto raw = c.u8(flags);
            c.set(3, raw);
            const auto category = compare(c, signed_field(c, raw, 4)) == 0 ? 1u : 4u;
            const auto changed = (raw & 0xf) | (category << 4);
            c.set({{0, category}, {3, changed}});
            c.put8(flags, changed);
        }
    }
    c.put8(frame.at(0x75), read(0x1009e724, 3));
    program.call(0x103e25d0, 0x1009e738, {c.u32(offers), proposal, 0});
    offer_id = c.u32(frame.at(0x68));
    revision = c.u32(frame.at(0x6c));
    c.set({{22, offer_id}, {21, revision}});
    program.call(0x10411f10, 0x1009e74c, {proposal, 0xffffffff});
}

void DetailCommand::mark_participant() const {
    if (compare_logical(c, link) == 0) return;
    const auto offer = c.u32(link), wanted = c.u32(draft), participant = c.u32(offer + 0xc);
    c.set({{0, wanted}, {3, participant}, {4, offer}});
    if (compare(c, participant, wanted) == 0) {
        c.set(0, 1);
        c.put8(offer + 0x10, 1);
        return;
    }
    const auto entries = c.u32(offer + 4), start = c.u32(entries + 4);
    c.set({{0, start}, {3, entries}, {16, 1}});
    c.put32(entries + 0xc, start);
    for (;;) {
        const auto item = program.call(0x10738e00, 0x1009e7d0, {c.u32(c.u32(link) + 4)});
        if (compare_logical(c, item) == 0) break;
        const auto id = c.u32(item), target = c.u32(draft);
        c.set({{0, target}, {4, id}});
        if (compare(c, id, target) == 0) c.put8(item + 4, 1);
    }
}
}

std::uint32_t offer_details_command(Cpu& c, std::uint32_t pressed) {
    Frame frame(c, 0x1009e290, 0xe0, {.first_gpr = 16});
    DetailCommand action{c, frame, Program{c}, c.toc(-0x7bb0), c.toc(-0x7b7c),
                         c.toc(-0x7b40), c.toc(-0x7b78), c.toc(-0x7be8), c.toc(-0x7b80)};
    const auto screens = action.screens;
    keep(c, {{16, pressed}, {23, action.cache}, {24, action.today}, {25, action.offers}, {26, action.contracts},
             {27, action.clubs}, {28, screens}, {29, screens + 0x2c58}});
    const Manager manager{c, screens};
    const auto group = manager.group(), layout = c.u32(group + 0x24);
    c.set({{3, group}, {4, layout}});
    const detail::Controls controls{c, screens, layout, std::int16_t(pressed)};
    if (compare(c, controls.action(3), 0x14) != 0) {
        if (compare(c, controls.action(3), 0x15) == 0) return close_details;
        const auto handled = action.program.call(0x102a54b0, 0x1009e83c, {pressed});
        if (compare(c, handled) != 0) return handled;
        const auto common = action.program.call(0x102a83a0, 0x1009e850, {pressed});
        c.s.wr<1>(reg::xer_ca, common != 0 && common != 0x80000000);
        return common;
    }

    c.set({{21, 0xffffffff}, {22, 0xffffffff}});
    action.draft = action.program.call(current_draft, 0x1009e32c, {c.u32(c.toc(-0x7a54)), 0});
    const auto people = c.u32(c.toc(-0x7be0)), offset = c.u32(action.draft) * 0x70;
    const auto active_group = manager.group();
    action.instance = c.u32(active_group + 4);
    action.person = people + offset;
    c.set({{0, offset}, {5, people}, {6, active_group}, {16, action.person}, {17, action.instance}, {31, action.draft}});
    const auto read_byte = [&](std::uint32_t back, unsigned slot, unsigned kept) {
        const auto value = std::int8_t(action.read(back, slot));
        c.set(kept, std::uint32_t(value));
        return value;
    };
    action.role = read_byte(0x1009e360, 4, 20);
    const auto withdraw = read_byte(0x1009e370, 5, 18);
    action.option = read_byte(0x1009e380, 6, 19);
    const auto mode = std::int8_t(action.read(0x1009e390, 7));
    c.set({{0, std::uint32_t(withdraw)}, {18, std::uint32_t(mode)}});
    action.mode = mode;
    if (compare(c, withdraw) != 0) {
        action.program.call(0x1007f840, 0x1009e3ac, {c.u32(action.contracts), action.person, action.destination()});
        return leave_negotiation;
    }

    action.link = action.read(0x1009e3c0, 0);
    c.set(29, action.link);
    if (compare(c, action.link) != 0) {
        if (action.refresh()) return redraw;
    } else {
        action.prepare_proposal({c, action.program, action.contracts, action.cache, action.person});
    }
    action.program.call(0x1007f200, 0x1009e770, {c.u32(action.contracts), action.draft, action.offer_id, action.revision,
                         std::uint32_t(action.role), 0, 1, 0});
    action.mark_participant();
    c.set(0, 0xffffffff);
    c.put32(action.draft, 0xffffffff);
    return leave_negotiation;
}

namespace {
struct DetailScreen {
    Cpu& c;
    const Frame& frame;
    Program program;
    std::uint32_t screens, strings, text, ink, border, offers, club, person, employer;
    int withdraw, option, mode;

    std::uint32_t read(std::uint32_t back, unsigned slot) const {
        return program.call(history, back, {screens, slot});
    }

    void words(std::initializer_list<std::uint32_t> values) const {
        auto at = frame.at(0x38);
        for (auto value : values) {
            c.put32(at, value);
            at += 4;
        }
    }

    std::uint32_t layout() const {
        const auto offset = std::uint32_t(c.i16(screens + 0x2c58)) * 0x2c0;
        c.set(0, offset);
        return c.u32(screens + offset + 0x24);
    }

    std::int32_t compensation() const;
    void draw_heading() const;
    void append_notes() const;
    void draw_message(std::int32_t fee, int rows) const;
    void draw_controls() const;
};

void DetailScreen::draw_heading() const {
    const auto box = frame.at(0x88), name = frame.at(0x1dc);
    program.call(0x107136d0, 0x1009ddb0, {person, 0, 0});
    program.call(0x100c7a50, 0x1009ddc4, {club, 1, frame.at(0x66), frame.at(0x64)});
    program.call(0x100caeb0, 0x1009ddd8, {person, name, 300, 1});
    const auto affiliation = c.u32(person + 0x24);
    c.set(6, affiliation);
    if (compare_logical(c, affiliation) != 0) {
        program.call(format, 0x1009ddf8, {text, strings + 0x215, name, affiliation + 0x221});
        program.call(0x1074b830, 0x1009de04, {box, text});
        program.call(0x1074bab0, 0x1009de1c, {box, c.u32(person + 0x24) + 0x221, 0x7d5, club});
    } else {
        program.call(0x1077dbf0, 0x1009de2c, {text, name});
        program.call(0x1074b830, 0x1009de38, {box, text});
    }
    c.set(25, screens + 0x24);
    words({0, 0x30, c.u16(frame.at(0x64)), 0xffffffff});
    const auto grid = program.call(add_grid, 0x1009de88, {layout(), 100, 10, 790, 70, 1, 0, 1});
    c.set(0, 0);
    c.put32(frame.at(0x6c), 0);
    words({0});
    program.call(0x1074bcd0, 0x1009deb8, {box, grid, 100, 790, frame.at(0x6c), 7, c.u16(frame.at(0x66)), 1});
    c.set(0, std::uint32_t(withdraw));
    const bool cancellation = compare(c, withdraw) != 0;
    program.call(localized, cancellation ? 0x1009decc : 0x1009dedc,
                 {text, strings + (cancellation ? 0x127e : 0x21d)});
    program.call(0x107442f0, 0x1009dee8, {screens, text});
}

std::int32_t DetailScreen::compensation() const {
    if (compare_logical(c, employer) == 0 || compare_logical(c, employer, club) == 0 ||
        compare_logical(c, read(0x1009df18, 0)) != 0) return 0;
    c.set(0, std::uint32_t(mode));
    bool full = compare(c, mode) == 0;
    if (!full) {
        c.set(0, std::uint32_t(option));
        full = compare(c, option) != 0;
    }
    const auto fee = std::int32_t(program.call(0x103fac70, 0x1009df4c, {c.u32(offers), person, club, full}));
    c.set(23, std::uint32_t(fee));
    if (compare(c, fee) > 0) {
        c.put32(frame.at(0x68), std::uint32_t(fee));
        const auto amount = program.call(0x10009f20, 0x1009df64, {frame.at(0x68), 6});
        program.call(0x1000b410, 0x1009df7c, {frame.at(0x114), 200, amount, c.r(4), 6});
    }
    return fee;
}

void DetailScreen::append_notes() const {
    const auto note = frame.at(0x308);
    const auto append = [&](std::uint32_t back) {
        program.call(0x1077dc60, back, {text, note});
    };
    c.set(0, std::uint32_t(mode));
    if (compare(c, mode) == 0) return;
    c.set(0, std::uint32_t(option));
    if (compare(c, option) != 0) {
        c.set(0, std::uint32_t(mode));
        compare(c, mode);
        if (compare(c, option, 3) == 0) {
            program.call(localized, 0x1009e0ac, {note, strings + 0x1406});
            append(0x1009e0b8);
        }
        return;
    }
    const auto date = frame.at(0x80), today = frame.at(0x70), calendar = c.toc(-0x7b80);
    program.call(0x103fb130, 0x1009dffc, {c.u32(offers), date, 0});
    c.put32(today, c.u32(calendar));
    const auto today_tail = c.u32(calendar + 4);
    c.put32(today + 4, today_tail);
    const auto day = c.u16(date + 2), current_day = c.u16(today + 2);
    c.set({{0, day}, {3, current_day}, {4, 0}, {5, today_tail}});
    bool same = compare_logical(c, current_day, day) == 0;
    if (same) {
        const auto year = c.i16(date), current_year = c.i16(today);
        c.set({{0, std::uint32_t(year)}, {3, std::uint32_t(current_year)}});
        same = compare(c, current_year, year) == 0;
    }
    c.set(4, same);
    if (compare(c, same) == 0) {
        const auto date_text = frame.at(0xb0);
        program.call(0x10716eb0, 0x1009e054, {date, date_text, 100, 255});
        program.call(localized, 0x1009e064, {note, strings + 0x1374, date_text});
        append(0x1009e070);
    } else {
        program.call(localized, 0x1009e080, {note, strings + 0x13b4});
        append(0x1009e08c);
    }
}

void DetailScreen::draw_message(std::int32_t fee, int rows) const {
    const auto name = frame.at(0x1dc);
    c.set(0, std::uint32_t(withdraw));
    if (compare(c, withdraw) != 0)
        program.call(localized, 0x1009df94, {text, strings + 0x1296, name});
    else {
        program.call(localized, 0x1009dfa8, {text, strings + 0x12dd, name});
        if (compare(c, fee) != 0) {
            const auto gender = std::uint32_t(c.i8(employer + 0x23b));
            const auto fee_note = frame.at(0x308);
            program.call(localized, 0x1009dfd0, {fee_note, strings + 0x1317, gender, gender, employer + 0x221, frame.at(0x114)});
            program.call(0x1077dc60, 0x1009dfdc, {text, fee_note});
        }
    }
    append_notes();
    c.set(0, 0);
    const auto color = c.u16(ink);
    const bool include_club = compare(c, fee) != 0 && compare_logical(c, employer) != 0;
    if (include_club) {
        words({color, text, name, 0x7da, person, employer + 0x221, 0x7d5, employer, 0});
        c.set({{11, 0x7d5}, {12, employer + 0x221}});
    } else {
        words({color, text, name, 0x7da, person, 0});
        c.set(11, 0x7da);
    }
    program.call(0x1074a350, include_club ? 0x1009e128 : 0x1009e178, {110, 140, 780, 545, 2, 0, std::uint32_t(rows), 3});
}

void DetailScreen::draw_controls() const {
    program.call(0x102a1070, 0x1009e184, {4, 0});
    words({0, 1, c.u16(frame.at(0x64)), 0xffffffff});
    const auto grid = program.call(add_grid, 0x1009e1d0, {layout(), 100, 555, 790, 590, 2, frame.at(0x60), 1});
    c.set(17, grid);
    struct Button { std::uint32_t text_back, button_back; unsigned label, action; };
    constexpr std::array buttons{Button{0x1009e1e0, 0x1009e220, 0x80, 0x15}, Button{0x1009e22c, 0x1009e26c, 0x83, 0x14}};
    unsigned column = 0;
    for (const auto& entry : buttons) {
        program.call(localized, entry.text_back, {text, strings + entry.label});
        c.set(0, 0);
        words({c.u16(ink), text, entry.action, 0});
        program.call(0x107444e0, entry.button_back, {screens, grid, column++, 0, 0x30, c.u16(border), 12, 3});
    }
}
}

void draw_offer_details(Cpu& c) {
    Frame frame(c, 0x1009dc00, 0x540, {.first_gpr = 16});
    Program program{c};
    const auto screens = c.toc(-0x7bb0), strings = c.toc(-0x56cc), text = c.toc(-0x7bb8),
               ink = c.toc(-0x7bc8), border = c.toc(-0x7bc4), cache = c.toc(-0x7b78), offers = c.toc(-0x7b40);
    keep(c, {{16, border}, {18, offers}, {19, c.toc(-0x7ba8)}, {20, cache},
             {28, ink}, {29, strings}, {30, text}, {31, screens}});
    const auto foreground = c.u16(ink), accent = c.u16(c.toc(-0x7b14)), style = c.u16(c.r(2) - 0x39f4);
    c.put16(frame.at(0x66), accent);
    c.put16(frame.at(0x64), foreground);
    c.put16(frame.at(0x60), style);
    c.set({{0, style}, {5, foreground}, {6, accent}});
    const auto box = frame.at(0x88);
    program.call(0x1074b6e0, 0x1009dc58, {box, 0});
    c.set(23, 0);
    program.call(0x10715880, 0x1009dc64, {frame.at(0x80)});
    const auto draft = program.call(current_draft, 0x1009dc74, {c.u32(c.toc(-0x7a54)), 0});
    const auto group = Manager{c, screens}.group(), instance = c.u32(group + 4), club = c.u32(instance + 0x24);
    const auto player_id = c.u32(draft);
    c.set({{0, player_id}, {3, instance}, {17, club}, {26, screens + 0x2c58}});
    if (compare(c, player_id, -1) == 0) {
        report_assertion(c, {0x1009dcac, strings + 0x87, strings + 0x9c, strings + 0xa9,
                             frame.at(0x90), frame.at(0x78), 0x1197});
        c.set({{0, 0}, {5, c.toc(-0x7bac)}});
        program.call(0x1074b7c0, 0x1009dcf8, {box, 0xffffffff});
        return;
    }
    const auto offset = player_id * 0x70, person = c.u32(c.toc(-0x7be0)) + offset;
    c.set({{0, offset}, {24, person}});
    const auto employer = ContractLookup{c, program, c.toc(-0x7b7c), cache, person}
        .employer(0x1009dd20, 19, true, 27, c.toc(-0x7be8));
    DetailScreen screen{c, frame, program, screens, strings, text, ink, border, offers, club, person, employer, 0, 0, 0};
    const auto read_byte = [&](std::uint32_t back, unsigned slot, unsigned kept) {
        const auto value = std::int8_t(screen.read(back, slot));
        c.set(kept, std::uint32_t(value));
        return value;
    };
    screen.withdraw = read_byte(0x1009dd7c, 5, 20);
    screen.option = read_byte(0x1009dd8c, 6, 22);
    screen.mode = read_byte(0x1009dd9c, 7, 21);
    screen.draw_heading();
    const auto line_height = program.call(0x1076d470, 0x1009def0, {3});
    const auto count = std::uint32_t(divide_signed<4>(405, line_height));
    const auto rows = std::int8_t(count);
    c.set({{0, count}, {19, std::uint32_t(rows)}});
    const auto fee = screen.compensation();
    screen.draw_message(fee, rows);
    screen.draw_controls();
    program.call(0x1074b7c0, 0x1009e278, {box, 0xffffffff});
}
}
