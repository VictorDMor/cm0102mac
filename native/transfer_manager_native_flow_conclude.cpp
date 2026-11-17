#include "transfer_manager_native_flow.hpp"
#include "transfer_manager_native_flow_common.hpp"
#include "debug_force_purchase.hpp"

namespace cm::aot::transfer_manager_native {
using namespace flow;

void conclude_offer_round(Cpu& c, std::uint32_t manager, std::uint32_t slot, std::uint32_t final_flag) {
    constexpr std::uint32_t kind_accepted = 11, stage_waiting = 5, stage_agreed = 0xf, answer_at = 0x3c, rating_at = 0x38;
    constexpr std::uint32_t pool_scale = 0, pool_limit = 0x18, item_flag = 5, rows_status_size = 10;
    Frame frame(c, 0x103e4740, 0x100, {.first_gpr = 18, .gpr_offset = -0x48, .first_fpr = 31});
    guest::Fpu fpu(c);
    const auto pool = c.toc(-0x4ba0), finance = c.toc(-0x7ab8), news = c.toc(toc::news), contracts = c.toc(toc::contracts);
    keep(c, 27, manager);
    keep(c, 21, pool);
    keep(c, 28, slot);
    keep(c, 22, finance);
    keep(c, 18, final_flag);
    keep(c, 23, news);
    keep(c, 31, 0);
    keep(c, 24, c.toc(toc::people));
    keep(c, 25, contracts);
    keep(c, 29, c.toc(toc::found_contract));
    keep(c, 26, c.toc(toc::clubs));
    const auto shortlist = [&] { return c.u32(manager + manager_shortlist); };
    invoke(c, 0x103721f0, 0x103e4788, {frame.at(answer_at)});
    auto offer = c.u32(slot);
    const auto person = person_of(c, c.u32(offer + offer_at::person));
    const auto club = club_of(c, c.u32(offer + offer_at::club));
    keep(c, 20, 0);
    keep(c, 19, person);
    keep(c, 30, club);
    // The club the person's contract is with, when that is not the offer's own club (0 when there is none).
    const auto partner = [&] {
        const auto contract = find_contract(c, person, 0x103e47bc);
        keep(c, 20, contract && (std::int8_t(c.u8(contract + contract_status)) >> 2) != 5);
        return contract_club(c, contract);
    }();
    keep(c, 20, partner);
    offer = c.u32(slot);
    c.put8(offer + offer_at::step36, 0);
    keep(c, 29, offer);
    if (std::int8_t(final_flag)) {
        c.put8(offer + offer_at::step35, 1);
        invoke(c, 0x1036fa00, 0x103e4840, {shortlist(), person, club, c.u32(c.u32(slot))});
    }
    c.put8(offer + offer_at::step36, 0);
    c.put8(offer + offer_at::stage, stage_waiting);
    std::uint32_t seen = 0;
    if (std::int8_t(invoke(c, 0x102cdbf0, 0x103e4868, {c.u32(news), club_of(c, c.u32(offer + offer_at::club))}))) {
        seen = 1;
        keep(c, 31, seen);
    }
    if (partner && std::int8_t(invoke(c, 0x102cdbf0, 0x103e4888, {c.u32(news), partner}))) {
        ++seen;
        keep(c, 31, seen);
    }
    invoke(c, 0x104024e0, 0x103e48a0, {manager, offer});
    const auto leave_list_if_empty = [&](std::uint32_t back) {
        const auto list = c.u32(offer + offer_at::listed);
        if (s16(c.u16(list)) == 0) {
            invoke(c, 0x10738c20, back, {list, 1});
            c.put32(offer + offer_at::listed, 0);
        }
    };
    const auto people_row = [&](std::uint32_t index) { return person_of(c, index); };
    if (c.u8(offer + offer_at::kind) == kind_accepted) {
        std::uint32_t buyer_free = 1;
        if (!std::int8_t(invoke(c, 0x1014c2e0, 0x103e48c4, {club, 0, 1, 0, 0})))
            buyer_free = invoke(c, 0x1036d510, 0x103e48e8, {shortlist(), person, club, 0});
        const auto rating = invoke(c, 0x10369080, 0x103e4908, {shortlist(), person, club, 0, 0, buyer_free, frame.at(rating_at)});
        (void)rating;
        const double value = c.f(1);
        const double limit = c.f64(pool + pool_limit);
        fpu.fcmp(0, value, limit);
        // The original force hook follows the rating/FP comparison, so keep
        // those calls and their RNG effects even when willingness is forced.
        const bool forced = forced_purchase_for_seller(c.s, offer, partner);
        if (value < limit && !forced) {
            invoke(c, 0x10403250, 0x103e4924, {manager, offer, c.u32(frame.at(rating_at))});
            c.put8(offer + offer_at::stage, stage_agreed);
            c.put8(offer + offer_at::step36, 3);
            invoke(c, 0x103ec4f0, 0x103e4940, {manager, offer});
            invoke(c, 0x103eb4a0, 0x103e4954, {manager, offer, 10, 0});
            invoke(c, 0x103f6190, 0x103e4960, {manager, offer});
            const auto status = c.u8(offer + offer_at::kind);
            const auto record = invoke(c, 0x1036d240, 0x103e49a8,
                                       {shortlist(), club_of(c, c.u32(offer + offer_at::club)), people_row(c.u32(offer + offer_at::person)),
                                        not_eleven(status), 0});
            if (!record) return;
            invoke(c, 0x10365590, 0x103e49f8,
                   {shortlist(), people_row(c.u32(offer + offer_at::person)), club_of(c, c.u32(offer + offer_at::club)), 1,
                    not_eleven(c.u8(record))});
            return;
        }
    }
    const auto after_loop = [&] {
        if (std::int8_t(seen)) invoke_native(c, 0x10402b90, 0x103e4be4, {manager, offer});
        invoke(c, 0x103eb4a0, 0x103e4bf8, {manager, offer, 0, 1});
        if (!c.u32(offer + offer_at::listed)) return;
        if (std::int8_t(invoke(c, 0x1014c2e0, 0x103e4c1c, {partner, 0, 1, 0, 0}))) return;
        const auto list = c.u32(offer + offer_at::listed);
        c.put32(list + 0xc, c.u32(list + 4));
        for (;;) {
            const auto entry = invoke(c, 0x10738e00, 0x103e4c70, {c.u32(offer + offer_at::listed)});
            if (!entry) return;
            const auto record = invoke(c, 0x1036d240, 0x103e4c58, {shortlist(), partner, people_row(c.u32(entry)), 0, 0});
            if (record) c.put32(record + 0x18, c.u32(offer + offer_at::own_slot));
        }
    };
    const auto list = c.u32(offer + offer_at::listed);
    if (!list) return after_loop();
    // The listed people: each whose worth the club values above the pay offered is signed off the list.
    c.put32(list + 0xc, c.u32(list + 4));
    keep(c, 26, 0x43300000);
    for (;;) {
        const auto entry = invoke(c, 0x10738e00, 0x103e4ba8, {c.u32(offer + offer_at::listed)});
        keep(c, 19, entry);
        if (!entry) {
            leave_list_if_empty(0x103e4bc8);
            return after_loop();
        }
        const auto listed_person = people_row(c.u32(entry));
        keep(c, 23, listed_person);
        if (std::int8_t(invoke(c, 0x1014c2e0, 0x103e4a3c, {partner, 0, 1, 0, 0}))) c.put8(entry + item_flag, 2);
        const auto account = invoke(c, 0x10103510, 0x103e4a58, {c.u32(finance), partner});
        const auto value = std::int32_t(invoke(c, 0x100f2680, 0x103e4a64, {account, listed_person, c.u8(entry + item_flag)}));
        const double scaled = fpu.fmul(c.f64(pool + pool_scale), int_to_double(c, fpu, pool, word(value)));
        c.set_f(31, scaled);
        const auto worth = std::int32_t(invoke(c, 0x10374f50, 0x103e4aa8,
                                               {frame.at(answer_at), listed_person, partner, 0xffffffff, c.u8(entry + item_flag), 0xb, 0, 0}));
        const double worth_value = int_to_double(c, fpu, pool, word(worth));
        fpu.fcmp(0, worth_value, scaled);
        if (!(worth_value > scaled)) continue;
        const auto pay = invoke(c, 0x10084be0, 0x103e4ae4, {c.u32(contracts), listed_person, club, 0, 0xffffffff});
        const auto total = c.u32(offer + offer_at::amount) + pay;
        keep(c, 18, total);
        invoke_native(c, 0x10403890, 0x103e4b00, {manager, offer, listed_person, total});
        invoke(c, 0x10738e60, 0x103e4b0c, {c.u32(offer + offer_at::listed), entry});
        const auto row = c.u32(manager + manager_status_rows) + c.u32(listed_person) * rows_status_size + 2;
        c.put8(row, c.u8(row) - 1);
        c.put32(offer + offer_at::amount, total);
        if (!partner) continue;
        if (std::int8_t(invoke(c, 0x1014c2e0, 0x103e4b4c, {partner, 0, 1, 0, 0}))) continue;
        invoke_native(c, 0x103ee1b0, 0x103e4b64, {manager, slot, 1});
        if (!std::int8_t(invoke(c, 0x103fb390, 0x103e4b70, {manager, offer}))) continue;
        leave_list_if_empty(0x103e4b90);
        return;
    }
}
}
