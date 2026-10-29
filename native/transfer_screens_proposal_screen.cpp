#include "transfer_screens_detail.hpp"

namespace cm::aot::transfer_screens {
using namespace detail;
namespace {
struct ProposalScreen {
    Cpu& c;
    Frame frame;
    Program p;
    Kept<13> current_contract;
    Kept<14> manager_club;
    Kept<15> draft_slot_or_club;
    Kept<16> pending;
    Kept<17> active_offer;
    Kept<18> allocation_or_error_flag;
    Kept<19> draft_or_negotiated_link;
    Kept<20> bid_selector;
    Kept<21> person;
    Kept<22> disabled_colour;
    Kept<23> heading_colour;
    Kept<24> enabled_colour;
    Kept<25> secondary_colour;
    Kept<26> selected_colour;
    Kept<27> primary_colour;
    Kept<28> text;
    Kept<29> pool;
    Kept<30> clubs;
    Kept<31> screens;

    explicit ProposalScreen(Cpu& cpu)
        : c(cpu), frame(c, 0x10417b60, 0x370, {.first_gpr = 13}), p{c},
          current_contract(c, c.toc(slot::current_contract)), manager_club(c, c.r(14)),
          draft_slot_or_club(c, c.toc(-0x7ba8)), pending(c, c.r(16)), active_offer(c, c.r(17)),
          allocation_or_error_flag(c, c.toc(-0x7bac)), draft_or_negotiated_link(c, c.r(19)),
          bid_selector(c, missing), person(c, c.r(21)), disabled_colour(c, c.toc(-0x7bc4)),
          heading_colour(c, c.toc(-0x7bdc)), enabled_colour(c, c.toc(-0x7bc8)),
          secondary_colour(c, c.toc(-0x7bd4)), selected_colour(c, c.toc(-0x7bd0)),
          primary_colour(c, c.toc(-0x7bcc)), text(c, c.toc(-0x7bb8)),
          pool(c, c.toc(slot::pool)), clubs(c, c.toc(slot::clubs)), screens(c, c.toc(slot::screens)) {
        const auto defaults = c.toc(-0x4ac8);
        c.put32(frame.at(0xc4), c.u32(defaults));
        c.put16(frame.at(0xc8), c.u16(defaults + 4)); c.put8(frame.at(0xca), c.u8(defaults + 6));
        c.set({{10, c.u16(c.r(2) - 0x37dc)}, {9, c.u8(c.r(2) - 0x37da)},
               {8, c.u16(c.r(2) - 0x37d8)}, {7, c.u16(c.r(2) - 0x37d4)},
               {6, c.u16(c.r(2) - 0x37d2)}, {5, c.u16(c.r(2) - 0x37d0)}});
        c.put16(frame.at(0xa0), c.r(10)); c.put8(frame.at(0xa2), c.r(9));
        c.put16(frame.at(0x6c), c.r(8)); c.put16(frame.at(0x9c), c.r(7));
        c.put16(frame.at(0x9e), c.r(6)); c.put16(frame.at(0x98), c.r(5));
        c.put8(frame.at(0x9a), c.u8(c.r(2) - 0x37ce));
        p.call(0x1074b6e0, 0x10417c08, {frame.at(0xbc), 0});
        c.put32(frame.at(0x94), 0); active_offer = 0;
        p.call(0x10738c00, 0x10417c1c, {frame.at(0xcc)});
        c.put8(frame.at(0x68), 0);
        const auto footer_defaults = c.toc(-0x4acc);
        pending = c.u32(footer_defaults); manager_club = c.u32(footer_defaults + 4);
        for (unsigned i = 0; i < 10; ++i) c.put32(frame.at(0x15c + i * 4), c.u32(footer_defaults + i * 4));
        c.set({{5, footer_defaults}, {10, c.u32(footer_defaults + 0x10)}, {9, c.u32(footer_defaults + 0x14)},
               {8, c.u32(footer_defaults + 0x18)}, {7, c.u32(footer_defaults + 0x1c)}, {6, c.u32(footer_defaults + 0x20)}});
        const auto id = history(0x10417c84, 0);
        c.set(5, id * 0x70); person = indirect(c, slot::people) + c.r(5);
        if (compare_logical(c, history(0x10417ca0, 1)) != 0) active_offer = c.u32(history(0x10417cb4, 1));
        c.put8(frame.at(0x317), history(0x10417cc4, 4));
        c.put8(frame.at(0x316), history(0x10417cdc, 6));
        const auto pending_flag = history(0x10417cf4, 7);
        compare_logical(c, person); pending = std::uint32_t(std::int32_t(std::int8_t(pending_flag)));
    }
    std::uint32_t history(std::uint32_t back, unsigned index) {
        return p.call(0x1073ef70, back, {screens, index});
    }
    std::uint32_t controls() const {
        return c.u32(screens + std::uint32_t(c.i16(screens + 0x2c58)) * 0x2c0 + 0x24);
    }
    void cleanup(std::uint32_t list_back, std::uint32_t text_back) {
        p.call(0x10738c20, list_back, {frame.at(0xcc), missing});
        p.call(0x1074b7c0, text_back, {frame.at(0xbc), missing});
    }
    void assertion(std::uint32_t at, unsigned name, unsigned extension, unsigned line) {
        report_assertion(c, {at, pool, pool + 0x15, pool + 0x22, frame.at(name), frame.at(extension), line});
    }
    bool acquire_context() {
        if (!std::uint32_t(person)) {
            assertion(0x10417d14, 0x13c, 0xb4, 0x208); cleanup(0x10417d5c, 0x10417d68); return false;
        }
        manager_club = 0;
        const auto contract = p.call(0x10082f20, 0x10417d80, {indirect(c, slot::contracts), person});
        compare_logical(c, contract); c.put32(current_contract, contract);
        if (contract) {
            c.set(3, c.u32(current_contract));
            if (compare(c, c.i8(c.r(3) + 0x38) >> 2, 5) != 0) manager_club = 1;
        }
        std::uint32_t contracted_club = 0;
        if (compare(c, manager_club) != 0) {
            c.set({{3, c.u32(current_contract)}, {4, c.u32(clubs)}});
            contracted_club = c.r(4) + c.u32(c.r(3) + 4) * 0x248;
        }
        if (compare_logical(c, contracted_club) == 0) { cleanup(0x10417de4, 0x10417df0); return false; }
        const auto manager = c.u32(screens + std::uint32_t(c.i16(screens + 0x2c58)) * 0x2c0 + 4);
        c.set(3, manager);
        if (compare(c, manager) == 0) {
            assertion(0x10417e28, 0x11c, 0xac, 0x21b); cleanup(0x10417e70, 0x10417e7c); return false;
        }
        manager_club = c.u32(manager + 0x24);
        if (compare_logical(c, manager_club) == 0) {
            assertion(0x10417e9c, 0xfc, 0xa4, 0x222); cleanup(0x10417ee4, 0x10417ef0); return false;
        }
        return true;
    }
    void ensure_draft() {
        draft_slot_or_club = p.call(0x10148930, 0x10417f04, {indirect(c, -0x7a54), 0});
        if (compare_logical(c, c.u32(draft_slot_or_club)) == 0) {
            allocation_or_error_flag = p.call(0x10773520, 0x10417f1c, {0x54});
            if (compare(c, allocation_or_error_flag) != 0)
                p.call(0x10411ea0, 0x10417f40, {allocation_or_error_flag, person, manager_club, 0, missing, 0});
            compare_logical(c, active_offer); c.put32(draft_slot_or_club, allocation_or_error_flag);
            if (std::uint32_t(active_offer)) {
                const bool negotiated = compare(c, c.i32(active_offer + 0x30), -1) != 0;
                if (negotiated) {
                    draft_or_negotiated_link = p.call(0x103eb470, 0x10417f68,
                        {indirect(c, slot::transfers), c.u32(active_offer + 0x30)});
                    c.set({{7, c.u32(draft_slot_or_club)}, {8, c.u32(draft_or_negotiated_link)}});
                    copy_offer_fields(c, c.r(7), c.r(8)); p.history(0x10418060, screens, 2, 1);
                } else {
                    c.set(3, c.u32(draft_slot_or_club)); copy_offer_fields(c, c.r(3), active_offer);
                    c.set(4, c.u8(active_offer + 0x51));
                }
                c.set(3, c.u32(draft_slot_or_club)); c.put32(c.r(3), 0);
                c.set(3, c.u32(draft_slot_or_club));
                if (compare_logical(c, c.u32(c.r(3) + 4)) != 0) {
                    allocation_or_error_flag = p.call(0x10773520, negotiated ? 0x10418084 : 0x104181b4, {16});
                    if (compare(c, allocation_or_error_flag) != 0)
                        p.call(0x10738c00, negotiated ? 0x10418094 : 0x104181c4, {allocation_or_error_flag});
                    c.set(3, c.u32(draft_slot_or_club)); c.put32(c.r(3) + 4, allocation_or_error_flag);
                    c.set(3, c.u32(draft_slot_or_club));
                    p.call(0x10739010, negotiated ? 0x104180b0 : 0x104181dc,
                        {c.u32(c.r(3) + 4), c.u32((negotiated ? c.u32(draft_or_negotiated_link) : std::uint32_t(active_offer)) + 4)});
                }
            }
        }
        draft_or_negotiated_link = c.u32(draft_slot_or_club);
    }
    void stack_arguments(std::initializer_list<std::uint32_t> values) {
        unsigned offset = 0x38;
        for (auto value : values) { c.put32(frame.at(offset), value); offset += 4; }
    }
    void heading_and_other_bids() {
        p.call(0x107136d0, 0x104181f0, {person, 0, 0});
        p.call(0x100c7a50, 0x10418204, {manager_club, 1, frame.at(0x72), frame.at(0x70)});
        p.call(0x1077b720, 0x10418214, {text, pool + 0x28, manager_club + 0x221});
        p.call(0x1074b830, 0x10418220, {frame.at(0xbc), text});
        p.call(0x1074bab0, 0x10418234, {frame.at(0xbc), manager_club + 0x221, 0x7d5, manager_club});
        stack_arguments({0, 0x30, c.u16(frame.at(0x70)), missing});
        const auto title = p.call(0x107198c0, 0x10418284, {controls(), 100, 10, 790, 70, 1, 0, 1});
        c.put32(frame.at(0x90), 0); stack_arguments({0});
        p.call(0x1074bcd0, 0x104182b8, {frame.at(0xbc), title, 100, 790, frame.at(0x90), 7, c.u16(frame.at(0x72)), 1});
        if (compare(c, c.i8(draft_or_negotiated_link + 0x34)) != 0 || compare_logical(c, active_offer) == 0) return;
        draft_slot_or_club = 0;
        const auto contract = p.call(0x10082f20, 0x104182e0, {indirect(c, slot::contracts), person});
        compare_logical(c, contract); c.put32(current_contract, contract);
        if (contract) {
            c.set(3, c.u32(current_contract));
            if (compare(c, c.i8(c.r(3) + 0x38) >> 2, 5) != 0) draft_slot_or_club = 1;
        }
        std::uint32_t owner_club = 0;
        if (compare(c, draft_slot_or_club) != 0) {
            c.set({{3, c.u32(current_contract)}, {4, c.u32(clubs)}});
            owner_club = c.r(4) + c.u32(c.r(3) + 4) * 0x248;
        }
        if (compare_logical(c, manager_club, owner_club) != 0) return;
        p.call(0x103eaea0, 0x10418350, {indirect(c, slot::transfers), person, frame.at(0xcc), 0});
        c.put32(frame.at(0xd8), c.u32(frame.at(0xd0)));
        for (;;) {
            const auto item = p.call(0x10738e00, 0x104184d0, {frame.at(0xcc)});
            c.set(4, item); if (compare(c, item) == 0) break;
            p.call(0x103eb440, 0x1041836c, {indirect(c, slot::transfers), c.u32(c.r(4))});
            allocation_or_error_flag = c.u32(c.r(3));
            if (compare(c, c.i8(allocation_or_error_flag + 0x34)) != 0) continue;
            c.set(3, c.u32(allocation_or_error_flag + 8));
            if (compare(c, c.r(3), c.u32(draft_or_negotiated_link + 8)) == 0) continue;
            c.set({{3, c.u32(allocation_or_error_flag + 8)}, {4, c.u32(clubs)}});
            const bool first_bid = compare(c, std::int16_t(std::uint32_t(bid_selector)), -1) == 0;
            draft_slot_or_club = c.r(4) + c.r(3) * 0x248;
            if (first_bid) {
                stack_arguments({1, c.u16(frame.at(0x72)), 0, 0, 0});
                const auto background = p.call(0x10744430, 0x104183f4,
                    {screens, 105, 15, 120, 35, 0x30, c.u16(frame.at(0x70)), 12});
                stack_arguments({0, 0x30, c.u16(primary_colour), background});
                bid_selector = p.call(0x107198c0, 0x10418440, {controls(), 0, 0, 0, 0, 0, 0, 0});
            }
            const auto offer_id = c.u32(allocation_or_error_flag + 0x28);
            const auto caption = draft_slot_or_club + 0x1ee;
            allocation_or_error_flag = 1; draft_slot_or_club = 0;
            stack_arguments({16, c.u16(primary_colour), c.u16(selected_colour), 1, allocation_or_error_flag,
                             c.u16(secondary_colour), caption, draft_slot_or_club, 39, offer_id, bid_selector});
            p.call(0x10719740, 0x104184c8, {controls(), 2, 0, 0, 0, 0, 0, 0});
        }
    }
    void player_introduction() {
        p.call(0x100caeb0, 0x104184ec, {person, frame.at(0x184), 400, 5});
        const bool existing_or_pending = compare(c, c.i8(frame.at(0x316))) != 0 ||
                                         compare(c, std::int8_t(std::uint32_t(pending))) != 0;
        const auto club_base = c.u32(clubs), club = club_base + c.u32(draft_or_negotiated_link + 8) * 0x248;
        const auto gender = std::uint32_t(std::int32_t(c.i8(club + 0x23b)));
        c.set(10, club_base);
        if (existing_or_pending) {
            c.set(9, club_base);
            p.call(0x10727890, 0x10418554, {text, pool + 0x2b, gender, gender, club + 0x221, frame.at(0x184)});
        } else {
            p.call(0x10727890, 0x104185b0, {text, pool + 0x7e, gender, gender, 100, club + 0x221, frame.at(0x184)});
        }
        p.call(0x1074c570, 0x104185b8, {frame.at(0xbc)});
        p.call(0x1074b830, 0x104185c4, {frame.at(0xbc), text});
        p.call(0x1074bab0, 0x104185d8, {frame.at(0xbc), frame.at(0x184), 0x7d7, person});
        c.set(7, c.u32(clubs));
        const auto tagged_club = c.u32(clubs) + c.u32(draft_or_negotiated_link + 8) * 0x248;
        p.call(0x1074bab0, 0x10418608, {frame.at(0xbc), tagged_club + 0x221, 0x7d5, tagged_club});
        c.put32(frame.at(0x90), 0); stack_arguments({0, 2, 0, missing});
        draft_slot_or_club = p.call(0x107198c0, 0x10418660, {controls(), 100, 80, 790, 125, 1, 0, 1});
        stack_arguments({0});
        p.call(0x1074bcd0, 0x10418690,
            {frame.at(0xbc), draft_slot_or_club, 100, 790, frame.at(0x90), 6, c.u16(selected_colour), 1});
    }
    std::uint32_t query_owner_club(std::uint32_t back) {
        const auto contract = p.call(0x10082f20, back, {indirect(c, slot::contracts), person});
        compare_logical(c, contract); c.put32(current_contract, contract);
        if (!contract) return 0;
        c.set({{3, c.u32(current_contract)}, {4, c.u32(clubs)}});
        return c.r(4) + c.u32(c.r(3) + 4) * 0x248;
    }
    void valuation_and_board_limit() {
        draft_slot_or_club = 140; allocation_or_error_flag = 380;
        const auto row_height = p.call(0x1076d470, 0x104186a0, {3});
        const auto rows = std::uint32_t(divide_signed<4>(240, row_height));
        stack_arguments({0, 2, 0, missing});
        draft_slot_or_club = p.call(0x107198c0, 0x104186f8,
            {controls(), 110, 140, 780, 380, 7, frame.at(0xc4), rows & 0xff});
        c.put32(frame.at(0x8c), c.u32(person + 8));
        p.call(0x10009f20, 0x10418714, {frame.at(0x8c), 6});
        p.call(0x1000b410, 0x1041872c, {frame.at(0xdc), 30, c.r(3), c.r(4), 6});
        p.call(0x100caeb0, 0x10418740, {person, frame.at(0x184), 400, 1});
        c.put32(frame.at(0x318), query_owner_club(0x10418750));
        bid_selector = query_owner_club(0x10418790);
        c.set(3, query_owner_club(0x104187c8));
        p.call(0x10727890, 0x1041881c,
            {text, pool + 0xd9, std::uint32_t(std::int32_t(c.i8(c.r(3) + 0x23b))),
             std::uint32_t(std::int32_t(c.i8(bid_selector + 0x23b))), c.u32(frame.at(0x318)) + 0x221,
             frame.at(0x184), frame.at(0xdc)});
        p.call(0x1074b830, 0x10418828, {frame.at(0xbc), text});
        p.call(0x1074bab0, 0x1041883c, {frame.at(0xbc), frame.at(0x184), 0x7d7, person});
        bid_selector = query_owner_club(0x1041884c);
        const auto owner = query_owner_club(0x10418884);
        p.call(0x1074bab0, 0x104188c0, {frame.at(0xbc), owner + 0x221, 0x7d5, bid_selector});
        if (compare(c, std::int8_t(std::uint32_t(pending))) == 0)
            p.call(0x1074bab0, 0x104188dc, {frame.at(0xbc), frame.at(0xdc), 0x26, 0});
        p.call(0x1074b830, 0x104188e8, {frame.at(0xbc), text});
        c.put32(frame.at(0x90), 0); stack_arguments({0});
        p.call(0x1074bcd0, 0x1041891c,
            {frame.at(0xbc), draft_slot_or_club, 118, 780, frame.at(0x90), 3, c.u16(enabled_colour), 0});
        if (compare(c, c.i8(frame.at(0x316))) == 0) return;
        p.call(0x10103510, 0x10418938, {indirect(c, -0x7ab8), manager_club});
        p.call(0x100f5770, 0x10418950, {c.r(3), person, 0, frame.at(0x68), 1, 0});
        current_contract = c.r(4); bid_selector = c.r(3); c.put32(frame.at(0x94), current_contract);
        p.call(0x10009f20, 0x1041896c, {frame.at(0x94), 6});
        p.call(0x1000b410, 0x10418984, {frame.at(0xdc), 30, c.r(3), c.r(4), 6});
        c.set(3, c.toc(-0x797c));
        bool board_decision = compare(c, c.i8(c.r(3))) != 0;
        if (board_decision) {
            c.set(3, c.toc(-0x7978));
            board_decision = compare_logical(c, c.u32(c.r(3))) != 0;
            if (board_decision) board_decision = compare(c, c.i8(frame.at(0x68))) != 0;
        }
        if (board_decision) {
            const auto board = p.call(0x106fdb00, 0x104189c0, {indirect(c, -0x7978), c.u32(manager_club + 0x90)});
            if (compare_logical(c, board) != 0) {
                const auto decision = std::int8_t(p.call(0x106fe280, 0x104189d0, {board, person}));
                const auto relation = compare(c, decision, 2);
                unsigned kind = 0;
                if (!relation) kind = 2;
                else if (relation < 0) {
                    const auto zero = compare(c, decision);
                    if (zero > 0) kind = 1;
                } else if (compare(c, decision, 4) < 0) kind = 3;
                c.set(3, current_contract);
                const bool zero = compare(c, std::uint32_t(current_contract) | std::uint32_t(bid_selector)) == 0;
                struct Message { unsigned empty, limited; std::uint32_t empty_back, limited_back; };
                static constexpr Message messages[]{
                    {0x37b,0x415,0x10418ad0,0x10418ae4}, {0x260,0x2f7,0x10418a94,0x10418aa8},
                    {0x160,0x20c,0x10418a58,0x10418a6c}, {0x160,0x1bd,0x10418a1c,0x10418a30}};
                const auto& message = messages[kind];
                if (zero) p.call(0x10727890, message.empty_back, {text, pool + message.empty});
                else p.call(0x10727890, message.limited_back, {text, pool + message.limited, frame.at(0xdc)});
            } else p.call(0x10727890, 0x10418af8, {text, pool + 0x1bd, frame.at(0xdc)});
        } else p.call(0x10727890, 0x10418b0c, {text, pool + 0x1bd, frame.at(0xdc)});
        p.call(0x1074c570, 0x10418b14, {frame.at(0xbc)});
        p.call(0x1074b830, 0x10418b20, {frame.at(0xbc), text});
        p.call(0x1074bab0, 0x10418b34, {frame.at(0xbc), frame.at(0xdc), 0x25, 0});
        c.put32(frame.at(0x90), c.u32(frame.at(0x90)) + 1); stack_arguments({0});
        p.call(0x1074bcd0, 0x10418b6c,
            {frame.at(0xbc), draft_slot_or_club, 118, 780, frame.at(0x90), 3, c.u16(enabled_colour), 0});
    }
    // The grid service takes position/style in registers and caption/action on
    // the guest stack. Static cells consume only the caption pair.
    std::uint32_t grid_cell(std::uint32_t back, unsigned column, unsigned style,
                           unsigned background, unsigned alignment, unsigned foreground,
                           std::uint32_t caption, int action = -1) {
        if (action < 0) stack_arguments({foreground, caption});
        else stack_arguments({foreground, caption, std::uint32_t(action), 0});
        return p.call(action < 0 ? 0x107445a0 : 0x107444e0, back,
            {screens, draft_slot_or_club, column, c.u32(frame.at(0x90)), style, background, alignment, 3});
    }
    void arrow(std::uint32_t back, unsigned column, bool enabled, int action = -1) {
        grid_cell(back, column, 0x30, c.u16(heading_colour), 12,
                  c.u16(enabled ? enabled_colour : disabled_colour), pool + (column == 2 ? 0x4a9 : 0x4ab), action);
    }
    void base_fee_row() {
        c.set(5, c.u32(frame.at(0x90))); c.put32(frame.at(0x90), c.r(5) + 2);
        p.call(0x10727890, 0x10418b84, {text, pool + 0x49c});
        manager_club = extended(std::int16_t(grid_cell(0x10418bc4, 1, 1, 0, 1, c.u16(enabled_colour), text, 0)));
        p.history(0x10418be0, screens, 15, manager_club);
        bool decrease = compare(c, c.i32(draft_or_negotiated_link + 0x20)) > 0;
        if (decrease) decrease = compare(c, std::int8_t(std::uint32_t(pending))) == 0;
        arrow(decrease ? 0x10418c70 : 0x10418c28, 2, decrease, decrease ? 4 : -1);
        c.put32(frame.at(0x8c), c.u32(draft_or_negotiated_link + 0x20));
        p.call(0x10009f20, 0x10418c84, {frame.at(0x8c), 5});
        p.call(0x1000b410, 0x10418c9c, {text, 2000, c.r(3), c.r(4), 5});
        bid_selector = 0;
        if (compare(c, c.i8(frame.at(0x316))) != 0) {
            person = 1; clubs = frame.at(0x88);
            c.put32(frame.at(0x88), c.u32(draft_or_negotiated_link + 0x20));
            p.call(0x10009f20, 0x10418cc8, {frame.at(0x94), 4});
            manager_club = c.r(4); current_contract = c.r(3);
            p.call(0x10009f20, 0x10418cdc, {clubs, 4});
            const auto maximum = std::bit_cast<std::int64_t>((std::uint64_t(current_contract) << 32) | std::uint32_t(manager_club));
            const auto amount = std::bit_cast<std::int64_t>((std::uint64_t(c.r(3)) << 32) | c.r(4));
            const bool over = maximum < amount;
            c.set({{5, current_contract ^ 0x80000000u}, {3, over}});
            c.s.wr<1>(reg::xer_ca, !over);
            if (compare(c, c.r(3)) != 0) bid_selector = 1;
        }
        const bool over_limit = compare(c, bid_selector) != 0;
        const auto amount_control = grid_cell(over_limit ? 0x10418d48 : 0x10418d90,
            3, 1, c.u16(secondary_colour), 12, c.u16(over_limit ? c.toc(-0x7a74) : std::uint32_t(enabled_colour)), text, 0);
        c.set(3, extended(std::int16_t(amount_control)));
        compare(c, std::int8_t(std::uint32_t(pending))); manager_club = c.r(3);
        if (!std::int8_t(std::uint32_t(pending))) {
            stack_arguments({0, 0x130, c.u16(primary_colour), extended(std::int16_t(std::uint32_t(manager_club)))});
            c.put16(frame.at(0x314), p.call(0x107198c0, 0x10418df0, {controls(),0,0,0,0,0,0,0}));
        } else c.put16(frame.at(0x314), missing);
        c.set(3, 100000000);
        bool increase = compare(c, c.i32(draft_or_negotiated_link + 0x20), 100000000) < 0;
        if (increase) increase = compare(c, std::int8_t(std::uint32_t(pending))) == 0;
        arrow(increase ? 0x10418e98 : 0x10418e50, 4, increase, increase ? 3 : -1);
        const bool editable = compare(c, std::int8_t(std::uint32_t(pending))) == 0;
        grid_cell(editable ? 0x10418f1c : 0x10418ed4, 5, 0x30, c.u16(heading_colour), 12,
                  c.u16(editable ? enabled_colour : disabled_colour), pool + 0x4ad, editable ? 5 : -1);
    }
    void small_term_row(bool installments) {
        struct Sites {
            std::uint32_t heading, static_positive, static_absent, static_down,
                static_value, static_dash, static_amount, static_up,
                edit_positive, edit_absent, minimum_down, inactive_down, active_down,
                edit_value, edit_dash, edit_amount, maximum_up, inactive_up, active_up, history;
        };
        static constexpr Sites months{0x10418f34,0x10418f78,0x10418fb0,0x10418fec,
            0x1041900c,0x1041901c,0x1041904c,0x10419080,0x104190d0,0x10419118,
            0x10419160,0x104191b4,0x104191fc,0x1041921c,0x1041922c,0x1041925c,
            0x104192a0,0x104192f4,0x1041933c,0x10419350};
        static constexpr Sites percentage{0x10419368,0x104193ac,0x104193e4,0x10419420,
            0x10419440,0x10419450,0x10419480,0x104194b4,0x10419504,0x1041954c,
            0x10419594,0x104195e8,0x10419630,0x10419650,0x10419660,0x10419690,
            0x104196d4,0x10419728,0x10419770,0x10419784};
        const auto& sites = installments ? months : percentage;
        const auto field = draft_or_negotiated_link + (installments ? 0x51u : 0x50u);
        const auto toggle = installments ? 7 : 10;
        c.set(5, c.u32(frame.at(0x90))); c.put32(frame.at(0x90), c.r(5) + 1);
        p.call(0x10727890, sites.heading, {text, pool + (installments ? 0x4afu : 0x4e1u)});
        const bool fixed = compare(c, std::int8_t(std::uint32_t(pending))) != 0;
        const bool active_term = compare(c, c.i8(field)) > 0;
        const auto heading_back = fixed ? (active_term ? sites.static_positive : sites.static_absent)
                                       : (active_term ? sites.edit_positive : sites.edit_absent);
        c.set(3, extended(std::int16_t(grid_cell(heading_back, 1, 1, 0, 1,
            c.u16(active_term ? enabled_colour : disabled_colour), text, fixed ? -1 : toggle))));
        manager_club = c.r(3);
        if (fixed) arrow(sites.static_down, 2, false);
        else if (compare(c, c.u8(field), installments ? 6 : 5) == 0) arrow(sites.minimum_down, 2, false);
        else if (compare(c, c.i8(field)) <= 0) arrow(sites.inactive_down, 2, false, toggle);
        else arrow(sites.active_down, 2, true, installments ? 9 : 12);
        if (compare(c, c.i8(field)) > 0)
            p.call(installments ? 0x10727890 : 0x1077b720, fixed ? sites.static_value : sites.edit_value,
                {text, pool + (installments ? 0x4c4u : 0x4f9u), std::uint32_t(std::int32_t(c.i8(field)))});
        else p.call(0x1077b720, fixed ? sites.static_dash : sites.edit_dash, {text, pool + 0x4a9});
        grid_cell(fixed ? sites.static_amount : sites.edit_amount, 3, 1,
                  installments ? c.u16(primary_colour) : 0u, 12, c.u16(enabled_colour), text);
        if (fixed) arrow(sites.static_up, 4, false);
        else if (compare(c, c.i8(field), installments ? 24 : 25) >= 0) arrow(sites.maximum_up, 4, false);
        else if (compare(c, c.i8(field)) <= 0) arrow(sites.inactive_up, 4, false, toggle);
        else arrow(sites.active_up, 4, true, installments ? 8 : 11);
        p.history(sites.history, screens, installments ? 16 : 17, manager_club);
    }
    void conditional_fee_row(bool international) {
        struct Sites {
            std::uint32_t singular, plural, static_positive, static_absent, static_down, static_history,
                static_convert, static_format, static_dash, static_amount, static_up,
                edit_positive, clear_text, append_text, format_count, tag_count, paint_count, edit_absent,
                inactive_down, minimum_down, active_down, edit_history, edit_convert, edit_format, edit_dash,
                limit_conversion, amount_conversion, over_limit_amount, normal_amount, overlay,
                inactive_up, maximum_up, active_up;
        };
        static constexpr Sites league{0x104197a8,0x104197c0,0x10419804,0x10419838,0x1041986c,0x10419888,
            0x104198a8,0x104198c0,0x104198d0,0x10419910,0x10419948,
            0x1041999c,0x104199ac,0x104199b8,0x104199cc,0x104199e0,0x10419a0c,0x10419a50,
            0x10419aa0,0x10419ae8,0x10419b34,0x10419b4c,0x10419b6c,0x10419b84,0x10419b94,
            0x10419bc0,0x10419bd4,0x10419c40,0x10419c88,0x10419ce0,0x10419d34,0x10419d80,0x10419dc8};
        static constexpr Sites internationals{0x10419dec,0x10419e04,0x10419e48,0x10419e7c,0x10419eb0,0x10419ecc,
            0x10419eec,0x10419f04,0x10419f14,0x10419f44,0x10419f7c,
            0x10419fd0,0x10419fe0,0x10419fec,0x1041a000,0x1041a014,0x1041a040,0x1041a084,
            0x1041a0d4,0x1041a11c,0x1041a168,0x1041a180,0x1041a1a0,0x1041a1b8,0x1041a1c8,
            0x1041a1f0,0x1041a204,0x1041a270,0x1041a2b8,0x1041a310,0x1041a364,0x1041a3b0,0x1041a3f8};
        const auto& sites = international ? internationals : league;
        const auto field = draft_or_negotiated_link + (international ? 0x48u : 0x40u);
        const auto appearances = field + 4;
        const auto toggle = international ? 18 : 13;
        c.set(3, c.u32(frame.at(0x90))); c.put32(frame.at(0x90), c.r(3) + 1);
        if (compare(c, c.u8(appearances), 1) == 0)
            p.call(0x10727890, sites.singular, {text, pool + (international ? 0x546u : 0x4feu)});
        else p.call(0x10727890, sites.plural,
            {text, pool + (international ? 0x565u : 0x516u), std::uint32_t(std::int32_t(c.i8(appearances)))});
        const bool fixed = compare(c, std::int8_t(std::uint32_t(pending))) != 0;
        const bool active_fee = compare(c, c.i32(field)) > 0;
        if (fixed) {
            grid_cell(active_fee ? sites.static_positive : sites.static_absent, 1, 1, 0, 1,
                      c.u16(active_fee ? enabled_colour : disabled_colour), text);
            manager_club = extended(std::int16_t(grid_cell(sites.static_down, 2, 0x30, c.u16(heading_colour),
                12, c.u16(disabled_colour), pool + 0x4a9)));
            p.history(sites.static_history, screens, international ? 19 : 18, manager_club);
            if (compare(c, c.i32(field)) > 0) {
                c.put32(frame.at(0x8c), c.u32(field));
                p.call(0x10009f20, sites.static_convert, {frame.at(0x8c), 5});
                p.call(0x1000b410, sites.static_format, {text, 2000, c.r(3), c.r(4), 5});
            } else p.call(0x1077b720, sites.static_dash, {text, pool + 0x4a9});
            grid_cell(sites.static_amount, 3, 1, international ? 0u : c.u16(secondary_colour),
                      12, c.u16(enabled_colour), text, international ? -1 : 0);
            if (international) bid_selector = missing; else clubs = missing;
            arrow(sites.static_up, 4, false);
            return;
        }
        if (active_fee) {
            grid_cell(sites.edit_positive, 1, 1, 0, 1, c.u16(enabled_colour), 0, toggle);
            current_contract = 148; manager_club = current_contract + 335;
            p.call(0x1074c570, sites.clear_text, {frame.at(0xbc)});
            p.call(0x1074b830, sites.append_text, {frame.at(0xbc), text});
            p.call(0x1077b720, sites.format_count, {text, pool + 0x543, std::uint32_t(std::int32_t(c.i8(appearances)))});
            p.call(0x1074bab0, sites.tag_count, {frame.at(0xbc), text, international ? 19u : 14u, 0});
            stack_arguments({0});
            p.call(0x1074bcd0, sites.paint_count,
                {frame.at(0xbc), draft_slot_or_club, current_contract, manager_club, frame.at(0x90), 3, c.u16(enabled_colour), 0});
        } else grid_cell(sites.edit_absent, 1, 1, 0, 1, c.u16(disabled_colour), text, toggle);
        std::uint32_t down = 0;
        if (compare(c, c.i32(field)) <= 0)
            down = grid_cell(sites.inactive_down, 2, 0x30, c.u16(heading_colour), 12, c.u16(disabled_colour), pool + 0x4a9, toggle);
        else if (compare(c, c.i32(field), 1000) <= 0)
            down = grid_cell(sites.minimum_down, 2, 0x30, c.u16(heading_colour), 12, c.u16(disabled_colour), pool + 0x4a9);
        else down = grid_cell(sites.active_down, 2, 0x30, c.u16(heading_colour), 12, c.u16(enabled_colour), pool + 0x4a9,
                              international ? 21 : 16);
        manager_club = extended(std::int16_t(down));
        p.history(sites.edit_history, screens, international ? 19 : 18, manager_club);
        if (compare(c, c.i32(field)) > 0) {
            c.put32(frame.at(0x8c), c.u32(field));
            p.call(0x10009f20, sites.edit_convert, {frame.at(0x8c), 5});
            p.call(0x1000b410, sites.edit_format, {text, 2000, c.r(3), c.r(4), 5});
        } else p.call(0x1077b720, sites.edit_dash, {text, pool + 0x4a9});
        if (international) manager_club = 0; else bid_selector = 0;
        if (compare(c, c.i8(frame.at(0x316))) != 0) {
            person = 1;
            const auto scratch = frame.at(international ? 0x80 : 0x84);
            if (!international) clubs = scratch;
            c.put32(scratch, c.u32(field));
            p.call(0x10009f20, sites.limit_conversion, {frame.at(0x94), 4});
            if (international) { current_contract = c.r(4); bid_selector = c.r(3); }
            else { manager_club = c.r(4); current_contract = c.r(3); }
            p.call(0x10009f20, sites.amount_conversion, {international ? scratch : std::uint32_t(clubs), 4});
            const auto high = international ? std::uint32_t(bid_selector) : std::uint32_t(current_contract);
            const auto low = international ? std::uint32_t(current_contract) : std::uint32_t(manager_club);
            const auto maximum = std::bit_cast<std::int64_t>((std::uint64_t(high) << 32) | low);
            const auto amount = std::bit_cast<std::int64_t>((std::uint64_t(c.r(3)) << 32) | c.r(4));
            const bool over = maximum < amount;
            c.set({{5, high ^ 0x80000000u}, {3, over}}); c.s.wr<1>(reg::xer_ca, !over);
            if (compare(c, c.r(3)) != 0) { if (international) manager_club = 1; else bid_selector = 1; }
        }
        const bool over_limit = compare(c, international ? std::uint32_t(manager_club) : std::uint32_t(bid_selector)) != 0;
        c.set(3, extended(std::int16_t(grid_cell(over_limit ? sites.over_limit_amount : sites.normal_amount,
            3, 1, international ? 0u : c.u16(secondary_colour), 12,
            c.u16(over_limit ? c.toc(-0x7a74) : std::uint32_t(enabled_colour)), text, 0))));
        manager_club = c.r(3);
        stack_arguments({0, 0x130, c.u16(primary_colour), extended(std::int16_t(std::uint32_t(manager_club)))});
        const auto overlay = p.call(0x107198c0, sites.overlay, {controls(),0,0,0,0,0,0,0});
        if (international) bid_selector = overlay; else clubs = overlay;
        if (compare(c, c.i32(field)) <= 0) arrow(sites.inactive_up, 4, false, toggle);
        else {
            c.set(3, 100000000);
            if (compare(c, c.i32(field), 100000000) >= 0) arrow(sites.maximum_up, 4, false);
            else arrow(sites.active_up, 4, true, international ? 20 : 15);
        }
    }
    std::uint32_t screen_button(std::uint32_t back, unsigned left, unsigned right,
                                unsigned style, bool enabled, int action = 0, unsigned payload = 0) {
        if (enabled) stack_arguments({1, c.u16(enabled_colour), text, std::uint32_t(action), payload});
        else stack_arguments({1, c.u16(enabled_colour), text});
        return p.call(enabled ? 0x10744430 : 0x10744390, back,
            {screens, left, draft_slot_or_club, right, allocation_or_error_flag, style, c.u16(disabled_colour), enabled ? 12u : 44u});
    }
    void exchange_controls() {
        c.set(3, c.u32(frame.at(0x90)));
        const bool fixed = compare(c, std::int8_t(std::uint32_t(pending))) != 0;
        c.put32(frame.at(0x90), c.r(3) + 1);
        if (!fixed) {
            p.call(0x10727890, 0x1041a418, {text, pool + 0x599});
            const bool editable = compare(c, c.u8(draft_or_negotiated_link + 0x35), 1) != 0;
            grid_cell(editable ? 0x1041a464 : 0x1041a498, 1, 1, 0, 1, c.u16(enabled_colour), text, editable ? 34 : -1);
            if (compare(c, c.i8(draft_or_negotiated_link + 0x35)) == 0) {
                p.call(0x10727890, 0x1041a4b0, {text, pool + 0x5b2});
                grid_cell(0x1041a4f0, 3, 1, 0, 12, c.u16(enabled_colour), text, 34);
            } else if (compare(c, c.u8(draft_or_negotiated_link + 0x35), 2) == 0) {
                p.call(0x10727890, 0x1041a50c, {text, pool + 0x5b5});
                grid_cell(0x1041a54c, 3, 1, 0, 12, c.u16(enabled_colour), text, 34);
            } else if (compare(c, c.u8(draft_or_negotiated_link + 0x35), 1) == 0) {
                p.call(0x10727890, 0x1041a568, {text, pool + 0x5b5});
                grid_cell(0x1041a598, 3, 1, 0, 12, c.u16(disabled_colour), text);
            }
        }
        draft_slot_or_club = allocation_or_error_flag + 5; allocation_or_error_flag = draft_slot_or_club + 20;
        p.call(0x10727890, 0x1041a5ac, {text, pool + 0x5b9});
        bool below_limit = true;
        if (compare_logical(c, c.u32(draft_or_negotiated_link + 4)) != 0) {
            c.set(3, c.u32(draft_or_negotiated_link + 4)); below_limit = compare(c, c.i16(c.r(3)), 5) < 0;
        }
        bool can_add = below_limit;
        if (can_add) can_add = compare(c, std::int8_t(std::uint32_t(pending))) == 0;
        screen_button(can_add ? 0x1041a618 : 0x1041a654, 110, 235, 0x30, can_add, 25);
        p.call(0x10727890, 0x1041a660, {text, pool + 0x5c2});
        bool can_remove = compare_logical(c, c.u32(draft_or_negotiated_link + 4)) != 0;
        if (can_remove) can_remove = compare(c, std::int8_t(std::uint32_t(pending))) == 0;
        if (can_remove) {
            const bool removing = compare(c, c.i8(frame.at(0x317))) != 0;
            screen_button(removing ? 0x1041a6c8 : 0x1041a714, 235, 360, removing ? 0x70 : 0x30, true, 30, removing ? 0 : 1);
        } else screen_button(0x1041a750, 235, 360, 0x30, false);
        if (compare_logical(c, active_offer) == 0 || compare(c, std::int8_t(std::uint32_t(pending))) != 0) return;
        const bool equal = byte_result(c, p.call(0x10416e10, 0x1041a76c, {draft_or_negotiated_link, active_offer}));
        p.call(0x10727890, equal ? 0x1041a7dc : 0x1041a780, {text, pool + 0x5c9});
        manager_club = extended(std::int16_t(screen_button(equal ? 0x1041a814 : 0x1041a7c8, 655, 780, 0x30, !equal, 26)));
        p.history(0x1041a82c, screens, 22, manager_club);
        if (compare(c, c.i32(active_offer + 0x30), -1) == 0) return;
        const auto negotiated = p.call(0x103eb470, 0x1041a848, {indirect(c, slot::transfers), c.u32(active_offer + 0x30)});
        const bool matches_negotiated = byte_result(c, p.call(0x10416e10, 0x1041a85c, {draft_or_negotiated_link, c.u32(negotiated)}));
        p.call(0x10727890, matches_negotiated ? 0x1041a8c8 : 0x1041a870, {text, pool + 0x5cf});
        screen_button(matches_negotiated ? 0x1041a900 : 0x1041a8b8, 530, 655, 0x30, !matches_negotiated, 32);
    }
    void exchange_players_and_total() {
        draft_slot_or_club = allocation_or_error_flag + 5; allocation_or_error_flag = 545;
        const auto height = p.call(0x1076d470, 0x1041a910, {3});
        const auto rows = std::uint32_t(divide_signed<4>(allocation_or_error_flag - draft_slot_or_club, height));
        stack_arguments({0,2,0,missing});
        draft_slot_or_club = p.call(0x107198c0, 0x1041a968,
            {controls(),110,draft_slot_or_club,780,allocation_or_error_flag,3,frame.at(0xa0),rows & 0xff});
        c.put32(frame.at(0x90), 0); c.put32(frame.at(0x7c), c.u32(draft_or_negotiated_link + 0x20));
        p.call(0x10009f20, 0x1041a988, {frame.at(0x7c),4});
        allocation_or_error_flag = c.r(4); manager_club = c.r(3);
        const auto add_amount = [&] {
            const auto prior = (std::uint64_t(manager_club) << 32) | std::uint32_t(allocation_or_error_flag);
            const auto amount = (std::uint64_t(c.r(3)) << 32) | c.r(4);
            const auto sum = prior + amount; c.s.wr<1>(reg::xer_ca, sum < prior);
            c.set(4, std::uint32_t(sum)); allocation_or_error_flag = c.r(4); manager_club = std::uint32_t(sum >> 32);
        };
        if (compare(c, c.i32(draft_or_negotiated_link + 0x40)) >= 0) {
            c.put32(frame.at(0x78), c.u32(draft_or_negotiated_link + 0x40));
            p.call(0x10009f20, 0x1041a9b0, {frame.at(0x78),4}); add_amount();
        }
        if (compare(c, c.i32(draft_or_negotiated_link + 0x48)) >= 0) {
            c.put32(frame.at(0x74), c.u32(draft_or_negotiated_link + 0x48));
            p.call(0x10009f20, 0x1041a9e0, {frame.at(0x74),4}); add_amount();
        }
        if (compare_logical(c, c.u32(draft_or_negotiated_link + 4)) != 0) {
            c.set(3, c.u32(draft_or_negotiated_link + 4)); c.put32(c.r(3) + 12, c.u32(c.r(3) + 4));
            for (;;) {
                person = p.call(0x10738e00, 0x1041ab54, {c.u32(draft_or_negotiated_link + 4)});
                if (compare(c, person) == 0) break;
                c.set(5, indirect(c, slot::people)); heading_colour = c.r(5) + c.u32(person) * 0x70;
                p.call(0x1077b720, 0x1041aa2c, {text,pool + 0x5de});
                p.call(0x100caeb0, 0x1041aa40, {heading_colour,text + 2,2000,2});
                const bool remove = compare(c, c.i8(frame.at(0x317))) != 0;
                stack_arguments({c.u16(enabled_colour),text,remove ? 31u : 0x7d7u,
                                 remove ? std::uint32_t(person) : std::uint32_t(heading_colour)});
                p.call(0x107444e0, remove ? 0x1041aa88 : 0x1041aac8,
                    {screens,draft_slot_or_club,1,c.u32(frame.at(0x90)),1,0,1,3});
                c.put32(frame.at(0x8c), c.u32(heading_colour + 8));
                p.call(0x10009f20, 0x1041aadc, {frame.at(0x8c),6});
                p.call(0x1000b410, 0x1041aaf4, {text,2000,c.r(3),c.r(4),6});
                grid_cell(0x1041ab24,2,1,0,12,c.u16(enabled_colour),text);
                c.set(5,c.u32(frame.at(0x90))); c.put32(frame.at(0x90),c.r(5) + 1);
                p.call(0x10009f20, 0x1041ab3c, {frame.at(0x8c),4}); add_amount();
            }
        }
        p.call(0x10727890, 0x1041ab68, {text,pool + 0x5e1});
        grid_cell(0x1041ab98,1,1,0,1,c.u16(enabled_colour),text);
        p.call(0x1000b410, 0x1041abb0, {text,2000,manager_club,allocation_or_error_flag,0x22});
        manager_club = extended(std::int16_t(grid_cell(0x1041abe0,2,1,0,12,c.u16(selected_colour),text)));
        p.history(0x1041abfc, screens, 20, manager_club);
        p.call(0x102a1070, 0x1041ac08, {4,0});
    }
    void footer_container(std::uint32_t back, unsigned columns, unsigned defaults) {
        stack_arguments({0,1,c.u16(frame.at(0x70)),missing});
        draft_slot_or_club = p.call(0x107198c0, back,
            {controls(),100,555,790,590,columns,frame.at(defaults),1});
    }
    std::uint32_t footer_button(std::uint32_t text_back, std::uint32_t control_back,
                               unsigned column, unsigned caption, int action) {
        p.call(0x10727890, text_back, {text,pool + caption});
        if (action >= 0) stack_arguments({c.u16(enabled_colour),text,std::uint32_t(action),0});
        else stack_arguments({c.u16(enabled_colour),text});
        return p.call(action >= 0 ? 0x107444e0 : 0x107445a0, control_back,
            {screens,draft_slot_or_club,column,0,0x30,c.u16(disabled_colour),action >= 0 ? 12u : 44u,3});
    }
    void footer() {
        if (compare(c, c.i8(frame.at(0x316))) != 0) {
            if (compare(c, std::int8_t(std::uint32_t(pending))) != 0) {
                footer_container(0x1041ac6c,2,0x6c);
                footer_button(0x1041ac80,0x1041acc0,0,0x5ed,33);
                footer_button(0x1041accc,0x1041ad0c,1,0x5f2,2);
            } else if (compare_logical(c, active_offer) != 0) {
                footer_container(0x1041ad68,3,0x9c);
                footer_button(0x1041ad7c,0x1041adbc,0,0x5ed,33);
                footer_button(0x1041adc8,0x1041ae08,1,0x5fb,2);
                footer_button(0x1041ae14,0x1041ae54,2,0x602,1);
            } else {
                footer_container(0x1041aea8,2,0x6c);
                footer_button(0x1041aebc,0x1041aefc,0,0x5ed,33);
                footer_button(0x1041af08,0x1041af48,1,0x602,1);
            }
            return;
        }
        if (compare(c, std::int8_t(std::uint32_t(pending))) != 0) {
            footer_container(0x1041afa4,2,0x6c);
            footer_button(0x1041afb8,0x1041aff8,0,0x5ed,33);
            footer_button(0x1041b004,0x1041b044,1,0x5fb,2);
            return;
        }
        const bool delay_allowed = compare_logical(c, c.u8(draft_or_negotiated_link + 0x36), 3) < 0;
        footer_container(delay_allowed ? 0x1041b2a0 : 0x1041b0a4,delay_allowed ? 4 : 3,delay_allowed ? 0x9c : 0x98);
        footer_button(delay_allowed ? 0x1041b2b4 : 0x1041b0b8,delay_allowed ? 0x1041b2f4 : 0x1041b0f8,0,0x5ed,33);
        footer_button(delay_allowed ? 0x1041b300 : 0x1041b104,delay_allowed ? 0x1041b340 : 0x1041b144,1,0x608,27);
        if (delay_allowed) footer_button(0x1041b34c,0x1041b38c,2,0x620,35);
        const bool unchanged = byte_result(c, p.call(0x10416e10,delay_allowed ? 0x1041b398 : 0x1041b150,
                                                    {draft_or_negotiated_link,active_offer}));
        std::uint32_t control = 0;
        const auto column = delay_allowed ? 3u : 2u;
        if (!unchanged) {
            if (compare(c, c.u8(active_offer + 0x3c) >> 4) == 0)
                control = footer_button(delay_allowed ? 0x1041b3b8 : 0x1041b170,
                                        delay_allowed ? 0x1041b3f8 : 0x1041b1b0,column,0x60f,28);
            else control = footer_button(delay_allowed ? 0x1041b40c : 0x1041b1c4,
                                         delay_allowed ? 0x1041b43c : 0x1041b1f4,column,0x619,-1);
        } else control = footer_button(delay_allowed ? 0x1041b450 : 0x1041b208,
                                       delay_allowed ? 0x1041b490 : 0x1041b248,column,0x619,29);
        manager_club = extended(std::int16_t(control));
        p.history(0x1041b4a8, screens, 21, manager_club);
    }
    void amount_menus() {
        current_contract = 0;
        while (compare(c, std::int16_t(std::uint32_t(current_contract)), 3) < 0) {
            if (compare(c, std::int16_t(std::uint32_t(current_contract))) == 0) {
                draft_slot_or_club = extended(c.i16(frame.at(0x314))); pending = 6;
            } else if (compare(c, std::int16_t(std::uint32_t(current_contract)), 1) == 0) {
                draft_slot_or_club = clubs; pending = 17;
            } else { draft_slot_or_club = bid_selector; pending = 22; }
            if (compare(c, std::int16_t(std::uint32_t(draft_slot_or_club)), -1) != 0) {
                manager_club = 0;
                while (compare(c, std::int16_t(std::uint32_t(manager_club)), 10) < 0) {
                    p.call(0x1000a330, 0x1041b50c,
                        {frame.at(0x8c),std::uint32_t(std::int32_t(std::int8_t(std::uint32_t(manager_club)))),frame.at(0x15c),10});
                    p.call(0x10009f20, 0x1041b518, {frame.at(0x8c),5});
                    p.call(0x1000b410, 0x1041b530, {text,2000,c.r(3),c.r(4),5});
                    active_offer = c.u32(frame.at(0x8c));
                    stack_arguments({16,c.u16(primary_colour),c.u16(selected_colour),1,1,c.u16(secondary_colour),
                                     text,0,pending,active_offer,draft_slot_or_club});
                    p.call(0x10719740, 0x1041b5ac, {controls(),2,0,0,0,0,0,0});
                    manager_club = manager_club + 1;
                }
            }
            current_contract = current_contract + 1;
        }
    }
    void run() {
        if (!acquire_context()) return;
        ensure_draft(); heading_and_other_bids(); player_introduction(); valuation_and_board_limit();
        base_fee_row(); small_term_row(true); small_term_row(false);
        conditional_fee_row(false); conditional_fee_row(true);
        exchange_controls(); exchange_players_and_total(); footer(); amount_menus();
        cleanup(0x1041b5d8,0x1041b5e4);
    }
};
}
void render_proposal(Cpu& c) { ProposalScreen screen(c); screen.run(); }
}
