#include "transfer_manager_native_daily.hpp"
#include "transfer_manager_native_detail.hpp"

namespace cm::aot::transfer_manager_native {
using guest::Cpu;
using guest::Frame;
namespace {
using daily::Lookup;
constexpr std::uint32_t list_init = 0x10738c00, list_free = 0x10738c20, list_copy = 0x10739010, list_next = 0x10738e00,
                        set_stage = 0x103eb4a0, ready_offers = 0x103ec440, record_stage = 0x103ec4f0, follow_up = 0x103f6190,
                        blocked_by = 0x103fb390, blocked_stage = 0x103fb630, first_contact = 0x103e4740,
                        reply_offer = 0x103e4d70, pending_offer = 0x103e5810, ask_other_side = 0x103ee1b0,
                        send_to_recipient = 0x103e44b0, send_home = 0x103e4130, decide_offer = 0x103e8280,
                        settle_offer = 0x103ed170, notify_club = 0x103e95a0, pending_stage = 0x103e7cf0,
                        find_shortlist_row = 0x1036d240, add_shortlist_row = 0x10365590, shortlist_check = 0x1036f170,
                        contract_exists = 0x1007f610, contract_note = 0x1007f200, affordable = 0x10080d30,
                        contract_terms = 0x103721f0, news_check = 0x102cdbf0, offer_fee = 0x10416840,
                        loan_to_club = 0x104024e0, finish_loan = 0x10403250, ask_for_staff = 0x103ffe80,
                        close_offer = 0x103fdc20;
constexpr std::uint32_t offer_kind = 0x24, offer_amount = 0x20, offer_age = 0x11, offer_rival = 0x10, offer_wait = 0x36,
                        offer_contract_term = 0x28, offer_contract_extra = 0x2c, manager_shortlist = 0x988;
}

void process_offer(Cpu& c, std::uint32_t manager, std::uint32_t slot) {
    Frame frame(c, 0x103eb650, 0x160, {.first_gpr = 20});
    Lookup look{c};
    const auto manager_cell = c.toc(toc::manager), constants = c.toc(-0x4ba0);
    keep(c, 25, manager_cell);
    keep(c, 31, manager);
    keep(c, 26, constants);
    keep(c, 20, slot);
    keep(c, 27, look.people);
    keep(c, 28, look.contracts);
    keep(c, 29, look.found_cell);
    keep(c, 30, look.clubs);
    invoke(c, list_init, 0x103eb688, {frame.at(0x58)});
    const auto offer = c.u32(slot);
    keep(c, 23, offer);
    keep(c, 21, 0);
    const auto person = look.person_of(offer);
    keep(c, 22, person);
    const auto found = look.find(person, 0x103eb6ac);
    keep(c, 21, look.usable(found));
    // The club of the person's contract (0 without a usable one) and the offer's own club.
    const auto person_club = look.usable(found) ? look.club_of_found(found) : 0;
    keep(c, 24, person_club);
    const auto offer_club = look.offer_club_of(offer);
    keep(c, 21, offer_club);
    const auto m988 = [&] { return c.u32(manager + manager_shortlist); };
    const auto stage = [&](std::uint32_t of) { return std::int32_t(c.i8(of + daily::offer_stage)); };
    const auto set = [&](std::uint32_t of, std::uint32_t value) { c.put8(of + daily::offer_stage, value); };
    const auto blocked = [&](std::uint32_t club, std::uint32_t back) {
        return std::int8_t(invoke(c, daily::club_flag, back, {club, 0, 1, 0, 0})) != 0;
    };
    const auto flag_if_not_eleven = [&](std::uint32_t record) { return std::uint32_t(std::int32_t(c.i8(record + offer_age)) != 0xb); };
    // One candidate person of an offer against a club: its terms and the contract manager's verdict, then the
    // shortlist rows or the club's note. `record_at` is the frame slot that holds the offer's address.
    const auto settle_candidate = [&](std::uint32_t candidate, std::uint32_t club, std::uint32_t record_at,
                                      std::uint32_t terms_at, std::uint32_t verdict_back, std::uint32_t note_back,
                                      std::uint32_t found_back, std::uint32_t add_back, std::uint32_t send_back,
                                      std::uint32_t notify_back, std::uint32_t terms_back) {
        invoke(c, contract_terms, terms_back, {terms_at});
        c.set(10, 2);
        c.set_f(1, c.f64(constants + 0x80));
        const auto at_offer = c.u32(record_at);
        const auto verdict = invoke(c, affordable, verdict_back,
                                    {c.u32(look.contracts), candidate, club, terms_at, c.u32(at_offer + offer_contract_term)});
        if (std::int8_t(verdict) != 0) {
            const auto again = c.u32(record_at);
            invoke(c, contract_note, note_back,
                   {c.u32(look.contracts), terms_at, c.u32(again + offer_contract_term), c.u32(again + offer_contract_extra), 0xb, 0, 1, 0});
            return;
        }
        const auto held = c.u32(record_at);
        if (c.u32(candidate) == c.u32(held + daily::offer_person)) {
            const auto flag = flag_if_not_eleven(held);
            const auto row = invoke(c, find_shortlist_row, found_back, {m988(), club, candidate, flag, 0});
            if (row != 0)
                invoke(c, add_shortlist_row, add_back, {m988(), candidate, club, 1, std::uint32_t(std::int32_t(c.i8(row)) != 0xb)});
            invoke_native(c, send_to_recipient, send_back, {manager, record_at, 0, 0x16});
        } else {
            c.put32(held + offer_amount, c.u32(held + offer_amount) + c.u32(candidate + 8));
            invoke_native(c, notify_club, notify_back, {c.u32(manager_cell), candidate, c.u32(record_at), 1, 0x16});
        }
    };

    if (stage(offer) == 0) {
        if (person_club != 0 && blocked(person_club, 0x103eb734)) {
            set(offer, 4);
            invoke(c, set_stage, 0x103eb758, {manager, offer, 0xe, 0});
            invoke(c, record_stage, 0x103eb764, {manager, offer});
        } else if (person_club == 0) {
            invoke_native(c, first_contact, 0x103eb780, {manager, slot, 0});
        } else if (c.u8(c.u32(slot) + offer_kind) == 1) {
            c.put32(frame.at(0x38), 1);
            const auto answer = invoke(c, shortlist_check, 0x103eb7c0, {m988(), person_club, person, 0, 0, offer_club, 0xffffffff, 0xffffffff});
            if (std::int8_t(answer) == 0) {
                const auto exists = invoke(c, contract_exists, 0x103eb7d4, {c.u32(look.contracts), c.u32(slot)});
                keep(c, 21, exists);
                set(c.u32(slot), 8);
                invoke(c, loan_to_club, 0x103eb7f0, {manager, c.u32(slot)});
                if (std::int8_t(exists) != 0) {
                    invoke(c, finish_loan, 0x103eb808, {manager, c.u32(slot), 0});
                    set(c.u32(slot), 0xf);
                    c.put8(c.u32(slot) + offer_wait, 3);
                    invoke(c, record_stage, 0x103eb82c, {manager, c.u32(slot)});
                    invoke(c, set_stage, 0x103eb840, {manager, c.u32(slot), 10, 0});
                    invoke(c, follow_up, 0x103eb84c, {manager, c.u32(slot)});
                }
            } else {
                const auto taken = c.u32(slot);
                keep(c, 20, taken);
                set(taken, 2);
                invoke(c, record_stage, 0x103eb868, {manager, taken});
                invoke_native(c, ask_for_staff, 0x103eb878, {manager, taken, 0});
                invoke(c, set_stage, 0x103eb88c, {manager, taken, 10, 0});
                invoke(c, follow_up, 0x103eb898, {manager, taken});
                invoke(c, ready_offers, 0x103eb8a4, {manager, taken});
                const auto row = invoke(c, find_shortlist_row, 0x103eb8ec,
                                        {m988(), look.offer_club_of(taken), look.person_of(taken), flag_if_not_eleven(taken), 0});
                if (row != 0)
                    invoke(c, add_shortlist_row, 0x103eb924, {m988(), look.person_of(taken), look.offer_club_of(taken), 1, 0});
            }
        } else {
            invoke_native(c, ask_other_side, 0x103eb938, {manager, slot, 0});
        }
    } else {
        const auto blocked_now = std::int8_t(invoke(c, blocked_by, 0x103eb948, {manager, offer})) != 0;
        const bool regular = (!blocked_now && stage(offer) != 0xf && stage(offer) != 4) || stage(offer) == 0x13;
        if (!regular) {
            if (stage(offer) == 0xf && c.u8(offer + offer_wait) < 3) {
                c.put8(offer + offer_wait, 3);
                invoke(c, set_stage, 0x103eb9a8, {manager, offer, 10, 0});
            } else {
                invoke_native(c, blocked_stage, 0x103eb9b8, {manager, offer});
            }
        } else {
            const auto current = c.u8(offer + daily::offer_stage);
            const auto current_signed = std::int32_t(std::int8_t(current));
            if (current_signed == 1) {
                if (blocked(offer_club, 0x103eb9e4)) {
                    set(offer, 4);
                    invoke(c, record_stage, 0x103eba00, {manager, offer});
                    invoke(c, set_stage, 0x103eba14, {manager, offer, 10, 0});
                } else if (c.u8(c.u32(slot) + offer_kind) == 1) {
                    invoke_native(c, pending_offer, 0x103eba34, {manager, slot});
                } else {
                    invoke_native(c, reply_offer, 0x103eba48, {manager, slot, 0});
                }
            } else if (current_signed == 5 || current_signed == 6) {
                if (blocked(offer_club, 0x103eba74)) {
                    set(offer, 9);
                    invoke(c, set_stage, 0x103eba98, {manager, offer, 10, 0});
                } else {
                    c.put32(frame.at(0x44), offer);
                    const auto candidate = c.u32(look.people) + c.u32(offer + daily::offer_person) * person_size;
                    keep(c, 27, candidate);
                    settle_candidate(candidate, offer_club, frame.at(0x44), frame.at(0xcc), 0x103ebadc, 0x103ebbcc, 0x103ebb28,
                                     0x103ebb60, 0x103ebb74, 0x103ebba0, 0x103ebab8);
                }
            } else if (c.u32(offer + daily::offer_list) != 0 && current_signed == 7) {
                // (A stage of 5 never reaches this test: it was taken above.)
                if (blocked(person_club, 0x103ebc04)) {
                    set(offer, 8);
                    invoke(c, set_stage, 0x103ebc28, {manager, offer, 10, 0});
                } else {
                    const auto list = frame.at(0x48);
                    invoke(c, list_init, 0x103ebc34, {list});
                    invoke(c, list_copy, 0x103ebc40, {list, c.u32(offer + daily::offer_list)});
                    c.put32(frame.at(0x54), c.u32(frame.at(0x4c)));
                    for (;;) {
                        const auto entry = invoke(c, list_next, 0x103ebd98, {list});
                        if (entry == 0) break;
                        c.put32(frame.at(0x40), offer);
                        const auto candidate = c.u32(look.people) + c.u32(entry) * person_size;
                        keep(c, 29, candidate);
                        settle_candidate(candidate, person_club, frame.at(0x40), frame.at(0x68), 0x103ebc8c, 0x103ebd7c, 0x103ebcd8,
                                         0x103ebd10, 0x103ebd24, 0x103ebd50, 0x103ebc68);
                        if (std::int8_t(invoke(c, blocked_by, 0x103ebd88, {manager, offer})) != 0) break;
                    }
                    invoke(c, list_free, 0x103ebdac, {list, 0xffffffff});
                }
            } else if (current_signed == 0xa) {
                invoke_native(c, settle_offer, 0x103ebdcc, {manager, offer, 0});
            } else if (current_signed == 0xe) {
                invoke_native(c, decide_offer, 0x103ebde8, {manager, offer, 0});
            } else if (current_signed == 0x11 || current_signed == 0x12) {
                set(offer, 0);
                if (std::int8_t(c.u8(offer + offer_rival)) != 0) {
                    invoke_native(c, pending_stage, 0x103ebf28, {manager, offer});
                } else {
                    const auto found_again = look.find(look.person_of(offer), 0x103ebe28);
                    const auto club = found_again ? look.club_at(c.u32(found_again + 4)) : 0;
                    const auto news = c.u32(c.toc(-0x7bf0));
                    if (std::int8_t(invoke(c, news_check, 0x103ebe58, {news, club})) != 0) {
                        invoke_native(c, close_offer, 0x103ebf04, {manager, offer});
                        invoke(c, set_stage, 0x103ebf18, {manager, offer, 0x1c, 0});
                    } else {
                        const auto fee = invoke(c, offer_fee, 0x103ebe7c, {offer, 0, 0, 0, 0, 1});
                        c.put32(frame.at(0x38), 0);
                        const auto verdict = std::int8_t(invoke(c, shortlist_check, 0x103ebeb4,
                                                                {m988(), person_club, look.person_of(offer), std::uint32_t(std::int32_t(fee) >> 31),
                                                                 fee, offer_club, 0xffffffff, 0xffffffff}));
                        if (verdict == 0) invoke_native(c, first_contact, 0x103ebedc, {manager, slot, 0});
                        else if (verdict > 0 && verdict < 3) invoke_native(c, send_home, 0x103ebef4, {manager, slot, 0, 0});
                    }
                }
            }
        }
    }
    invoke(c, list_free, 0x103ebf34, {frame.at(0x58), 0xffffffff});
}
}
