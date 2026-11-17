// Cases for transfer_manager_native_terms*.cpp: 0x103f89e0, 0x104058e0, 0x103f9d60, 0x103f4fd0, 0x103f45a0.
#include "transfer_manager_native_checks.hpp"
#include "guest_abi.hpp"
#include <string>

namespace cm::aot::transfer_manager_native_checks {
namespace {
using Writes = std::vector<Write>;
constexpr std::uint32_t toc_value = 0x108bba90;
struct RealOffer { std::uint32_t index, address; };
std::vector<RealOffer> real_offers(const Scenario& scenario, unsigned wanted) {
    std::vector<RealOffer> offers;
    const auto manager = scenario.transfer_manager();
    const auto array = scenario.r32(manager), size = scenario.r32(manager + 4);
    if (size == 0 || size > 100000) return offers;
    const auto step = size / wanted + 1;
    for (std::uint32_t i = 0; i < size; i += step)
        if (const auto offer = scenario.r32(array + i * 4)) offers.push_back({i, offer});
    return offers;
}
std::string tag(const char* what, unsigned a, unsigned b = 0, unsigned c = 0) {
    return std::string(what) + " " + std::to_string(a) + "/" + std::to_string(b) + "/" + std::to_string(c);
}
// A person with a contract at a club (kind byte not 5), and one without any contract.
std::uint32_t person_with_club(const Scenario& scenario, unsigned from, std::uint32_t& contract) {
    for (unsigned k = 0; k < scenario.people(); ++k) {
        const auto index = (from + k * 7) % scenario.people();
        const auto found = scenario.contract_of(scenario.person(index));
        if (found && (std::int8_t(scenario.s.memory.read<1>(found + 0x38)) >> 2) != 5) { contract = found; return index; }
    }
    return 0;
}
std::uint32_t person_without_contract(const Scenario& scenario, unsigned from) {
    for (unsigned k = 0; k < scenario.people(); ++k) {
        const auto index = (from + k * 5) % scenario.people();
        if (!scenario.contract_of(scenario.person(index))) return index;
    }
    return 0;
}
}

TRANSFER_MANAGER_NATIVE_CASES("terms") {
    if (scenario.match) return;
    const auto manager = scenario.transfer_manager();
    const auto offers = real_offers(scenario, 24);
    if (offers.size() < 6 || scenario.people() == 0) return;
    const auto date = scenario.toc(-0x7b80);
    const auto out = scratch_base + 0x1000, code = scratch_base + 0x1100, rows_out = scratch_base + 0x1200;
    const auto flags = [&](std::size_t k) { return std::uint32_t(k % 3 == 0 ? 0x100 : k % 3 == 1 ? 0 : 1); };
    for (std::size_t k = 0; k < offers.size(); ++k) {
        const auto offer = offers[k].address;
        const auto buyer_index = scenario.r32(offer + 8);
        const auto buyer = scenario.club(buyer_index);
        const auto person_index = scenario.r32(offer + 0xc);
        std::uint32_t contract = 0;
        const auto held_index = person_with_club(scenario, unsigned(k) * 13, contract);
        const auto held_club = contract ? scenario.club(scenario.r32(contract + 4)) : 0;
        const auto no_contract = person_without_contract(scenario, unsigned(k));
        const auto kind_byte = std::uint32_t(scenario.s.memory.read<1>(offer + 0x38));
        const auto role = scenario.r32(offer + 0x10) & 0xff;
        (void)role;
        const std::uint32_t own_clubs[] = {buyer, held_club, scenario.club((buyer_index + 3) % 20), 0};
        // 0x103f89e0 and 0x104058e0: every kind of date, the default date before, equal to and after the contract's.
        for (std::uint32_t nibble = 0; nibble < 6; ++nibble) {
            Writes base = {{1, offer + 0x38, (nibble << 4) | (kind_byte & 0xf)}};
            for (unsigned variant = 0; variant < 4; ++variant) {
                Writes writes = base;
                if (variant == 1) writes.push_back({4, offer + 0xc, no_contract});
                if (variant == 2) { writes.push_back({4, offer + 0xc, held_index}); writes.push_back({1, contract + 0x38, 0x14}); }
                if (variant == 3) {
                    writes.push_back({4, offer + 0xc, held_index});
                    writes.push_back({2, date, (k % 2) ? 0x7fffu : 0u});
                    writes.push_back({2, date + 2, (k % 3) ? 0xffffu : 0u});
                }
                cases.push_back({.entry = 0x103f89e0, .registers = {{2, toc_value}, {3, manager}, {4, offer}, {5, out}}, .memory = writes,
                                 .label = tag("term date", unsigned(k), nibble, variant)});
                for (const std::uint32_t status : {12u, 10u}) {
                    Writes news = writes;
                    news.push_back({1, offer + 0x34, status});
                    news.push_back({1, offer + 0x37, (k % 2) ? 1u : 0u});
                    cases.push_back({.entry = 0x104058e0, .registers = {{2, toc_value}, {3, manager}, {4, offer}}, .memory = news,
                                     .label = tag("term news", unsigned(k), nibble * 4 + variant, status)});
                    cases.push_back({.entry = 0x104058e0, .registers = {{2, toc_value}, {3, manager}, {4, offer}}, .memory = news,
                                     .label = tag("term news wanted", unsigned(k), nibble * 4 + variant, status),
                                     .answers = {{0x102cdbf0, 1}}});
                }
            }
        }
        // 0x103f9d60: the terms check for the buying club, the person's club, another club and none.
        for (unsigned c = 0; c < 4; ++c)
            for (std::uint32_t nibble : {0u, 1u, 3u, 5u}) {
                Writes writes = {{1, offer + 0x38, (nibble << 4) | (kind_byte & 0xf)}, {1, offer + 0x34, (k + c) % 4 == 0 ? 0x13u : 0xau}};
                cases.push_back({.entry = 0x103f9d60,
                                 .registers = {{2, toc_value}, {3, manager}, {4, offer}, {5, own_clubs[c]}, {6, code}, {7, flags(k + c)}},
                                 .memory = writes, .label = tag("terms check", unsigned(k), c, nibble)});
                cases.push_back({.entry = 0x103f9d60,
                                 .registers = {{2, toc_value}, {3, manager}, {4, offer}, {5, own_clubs[c]}, {6, 0}, {7, 1}},
                                 .memory = writes, .label = tag("terms check bare", unsigned(k), c, nibble)});
            }
        // 0x103f4fd0: room in the squad for the buying club, the person's club and another one.
        for (unsigned c = 0; c < 4; ++c)
            for (const std::uint32_t trim : {0u, 1u}) {
                Writes writes;
                if (c == 3) writes.push_back({4, offer + 4, 0});
                cases.push_back({.entry = 0x103f4fd0,
                                 .registers = {{2, toc_value}, {3, manager}, {4, own_clubs[c]}, {5, offer}, {6, trim}, {7, (k % 2) ? code : 0}},
                                 .memory = writes, .label = tag("squad room", unsigned(k), c, trim)});
            }
        // 0x103f45a0: the rows of a club's offers.
        for (unsigned c = 0; c < 3; ++c)
            cases.push_back({.entry = 0x103f45a0, .registers = {{2, toc_value}, {3, manager}, {4, own_clubs[c]}, {5, rows_out}},
                             .label = tag("offer rows", unsigned(k), c)});
        (void)person_index;
    }
}
}
