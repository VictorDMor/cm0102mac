#include "match_debugger_factory.hpp"
#include "match_debugging.hpp"
#include "debug_player_attributes.hpp"
#include "debug_attribute_lock.hpp"
#include "debug_attribute_persistence.hpp"
#include "platform.hpp"
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
std::vector<std::uint8_t> random_state(const State& s) {
    auto result = s.memory.copy_out(0x108b3f00, 16);
    for (auto global : {0x108b3f00u, 0x108b3f08u, 0x108b3f0cu}) {
        const auto bytes = s.memory.copy_out(s.memory.read<4>(global), 4);
        result.insert(result.end(), bytes.begin(), bytes.end());
    }
    const auto samples = s.memory.copy_out(s.memory.read<4>(0x108b3f04), 51000 * 2);
    result.insert(result.end(), samples.begin(), samples.end());
    return result;
}
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
        invoke(s, 0x10374af0, {contract, 1});
        invoke(s, 0x10380700, {contract, 1234567});
        s.memory.write<1>(contract + 0x1c, 1); s.memory.write<1>(contract + 0x1f, 1);
        require(invoke(s, 0x10380520, {contract}), "Minimum-fee release clause not active");
        const auto before = s.memory.copy_out(contract, 100);
        initialize_native_debugging(false);
        const auto disabled_registers = s.registers; const auto disabled_unique = s.unique; const auto disabled_pc = s.pc;
        append_debug_contract_action(s);
        require(!dispatch_debug_contract_action(s) && s.registers == disabled_registers &&
                s.unique == disabled_unique && s.pc == disabled_pc, "Disabled debugging exposed player actions");
        require(!extend_debug_contract(s, staff) && s.memory.copy_out(contract, 100) == before, "Disabled debugging changed contract");
        require(!remove_debug_sale_clause(s, staff) && s.memory.copy_out(contract, 100) == before,
                "Disabled debugging removed the sale clause");
        require(!cancel_scheduled_sale(s, staff) && s.memory.read<4>(transfer + 0xa58) == 3,
                "Disabled debugging cancelled a scheduled sale");
        select_native_debugging(true);
        const auto clause_registers = s.registers; const auto clause_unique = s.unique; const auto clause_pc = s.pc;
        require(remove_debug_sale_clause(s, staff), "Removing the sale clause failed");
        require(s.registers == clause_registers && s.unique == clause_unique && s.pc == clause_pc,
                "Sale clause action clobbered its caller");
        const auto cleared = s.memory.copy_out(contract, 100);
        for (unsigned n = 0; n < 100; ++n) if (n != 0x1d)
            require(cleared[n] == before[n], "Removing the sale clause changed another contract term");
        require(cleared[0x1d] == 0 && !invoke(s, 0x10380520, {contract}), "Sale release clause remains active");
        require(!remove_debug_sale_clause(s, staff) && !remove_debug_sale_clause(s, 0), "Missing clause or player accepted");
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
            require(cleared[n] == after[n], "Contract terms changed");
        require(s.memory.read<2>(contract + 0x30) == 149 && s.memory.read<2>(contract + 0x32) == 2005 &&
                s.memory.read<1>(contract + 0x34) == 0 &&
                s.memory.copy_out(staff + 0x54, 5) == s.memory.copy_out(contract + 0x30, 5), "Incorrect expiry or staff copy");
        for (const auto offer : {paid, loan, immediate})
            require(invoke(s, 0x1007f720, {manager, std::uint32_t(s.memory.read<4>(staff)), offer.club}), "Unrelated pending contract removed");
        require(invoke(s, 0x1007f720, {manager, person_id, std::uint32_t(s.memory.read<4>(club))}), "Internal renewal removed");
        s.memory.write<4>(deferred_list + 12, s.memory.read<4>(deferred_list + 4));
        const auto remaining = invoke(s, 0x10738e00, {deferred_list});
        require(remaining && s.memory.read<4>(remaining + 0x70) == paid.index, "Unrelated deferred contract removed");
        const auto contract_before_sale = s.memory.copy_out(contract, 100);
        const auto staff_before_sale = s.memory.copy_out(staff, 0x70);
        s.memory.write<1>(immediate.address + 0x10, 2); // Accepted terms alone do not establish a scheduled sale.
        require(cancel_scheduled_sale(s, staff), "Scheduled purchase sale cancellation failed");
        require(slot(paid) == paid.address && s.memory.read<1>(paid.address + 0x34) == 16 &&
                !s.memory.read<4>(transfer + 0xa58) && !invoke(s, 0x103f43b0, {transfer, staff, 0}),
                "Cancelled sale remains scheduled or its history was lost");
        require(!invoke(s, 0x1007f720, {manager, person_id, paid.club}) && !s.memory.read<2>(deferred_list),
                "Cancelled sale left a pending or deferred contract");
        require(s.memory.copy_out(contract, 100) == contract_before_sale && s.memory.copy_out(staff, 0x70) == staff_before_sale &&
                slot(loan) == loan.address && slot(immediate) == immediate.address && s.memory.read<1>(immediate.address + 0x10) == 2,
                "Cancellation changed the current contract, player or unrelated offers");
        require(!cancel_scheduled_sale(s, staff), "A second cancellation accepted an unscheduled offer");
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
        std::uint32_t exchanged = 0;
        for (std::uint32_t n = 0; n < staff_count; ++n) {
            const auto candidate = staff_base + n * 0x70;
            if (s.memory.read<4>(candidate + 0x24) == buyers[1] && s.memory.read<4>(candidate + 0x14)) { exchanged = candidate; break; }
        }
        require(exchanged, "Missing exchange player");
        const auto other_value = invoke(s, 0x10084be0, {manager, exchanged, buyers[1], 1, 0xffffffff, 0});
        if (!invoke(s, 0x10082f20, {manager, exchanged})) invoke(s, 0x10078e70, {manager, exchanged, other_value});
        const auto other_contract = invoke(s, 0x10082f20, {manager, exchanged});
        require(other_contract && s.memory.read<4>(other_contract + 4) == s.memory.read<4>(buyers[1]), "Exchange contract setup");
        const auto exchange = make_offer(buyers[1], 0, 4, 0);
        const auto exchange_list = invoke(s, 0x10773520, {16});
        invoke(s, 0x10738c00, {exchange_list});
        s.memory.write<4>(scratch + 0x100, s.memory.read<4>(exchanged)); s.memory.write<4>(scratch + 0x104, 2);
        invoke(s, 0x10738d10, {exchange_list, scratch + 0x100, 8});
        s.memory.write<4>(exchange.address + 4, exchange_list);
        invoke(s, 0x103f2ec0, {transfer, exchange.address, scratch + 0x180});
        invoke(s, 0x103f2ec0, {transfer, loan.address, scratch + 0x180});
        require(s.memory.read<4>(transfer + 0xa58) == 2 && s.memory.read<1>(exchange.address + 0x34) == 19,
                "Scheduled exchange setup");
        const auto club_before = s.memory.copy_out(club, 0x248);
        const auto buyer_before = s.memory.copy_out(buyers[1], 0x248);
        const auto main_before = s.memory.copy_out(contract, 100);
        const auto other_before = s.memory.copy_out(other_contract, 100);
        const auto random_before = random_state(s);
        require(cancel_scheduled_sale(s, exchanged), "Exchange player's sale cancellation failed");
        require(s.memory.read<1>(exchange.address + 0x34) == 16 && s.memory.read<4>(transfer + 0xa58) == 1 &&
                !(s.memory.read<1>(std::uint32_t(s.memory.read<4>(transfer + 0x9a0)) + s.memory.read<4>(exchanged) * 10) & 0x10) &&
                invoke(s, 0x103f43b0, {transfer, staff, 0}),
                "Exchange cancellation or unrelated scheduled loan flags incorrect");
        require(s.memory.copy_out(contract, 100) == main_before && s.memory.copy_out(other_contract, 100) == other_before &&
                s.memory.copy_out(club, 0x248) == club_before && s.memory.copy_out(buyers[1], 0x248) == buyer_before &&
                random_state(s) == random_before && slot(immediate) == immediate.address,
                "Exchange cancellation changed club records, contracts, RNG or an open offer");
        s.memory.write<4>(scratch, exchange.address);
        invoke(s, 0x103eb650, {transfer, scratch});
        require(!slot(exchange) && s.memory.read<4>(transfer + 0xa58) == 1 &&
                s.memory.copy_out(contract, 100) == main_before && s.memory.copy_out(other_contract, 100) == other_before,
                "Terminal sale collection changed the scheduled loan or current contracts");
        // Exercise the real profile getter, including its goalkeeper-specific
        // conversion, for the proposed maximum values at several abilities.
        const auto player = std::uint32_t(s.memory.read<4>(staff + 0x14));
        require(player != 0, "Missing player profile");
        debug_player_attributes::for_each(staff, player, [&](auto address, auto value) { s.memory.write<1>(address, value); });
        for (unsigned ability : {0u, 1u, 100u, 200u}) for (unsigned goalkeeper : {0u, 15u, 20u}) {
            s.memory.write<2>(player + 4, ability);
            s.memory.write<1>(player + 0xf, goalkeeper);
            for (unsigned attribute = 1; attribute <= 31; ++attribute)
                require(invoke(s, 0x100d2330, {staff, player, attribute}) == 20,
                        "Original player profile did not display a maximum attribute");
        }
        DebugAttributeLock locks;
        locks.add(staff, player);
        std::array<std::uint8_t, 8> overlapping{};
        locks.apply(player + 0x22, overlapping.data(), overlapping.size());
        require(overlapping == std::array<std::uint8_t, 8>{20, 100, 100, 1, 100, 100, 20, 20},
                "Lock did not protect a mixed multi-byte player write");
        std::array<std::uint8_t, 4> staff_write{3, 4, 5, 6};
        locks.apply(staff + 0x60, staff_write.data(), staff_write.size());
        require(staff_write == std::array<std::uint8_t, 4>{3, 20, 5, 20},
                "Lock changed the player's job or another unprotected staff byte");
        overlapping.fill(0);
        locks.remove(staff);
        locks.apply(player + 0x22, overlapping.data(), overlapping.size());
        require(locks.empty() && overlapping == std::array<std::uint8_t, 8>{},
                "Removing a lock continued to alter writes");
        const auto neighbor = std::uint32_t(s.memory.read<4>(exchanged + 0x14));
        const auto neighbor_before = s.memory.copy_out(neighbor, 0x48);
        s.memory.lock_debug_player_attributes(staff, player);
        invoke(s, 0x100c2cb0, {staff});
        debug_player_attributes::for_each(staff, player, [&](auto address, auto value) {
            require(s.memory.read<1>(address) == value, "Original normalization changed a locked attribute");
        });
        const auto lock_random = random_state(s);
        s.memory.write<4>(player + 0x23, 0);
        require(s.memory.read<4>(player + 0x23) == 0x64640164, "Scalar write bypassed player lock");
        s.memory.copy_in(player + 0x2c, std::vector<std::uint8_t>(8));
        require(s.memory.copy_out(player + 0x2c, 8) == std::vector<std::uint8_t>{20, 1, 20, 20, 20, 100, 100, 100},
                "Block write bypassed player lock");
        Vector128 zero{};
        s.memory.write_vector(player + 0x30, zero);
        s.memory.clear(player + 0x1b, 0x2a);
        debug_player_attributes::for_each(staff, player, [&](auto address, auto value) {
            require(s.memory.read<1>(address) == value, "Locked value changed after vector write or clear");
        });
        require(s.memory.read<2>(player + 4) == 200 && s.memory.read<1>(player + 0xf) == 20 &&
                s.memory.copy_out(neighbor, 0x48) == neighbor_before && random_state(s) == lock_random,
                "Lock changed ability, positions, a different player or RNG");
        s.memory.unlock_debug_player_attributes(staff);
        s.memory.write<1>(player + 0x25, 3);
        require(s.memory.read<1>(player + 0x25) == 3, "Unlocked record remained protected");
        const auto career = root / "lock-fixture.sav", other_career = root / "other-fixture.sav";
        const auto lock_directory = root / "lock-metadata";
        { std::ofstream out(career, std::ios::binary); out << "isolated save fixture"; }
        std::filesystem::copy_file(career, other_career);
        s.memory.lock_debug_player_attributes(staff, player);
        debug_attribute_persistence::save(s, lock_directory, career);
        s.memory.clear_debug_player_attributes();
        s.memory.write<1>(player + 0x25, 3);
        debug_attribute_persistence::load(s, lock_directory, other_career);
        require(s.memory.debug_locked_players().empty() && s.memory.read<1>(player + 0x25) == 3,
                "A different career inherited the lock");
        debug_attribute_persistence::load(s, lock_directory, career);
        require(s.memory.debug_locked_players().size() == 1 && s.memory.read<1>(player + 0x25) == 1,
                "Career metadata did not restore its player lock");
        s.memory.clear_debug_player_attributes();
        const auto original_birthday = s.memory.read<2>(staff + 0x34);
        s.memory.write<2>(staff + 0x34, original_birthday ^ 1);
        bool rejected = false;
        try { debug_attribute_persistence::load(s, lock_directory, career); }
        catch (const std::runtime_error&) { rejected = true; }
        require(rejected && s.memory.debug_locked_players().empty(), "Changed player identity retained a lock");
        s.memory.write<2>(staff + 0x34, original_birthday);
        { std::ofstream out(career, std::ios::binary | std::ios::app); out << 'x'; }
        debug_attribute_persistence::load(s, lock_directory, career);
        require(s.memory.debug_locked_players().empty(), "Changed save contents reused old lock metadata");
        s.memory.lock_debug_player_attributes(staff, player);
        s.memory.lock_debug_player_attributes(exchanged, neighbor);
        s.memory.unlock_debug_player_record(player);
        require(s.memory.debug_locked_players().size() == 1 && s.memory.debug_locked_players()[0].first == exchanged,
                "Reusing one player record removed another player's lock");
        s.memory.write<1>(player + 0x25, 7);
        s.memory.write<1>(neighbor + 0x25, 7);
        require(s.memory.read<1>(player + 0x25) == 7 && s.memory.read<1>(neighbor + 0x25) == 1,
                "Player record reuse retained or removed the wrong protection");
        s.memory.clear_debug_player_attributes();
        const auto platform_dir = root / "platform-fixture";
        std::filesystem::create_directories(platform_dir);
        const auto platform_save = platform_dir / "fixture.sav";
        { std::ofstream out(platform_save, std::ios::binary); out << "synthetic writer bytes"; }
        NativePlatform platform(argv[2], platform_dir, AudioMode::offline, EventMode::interactive);
        const auto guest_path = platform_save.string();
        require(guest_path.size() < 255, "Fixture path is too long");
        std::vector<std::uint8_t> path_bytes(guest_path.begin(), guest_path.end());
        path_bytes.push_back(0);
        s.memory.copy_in(scratch, path_bytes);
        s.memory.lock_debug_player_attributes(staff, player);
        s.wr<4>(12, scratch); s.wr<4>(16, 0x16);
        require(platform.debug_attribute_save_begin(s), "Platform refused the initial save");
        require(!platform.debug_attribute_save_begin(s), "Platform accepted a nested writer with active locks");
        require(!platform.debug_attribute_change_available(), "Attribute change accepted during a save");
        s.wr<1>(2304, 2); platform.debug_attribute_save_mode(s);
        s.wr<4>(12, 0); platform.debug_attribute_save_renamed(s);
        s.wr<4>(12, 1); platform.debug_attribute_save_end(s);
        require(platform.debug_attribute_change_available(), "Completed save left attribute changes disabled");
        s.wr<4>(12, scratch); s.wr<4>(16, 0x16);
        require(platform.debug_attribute_save_begin(s), "Platform refused a later save");
        platform.debug_attribute_save_abort();
        require(platform.debug_attribute_change_available(), "Aborted writer left attribute changes disabled");
        const auto failed_save = platform_dir / "failed.sav";
        { std::ofstream out(failed_save, std::ios::binary); out << "failed writer fixture"; }
        const auto failed_path = failed_save.string();
        require(failed_path.size() < 255, "Failed-save fixture path is too long");
        path_bytes.assign(failed_path.begin(), failed_path.end()); path_bytes.push_back(0);
        s.memory.copy_in(scratch, path_bytes);
        s.wr<4>(12, scratch); s.wr<4>(16, 0x16);
        require(platform.debug_attribute_save_begin(s), "Platform refused failed-save setup");
        s.wr<1>(2304, 2); platform.debug_attribute_save_mode(s);
        s.wr<4>(12, 0); platform.debug_attribute_save_renamed(s);
        s.wr<4>(12, 0); platform.debug_attribute_save_end(s);
        require(!std::filesystem::exists(platform_dir / "DebugAttributeLocks" /
                debug_attribute_persistence::save_key(failed_save)), "Failed save published attribute locks");
        path_bytes.assign(guest_path.begin(), guest_path.end()); path_bytes.push_back(0);
        s.memory.copy_in(scratch, path_bytes);
        s.wr<4>(12, scratch); platform.debug_attribute_load_begin(s);
        require(s.memory.debug_locked_players().empty(), "Load did not clear old career locks");
        s.memory.write<1>(player + 0x25, 7);
        s.wr<4>(12, 1); platform.debug_attribute_load_end(s);
        require(s.memory.read<1>(player + 0x25) == 1 && s.memory.debug_locked_players().size() == 1,
                "Platform save/load boundaries did not restore the selected lock");
        s.memory.clear_debug_player_attributes();
        s.wr<4>(12, scratch); s.wr<4>(16, 0x16);
        require(platform.debug_attribute_save_begin(s) && !platform.debug_attribute_save_begin(s),
                "Debugging allowed a nested writer before the first player was locked");
        platform.debug_attribute_save_abort();
        select_native_debugging(false);
        require(platform.debug_attribute_save_begin(s) && platform.debug_attribute_save_begin(s),
                "Disabled debugging changed nested-save availability");
        platform.debug_attribute_save_abort();
        std::cout << "Contract extension and sale cancellation: scheduled paid sale, exchange, deferred contracts, loan, unrelated offers and current terms passed\n";
    } catch (const std::exception& e) { std::cerr << e.what() << '\n'; return 1; }
}
