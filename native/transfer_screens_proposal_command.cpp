#include "transfer_screens_detail.hpp"
#include "guest_fpu.hpp"

namespace cm::aot::transfer_screens {
using namespace detail;
namespace {
// The editor shares one live draft with the manager record. The original keeps
// several temporary values in the same preserved registers on disjoint paths.
struct ProposalCommand {
    Cpu& c;
    Frame frame;
    Program p;
    Kept<13> temporary;
    Kept<14> contract_or_control;
    Kept<15> control_or_limit_low;
    Kept<16> person_or_over_limit;
    Kept<17> revision_or_amount;
    Kept<18> link;
    Kept<19> draft_slot;
    Kept<20> draft;
    Kept<21> constants;
    Kept<22> over_limit_colour;
    Kept<23> disabled_colour;
    Kept<24> finances;
    Kept<25> enabled_colour;
    Kept<26> text;
    Kept<27> pool;
    Kept<28> people;
    Kept<29> transfers;
    Kept<30> screens;
    Kept<31> valid_contract;

    ProposalCommand(Cpu& cpu, std::uint32_t selected)
        : c(cpu), frame(c, 0x1041b600, 0x100, {.first_gpr = 13}), p{c},
          temporary(c, c.r(13)), contract_or_control(c, c.toc(slot::current_contract)),
          control_or_limit_low(c, selected), person_or_over_limit(c, c.r(16)),
          revision_or_amount(c, c.r(17)), link(c, c.r(18)), draft_slot(c, c.r(19)), draft(c, c.r(20)),
          constants(c, c.toc(-0x4ad4)), over_limit_colour(c, c.toc(-0x7a74)),
          disabled_colour(c, c.toc(-0x7bc4)), finances(c, c.toc(-0x7ab8)),
          enabled_colour(c, c.toc(-0x7bc8)), text(c, c.toc(-0x7bb8)), pool(c, c.toc(slot::pool)),
          people(c, c.toc(slot::people)), transfers(c, c.toc(slot::transfers)),
          screens(c, c.toc(slot::screens)), valid_contract(c, c.r(31)) {
        c.set(5, c.toc(-0x7a54));
        draft_slot = p.call(0x10148930, 0x1041b650, {c.u32(c.r(5)), 0});
        draft = c.u32(draft_slot);
        link = history(0x1041b668, 1);
        revision_or_amount = history(0x1041b67c, 3);
        c.set({{4, std::uint32_t(c.i16(screens + 0x2c58)) * 0x2c0},
               {5, c.u32(draft + 12)}, {6, c.u32(people)}});
        person_or_over_limit = c.r(6) + c.r(5) * 0x70;
    }
    std::uint32_t history(std::uint32_t back, unsigned index) {
        return p.call(0x1073ef70, back, {screens, index});
    }
    bool is_action(int action) {
        return compare(c, control_action(c, screens, control_or_limit_low), action) == 0;
    }
    std::uint32_t payload() {
        const bool absent = compare(c, std::int16_t(std::uint32_t(control_or_limit_low)), -1) == 0;
        c.set(3, c.u32(screens + std::uint32_t(c.i16(screens + 0x2c58)) * 0x2c0 + 0x24));
        if (absent) return 0;
        c.set(3, c.r(3) + 0xc0000 + std::uint32_t(std::int16_t(std::uint32_t(control_or_limit_low))) * 0x17c);
        return c.u32(c.r(3) - 0x5754);
    }
    void modified(std::uint32_t first, std::uint32_t second) {
        p.history(first, screens, 2, 1); p.history(second, screens, 5, 1);
    }
    std::uint32_t refresh(std::uint32_t total_back, std::uint32_t redraw_back) {
        enter(c, total_back, {draft, link}); update_proposal_total(c, draft, link);
        p.call(0x10742ec0, redraw_back, {screens, 0, 0}); return result(c, 0);
    }
    void button(std::uint32_t back, std::uint32_t control, bool enabled, unsigned action) {
        p.call(0x10740620, back, {screens, control, enabled ? 2u : 6u});
        p.call(0x10741a80, back + 0x10, {screens, control, c.u16(enabled ? enabled_colour : disabled_colour)});
        p.call(0x10742100, back + 0x20, {screens, control, enabled ? action : 0u});
    }
    void limit(std::uint32_t lookup_back, std::uint32_t quote_back) {
        const auto manager = c.u32(c.u32(screens + std::uint32_t(c.i16(screens + 0x2c58)) * 0x2c0 + 4) + 0x24);
        p.call(0x10103510, lookup_back, {c.u32(finances), manager});
        p.call(0x100f5770, quote_back, {c.r(3), person_or_over_limit, 0, 0, 1, 0});
    }
    bool is_bidder_manager() {
        c.set(5, c.u32(screens + std::uint32_t(c.i16(screens + 0x2c58)) * 0x2c0 + 4));
        c.set({{3, c.u32(draft + 8) * 0x248}, {4, indirect(c, slot::clubs)}});
        c.set(4, c.r(4) + c.r(3));
        return compare_logical(c, c.u32(c.r(5) + 0x24), c.r(4)) == 0;
    }
    bool valid_active(std::uint32_t history_back) {
        history(history_back, 3); c.set(4, c.u32(link));
        if (compare(c, c.i32(c.r(4) + 0x2c), std::int32_t(c.r(3))) != 0) return false;
        c.set(3, c.u32(link));
        if (compare(c, c.i8(c.r(3) + 0x34)) == 0) return true;
        c.set(3, c.u32(link)); return compare(c, c.u8(c.r(3) + 0x34), 1) == 0;
    }
    std::uint32_t previous(std::uint32_t destroy_back, std::uint32_t history_back, std::uint32_t open_back,
                           bool use_temporary = false) {
        p.call(0x10411f10, destroy_back, {c.u32(draft_slot), 1}); c.put32(draft_slot, 0);
        const auto revision = history(history_back, 3);
        const auto offer = use_temporary ? std::uint32_t(temporary) : c.u32(link);
        enter(c, open_back, {offer, revision}); open_previous_offer(c, offer, revision);
        return result(c, std::uint32_t(-11));
    }
    void reset_draft(bool negotiated) {
        p.call(0x10411f10, negotiated ? 0x1041e3e8 : 0x1041e188, {c.u32(draft_slot), 1});
        contract_or_control = p.call(0x10773520, negotiated ? 0x1041e3f0 : 0x1041e190, {0x54});
        if (compare(c, contract_or_control) != 0)
            p.call(0x10411e60, negotiated ? 0x1041e400 : 0x1041e1a0, {contract_or_control});
        c.put32(draft_slot, contract_or_control);
        const auto source = negotiated ? c.u32(temporary) : std::uint32_t(temporary);
        c.set(3, c.u32(draft_slot));
        copy_offer_fields(c, c.r(3), source);
        c.set(4, c.u8(source + 0x51));
        if (negotiated) c.set(5, c.u8(source + 0x50));
        c.set(3, c.u32(draft_slot)); c.put32(c.r(3), 0);
        c.set(3, c.u32(draft_slot));
        if (compare_logical(c, c.u32(c.r(3) + 4)) != 0) {
            contract_or_control = p.call(0x10773520, negotiated ? 0x1041e508 : 0x1041e2a4, {0x10});
            if (compare(c, contract_or_control) != 0)
                p.call(0x10738c00, negotiated ? 0x1041e518 : 0x1041e2b4, {contract_or_control});
            c.set(3, c.u32(draft_slot)); c.put32(c.r(3) + 4, contract_or_control);
            c.set(3, c.u32(draft_slot));
            p.call(0x10739010, negotiated ? 0x1041e534 : 0x1041e2cc,
                   {c.u32(c.r(3) + 4), c.u32((negotiated ? c.u32(temporary) : std::uint32_t(temporary)) + 4)});
        }
        p.history(negotiated ? 0x1041e548 : 0x1041e2e0, screens, negotiated ? 5 : 2, 0);
    }
    std::uint32_t edit_fee(unsigned action);
    std::uint32_t edit_small_term(unsigned action);
    void toggle_conditional_fee(bool international) {
        const auto field = international ? 0x48u : 0x40u;
        modified(international ? 0x1041d834 : 0x1041d094, international ? 0x1041d848 : 0x1041d0a8);
        c.put32(draft + field, 0u - c.u32(draft + field));
        if (compare(c, c.i32(draft + field)) != 0) return;
        Fpu fp(c);
        const auto quarter_value = [&](unsigned scratch) {
            c.set({{3, c.u32(people) + c.u32(draft + 12) * 0x70}, {4, c.u32(people)}});
            c.put32(frame.at(scratch), 0x43300000);
            c.set_f(1, c.f64(constants + 0x10)); c.set_f(2, c.f64(constants + 8));
            c.put32(frame.at(scratch + 4), c.u32(c.r(3) + 8) ^ 0x80000000u);
            return fp.fmul(c.f(2), fp.fsub(c.f64(frame.at(scratch)), c.f(1)));
        };
        c.set_f(3, c.f64(constants));
        auto amount = quarter_value(international ? 0x94 : 0xa4);
        fp.fcmp(0, c.f(3), amount);
        if (c.f(3) <= amount) amount = quarter_value(0x9c);
        else amount = c.f64(constants);
        const auto result_slot = international ? 0xa4u : 0x94u;
        c.put64(frame.at(result_slot), fp.fctiwz(0, amount));
        c.put32(draft + field, c.u32(frame.at(result_slot + 4)));
    }
    void cycle_appearances(bool international) {
        modified(international ? 0x1041d968 : 0x1041d1c8, international ? 0x1041d97c : 0x1041d1dc);
        const auto address = draft + (international ? 0x4cu : 0x44u);
        if (compare(c, c.u8(address), 1) == 0) c.put8(address, 10);
        else { c.set(3, c.u8(address)); c.put8(address, c.r(3) + 10); }
        if (compare(c, c.i8(address), 50) > 0) c.put8(address, 1);
    }
    std::uint32_t run();
};

std::uint32_t ProposalCommand::edit_small_term(unsigned action) {
    const bool installments = action == 8 || action == 9;
    const bool increase = action == 8 || action == 11;
    const auto field = installments ? 0x51u : 0x50u;
    const auto step = installments ? 6u : 5u;
    const auto minimum = step;
    const auto maximum = installments ? 24 : 25;
    const auto down_action = installments ? 9u : 12u, up_action = down_action - 1;
    // Each block uses the same control layout, with distinct original call sites.
    const auto first = installments ? (increase ? 0x1041ca90u : 0x1041c900u)
                                    : (increase ? 0x1041ce60u : 0x1041ccd0u);
    modified(first, first + 0x14);
    c.set(5, c.u8(draft + field)); c.put8(draft + field, c.r(5) + (increase ? step : 0u - step));
    temporary = extended(std::int16_t(history(first + 0x2c, installments ? 16 : 17)));
    temporary = temporary + 1;
    if (increase) button(first + 0x48, temporary, true, down_action);
    else {
        const bool enabled = compare(c, c.u8(draft + field), minimum) != 0;
        button(first + (enabled ? 0x84 : 0x50), temporary, enabled, down_action);
    }
    temporary = temporary + 1;
    const auto format_back = first + (increase ? 0x80 : 0xbc);
    p.call(installments ? 0x10727890 : 0x1077b720, format_back,
           {text, pool + (installments ? 0x4c4u : 0x4f9u), std::uint32_t(std::int32_t(c.i8(draft + field)))});
    p.call(0x10741da0, format_back + 0x10, {screens, temporary, text});
    temporary = temporary + 1;
    if (increase) {
        const bool enabled = compare(c, c.i8(draft + field), maximum) < 0;
        button(first + (enabled ? 0xe8 : 0xb4), temporary, enabled, up_action);
    } else button(first + 0xe0, temporary, true, up_action);
    return refresh(first + (increase ? 0x114 : 0x10c), first + (increase ? 0x124 : 0x11c));
}

std::uint32_t ProposalCommand::edit_fee(unsigned action) {
    struct Sites {
        unsigned field, history_index, quote_slot, amount_slot, comparison_slot;
        bool increase, new_flag_before_modified;
        std::uint32_t limit_lookup, quote, modified_first, modified_second, new_flag;
        std::uint32_t step, history_control, down_disabled, down_enabled;
        std::uint32_t convert, format, set_text, compare_limit, compare_amount, warning_colour, normal_colour;
        std::uint32_t up_disabled, up_enabled, total, redraw;
    };
    static constexpr Sites base_increase{0x20,15,0x90,0x8c,0x5c,true,false,
        0x1041c010,0x1041c028,0x1041c040,0x1041c054,0x1041c060,0x1041c078,0x1041c08c,0,0x1041c0a8,
        0x1041c0d8,0x1041c0f0,0x1041c100,0x1041c128,0x1041c13c,0x1041c174,0x1041c188,
        0x1041c1b0,0x1041c1e4,0x1041c210,0x1041c220};
    static constexpr Sites base_decrease{0x20,15,0x88,0x84,0x58,false,true,
        0x1041c29c,0x1041c2b4,0x1041c2e0,0x1041c2f4,0x1041c2c4,0x1041c308,0x1041c31c,0x1041c340,0x1041c374,
        0x1041c3a4,0x1041c3bc,0x1041c3cc,0x1041c3f4,0x1041c408,0x1041c440,0x1041c454,
        0,0x1041c468,0x1041c494,0x1041c4a4};
    static constexpr Sites league_decrease{0x40,18,0x7c,0x78,0x4c,false,true,
        0x1041d294,0x1041d2ac,0x1041d2d8,0x1041d2ec,0x1041d2bc,0x1041d300,0x1041d314,0x1041d334,0x1041d368,
        0x1041d398,0x1041d3b0,0x1041d3c0,0x1041d3e8,0x1041d3fc,0x1041d434,0x1041d448,
        0,0x1041d45c,0x1041d488,0x1041d498};
    static constexpr Sites league_increase{0x40,18,0x74,0x70,0x48,true,true,
        0x1041d514,0x1041d52c,0x1041d558,0x1041d56c,0x1041d53c,0x1041d580,0x1041d594,0,0x1041d5ac,
        0x1041d5dc,0x1041d5f4,0x1041d604,0x1041d62c,0x1041d640,0x1041d678,0x1041d68c,
        0x1041d6b4,0x1041d6e8,0x1041d714,0x1041d724};
    static constexpr Sites international_decrease{0x48,19,0x6c,0x68,0x44,false,false,
        0x1041da34,0x1041da4c,0x1041da64,0x1041da78,0x1041da84,0x1041da9c,0x1041dab0,0x1041dad0,0x1041db04,
        0x1041db34,0x1041db4c,0x1041db5c,0x1041db84,0x1041db98,0x1041dbd0,0x1041dbe4,
        0,0x1041dbf8,0x1041dc24,0x1041dc34};
    static constexpr Sites international_increase{0x48,19,0x64,0x60,0x40,true,false,
        0x1041dcb0,0x1041dcc8,0x1041dce0,0x1041dcf4,0x1041dd00,0x1041dd18,0x1041dd2c,0,0x1041dd44,
        0x1041dd74,0x1041dd8c,0x1041dd9c,0x1041ddc4,0x1041ddd8,0x1041de10,0x1041de24,
        0x1041de4c,0x1041de80,0x1041deac,0x1041debc};
    const auto& sites = action == 3 ? base_increase : action == 4 ? base_decrease :
                        action == 16 ? league_decrease : action == 15 ? league_increase :
                        action == 21 ? international_decrease : international_increase;
    const bool base = sites.field == 0x20;
    const auto down_action = base ? 4u : sites.field == 0x40 ? 16u : 21u;
    const auto up_action = down_action - 1;
    limit(sites.limit_lookup, sites.quote); c.put32(frame.at(sites.quote_slot), c.r(4));
    const auto keep_new_flag = [&] {
        const auto flag = std::uint32_t(std::int32_t(std::int8_t(history(sites.new_flag, 6))));
        if (base) temporary = flag; else contract_or_control = flag;
    };
    if (sites.new_flag_before_modified) keep_new_flag();
    modified(sites.modified_first, sites.modified_second);
    if (!sites.new_flag_before_modified) keep_new_flag();
    c.put32(frame.at(sites.amount_slot), c.u32(draft + sites.field));
    p.call(sites.increase ? 0x1000a060 : 0x1000a170, sites.step, {frame.at(sites.amount_slot), 4});
    c.put32(draft + sites.field, c.u32(frame.at(sites.amount_slot)));
    const auto control = extended(std::int16_t(history(sites.history_control, sites.history_index)));
    if (base) contract_or_control = control + 1; else temporary = control;
    const auto current_control = [&]() -> std::uint32_t { return base ? contract_or_control : temporary; };
    const auto next_control = [&] { if (base) contract_or_control = contract_or_control + 1; else temporary = temporary + 1; };
    bool down_enabled = true;
    if (!sites.increase) down_enabled = compare(c, c.i32(draft + sites.field), base ? 0 : 1000) > 0;
    button(down_enabled ? sites.down_enabled : sites.down_disabled, current_control(), down_enabled, down_action);
    next_control();
    p.call(0x10009f20, sites.convert, {frame.at(sites.amount_slot), 5});
    p.call(0x1000b410, sites.format, {text, 2000, c.r(3), c.r(4), 5});
    p.call(0x10741da0, sites.set_text, {screens, current_control(), text});
    const bool check_limit = compare(c, std::int8_t(base ? std::uint32_t(temporary) : std::uint32_t(contract_or_control))) != 0;
    person_or_over_limit = 0;
    if (check_limit) {
        draft_slot = 1; revision_or_amount = frame.at(sites.comparison_slot);
        c.put32(frame.at(sites.comparison_slot), c.u32(draft + sites.field));
        p.call(0x10009f20, sites.compare_limit, {frame.at(sites.quote_slot), 4});
        control_or_limit_low = c.r(4);
        if (base) temporary = c.r(3); else contract_or_control = c.r(3);
        p.call(0x10009f20, sites.compare_amount, {revision_or_amount, 4});
        const auto limit_high = base ? std::uint32_t(temporary) : std::uint32_t(contract_or_control);
        const auto maximum = std::bit_cast<std::int64_t>((std::uint64_t(limit_high) << 32) | std::uint32_t(control_or_limit_low));
        const auto amount = std::bit_cast<std::int64_t>((std::uint64_t(c.r(3)) << 32) | c.r(4));
        const bool over = maximum < amount;
        c.set({{5, limit_high ^ 0x80000000u}, {3, over}});
        c.s.wr<1>(reg::xer_ca, !over); compare(c, c.r(3));
        if (over) person_or_over_limit = 1;
    }
    const bool over = compare(c, person_or_over_limit) != 0;
    p.call(0x10741a80, over ? sites.warning_colour : sites.normal_colour,
           {screens, current_control(), c.u16(over ? over_limit_colour : enabled_colour)});
    next_control();
    bool up_enabled = true;
    if (sites.increase) { c.set(3, 100000000); up_enabled = compare(c, c.i32(draft + sites.field), 100000000) < 0; }
    button(up_enabled ? sites.up_enabled : sites.up_disabled, current_control(), up_enabled, up_action);
    return refresh(sites.total, sites.redraw);
}

std::uint32_t ProposalCommand::run() {
    bool close = is_action(33);
    if (!close) {
        valid_contract = 0;
        const auto contract = p.call(0x10082f20, 0x1041b6fc, {indirect(c, slot::contracts), person_or_over_limit});
        compare_logical(c, contract); c.put32(contract_or_control, contract);
        if (contract) {
            c.set(3, c.u32(contract_or_control));
            if (compare(c, c.i8(c.r(3) + 0x38) >> 2, 5) != 0) valid_contract = 1;
        }
        std::uint32_t club = 0;
        if (compare(c, valid_contract) != 0) {
            c.set({{4, c.u32(contract_or_control)}, {3, indirect(c, slot::clubs)}});
            club = c.r(3) + c.u32(c.r(4) + 4) * 0x248;
        }
        close = compare_logical(c, club) == 0;
    }
    if (close) {
        p.call(0x10411f10, 0x1041b764, {draft, 1}); c.put32(draft_slot, 0);
        return result(c, std::uint32_t(-9));
    }
    if (is_action(39)) {
        p.call(0x10411f10, 0x1041b7d8, {draft, 1}); c.put32(draft_slot, 0);
        const auto offer_id = payload();
        const auto selected = p.call(0x103eb440, 0x1041b820, {c.u32(transfers), offer_id});
        enter(c, 0x1041b828, {selected, 0}); open_existing_offer(c, selected, 0);
        return result(c, std::uint32_t(-11));
    }
    const bool checks_revision = is_action(35) || is_action(2) || is_action(1) ||
                                 is_action(27) || is_action(28) || is_action(29);
    if (checks_revision && compare_logical(c, link) != 0) {
        c.set(3, c.u32(link));
        bool stale = compare(c, c.i32(c.r(3) + 0x2c), revision_or_amount) != 0;
        if (!stale) { c.set(3, c.u32(link)); stale = compare(c, c.u8(c.r(3) + 0x34), 2) == 0; }
        if (!stale) { c.set(3, c.u32(link)); stale = compare(c, c.u8(c.r(3) + 0x34), 3) == 0; }
        if (stale) {
            p.call(0x10411f10, 0x1041ba84, {draft, 1}); c.put32(draft_slot, 0);
            const auto active_offer = c.u32(link);
            enter(c, 0x1041ba98, {active_offer, revision_or_amount}); open_previous_offer(c, active_offer, revision_or_amount);
            return result(c, std::uint32_t(-11));
        }
    }
    if (is_action(35)) {
        if (byte_result(c, p.call(0x103f4d10, 0x1041bb0c, {c.u32(transfers), c.u32(link), 0, frame.at(0x3e)}))) {
            const auto reason = extended(c.i16(frame.at(0x3e)));
            enter(c, 0x1041bb24, {draft_slot, 35, reason}); open_offer_restriction(c, draft_slot, 35, reason);
        } else { enter(c, 0x1041bb3c, {draft_slot, 35, link}); open_offer_context(c, draft_slot, 35, link); }
        return result(c, std::uint32_t(-4));
    }
    if (is_action(2)) {
        if (!byte_result(c, history(0x1041bba8, 6)) &&
            byte_result(c, p.call(0x103f4d10, 0x1041bbc4, {c.u32(transfers), c.u32(link), 0, frame.at(0x3c)}))) {
            const auto reason = extended(c.i16(frame.at(0x3c)));
            enter(c, 0x1041bbdc, {draft_slot, 27, reason}); open_offer_restriction(c, draft_slot, 27, reason);
        } else { enter(c, 0x1041bbf4, {draft_slot, 2, link}); open_offer_context(c, draft_slot, 2, link); }
        return result(c, std::uint32_t(-4));
    }
    if (is_action(1)) {
        enter(c, 0x1041bc64, {draft_slot, 1, 0}); open_offer_context(c, draft_slot, 1, 0);
        return result(c, std::uint32_t(-4));
    }
    if (is_action(27)) {
        if (byte_result(c, p.call(0x103f4d10, 0x1041bcd8, {c.u32(transfers), c.u32(link), 0, frame.at(0x3a)}))) {
            const auto reason = extended(c.i16(frame.at(0x3a)));
            enter(c, 0x1041bcf0, {draft_slot, 27, reason}); open_offer_restriction(c, draft_slot, 27, reason);
        } else { enter(c, 0x1041bd04, {draft_slot, link}); open_bid_rejection(c, draft_slot, link); }
        return result(c, std::uint32_t(-4));
    }
    if (is_action(28)) {
        if (!valid_active(0x1041bd70)) return previous(0x1041bdac, 0x1041bdc0, 0x1041bdcc);
        if (byte_result(c, p.call(0x103f4d10, 0x1041bde8,
                                  {c.u32(transfers), c.u32(link), c.u32(draft_slot), frame.at(0x38)}))) {
            const auto reason = extended(c.i16(frame.at(0x38)));
            enter(c, 0x1041be00, {draft_slot, 28, reason}); open_offer_restriction(c, draft_slot, 28, reason);
        } else { enter(c, 0x1041be18, {draft_slot, 28, link}); open_offer_context(c, draft_slot, 28, link); }
        return result(c, std::uint32_t(-4));
    }
    if (is_action(29)) {
        if (!valid_active(0x1041be84)) return previous(0x1041bec0, 0x1041bed4, 0x1041bee0);
        enter(c, 0x1041bef8, {draft_slot, 29, link}); open_offer_context(c, draft_slot, 29, link);
        return result(c, std::uint32_t(-4));
    }
    if (is_action(6)) { c.put32(draft + 0x20, payload()); return result(c, std::uint32_t(-11)); }
    if (is_action(3)) return edit_fee(3);
    if (is_action(4)) return edit_fee(4);
    if (is_action(5)) {
        modified(0x1041c518, 0x1041c52c);
        c.set(5, c.u32(people) + c.u32(draft + 12) * 0x70);
        c.put32(frame.at(0x54), c.u32(c.r(5) + 8));
        p.call(0x10009f20, 0x1041c550, {frame.at(0x54), 4});
        control_or_limit_low = c.r(4); contract_or_control = c.r(3);
        if (compare_logical(c, c.u32(draft + 4)) != 0) {
            c.set(3, c.u32(draft + 4)); c.put32(c.r(3) + 12, c.u32(c.r(3) + 4));
            while (compare_logical(c, p.call(0x10738e00, 0x1041c5b0, {c.u32(draft + 4)})) != 0) {
                c.set(5, c.u32(people) + c.u32(c.r(3)) * 0x70);
                c.put32(frame.at(0x50), c.u32(c.r(5) + 8));
                p.call(0x10009f20, 0x1041c598, {frame.at(0x50), 4});
                const auto prior = (std::uint64_t(contract_or_control) << 32) | std::uint32_t(control_or_limit_low);
                const auto amount = (std::uint64_t(c.r(3)) << 32) | c.r(4);
                const auto difference = prior - amount;
                c.s.wr<1>(reg::xer_ca, prior >= amount);
                c.set(4, std::uint32_t(difference));
                control_or_limit_low = c.r(4); contract_or_control = std::uint32_t(difference >> 32);
            }
        }
        c.set({{5, 0}, {4, std::uint32_t(contract_or_control) ^ 0x80000000u}});
        const bool negative = std::int32_t(std::uint32_t(contract_or_control)) < 0;
        c.set(3, negative); compare(c, c.r(3)); c.s.wr<1>(reg::xer_ca, !negative);
        if (negative) { control_or_limit_low = 0; contract_or_control = 0; }
        c.put32(frame.at(0x80), 0);
        p.call(0x1000a010, 0x1041c5fc, {frame.at(0x80), contract_or_control, control_or_limit_low});
        if (is_bidder_manager()) {
            p.call(0x10103510, 0x1041c634, {c.u32(finances)});
            p.call(0x100f5770, 0x1041c64c, {c.r(3), person_or_over_limit, 0, 0, 1, 0});
            if (compare(c, c.i32(frame.at(0x80)), std::int32_t(c.r(4))) > 0) c.put32(frame.at(0x80), c.r(4));
        }
        c.put32(draft + 0x20, c.u32(frame.at(0x80))); return result(c, std::uint32_t(-11));
    }
    if (is_action(38)) {
        modified(0x1041c6d8, 0x1041c6ec);
        c.set(4, c.u32(people) + c.u32(draft + 12) * 0x70);
        c.put32(draft + 0x20, c.u32(c.r(4) + 8)); return result(c, std::uint32_t(-11));
    }
    if (is_action(37)) {
        if (is_bidder_manager()) {
            p.call(0x10103510, 0x1041c79c, {c.u32(finances)});
            p.call(0x100f5770, 0x1041c7b4, {c.r(3), person_or_over_limit, 0, 0, 1, 0}); temporary = c.r(4);
        }
        modified(0x1041c7cc, 0x1041c7e0);
        c.put32(draft + 0x20, temporary); return result(c, std::uint32_t(-11));
    }
    if (is_action(7)) {
        modified(0x1041c858, 0x1041c86c); c.put8(draft + 0x51, 0u - c.u8(draft + 0x51));
        if (compare(c, c.i8(draft + 0x51)) == 0) c.put8(draft + 0x51, 6);
        return result(c, std::uint32_t(-11));
    }
    if (is_action(9)) return edit_small_term(9);
    if (is_action(8)) return edit_small_term(8);
    if (is_action(10)) {
        modified(0x1041cc28, 0x1041cc3c); c.put8(draft + 0x50, 0u - c.u8(draft + 0x50));
        if (compare(c, c.i8(draft + 0x50)) == 0) c.put8(draft + 0x50, 10);
        return result(c, std::uint32_t(-11));
    }
    if (is_action(12)) return edit_small_term(12);
    if (is_action(11)) return edit_small_term(11);
    if (is_action(17)) { c.put32(draft + 0x40, payload()); return result(c, std::uint32_t(-11)); }
    if (is_action(13)) { toggle_conditional_fee(false); return result(c, std::uint32_t(-11)); }
    if (is_action(14)) { cycle_appearances(false); return result(c, std::uint32_t(-11)); }
    if (is_action(16)) return edit_fee(16);
    if (is_action(15)) return edit_fee(15);
    if (is_action(22)) { c.put32(draft + 0x48, payload()); return result(c, std::uint32_t(-11)); }
    if (is_action(18)) { toggle_conditional_fee(true); return result(c, std::uint32_t(-11)); }
    if (is_action(19)) { cycle_appearances(true); return result(c, std::uint32_t(-11)); }
    if (is_action(21)) return edit_fee(21);
    if (is_action(20)) return edit_fee(20);
    if (is_action(34)) {
        c.put8(draft + 0x35, compare(c, c.i8(draft + 0x35)) == 0 ? 2 : 0);
        return result(c, std::uint32_t(-11));
    }
    if (is_action(36)) {
        if (compare(c, c.i8(draft + 0x38) >> 4, 2) != 0) {
            c.set(3, 2); c.put8(draft + 0x38, (c.u8(draft + 0x38) & 15) | 0x20);
        } else {
            const bool eligible = compare_logical(c, history(0x1041dfd4, 9)) != 0;
            c.set(3, eligible ? 0 : 1);
            c.put8(draft + 0x38, (c.u8(draft + 0x38) & 15) | (eligible ? 0 : 0x10));
        }
        return result(c, std::uint32_t(-11));
    }
    if (is_action(25)) {
        modified(0x1041e074, 0x1041e088); p.call(0x100546e0, 0x1041e090, {draft});
        return result(c, std::uint32_t(-4));
    }
    if (is_action(26)) {
        p.history(0x1041e104, screens, 5, 1);
        c.set(4, history(0x1041e110, 1)); temporary = c.u32(c.r(4));
        const auto revision = history(0x1041e124, 3);
        bool valid = compare(c, c.i32(temporary + 0x2c), revision) == 0;
        if (valid && compare(c, c.i8(temporary + 0x34)) != 0) valid = compare(c, c.u8(temporary + 0x34), 1) == 0;
        if (!valid) return previous(0x1041e154, 0x1041e168, 0x1041e174, true);
        reset_draft(false); return result(c, std::uint32_t(-11));
    }
    if (is_action(32)) {
        p.history(0x1041e354, screens, 2, 1);
        if (!valid_active(0x1041e360)) return previous(0x1041e39c, 0x1041e3b0, 0x1041e3bc);
        temporary = p.call(0x103eb470, 0x1041e3d4, {c.u32(transfers), c.u32(c.u32(link) + 0x30)});
        reset_draft(true); return result(c, std::uint32_t(-11));
    }
    if (is_action(30)) {
        const auto selection = payload(); p.history(0x1041e5f0, screens, 4, selection);
        return result(c, std::uint32_t(-11));
    }
    if (is_action(31)) {
        temporary = payload(); modified(0x1041e6a0, 0x1041e6b4);
        c.set({{3, c.u32(people) + c.u32(temporary) * 0x70}, {4, temporary}, {5, c.u32(draft + 0x20)}});
        c.put32(draft + 0x20, c.r(5) + c.u32(c.r(3) + 8));
        p.call(0x10738e60, 0x1041e6e0, {c.u32(draft + 4)});
        c.set(3, c.u32(draft + 4));
        if (compare(c, c.i16(c.r(3))) == 0) {
            p.call(0x10738c20, 0x1041e6fc, {c.u32(draft + 4), 1}); c.put32(draft + 4, 0);
        }
        p.history(0x1041e718, screens, 4, 0); return result(c, std::uint32_t(-11));
    }
    if (compare(c, p.call(0x102a54b0, 0x1041e728, {control_or_limit_low})) != 0) return c.r(3);
    p.call(0x102a83a0, 0x1041e73c, {control_or_limit_low});
    c.s.wr<1>(reg::xer_ca, ((0u - c.r(3)) | c.r(3)) & 0x7fffffffu ? 1 : 0); return c.r(3);
}
}

std::uint32_t proposal_command(Cpu& c, std::uint32_t selected_control) {
    ProposalCommand command(c, selected_control); return command.run();
}
}
