#include "contract_screens.hpp"
#include "contract_screens_controls.hpp"
#include "guest_fpu.hpp"
#include "screen_manager_detail.hpp"

namespace cm::aot::contract_screens {
using namespace guest;

namespace {
using screen_manager::detail::Manager;
using screen_manager::detail::Program;

constexpr std::uint32_t read_history = 0x1073ef70, write_history = 0x1073f1e0,
                        widget_state = 0x10740620, widget_style = 0x10741450,
                        widget_action = 0x10742100, widget_text = 0x10741da0,
                        refresh_widgets = 0x10742ec0, localized = 0x10727890,
                        money_number = 0x10009f20, more_money = 0x1000a060, less_money = 0x1000a170;
constexpr std::uint32_t redraw = -11u;
constexpr unsigned wage_field = 8, signing_fee_field = 0xc, length_flags = 0x38;

struct BasicCommand {
    Cpu& c;
    const Frame& frame;
    Program call;
    std::uint32_t screens, strings, text, calendar, contract_manager, finance, draft;
    std::uint32_t person = 0, proposal = 0, wage_limit = 0, linked_offer = 0xffffffff, employer_layout = 0;
    std::int32_t first_month = 0, first_year = 0, month = 0, year = 0;

    std::uint32_t read(std::uint32_t back, unsigned slot) const {
        return call.call(read_history, back, {screens, slot});
    }
    void write(std::uint32_t back, unsigned slot, std::uint32_t value) const {
        call.call(write_history, back, {screens, slot, value, 0});
    }
    void edit_widget(std::uint32_t back, std::uint32_t widget, unsigned state, unsigned style, unsigned action,
                     std::uint32_t state_service = widget_state) const {
        call.call(state_service, back, {screens, widget, state});
        call.call(widget_style, back + 0x10, {screens, widget, style});
        call.call(widget_action, back + 0x20, {screens, widget, action});
    }
    void refresh(std::uint32_t back) const { call.call(refresh_widgets, back, {screens, 0, 0}); }

    void update_wage_text(std::uint32_t back, std::uint32_t widget, std::uint32_t value) const;
    void change_fee(bool raise) const;
    void change_wage(bool raise) const;
    void set_wage(std::uint32_t value) const {
        c.put32(draft + wage_field, value);
        const auto stored = c.u32(draft + wage_field);
        c.set(0, stored);
        if (compare(c, stored, wage_limit) > 0) c.put32(draft + wage_field, wage_limit);
    }

    std::uint32_t term_flags() const { return c.u8(draft + length_flags); }
    std::int32_t term_kind(std::uint32_t flags) const {
        const auto kind = (std::int32_t(flags << 24) & std::int32_t(0xfe000000)) >> 26;
        c.s.wr<1>(reg::xer_ca, (flags & 0x82) == 0x82);
        return kind;
    }
    void set_term(std::uint32_t flags, unsigned kind) const {
        c.put8(draft + length_flags, (flags & 3) | ((kind & 0x3f) << 2));
    }
    void duration_text(std::uint32_t month_back, std::uint32_t text_back, std::uint32_t widget_back,
                       std::uint32_t widget, unsigned display_year, bool keep_year = false) const;
    void duration_upper(std::uint32_t off, std::uint32_t on, std::uint32_t widget, unsigned display_year) const;
    void write_duration(std::uint32_t month_back, std::uint32_t year_back) const;
    void toggle_duration() const;
    void increase_duration();
    void decrease_duration();
    void reset_duration() const;

    std::uint32_t employer(std::uint32_t back) const;
    double salary_as_double(std::uint32_t salary, unsigned constant_register) const;
    void cycle_role() const;
    void cycle_status() const;
};

void BasicCommand::update_wage_text(std::uint32_t back, std::uint32_t widget, std::uint32_t value) const {
    const auto number = call.call(money_number, back, {value, 6});
    call.call(0x1000b410, back + 0x18, {text, 200, number, c.r(4), 6});
    if (compare_logical(c, call.call(0x1077dbd0, back + 0x20, {text}), 8) > 0) {
        const auto short_number = call.call(money_number, back + 0x34, {value, 5});
        call.call(0x1000b410, back + 0x4c, {text, 200, short_number, c.r(4), 5});
    }
    call.call(widget_text, back + 0x5c, {screens, widget + 1, text});
}

void BasicCommand::change_fee(bool raise) const {
    if (raise) {
        read(0x1009a660, 8);
        call.call(0x10380560, 0x1009a668, {draft});
        c.put32(frame.at(0x44), read(0x1009a674, 0x18));
        const auto widget = extended(std::int16_t(read(0x1009a684, 0xf)));
        c.set(15, widget);
        edit_widget(0x1009a698, widget, 2, 12, 5);
        const auto fee = c.u32(draft + signing_fee_field);
        c.set(0, fee);
        c.put32(frame.at(0x40), fee);
        call.call(0x10372590, 0x1009a6d4, {draft, text, 0, 2});
        call.call(widget_text, 0x1009a6e4, {screens, widget + 1, text});
        call.call(more_money, 0x1009a6f0, {frame.at(0x40), 8});
        const auto cap_high = call.call(money_number, 0x1009a6fc, {frame.at(0x44), 8});
        const auto cap_low = c.r(4);
        c.set({{13, cap_high}, {14, cap_low}});
        const auto next_high = call.call(money_number, 0x1009a710, {frame.at(0x40), 8}), next_low = c.r(4);
        const auto cap = std::int64_t((std::uint64_t(cap_high) << 32) | cap_low);
        const auto next = std::int64_t((std::uint64_t(next_high) << 32) | next_low);
        c.s.wr<1>(reg::xer_ca, cap >= next);
        c.set({{0, cap_low - next_low}, {3, cap < next}, {5, cap_high ^ 0x80000000}});
        const bool allowed = compare(c, c.r(3)) == 0;
        edit_widget(allowed ? 0x1009a770 : 0x1009a73c, widget + 2, allowed ? 2 : 6,
                    allowed ? 12 : 44, allowed ? 4 : 0, widget_action);
        refresh(0x1009a7a0);
        return;
    }

    call.call(0x10380630, 0x1009a7b0, {draft});
    const auto widget = extended(std::int16_t(read(0x1009a7bc, 0xf)));
    const auto fee = c.u32(draft + signing_fee_field);
    c.set({{0, fee}, {13, widget}});
    const bool allowed = compare(c, fee, 100) > 0;
    edit_widget(allowed ? 0x1009a810 : 0x1009a7dc, widget, allowed ? 2 : 6, allowed ? 12 : 44,
                allowed ? 5 : 0, widget_action);
    call.call(0x10372590, 0x1009a844, {draft, text, 0, 2});
    call.call(widget_text, 0x1009a854, {screens, widget + 1, text});
    edit_widget(0x1009a864, widget + 2, 2, 12, 4);
    refresh(0x1009a894);
}

void BasicCommand::change_wage(bool raise) const {
    const auto amount = frame.at(0x48);
    call.call(raise ? more_money : less_money, raise ? 0x1009a948 : 0x1009aa9c, {amount, 4});
    const auto adjusted = c.u32(amount);
    c.set(0, adjusted);
    c.put32(draft + wage_field, adjusted);
    const auto widget = extended(std::int16_t(read(raise ? 0x1009a95c : 0x1009aab0, 0x10)));
    c.set(13, widget);
    if (raise) {
        edit_widget(0x1009a970, widget, 2, 12, 0x18);
        const auto shown = frame.at(0x3c), stored = c.u32(draft + wage_field);
        c.set(0, stored);
        c.put32(shown, stored);
        update_wage_text(0x1009a9a4, widget, shown);
        const bool allowed = compare(c, call.call(more_money, 0x1009aa0c, {shown, 4}), wage_limit) <= 0;
        edit_widget(allowed ? 0x1009aa58 : 0x1009aa24, widget + 2, allowed ? 2 : 6, allowed ? 12 : 44,
                    allowed ? 0x17 : 0, widget_action);
        refresh(0x1009aa88);
    } else {
        const auto stored = c.u32(draft + wage_field);
        c.set(0, stored);
        const bool allowed = compare(c, stored) != 0;
        edit_widget(allowed ? 0x1009ab04 : 0x1009aad0, widget, allowed ? 2 : 6, allowed ? 12 : 44,
                    allowed ? 0x18 : 0, widget_action);
        const auto shown = frame.at(0x38), refreshed = c.u32(draft + wage_field);
        c.set(0, refreshed);
        c.put32(shown, refreshed);
        update_wage_text(0x1009ab38, widget, shown);
        edit_widget(0x1009aba4, widget + 2, 2, 12, 0x17);
        refresh(0x1009abd4);
    }
}

void BasicCommand::duration_text(std::uint32_t month_back, std::uint32_t text_back, std::uint32_t widget_back,
                                 std::uint32_t widget, unsigned display_year, bool keep_year) const {
    const auto month_name = call.call(0x107188a0, month_back, {std::uint32_t(month)});
    if (keep_year) c.set(13, display_year);
    call.call(localized, text_back, {text, strings + 0xe66, month_name, display_year});
    call.call(widget_text, widget_back, {screens, widget + 1, text});
}

void BasicCommand::duration_upper(std::uint32_t off, std::uint32_t on, std::uint32_t widget, unsigned display_year) const {
    const auto maximum_year = c.u16(calendar + 2);
    const auto difference = display_year - std::uint32_t(c.i8(c.toc(-0x7990)));
    c.set({{0, maximum_year}, {3, difference}});
    const bool allowed = compare(c, difference, maximum_year) < 0;
    edit_widget(allowed ? on : off, widget + 2, allowed ? 2 : 6, allowed ? 12 : 44, allowed ? 9 : 0);
}

void BasicCommand::write_duration(std::uint32_t month_back, std::uint32_t year_back) const {
    c.set({{18, std::uint32_t(year)}, {20, std::uint32_t(month)}});
    write(month_back, 0xb, std::uint32_t(std::int8_t(month)));
    write(year_back, 0xc, std::uint16_t(year));
}

void BasicCommand::toggle_duration() const {
    const auto widget = extended(std::int16_t(read(0x1009abe8, 0x11)));
    const auto flags = term_flags();
    const auto kind = term_kind(flags);
    c.set({{4, flags}, {0, std::uint32_t(kind)}, {13, widget}});
    if (compare(c, kind, 1) == 0) {
        c.set(0, 2);
        set_term(flags, 2);
        edit_widget(0x1009ac20, widget, 2, 12, 0xa);
        duration_text(0x1009ac48, 0x1009ac5c, 0x1009ac6c, widget, year);
        duration_upper(0x1009ac98, 0x1009accc, widget, year);
    } else {
        c.set(0, 1);
        set_term(flags, 1);
        edit_widget(0x1009ad0c, widget, 3, 44, 0);
        call.call(localized, 0x1009ad38, {text, strings + 0xe57});
        call.call(widget_text, 0x1009ad48, {screens, widget + 1, text});
        edit_widget(0x1009ad58, widget + 2, 2, 12, 9);
    }
    refresh(0x1009ad88);
}

void BasicCommand::increase_duration() {
    const auto flags = term_flags();
    const auto kind = term_kind(flags);
    c.set({{0, std::uint32_t(kind)}, {3, flags}});
    const bool rolling = compare(c, kind, 1) == 0;
    c.set({{0, 2}, {3, (flags & 3) | 8}});
    set_term(flags, 2);
    if (!rolling) {
        const auto current_year = c.u16(calendar + 2);
        const auto anniversary = current_year + 1;
        c.set({{0, anniversary}, {3, current_year}});
        const bool at_anniversary = compare(c, year, anniversary) == 0 && compare(c, month, first_month) >= 0;
        if (at_anniversary || compare(c, year, anniversary) > 0) {
            ++year;
        } else {
            ++month;
            c.set(0, std::uint32_t(std::int8_t(month)));
            if (compare(c, std::int8_t(month), 11) > 0) {
                month = 0;
                ++year;
            }
        }
        write_duration(0x1009ae18, 0x1009ae2c);
    }
    const auto widget = extended(std::int16_t(read(0x1009ae38, 0x11)));
    c.set(14, widget);
    edit_widget(0x1009ae4c, widget, 2, 12, 0xa);
    const auto shown_year = std::uint16_t(year);
    duration_text(0x1009ae74, 0x1009ae8c, 0x1009ae9c, widget, shown_year, true);
    duration_upper(0x1009aec8, 0x1009aefc, widget, shown_year);
    refresh(0x1009af2c);
}

void BasicCommand::decrease_duration() {
    const auto start_year_address = calendar + 2;
    c.set(13, start_year_address);
    const auto previous = [&](std::uint32_t date, std::uint32_t get_month, bool first, std::int32_t test_year,
                              std::int32_t& month_value, std::int32_t& year_value) {
        const auto end_year = [&] {
            const auto current = c.u16(date + 2);
            c.set({{0, current + 1u}, {3, current}});
            return current + 1;
        };
        bool full_year = false;
        if (first) {
            full_year = compare(c, test_year, end_year()) == 0;
        } else {
            // The initial comparison uses a date year loaded before the widget is saved in r14.
            full_year = compare(c, test_year, c.r(0)) == 0;
        }
        if (full_year) {
            const auto boundary = std::int8_t(call.call(0x10718360, get_month, {date}));
            const auto shown_month = std::int8_t(month_value);
            c.set({{0, std::uint32_t(first ? boundary : shown_month)}, {3, first ? c.r(3) : std::uint32_t(boundary)}});
            full_year = compare(c, first ? month_value : shown_month, boundary) > 0;
        }
        if (full_year || compare(c, test_year, end_year()) > 0) {
            --year_value;
        } else {
            --month_value;
            c.set(0, std::uint32_t(std::int8_t(month_value)));
            if (compare(c, std::int8_t(month_value)) < 0) {
                month_value = 11;
                --year_value;
            }
        }
    };
    previous(calendar, 0x1009af50, true, year, month, year);
    write_duration(0x1009af9c, 0x1009afb0);

    const auto widget = extended(std::int16_t(read(0x1009afbc, 0x11)));
    auto before_month = month, before_year = year;
    const auto shown_year = std::uint16_t(year);
    const auto draft_date = draft + 0x28;
    const auto draft_year = c.u16(draft_date + 2);
    c.set({{0, draft_year + 1u}, {4, draft_year}, {13, shown_year}, {14, widget},
           {15, std::uint32_t(month)}, {16, std::uint32_t(year)}});
    previous(draft_date, 0x1009afe4, false, shown_year, before_month, before_year);
    c.set({{15, std::uint32_t(before_month)}, {16, std::uint32_t(before_year)}});
    const auto candidate_year = std::uint16_t(before_year);
    c.set(0, candidate_year);
    const bool too_early = (compare_logical(c, candidate_year, first_year) == 0 &&
                            (c.set(0, std::uint32_t(std::int8_t(before_month))),
                             compare(c, std::int8_t(before_month), first_month) <= 0)) ||
                           (c.set(0, candidate_year), compare_logical(c, candidate_year, first_year) < 0);
    if (too_early) {
        edit_widget(0x1009b054, widget, 6, 44, 0);
    } else {
        const auto current_year = c.u16(draft_date + 2);
        c.set({{0, current_year}, {3, shown_year}});
        bool toggle = false;
        if (compare_logical(c, shown_year, current_year) == 0) {
            const auto after_month = std::int8_t(call.call(0x10718360, 0x1009b090, {draft_date})) + 1;
            c.set({{0, std::uint32_t(std::int8_t(month))}, {3, std::uint32_t(after_month)}});
            toggle = compare(c, std::int8_t(month), after_month) == 0;
        }
        if (!toggle) {
            const auto reloaded = c.u16(draft_date + 2);
            c.set({{0, reloaded + 1u}, {3, reloaded}});
            if (compare(c, shown_year, reloaded + 1) == 0) {
                c.set(0, std::uint32_t(std::int8_t(month)));
                if (compare(c, std::int8_t(month)) == 0) {
                    const auto last_month = std::int8_t(call.call(0x10718360, 0x1009b0c4, {draft_date}));
                    c.set(0, std::uint32_t(last_month));
                    toggle = compare(c, last_month, 11) == 0;
                }
            }
        }
        if (toggle) {
            edit_widget(0x1009b0e0, widget, 2, 12, 0x1d);
        } else {
            const auto kind = term_kind(term_flags());
            c.set(0, std::uint32_t(kind));
            const bool rolling = compare(c, kind, 1) == 0;
            edit_widget(rolling ? 0x1009b12c : 0x1009b160, widget, rolling ? 6 : 2, rolling ? 44 : 12, rolling ? 0 : 0xa);
        }
    }
    duration_text(0x1009b188, 0x1009b19c, 0x1009b1ac, widget, shown_year);
    edit_widget(0x1009b1bc, widget + 2, 2, 12, 9);
    refresh(0x1009b1ec);
}

void BasicCommand::reset_duration() const {
    const auto proposal_month = std::int8_t(call.call(0x10718360, 0x1009b1fc, {proposal + 0x30}));
    write(0x1009b210, 0xb, std::uint32_t(proposal_month));
    write(0x1009b224, 0xc, c.u16(proposal + 0x32));
    const auto kind = term_kind(c.u8(proposal + length_flags));
    const auto flags = term_flags();
    c.set({{0, (flags & 3) | ((std::uint32_t(kind) & 0x3f) << 2)}, {4, std::uint32_t(kind)}});
    set_term(flags, kind);
}

std::uint32_t BasicCommand::employer(std::uint32_t back) const {
    const auto current = call.call(0x10082f20, back, {c.u32(contract_manager), person});
    const auto cached = c.toc(-0x7b78);
    c.set(4, cached);
    const bool exists = compare_logical(c, current) != 0;
    c.put32(cached, current);
    if (!exists) {
        c.set(0, 0);
        return 0;
    }
    const auto clubs = c.u32(c.toc(-0x7be8));
    const auto club = clubs + c.u32(current + 4) * 0x248;
    c.set({{0, club}, {3, clubs}});
    return club;
}

double BasicCommand::salary_as_double(std::uint32_t salary, unsigned constant_register) const {
    const auto constants = c.toc(-0x56d0);
    c.set({{0, 0x43300000}, {3, salary ^ 0x80000000}, {constant_register, constants}});
    c.put32(frame.at(0x84), c.r(3));
    c.put32(frame.at(0x80), c.r(0));
    const auto bias = c.f64(constants + 8);
    c.set_f(1, bias);
    c.set_f(0, c.f64(frame.at(0x80)));
    return Fpu(c).fsub(c.f(0), bias);
}

void BasicCommand::cycle_role() const {
    auto role = std::int32_t(std::int8_t(read(0x1009b2c8, 8)));
    const auto group = c.i16(screens + 0x2c58);
    const auto profile = c.u32(person + 0x14);
    const auto instance = c.u32(screens + 4 + std::uint32_t(group) * 0x2c0);
    const auto club = c.u32(instance + 0x24);
    c.set({{0, profile}, {3, instance}, {4, std::uint32_t(group)}, {14, std::uint32_t(role)}, {15, club}});
    if (compare_logical(c, profile) != 0) {
        role = compare(c, role, 11) == 0 ? 15 : 11;
        c.set(14, std::uint32_t(role));
    } else {
        do {
            ++role;
            c.set({{0, std::uint32_t(std::int8_t(role))}, {14, std::uint32_t(role)}});
            if (compare(c, std::int8_t(role), 11) == 0) {
                role = 0;
                c.set(14, 0);
            }
            const auto supported = std::int8_t(call.call(0x1007e4f0, 0x1009b324, {c.u32(contract_manager), std::uint32_t(role), person}));
            c.set(0, std::uint32_t(supported));
            if (compare(c, supported) != 0) break;
        } while (true);

        const auto role_code = std::int8_t(role);
        c.set(0, std::uint32_t(role_code));
        const bool special = compare(c, role_code, 11) == 0 || compare(c, role_code, 12) == 0 ||
                             compare(c, role_code, 15) == 0 || compare(c, role_code, 13) == 0;
        auto budget = call.call(0x10103510, special ? 0x1009b458 : 0x1009b35c, {c.u32(finance), club});
        double cap;
        if (special) {
            budget = call.call(0x100f2680, 0x1009b464, {budget, person, 0xffffffff});
            cap = salary_as_double(budget, 4);
            c.set_f(31, cap);
        } else {
            budget = call.call(0x100f4790, 0x1009b364, {budget, std::uint32_t(role)});
            cap = salary_as_double(budget, 5);
            c.set_f(31, cap);
            c.set(4, person);
            if (compare_logical(c, employer(0x1009b390)) != 0 && compare_logical(c, employer(0x1009b3d0), club) == 0) {
                const auto current = call.call(0x10084b60, 0x1009b410, {c.u32(contract_manager), person});
                const auto wage = salary_as_double(current, 4);
                c.set_f(0, wage);
                Fpu(c).fcmp(0, wage, cap);
                if (wage > cap) {
                    c.put32(frame.at(0x84), c.r(3));
                    c.put32(frame.at(0x80), c.r(0));
                    c.set_f(0, c.f64(frame.at(0x80)));
                    cap = Fpu(c).fsub(c.f(0), c.f(1));
                    c.set_f(31, cap);
                }
            }
        }
        c.put64(frame.at(0x80), Fpu(c).fctiwz(0, cap));
        write(0x1009b4a0, 0x18, c.u32(frame.at(0x84)));
    }
    write(0x1009b4b4, 8, std::uint32_t(std::int8_t(role)));
}

void BasicCommand::cycle_status() const {
    const auto flags = c.u8(draft + 0x60);
    const auto status = flags & 0xf;
    c.set({{0, status}, {3, flags}});
    auto next = status;
    const auto order = compare(c, status, 3);
    if (order < 0) {
        const auto before = compare(c, status, 1);
        if (before == 0) {
            const auto age = std::int8_t(c.i16(person + 0x5c));
            next = compare(c, age, 24) < 0 ? 5 : 4;
        } else if (before > 0) {
            next = 1;
        }
    } else if (order == 0) {
        next = 2;
    } else {
        const auto after = compare(c, status, 5);
        if (after == 0) next = 4;
        else if (after < 0) next = 3;
    }
    if (next != status) c.put8(draft + 0x60, (flags & 0xf0) | next);
    const auto stored = c.u8(draft + 0x60);
    c.set(0, stored);
    call.call(0x10374f50, 0x1009b5b0, {proposal, person, employer_layout, linked_offer, stored & 0xfu,
                                       0xb, frame.at(0x54), frame.at(0x50)});
    write(0x1009b5c4, 0x19, c.u32(frame.at(0x54)));
    const auto constants = c.toc(-0x56d0);
    c.set_f(0, c.f32(frame.at(0x50)));
    c.set_f(1, c.f64(constants + 0x30));
    Fpu fp(c);
    c.put64(frame.at(0x80), fp.fctiwz(0, fp.fmul(c.f(1), c.f(0))));
    write(0x1009b5f0, 0x1a, c.u32(frame.at(0x84)));
}
}

std::uint32_t negotiation_basic_command(Cpu& c, std::uint32_t pressed) {
    Frame frame(c, 0x1009a440, 0xf0, {.first_gpr = 13, .gpr_offset = -0x5c, .first_fpr = 31});
    const auto drafts = c.toc(-0x7a54), finance = c.toc(-0x7ab8), calendar = c.toc(-0x7b80),
               contract_manager = c.toc(-0x7b7c), strings = c.toc(-0x56cc),
               text = c.toc(-0x7bb8), screens = c.toc(-0x7bb0);
    keep(c, {{13, finance}, {14, c.toc(-0x7ba8)}, {15, pressed}, {16, 0xffffffff}, {26, calendar},
             {27, contract_manager}, {28, strings}, {29, text}, {30, screens}});
    c.set({{0, c.lr()}, {5, drafts}});
    Program program{c};
    const auto draft = program.call(0x101487d0, 0x1009a488, {c.u32(drafts), 0});
    c.set(23, draft);
    if (compare(c, draft) == 0) {
        report_assertion(c, {0x1009a4a4, strings + 0x87, strings + 0x9c, strings + 0xa9,
                             frame.at(0x60), frame.at(0x58), 0xb2b});
        c.set({{0, 0}, {4, c.toc(-0x7bac)}});
        program.call(0x1076f7d0, 0x1009a4ec, {0xffffffff});
    }
    BasicCommand command{c, frame, program, screens, strings, text, calendar, contract_manager, finance, draft};
    const auto person_id = command.read(0x1009a4f8, 0);
    const auto group_index = screens + 0x2c58, instance_table = screens + 4;
    const auto group_offset = std::uint32_t(c.i16(group_index)) * 0x2c0, people = c.toc(-0x7be0);
    const auto people_base = c.u32(people);
    const auto instance = c.u32(instance_table + group_offset);
    command.person = people_base + person_id * 0x70;
    command.employer_layout = c.u32(instance + 0x24);
    c.set({{0, group_offset}, {5, instance}, {6, person_id * 0x70}});
    keep(c, {{14, command.employer_layout}, {21, command.person}, {24, instance_table}, {25, group_index}});
    command.proposal = command.read(0x1009a52c, 2);
    c.set({{0, command.proposal}, {22, command.proposal}});
    command.wage_limit = command.read(0x1009a540, 0x15);
    c.set(31, command.wage_limit);
    const auto fee = c.u32(draft + signing_fee_field), wage = c.u32(draft + wage_field);
    c.set(0, wage);
    c.put32(frame.at(0x4c), fee);
    c.put32(frame.at(0x48), wage);
    const auto link = command.read(0x1009a560, 3);
    if (compare_logical(c, link) != 0) {
        const auto offer = c.u32(link);
        c.set(3, offer);
        if (compare_logical(c, offer) != 0) {
            command.linked_offer = c.u32(offer + 0x28);
            c.set(16, command.linked_offer);
        }
    }
    command.first_month = std::int8_t(command.read(0x1009a584, 9));
    c.set(19, std::uint32_t(command.first_month));
    command.first_year = std::uint16_t(command.read(0x1009a594, 0xa));
    c.set(17, std::uint32_t(command.first_year));
    command.month = std::int8_t(command.read(0x1009a5a4, 0xb));
    c.set(20, std::uint32_t(command.month));
    command.year = std::uint16_t(command.read(0x1009a5b4, 0xc));
    c.set(18, std::uint32_t(command.year));

    const auto group = c.i16(group_index);
    c.set(4, extended(group));
    const auto layout = c.u32(Manager{c, screens}.group(group) + 0x24);
    c.set(3, layout);
    const detail::Controls controls{c, screens, layout, std::int16_t(pressed)};
    const auto action = controls.action(4);
    const auto index = std::uint32_t(action - 4);
    c.set({{0, index}, {4, std::uint32_t(action)}});
    if (compare_logical(c, index, 0x2c) > 0) return 0;
    const auto table_offset = index * 4, destination = c.u32(c.toc(-0x56f8) + table_offset);
    c.set({{0, table_offset}, {4, destination}});
    c.s.wr<4>(reg::ctr, destination);

    switch (action) {
    case 4: command.change_fee(true); return 0;
    case 5: command.change_fee(false); return 0;
    case 6: {
        const auto proposal_fee = c.u32(command.proposal + signing_fee_field);
        c.set(0, proposal_fee);
        c.put32(draft + signing_fee_field, proposal_fee);
        return redraw;
    }
    case 7:
    case 8: {
        const auto value = controls.value();
        c.set(0, value);
        c.put32(draft + signing_fee_field, value);
        return redraw;
    }
    case 9: command.increase_duration(); return 0;
    case 0xa: command.decrease_duration(); return 0;
    case 0xb: command.reset_duration(); return redraw;
    case 0x16: {
        const auto proposal_wage = c.u32(command.proposal + wage_field);
        command.set_wage(proposal_wage);
        return redraw;
    }
    case 0x17: command.change_wage(true); return 0;
    case 0x18: command.change_wage(false); return 0;
    case 0x19: command.set_wage(controls.value()); return redraw;
    case 0x1a: command.write(0x1009b278, 7, controls.value()); return redraw;
    case 0x1c: command.cycle_role(); return redraw;
    case 0x1d: command.toggle_duration(); return 0;
    case 0x1e: {
        const auto flags = command.term_flags();
        const auto toggled = flags ^ 1;
        c.set({{4, toggled}, {0, toggled & 1}});
        c.s.wr<1>(reg::xer_ca, 0);
        c.put8(draft + length_flags, toggled);
        return redraw;
    }
    case 0x1f: command.cycle_status(); return redraw;
    case 0x30: command.write(0x1009b2b4, 0x1c, controls.value()); return redraw;
    default: return 0;
    }
}
}
