#include "transfer_manager_offer_news.hpp"
#include "guest_values.hpp"
#include "transfer_manager_adapter.hpp"
#include "transfer_manager_candidate_clubs.hpp"
#include "transfer_manager_offer_contracts.hpp"
#include "transfer_manager_offer_state.hpp"
#include "transfer_manager_schedule.hpp"

namespace cm::aot::transfer_manager {
namespace {
using namespace guest;
constexpr std::uint32_t club_size = 0x248, staff_size = 0x70;
constexpr std::uint32_t contract_lookup = 0x10082f20, create_news = 0x102bdf90, add_argument = 0x102cb4b0,
    dispatch_news = 0x102cc140, candidate_clubs = 0x103fb840;
constexpr std::uint32_t response_news = 0x104024e0, termination_news = 0x10403250;

std::uint32_t call(Cpu& c, std::uint32_t entry, std::uint32_t back,
                   std::initializer_list<std::uint32_t> args = {}) {
    unsigned reg = 3;
    for (const auto arg : args) c.set(reg++, arg);
    c.call(entry, back);
    return c.r(3);
}

void owned_call(Cpu& c, std::uint32_t entry, std::uint32_t back, std::initializer_list<std::uint32_t> args,
                auto body) {
    unsigned reg = 3;
    for (const auto arg : args) c.set(reg++, arg);
    if (!transfer_manager_direct_calls_enabled()) {
        c.call(entry, back);
        return;
    }
    c.link(back);
    c.s.pc = entry;
    body();
    c.finish();
}

std::uint32_t find_person(Cpu& c, std::uint32_t offer) {
    const auto offset = c.u32(offer + 0xc) * staff_size;
    const auto people = c.u32(c.toc(-0x7be0));
    c.set({{0, offset}, {4, people}});
    return people + offset;
}

std::uint32_t seller_club(Cpu& c, std::uint32_t person, std::uint32_t cache, std::uint32_t clubs,
                         unsigned present_register, std::uint32_t back) {
    const auto contracts = c.u32(c.toc(-0x7b7c));
    const auto contract = call(c, contract_lookup, back, {contracts, person});
    compare_logical(c, contract);
    c.put32(cache, contract);
    bool selling = false;
    if (contract) {
        const auto type = c.i8(contract + 0x38);
        const auto class_id = type >> 2;
        c.s.wr<1>(reg::xer_ca, type < 0 && (std::uint8_t(type) & 2));
        c.set(0, std::uint32_t(class_id));
        selling = compare(c, class_id, 5) != 0;
    }
    c.set(present_register, selling);
    compare(c, selling);
    if (!selling) return 0;
    const auto offset = c.u32(c.u32(cache) + 4) * club_size;
    c.set(0, offset);
    return c.u32(clubs) + offset;
}

std::uint32_t recipients(Cpu& c, const Frame& frame, std::uint32_t manager, std::uint32_t offer,
                         std::uint32_t sources, std::uint32_t back) {
    const auto array = frame.at(0x38);
    owned_call(c, candidate_clubs, back, {manager, offer, sources, array, 30},
               [&] { collect_candidate_clubs(c, manager, offer, sources, array, 30); });
    return c.r(3);
}

struct OfferNews {
    Cpu& c;
    std::uint32_t address;

    void create(std::uint32_t category, std::uint32_t back) const {
        call(c, create_news, back, {address, category, 0});
    }

    void put(std::uint32_t index, std::uint32_t value, std::uint32_t back) const {
        call(c, add_argument, back, {address, index, value});
    }

    void person_details(std::uint32_t person, std::uint32_t back) const {
        put(0, c.u32(person), back);
        for (unsigned index = 1; index <= 3; ++index)
            put(index, c.u32(c.u32(person + 0x24 + index * 4) + 0x20), back + index * 0x14);
    }

    void dispatch(std::uint32_t clubs, std::uint32_t count, std::uint32_t array, std::uint32_t back,
                  unsigned cursor_reg, unsigned index_reg, unsigned count_reg) const {
        const auto size = std::int8_t(count);
        c.set(count_reg, std::uint32_t(size));
        for (std::uint32_t index = 0; ; ++index, array += 4) {
            c.set({{0, index}, {cursor_reg, array}, {index_reg, index}});
            if (compare(c, index, size) >= 0) return;
            call(c, dispatch_news, back, {c.u32(clubs), address, c.u32(array)});
        }
    }
};
}

void announce_offer_response(Cpu& c, std::uint32_t manager, std::uint32_t offer) {
    Frame frame(c, response_news, 0x1b0, {.first_gpr = 26});
    c.set(0, c.lr());
    Kept<26> seller(c, c.toc(-0x7b78));
    const Kept<27> selling(c, 0);
    Kept<28> transfers(c, manager);
    const Kept<29> record(c, offer);
    const Kept<31> recipients_slot(c, c.toc(-0x7bf0));
    const Kept<30> person(c, find_person(c, record));
    seller = seller_club(c, person, seller, c.toc(-0x7be8), 27, 0x10402528);
    transfers = recipients(c, frame, transfers, record, 0x2d, 0x10402590);
    const OfferNews news{c, frame.at(0xb0)};
    news.create(0xfa3, 0x104025a4);
    news.person_details(person, 0x104025b4);
    news.put(4, nonzero_logical(c, seller) ? c.u32(seller) : 0xffffffff, 0x10402610);
    news.put(5, c.u32(record + 8), 0x10402620);
    news.put(6, c.u32(record + 4), 0x10402630);
    news.put(7, c.u32(record + 0x28), 0x10402640);
    news.put(8, c.u32(record + 0x2c), 0x10402650);
    news.put(9, std::uint32_t(c.i8(record + 0x24)), 0x10402664);
    news.dispatch(recipients_slot, transfers, frame.at(0x38), 0x10402688, 28, 26, 29);
}

void announce_offer_termination(Cpu& c, std::uint32_t manager, std::uint32_t offer, std::uint32_t reason) {
    Frame frame(c, termination_news, 0x1c0, {.first_gpr = 24});
    c.set(0, c.lr());
    Kept<24> seller(c, c.toc(-0x7b78));
    const Kept<25> selling(c, 0);
    Kept<26> transfers(c, manager);
    const Kept<27> record(c, offer);
    const Kept<28> detail(c, reason);
    const Kept<30> recipients_slot(c, c.toc(-0x7bf0));
    const Kept<31> clubs(c, c.toc(-0x7be8));
    const Kept<29> person(c, find_person(c, record));
    seller = seller_club(c, person, seller, clubs, 25, 0x104032a0);
    transfers = recipients(c, frame, transfers, record, 0x2f, 0x10403304);
    const OfferNews news{c, frame.at(0xb0)};
    news.create(0xfae, 0x10403318);
    news.person_details(person, 0x10403328);
    news.put(4, nonzero_logical(c, seller) ? c.u32(seller) : 0xffffffff, 0x10403384);
    news.put(5, c.u32(record + 8), 0x10403394);
    news.put(6, std::uint32_t(c.i8(record + 0x24)), 0x104033a8);
    news.put(7, detail, 0x104033b8);
    if (nonzero_logical(c, seller)) {
        const auto representative = result(c, c.u32(seller + 0x90));
        if (nonzero_logical(c, representative)) news.put(8, c.u32(representative), 0x104033dc);
    }
    const auto buyer = c.i32(record + 8);
    c.set(0, std::uint32_t(buyer));
    if (compare(c, buyer, -1) > 0) {
        const auto offset = std::uint32_t(buyer) * club_size;
        c.set(0, offset);
        const auto representative = result(c, c.u32(c.u32(clubs) + offset + 0x90));
        if (nonzero_logical(c, representative)) news.put(9, c.u32(representative), 0x10403410);
    }
    news.dispatch(recipients_slot, transfers, frame.at(0x38), 0x10403430, 26, 24, 27);
}

void conclude_offer_response(Cpu& c, std::uint32_t manager, std::uint32_t offer_slot, std::uint32_t firm) {
    Frame frame(c, 0x103e4c90, 0x50, {.first_gpr = 29});
    c.set(0, c.lr());
    const Kept<29> transfers(c, manager);
    const Kept<30> slot(c, offer_slot);
    const Field<std::uint32_t> current(c, slot);
    const auto confirmed = std::int8_t(firm);
    c.set(0, std::uint32_t(confirmed));
    if (nonzero(c, confirmed)) {
        const auto record = result(c, current);
        c.set(0, 1);
        c.put8(record + 0x35, 1);
    }
    const Kept<31> terminate(c, call(c, 0x1007f610, 0x103e4cd4, {c.u32(c.toc(-0x7b7c)), current}));
    c.set(0, 8);
    c.put8(current + 0x34, 8);
    owned_call(c, response_news, 0x103e4cf0, {transfers, current},
               [&] { announce_offer_response(c, transfers, c.r(4)); });
    const auto decision = std::int8_t(std::uint32_t(terminate));
    c.set(0, std::uint32_t(decision));
    if (!nonzero(c, decision)) return;
    owned_call(c, termination_news, 0x103e4d08, {transfers, current, 0},
               [&] { announce_offer_termination(c, transfers, c.r(4), 0); });
    c.set({{0, 3}, {5, 15}});
    c.put8(current + 0x34, 15);
    c.put8(current + 0x36, 3);
    owned_call(c, 0x103ec4f0, 0x103e4d2c, {transfers, current},
               [&] { forget_offer_approaches(c, transfers, c.r(4)); });
    owned_call(c, 0x103eb4a0, 0x103e4d40, {transfers, current, 10, 0},
               [&] { schedule_transfer_offer(c, transfers, c.r(4), 10, 0); });
    owned_call(c, 0x103f6190, 0x103e4d4c, {transfers, current},
               [&] { clear_inactive_offer_flag(c, transfers, c.r(4)); });
}
}
