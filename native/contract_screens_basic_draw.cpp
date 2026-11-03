#include "contract_screens.hpp"
#include "contract_screens_money.hpp"
#include "guest_fpu.hpp"
#include <array>

namespace cm::aot::contract_screens {
using namespace guest;

namespace {
using screen_manager::detail::Manager;
using screen_manager::detail::Program;

constexpr std::uint32_t read_history = 0x1073ef70, write_history = 0x1073f1e0,
                        grid_control = 0x107198c0, add_button = 0x107444e0, add_label = 0x107445a0,
                        localized = 0x10727890, date_month = 0x10718360, amount_step = 0x1000a060,
                        parse_money = 0x10009f20, contract_text = 0x10372590;
constexpr unsigned wage_field = 8, fee_field = 0xc, term_field = 0x38;

struct BasicScreen {
    Cpu& c;
    const Frame& frame;
    Program call;
    std::uint32_t screens, strings, text, constants, ink, accent, field, selected;
    std::uint32_t draft, proposal, person, top, bottom;
    std::uint32_t grid = 0, target_club = 0, current_club = 0, wage_cap = 0;
    double fee_cap = 0, score = 0;

    std::uint32_t read(std::uint32_t back, unsigned slot) const {
        return call.call(read_history, back, {screens, slot});
    }
    void write_widget(std::uint32_t back, unsigned slot, std::uint32_t handle) const {
        c.set(0, handle);
        call.call(write_history, back, {screens, slot, extended(std::int16_t(handle)), 0});
    }
    void words(std::initializer_list<std::uint32_t> values) const {
        auto address = frame.at(0x38);
        for (const auto value : values) {
            c.put32(address, value);
            address += 4;
        }
    }
    void caption(std::uint32_t back, unsigned offset, std::initializer_list<std::uint32_t> values = {}) const {
        unsigned reg = 5;
        for (const auto value : values) c.set(reg++, value);
        call.call(localized, back, {text, strings + offset});
    }
    std::uint32_t row() const { return c.u32(frame.at(0x7c)); }
    void advance(unsigned rows = 1, unsigned scratch = 3) const {
        const auto old = row();
        c.set({{scratch, old}, {0, old + rows}});
        c.put32(frame.at(0x7c), old + rows);
    }
    std::uint32_t group_offset() const {
        return std::uint32_t(c.i16(screens + 0x2c58)) * 0x2c0;
    }
    std::uint32_t layout(bool indexed = false) const {
        const auto offset = group_offset();
        c.set(0, offset + (indexed ? 0x24 : 0));
        return c.u32(screens + 0x24 + offset);
    }
    void draw_box(std::uint32_t back) const {
        c.set(0, 0);
        words({0});
        call.call(0x1074bcd0, back, {frame.at(0xa0), grid, 120, 770, frame.at(0x7c), 3, c.u16(ink), 0});
    }
    void attach(std::uint32_t back, std::uint32_t value, unsigned category, std::uint32_t argument) const {
        call.call(0x1074bab0, back, {frame.at(0xa0), value, category, argument});
    }
    void assign(std::uint32_t back) const { call.call(0x1074b830, back, {frame.at(0xa0), text}); }
    std::uint32_t cell(std::uint32_t back, unsigned column, unsigned type, unsigned decoration, unsigned style,
                       std::uint32_t caption, bool clickable = true, unsigned action = 0, unsigned value = 0) const {
        if (clickable) {
            c.set(0, value);
            words({c.u16(ink), caption, action, value});
        } else {
            c.set(0, type == 0x30 || !caption ? caption : c.u16(ink));
            words({c.u16(ink), caption});
        }
        return call.call(clickable ? add_button : add_label, back, {screens, grid, column, row(), type, decoration, style, 3});
    }
    std::uint32_t adjust_button(std::uint32_t back, unsigned column, unsigned title, bool enabled, unsigned action = 0) const {
        return cell(back, column, 0x30, c.u16(accent), enabled ? 12 : 44, strings + title, enabled, action);
    }

    double integer_for_fpu(std::uint32_t value, unsigned input, unsigned bias_register, unsigned offset) const {
        c.set({{0, 0x43300000}, {3, value ^ 0x80000000}});
        c.put32(frame.at(offset), c.r(0));
        c.put32(frame.at(offset + 4), c.r(3));
        c.set_f(input, c.f64(frame.at(offset)));
        c.set_f(bias_register, c.f64(constants + 8));
        return c.f(input);
    }
    double compare_fee(std::uint32_t value) const {
        Fpu fp(c);
        const auto operand = integer_for_fpu(value, 1, 2, 0x8f0);
        c.set_f(0, fp.fmul(c.f64(constants + 0x78), fee_cap));
        c.set_f(1, fp.fsub(operand, c.f(2)));
        fp.fcmp(0, c.f(1), c.f(0));
        return c.f(1);
    }
    bool score_above(unsigned offset) const {
        c.set_f(0, c.f64(constants + offset));
        Fpu(c).fcmp(0, score, c.f(0));
        return score > c.f(0);
    }
    bool score_below(unsigned offset) const {
        score_above(offset);
        return score < c.f(0);
    }
    std::int32_t kind(std::uint32_t flags) const {
        c.s.wr<1>(reg::xer_ca, (flags & 0x82) == 0x82);
        return std::int8_t(flags) >> 2;
    }
    bool restricted(std::uint32_t flags) const {
        const auto value = flags & 1 ? -1 : 0;
        c.s.wr<1>(reg::xer_ca, 0);
        c.set(0, std::uint32_t(value));
        return compare(c, value) != 0;
    }
    std::uint32_t current_contract(std::uint32_t back) const {
        return call.call(0x10082f20, back, {c.u32(c.toc(-0x7b7c)), person});
    }
    std::uint32_t employer(std::uint32_t back, unsigned destination) const;
    bool same_club() const;

    void initialize();
    void reputation_threshold() const;
    bool qualifies_for_short_message() const;
    void short_message() const;
    void general_message() const;
    void renewal_message() const;
    void draft_message() const;
    void money_controls();
    void duration_controls() const;
    void options() const;
    void presets() const;
    void notes() const;
};

void BasicScreen::initialize() {
    const auto table = c.toc(-0x56ec);
    const auto head = c.u32(table);
    const auto middle = c.u16(table + 4), tail = std::uint16_t(c.u8(table + 6));
    c.put32(frame.at(0x7c), 0);
    c.put32(frame.at(0xa8), head);
    c.put16(frame.at(0xac), middle);
    c.put8(frame.at(0xae), tail);
    c.set({{0, tail}, {5, middle}, {8, head}, {9, table}, {10, 0}});
    call.call(0x1074b6e0, 0x100981b0, {frame.at(0xa0), 0});
    call.call(0x103721f0, 0x100981b8, {frame.at(0xf0)});

    const auto values = c.toc(-0x56f0);
    c.put8(frame.at(0x410), 0);
    c.set({{18, 0xffffffff}, {19, 0}, {12, values}});
    for (unsigned i = 0; i < 8; ++i) {
        const auto value = c.u32(values + 4 * i);
        c.put32(frame.at(0xb0 + 4 * i), value);
        c.set(i == 7 ? 0 : 11 - i, value);
    }
    const auto mode = std::int8_t(read(0x10098218, 0x1c));
    c.set(0, std::uint32_t(mode));
    c.put8(frame.at(0x905), mode);
    wage_cap = read(0x1009822c, 0x15);
    c.set(31, wage_cap);
    c.put32(frame.at(0x918), read(0x1009823c, 3));
    const auto role = std::int8_t(read(0x1009824c, 8));
    c.set(0, std::uint32_t(role));
    c.put8(frame.at(0x90c), role);
    const auto offer_mode = std::int8_t(read(0x10098260, 0xd));
    c.set(0, std::uint32_t(offer_mode));
    c.put8(frame.at(0x903), offer_mode);
    const auto note = std::int8_t(read(0x10098274, 0x1d));
    c.set(0, std::uint32_t(note));
    c.put8(frame.at(0x904), note);
    Fpu fp(c);
    auto fee = read(0x10098288, 0x18);
    integer_for_fpu(fee, 0, 1, 0x8f8);
    fee_cap = fp.fsub(c.f(0), c.f(1));
    c.set_f(30, fee_cap);
    const auto recommendation = read(0x100982b0, 0x19);
    c.set(0, recommendation);
    c.put32(frame.at(0x908), recommendation);
    const auto base_score = read(0x100982c4, 0x1a);
    integer_for_fpu(base_score, 1, 2, 0x8f0);
    const auto scale = c.toc(-0x56f4);
    c.set(5, scale);
    c.set_f(0, c.f32(scale));
    c.set_f(1, fp.fsubs(c.f(1), c.f(2)));
    score = fp.fdivs(c.f(1), c.f(0));
    c.set_f(31, score);
    const auto notes = read(0x100982f8, 0x1b);
    const auto club_id = c.u32(proposal + 4);
    c.set(0, club_id);
    c.put32(frame.at(0x91c), notes);
    c.put32(frame.at(0x914), club_id);
    if (compare(c, club_id) > 0) {
        const auto count = c.u32(c.toc(-0x7b50));
        c.set(3, count);
        if (compare(c, club_id, count) < 0) {
            const auto clubs = c.u32(c.toc(-0x7be8));
            c.set({{0, club_id * 0x248}, {3, clubs}});
            target_club = clubs + club_id * 0x248;
            c.set(19, target_club);
        }
    }
    const auto height = call.call(0x1076d470, 0x10098334, {3});
    const auto rows = std::uint8_t(divide_signed<4>(bottom - top, height));
    c.set(20, screens + 0x2c58);
    c.put32(frame.at(0x910), screens + 0x24);
    words({0, 2, 0, 0xffffffff});
    grid = call.call(grid_control, 0x10098398, {layout(), 110, top, 780, bottom, 7, frame.at(0xa8), rows});
    c.set(22, grid);
    current_club = employer(0x100983b0, 21);
}

std::uint32_t BasicScreen::employer(std::uint32_t back, unsigned destination) const {
    c.set(13, 0);
    const auto contract = current_contract(back), cache = c.toc(-0x7b78);
    c.set(4, cache);
    const bool present = compare_logical(c, contract) != 0;
    c.put32(cache, contract);
    bool employed = false;
    if (present) {
        const auto type = kind(c.u8(contract + term_field));
        c.set(0, std::uint32_t(type));
        employed = compare(c, type, 5) != 0;
        if (employed) c.set(13, 1);
    }
    std::uint32_t result = 0;
    if (compare(c, employed) != 0) {
        const auto latest = c.u32(c.toc(-0x7b78));
        const auto clubs = c.u32(c.toc(-0x7be8)), index = c.u32(latest + 4) * 0x248;
        c.set({{0, destination == 0 ? clubs + index : index}, {3, clubs}, {4, latest}});
        result = clubs + index;
    }
    c.set(destination, result);
    return result;
}

void BasicScreen::reputation_threshold() const {
    const bool has_club = compare_logical(c, target_club) != 0;
    c.set(13, 0);
    if (!has_club) return;
    const auto status = c.u8(proposal + 0x60) & 0xf;
    c.set(0, status);
    const auto order = compare(c, status, 3);
    if ((order < 0 && compare(c, status, 1) < 0) ||
        (order > 0 && compare(c, status, 5) != 0)) return;
    const auto rating = c.i16(target_club + 0x1ec);
    c.set(0, extended(rating));
    enum class Formula { multiply, add, fused };
    struct Rate {
        Formula formula;
        unsigned factor, base = 0;
        std::uint32_t site = 0;
    };
    constexpr Rate high[]{{Formula::fused, 0x40, 0x38, 0x10098474}, {Formula::add, 0x48},
                          {Formula::fused, 0x58, 0x60, 0x100985f0}, {Formula::multiply, 0x68}};
    constexpr Rate middle[]{{Formula::add, 0x48}, {Formula::fused, 0x58, 0x60, 0x10098570},
                            {Formula::fused, 0x68, 0x50, 0x10098630}, {Formula::multiply, 0x70}};
    constexpr Rate low[]{{Formula::fused, 0x58, 0x50, 0x100984f0}, {Formula::multiply, 0x68},
                         {Formula::multiply, 0x70}};
    const unsigned band = status == 5 ? 3 : status - 1;
    const Rate* rate = nullptr;
    if (compare(c, rating, 7250) > 0) rate = &high[band];
    else if (compare(c, rating, 6250) > 0) rate = &middle[band];
    else if (band < 3 && compare(c, rating, 4750) > 0) rate = &low[band];
    if (!rate) return;

    Fpu fp(c);
    if (rate->formula == Formula::fused) {
        const auto value = integer_for_fpu(extended(rating), 1, 2, 0x8f0);
        c.set_f(3, c.f64(constants + rate->factor));
        c.set_f(0, c.f64(constants + rate->base));
        c.set_f(1, fp.fsub(value, c.f(2)));
        c.set_f(0, fp.multiply_add(rate->site, c.f(3), c.f(1), c.f(0)));
    } else {
        const auto value = integer_for_fpu(extended(rating), 0, 1, 0x8f0);
        c.set_f(2, c.f64(constants + rate->factor));
        c.set_f(0, fp.fsub(value, c.f(1)));
        c.set_f(0, rate->formula == Formula::add ? fp.fadd(c.f(2), c.f(0)) : fp.fmul(c.f(2), c.f(0)));
    }
    c.put64(frame.at(0x8f8), fp.fctiwz(0, c.f(0)));
    c.set(13, c.u32(frame.at(0x8fc)));
}

bool BasicScreen::qualifies_for_short_message() const {
    const auto eligible = std::int8_t(call.call(0x10089ea0, 0x10098708, {c.u32(c.toc(-0x7b7c)), proposal}));
    c.set(0, std::uint32_t(eligible));
    if (compare(c, eligible) == 0) return false;
    const auto fee = compare_fee(c.u32(proposal + fee_field));
    if (!(fee < c.f(0))) return false;
    if (score_above(0x80)) return true;
    if (compare_logical(c, current_club, target_club) == 0) {
        const auto renewal = c.u32(frame.at(0x91c)) & 2;
        c.set(0, renewal);
        if (compare(c, renewal) != 0) return true;
    }
    const auto role = c.u8(frame.at(0x90c));
    c.set(0, role);
    if (compare(c, role, 8) == 0 || compare(c, role, 10) == 0 || compare(c, role, 9) == 0) {
        const auto proposed = c.u32(proposal + fee_field);
        c.set(0, proposed);
        if (compare(c, proposed, 3200) < 0) return true;
    }
    const auto age = std::int8_t(c.i16(person + 0x5c));
    c.set(0, std::uint32_t(age));
    if (compare(c, age, 24) >= 0) return false;
    const auto ceiling = std::int16_t(c.r(13)) + 3250;
    const auto proposed = c.u32(proposal + fee_field);
    c.set({{0, std::uint32_t(ceiling)}, {3, extended(std::int16_t(c.r(13)))}, {4, proposed}});
    return compare(c, proposed, ceiling) < 0;
}

void BasicScreen::short_message() const {
    const auto name = frame.at(0x730), fee = frame.at(0x5a0), date = frame.at(0x280), wage = frame.at(0x154);
    call.call(0x100caeb0, 0x100987c0, {person, name, 2000, 1});
    call.call(contract_text, 0x100987d4, {proposal, fee, 2, 2});
    const auto amount = c.u32(proposal + wage_field);
    c.set(0, amount);
    c.put32(frame.at(0x78), amount);
    detail::money_text(c, call, wage, frame.at(0x78), {0x100987e8, 0x10098800}, 6);
    const auto flags = c.u8(proposal + term_field);
    const auto term = kind(flags);
    c.set({{0, std::uint32_t(term)}, {3, flags}});
    if (compare(c, term, 1) == 0) {
        if (restricted(flags)) caption(0x1009883c, 0x28a, {name, fee});
        else caption(0x10098854, 0x2f1, {name, fee});
    } else {
        call.call(0x10374d80, 0x10098868, {proposal, date, 1});
        if (restricted(c.u8(proposal + term_field))) {
            caption(0x10098898, 0x356, {100, name, fee, date});
        } else {
            const auto wage_value = c.u32(proposal + wage_field);
            c.set(0, wage_value);
            if (compare(c, wage_value) == 0) {
                caption(0x10098968, 0x5dd, {100, name, fee, date});
            } else {
                const auto id = c.u32(person), odd = id & 1, pair = (id >> 31) & 1;
                const auto remainder = (odd ^ pair) - pair;
                c.set({{0, remainder}, {3, pair}});
                if (compare(c, remainder) != 0) {
                    caption(0x100988e0, 0x3e0, {100, name, fee, date, wage});
                } else {
                    const auto high = [&](unsigned offset) {
                        const auto value = c.i8(person + offset);
                        c.set(0, std::uint32_t(value));
                        return compare(c, value, 10) > 0;
                    };
                    if (high(0x67) && high(0x65)) caption(0x10098924, 0x47e, {100, name, fee, date, wage});
                    else caption(0x10098948, 0x52e, {100, name, fee, date, wage});
                }
            }
        }
        attach(0x1009897c, date, 0xb, 0);
    }
    assign(0x10098988);
    attach(0x1009899c, name, 0x7da, person);
    attach(0x100989b0, fee, 6, 0);
}

void BasicScreen::renewal_message() const {
    const auto contract = current_contract(0x10098d70);
    const auto name = frame.at(0x730);
    if (compare_logical(c, contract) != 0) {
        const auto flags = c.u32(contract + 0x50) & 2;
        c.set(0, flags);
        if (compare(c, flags) != 0) {
            const auto requested = c.u32(proposal + fee_field);
            c.set(0, requested);
            if (compare(c, requested, 25000) < 0) {
                const auto old = c.u32(contract + fee_field);
                c.set(0, old);
                if (compare(c, old, 2100) > 0) {
                    caption(0x10098dac, 0xb52, {name});
                    return;
                }
            }
        }
    }
    if (compare_logical(c, contract) != 0) {
        const auto flags = c.u32(contract + 0x50) & 0x6214;
        c.set(0, flags);
        if (compare(c, flags) != 0) {
            caption(0x10098dd4, 0xba6, {name});
            return;
        }
    }
    if (compare_logical(c, contract) != 0) {
        const auto flags = c.u32(contract + 0x50) & 2;
        c.set(0, flags);
        if (compare(c, flags) != 0) {
            const auto mode = c.u8(frame.at(0x903));
            c.set(0, mode);
            if (compare(c, mode, 1) == 0) caption(0x10098e08, 0xb52, {name});
            else caption(0x10098e1c, 0xc09, {name});
            return;
        }
    }
    caption(0x10098e30, 0xc6e, {name});
}

void BasicScreen::general_message() const {
    const auto name = frame.at(0x730), fee = frame.at(0x5a0), date = frame.at(0x280), reason = frame.at(0x410);
    call.call(0x100caeb0, 0x100989c8, {person, name, 2000, 1});
    call.call(contract_text, 0x100989dc, {proposal, fee, 2, 2});
    const bool employed = compare_logical(c, current_club) != 0;
    c.set({{6, 0xffffffff}, {7, 0xffffffff}});
    if (employed && compare_logical(c, current_club, target_club) == 0) {
        renewal_message();
    } else {
        std::uint32_t origin_nation = 0xffffffff, target_nation = 0xffffffff;
        if (compare_logical(c, current_club) != 0) {
            const auto nation = c.u32(current_club + 0x90);
            c.set(3, nation);
            if (compare_logical(c, nation) != 0) origin_nation = c.u32(nation);
        }
        const auto nation = c.u32(target_club + 0x90);
        c.set(3, nation);
        if (compare_logical(c, nation) != 0) target_nation = c.u32(nation);
        const bool from_club = compare_logical(c, current_club) != 0;
        c.set(0, reason);
        words({reason});
        c.set_f(1, score);
        const auto manager = c.u32(c.toc(-0x7b40));
        const auto result = call.call(0x10410e50, from_club ? 0x10098a48 : 0x10098a74,
            {manager, from_club ? c.u32(current_club) : 0xffffffff, c.u32(target_club), origin_nation, target_nation,
             c.u32(frame.at(0x908))});
        c.set(21, result);
        const auto compared_fee = compare_fee(read(0x10098a84, 0x16));
        const auto has_reason = [&] {
            const auto available = std::int8_t(result);
            c.set(0, std::uint32_t(available));
            return compare(c, available) != 0;
        };
        if (compared_fee > c.f(0)) {
            score_above(0x20);
            const bool at_least = score >= c.f(0);
            if (at_least) c.set_cr(0, c.cr(0) | 2);
            bool dated = false;
            if (at_least) {
                const auto requested = c.u32(proposal + fee_field);
                c.set({{0, 175000}, {3, 0x30000}, {4, requested}});
                if (compare(c, requested, 175000) < 0) {
                    const auto type = kind(c.u8(proposal + term_field));
                    c.set(0, std::uint32_t(type));
                    dated = compare(c, type, 1) != 0;
                }
            }
            if (dated) {
                call.call(0x10374d80, 0x10098afc, {proposal, date, 1});
                if (restricted(c.u8(proposal + term_field))) caption(0x10098b2c, 0x356, {100, name, fee, date});
                else caption(0x10098b4c, 0x5dd, {100, name, fee, date});
                attach(0x10098b60, date, 0xb, 0);
            } else if (has_reason()) {
                caption(0x10098b80, 0x64d, {name, reason});
            } else if (score_below(0x58)) {
                const auto style = std::uint32_t(c.i8(target_club + 0x242));
                caption(0x10098bb0, 0x6b2, {style, style, name, target_club + 0x1ee});
            } else {
                caption(0x10098bc4, 0x71d, {name});
            }
        } else {
            const auto status = c.u8(proposal + 0x60) & 0xf;
            c.set(0, status);
            struct Reasons { std::uint32_t no_back, yes_back; unsigned no_text, yes_text; };
            constexpr Reasons reasons[]{{0x10098c3c, 0x10098c28, 0x71d, 0x64d},
                                        {0x10098c7c, 0x10098c68, 0x7e8, 0x768},
                                        {0x10098cbc, 0x10098ca8, 0x8c6, 0x84e},
                                        {0x10098cfc, 0x10098ce8, 0x9aa, 0x924},
                                        {0x10098d3c, 0x10098d28, 0xa91, 0xa16}};
            int variant = -1;
            const auto order = compare(c, status, 4);
            if (order == 0) variant = 2;
            else if (order < 0) {
                if (compare(c, status, 3) >= 0) variant = 1;
                else if (compare(c, status, 1) >= 0) variant = 0;
            } else {
                const auto upper = compare(c, status, 6);
                if (upper == 0) variant = 4;
                else if (upper < 0) variant = 3;
            }
            if (variant >= 0) {
                const auto& wording = reasons[variant];
                if (has_reason() && score_above(0x40)) caption(wording.yes_back, wording.yes_text, {name, reason});
                else caption(wording.no_back, wording.no_text, {name});
            } else {
                const auto profile = c.u32(person + 0x1c);
                c.set(0, profile);
                if (compare_logical(c, profile) != 0) caption(0x10098d5c, 0xaf5, {name});
            }
        }
    }
    assign(0x10098e3c);
    attach(0x10098e50, name, 0x7da, person);
}

void BasicScreen::draft_message() const {
    draw_box(0x10098e7c);
    call.call(0x1074c570, 0x10098e84, {frame.at(0xa0)});
    const auto converted = Fpu(c).fctiwz(0, fee_cap);
    const auto limit = std::uint32_t(converted), next = row() + 2;
    c.put64(frame.at(0x8f0), converted);
    c.put64(frame.at(0x8f8), converted);
    c.set({{0, limit}, {7, limit}, {8, next}});
    c.put32(frame.at(0x7c), next);
    c.put32(frame.at(0xfc), limit);
    c.put32(frame.at(0x74), limit);
    const auto description = frame.at(0x348);
    call.call(contract_text, 0x10098ec0, {frame.at(0xf0), description, 2, 2});
    const auto lower_club_rating = [&] {
        if (compare_logical(c, person) == 0 || compare_logical(c, target_club) == 0) return false;
        const auto profile = c.u32(person + 0x14);
        c.set(3, profile);
        if (compare_logical(c, profile) == 0) return false;
        const auto rating = c.i16(profile + 0xa), club_rating = c.i16(target_club + 0x1ec);
        c.set({{0, std::uint32_t(rating + 750)}, {3, extended(rating)}, {4, extended(club_rating)}});
        return compare(c, club_rating, rating + 750) < 0;
    };
    if (lower_club_rating()) {
        const auto interested = std::int8_t(call.call(0x100d9520, 0x10098efc, {person, c.u32(target_club + 0x80)}));
        c.set(0, std::uint32_t(interested));
        if (compare(c, interested) != 0) {
            if (compare_logical(c, employer(0x10098f18, 0), target_club) == 0) caption(0x10098f88, 0xcd2, {description});
            else caption(0x10098f9c, 0xd48, {description});
        } else {
            caption(0x10098fb0, 0xdb1, {description});
        }
    } else {
        caption(0x10098fc4, 0xdb1, {description});
    }
    const auto draft_fee = c.u32(draft + fee_field);
    c.set({{0, draft_fee}, {3, limit}});
    if (compare(c, draft_fee, limit) > 0) c.put32(draft + fee_field, limit);
    assign(0x10098fe4);
    c.put64(frame.at(0x8f0), Fpu(c).fctiwz(0, fee_cap));
    attach(0x10099000, description, 7, c.u32(frame.at(0x8f4)));
    draw_box(0x1009902c);
    advance(1, 4);
    cell(0x1009906c, 0, 1, 0, 12, 0, false);
}

void BasicScreen::money_controls() {
    advance(1, 4);
    const auto settings = c.u32(c.u32(c.toc(-0x7ac8)) + 4);
    c.set(3, settings);
    c.set(0, settings & 0x200000);
    if (compare(c, c.r(0)) != 0) {
        caption(0x10099098, 0xdf8);
    } else {
        c.set(0, settings & 0x400000);
        if (compare(c, c.r(0)) != 0) caption(0x100990b0, 0xe04);
        else caption(0x100990c0, 0xe11);
    }
    cell(0x100990fc, 1, 1, c.u16(field), 1, text);
    const auto fee = c.u32(draft + fee_field);
    c.set(0, fee);
    const bool reduce_fee = compare(c, fee, 100) > 0;
    write_widget(0x100991a0, 0xf, adjust_button(reduce_fee ? 0x10099188 : 0x1009913c, 2, 0xe1d, reduce_fee, 5));
    const auto fee_label = frame.at(0x5a0);
    call.call(contract_text, 0x100991b4, {draft, fee_label, 0, 2});
    const auto fee_cell = cell(0x100991f4, 3, 1, c.u16(c.r(24)), 12, fee_label);
    c.put32(frame.at(0x920), fee_cell);
    const auto fee_amount = frame.at(0x70), latest_fee = c.u32(draft + fee_field);
    c.set(0, latest_fee);
    c.put32(fee_amount, latest_fee);
    call.call(amount_step, 0x1009920c, {fee_amount, 8});
    const auto cap_high = call.call(parse_money, 0x10099218, {frame.at(0x74), 8}), cap_low = c.r(4);
    c.set({{13, cap_low}, {16, cap_high}});
    const auto next_high = call.call(parse_money, 0x1009922c, {fee_amount, 8}), next_low = c.r(4);
    const auto cap = std::int64_t((std::uint64_t(cap_high) << 32) | cap_low);
    const auto next = std::int64_t((std::uint64_t(next_high) << 32) | next_low);
    c.s.wr<1>(reg::xer_ca, cap >= next);
    c.set({{0, cap_low - next_low}, {3, cap < next}, {5, cap_high ^ 0x80000000}});
    const bool raise_fee = compare(c, c.r(3)) == 0;
    adjust_button(raise_fee ? 0x100992c4 : 0x1009927c, 4, 0xe1f, raise_fee, 4);
    adjust_button(0x10099308, 5, 0xe21, true, 6);

    advance(1, 5);
    caption(0x10099320, 0xe23);
    cell(0x1009935c, 1, 1, c.u16(field), 1, text);
    const auto wage = c.u32(draft + wage_field);
    c.set(0, wage);
    const bool reduce_wage = compare(c, wage) != 0;
    write_widget(0x10099400, 0x10, adjust_button(reduce_wage ? 0x100993e8 : 0x1009939c, 2, 0xe1d, reduce_wage, 0x18));
    const auto wage_amount = frame.at(0x6c), shown = c.u32(draft + wage_field);
    c.set(0, shown);
    c.put32(wage_amount, shown);
    detail::money_text(c, call, text, wage_amount, {0x10099414, 0x1009942c, 0x10099434, 0x10099448, 0x10099460}, 6);
    const auto wage_cell = cell(0x1009949c, 3, 1, c.u16(c.r(24)), 12, text);
    c.put32(frame.at(0x924), wage_cell);
    const auto step = frame.at(0x68), latest_wage = c.u32(draft + wage_field);
    c.set(0, latest_wage);
    c.put32(step, latest_wage);
    const bool raise_wage = compare(c, call.call(amount_step, 0x100994b4, {step, 4}), wage_cap) <= 0;
    adjust_button(raise_wage ? 0x10099538 : 0x100994f0, 4, 0xe1f, raise_wage, 0x17);
    adjust_button(0x1009957c, 5, 0xe21, true, 0x16);
}

void BasicScreen::duration_controls() const {
    const auto calendar = c.toc(-0x7b80);
    const auto next = row() + 1;
    c.set(5, next);
    c.put32(frame.at(0x7c), next);
    c.put32(frame.at(0x88), c.u32(calendar));
    const auto tail = c.u32(calendar + 4);
    c.put32(frame.at(0x8c), tail);
    const auto draft_year = c.u16(draft + 0x2a), calendar_year = c.u16(frame.at(0x8a));
    c.set({{0, tail}, {3, draft_year}, {4, calendar_year}});
    const auto year_order = compare_logical(c, calendar_year, draft_year);
    bool starts_later = year_order < 0;
    if (year_order == 0) {
        const auto day = c.i16(draft + 0x28), now = c.i16(frame.at(0x88));
        const auto changed = std::int32_t(day ^ now);
        const auto half = changed >> 1;
        c.s.wr<1>(reg::xer_ca, (std::uint32_t(changed) >> 31) & (changed & 1));
        starts_later = half - (changed & day) < 0;
        c.set({{3, std::uint32_t(half)}, {4, extended(day)}});
    }
    c.set(0, starts_later);
    if (compare(c, starts_later) != 0) {
        caption(0x100995f4, 0xe32);
        cell(0x10099630, 1, 1, c.u16(field), 1, text);
        call.call(0x10715e00, 0x10099644, {draft + 0x28, text, 2000, 0xff});
        cell(0x10099680, 3, 1, 0, 12, text);
        advance();
    }
    caption(0x10099698, 0xe46);
    cell(0x100996d4, 1, 1, c.u16(field), 1, text);
    const auto start_month = std::int8_t(read(0x100996e0, 9));
    c.set(0, std::uint32_t(start_month));
    c.put8(frame.at(0x902), start_month);
    const auto start_year = std::uint16_t(read(0x100996f4, 0xa));
    c.set(0, start_year);
    c.put16(frame.at(0x900), start_year);
    const auto month = std::int8_t(read(0x10099708, 0xb));
    c.set(13, std::uint32_t(month));
    const auto year = std::uint16_t(read(0x10099718, 0xc));
    auto prior_year = int(year), prior_month = int(month);
    const auto base_year = c.u16(draft + 0x2a);
    c.set({{0, base_year + 1u}, {4, base_year}, {16, year}, {19, year}, {21, std::uint32_t(month)}});
    bool whole_year = false;
    if (compare(c, year, base_year + 1) == 0) {
        const auto threshold = std::int8_t(call.call(date_month, 0x1009973c, {draft + 0x28}));
        c.set(0, std::uint32_t(threshold));
        whole_year = compare(c, month, threshold) > 0;
    }
    if (!whole_year) {
        const auto latest = c.u16(draft + 0x2a);
        c.set({{0, latest + 1u}, {3, latest}});
        whole_year = compare(c, year, latest + 1) > 0;
    }
    if (whole_year) {
        --prior_year;
    } else {
        --prior_month;
        c.set(0, std::uint32_t(std::int8_t(prior_month)));
        if (compare(c, std::int8_t(prior_month)) < 0) {
            prior_month = 11;
            --prior_year;
        }
    }
    const auto test_year = std::uint16_t(prior_year);
    c.set({{0, start_year}, {3, test_year}, {16, std::uint32_t(prior_year)}, {21, std::uint32_t(prior_month)}});
    bool lower = false;
    if (compare_logical(c, test_year, start_year) == 0) {
        c.set({{0, std::uint8_t(start_month)}, {3, std::uint32_t(std::int8_t(prior_month))}});
        lower = compare(c, std::int8_t(prior_month), std::uint8_t(start_month)) <= 0;
    }
    if (!lower) {
        c.set({{0, start_year}, {3, test_year}});
        lower = compare_logical(c, test_year, start_year) < 0;
        if (!lower) {
            const auto term = kind(c.u8(draft + term_field));
            c.set(0, std::uint32_t(term));
            lower = compare(c, term, 1) == 0;
        }
    }
    std::uint32_t decrement;
    if (lower) {
        decrement = adjust_button(0x100997f0, 2, 0xe1d, false);
    } else {
        bool toggle = false;
        const auto contract_year = c.u16(draft + 0x2a);
        c.set(0, contract_year);
        if (compare_logical(c, year, contract_year) == 0) {
            const auto next_month = std::int8_t(call.call(date_month, 0x1009980c, {draft + 0x28})) + 1;
            c.set({{0, std::uint32_t(next_month)}, {3, std::uint32_t(next_month - 1)}});
            toggle = compare(c, month, next_month) == 0;
        }
        if (!toggle) {
            const auto latest = c.u16(draft + 0x2a);
            c.set({{0, latest + 1u}, {3, latest}});
            if (compare(c, year, latest + 1) == 0) {
                c.set(0, std::uint32_t(month));
                if (compare(c, month) == 0) {
                    const auto last = std::int8_t(call.call(date_month, 0x1009983c, {draft + 0x28}));
                    c.set(0, std::uint32_t(last));
                    toggle = compare(c, last, 11) == 0;
                }
            }
        }
        if (toggle) {
            decrement = adjust_button(0x1009988c, 2, 0xe1d, true, 0x1d);
        } else {
            const auto term = kind(c.u8(draft + term_field));
            c.set(0, std::uint32_t(term));
            const bool enabled = compare(c, term, 1) != 0;
            decrement = adjust_button(enabled ? 0x1009992c : 0x100998e0, 2, 0xe1d, enabled, 0xa);
        }
    }
    write_widget(0x10099944, 0x11, decrement);
    const auto term = kind(c.u8(draft + term_field));
    c.set(0, std::uint32_t(term));
    if (compare(c, term, 1) == 0) {
        caption(0x10099968, 0xe57);
    } else {
        const auto name = call.call(0x107188a0, 0x10099974, {std::uint32_t(month)});
        caption(0x10099988, 0xe66, {name, year});
    }
    cell(0x100999c4, 3, 1, 0, 12, text);
    const auto upper = kind(c.u8(draft + term_field));
    c.set({{0, std::uint32_t(upper)}, {5, std::uint32_t(upper)}});
    bool last_year = false;
    if (compare(c, upper, 2) == 0) {
        const auto settings = c.toc(-0x7990);
        const auto today = c.u16(c.toc(-0x7b80) + 2);
        const auto remaining = year - c.i8(settings);
        c.set({{0, today}, {3, std::uint32_t(remaining)}, {4, settings}});
        last_year = compare(c, remaining, today) >= 0;
    }
    if (last_year) {
        adjust_button(0x10099a30, 4, 0xe1f, false);
    } else {
        c.set(0, std::uint32_t(upper));
        const bool rolling = compare(c, upper, 1) == 0;
        adjust_button(rolling ? 0x10099a84 : 0x10099acc, 4, 0xe1f, true, rolling ? 0x1d : 9);
    }
    adjust_button(0x10099b10, 5, 0xe21, true, 0xb);
}

bool BasicScreen::same_club() const {
    const auto employer = c.u32(person + 0x24);
    c.set(3, employer);
    if (compare_logical(c, employer) == 0) return false;
    const auto id = c.u32(employer), target = c.u32(frame.at(0x914));
    c.set({{0, target}, {3, id}});
    return compare(c, id, target) == 0;
}

void BasicScreen::options() const {
    advance(1, 5);
    caption(0x10099b28, 0xe92);
    cell(0x10099b58, 1, 1, 0, 1, text, false);
    if (restricted(c.u8(draft + term_field))) caption(0x10099b78, 0xea0);
    else caption(0x10099b8c, 0xeaa, {strings + 0x214});
    cell(0x10099bcc, 3, 1, c.u16(field), 12, text, true, 0x1e);

    advance();
    if (same_club()) return;
    const auto link = c.u32(frame.at(0x918));
    c.set(0, link);
    if (compare_logical(c, link) != 0) return;
    caption(0x10099c0c, 0xed2);
    cell(0x10099c4c, 1, 1, 0, 1, text, true, 0x1a);
    const auto choice = std::int8_t(read(0x10099c58, 7));
    c.set(0, std::uint32_t(choice));
    const bool enabled = compare(c, choice) != 0;
    caption(enabled ? 0x10099cbc : 0x10099c6c, enabled ? 0x83 : 0x80);
    cell(enabled ? 0x10099cfc : 0x10099cac, 3, 1, 0, 12, text, true, 0x1a, enabled ? 0 : 1);
}

void BasicScreen::presets() const {
    constexpr std::array<unsigned, 8> constant_offsets{0x88, 0x98, 0xa0, 0xa8, 0xb0, 0xb8, 0xc0, 0xc8};
    constexpr std::array<unsigned, 8> scratch_offsets{0x8f0, 0x8f8, 0x8e8, 0x8e0, 0x8d8, 0x8d0, 0x8c8, 0x8c0};
    Fpu fp(c);
    const auto scale = c.f64(constants + 0x90);
    c.set_f(8, scale);
    for (unsigned index = 0; index < 8; ++index)
        c.set_f(7 - index, fp.fmul(fee_cap, c.f64(constants + constant_offsets[index])));
    for (unsigned index = 0; index < 8; ++index)
        c.set_f(7 - index, fp.fmul(c.f(7 - index), scale));
    for (unsigned index = 0; index < 8; ++index) {
        const auto rounded = fp.fctiwz(7 - index, c.f(7 - index));
        c.put64(frame.at(scratch_offsets[index]), rounded);
        c.set(index == 7 ? 0 : 9 - index, std::uint32_t(rounded));
    }
    c.set(0, c.u32(frame.at(0x8c4)));
    for (unsigned index = 0; index < 8; ++index)
        c.put32(frame.at(0xd0 + 4 * index), c.r(index == 7 ? 0 : 9 - index));

    const auto popover = [&](std::uint32_t back, std::uint32_t handle) {
        words({0, 0x130, c.u16(field), handle});
        return call.call(grid_control, back, {layout(), 0, 0, 0, 0, 0, 0, 0});
    };
    const auto item = [&](std::uint32_t back, std::uint32_t grid_id, unsigned action, std::uint32_t value) {
        const auto style = c.u16(c.r(24)), selection = c.u16(selected);
        c.set({{11, style}, {12, selection}});
        words({16, c.u16(field), selection, 1, 1, style, text, 0, action, value, grid_id});
        call.call(0x10719740, back, {layout(true), 2, 0, 0, 0, 0, 0, 0});
    };

    if (compare(c, c.u32(frame.at(0xd0)), 100) > 0) {
        const auto fee_grid = popover(0x10099e38, c.u32(frame.at(0x920)));
        c.set(13, fee_grid);
        for (unsigned i = 0;; ++i) {
            c.set({{0, i}, {15, i}});
            if (compare(c, i, 8) >= 0) break;
            const auto value = call.call(0x1000a330, 0x10099e58, {frame.at(0x6c), i, frame.at(0xd0), 8});
            c.put32(frame.at(0xfc), value);
            call.call(contract_text, 0x10099e70, {frame.at(0xf0), text, 0, 2});
            item(0x10099eec, fee_grid, 8, c.u32(frame.at(0xfc)));
        }
    }
    const auto wage_handle = c.u32(frame.at(0x924));
    c.set(0, extended(std::int16_t(wage_handle)));
    auto wage_grid = 0xffffffffu;
    if (compare(c, std::int16_t(wage_handle), -1) != 0) {
        constexpr std::uint32_t max_preset_wage = 3'000'000;
        constexpr auto reciprocal = std::int32_t(0x92492493u);
        c.set({{3, 0x92490000}, {4, max_preset_wage}, {6, std::uint32_t(reciprocal)}, {7, 0}});
        const auto cap = compare(c, wage_cap, max_preset_wage) < 0 ? wage_cap : max_preset_wage;
        for (unsigned i = 0;; ++i) {
            c.set({{0, i}, {3, frame.at(0xb0 + 4 * i)}, {7, i}});
            if (compare(c, i, 8) >= 0) break;
            const auto product = std::int32_t(i * cap);
            const auto high = std::int32_t((std::int64_t(product) * reciprocal) >> 32);
            const auto added = std::int32_t(std::uint32_t(high) + std::uint32_t(product));
            const auto quotient = added >> 2;
            c.s.wr<1>(reg::xer_ca, (std::uint32_t(added) >> 31) & unsigned((added & 3) != 0));
            c.set(5, std::uint32_t(quotient) >> 31);
            c.put32(frame.at(0xb0 + 4 * i), std::uint32_t(quotient) + c.r(5));
        }
        const auto first = c.u32(frame.at(0xb0)), second = c.u32(frame.at(0xb4));
        c.set({{0, second - first}, {3, first}});
        if (compare(c, second - first, 1000) > 0) {
            wage_grid = popover(0x1009a000, wage_handle);
            c.set(18, wage_grid);
        }
    }
    c.set(0, extended(std::int16_t(wage_grid)));
    if (compare(c, std::int16_t(wage_grid), -1) == 0) return;
    for (unsigned i = 0;; ++i) {
        c.set({{0, i}, {13, i}});
        if (compare(c, i, 8) >= 0) break;
        const auto local = frame.at(0x6c);
        call.call(0x1000a330, 0x1009a030, {local, i, frame.at(0xb0), 8});
        detail::money_text(c, call, text, local, {0x1009a03c, 0x1009a054}, 5);
        item(0x1009a0d0, wage_grid, 0x19, c.u32(local));
    }
}

void BasicScreen::notes() const {
    const auto enabled = c.i8(frame.at(0x904));
    c.set(0, std::uint32_t(enabled));
    const bool inspect = compare(c, enabled) != 0;
    advance(2);
    if (inspect && compare_logical(c, current_contract(0x1009a108)) != 0) {
        const auto latest = current_contract(0x1009a120);
        const auto status = std::int8_t(call.call(0x10380710, 0x1009a124, {latest}));
        c.set(0, std::uint32_t(status));
        if (compare(c, status) == 0) {
            const auto first = frame.at(0x98), second = frame.at(0x90);
            call.call(0x10715880, 0x1009a134, {first});
            call.call(0x10715880, 0x1009a13c, {second});
            if (same_club()) caption(0x1009a174, 0xfb2);
            else caption(0x1009a164, 0xee4);
            assign(0x1009a180);
            call.call(0x103fb130, 0x1009a194, {c.u32(c.toc(-0x7b40)), first, second});
            const auto calendar = c.toc(-0x7b80);
            const auto front = c.u32(calendar), back = c.u32(calendar + 4);
            c.put32(frame.at(0x80), front);
            c.put32(frame.at(0x84), back);
            const auto year = c.u16(frame.at(0x82)), date_year = c.u16(first + 2);
            c.set({{0, date_year}, {3, year}, {5, back}});
            bool same_date = false;
            if (compare_logical(c, year, date_year) == 0) {
                const auto day = c.i16(frame.at(0x80)), date_day = c.i16(first);
                c.set({{0, extended(date_day)}, {3, extended(day)}});
                same_date = compare(c, day, date_day) == 0;
            }
            c.set(4, same_date);
            const auto date_text = frame.at(0x1b8);
            compare(c, same_date);
            call.call(0x10716eb0, same_date ? 0x1009a1ec : 0x1009a214, {same_date ? second : first, date_text, 200, 0xff});
            caption(same_date ? 0x1009a1fc : 0x1009a224, same_date ? 0x101b : 0x104e, {date_text});
            call.call(0x1074b960, 0x1009a230, {frame.at(0xa0), text});
            draw_box(0x1009a25c);
            return;
        }
    }
    const auto active = c.i8(frame.at(0x904));
    c.set(0, std::uint32_t(active));
    if (compare(c, active) == 0 || compare_logical(c, current_contract(0x1009a27c)) == 0) return;
    const auto latest = current_contract(0x1009a294);
    const auto status = std::int8_t(call.call(0x10380710, 0x1009a298, {latest}));
    c.set(0, std::uint32_t(status));
    if (compare(c, status) == 0) return;
    if (same_club()) caption(0x1009a2d8, 0x1159);
    else caption(0x1009a2c8, 0x107e);
    assign(0x1009a2e4);
    draw_box(0x1009a310);
    advance(2, 5);
    caption(0x1009a328, 0x11be);
    cell(0x1009a358, 1, 1, 0, 1, text, false);
    const auto mode = c.u8(frame.at(0x905));
    c.set(0, mode);
    const bool alternative = compare(c, mode, 3) == 0;
    caption(alternative ? 0x1009a370 : 0x1009a3c0, alternative ? 0x11cc : 0x11dc);
    cell(alternative ? 0x1009a3b0 : 0x1009a400, 3, 1, 0, 12, text, true, 0x30, alternative ? 0 : 3);
    advance();
}
}

void draw_negotiation_basic(Cpu& c, std::uint32_t draft, std::uint32_t proposal, std::uint32_t person,
                           std::uint32_t top, std::uint32_t bottom) {
    Frame frame(c, 0x10098130, 0x990, {.first_gpr = 13, .gpr_offset = -0x5c, .first_fpr = 30});
    BasicScreen screen{c, frame, Program{c}, c.toc(-0x7bb0), c.toc(-0x56cc), c.toc(-0x7bb8), c.toc(-0x56d0),
                       c.toc(-0x7bc8), c.toc(-0x7bdc), c.toc(-0x7bcc), c.toc(-0x7bd0),
                       draft, proposal, person, top, bottom};
    keep(c, {{13, top}, {14, screen.selected}, {15, draft}, {16, proposal}, {17, person}, {21, bottom},
             {23, screen.accent}, {24, c.toc(-0x7bd4)}, {25, screen.field}, {26, screen.constants},
             {27, screen.ink}, {28, screen.strings}, {29, screen.text}, {30, screen.screens}});
    screen.initialize();
    screen.reputation_threshold();
    if (screen.qualifies_for_short_message()) screen.short_message();
    else screen.general_message();
    screen.draft_message();
    screen.money_controls();
    screen.duration_controls();
    screen.options();
    screen.presets();
    screen.notes();
    screen.call.call(0x1074b7c0, 0x1009a418, {frame.at(0xa0), 0xffffffff});
}
}
