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
    0x10426830u, 0x1042b3a0u, 0x10426970u, 0x10423040u, 0x1041e770u, 0x1041b600u};
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
        actual_reset_services = 0, destroy_revision_delta = 0;
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
    0x10738e60, 0x10738c20};

void trace(Cpu& c, std::uint32_t entry) {
    if (++call_count > 1000) throw std::runtime_error("fixture call limit");
    auto n = c.u32(trace_at);
    const auto put = [&](std::uint32_t value) { c.put32(trace_at + 4 + 4 * n++, value); };
    put(entry); put(c.lr());
    for (unsigned r = 3; r <= 10; ++r) put(c.r(r));
    for (unsigned r = 13; r <= 31; ++r) put(c.r(r));
    for (unsigned f = 0; f < 8; ++f) put(c.cr(f));
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
    if (std::getenv("CM_TRANSFER_SCREENS_TRACE") && active.entry == 0x1041b600)
        std::cerr << (original_mode ? "original " : "rewrite ") << hex_address(target)
                  << " lr=" << hex_address(c.lr()) << " r3=" << hex_address(c.r(3))
                  << " r4=" << hex_address(c.r(4)) << " r14=" << hex_address(c.r(14)) << '\n';
    if (target == 0x103eb440 && active.actual_query) return false;
    if ((target == 0x10009f20 && (active.actual_total_services & 1)) ||
        (target == 0x10738e00 && (active.actual_total_services & 2)) ||
        (target == 0x10416e10 && (active.actual_total_services & 4))) return false;
    switch (target) {
    case 0x1073e4b0: c.set(3, active.create_result); break;
    case 0x1073f1e0:
        if (c.r(4) >= 32) throw std::runtime_error("Invalid fixture history index");
        c.put32(history + c.r(4) * 4, c.r(5)); break;
    case 0x10082f20: c.set(3, active.contract_present ? contract : 0); break;
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
        c.set(3, 1); break;
    case 0x102cb570: c.set(3, c.u32(c.r(3) + 8 + c.r(4) * 4)); break;
    case 0x103eb440: c.set(3, link); break;
    case 0x10094ae0: c.set(3, 1); break;
    case 0x103ea350: c.set(3, active.mode); break;
    case 0x1073ef70: c.set(3, c.u32(history + c.r(4) * 4)); break;
    case 0x10775170: c.put32(data + 0x120, c.r(3)); break;
    case 0x102a54b0: c.set(3, active.command); break;
    case 0x102a83a0: c.set(3, active.fallback); break;
    case 0x10718360: c.set(3, active.mode); break;
    case 0x103e44b0: case 0x103e6290: c.set(3, 1); break;
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
        if (active.total_example) {
            const auto expected = active.total_example == 1 ? 6000000000ull : 12000ull;
            if (((std::uint64_t(c.r(5)) << 32) | c.r(6)) != expected)
                throw std::runtime_error("Actual monetary total disagrees with independent expected value");
        }
        c.put32(c.r(3), c.r(5)); c.put32(c.r(3) + 4, c.r(6)); break;
    case 0x10727890: c.put32(c.r(3), c.r(4)); break;
    case 0x10741da0: case 0x10742100: case 0x10741450: case 0x10740620: break;
    case 0x10148930: c.set(3, data + 0x6f00); break;
    case 0x10411f10:
        c.put32(data + 0x170, c.r(3));
        if (active.destroy_revision_delta) c.put32(history + 12, c.u32(history + 12) + active.destroy_revision_delta);
        break;
    case 0x103f4d10: c.put16(c.r(6), 0xff85); c.set(3, active.eligible); break;
    case 0x10103510: c.set(3, data + 0xa400); break;
    case 0x100f5770: c.set({{3, 0}, {4, std::uint32_t(active.board_limit)}}); break;
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
    const auto allocated_draft = make_offer(data + 0x7000), allocated_offer = make_offer(offer);
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
    if (test.entry == 0x1041e770 || test.entry == 0x1041b600) {
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
        if (test.entry == 0x1041b600) {
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
    }
    if (test.actual_reset_services) actual_heap_offers(c, test);
    c.s.budget = 100000;
}
void returned(State& s) {
    Cpu c(s);
    if (c.sp() != stack || c.lr() != sentinel || s.pc != sentinel)
        throw std::runtime_error("fixture did not return");
    if (active.actual_reset_services && (active.action == 26 || active.action == 32)) {
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
