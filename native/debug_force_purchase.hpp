#pragma once
#include <cstdint>
#include <unordered_map>

namespace cm::aot {
struct State;

// Screen-history fields owned by the new-proposal screen and its confirmation.
// A seller ID plus one makes an uninitialized history field mean a normal bid.
constexpr unsigned forced_proposal_seller_slot = 10;
constexpr unsigned forced_confirmation_seller_slot = 3;

struct ForcedPurchase {
    std::uint32_t offer, revision, player, buyer, seller;
};
struct ForcedPurchases {
    std::unordered_map<std::uint32_t, ForcedPurchase> offers;
};
ForcedPurchases& forced_purchases(State&);
const ForcedPurchases& forced_purchases(const State&);
void clear_forced_purchases(State&);

// Returns the selling club address if this player can be bought by the active human.
std::uint32_t forced_purchase_seller(State&, std::uint32_t player);
void record_forced_purchase(State&, std::uint32_t transfers, std::uint32_t draft,
                            std::uint32_t expected_revision, std::uint32_t seller);
bool forced_purchase_for_seller(State&, std::uint32_t offer, std::uint32_t seller);
bool forced_purchase_contract(State&, std::uint32_t contract);
}
