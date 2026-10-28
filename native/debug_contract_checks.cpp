#include "match_debugger_factory.hpp"
#include "match_debugging.hpp"
#include <algorithm>
#include <array>
#include <chrono>
#include <iostream>
#include <stdexcept>
#include <vector>

namespace {
using namespace cm::aot;
using namespace cm::aot::match_debugger;
void require(bool value, const char* reason) { if (!value) throw std::runtime_error(reason); }
std::uint32_t invoke(State& s, std::uint32_t entry, std::initializer_list<std::uint32_t> args) {
    unsigned reg = 3;
    for (auto value : args) s.wr<4>(reg++ * 4, value);
    s.wr<4>(4128, return_sentinel); s.pc = entry; s.budget = s.executed + 500000000;
    try { run_until(s, return_sentinel); }
    catch (const std::exception& e) { throw std::runtime_error(hex_address(entry) + " at " + hex_address(s.pc) + ": " + e.what()); }
    return std::uint32_t(s.r<4>(12));
}
struct Offer { std::uint32_t index, revision, club, address; };
}

int main(int argc, char** argv) {
    try {
        require(argc == 4, "Expected original program, game and private root");
        const auto root = std::filesystem::path(argv[3]) / std::to_string(std::chrono::steady_clock::now().time_since_epoch().count());
        auto live = create_match({argv[1], argv[2], root}, {677, 5677, 12345, 2001, 194});
        auto& s = *live.state;
        const auto toc = std::uint32_t(s.r<4>(8));
        for (const auto spec : std::array<std::array<unsigned, 3>, 4>{{
                 {0x7bf0, 0x20, 0x102cb930}, {0x7b40, 0xa7c, 0x103df0d0},
                 {0x7ab8, 4, 0x100f84e0}, {0x7b24, 0x10, 0x100031b0}}}) {
            const auto slot = std::uint32_t(s.memory.read<4>(toc - spec[0]));
            if (!s.memory.read<4>(slot)) {
                const auto object = invoke(s, 0x10773520, {spec[1]});
                invoke(s, spec[2], {object, 0}); s.memory.write<4>(slot, object);
            }
        }
        const auto club = std::uint32_t(s.memory.read<4>(live.match + 0x7aac + 0x34));
        const auto staff = std::uint32_t(s.memory.read<4>(club + 0x98));
        const auto manager = std::uint32_t(s.memory.read<4>(s.memory.read<4>(toc - 0x7b7c)));
        const auto transfer = std::uint32_t(s.memory.read<4>(s.memory.read<4>(toc - 0x7b40)));
        require(staff && manager && transfer, "Missing original contract fixture");
        const auto value = invoke(s, 0x10084be0, {manager, staff, club, 1, 0xffffffff, 0});
        if (!invoke(s, 0x10082f20, {manager, staff})) invoke(s, 0x10078e70, {manager, staff, value});
        const auto contract = invoke(s, 0x10082f20, {manager, staff});
        require(contract && s.memory.read<4>(contract + 4) == s.memory.read<4>(club), "Original primary contract setup");
        for (auto date : {contract + 0x30, staff + 0x54}) {
            s.memory.write<2>(date, 150); s.memory.write<2>(date + 2, 2004); s.memory.write<1>(date + 4, 1);
        }
        const auto today = std::uint32_t(s.memory.read<4>(toc - 0x7b80));
        s.memory.write<2>(today, 100); s.memory.write<2>(today + 2, 2004); s.memory.write<1>(today + 4, 1);
        const auto staff_count = std::uint32_t(s.memory.read<4>(s.memory.read<4>(toc - 0x7be4)));
        const auto staff_base = std::uint32_t(s.memory.read<4>(s.memory.read<4>(toc - 0x7be0)));
        std::vector<std::uint32_t> buyers;
        for (unsigned n = 0; n < staff_count && buyers.size() < 9; ++n) {
            const auto candidate = std::uint32_t(s.memory.read<4>(staff_base + n * 0x70 + 0x24));
            if (candidate && candidate != club && std::find(buyers.begin(), buyers.end(), candidate) == buyers.end()) buyers.push_back(candidate);
        }
        require(buyers.size() == 9, "Need distinct actual buyer clubs");
        const auto scratch = invoke(s, 0x10773520, {0x200});
        const auto original_direct = invoke(s, 0x10773520, {0x54});
        invoke(s, 0x10411e60, {original_direct});
        invoke(s, 0x10411ea0, {original_direct, staff, buyers[0], 0, 0xffffffff, 1});
        require(s.memory.read<1>(original_direct + 0x37) == 1, "Original direct constructor marker");
        const auto make_offer = [&](std::uint32_t buyer, unsigned direct, unsigned timing, unsigned loan) {
            const auto input = invoke(s, 0x10773520, {0x54});
            invoke(s, 0x10411e60, {input}); invoke(s, 0x104164f0, {input, staff, buyer, 10000});
            require(s.memory.read<1>(input + 0x37) == 0, "Original purchase constructor marker");
            s.memory.write<1>(input + 0x37, direct); s.memory.write<1>(input + 0x38, timing << 4);
            s.memory.write<1>(input + 0x24, loan);
            if (direct) s.memory.write<4>(input + 0x20, 0);
            invoke(s, 0x103e25d0, {transfer, input, 0});
            const auto index = std::uint32_t(s.memory.read<4>(input + 0x28));
            const auto revision = std::uint32_t(s.memory.read<4>(input + 0x2c));
            const auto published = std::uint32_t(s.memory.read<4>(s.memory.read<4>(transfer) + index * 4));
            require(published && s.memory.read<4>(published + 12) == s.memory.read<4>(staff), "Offer registration");
            s.memory.copy_in(scratch, s.memory.copy_out(contract, 100));
            s.memory.write<4>(scratch + 4, s.memory.read<4>(buyer));
            invoke(s, 0x1007f200, {manager, scratch, index, revision, 11, 0, 0, 0});
            return Offer{index, revision, std::uint32_t(s.memory.read<4>(buyer)), published};
        };
        const auto direct = make_offer(buyers[0], 1, 3, 0);
        const auto paid = make_offer(buyers[1], 0, 3, 0);
        const auto loan = make_offer(buyers[2], 1, 3, 1);
        const auto immediate = make_offer(buyers[3], 1, 5, 0);
        const auto window = make_offer(buyers[4], 1, 4, 0);
        const auto deferred = make_offer(buyers[5], 1, 3, 0);
        const auto chosen = make_offer(buyers[6], 1, 3, 0);
        const auto orphan = make_offer(buyers[7], 1, 3, 0);
        const auto accepted = make_offer(buyers[8], 1, 3, 0);
        s.memory.write<1>(accepted.address + 0x10, 2);
        invoke(s, 0x1007f840, {manager, staff, buyers[8]});
        s.memory.write<4>(chosen.address + 0x20, 10000); // A direct agreement can include compensation.
        const auto person_id = std::uint32_t(s.memory.read<4>(staff));
        const auto chosen_row = invoke(s, 0x1007f720, {manager, person_id, chosen.club});
        require(chosen_row, "Missing selected contract fixture");
        s.memory.write<1>(chosen_row + 0x64, 2); s.memory.write<1>(chosen.address + 0x10, 2);
        invoke(s, 0x103ea250, {transfer, staff});
        s.memory.write<1>(contract + 0x54, 1);
        s.memory.write<2>(scratch + 0x180, 151); s.memory.write<2>(scratch + 0x182, 2004);
        s.memory.write<1>(scratch + 0x184, 1);
        invoke(s, 0x103f2ec0, {transfer, chosen.address, scratch + 0x180});
        invoke(s, 0x1007f840, {manager, staff, buyers[7]});
        invoke(s, 0x103f2ec0, {transfer, orphan.address, scratch + 0x180});
        invoke(s, 0x103f2ec0, {transfer, paid.address, scratch + 0x180});
        require(s.memory.read<1>(chosen.address + 0x34) == 19 && s.memory.read<4>(transfer + 0xa58) == 3,
                "Original scheduled transfer setup");
        require(s.memory.read<4>(contract + 0x58) == paid.index &&
                invoke(s, 0x103f43b0, {transfer, staff, 0}), "Scheduled transfer query setup");
        const auto deferred_row = invoke(s, 0x1007f720, {manager, person_id, deferred.club});
        require(deferred_row, "Missing deferred contract fixture");
        s.memory.copy_in(scratch + 0x80, s.memory.copy_out(deferred_row, 136));
        auto deferred_list = std::uint32_t(s.memory.read<4>(manager + 0x38));
        if (!deferred_list) {
            deferred_list = invoke(s, 0x10773520, {16}); invoke(s, 0x10738c00, {deferred_list});
            s.memory.write<4>(manager + 0x38, deferred_list);
        }
        invoke(s, 0x10738d10, {deferred_list, scratch + 0x80, 136});
        const auto paid_row = invoke(s, 0x1007f720, {manager, person_id, paid.club});
        require(paid_row, "Missing paid contract fixture");
        s.memory.write<1>(paid_row + 0x64, 2);
        s.memory.copy_in(scratch + 0x80, s.memory.copy_out(paid_row, 136));
        invoke(s, 0x10738d10, {deferred_list, scratch + 0x80, 136});
        invoke(s, 0x1007f840, {manager, staff, buyers[5]});
        s.memory.write<1>(deferred.address + 0x34, 15);
        s.memory.copy_in(scratch, s.memory.copy_out(contract, 100));
        invoke(s, 0x1007f200, {manager, scratch, 0xffffffff, 0xffffffff, 11, 0, 0, 0});
        require(invoke(s, 0x1007f720, {manager, person_id, std::uint32_t(s.memory.read<4>(club))}), "Missing internal renewal fixture");
        const auto before = s.memory.copy_out(contract, 100);
        initialize_native_debugging(false);
        require(!extend_debug_contract(s, staff) && s.memory.copy_out(contract, 100) == before, "Disabled debugging changed contract");
        select_native_debugging(true);
        require(!extend_debug_contract(s, 0), "Missing player accepted");
        s.memory.write<2>(contract + 0x32, 0);
        require(!extend_debug_contract(s, staff) && s.memory.read<4>(s.memory.read<4>(transfer) + direct.index * 4) == direct.address,
                "Invalid date changed a proposal");
        s.memory.write<2>(contract + 0x32, 2004);
        const auto registers = s.registers; const auto unique = s.unique; const auto pc = s.pc;
        require(extend_debug_contract(s, staff), "Contract extension failed");
        require(s.registers == registers && s.unique == unique && s.pc == pc, "Contract extension clobbered caller");
        const auto slot = [&](Offer offer) { return std::uint32_t(s.memory.read<4>(s.memory.read<4>(transfer) + offer.index * 4)); };
        require(!slot(direct) && !slot(window) && !slot(deferred) && s.memory.read<2>(deferred_list) == 1,
                "Direct pending or deferred pre-contracts not removed");
        require(!slot(chosen) && !slot(orphan) && !slot(accepted) && s.memory.read<4>(transfer + 0xa58) == 1 &&
                s.memory.read<4>(s.memory.read<4>(transfer + 0xa50) + 8) == paid.index,
                "Accepted pre-contract or future transfer was not cancelled");
        const auto flags = std::uint32_t(s.memory.read<4>(transfer + 0x9a0)) + person_id * 10;
        require((s.memory.read<1>(flags) & 0x50) == 0x50,
                "Scheduled purchase or selected-contract flags are inconsistent");
        require(slot(paid) == paid.address && slot(loan) == loan.address && slot(immediate) == immediate.address,
                "Unrelated offer removed");
        const auto after = s.memory.copy_out(contract, 100);
        for (unsigned n = 0; n < 100; ++n) if (n < 0x30 || n > 0x34)
            require(before[n] == after[n], "Contract terms changed");
        require(s.memory.read<2>(contract + 0x30) == 149 && s.memory.read<2>(contract + 0x32) == 2005 &&
                s.memory.read<1>(contract + 0x34) == 0 &&
                s.memory.copy_out(staff + 0x54, 5) == s.memory.copy_out(contract + 0x30, 5), "Incorrect expiry or staff copy");
        for (const auto offer : {paid, loan, immediate})
            require(invoke(s, 0x1007f720, {manager, std::uint32_t(s.memory.read<4>(staff)), offer.club}), "Unrelated pending contract removed");
        require(invoke(s, 0x1007f720, {manager, person_id, std::uint32_t(s.memory.read<4>(club))}), "Internal renewal removed");
        s.memory.write<4>(deferred_list + 12, s.memory.read<4>(deferred_list + 4));
        const auto remaining = invoke(s, 0x10738e00, {deferred_list});
        require(remaining && s.memory.read<4>(remaining + 0x70) == paid.index, "Unrelated deferred contract removed");
        invoke(s, 0x103f3540, {transfer, paid.address});
        invoke(s, 0x103fb630, {transfer, paid.address});
        const auto final_agreement = make_offer(buyers[7], 1, 3, 0);
        const auto final_row = invoke(s, 0x1007f720, {manager, person_id, final_agreement.club});
        require(final_row, "Missing final agreement");
        s.memory.write<1>(final_row + 0x64, 2);
        invoke(s, 0x103ea250, {transfer, staff});
        invoke(s, 0x103f2ec0, {transfer, final_agreement.address, scratch + 0x180});
        require(extend_debug_contract(s, staff) && s.memory.read<2>(contract + 0x32) == 2006 &&
                s.memory.copy_out(staff + 0x54, 5) == s.memory.copy_out(contract + 0x30, 5), "Repeated extension failed");
        require(!slot(final_agreement) && !s.memory.read<4>(transfer + 0xa58) &&
                !(s.memory.read<1>(flags) & 0x50), "Last accepted agreement left transfer flags behind");
        require(!invoke(s, 0x103f43b0, {transfer, staff, 0}), "Cancelled transfer remains visible to the original query");
        std::cout << "Contract extension: original services, pending and accepted pre-contracts, scheduled purchase, loan, gating and terms passed\n";
    } catch (const std::exception& e) { std::cerr << e.what() << '\n'; return 1; }
}
