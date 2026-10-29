// Deterministic external service boundaries; both sides execute complete owned bodies.
#include "transfer_screens_adapter.hpp"
#include "routine_check.hpp"
#include "semantic_io_checks.hpp"
#include "guest_abi.hpp"
#include <array>
#include <cstdlib>
#include <iostream>
#include <limits>
#include <map>
#include <set>
#include <vector>

#if defined(CM_TRANSFER_SCREENS_COVERAGE)
extern "C" int __llvm_profile_write_file(void);
#endif
namespace cm::aot { bool transfer_screens_reference(State&); }
using namespace cm::aot;
using guest::Cpu;
namespace {
constexpr std::uint32_t data = 0x60000000, screens = data + 0x10000,
    player = data + 0x1000, link = data + 0x2000, offer = data + 0x3000,
    contract = data + 0x4000, clubs = data + 0x5000, people = data + 0x8000,
    history = data + 0x20000, trace_at = data + 0x30000,
    stack = 0x71010000, sentinel = 0xfffffff0;
constexpr std::array entries{0x10417590u, 0x10417880u, 0x1041e760u, 0x1041ea60u, 0x10422570u,
    0x10421a20u, 0x10421af0u, 0x10421be0u, 0x10421ce0u, 0x10421dd0u,
    0x1041f830u, 0x10420aa0u, 0x10422ac0u, 0x10423160u, 0x104248a0u,
    0x10424930u, 0x10426550u, 0x1042a780u, 0x1042ae20u, 0x1042b500u,
    0x1042ed80u, 0x1042ee90u, 0x10428650u, 0x10420e40u, 0x10421960u, 0x10422a00u,
    0x10426830u, 0x1042b3a0u, 0x10426970u, 0x10423040u, 0x1041e770u, 0x1041b600u, 0x10417b60u, 0x10420b50u, 0x10420f00u, 0x10421070u, 0x1041eb10u, 0x1041f3a0u, 0x10422600u, 0x10422b50u, 0x10422e40u, 0x10421fb0u, 0x10422310u, 0x1042c6d0u, 0x10421ee0u, 0x104265c0u, 0x104286c0u, 0x10428750u, 0x1042a810u, 0x1042abb0u, 0x1042f670u, 0x1042f7c0u, 0x1042fb30u, 0x104249e0u, 0x10424d40u, 0x10424f80u, 0x1042efa0u, 0x1042f380u, 0x1042ae90u, 0x1042bf70u, 0x1042c2c0u, 0x1042b580u, 0x1042bae0u, 0x10425980u, 0x10425240u, 0x1041f8f0u, 0x104204f0u, 0x10423270u, 0x10424220u};
struct Case {
    std::uint32_t entry;
    int create = 1, create_result = 1, contract_present = 1, contract_type = 0,
        link_present = 1, offer_present = 1, status = 5, response = 0,
        eligible = 1, controlled = 1, group = 0, news_revision = 17,
        action = 0, default_action = 0, control = 0, command = 0, fallback = 1,
        candidates = 1, actual_query = 0, clear_link = 0, mixed_terms = 0,
        fee = 100, league_fee = -1, international_fee = -1,
        exchange_count = 0, exchange_list = 0, proposals_equal = 1,
        negotiation_flags = 0, actual_total_services = 0, monetary_high = 0,
        installments = 12, sell_on = 10, appearances = 10, public_bid = 0, offer_kind = 0,
        board_limit = 1000000, valuation = 1000000, bid_revision = 17, increment = 1000,
        player_id = 3, bidder_id = 2, selected_value = 25000,
        total_example = 0, distinct_lists = 0, reference_adjustment = 0, term_difference = 0,
        actual_reset_services = 0, destroy_revision_delta = 0,
        pending_offer = 0, removing_exchange = 0, draft_present = 1, negotiated_id = -1,
        manager_present = 1, manager_club_present = 1, person_present = 1, manager_club_id = 2,
        pending_offers = 0, board_enabled = 0, board_present = 1, board_reason = 1, board_decision = 0,
        delay_count = 0, row_height = 21, first_control = 10, board_context = 1,
        contract_null_query = 0, contract_type_query = 0,
        reason = 0, feedback_kind = 0, feedback_offer_id = 17, feedback_pattern = 0,
        temporary_offer = 0, draft_link_mode = 1, original_link_present = 1,
        mutate_fee = 0, mutate_payload = 0, varying_owner = 0,
        selected_player_id = 4, current_salary = 1000, offered_salary = 500, proposed_present = 1,
        term_byte_a = 13, term_byte_b = 128, term_word_a = 2005, term_word_b = 7,
        expiry_year = 2002, cup_option = 1, parent_option = 128, history_terms_mutation = 0,
        loan_mode = 0, transfer_list_result = 0, loan_list_result = 0,
        status_mask = 0, squad_status = 7, registration = 1, club_kind = 0,
        happiness = 60, player_age = 14, listing_eligible = 1, status_listed_value = 0,
        status_history_mutation = 0, actual_header = 0, same_owner = 1, popup_active = 0, popup_action = 0, recall_mutation = 0, alternate_club_present = 1, current_day = 29, second_day = 31, day_mutation = 0, actual_trial_constructor = 0, temporary_club_mutation = 0, release_days = 365, release_salary_mutation = 0, proposed_type = 24, news_proposed_present = 1, random_word = 0, random_salt = 0, random_wrap = 0, shortlist_rating = 11100, club_restricted = 0, free_move = 0, budget_high = 0, bank_high = 0, bank_low = 1000000, shortlist_mutation = 0, rejection_mask = 15, pending_failure = -1, pending_pattern = 0, rejection_mutation = 0, completion_status = 0, completion_mutation = 0, completion_constant = 0;
    double interest = 3.0;
    std::uint32_t mode = 1;
};
Case active{entries[0]};
bool original_mode = false, production_mode = false;
bool preparing_heap = false;
unsigned call_count = 0;
const std::set<std::uint32_t> dependencies{
    0x1073e4b0, 0x1073f1e0, 0x10082f20, 0x103f8820, 0x101929b0,
    0x102bdf90, 0x102ccdd0, 0x102cb570, 0x103eb440, 0x10094ae0,
    0x103ea350, 0x1073ef70, 0x10775170, 0x102a54b0, 0x102a83a0,
    0x103e44b0, 0x103e6290, 0x10718360, 0x10009f20, 0x10738e00,
    0x1000b410, 0x10741da0, 0x10416e10, 0x10727890,
    0x10742100, 0x10741450, 0x10740620, 0x10148930, 0x10411f10,
    0x103f4d10, 0x10103510, 0x100f5770, 0x1000a060, 0x1000a170,
    0x10741a80, 0x10742ec0, 0x1077b720, 0x1000a010, 0x100546e0,
    0x103eb470, 0x10773520, 0x10411e60, 0x10738c00, 0x10739010,
    0x10738e60, 0x10738c20, 0x1000a330, 0x100c7a50, 0x100caeb0, 0x102a1070,
    0x103eaea0, 0x10411ea0, 0x106fdb00, 0x106fe280, 0x107136d0,
    0x10719740, 0x107198c0, 0x10739890, 0x10744390, 0x10744430,
    0x107444e0, 0x107445a0, 0x1074b6e0, 0x1074b7c0, 0x1074b830,
    0x1074bab0, 0x1074bcd0, 0x1074c570, 0x10768810, 0x10769b60, 0x1076d470,
    0x10744250, 0x107442f0, 0x1074a350, 0x103ea290, 0x1007f720, 0x10168a00,
    0x103f89e0, 0x10716eb0, 0x103e8500, 0x103e4130, 0x103e25d0, 0x103e4390,
    0x103e2db0, 0x103e4c90, 0x103e4740, 0x10416710, 0x103f6240,
    0x103e95a0, 0x1014c2e0, 0x103e4d70, 0x102cd0c0, 0x103fa8e0,
    0x10082f90, 0x100d3040, 0x100d3230, 0x10084be0,
    0x10365cc0, 0x10365c80, 0x10365c30, 0x10363130, 0x1037f110, 0x1037fa80, 0x103fa850, 0x100f05a0, 0x100f0720, 0x1007fb20, 0x1030ffa0, 0x103721f0, 0x10372260, 0x1007f200, 0x107182f0, 0x107156f0, 0x103e3240, 0x103e3170, 0x10374d10, 0x103fa290, 0x100f2680, 0x103ecb80, 0x1030fda0, 0x10310310, 0x100c7d00, 0x10369080, 0x1036fbf0, 0x10089a40, 0x10085860, 0x10698bb0, 0x103ea440, 0x103ea600, 0x103ea760, 0x10148a90, 0x103f9d60, 0x103e9910, 0x103e80e0};

void trace(Cpu& c, std::uint32_t entry) {
    if (++call_count > 1000) throw std::runtime_error("fixture call limit");
    auto n = c.u32(trace_at);
    const auto put = [&](std::uint32_t value) { c.put32(trace_at + 4 + 4 * n++, value); };
    put(entry); put(c.lr());
    for (unsigned r = 3; r <= 10; ++r) put(c.r(r));
    for (unsigned r = 13; r <= 31; ++r) put(c.r(r));
    for (unsigned f = 0; f < 8; ++f) put(c.cr(f));
    // UI factories also consume these stack arguments: compare the controls'
    // colours, captions, actions, payloads and parent IDs as well as r3-r10.
    unsigned stack_arguments = 0;
    switch (entry) {
    case 0x10719740: stack_arguments = 11; break;
    case 0x107198c0: case 0x107444e0: stack_arguments = 4; break;
    case 0x10744430: stack_arguments = 5; break;
    case 0x10744390: stack_arguments = 3; break;
    case 0x1074a350: stack_arguments = active.entry == 0x10422600 ? 12 : (active.entry == 0x10422b50 || active.entry == 0x1042f7c0 || active.entry == 0x1042ae90) ? 9 : (active.entry == 0x104265c0 || active.entry == 0x1042a810 || active.entry == 0x104249e0 || active.entry == 0x10424f80 || active.entry == 0x1042efa0) ? 6 : 3; break;
    case 0x10727890:
        if (c.lr() == 0x1041efa8) stack_arguments = 3;
        if (c.lr() == 0x10422170) stack_arguments = 2;
        break;
    case 0x107445a0: stack_arguments = 2; break;
    case 0x1074bcd0: stack_arguments = 1; break;
    }
    for (unsigned i = 0; i < stack_arguments; ++i) put(c.u32(c.sp() + 0x38 + i * 4));
    c.put32(trace_at, n);
}
bool probe(State& s) {
    Cpu c(s); const auto target = s.pc;
    if (preparing_heap) return false;
    if (target >= 0x10417590 && target < 0x1042fe10) {
        if (original_mode) return transfer_screens_reference(s);
        if (production_mode) return false;
        if (transfer_screens_lists(target)) { transfer_screens_run_rewrite(s, target); return true; }
        throw std::runtime_error("Unimplemented owned dependency in fixture " + hex_address(target));
    }
    // Cpu::call executes compiler leaf entries before the probe. Execute the
    // same actual leaf on the reference's dispatcher path instead of modeling
    // only one side's call (notably the exchange-list constructor).
    if (is_leaf_entry(target)) return run_leaf(s);
    if (active.actual_reset_services && (target == 0x10411e60 || target == 0x10411f10 ||
        target == 0x10773520 || target == 0x10775170 || target == 0x10739010 ||
        target == 0x10738e60 || target == 0x10738c20)) return false;
    if (!dependencies.contains(target)) return false;
    trace(c, target);
    if (std::getenv("CM_TRANSFER_SCREENS_TRACE"))
        std::cerr << (original_mode ? "original " : "rewrite ") << hex_address(target)
                  << " lr=" << hex_address(c.lr()) << " r3=" << hex_address(c.r(3))
                  << " r4=" << hex_address(c.r(4)) << " r14=" << hex_address(c.r(14)) << '\n';
    if (target == 0x103eb440 && active.actual_query) return false;
    if ((target == 0x10009f20 && (active.actual_total_services & 1)) ||
        (target == 0x10738e00 && (active.actual_total_services & 2)) ||
        (target == 0x10416e10 && (active.actual_total_services & 4))) return false;
    if (active.actual_header && (target==0x10744250 || target==0x10719740)) return false;
    if (active.actual_trial_constructor && target==0x103721f0) return false;
    switch (target) {
    case 0x1073e4b0: c.set(3, active.create_result); break;
    case 0x1073f1e0:
        if (c.r(4) >= 32) throw std::runtime_error("Invalid fixture history index");
        c.put32(history + c.r(4) * 4, c.r(5));
        // Noncanonical mutations characterize the caller's live history reads.
        if (active.status_history_mutation) {
            if (c.r(4)==7) c.put32(data+0xb024,clubs+3*0x248);
            if (c.r(4)==14) c.put8(contract+0x60,0xaf);
            if (c.r(4)==3) c.put8(contract+0x5c,4);
        }
        if (active.history_terms_mutation) {
            if (c.r(4)==1) c.put32(contract+12,c.u32(contract+12)+7);
            if (c.r(4)==12) { c.put16(data+0x4532,65530); c.put8(data+0x451c,255); c.put8(data+0x451d,127); }
        }
        break;
    case 0x10082f20: {
        const auto query = c.u32(data + 0x1b0) + 1; c.put32(data + 0x1b0, query);
        c.put8(contract + 0x38, active.contract_type_query == int(query) ? 20 : active.contract_type);
        if (active.varying_owner) c.put32(contract+4,query);
        c.set(3, active.contract_present && active.contract_null_query != int(query) ? contract : 0);
        break;
    }
    case 0x103f8820:
        // Dependency-boundary mutation checks the caller's live reread. This
        // characterizes the contract; it is not a claim that country rules do it.
        if (active.clear_link) c.put32(link, 0);
        c.set(3, active.eligible); break;
    case 0x101929b0: c.set(3, active.controlled); break;
    case 0x102bdf90:
        for (unsigned i = 0; i < 0xe4; ++i) c.put8(c.r(3) + i, 0);
        break;
    case 0x102ccdd0:
        for (unsigned i = 0; i < 13; ++i) c.put32(c.r(6) + 8 + i * 4, i == 0 ? 3 : 17);
        c.put32(c.r(6) + 8 + (active.entry == 0x10421be0 ? 8 : active.entry == 0x10421dd0 ? 12 : 7) * 4,
                active.news_revision);
        if (active.entry==0x104286c0) c.put32(c.r(6)+8+2*4,3);
        c.set(3, 1); break;
    case 0x102cb570: c.set(3, c.u32(c.r(3) + 8 + c.r(4) * 4)); break;
    case 0x103eb440:
        if (active.entry == 0x10421070) c.set(3,active.link_present ? link : 0);
        else c.set(3, (active.entry == 0x10417b60 || active.entry == 0x10420f00 || active.entry == 0x1041f8f0 || active.entry == 0x104204f0) && c.r(4) >= 20 ?
            data + 0x65000 + (c.r(4) - 20) * 4 : link);
        break;
    case 0x10094ae0: c.set(3, 1); break;
    case 0x103ea350: c.set(3, active.mode); break;
    case 0x1073ef70: c.set(3, c.u32(history + c.r(4) * 4)); break;
    case 0x10775170: c.put32(data + 0x120, c.r(3)); break;
    case 0x102a54b0: c.set(3, active.command); break;
    case 0x102a83a0: c.set(3, active.fallback); break;
    case 0x10718360: c.set(3, active.mode); break;
    case 0x103e44b0: case 0x103e6290:
    case 0x103e8500: case 0x103e4130: case 0x103e25d0: case 0x103e4390:
    case 0x103e2db0: case 0x103e4c90: case 0x103e4740:
        if (active.entry == 0x1041f3a0) {
            if (active.mutate_payload) c.put32(data+0x6f00,data+0x7100);
            if (active.mutate_fee) c.put32(c.u32(data+0x6f00)+0x20,
                c.u32(offer+0x20)+(active.mutate_fee==2 ? 17 : 0));
        }
        if (active.entry==0x104204f0 && active.rejection_mutation>=2) {
            if (active.rejection_mutation==3) c.put32(data+0x6f00,data+0x7100);
            c.put32(c.u32(data+0x6f00)+0x20,c.u32(offer+0x20)+(active.rejection_mutation==2 ? 0 : 37));
        }
        c.set(3,1); break;
    case 0x10009f20: {
        const auto amount = c.i32(c.r(3));
        c.set(4, std::uint32_t(amount));
        c.set(3, active.monetary_high ? std::uint32_t(active.monetary_high) : amount < 0 ? 0xffffffffu : 0u);
        break;
    }
    case 0x10738e00: {
        const auto cursor = c.u32(c.r(3) + 12);
        if (cursor) { c.put32(c.r(3) + 12, c.u32(cursor + 8)); c.set(3, c.u32(cursor)); }
        else c.set(3, 0);
        break;
    }
    case 0x10416e10: c.set(3, active.proposals_equal); break;
    case 0x1000b410:
        if (active.total_example && (active.entry != 0x10417b60 || c.r(7) == 0x22)) {
            const auto expected = active.total_example == 1 ? 6000000000ull : 12000ull;
            if (((std::uint64_t(c.r(5)) << 32) | c.r(6)) != expected)
                throw std::runtime_error("Actual monetary total disagrees with independent expected value");
        }
        c.put32(c.r(3), c.r(5)); c.put32(c.r(3) + 4, c.r(6)); break;
    case 0x10727890: c.put32(c.r(3), c.r(4)); break;
    case 0x10741da0: case 0x10742100: case 0x10741450: case 0x10740620: break;
    case 0x10148930: c.set(3, data + 0x6f00); break;
    case 0x10411f10:
        if (active.entry==0x10424220 && active.completion_mutation==2) c.put32(link,data+0x7100);
        c.put32(data + 0x170, c.r(3));
        if (active.destroy_revision_delta) c.put32(history + 12, c.u32(history + 12) + active.destroy_revision_delta);
        break;
    case 0x103f4d10: c.put16(c.r(6), 0xff85); c.set(3, active.eligible); break;
    case 0x10103510: c.set(3, active.entry==0x10425240 && !active.board_present ? 0 : data + 0xa400); break;
    case 0x100f5770:
        if (active.entry==0x10425240) { c.set({{3,std::uint32_t(active.budget_high)},{4,std::uint32_t(active.board_limit)}}); break; }
        if (c.r(6)) c.put8(c.r(6), active.board_reason);
        c.set({{3, 0}, {4, std::uint32_t(active.board_limit)}}); break;
    case 0x1000a060: c.put32(c.r(3), c.u32(c.r(3)) + std::uint32_t(active.increment)); break;
    case 0x1000a170: c.put32(c.r(3), c.u32(c.r(3)) - std::uint32_t(active.increment)); break;
    case 0x1077b720: c.put32(c.r(3), c.r(4)); break;
    case 0x1000a010: c.put32(c.r(3), c.r(5)); break;
    case 0x103eb470: c.set(3, data + 0x6e00); break;
    case 0x10773520: {
        const auto allocated = data + 0x70000 + c.u32(data + 0x160) * 0x100;
        c.put32(data + 0x160, c.u32(data + 0x160) + 1);
        for (unsigned i = 0; i < c.r(3); ++i) c.put8(allocated + i, 0x5a);
        c.set(3, allocated); break;
    }
    case 0x10411e60: break; // Field copying must preserve the allocation's padding.
    case 0x10738c00: for (unsigned i = 0; i < 16; i += 4) c.put32(c.r(3) + i, 0); break;
    case 0x10739010:
        for (unsigned i = 0; i < 16; i += 4) c.put32(c.r(3) + i, c.u32(c.r(4) + i)); break;
    case 0x10738e60: c.put16(c.r(3), c.u16(c.r(3)) - 1); break;
    case 0x10738c20: c.put32(data + 0x174, c.r(3)); break;
    case 0x10741a80: case 0x10742ec0: case 0x100546e0: break;
    case 0x1076d470: c.set(3, active.row_height); break;
    case 0x100c7a50: c.put16(c.r(5), 0x1234); c.put16(c.r(6), 0x2345); break;
    case 0x100caeb0:
        c.put32(c.r(4),c.r(3));
        if (active.entry==0x1042f7c0 && active.recall_mutation) {
            const auto count=c.u32(data+0x204)+1; c.put32(data+0x204,count);
            if (count==2) c.put32(c.r(3)+0x24,clubs+unsigned(active.same_owner ? 1 : 2)*0x248);
        }
        break;
    case 0x107198c0: case 0x10719740: case 0x10744430: case 0x10744390:
    case 0x107444e0: case 0x107445a0:
        c.set(3, c.u32(data + 0x194)); c.put32(data + 0x194, c.r(3) + 1); break;
    case 0x1074b6e0: c.put32(c.r(3), 0); c.put32(c.r(3) + 4, 0); break;
    case 0x1074bcd0:
        c.put16(c.sp() + 0x32, c.r(9)); // Original parameter home-slot spill.
        c.put32(c.r(7), c.u32(c.r(7)) + 1); break;
    case 0x1000a330: c.put32(c.r(3), c.u32(c.r(5) + c.r(4) * 4)); break;
    case 0x106fdb00: c.set(3, active.board_present ? data + 0x1e0 : 0); break;
    case 0x106fe280: c.set(3, active.board_decision); break;
    case 0x10411ea0:
        if(active.entry==0x10423270) {
            // Deliberately distinct constructor output characterizes the
            // caller's field assignment and preservation of record gaps.
            c.s.memory.copy_in(c.r(3),std::vector<std::uint8_t>(0x54,0xd3));
        } else c.s.memory.copy_in(c.r(3), c.s.memory.copy_out(data + 0x7000, 0x54));
        break;
    case 0x10739890: c.put8(c.r(6), 0); c.put8(c.r(7), 0); break;
    case 0x10768810: c.set(3, 123); break;
    case 0x10769b60: c.set(3, 1); break;
    case 0x103eaea0:
        c.put16(c.r(5), active.pending_offers);
        c.put32(c.r(5) + 4, active.pending_offers ? data + 0x66000 : 0);
        c.put32(c.r(5) + 8, active.pending_offers ? data + 0x66000 + (active.pending_offers - 1) * 16 : 0);
        c.put32(c.r(5) + 12, 0); break;
    case 0x107136d0: case 0x102a1070: case 0x1074b7c0: case 0x1074b830:
    case 0x1074bab0: case 0x1074c570: case 0x10744250: case 0x107442f0: case 0x1074a350: break;
    case 0x103ea290: c.put32(c.r(5),clubs + active.bidder_id * 0x248); break;
    case 0x1007f720:
        if (active.entry==0x1042c6d0 || active.entry==0x10421ee0 || active.entry==0x1042bf70 || active.entry==0x1042c2c0) c.set(3,active.proposed_present ? data+0x4500 : 0);
        else c.set(3,active.contract_present ? contract : 0);
        break;
    case 0x10168a00: c.put32(c.r(4),c.r(3)); break;
    case 0x103f89e0: c.put16(c.r(5),17); c.put16(c.r(5)+2,2002); c.put8(c.r(5)+4,3); break;
    case 0x10716eb0: c.put32(c.r(4),c.u32(c.r(3))); break;
    case 0x10416710: c.set(3,123456); break;
    case 0x103f6240: c.put16(c.r(7),0x37); c.set(3,1); break;
    case 0x10148a90: c.set(3,data+0x6f00); break;
    case 0x103f9d60:
        c.put16(c.r(6),active.reason);
        if (active.completion_mutation==1) c.put32(c.r(4)+0x20,c.u32(c.r(4)+0x20)+17);
        c.set(3,active.completion_status); break;
    case 0x103e9910: case 0x103e80e0: c.put32(data+0x248,target); c.set(3,1); break;
    case 0x103ea440: {
        if (c.lr()==0x10420234) {
            const auto i=(c.r(5)-(data+0x64000))/0x100;
            c.set(3,int(i)==active.pending_failure ? 0 : active.eligible);
        } else {
            unsigned bit=c.r(4)==0x2000 ? 0 : c.r(4)==0x1000 ? 1 : c.r(4)==10 ? 2 : 3;
            c.set(3,(active.rejection_mask&(1u<<bit)) ? active.eligible : 0);
        }
        break;
    }
    case 0x103ea600: c.put32(c.r(5),c.r(4)); break;
    case 0x103ea760:
        if (active.rejection_mutation==1) c.put32(data+0x7020,c.u32(offer+0x20)+37);
        c.set(3,active.selected_value); break;
    case 0x1030fda0: case 0x10310310:
        c.put32(data+0x23c,target);
        if (active.shortlist_mutation) c.put32(data+0xb024,clubs+3*0x248);
        break;
    case 0x100c7d00: c.set(3,active.club_restricted); break;
    case 0x10369080: c.set_f(1,active.interest); break;
    case 0x1036fbf0: c.put32(c.r(4),23); c.put32(c.r(7),29); c.set(3,active.free_move); break;
    case 0x10089a40: case 0x10085860: c.set(3,active.selected_value); break;
    case 0x10698bb0: c.put32(data+0x240,c.r(6)); c.put32(data+0x244,c.r(5)); break;
    case 0x103fa290: c.put32(c.r(6),0xdeadbeef); c.set(3,active.eligible); break;
    case 0x100f2680: c.set(3,active.board_limit); break;
    case 0x103ecb80: c.set(3,active.news_proposed_present ? data+0x4500 : 0); break;
    case 0x10374d10:
        if (active.release_salary_mutation) c.put32(c.r(3)+12,c.u32(c.r(3)+12)+17);
        c.set(3,active.release_days); break;
    case 0x103721f0:
        // Field-specific constructor model; untouched contract gaps retain the
        // caller's stack bytes. Actual-constructor cases bypass this boundary.
        for (auto offset:{0x28u,0x30u}) { c.put16(c.r(3)+offset,40); c.put16(c.r(3)+offset+2,1972); c.put8(c.r(3)+offset+4,1); }
        c.put32(c.r(3)+4,0xffffffff); c.put8(c.r(3)+0x54,0); c.put32(c.r(3)+0x58,0xffffffff);
        c.put16(c.r(3)+0x42,0); c.put8(c.r(3)+0x5c,0); c.put32(c.r(3)+0x48,0);
        c.put32(c.r(3)+0x50,0); c.put32(c.r(3)+8,0);
        if (active.temporary_club_mutation) c.put32(data+0xb024,clubs+3*0x248);
        break;
    case 0x10372260:
        c.put32(c.r(3),c.u32(c.r(4))); c.put32(c.r(3)+4,c.u32(c.r(5)));
        c.put32(data+0x220,c.r(6)); break;
    case 0x1007f200:
        for (unsigned i=0;i<0x64;++i) c.put8(data+0x6d000+i,c.u8(c.r(4)+i));
        c.put32(data+0x224,target); c.set(3,23); break;
    case 0x107182f0: {
        const auto n=c.u32(data+0x218)+1; c.put32(data+0x218,n);
        c.set(3,active.day_mutation && n>1 ? active.second_day : active.current_day); break;
    }
    case 0x107156f0:
        // Deterministic five-byte boundary payload captures day/year/month;
        // it is not a calendar-conversion implementation or real-world date.
        c.put16(c.r(3),c.r(4)); c.put16(c.r(3)+2,c.r(6)); c.put8(c.r(3)+4,c.r(5));
        if (active.temporary_club_mutation) c.put32(data+0xb024,clubs+3*0x248);
        break;
    case 0x103e3240: case 0x103e3170: {
        const auto date=c.r(target==0x103e3240 ? 5 : 6);
        for (unsigned i=0;i<5;++i) c.put8(data+0x6d000+i,c.u8(date+i));
        c.put32(data+0x224,target); c.set(3,29); break;
    }
    case 0x1030ffa0: c.put32(data+0x208,c.r(6)); break;
    case 0x103fa850: c.put32(c.r(6),0xcafebabe); c.set(3,active.eligible); break;
    case 0x100f05a0: case 0x100f0720: c.set(3,17); break;
    case 0x1007fb20:
        c.put32(data+0x1fc,c.r(5));
        if (active.recall_mutation) { c.put32(contract+4,3); c.put32(c.r(4)+0x24,clubs+unsigned(active.same_owner ? 1 : 2)*0x248); }
        break;
    case 0x10082f90: c.set(3,active.contract_present ? contract : 0); break;
    case 0x100d3040: c.set(3,active.transfer_list_result); break;
    case 0x100d3230: c.set(3,active.loan_list_result); break;
    case 0x10084be0: c.set(3,active.eligible); break;
    case 0x10365cc0: case 0x10365c80: case 0x10365c30: case 0x10363130:
        c.put32(data+0x1f0,target); break;
    case 0x1037f110: c.set(3,active.listing_eligible); break;
    case 0x1037fa80: c.put32(data+0x1f4,c.r(4)); break;
    case 0x103e95a0: c.put32(data+0x1e8,c.r(4)); break;
    case 0x1014c2e0: c.set(3,active.controlled); break;
    case 0x103e4d70: c.put32(data+0x1ec,c.r(4)); break;
    case 0x102cd0c0: c.put32(data+0x68000+c.r(6)*4,c.r(7)); break;
    case 0x103fa8e0:
        c.put8(c.r(7),active.term_byte_a); c.put16(c.r(8),active.term_word_a);
        c.put8(c.r(9),active.term_byte_b); c.put16(c.r(10),active.term_word_b); break;
    }
    s.pc = c.lr(); return true;
}
void actual_heap_offers(Cpu& c, const Case& test) {
    constexpr auto context = data + 0x7f000, classes = data + 0x7f100, arena = data + 0x80000;
    const auto registers = c.s.registers;
    c.s.memory.clear(context, 52);
    c.put32(c.r(2) - 0x3b74, context); c.put8(c.r(2) - 0x3510, 1); c.put32(c.r(2) - 0x3b70, classes);
    unsigned n = 0;
    for (auto size : {4u, 12u, 28u, 44u, 60u, 76u}) c.put32(classes + n++ * 4, size);
    c.put32(arena, arena); c.put32(arena + 4, arena); c.put32(arena + 16, 4096 - 32);
    c.put32(arena + 20, 4096 | 3); c.put32(arena + 24, 4096 - 32); c.put32(arena + 28, arena | 1);
    c.put32(arena + 32, arena + 24); c.put32(arena + 36, arena + 24);
    c.put32(arena + 4096 - 12, 4096 - 32); c.put32(arena + 4096 - 8, 4096 | 3);
    c.put32(arena + 4096 - 4, arena + 24); c.s.allocated[arena] = 4096; c.put32(context, arena);
    const auto invoke = [&](std::uint32_t entry, std::initializer_list<std::uint32_t> arguments) {
        unsigned r = 3; for (auto argument : arguments) c.set(r++, argument);
        c.call(entry, sentinel); return c.r(3);
    };
    preparing_heap = true;
    const auto make_list = [&] {
        if (!test.exchange_list) return 0u;
        const auto list = invoke(0x10773520, {16}); invoke(0x10738c00, {list});
        for (int i = 0; i < test.exchange_count; ++i) invoke(0x10738d10, {list, data + 0x61000 + i * 4, 4});
        return list;
    };
    const auto make_offer = [&](std::uint32_t source) {
        const auto value = invoke(0x10773520, {0x54}); invoke(0x10411e60, {value});
        c.s.memory.copy_in(value, c.s.memory.copy_out(source, 0x54));
        c.put32(value, 0); c.put32(value + 4, make_list()); return value;
    };
    const auto allocated_draft = (test.entry == 0x10417b60 || test.entry==0x10423270) && !test.draft_present ? 0u : make_offer(data + 0x7000);
    const auto allocated_offer = make_offer(offer);
    c.put32(data + 0x6f00, allocated_draft); c.put32(link, allocated_offer); c.put32(data + 0x6e00, allocated_offer);
    c.put32(data + 0x190, allocated_offer);
    if (test.action == 31 && test.control != -1)
        c.put32(data + 0x100000 + 0xba8ac + std::uint32_t(std::int16_t(test.control)) * 0x17c,
                c.u32(c.u32(c.u32(allocated_draft + 4) + 4)));
    preparing_heap = false; c.s.registers = registers; c.s.executed = 0;
}
void initialize(Cpu& c, const Case& test) {
    c.s.memory.map(data, 0x400000); c.s.memory.map(stack - 0x10000, 0x20000);
    c.put32(data + 0x160, 0);
    c.put32(data + 0x1b0, 0);
    c.put32(data + 0x1e8, 0); c.put32(data + 0x1ec, 0);
    c.put32(data+0x1f0,0); c.put32(data+0x1f4,0);
    c.s.memory.copy_in(stack - 0x10000, std::vector<std::uint8_t>(0x20000, 0xa5));
    for (unsigned r = 0; r < 32; ++r) c.set(r, 0x51000000 + r * 0x101);
    c.set(1, stack); c.set(2, 0x108bba90); c.set_lr(sentinel);
    c.s.budget = 100000;
    for (unsigned f = 0; f < 8; ++f) c.set_cr(f, f + 1);
    const auto variable = [&](int slot, unsigned offset, std::uint32_t value) {
        c.put32(c.r(2) + std::uint32_t(slot), data + offset); c.put32(data + offset, value);
    };
    c.put32(c.r(2) - 0x7bb0, screens);
    variable(-0x7b78, 0x100, 0); variable(-0x7b7c, 0x104, data + 0x9000);
    variable(-0x7be8, 0x108, clubs); variable(-0x7be0, 0x10c, people);
    variable(-0x7b40, 0x110, data + 0xa000);
    variable(-0x7bf0, 0x114, data + 0xd000);
    c.put32(c.r(2) - 0x7b80, data + 0x130); c.put16(data + 0x132, 2002);
    c.put32(player, 3); c.put32(link, test.offer_present ? offer : 0);
    c.put32(data + 0xa000, data + 0x2200); c.put32(data + 0xa004, 18);
    for (unsigned i = 0; i < 18; ++i) c.put32(data + 0x2200 + i * 4, offer);
    c.put32(people + 3 * 0x70, 3);
    c.put32(offer + 8, 2); c.put32(offer + 12, 3); c.put32(offer + 0x2c, 17);
    c.put8(offer + 0x34, test.status); c.put8(offer + 0x35, test.response);
    c.put32(contract + 4, 1); c.put8(contract + 0x38, test.contract_type);
    c.put16(screens + 0x2c58, test.group);
    c.put32(screens + std::uint32_t(test.group) * 0x2c0 + 4, data + 0xb000);
    c.put32(screens + std::uint32_t(test.group) * 0x2c0 + 0x24, data + 0x100000);
    c.put32(screens + std::uint32_t(test.group) * 0x2c0 + 0x10, data + 0xe000);
    c.put32(data + 0xb024, data + 0xc000);
    c.put16(screens + 0x25819e, test.default_action);
    if (test.entry != 0x1041e770 && std::int16_t(test.control) >= 0)
        c.put32(data + 0x100000 + 0xba86c + std::uint32_t(std::int16_t(test.control)) * 0x17c, test.action);
    c.put32(history + 16, test.candidates ? data + 0x40000 : 0);
    c.put32(history + 20, 23);
    c.put32(history, offer); c.put32(history + 4, 2);
    c.set({{3, player}, {4, test.link_present ? link : 0}, {5, 0x12345678}});
    if (test.entry == 0x10417880) c.set({{3, link}, {4, std::uint32_t(test.create)}});
    if (test.entry == 0x10422570) c.set({{3, offer}, {4, 17}});
    if (test.entry == 0x10423160) c.set(3, link);
    if (test.entry == 0x104248a0 || test.entry == 0x1042a780 || test.entry == 0x1042ed80) c.set(4, test.mode);
    if (test.entry == 0x10424930 || test.entry == 0x10420aa0) c.set(5, test.mode);
    if (test.entry == 0x1042ee90) for (unsigned r = 4; r <= 9; ++r) c.set(r, test.mode);
    if (test.mixed_terms) c.set({{4, 2}, {5, 2002}, {6, 50}, {7, 1}, {8, 0}, {9, offer}});
    if (test.entry == 0x10420e40 || test.entry == 0x10421960 || test.entry == 0x10422a00 ||
        test.entry == 0x10426830 || test.entry == 0x1042b3a0)
        c.set(3, test.control);
    if (test.entry == 0x1041e770 || test.entry == 0x1041b600 || test.entry == 0x10417b60 || test.entry==0x10423270) {
        constexpr auto draft = data + 0x7000;
        c.put_f64(c.r(2) - 0x3aec, 1.0); c.put_f64(c.r(2) - 0x3afc, 1.0);
        c.put32(c.r(2) - 0x7bb8, data + 0x50000);
        c.put32(draft + 0x20, test.fee);
        c.put32(draft + 0x40, test.league_fee); c.put32(draft + 0x48, test.international_fee);
        c.put32(draft + 4, test.exchange_list ? data + 0x60000 : 0);
        c.put16(data + 0x60000, test.exchange_count);
        c.put32(data + 0x60004, test.exchange_count ? data + 0x60100 : 0);
        c.put32(data + 0x60008, test.exchange_count ? data + 0x60100 + (test.exchange_count - 1) * 16 : 0);
        c.put32(data + 0x6000c, data + 0x60fff);
        for (int i = 0; i < test.exchange_count; ++i) {
            const auto node = data + 0x60100 + i * 16;
            c.put32(node, data + 0x61000 + i * 4);
            c.put16(node + 4, 4);
            c.put32(node + 8, i + 1 < test.exchange_count ? node + 16 : 0);
            c.put32(node + 12, i ? node - 16 : 0);
            c.put32(data + 0x61000 + i * 4, i + 3);
            c.put32(people + (i + 3) * 0x70 + 8, 0x40000000 + i * 17);
        }
        // Equal scalars and a shared exchange list are valid comparator inputs.
        for (unsigned i = 0; i < 0x54; i += 4) c.put32(offer + i, c.u32(draft + i));
        c.put8(offer + 0x3c, test.negotiation_flags);
        if (!test.proposals_equal) c.put32(offer + 0x20, std::uint32_t(test.fee) ^ 1);
        if (test.reference_adjustment) c.put32(offer + 0x20, std::uint32_t(test.fee) + test.reference_adjustment);
        if (test.total_example == 2) {
            c.put32(people + 3 * 0x70 + 8, 999); c.put32(people + 4 * 0x70 + 8, 1499);
        }
        if (test.distinct_lists) {
            c.put32(offer + 4, data + 0x62000); c.put16(data + 0x62000, test.exchange_count);
            c.put32(data + 0x62004, test.exchange_count ? data + 0x62100 : 0);
            c.put32(data + 0x62008, test.exchange_count ? data + 0x62100 + (test.exchange_count - 1) * 16 : 0);
            c.put32(data + 0x6200c, data + 0x62fff);
            for (int i = 0; i < test.exchange_count; ++i) {
                const auto node = data + 0x62100 + i * 16;
                c.put32(node, data + 0x63000 + i * 4); c.put16(node + 4, 4);
                c.put32(node + 8, i + 1 < test.exchange_count ? node + 16 : 0);
                c.put32(node + 12, i ? node - 16 : 0);
                c.put32(data + 0x63000 + i * 4, test.distinct_lists == 1 ? test.exchange_count - i + 2 : 3);
            }
        }
        if (test.term_difference == 1) {
            c.put32(offer + 0x40, 0); c.put8(offer + 0x44, 10); c.put32(offer + 0x48, -7);
            c.put8(offer + 0x50, 128); c.put8(offer + 0x51, 255);
        }
        if (test.term_difference == 2) { c.put32(offer + 0x40, 1000); c.put8(offer + 0x44, 10); }
        if (test.term_difference == 3) c.put8(offer + 0x50, 5);
        if (test.term_difference == 4) c.put8(offer + 0x38, 0x20);
        for (unsigned i = 20; i <= 22; ++i) c.put32(history + i * 4, std::uint32_t(test.control) + i - 20);
        c.set({{3, draft}, {4, test.link_present ? link : 0}});
        if (test.entry == 0x1041b600 || test.entry == 0x10417b60) {
            variable(-0x7a54, 0x144, data + 0xa300); variable(-0x7ab8, 0x148, data + 0xa400);
            for (const auto [toc_offset, offset, colour] : std::array{
                std::array{-0x7a74, 0x180, 0x1234}, std::array{-0x7bc4, 0x184, 0x2345}, std::array{-0x7bc8, 0x188, 0x3456}}) {
                c.put32(c.r(2) + std::uint32_t(toc_offset), data + offset); c.put16(data + offset, colour);
            }
            c.put32(data + 0x6f00, draft); c.put32(data + 0x6e00, offer);
            c.put32(draft + 8, test.bidder_id); c.put32(draft + 12, test.player_id);
            c.put8(draft + 0x50, test.sell_on); c.put8(draft + 0x51, test.installments);
            c.put8(draft + 0x44, test.appearances); c.put8(draft + 0x4c, test.appearances);
            c.put8(draft + 0x35, test.public_bid); c.put8(draft + 0x38, test.offer_kind);
            c.put32(offer + 0x2c, test.bid_revision); c.put8(offer + 0x34, test.status);
            c.put32(history + 4, test.link_present ? link : 0); c.put32(history + 12, test.news_revision);
            c.put32(history + 24, test.create); c.put32(history + 36, test.eligible);
            c.put32(people + test.player_id * 0x70 + 8, test.valuation);
            c.put32(data + 0xb024, clubs + 2 * 0x248);
            for (unsigned i = 15; i <= 22; ++i) c.put32(history + i * 4, 10 + i * 4);
            if (std::int16_t(test.control) >= 0)
                c.put32(data + 0x100000 + 0xba8ac + std::uint32_t(std::int16_t(test.control)) * 0x17c,
                        (test.action == 31 || (!test.action && test.default_action == 31)) ?
                            data + 0x61000 : std::uint32_t(test.selected_value));
            c.set(3, test.control);
        }
        if (test.entry == 0x10417b60) {
            unsigned colour_index = 0;
            for (int slot : {-0x7bc4,-0x7bdc,-0x7bc8,-0x7bd4,-0x7bd0,-0x7bcc}) {
                const auto address = data + 0x400 + 4 * colour_index++;
                c.put32(c.r(2) + std::uint32_t(slot), address); c.put16(address, 0x3456 + colour_index);
            }
            c.put32(c.r(2) - 0x7ba8, data + 0x54000); c.put32(c.r(2) - 0x7bac, data + 0x198);
            variable(-0x797c, 0x19c, test.board_enabled);
            variable(-0x7978, 0x1a0, test.board_context ? data + 0x1a4 : 0);
            c.put8(data + 0x19c, test.board_enabled); c.put32(data + 0x194, test.first_control);
            c.put32(data + 0x6f00, test.draft_present ? draft : 0);
            c.put32(history, test.person_present ? test.player_id : 0);
            c.put32(history + 16, test.removing_exchange); c.put32(history + 28, test.pending_offer);
            c.put32(data + 0xb024, test.manager_club_present ? clubs + test.manager_club_id * 0x248 : 0);
            if (!test.manager_present) c.put32(screens + std::uint32_t(test.group) * 0x2c0 + 4, 0);
            if (!test.person_present) c.put32(data + 0x10c, 0);
            c.put8(draft + 0x34, test.status); c.put8(draft + 0x36, test.delay_count);
            c.put32(offer + 0x30, test.negotiated_id);
            for (int i = 0; i < 8; ++i) c.put8(clubs + i * 0x248 + 0x23b, i % 3);
            // The proposed target is player3; exchanged players are distinct.
            for (int i = 0; i < test.exchange_count; ++i) {
                c.put32(data + 0x61000 + i * 4, i + 4);
                c.put32(people + (i + 4) * 0x70 + 8, 0x40000000 + i * 17);
            }
            if (test.total_example == 2) {
                c.put32(people + 4 * 0x70 + 8, 999); c.put32(people + 5 * 0x70 + 8, 1499);
            }
            for (int i = 0; i < test.pending_offers; ++i) {
                const auto bid = data + 0x64000 + i * 0x100, node = data + 0x66000 + i * 16;
                c.put32(data + 0x65000 + i * 4, bid);
                c.put32(bid + 8, i == 1 ? test.bidder_id : 3 + i); c.put32(bid + 0x28, 20 + i);
                c.put8(bid + 0x34, i == 0 ? 1 : 0);
                c.put32(node, data + 0x67000 + i * 4); c.put16(node + 4, 4);
                c.put32(node + 8, i + 1 < test.pending_offers ? node + 16 : 0);
                c.put32(node + 12, i ? node - 16 : 0); c.put32(data + 0x67000 + i * 4, 20 + i);
            }
        }
    }
    if (test.entry==0x10423270 || test.entry==0x10424220) {
        c.put32(data+0x248,0);
        variable(-0x7a54,0x144,data+0xa300);
        c.put_f64(c.r(2)-0x3aec,1.0); c.put_f64(c.r(2)-0x3afc,1.0);
        c.put32(c.r(2)-0x7bb8,data+0x50000);
        unsigned n=0;
        for (int slot:{-0x7bc4,-0x7bc8,-0x7b14}) {
            const auto colour=data+0x400+4*n++;
            c.put32(c.r(2)+std::uint32_t(slot),colour); c.put16(colour,0x3456+n);
        }
        c.put32(c.r(2)-0x7ba8,data+0x54000); c.put32(c.r(2)-0x7bac,data+0x198);
        c.put32(data+0x194,test.first_control);
        c.put32(data+0x6f00,test.draft_present ? data+0x7000 : 0);
        c.put32(data+0xb024,clubs+test.manager_club_id*0x248);
        for (unsigned i=0;i<8;++i) {
            c.put32(clubs+i*0x248,i); c.put8(clubs+i*0x248+0x23b,(i+1)%3); c.put8(clubs+i*0x248+0x242,i%3);
        }
        c.put32(people+3*0x70+8,test.valuation);
        for (auto bid:{offer,data+0x7000,data+0x7100}) {
            c.put32(bid+8,test.bidder_id); c.put32(bid+12,3); c.put32(bid+0x2c,test.bid_revision);
            c.put8(bid+0x34,test.status); c.put32(bid+0x20,test.fee); c.put8(bid+0x50,test.sell_on);
        }
        if(test.entry==0x10424220) c.set(3,test.control);
        c.put32(history,link); c.put32(history+4,data+0xb000); c.put32(history+8,23);
        c.put32(history+12,test.reason); c.put32(history+16,test.news_revision);
        c.put32(history+20,3); c.put32(history+24,test.completion_status);
        if(test.entry==0x10423270) {
            // Exchange names are distinct from the subject's valuation.
            for(int i=0;i<test.exchange_count;++i)c.put32(data+0x61000+i*4,4+i);
        }
    }
    if (test.entry == 0x10420b50 || test.entry == 0x10420f00 || test.entry == 0x10421070) {
        c.put32(c.r(2)-0x7bb8,data+0x50000);
        unsigned n=0;
        for (int slot:{-0x7bc4,-0x7bc8,-0x7b14}) {
            const auto colour=data+0x400+4*n++;
            c.put32(c.r(2)+std::uint32_t(slot),colour); c.put16(colour,0x3456+n);
        }
        c.put32(data+0x194,test.first_control);
        variable(-0x7a18,0x1c0,1); variable(-0x6678,0x1c4,2); variable(-0x7b98,0x1c8,data+0x68000);
        for (unsigned i=0;i<8;++i) {
            c.put32(clubs+i*0x248,i); c.put8(clubs+i*0x248+0x23b,(i+1)%3);
            c.put8(clubs+i*0x248+0x242,i%3); c.put32(clubs+i*0x248+0x2c,data+0x68000+3*0x128);
        }
        c.put32(contract+0x70,test.feedback_offer_id);
        c.put32(data+0xb024,clubs+test.manager_club_id*0x248);
        c.put32(history, test.entry == 0x10420b50 ? link : player);
        c.put32(history+4,test.entry == 0x10420b50 ? std::uint32_t(test.action) : clubs+2*0x248);
        c.put32(history+8,test.reason); c.put32(history+12,7); c.put32(history+16,test.feedback_kind);
        c.set({{3,player},{4,clubs+2*0x248},{5,std::uint32_t(test.reason)},{6,std::uint32_t(test.feedback_kind)}});
        for (int i=0;i<test.pending_offers;++i) {
            const auto bid=data+0x64000+i*0x100,node=data+0x66000+i*16;
            c.put32(data+0x65000+i*4,bid); c.put32(bid+8,i+1);
            static constexpr int statuses[]{10,11,12,13,14,19,20};
            c.put8(bid+0x34,test.feedback_pattern ? statuses[i%7] : test.status);
            c.put32(node,data+0x67000+i*4); c.put16(node+4,4);
            c.put32(node+8,i+1<test.pending_offers ? node+16 : 0);
            c.put32(node+12,i ? node-16 : 0); c.put32(data+0x67000+i*4,20+i);
        }
    }
    if (test.entry == 0x1041eb10 || test.entry == 0x1041f3a0 || test.entry == 0x10422600 || test.entry == 0x1041f8f0 || test.entry == 0x104204f0) {
        c.put32(c.r(2)-0x7bb8,data+0x50000);
        unsigned n=0;
        for (int slot:{-0x7bc4,-0x7bc8,-0x7b14,-0x7bd0}) {
            const auto colour=data+0x400+4*n++;
            c.put32(c.r(2)+std::uint32_t(slot),colour); c.put16(colour,0x3456+n);
        }
        c.put32(c.r(2)-0x7ba8,data+0x54000); c.put32(c.r(2)-0x7bac,data+0x198);
        c.put32(data+0x194,test.first_control);
        for (unsigned i=0;i<8;++i) { c.put32(clubs+i*0x248,i); c.put8(clubs+i*0x248+0x23b,(i+1)%3); }
        c.put32(data+0xb024,test.manager_club_present ? clubs+test.manager_club_id*0x248 : 0);
        if (!test.manager_present) c.put32(screens+std::uint32_t(test.group)*0x2c0+4,0);
        c.put32(offer+8,test.bidder_id); c.put8(offer+0x24,test.temporary_offer);
        c.put8(offer+0x38,test.offer_kind); c.put32(offer+0x20,test.fee);
        c.s.memory.copy_in(data+0x7000,c.s.memory.copy_out(offer,0x54));
        c.put32(data+0x7020,std::uint32_t(test.fee)+test.reference_adjustment);
        c.s.memory.copy_in(data+0x7100,c.s.memory.copy_out(data+0x7000,0x54));
        c.put32(data+0x7120,std::uint32_t(test.fee)+200);
        c.put32(data+0x6f00,data+0x7000);
        c.put32(history,test.draft_link_mode==0 ? 0 : test.draft_link_mode==2 ? link : data+0x6f00);
        c.put32(history+4,test.reason); c.put32(history+8,test.original_link_present ? link : 0);
        if (test.entry == 0x10422600) { c.put32(history,offer); c.put32(history+4,test.news_revision); }
        if (test.entry == 0x1041f3a0) c.set(3,test.control);
    }
    if (test.entry==0x1041f8f0 || test.entry==0x104204f0) {
        c.put32(history+4,link); c.put32(history+8,test.reason);
        c.put32(people+3*0x70+8,test.valuation);
        if (test.entry==0x104204f0) c.set(3,test.control);
        if (std::int16_t(test.control)>=0)
            c.put32(data+0x100000+0xba8ac+std::uint32_t(std::int16_t(test.control))*0x17c,test.selected_value);
        for (int i=0;i<test.pending_offers;++i) {
            const auto bid=data+0x64000+i*0x100,node=data+0x66000+i*16;
            c.put32(data+0x65000+i*4,bid); c.put32(bid+12,3); c.put32(bid+0x28,20+i);
            c.put8(bid+0x34,test.pending_pattern==0 ? 0 : test.pending_pattern==1 ? (i%2 ? 0 : 1) : test.status);
            c.put32(node,data+0x67000+i*4); c.put16(node+4,4);
            c.put32(node+8,i+1<test.pending_offers ? node+16 : 0); c.put32(node+12,i ? node-16 : 0);
            c.put32(data+0x67000+i*4,20+i);
        }
    }
    if (test.entry==0x10422b50 || test.entry==0x10422e40 || test.entry==0x10421fb0 ||
        test.entry==0x10422310 || test.entry==0x1042c6d0 || test.entry==0x10421ee0 || test.entry==0x104265c0) {
        c.put_f64(c.r(2)-0x3aec,1.0); c.put_f64(c.r(2)-0x3afc,1.0);
        c.put32(c.r(2)-0x7bb8,data+0x50000);
        unsigned n=0;
        for (int slot:{-0x7bc4,-0x7bc8,-0x7b14}) {
            const auto colour=data+0x400+4*n++;
            c.put32(c.r(2)+std::uint32_t(slot),colour); c.put16(colour,0x3456+n);
        }
        c.put32(c.r(2)-0x7ba8,data+0x54000); c.put32(c.r(2)-0x7bac,data+0x198);
        c.put32(data+0x194,test.first_control);
        for (unsigned i=0;i<8;++i) { c.put32(clubs+i*0x248,i); c.put8(clubs+i*0x248+0x23b,(i+1)%3); }
        c.put32(data+0xb024,clubs+test.manager_club_id*0x248);
        c.put32(offer+4,data+0x60000); c.put32(offer+0x20,test.fee);
        c.put32(history,link); c.put32(history+4,test.selected_player_id); c.put32(history+8,link);
        c.put32(people+test.selected_player_id*0x70+8,test.selected_value);
        c.put16(data+0x60000,test.exchange_count);
        c.put32(data+0x60004,test.exchange_count ? data+0x60100 : 0);
        c.put32(data+0x60008,test.exchange_count ? data+0x60100+(test.exchange_count-1)*16 : 0);
        c.put32(data+0x6000c,data+0x60fff);
        for (int i=0;i<test.exchange_count;++i) {
            const auto node=data+0x60100+i*16;
            c.put32(node,data+0x61000+i*4); c.put16(node+4,4);
            c.put32(node+8,i+1<test.exchange_count ? node+16 : 0); c.put32(node+12,i ? node-16 : 0);
            c.put32(data+0x61000+i*4,4+i);
        }
        c.put32(contract+12,test.current_salary); c.put32(data+0x450c,test.offered_salary);
        c.put16(data+0x4532,test.expiry_year); c.put8(data+0x451c,test.cup_option); c.put8(data+0x451d,test.parent_option);
        if (test.entry==0x10422e40 || test.entry==0x10422310) c.set(3,test.control);
        if (test.entry==0x10422310) { c.put32(history,data+0xb000); c.put32(history+4,23); }
        if (test.entry==0x1042c6d0) c.set(3,offer);
        if (test.entry==0x10421ee0) c.set({{3,data+0xb000},{4,23}});
    }
    if (test.entry==0x104286c0 || test.entry==0x10428750 || test.entry==0x1042a810 || test.entry==0x1042abb0) {
        c.put32(c.r(2)-0x7bb8,data+0x50000);
        unsigned n=0;
        for (int slot:{-0x7bc4,-0x7bc8,-0x7b14}) {
            const auto colour=data+0x400+4*n++;
            c.put32(c.r(2)+std::uint32_t(slot),colour); c.put16(colour,0x3456+n);
        }
        c.put32(data+0x194,test.first_control);
        c.put32(c.r(2)-0x7ab8,data+0x200); c.put32(data+0x200,data+0xa300);
        c.put8(data+0xa565,test.club_kind);
        c.put32(data+0xb024,test.manager_club_present ? clubs+2*0x248 : 0);
        c.put32(data+0xa988,data+0x69000);
        if (test.actual_header) {
            c.put16(data+0x100000+0x129da6,1199);
            c.put8(data+0x100000+0x12a941,0);
        }
        const auto person=people+3*0x70;
        c.put32(person,3); c.put32(person+8,test.valuation);
        c.put32(person+0x14,test.registration ? data+0x6b000 : 0);
        c.put32(person+0x24,clubs+2*0x248); c.put8(person+0x65,test.player_age);
        c.put32(contract+0x50,test.status_listed_value); c.put8(contract+0x5c,test.status_mask);
        c.put8(contract+0x60,test.squad_status); c.put8(contract+0x4c,test.happiness);
        c.put32(history,person); c.put32(history+4,test.loan_mode);
        if (test.entry==0x10428750) c.set({{3,person},{4,data+0xb200},{5,23},{6,test.mode}});
        if (test.entry==0x104286c0) c.set({{3,data+0xb200},{4,23}});
        if (test.entry==0x1042abb0) c.set(3,test.control);
    }
    if (test.entry==0x1042f670 || test.entry==0x1042f7c0 || test.entry==0x1042fb30) {
        c.put32(c.r(2)-0x7bb8,data+0x50000);
        unsigned n=0;
        for (int slot:{-0x7bc4,-0x7bc8,-0x7b14}) {
            const auto colour=data+0x400+4*n++;
            c.put32(c.r(2)+std::uint32_t(slot),colour); c.put16(colour,0x3456+n);
        }
        c.put32(c.r(2)-0x7ba8,data+0x54000); c.put32(c.r(2)-0x7bac,data+0x198);
        c.put32(data+0x194,test.first_control); c.put32(data+0x204,0); c.put32(data+0x1fc,0);
        for (unsigned i=0;i<8;++i) { c.put32(clubs+i*0x248,i); c.put8(clubs+i*0x248+0x23b,(i+1)%3); }
        c.put32(data+0xb024,clubs+2*0x248);
        if (!test.manager_present) c.put32(screens+std::uint32_t(test.group)*0x2c0+4,0);
        const auto person=people+3*0x70;
        c.put32(person+0x24,clubs+unsigned(test.same_owner ? 2 : 1)*0x248);
        c.put32(history,person); c.put32(history+4,c.u32(person+0x24));
        c.put8(screens+0x2c5f,test.popup_active);
        c.put32(screens+0x2581a0,test.popup_action); c.put32(screens+0x2581a4,123);
        c.set(3,test.entry==0x1042fb30 ? std::uint32_t(test.control) : person);
    }
    if (test.entry==0x104249e0 || test.entry==0x10424d40 || test.entry==0x10424f80) {
        c.put32(c.r(2)-0x7bb8,data+0x50000);
        unsigned n=0;
        for (int slot:{-0x7bc4,-0x7bc8,-0x7b14}) {
            const auto colour=data+0x400+4*n++;
            c.put32(c.r(2)+std::uint32_t(slot),colour); c.put16(colour,0x3456+n);
        }
        c.put32(data+0x194,test.first_control); c.put32(data+0x208,0);
        c.put32(c.r(2)-0x7b1c,data+0x20c); c.put32(data+0x20c,data+0x6c000);
        c.put32(data+0xb024,test.manager_club_present ? clubs+2*0x248 : 0);
        c.put32(data+0xb020,test.alternate_club_present ? clubs+3*0x248 : 0);
        c.put32(history,3); c.put32(history+4,test.mode);
        if (test.entry==0x10424d40) c.set(3,test.control);
    }
    if (test.entry==0x1042efa0 || test.entry==0x1042f380) {
        c.put32(c.r(2)-0x7bb8,data+0x50000);
        unsigned n=0;
        for (int slot:{-0x7bc4,-0x7bc8,-0x7b14}) {
            const auto colour=data+0x400+4*n++;
            c.put32(c.r(2)+std::uint32_t(slot),colour); c.put16(colour,0x3456+n);
        }
        c.put32(c.r(2)-0x7ba8,data+0x54000); c.put32(c.r(2)-0x7bac,data+0x198);
        c.put32(data+0x194,test.first_control); c.put32(data+0x218,0); c.put32(data+0x220,0); c.put32(data+0x224,0);
        for (unsigned i=0;i<8;++i) { c.put32(clubs+i*0x248,i); c.put8(clubs+i*0x248+0x23b,(i+1)%3); }
        c.put32(data+0xb024,clubs+2*0x248);
        if (!test.manager_present) c.put32(screens+std::uint32_t(test.group)*0x2c0+4,0);
        const auto person=people+3*0x70;
        c.put32(history,person); c.put32(history+4,test.mode); c.put32(history+8,test.term_byte_a);
        c.put32(history+12,test.term_word_a); c.put32(history+16,test.term_byte_b);
        c.put32(history+20,test.cup_option); c.put32(history+24,test.parent_option);
        c.put32(history+28,test.offer_present ? offer : 0);
        if (test.entry==0x1042f380) c.set(3,test.control);
    }
    if (test.entry==0x1042ae90) {
        c.put_f64(c.r(2)-0x3aec,1.0); c.put_f64(c.r(2)-0x3afc,1.0);
        c.put32(c.r(2)-0x7bb8,data+0x50000);
        unsigned n=0;
        for (int slot:{-0x7bc4,-0x7bc8,-0x7b14}) {
            const auto colour=data+0x400+4*n++;
            c.put32(c.r(2)+std::uint32_t(slot),colour); c.put16(colour,0x3456+n);
        }
        c.put32(data+0x194,test.first_control);
        for (unsigned i=0;i<12;++i) { c.put32(clubs+i*0x248,i); c.put8(clubs+i*0x248+0x23b,(i+1)%3); }
        c.put32(history,people+3*0x70); c.put32(history+4,0x12345678);
        c.put32(contract+12,test.current_salary);
    }
    if (test.entry==0x1042bf70 || test.entry==0x1042c2c0) {
        c.put32(c.r(2)-0x7bb8,data+0x50000);
        c.put32(c.r(2)-0x7ba8,data+0x54000); c.put32(c.r(2)-0x7bac,data+0x198);
        variable(-0x7ab8,0x148,data+0xa400);
        for (unsigned i=0;i<8;++i) c.put32(clubs+i*0x248,i);
        c.put32(data+0xb024,clubs+2*0x248);
        c.put32(contract+12,test.current_salary); c.put32(data+0x450c,test.offered_salary);
        c.put8(data+0x4538,test.proposed_type);
        c.put16(data+0x4532,test.expiry_year); c.put8(data+0x451c,test.cup_option); c.put8(data+0x451d,test.parent_option);
        for (unsigned i=0;i<32;++i) c.put32(history+4*i,0xabc00000+i);
        if (test.entry==0x1042bf70) c.set({{3,data+0xb000},{4,23}});
        else c.set(3,people+3*0x70);
    }
    if (test.entry==0x1042b580 || test.entry==0x1042bae0) {
        c.put32(c.r(2)-0x7bb8,data+0x50000);
        unsigned n=0;
        for (int slot:{-0x7bc4,-0x7bc8,-0x7bdc,-0x7bcc}) {
            const auto colour=data+0x400+4*n++;
            c.put32(c.r(2)+std::uint32_t(slot),colour); c.put16(colour,0x3456+n);
        }
        c.put32(c.r(2)-0x7ba8,data+0x54000); c.put32(c.r(2)-0x7bac,data+0x198);
        c.put32(data+0x194,test.first_control);
        c.put32(data+0xb024,test.manager_club_present ? clubs+2*0x248 : 0);
        if (!test.manager_present) c.put32(screens+std::uint32_t(test.group)*0x2c0+4,0);
        c.put32(history,test.person_present ? people+3*0x70 : 0);
        c.put32(history+4,test.mode); c.put32(history+8,test.first_control);
        if (test.entry==0x1042bae0) c.set(3,test.control);
    }
    if (test.entry==0x10425980 || test.entry==0x10425240) {
        constexpr auto words=data+0x6e000;
        variable(-0x7b90,0x230,std::uint32_t(test.random_salt));
        variable(-0x7b84,0x234,words);
        variable(-0x7b88,0x238,words+(test.random_wrap==1 ? 0 : test.random_wrap==2 ? 2 : 126));
        c.put32(c.r(2)-0x7b8c,words); c.put32(c.r(2)-0x34c4,0x12345678);
        for (unsigned i=0;i<128;i+=2) c.put16(words+i,test.random_word);
        c.set(3,test.mode);
    }
    if (test.entry==0x10425240) {
        variable(-0x7b1c,0x20c,data+0x6c000); variable(-0x7ab8,0x148,data+0xa400);
        variable(-0x7b94,0x248,data+0x6c100);
        c.put32(data+0x23c,0); c.put32(data+0x240,0xffffffff); c.put32(data+0x244,0);
        c.put32(data+0xb024,test.manager_club_present ? clubs+2*0x248 : 0);
        c.put32(data+0xb020,test.alternate_club_present ? clubs+3*0x248 : 0);
        c.put32(data+0xa000+0x988,data+0x6c200);
        c.put32(data+0xa400,test.bank_high); c.put32(data+0xa404,test.bank_low);
        const auto person=people+3*0x70;
        c.put32(person+0x14,data+0x6c300); c.put16(data+0x6c30a,test.shortlist_rating);
        c.put32(person+0x24,test.same_owner ? clubs+0x248 : 0);
        c.put32(history,3); c.put32(history+4,test.mode); c.set(3,test.control);
    }
    if(test.entry==0x10423270 && test.completion_constant) {
        // Noncanonical constants characterize the unordered BLE behavior.
        c.put_f64(c.toc(-0x4ad4)+0x10,test.completion_constant==1 ?
            std::numeric_limits<double>::infinity() : std::numeric_limits<double>::quiet_NaN());
    }
    if (test.actual_reset_services) actual_heap_offers(c, test);
    c.s.budget = 100000;
}
void returned(State& s) {
    Cpu c(s);
    if (c.sp() != stack || c.lr() != sentinel || s.pc != sentinel)
        throw std::runtime_error("fixture did not return");
    if (active.actual_header && c.u8(data+0x100000+0x12a941)!=1)
        throw std::runtime_error("Actual full-capacity header guard was not executed");
    if (active.entry==0x10425240) {
        auto action=std::int16_t(active.control)==-1 ? 0 : std::int16_t(active.action);
        if (!action) action=std::int16_t(active.default_action);
        const auto expected=action==1 ? active.mode ? 0x1030fda0u : 0x10310310u : 0u;
        if (c.u32(data+0x23c)!=expected) throw std::runtime_error("Shortlist add/remove selection mismatch");
    }
    if (active.entry==0x10425980) {
        const auto bound=std::int32_t(active.mode);
        const bool wide=bound>65535 || bound< -65535;
        const auto correction=std::int32_t(active.mode*10u)/65535;
        const unsigned draws=bound==0 ? 0 : wide && correction!=0 ? 2 : 1;
        unsigned cursor_offset=0,lcg=0x12345678,salt=std::uint32_t(active.random_salt);
        const unsigned end=active.random_wrap==1 ? 0 : active.random_wrap==2 ? 2 : 126;
        for (unsigned i=0;i<draws;++i) {
            cursor_offset+=2;
            if (cursor_offset>end) { cursor_offset=0; lcg=lcg*0x41c64e6du+0x3039u; salt=(lcg>>16)&0x7fff; }
        }
        if (c.u32(data+0x234)!=data+0x6e000+cursor_offset || c.u32(data+0x230)!=salt || c.u32(c.r(2)-0x34c4)!=lcg)
            throw std::runtime_error("Transfer range RNG consumption/state mismatch");
    }
    if (active.entry==0x10423270) {
        if(!active.draft_present) {
            const auto clone=c.u32(data+0x6f00), source=active.actual_reset_services ? c.u32(data+0x190) : offer;
            // Independent byte mask from the original record declaration.
            std::array<bool,0x54> assigned{};
            for(unsigned i=0;i<16;++i)assigned[i]=true;
            for(auto offset:{0x10u,0x11u,0x18u,0x1cu,0x24u,0x34u,0x35u,0x36u,0x37u,0x38u,0x44u,0x4cu,0x50u,0x51u})assigned[offset]=true;
            for(auto offset:{0x14u,0x16u})for(unsigned i=0;i<2;++i)assigned[offset+i]=true;
            for(auto offset:{0x20u,0x28u,0x2cu,0x30u,0x3cu,0x40u,0x48u})for(unsigned i=0;i<4;++i)assigned[offset+i]=true;
            if(!clone || clone==source || c.u32(clone)!=0)throw std::runtime_error("Completion clone ownership mismatch");
            for(unsigned i=8;i<0x54;++i) {
                if(active.completion_mutation==1 && i>=0x20 && i<0x24)continue;
                const auto expected=assigned[i] ? c.u8(source+i) : 0xd3;
                if(c.u8(clone+i)!=expected)throw std::runtime_error("Completion clone fields/gaps mismatch");
            }
        }

        if(c.u32(history+12)!=std::uint32_t(std::int32_t(std::int16_t(active.reason))) ||
            c.u32(history+24)!=std::uint32_t(std::int32_t(std::int8_t(active.completion_status))))
            throw std::runtime_error("Completion history reason/status widths mismatch");
    }
    if (active.entry==0x104204f0) {
        auto action=std::int16_t(active.control)==-1 ? 0 : std::int16_t(active.action);
        if(!action)action=std::int16_t(active.default_action);
        if(action==1 && c.u32(history+8)!=(std::int16_t(active.control)==-1 ? 0u : std::uint32_t(active.selected_value)))
            throw std::runtime_error("Rejection reason lost full payload width");
    }
    if (active.entry==0x1042bae0) {
        auto action=std::int16_t(active.control)==-1 ? 0 : std::int16_t(active.action);
        if (!action) action=std::int16_t(active.default_action);
        if (action==1 || action==2) {
            const auto expected=std::uint32_t(std::int32_t(std::int8_t(std::int8_t(active.mode)+(action==1 ? -1 : 1))));
            if (c.u32(history+4)!=expected || c.r(3)!=0) throw std::runtime_error("Trial duration signed-byte update mismatch");
        }
    }
    if (active.entry==0x1042c2c0) {
        const bool opened=std::int8_t(active.eligible) && std::int8_t(active.create_result);
        if (c.r(3)!=unsigned(opened)) throw std::runtime_error("Loan terms opening result mismatch");
        if (opened) {
            const auto wage=std::uint32_t(active.current_salary)+(active.history_terms_mutation ? 7u : 0u);
            const bool proposed=active.proposed_present && (std::int8_t(active.proposed_type)>>2)==6;
            const auto product=std::int32_t(std::uint32_t(active.offered_salary)*100u);
            const auto quotient=proposed ? std::int8_t(std::int64_t(product)/std::int32_t(wage)) : 0;
            const auto percentage=std::uint32_t(std::int32_t(std::int8_t((int(quotient)/10)*10)));
            if (c.u32(history+44)!=percentage) throw std::runtime_error("Loan percentage byte-before-tens mismatch");
        }
    }
    if (active.entry==0x1042ae90) {
        std::int32_t expected=0;
        const int kind=std::int8_t(active.contract_type_query==3 ? 20 : active.contract_type)>>2;
        if (active.contract_present && active.contract_null_query!=3 && kind==2) {
            const int days=std::int16_t(active.release_days);
            if (days>0) {
                const auto salary=std::uint32_t(active.current_salary)+(active.release_salary_mutation ? 17u : 0u);
                expected=std::int32_t(std::uint32_t(days)*salary)/7;
            }
            if (std::int8_t(active.listing_eligible)) expected/=4;
        }
        if (c.u32(history+4)!=std::uint32_t(expected)) throw std::runtime_error("Release compensation mismatch");
    }
    if (active.entry==0x1042f380) {
        auto action=std::int16_t(active.control)==-1 ? 0 : std::int16_t(active.action);
        if (!action) action=std::int16_t(active.default_action);
        const bool loan=std::int8_t(active.mode)==-1;
        const auto expected=action!=2 ? 0u : !loan ? 0x1007f200u : active.offer_present ? 0x103e3240u : 0x103e3170u;
        if (c.u32(data+0x224)!=expected) throw std::runtime_error("Temporary offer service selection mismatch");
        if (action==2 && loan) {
            const int day=std::int16_t(active.current_day);
            const auto calls=day>28 ? 1u : 2u;
            const int selected=day>28 ? 28 : active.day_mutation ? std::int16_t(active.second_day) : day;
            if (c.u32(data+0x218)!=calls || c.u16(data+0x6d000)!=std::uint16_t(selected) ||
                c.u16(data+0x6d002)!=std::uint16_t(active.term_word_a) || c.u8(data+0x6d004)!=std::uint8_t(active.term_byte_a))
                throw std::runtime_error("Temporary loan date query/width mismatch");
        }
    }
    if (active.entry==0x10424d40) {
        auto action=std::int16_t(active.control)==-1 ? 0 : std::int16_t(active.action);
        if (!action) action=std::int16_t(active.default_action);
        const unsigned expected=action==1 ? 1 : action==10 ? 2 : 0;
        if (c.u32(data+0x208)!=expected) throw std::runtime_error("Comparison selection side mismatch");
    }
    if (active.entry==0x1042abb0) {
        auto action=std::int16_t(active.control)==-1 ? 0 : std::int16_t(active.action);
        if (!action) action=std::int16_t(active.default_action);
        const bool changed=action==2 && !std::int8_t(active.loan_mode) &&
            std::int8_t(active.transfer_list_result) && !(std::uint32_t(active.status_listed_value)&0x10000) &&
            std::int8_t(active.listing_eligible) && std::int8_t(active.player_age)<15;
        const auto expected=changed ? std::max(-100,int(std::int8_t(active.happiness))-50) : int(std::int8_t(active.happiness));
        if (c.i8(contract+0x4c)!=expected || (c.u32(data+0x1f4)!=0)!=changed)
            throw std::runtime_error("Listing happiness/notification mismatch");
    }
    if (active.entry==0x10422e40) {
        auto action=std::int16_t(active.control)==-1 ? 0 : std::int16_t(active.action);
        if (!action) action=std::int16_t(active.default_action);
        const auto expected=std::uint32_t(active.fee)+(action==2 ? std::uint32_t(active.selected_value) : 0u);
        if (c.u32(offer+0x20)!=expected) throw std::runtime_error("Exchange removal fee ordering/value mismatch");
        if (action==2) {
            const bool found=active.selected_player_id>=4 && active.selected_player_id<4+active.exchange_count;
            if (bool(c.u32(data+0x1e8))!=found) throw std::runtime_error("Exchange removal match service mismatch");
        }
    }
    if (active.entry == 0x10420f00) {
        unsigned expected=7;
        if (std::int8_t(active.create_result) && (std::int16_t(active.reason)==18 || std::int16_t(active.reason)==32)) {
            static constexpr int statuses[]{10,11,12,13,14,19,20};
            for (int i=0;i<active.pending_offers;++i) {
                const auto status=active.feedback_pattern ? statuses[i%7] : std::uint8_t(active.status);
                if ((status>=11 && status<=14) || status==19) expected=i+1;
            }
        }
        if (c.u32(history+12)!=expected) throw std::runtime_error("Feedback did not preserve the last matching bidder");
    }
    if (active.actual_reset_services && (active.action == 26 || active.action == 32 ||
        ((active.entry == 0x10417b60 || active.entry==0x10423270) && !active.draft_present && active.link_present))) {
        const auto restored = c.u32(data + 0x6f00), source = c.u32(data + 0x190);
        if (!restored || restored == source || c.u32(restored) != 0)
            throw std::runtime_error("Reset did not produce an independent detached draft");
        const auto copied_list = c.u32(restored + 4), source_list = c.u32(source + 4);
        if (bool(copied_list) != bool(source_list)) throw std::runtime_error("Reset lost exchange-list presence");
        if (source_list) {
            if (copied_list == source_list || c.i16(copied_list) != active.exchange_count ||
                c.u32(copied_list + 12) || c.u32(source_list + 12))
                throw std::runtime_error("Reset list ownership/count/cursor mismatch");
            auto copied_node = c.u32(copied_list + 4), source_node = c.u32(source_list + 4);
            for (int i = 0; i < active.exchange_count; ++i) {
                if (!copied_node || !source_node || copied_node == source_node ||
                    c.u32(copied_node) == c.u32(source_node) || c.u32(c.u32(copied_node)) != c.u32(c.u32(source_node)))
                    throw std::runtime_error("Reset exchange payload was not deeply copied");
                copied_node = c.u32(copied_node + 8); source_node = c.u32(source_node + 8);
            }
            if (copied_node || source_node) throw std::runtime_error("Reset list length mismatch");
        }
    }
}
void original(State& s, std::uint32_t entry) {
    original_mode = true; s.pc = entry;
    try { run_until(s, sentinel); returned(s); }
    catch (const std::exception& error) {
        Cpu c(s);
        throw std::runtime_error(std::string(error.what()) + " at pc " + hex_address(s.pc) +
            " lr " + hex_address(c.lr()) + " r3 " + hex_address(c.r(3)) + " r4 " + hex_address(c.r(4)));
    }
}
void rewrite(State& s, std::uint32_t entry) {
    original_mode = false;
    if (production_mode) { s.pc = entry; run_until(s, sentinel); }
    else transfer_screens_run_rewrite(s, entry);
    returned(s);
#if defined(CM_TRANSFER_SCREENS_COVERAGE)
    // Each comparison exits its child through _exit, bypassing the runtime's
    // exit flush. Use an LLVM_PROFILE_FILE %m merge pool to bound disk use.
    __llvm_profile_write_file();
#endif
}
void corrupted(State& s, std::uint32_t entry) {
    rewrite(s, entry); Cpu c(s); c.put32(history, c.u32(history) ^ 1);
}
void corrupted_result(State& s, std::uint32_t entry) {
    rewrite(s, entry); Cpu c(s); c.set(3, c.r(3) ^ 1);
}
struct ObservedWrites final : MemoryObserver {
    unsigned count = 0;
    void mapped(std::uint32_t, std::uint64_t) override {}
    void written(std::uint32_t, const std::uint8_t*, std::size_t) override { ++count; }
    void cleared(std::uint32_t, std::uint64_t) override { ++count; }
};
}

int main(int argc, char** argv) {
    if (argc != 2) { std::cerr << "Usage: cm-transfer-screens-checks <original-segments>\n"; return 2; }
    try {
        auto base = std::make_unique<State>(); load_program(*base, argv[1]); dispatch_probe = probe;
        std::vector<Case> cases;
        for (auto entry : entries) cases.push_back({entry});
        for (auto entry : {entries[0], entries[1]}) {
            for (int created : {0, 1, -1, 128, 256, 0x12340001}) for (int eligible : {0, 1, -1, 128, 256})
                for (int status : {0, 4, 5, 16, 17, 127, 128, 255}) {
                    Case t{entry}; t.create_result = created; t.eligible = eligible; t.status = status; cases.push_back(t);
                }
            for (int present : {0, 1}) for (int type : {0, 16, 20, 21, 23, 24, 127, 128, 255}) {
                Case t{entry}; t.contract_present = present; t.contract_type = type; cases.push_back(t);
            }
            for (int group : {0, 1, 3}) { Case t{entry}; t.group = group; cases.push_back(t); }
        }
        for (int present : {0, 1}) for (int offer_present : {0, 1}) {
            Case t{entries[0]}; t.link_present = present; t.offer_present = offer_present; cases.push_back(t);
        }
        for (int create : {0, 1, -1, 128, 256}) for (int response : {0, 1, 2, 128, 255})
            for (int controlled : {0, 1, -1, 127, 128, 255, 256}) {
                Case t{entries[1]}; t.create = create; t.response = response; t.controlled = controlled; cases.push_back(t);
            }
        for (auto entry : {entries[3], entries[4]}) for (int created : {0, 1, -1, 256}) {
            Case t{entry}; t.create_result = created; cases.push_back(t);
        }
        for (auto entry : {0x10421a20u, 0x10421af0u, 0x10421be0u, 0x10421ce0u, 0x10421dd0u, 0x10423040u})
            for (int revision : {16, 17, 18}) for (int status : {0, 2, 3, 5, 16, 255})
                for (int created : {0, 1}) {
                    Case t{entry}; t.news_revision = revision; t.status = status;
                    t.create_result = created; cases.push_back(t);
                }
        for (unsigned index = 10; index < 22; ++index) for (int created : {0, 1, 128, 256})
            for (auto mode : {0u, 1u, 2u, 127u, 128u, 255u, 256u, 65535u, 0xdeadbeefu, 0x80000000u}) {
                Case t{entries[index]}; t.create_result = created; t.mode = mode; cases.push_back(t);
            }
        for (int present : {0, 1}) { Case t{0x10428650}; t.candidates = present; cases.push_back(t); }
        for (auto entry : {0x10420e40u, 0x10421960u, 0x10422a00u, 0x10426830u, 0x1042b3a0u})
            for (int control : {-1, 0, 1, 5, 65535}) for (int action : {0, 1, 2, 3, 255, 65537})
                for (int fallback_action : {0, 1, 3}) for (int command : {0, 1, -1}) {
                    Case t{entry}; t.control = control; t.action = action; t.default_action = fallback_action;
                    t.command = command; t.fallback = command ? 7 : -9; cases.push_back(t);
                }
        for (int created : {0, 1, 128, 256}) for (auto month : {0u, 1u, 12u, 128u, 255u, 65535u}) {
            Case t{0x10426970}; t.create_result = created; t.mode = month; cases.push_back(t);
        }
        { Case t{0x10417880}; t.clear_link = 1; cases.push_back(t); }
        { Case t{0x1042ee90}; t.mixed_terms = 1; cases.push_back(t); }
        for (auto entry : {0x10420e40u, 0x10421960u, 0x10422a00u, 0x10426830u, 0x1042b3a0u})
            for (int fallback : {0, 1, -1, std::numeric_limits<int>::min(), std::numeric_limits<int>::max()}) {
                Case t{entry}; t.fallback = fallback; cases.push_back(t);
            }
        for (auto entry : {0x10421a20u, 0x10421af0u, 0x10421be0u, 0x10421ce0u, 0x10421dd0u, 0x10423040u})
            for (int revision : {16, 17, 18}) for (int status : {2, 3, 5}) {
                Case t{entry}; t.actual_query = 1; t.news_revision = revision; t.status = status; cases.push_back(t);
            }
        for (int fee : {0, 1, -1, 0x7fffffff, std::numeric_limits<int>::min()})
            for (int league : {-1, 0, 0x7fffffff}) for (int international : {-1, 0, 0x7fffffff})
                for (int high : {0, 7, -1}) {
                    Case t{0x1041e770}; t.fee = fee; t.league_fee = league;
                    t.international_fee = international; t.monetary_high = high; cases.push_back(t);
                }
        for (int equal : {0, 1, 128, 256}) for (int flags : {0, 1, 15, 16, 240, 255})
            for (int control : {-1, 0, 32767, 65535}) for (int link_present : {0, 1}) {
                Case t{0x1041e770}; t.proposals_equal = equal; t.negotiation_flags = flags;
                t.control = control; t.link_present = link_present; cases.push_back(t);
            }
        for (int count : {0, 1, 2, 4}) for (int services : {0, 1, 2, 4, 7})
            for (int equal : {0, 1}) {
                Case t{0x1041e770}; t.exchange_list = 1; t.exchange_count = count;
                t.actual_total_services = services; t.proposals_equal = equal; cases.push_back(t);
            }
        {
            Case t{0x1041e770}; t.actual_total_services = 7; t.total_example = 1;
            t.fee = t.league_fee = t.international_fee = 2000000000; cases.push_back(t);
            t = Case{0x1041e770}; t.actual_total_services = 7; t.total_example = 2; t.fee = 10000;
            t.exchange_list = 1; t.exchange_count = 2; t.distinct_lists = 1; cases.push_back(t);
        }
        for (int delta : {0, 1, 1000}) for (int terms : {0, 1, 2, 3, 4}) {
            Case t{0x1041e770}; t.actual_total_services = 7; t.fee = 10000;
            t.reference_adjustment = delta; t.term_difference = terms; cases.push_back(t);
        }
        for (int count : {0, 1, 2, 4}) for (int distinct : {1, 2}) {
            Case t{0x1041e770}; t.actual_total_services = 7; t.fee = 10000;
            t.exchange_list = 1; t.exchange_count = count; t.distinct_lists = distinct; cases.push_back(t);
        }
        for (int action = 0; action <= 40; ++action) {
            Case t{0x1041b600}; t.action = action; t.status = 0; t.exchange_list = 1; t.exchange_count = 2;
            cases.push_back(t);
        }
        for (int action : {3, 4, 15, 16, 20, 21})
            for (int fee : {0, 1000, 1001, 99999000, 100000000, std::numeric_limits<int>::max()})
                for (int fresh : {0, 1}) for (int limit : {0, 100000000}) {
                    Case t{0x1041b600}; t.action = action; t.create = fresh; t.board_limit = limit;
                    t.fee = t.league_fee = t.international_fee = fee; cases.push_back(t);
                }
        for (int action : {7, 8, 9, 10, 11, 12}) for (int value : {0, 1, 5, 6, 12, 18, 24, 25, 127, 128, 255}) {
            Case t{0x1041b600}; t.action = action; t.installments = t.sell_on = value; cases.push_back(t);
        }
        for (int action : {13, 18}) for (int value : {-1000, 0, 1000, std::numeric_limits<int>::min()})
            for (int valuation : {0, 3, 4000, 100000000, std::numeric_limits<int>::max()}) {
                Case t{0x1041b600}; t.action = action; t.league_fee = t.international_fee = value;
                t.valuation = valuation; cases.push_back(t);
            }
        for (int action : {14, 19}) for (int value : {0, 1, 10, 40, 50, 60, 127, 128, 255}) {
            Case t{0x1041b600}; t.action = action; t.appearances = value; cases.push_back(t);
        }
        for (int action : {35, 2, 1, 27, 28, 29, 26, 32}) for (int status : {0, 1, 2, 3, 5, 128, 255})
            for (int revision : {16, 17, 18}) for (int eligible : {0, 1, 128, 256}) {
                Case t{0x1041b600}; t.action = action; t.status = status; t.bid_revision = revision;
                t.eligible = eligible; cases.push_back(t);
            }
        for (int action : {0, 5, 6, 17, 22, 30, 31, 33, 34, 36, 37, 38, 39})
            for (int control : {-1, 0, 2}) {
                // Removing an exchange player requires a real selected payload;
                // the original dereferences null for this action with control -1.
                if (action == 31 && control == -1) continue;
                Case t{0x1041b600}; t.control = control; t.default_action = action;
                t.exchange_list = 1; t.exchange_count = 1; cases.push_back(t);
            }
        for (int action : {26, 32, 33}) for (int list : {0, 1}) for (int count : {0, 1, 3}) {
            Case t{0x1041b600}; t.action = action; t.status = 0; t.actual_reset_services = 1;
            t.exchange_list = list; t.exchange_count = count; cases.push_back(t);
        }
        for (int count : {1, 3}) {
            Case t{0x1041b600}; t.action = 31; t.actual_reset_services = 1;
            t.exchange_list = 1; t.exchange_count = count; cases.push_back(t);
        }
        for (int action : {28, 29, 26, 32}) {
            Case t{0x1041b600}; t.action = action; t.status = 5; t.destroy_revision_delta = 9; cases.push_back(t);
        }
        for (int present : {0, 1}) for (int type : {0, 20, 23, 24, 128, 255}) {
            Case t{0x1041b600}; t.contract_present = present; t.contract_type = type; cases.push_back(t);
        }
        for (int action : {1, 2}) {
            Case t{0x1041b600}; t.action = action; t.link_present = 0; cases.push_back(t);
        }
        for (int eligible : {0, 1, -1, 128, 256}) {
            Case t{0x1041b600}; t.action = 2; t.status = 0; t.create = 0; t.eligible = eligible; cases.push_back(t);
        }
        for (int action : {5, 37}) for (int bidder : {2, 3}) for (int limit : {0, 2000000}) {
            Case t{0x1041b600}; t.action = action; t.bidder_id = bidder; t.board_limit = limit;
            t.valuation = 1000000; cases.push_back(t);
        }
        for (int value : {1, 2, 128, 255}) {
            Case t{0x1041b600}; t.action = 34; t.public_bid = value; cases.push_back(t);
        }
        for (int kind : {0, 1, 0x20, 0x21, 0xa0, 0xff}) for (int eligible : {0, 1}) {
            Case t{0x1041b600}; t.action = 36; t.offer_kind = kind; t.eligible = eligible; cases.push_back(t);
        }
        for (int command : {0, 1, -1}) for (int fallback : {0, 1, -1, std::numeric_limits<int>::min()}) {
            Case t{0x1041b600}; t.command = command; t.fallback = fallback; cases.push_back(t);
        }
        for (int fresh : {0, 1}) for (int pending : {0, 1})
            for (int fee : {-1, 0, 1, 999, 1000, 1001, 100000000, std::numeric_limits<int>::max()}) {
                Case t{0x10417b60}; t.create = fresh; t.pending_offer = pending;
                t.fee = t.league_fee = t.international_fee = fee; cases.push_back(t);
            }
        for (int pending : {0, 1}) for (int value : {0, 1, 5, 6, 10, 24, 25, 127, 128, 255}) {
            Case t{0x10417b60}; t.pending_offer = pending; t.installments = t.sell_on = value; cases.push_back(t);
        }
        for (int fresh : {0, 1}) for (int count : {0, 1, 4, 5}) for (int remove : {0, 1}) {
            Case t{0x10417b60}; t.create = fresh; t.exchange_list = 1; t.exchange_count = count;
            t.removing_exchange = remove; cases.push_back(t);
        }
        for (int fresh : {0, 1}) for (int pending : {0, 1}) for (int count : {0, 2, 3, 255})
            for (int equal : {0, 1}) for (int flags : {0, 16}) {
                Case t{0x10417b60}; t.create = fresh; t.pending_offer = pending; t.delay_count = count;
                t.proposals_equal = equal; t.negotiation_flags = flags; cases.push_back(t);
            }
        for (int public_bid : {0, 1, 2, 128, 255}) for (int appearances : {1, 10, 128, 255}) {
            Case t{0x10417b60}; t.public_bid = public_bid; t.appearances = appearances;
            t.league_fee = t.international_fee = 2000; cases.push_back(t);
        }
        for (int decision : {-1, 0, 1, 2, 3, 4, 127, 128, 256}) for (int limit : {0, 10000}) {
            Case t{0x10417b60}; t.board_enabled = 1; t.board_decision = decision; t.board_limit = limit; cases.push_back(t);
        }
        for (int present : {0, 1}) for (int reason : {0, 1}) {
            Case t{0x10417b60}; t.board_enabled = 1; t.board_present = present; t.board_reason = reason; cases.push_back(t);
        }
        for (int link_present : {0, 1}) for (int draft : {0, 1}) for (int negotiated : {-1, 0}) {
            Case t{0x10417b60}; t.link_present = link_present; t.draft_present = draft;
            t.negotiated_id = negotiated; cases.push_back(t);
        }
        for (int type : {0, 20, 23, 24, 128}) {
            Case t{0x10417b60}; t.contract_type = type; cases.push_back(t);
        }
        { Case t{0x10417b60}; t.contract_present = 0; cases.push_back(t); }
        { Case t{0x10417b60}; t.person_present = 0; cases.push_back(t); }
        { Case t{0x10417b60}; t.manager_present = 0; cases.push_back(t); }
        { Case t{0x10417b60}; t.manager_club_present = 0; cases.push_back(t); }
        { Case t{0x10417b60}; t.status = 0; t.manager_club_id = 1; cases.push_back(t); }
        for (int count : {0, 1, 3}) for (int negotiated : {-1, 0}) {
            Case t{0x10417b60}; t.draft_present = 0; t.exchange_list = 1; t.exchange_count = count;
            t.negotiated_id = negotiated; t.actual_reset_services = 1; cases.push_back(t);
        }
        for (int services : {1, 2, 4, 7}) for (int list : {0, 1}) for (int fresh : {0, 1}) {
            Case t{0x10417b60}; t.actual_total_services = services; t.exchange_list = list;
            t.exchange_count = list ? 2 : 0; t.create = fresh; cases.push_back(t);
        }
        {
            Case t{0x10417b60}; t.actual_total_services = 7; t.total_example = 1;
            t.fee = t.league_fee = t.international_fee = 2000000000; cases.push_back(t);
            t = Case{0x10417b60}; t.actual_total_services = 7; t.total_example = 2; t.fee = 10000;
            t.exchange_list = 1; t.exchange_count = 2; cases.push_back(t);
        }
        for (int control : {-1, 0, 32760, 65530}) for (int height : {1, 21, 1000}) {
            Case t{0x10417b60}; t.first_control = control; t.row_height = height; cases.push_back(t);
        }
        for (int count : {0, 1, 2, 4}) for (int actual_iterator : {0, 2}) {
            Case t{0x10417b60}; t.status = 0; t.manager_club_id = 1;
            t.pending_offers = count; t.actual_total_services = actual_iterator; cases.push_back(t);
        }
        { Case t{0x10417b60}; t.status = 0; t.link_present = 0; cases.push_back(t); }
        { Case t{0x10417b60}; t.status = 0; t.manager_club_id = 2; cases.push_back(t); }
        { Case t{0x10417b60}; t.board_enabled = 1; t.board_context = 0; cases.push_back(t); }
        { Case t{0x10417b60}; t.negotiated_id = 0; t.proposals_equal = 0; cases.push_back(t); }
        // Deliberately noncanonical boundary mutations characterize the live
        // rereads; they do not assert that normal UI services alter contracts.
        { Case t{0x10417b60}; t.status = 0; t.manager_club_id = 1; t.contract_null_query = 2; cases.push_back(t); }
        { Case t{0x10417b60}; t.status = 0; t.manager_club_id = 1; t.contract_type_query = 2; cases.push_back(t); }
        { Case t{0x10417b60}; t.contract_null_query = 5; cases.push_back(t); }
        for (int action:{0,27,28,35,65563,-1}) for (int reason:{-1,0,27,65563,32768}) {
            Case t{0x10420b50}; t.action=action; t.reason=reason; cases.push_back(t);
        }
        for (int created:{0,1,-1,128,256,0x12340001}) for (int reason:{0,18,32,65554})
            for (int kind:{-1,0,2,128,255,256}) {
                Case t{0x10420f00}; t.create_result=created; t.reason=reason; t.feedback_kind=kind;
                t.pending_offers=7; t.feedback_pattern=1; cases.push_back(t);
            }
        for (int status:{0,10,11,12,13,14,15,18,19,20,127,128,255})
            for (int count:{0,1,3}) for (int iterator:{0,2}) {
                Case t{0x10420f00}; t.reason=18; t.status=status; t.pending_offers=count;
                t.actual_total_services=iterator; cases.push_back(t);
            }
        for (int reason:{0,1,7,18,25,26,32,33,39,40,41,-1,65543})
            for (int kind:{0,1,2,3,255,256}) {
                Case t{0x10421070}; t.reason=reason; t.feedback_kind=kind; cases.push_back(t);
            }
        for (int present:{0,1}) for (int type:{0,16,20,21,23,24,127,128,255}) {
            Case t{0x10421070}; t.reason=1; t.contract_present=present; t.contract_type=type; cases.push_back(t);
        }
        for (int present:{0,1}) for (int id:{-1,0,17}) for (int handle:{0,1}) for (int payload:{0,1}) {
            Case t{0x10421070}; t.reason=33; t.contract_present=present; t.feedback_offer_id=id;
            t.link_present=handle; t.offer_present=payload; cases.push_back(t);
        }
        for (auto entry:{0x10420b50u,0x10421070u}) for (int height:{1,21,1000}) for (int group:{0,1,3}) {
            Case t{entry}; t.row_height=height; t.group=group; cases.push_back(t);
        }
        for (int reason:{0,1,2,27,28,29,35,-1}) for (int temporary:{0,1,2,128,255})
            for (int kind:{0,16,32,48,128,255}) {
                Case t{0x1041eb10}; t.reason=reason; t.temporary_offer=temporary; t.offer_kind=kind; cases.push_back(t);
            }
        // Explicit noncanonical query results characterize the caller's order;
        // the actual lookup is pure and normally returns a stable owner.
        for (int reason:{1,35}) for (int kind:{0,16,32}) {
            Case t{0x1041eb10}; t.reason=reason; t.offer_kind=kind; t.varying_owner=1; cases.push_back(t);
        }
        { Case t{0x1041eb10}; t.draft_link_mode=0; cases.push_back(t); }
        { Case t{0x1041eb10}; t.manager_present=0; cases.push_back(t); }
        for (int reason:{0,1,2,27,28,29,35,-1}) for (int response:{0,1,2,128,255})
            for (int temporary:{0,1,2}) for (int links:{1,2}) {
                Case t{0x1041f3a0}; t.action=3; t.reason=reason; t.response=response;
                t.temporary_offer=temporary; t.draft_link_mode=links; cases.push_back(t);
            }
        for (int reason:{27,29,35}) {
            Case t{0x1041f3a0}; t.action=3; t.reason=reason; t.draft_link_mode=0; cases.push_back(t);
        }
        for (int club:{0,1}) for (int bidder:{1,2}) for (int original:{0,1}) {
            Case t{0x1041f3a0}; t.action=3; t.reason=2; t.manager_club_present=club;
            t.bidder_id=bidder; t.original_link_present=original; cases.push_back(t);
        }
        // Boundary mutations test live post-service rereads without asserting
        // that the canonical services make these particular mutations.
        for (int adjustment:{0,17}) for (int contract_present:{0,1}) for (int fee_mutation:{0,1,2}) {
            Case t{0x1041f3a0}; t.action=3; t.reason=28; t.reference_adjustment=adjustment;
            t.contract_present=contract_present; t.mutate_fee=fee_mutation; cases.push_back(t);
        }
        for (int reason:{1,2,27,28,29,35}) {
            Case t{0x1041f3a0}; t.action=3; t.reason=reason; t.mutate_payload=1;
            t.destroy_revision_delta=3; cases.push_back(t);
        }
        for (int control:{-1,0,3,65535}) for (int action:{0,2,3,65539,32768})
            for (int fallback_action:{0,2}) {
                Case t{0x1041f3a0}; t.control=control; t.action=action; t.default_action=fallback_action;
                t.reason=1; cases.push_back(t);
            }
        for (int command:{0,1,-1}) for (int fallback:{0,1,-1,std::numeric_limits<int>::min()}) {
            Case t{0x1041f3a0}; t.command=command; t.fallback=fallback; cases.push_back(t);
        }
        for (int revision:{16,17,18,-1}) for (int status:{0,1,2,3,4,5,6,127,128,255}) {
            Case t{0x10422600}; t.news_revision=revision; t.status=status; cases.push_back(t);
        }
        for (auto entry:{0x1041eb10u,0x10422600u}) for (int height:{1,21,1000}) for (int group:{0,1,3}) {
            Case t{entry}; t.row_height=height; t.group=group; cases.push_back(t);
        }
        for (int fee:{-1,0,1,1000,2147483647}) for (int value:{0,1,25000,2147483647}) for (int services:{0,1}) {
            Case t{0x10422b50}; t.fee=fee; t.selected_value=value; t.actual_total_services=services; cases.push_back(t);
        }
        for (int count:{0,1,3}) for (int selected:{4,5,7}) for (int controlled:{0,1,-1,128,256}) for (int iterator:{0,2}) {
            Case t{0x10422e40}; t.action=2; t.exchange_count=count; t.selected_player_id=selected;
            t.controlled=controlled; t.actual_total_services=iterator; cases.push_back(t);
        }
        for (auto entry:{0x10422e40u,0x10422310u}) for (int control:{-1,0,3,65535})
            for (int action:{0,1,2,3,65538,32768}) for (int fallback_action:{0,1,2,3}) {
                Case t{entry}; t.control=control; t.action=action; t.default_action=fallback_action; cases.push_back(t);
            }
        for (auto entry:{0x10422e40u,0x10422310u}) for (int command:{0,1,-1})
            for (int fallback:{0,1,-1,std::numeric_limits<int>::min()}) {
                Case t{entry}; t.command=command; t.fallback=fallback; cases.push_back(t);
            }
        for (int status:{0,3,16,17,18,127,128,255}) for (int action:{1,2,3}) {
            Case t{0x10422310}; t.status=status; t.action=action; cases.push_back(t);
        }
        for (auto entry:{0x10422b50u,0x10421fb0u,0x104265c0u}) for (int height:{1,21,1000}) for (int group:{0,1,3}) {
            Case t{entry}; t.row_height=height; t.group=group; cases.push_back(t);
        }
        for (int created:{0,1,-1,128,256}) for (int proposed:{0,1}) {
            Case t{0x1042c6d0}; t.create_result=created; t.proposed_present=proposed; cases.push_back(t);
        }
        // Numeric boundary characterization includes wrapped/negative wages.
        for (int current:{1,10,999,1000,10000,2147483647})
            for (int proposed:{-2147483647,-1000,-99,-5,-1,0,1,5,9,10,99,100,105,21474836,21474837,2147483647}) {
                Case t{0x1042c6d0}; t.current_salary=current; t.offered_salary=proposed; cases.push_back(t);
            }
        for (int byte:{0,1,127,128,255,256}) for (int word:{0,2002,32768,65535}) {
            Case t{0x1042c6d0}; t.term_byte_a=byte; t.term_byte_b=255-byte; t.mode=byte;
            t.term_word_a=word; t.term_word_b=65535-word; t.expiry_year=word;
            t.cup_option=byte; t.parent_option=255-byte; cases.push_back(t);
        }
        { Case t{0x1042c6d0}; t.contract_present=0; t.proposed_present=0; cases.push_back(t); }
        // Explicit history-boundary mutation tests the uncached repeated reads.
        { Case t{0x1042c6d0}; t.history_terms_mutation=1; cases.push_back(t); }
        for (int revision:{16,17,18,-1}) for (int status:{0,2,3,5,17,255}) for (int created:{0,1}) {
            Case t{0x10421ee0}; t.news_revision=revision; t.status=status; t.create_result=created; cases.push_back(t);
        }
        for (auto entry:{0x104286c0u,0x10428750u}) {
            for (int created:{0,1,-1,128,256}) for (int present:{0,1}) {
                Case t{entry}; t.create_result=created; t.contract_present=present; cases.push_back(t);
            }
            for (int manager_club:{0,1}) for (int kind:{0,1,2,255}) for (int present:{0,1})
                for (int registered:{0,1}) {
                    Case t{entry}; t.manager_club_present=manager_club; t.club_kind=kind;
                    t.contract_present=present; t.registration=registered; cases.push_back(t);
                }
            for (int flags:{0,4,8,12,255}) for (int listed:{0,1,-1,65536}) for (int role:{0,7,15,16,255}) {
                Case t{entry}; t.status_mask=flags; t.status_listed_value=listed; t.squad_status=role; cases.push_back(t);
            }
            for (int loan:{0,1,-1,127,128,255,256}) for (int other:{0,1,-1,128,256}) {
                Case t{entry}; t.transfer_list_result=loan; t.loan_list_result=other; cases.push_back(t);
            }
            for (unsigned mode:{0u,1u,127u,128u,255u,256u,0xffffffffu}) {
                Case t{entry}; t.mode=mode; t.eligible=std::int32_t(mode); cases.push_back(t);
            }
            for (int group:{0,1,3}) { Case t{entry}; t.group=group; cases.push_back(t); }
            { Case t{entry}; t.status_history_mutation=1; cases.push_back(t); }
        }
        for (int loan:{0,1,-1,128,256}) for (int listed:{0,1,-1,128,256}) {
            Case t{0x1042a810}; t.loan_mode=loan; t.transfer_list_result=listed; t.loan_list_result=listed; cases.push_back(t);
        }
        for (int height:{1,21,1000}) for (int group:{0,1,3}) {
            Case t{0x1042a810}; t.row_height=height; t.group=group; cases.push_back(t);
        }
        for (int query:{1,2}) { Case t{0x1042a810}; t.contract_null_query=query; cases.push_back(t); }
        { Case t{0x1042a810}; t.varying_owner=1; cases.push_back(t); }
        for (int control:{-1,0,3,65535}) for (int action:{0,1,2,3,65538,32768}) for (int default_action:{0,1,2,3}) {
            Case t{0x1042abb0}; t.control=control; t.action=action; t.default_action=default_action; cases.push_back(t);
        }
        for (int loan:{0,1,-1,128,256}) for (int listed:{0,1,-1,128,256}) {
            Case t{0x1042abb0}; t.action=2; t.loan_mode=loan; t.transfer_list_result=listed; t.loan_list_result=listed; cases.push_back(t);
        }
        for (int flags:{0,1,65536,-1}) for (int eligible:{0,1,-1,128,256}) for (int age:{14,15,16,127,128,255}) {
            Case t{0x1042abb0}; t.action=2; t.transfer_list_result=1; t.status_listed_value=flags;
            t.listing_eligible=eligible; t.player_age=age; cases.push_back(t);
        }
        for (int happiness:{-128,-100,-51,-50,-49,0,60,127,128,255,256}) {
            Case t{0x1042abb0}; t.action=2; t.transfer_list_result=1; t.happiness=happiness; cases.push_back(t);
        }
        for (int command:{0,1,-1}) for (int fallback:{0,1,-1,std::numeric_limits<int>::min()}) {
            Case t{0x1042abb0}; t.command=command; t.fallback=fallback; cases.push_back(t);
        }
        // Real heading factory rejects a full control array before consuming
        // its caption. The other UI services retain the declared boundary model.
        for (int present:{0,1}) for (int query:{0,2}) {
            Case t{0x1042a810}; t.actual_header=1; t.contract_present=present; t.contract_null_query=query; cases.push_back(t);
        }
        for (int eligible:{0,1,-1,128,256}) for (int created:{0,1,-1,128,256}) for (int same:{0,1}) {
            Case t{0x1042f670}; t.eligible=eligible; t.create_result=created; t.same_owner=same; cases.push_back(t);
        }
        for (int manager:{0,1}) for (int same:{0,1}) for (int height:{1,21,1000}) for (int group:{0,1,3}) {
            Case t{0x1042f7c0}; t.manager_present=manager; t.same_owner=same; t.row_height=height; t.group=group; cases.push_back(t);
        }
        for (int control:{-1,0,3,65535}) for (int action:{0,1,2,3,65538,32768}) for (int default_action:{0,1,2,3}) {
            Case t{0x1042fb30}; t.control=control; t.action=action; t.default_action=default_action; cases.push_back(t);
        }
        for (int active:{0,1,-1,128,256}) for (int dialog:{0,1,2,3,-1}) for (int action:{0,1,2}) {
            Case t{0x1042fb30}; t.popup_active=active; t.popup_action=dialog; t.action=action; cases.push_back(t);
        }
        for (int same:{0,1}) for (int group:{0,1,3}) {
            Case t{0x1042fb30}; t.action=2; t.same_owner=same; t.group=group; cases.push_back(t);
        }
        for (int command:{0,1,-1}) for (int fallback:{0,1,-1,std::numeric_limits<int>::min()}) {
            Case t{0x1042fb30}; t.command=command; t.fallback=fallback; cases.push_back(t);
        }
        // Explicit service mutations, not claims of ordinary loan service effects.
        for (auto entry:{0x1042f7c0u,0x1042fb30u}) for (int same:{0,1}) {
            Case t{entry}; t.action=2; t.same_owner=same; t.recall_mutation=1; cases.push_back(t);
        }
        for (auto entry:{0x104249e0u,0x10424f80u}) {
            for (int club:{0,1}) for (int alternative:{0,1}) for (unsigned mode:{0u,1u,2u,127u,128u,255u,256u,0xffffffffu}) {
                Case t{entry}; t.manager_club_present=club; t.alternate_club_present=alternative; t.mode=mode; cases.push_back(t);
            }
            for (int height:{1,21,1000}) for (int group:{0,1,3}) {
                Case t{entry}; t.row_height=height; t.group=group; cases.push_back(t);
            }
        }
        for (int control:{-1,0,3,65535}) for (int action:{0,1,2,3,10,11,65537,32768}) for (int default_action:{0,1,2,10,11}) {
            Case t{0x10424d40}; t.control=control; t.action=action; t.default_action=default_action; cases.push_back(t);
        }
        for (int command:{0,1,-1}) for (int fallback:{0,1,-1,std::numeric_limits<int>::min()}) {
            Case t{0x10424d40}; t.command=command; t.fallback=fallback; cases.push_back(t);
        }
        for (unsigned mode:{0u,1u,2u,127u,128u,255u,256u,0xffffffffu}) for (int existing:{0,1}) {
            for (auto entry:{0x1042efa0u,0x1042f380u}) {
                Case t{entry}; t.mode=mode; t.offer_present=existing; t.action=2; cases.push_back(t);
            }
        }
        for (int manager:{0,1}) for (int height:{1,21,1000}) for (int group:{0,1,3}) {
            Case t{0x1042efa0}; t.manager_present=manager; t.row_height=height; t.group=group; cases.push_back(t);
        }
        for (int control:{-1,0,3,65535}) for (int action:{0,1,2,3,65538,32768}) for (int default_action:{0,1,2,3}) {
            Case t{0x1042f380}; t.control=control; t.action=action; t.default_action=default_action; cases.push_back(t);
        }
        for (int day:{-32768,-1,0,1,27,28,29,31,32767,32768,65535}) for (int existing:{0,1}) {
            Case t{0x1042f380}; t.action=2; t.mode=255; t.current_day=day; t.offer_present=existing; cases.push_back(t);
        }
        for (int byte:{0,1,127,128,255,256}) for (int year:{0,2002,32767,32768,65535}) for (int existing:{0,1}) {
            Case t{0x1042f380}; t.action=2; t.mode=255; t.offer_present=existing;
            t.term_byte_a=byte; t.term_byte_b=255-byte; t.term_word_a=year; t.cup_option=byte; t.parent_option=255-byte; cases.push_back(t);
        }
        for (unsigned mode:{0u,1u,2u,128u}) { Case t{0x1042f380}; t.action=2; t.mode=mode; t.actual_trial_constructor=1; cases.push_back(t); }
        // Explicit boundary mutations characterize rereads; ordinary calendar
        // queries are stable. The second day is deliberately not clamped.
        for (int existing:{0,1}) for (int day:{28,-1}) {
            Case t{0x1042f380}; t.action=2; t.mode=255; t.offer_present=existing; t.current_day=day; t.day_mutation=1; cases.push_back(t);
        }
        for (unsigned mode:{1u,255u}) for (int existing:{0,1}) {
            Case t{0x1042f380}; t.action=2; t.mode=mode; t.offer_present=existing; t.temporary_club_mutation=1; cases.push_back(t);
        }
        for (int command:{0,1,-1}) for (int fallback:{0,1,-1,std::numeric_limits<int>::min()}) {
            Case t{0x1042f380}; t.command=command; t.fallback=fallback; cases.push_back(t);
        }
        for (int type:{0,4,7,8,9,10,11,12,127,128,255}) for (int eligible:{0,1,128,256}) {
            Case t{0x1042ae90}; t.contract_type=type; t.listing_eligible=eligible; cases.push_back(t);
        }
        for (int days:{-32768,-1,0,1,6,7,8,28,365,32767,32768,65535}) {
            Case t{0x1042ae90}; t.contract_type=8; t.release_days=days; cases.push_back(t);
        }
        for (int days:{1,7,32767}) for (int salary:{std::numeric_limits<int>::min(),-1000,-7,-5,-4,-3,-2,-1,0,1,6,7,8,1000,std::numeric_limits<int>::max()})
            for (int eligible:{0,1}) for (int money:{0,1}) {
                Case t{0x1042ae90}; t.contract_type=8; t.release_days=days; t.current_salary=salary;
                t.listing_eligible=eligible; t.actual_total_services=money; cases.push_back(t);
            }
        for (int high:{0,1,-1,std::numeric_limits<int>::max(),std::numeric_limits<int>::min()}) {
            Case t{0x1042ae90}; t.contract_type=8; t.monetary_high=high; cases.push_back(t);
        }
        for (int height:{1,21,1000}) for (int group:{0,1,3}) {
            Case t{0x1042ae90}; t.row_height=height; t.group=group; cases.push_back(t);
        }
        // Explicit dependency mutations characterize live reads; no claim
        // that the canonical remaining-days helper mutates the salary.
        { Case t{0x1042ae90}; t.contract_type=8; t.release_salary_mutation=1; cases.push_back(t); }
        for (int kind:{0,8}) { Case t{0x1042ae90}; t.contract_type=kind; t.varying_owner=1; cases.push_back(t); }
        { Case t{0x1042ae90}; t.contract_type=8; t.contract_null_query=3; cases.push_back(t); }
        { Case t{0x1042ae90}; t.contract_type=8; t.contract_type_query=3; cases.push_back(t); }
        for (auto entry:{0x1042bf70u,0x1042c2c0u}) {
            for (int eligible:{0,1,-1,128,256}) for (int created:{0,1,-1,128,256}) {
                Case t{entry}; t.eligible=eligible; t.create_result=created; cases.push_back(t);
            }
            for (int current:{-1000,1,10,999,1000,10000,2147483647})
                for (int proposed:{-2147483647,-1000,-99,-5,-1,0,1,5,9,10,99,100,105,1270,1280,2550,2560,21474836,21474837,2147483647}) {
                    Case t{entry}; t.current_salary=current; t.offered_salary=proposed; cases.push_back(t);
                }
            for (int type:{0,20,23,24,25,26,27,28,127,128,255}) for (int present:{0,1}) {
                Case t{entry}; t.proposed_type=type; t.proposed_present=present; cases.push_back(t);
            }
            for (int byte:{0,1,127,128,255,256}) for (int word:{0,2002,32768,65535}) {
                Case t{entry}; t.term_byte_a=byte; t.term_byte_b=255-byte; t.mode=byte;
                t.term_word_a=word; t.term_word_b=65535-word; t.expiry_year=word;
                t.cup_option=byte; t.parent_option=255-byte; cases.push_back(t);
            }
            for (int group:{0,1,3}) { Case t{entry}; t.group=group; cases.push_back(t); }
            // Deliberately noncanonical callbacks characterize later reads.
            { Case t{entry}; t.history_terms_mutation=1; cases.push_back(t); }
            { Case t{entry}; t.contract_null_query=2; cases.push_back(t); }
        }
        for (int revision:{16,17,18,-1}) for (int status:{0,1,2,3,4,5,127,128,255}) for (int created:{0,1}) {
            Case t{0x1042bf70}; t.news_revision=revision; t.status=status; t.create_result=created; cases.push_back(t);
        }
        { Case t{0x1042bf70}; t.news_proposed_present=0; cases.push_back(t); }
        for (int weeks:{-129,-128,-1,0,1,2,3,4,5,127,128,255,256,257}) {
            Case t{0x1042b580}; t.mode=weeks; cases.push_back(t);
        }
        for (int person:{0,1}) for (int manager:{0,1}) for (int club:{0,1}) {
            Case t{0x1042b580}; t.person_present=person; t.manager_present=manager; t.manager_club_present=club; cases.push_back(t);
        }
        for (int height:{1,21,1000}) for (int group:{0,1,3}) for (int id:{-1,10,32767,65535}) {
            Case t{0x1042b580}; t.row_height=height; t.group=group; t.first_control=id; cases.push_back(t);
        }
        for (int control:{-1,0,3,65535}) for (int action:{0,1,2,3,4,65537,65538,32768}) for (int fallback_action:{0,1,2,3,4}) {
            Case t{0x1042bae0}; t.control=control; t.action=action; t.default_action=fallback_action; cases.push_back(t);
        }
        for (int action:{1,2}) for (int weeks:{-129,-128,-1,0,1,2,3,4,5,127,128,255,256,257}) for (int id:{-1,10,32767,65535}) {
            Case t{0x1042bae0}; t.action=action; t.mode=weeks; t.first_control=id; cases.push_back(t);
        }
        for (int created:{0,1,-1,128,256}) { Case t{0x1042bae0}; t.action=4; t.create_result=created; cases.push_back(t); }
        for (int command:{0,1,-1}) for (int fallback:{0,1,-1,std::numeric_limits<int>::min()}) {
            Case t{0x1042bae0}; t.command=command; t.fallback=fallback; cases.push_back(t);
        }
        for (auto bound:{0u,1u,2u,17u,65534u,65535u,65536u,65537u,1000000u,2147483647u,2147483648u,0xffff0000u,0xffff0001u,0xfffffffeu,0xffffffffu})
            for (int word:{0,32767,65534,65535}) for (int salt:{0,65535,std::numeric_limits<int>::max(),std::numeric_limits<int>::min(),-1})
                for (int wrap:{0,1,2}) {
                    Case t{0x10425980}; t.mode=bound; t.random_word=word; t.random_salt=salt; t.random_wrap=wrap; cases.push_back(t);
                }
        for (int control:{-1,0,3,65535}) for (int action:{0,1,2,3,65537,32768}) for (int fallback_action:{0,1,2}) for (auto mode:{0u,1u,0xffffffffu}) {
            Case t{0x10425240}; t.control=control; t.action=action; t.default_action=fallback_action; t.mode=mode; cases.push_back(t);
        }
        for (int club:{0,1}) for (int alternate:{0,1}) for (int board:{0,1}) for (int restricted:{0,1,128,256}) {
            Case t{0x10425240}; t.action=1; t.manager_club_present=club; t.alternate_club_present=alternate;
            t.board_present=board; t.club_restricted=restricted; cases.push_back(t);
        }
        for (int rating:{-32768,-300,-1,0,299,300,4999,5000,6999,7000,10800,11099,11100,11399,11400,32767})
            for (int word:{0,1,17,32767,65535}) for (int wrap:{0,1,2}) {
                Case t{0x10425240}; t.action=1; t.shortlist_rating=rating; t.random_word=word; t.random_wrap=wrap; cases.push_back(t);
            }
        // A negative multiply-high result divisible by 32 clears the shift carry.
        { Case t{0x10425240}; t.action=1; t.shortlist_rating=-32699; cases.push_back(t); }
        for (double interest:{-1.0,0.0,0.1,0.5,1.0,2.74,2.75,3.0,std::numeric_limits<double>::infinity(),std::numeric_limits<double>::quiet_NaN()}) {
            Case t{0x10425240}; t.action=1; t.interest=interest; cases.push_back(t);
        }
        for (int present:{0,1}) for (int kind:{0,20,23,24,127,128,255}) for (int controlled:{0,1,128,256}) for (int free:{0,1,128,256}) {
            Case t{0x10425240}; t.action=1; t.contract_present=present; t.contract_type=kind; t.controlled=controlled; t.free_move=free; cases.push_back(t);
        }
        for (int high:{-1,0,1,std::numeric_limits<int>::min()}) for (int low:{0,1,10000,-1})
            for (int bank:{0,1}) for (int fee:{-1,0,25000,2147483647}) {
                Case t{0x10425240}; t.action=1; t.budget_high=high; t.board_limit=low;
                t.bank_high=bank ? high : 0; t.bank_low=bank ? low : 0; t.selected_value=fee; cases.push_back(t);
            }
        for (int word:{0,1,65535}) for (int salt:{-1,std::numeric_limits<int>::min(),std::numeric_limits<int>::max()}) for (int wrap:{0,1,2}) {
            Case t{0x10425240}; t.action=1; t.random_word=word; t.random_salt=salt; t.random_wrap=wrap; cases.push_back(t);
        }
        for (int group:{0,1,3}) { Case t{0x10425240}; t.action=1; t.group=group; cases.push_back(t); }
        for (int command:{0,1,-1}) for (int fallback:{0,1,-1,std::numeric_limits<int>::min()}) {
            Case t{0x10425240}; t.command=command; t.fallback=fallback; cases.push_back(t);
        }
        { Case t{0x10425240}; t.action=1; t.shortlist_mutation=1; cases.push_back(t); }
        { Case t{0x10425240}; t.action=1; t.varying_owner=1; cases.push_back(t); }
        // The original interest threshold is 2.75. Exercise the remaining
        // low-rating exits after a zero first draw, independently of it.
        for (int club:{0,1}) for (int controlled:{0,1,128,256}) {
            Case t{0x10425240}; t.action=1; t.shortlist_rating=0;
            t.same_owner=club; t.controlled=controlled; cases.push_back(t);
        }
        // Exact negative arithmetic shifts, including zero and nonzero
        // refinement bounds, distinguish the carry's discarded-bit test.
        for (auto bound:{1717980365u,1717967258u}) {
            Case t{0x10425980}; t.mode=bound; cases.push_back(t);
        }
        // An opaque null hyperlink target is valid when the separately
        // queried caption remains nonnull; this is a query-mutation case.
        for (int kind:{0,8}) {
            Case t{0x1042ae90}; t.contract_type=kind;
            t.contract_null_query=kind ? 7 : 4; cases.push_back(t);
        }
        for (int reason:{0,0x200,0x400,0x2000,0x1000,10,0x800,-1,0x1000a,0x18000})
            for (int mask:{0,1,2,4,8,15}) for (int eligible:{0,1,128,256}) {
                Case t{0x1041f8f0}; t.reason=reason; t.rejection_mask=mask; t.eligible=eligible; cases.push_back(t);
            }
        for (int count:{0,1,2,4}) for (int failure:{-1,0,1,3}) for (int pattern:{0,1,2}) for (int iterator:{0,2}) {
            Case t{0x1041f8f0}; t.pending_offers=count; t.pending_failure=failure;
            t.pending_pattern=pattern; t.actual_total_services=iterator; cases.push_back(t);
        }
        for (int draft:{0,1,2}) for (int manager:{0,1}) for (int group:{0,1,3}) {
            Case t{0x1041f8f0}; t.draft_link_mode=draft; t.manager_present=manager; t.group=group; cases.push_back(t);
        }
        for (int height:{1,21,1000}) { Case t{0x1041f8f0}; t.row_height=height; cases.push_back(t); }
        for (int action:{0,1,2,3,4,5,65537,32768}) for (int control:{-1,0,3,65535}) for (int fallback_action:{0,1,4}) {
            Case t{0x104204f0}; t.action=action; t.control=control; t.default_action=fallback_action; cases.push_back(t);
        }
        for (int action:{2,4}) for (int equal:{0,1}) for (int contract:{0,1}) for (int draft:{1,2})
            for (int metadata:{-1,0,0x12345678}) {
                Case t{0x104204f0}; t.action=action; t.reference_adjustment=equal ? 0 : 37;
                t.contract_present=contract; t.draft_link_mode=draft; t.selected_value=metadata; cases.push_back(t);
            }
        for (int action:{2,4}) for (int reason:{-32768,-1,0,10,0x800,0x2000,0x1000a}) for (int response:{0,2,128,258}) {
            Case t{0x104204f0}; t.action=action; t.reason=reason; t.response=response; cases.push_back(t);
        }
        for (int count:{0,1,2,4}) for (int pattern:{0,1,2}) for (int status:{-1,0,1,128,256}) for (int iterator:{0,2}) {
            Case t{0x104204f0}; t.action=4; t.pending_offers=count; t.pending_pattern=pattern;
            t.status=status; t.actual_total_services=iterator; cases.push_back(t);
        }
        for (int control:{-1,0,3,65535}) for (int payload:{-1,0,10,65546,std::numeric_limits<int>::min()}) {
            Case t{0x104204f0}; t.action=1; t.default_action=1; t.control=control; t.selected_value=payload; cases.push_back(t);
        }
        for (int action:{2,4}) for (int mutation:{1,2,3}) {
            Case t{0x104204f0}; t.action=action; t.rejection_mutation=mutation; t.reference_adjustment=17; cases.push_back(t);
        }
        for (int command:{0,1,-1}) for (int fallback:{0,1,-1,std::numeric_limits<int>::min()}) {
            Case t{0x104204f0}; t.command=command; t.fallback=fallback; cases.push_back(t);
        }
        for(int status:{0,1,2,3,128,256}) for(int reason:{-32768,-1,0,3,4,34,5,35,23,24,14,20,21,12,13,15,16,17,65539}) {
            Case t{0x10423270};t.completion_status=status;t.reason=reason;cases.push_back(t);
        }
        for(int list:{0,1})for(int count:{0,1,2,3,4,5,6})for(int draft:{0,1})for(int iterator:{0,2}) {
            Case t{0x10423270};t.exchange_list=list;t.exchange_count=count;t.draft_present=draft;
            t.actual_total_services=iterator;cases.push_back(t);
        }
        for(int manager:{1,2})for(int exchanges:{0,1})for(int fee:{-1,0,1,16777215,16777216,16777217,2147483647})
            for(int extra:{-2,0,1,1000000})for(int value:{-100,100,2147483647}) {
                Case t{0x10423270};t.manager_club_id=manager;t.exchange_list=exchanges;t.exchange_count=exchanges;
                t.fee=fee;t.league_fee=extra;t.international_fee=0;t.valuation=value;cases.push_back(t);
            }
        for(int kind:{0,20,23,128,255})for(int present:{0,1}) {
            Case t{0x10423270};t.contract_type=kind;t.contract_present=present;
            t.fee=0;t.sell_on=0;t.league_fee=0;t.international_fee=0;cases.push_back(t);
        }
        for(int percent:{-128,-1,0,1,100,127,256})for(int value:{-2147483647,-32768,0,32767,2147483647}) {
            Case t{0x10423270};t.sell_on=percent;t.valuation=value;cases.push_back(t);
        }
        for(int draft:{0,1})for(int count:{0,3}) {
            Case t{0x10423270};t.draft_present=draft;t.exchange_list=1;t.exchange_count=count;t.actual_reset_services=1;
            t.actual_total_services=3;cases.push_back(t);
        }
        for(int height:{1,21,1000})for(int group:{0,1,3}) {
            Case t{0x10423270};t.row_height=height;t.group=group;cases.push_back(t);
        }
        {Case t{0x10423270};t.completion_mutation=1;cases.push_back(t);}
        for(int action:{0,1,2,3,4,5,65537,32768})for(int control:{-1,0,3,65535})for(int fallback:{0,1,2}) {
            Case t{0x10424220};t.action=action;t.control=control;t.default_action=fallback;cases.push_back(t);
        }
        for(int action:{1,2,3,4})for(int revision:{16,17,18})for(int status:{0,2,3,5,128,255})for(int create:{0,1}) {
            Case t{0x10424220};t.action=action;t.news_revision=revision;t.status=status;t.create_result=create;cases.push_back(t);
        }
        for(int action:{1,2,3,4})for(int manager:{1,2})for(int eligible:{0,1,128,256})for(int create:{0,1,256}) {
            Case t{0x10424220};t.action=action;t.manager_club_id=manager;t.eligible=eligible;t.create_result=create;cases.push_back(t);
        }
        for(int action:{1,2,3,4})for(int reason:{-32768,-129,-128,-1,0,1,127,128,255,256})for(int status:{0,1,2,3,258}) {
            Case t{0x10424220};t.action=action;t.reason=reason;t.completion_status=status;cases.push_back(t);
        }
        for(int action:{1,2,3,4}) {Case t{0x10424220};t.action=action;t.completion_mutation=2;t.news_revision=16;cases.push_back(t);}
        for(int command:{0,1,-1})for(int fallback:{0,1,-1,std::numeric_limits<int>::min()}) {
            Case t{0x10424220};t.command=command;t.fallback=fallback;cases.push_back(t);
        }
        // Negative multiply-high result -32 has no discarded shift bits.
        {Case t{0x10423270};t.valuation=-99;t.sell_on=1;cases.push_back(t);}
        for(int constant:{1,2}) {Case t{0x10423270};t.completion_constant=constant;cases.push_back(t);}
        unsigned completed = 0;
        const auto selected_entry = std::getenv("CM_TRANSFER_SCREENS_ENTRY");
        const auto entry_filter = selected_entry ? std::strtoul(selected_entry, nullptr, 0) : 0;
        const auto selected_action = std::getenv("CM_TRANSFER_SCREENS_ACTION");
        const auto action_filter = selected_action ? std::strtol(selected_action, nullptr, 0) : 0;
        std::map<std::uint32_t, std::array<unsigned, 2>> counts;
        for (bool production : {false, true}) {
          if (production && std::getenv("CM_TRANSFER_SCREENS_CONTROLLED_ONLY")) continue;
          production_mode = production;
          for (const auto& test : cases) {
            if (entry_filter && test.entry != entry_filter) continue;
            if (selected_action && test.action != action_filter) continue;
            auto s = cm::semantic_io_checks::clone(*base); Cpu c(*s); active = test; call_count = 0;
            initialize(c, test); s->pc = test.entry;
            ObservedWrites observer;
            if (production) s->memory.observer = &observer;
            const auto differences = compare_routine_call(*s, test.entry, original, rewrite,
                RoutineChecker::result_r3 | RoutineChecker::result_f1);
            if (!differences.empty()) {
                std::cerr << "Case " << completed << " entry " << hex_address(test.entry) << '\n';
                for (const auto& d : differences) std::cerr << d << '\n';
                return 1;
            }
            ++completed;
            ++counts[test.entry][production];
          }
        }
        production_mode = false;
        auto s = cm::semantic_io_checks::clone(*base); Cpu c(*s); active = Case{entries[0]}; call_count = 0;
        initialize(c, active); s->pc = active.entry;
        if (compare_routine_call(*s, active.entry, original, corrupted, RoutineChecker::result_r3).empty())
            throw std::runtime_error("Corrupted history was not detected");
        active = Case{0x10420e40}; call_count = 0;
        auto result_case = cm::semantic_io_checks::clone(*base); Cpu result_cpu(*result_case);
        initialize(result_cpu, active); result_case->pc = active.entry;
        const auto result_differences = compare_routine_call(*result_case, active.entry, original,
            corrupted_result, RoutineChecker::result_r3);
        if (result_differences.size() != 1 || !result_differences.front().starts_with("r3 "))
            throw std::runtime_error("Corrupted command result was not detected independently");
        for (auto entry : entries) {
            if (std::getenv("CM_TRANSFER_SCREENS_CONTROLLED_ONLY")) continue;
            if (entry_filter && entry != entry_filter) continue;
            active = Case{entry};
            const auto prepared = [&] {
                auto state = cm::semantic_io_checks::clone(*base); Cpu cpu(*state);
                call_count = 0; initialize(cpu, active); return state;
            };
            auto expected = prepared(); original(*expected, entry);
            const auto limit = expected->executed - 1;
            auto normal = prepared(); normal->budget = limit; ObservedWrites observer;
            normal->memory.observer = &observer; production_mode = true; rewrite(*normal, entry);
            auto rejected = prepared(); rejected->budget = limit; bool exhausted = false;
            try { original(*rejected, entry); } catch (const std::exception& error) {
                exhausted = std::string_view(error.what()).find("budget") != std::string_view::npos;
            }
            if (!exhausted || normal->executed >= expected->executed)
                throw std::runtime_error("Production ownership budget control failed " + hex_address(entry));
        }
        for (const auto& [entry, n] : counts)
            std::cout << "entry " << hex_address(entry) << ": " << n[0] << " controlled, " << n[1] << " production\n";
        if (!completed) throw std::runtime_error("No comparison cases selected");
        std::cout << "TRANSFER_SCREENS: " << completed << " returned comparisons; corruption controls passed; "
                  << (std::getenv("CM_TRANSFER_SCREENS_CONTROLLED_ONLY") ? "production controls not run\n" : "production controls passed\n");
    } catch (const std::exception& e) {
        std::cerr << "Entry " << hex_address(active.entry) << " action " << active.action << ": " << e.what() << '\n';
        return 1;
    }
}
