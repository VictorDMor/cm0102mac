#pragma once
#include "guest_abi.hpp"

namespace cm::aot::transfer_screens {
using guest::Cpu;
void open_player_offer(Cpu&, std::uint32_t player, std::uint32_t offer_link);
void open_existing_offer(Cpu&, std::uint32_t offer_link, std::uint32_t create_screen);
void idle_callback(Cpu&);
void render_proposal(Cpu&);
void render_offer_restriction(Cpu&);
void open_transfer_feedback(Cpu&, std::uint32_t player, std::uint32_t club,
                            std::uint32_t reason, std::uint32_t kind);
void render_transfer_feedback(Cpu&);
void render_offer_context(Cpu&);
std::uint32_t offer_context_command(Cpu&, std::uint32_t control);
void render_previous_offer(Cpu&);
void render_remove_exchange_player(Cpu&);
std::uint32_t remove_exchange_player_command(Cpu&, std::uint32_t control);
void render_offer_news_decision(Cpu&);
std::uint32_t offer_news_decision_command(Cpu&, std::uint32_t control);
void open_offer_contract_terms(Cpu&, std::uint32_t offer);
void open_offer_terms_from_news(Cpu&, std::uint32_t manager, std::uint32_t item);
void render_withdraw_bid(Cpu&);
void update_proposal_total(Cpu&, std::uint32_t draft_offer, std::uint32_t offer_link);
std::uint32_t proposal_command(Cpu&, std::uint32_t selected_control);
void open_offer_context(Cpu&, std::uint32_t, std::uint32_t, std::uint32_t);
void open_previous_offer(Cpu&, std::uint32_t offer, std::uint32_t revision);
void open_existing_from_news(Cpu&, std::uint32_t manager, std::uint32_t item);
void open_player_from_news(Cpu&, std::uint32_t manager, std::uint32_t item);
void open_contract_from_news(Cpu&, std::uint32_t manager, std::uint32_t item);
void open_offer_news_context(Cpu&, std::uint32_t manager, std::uint32_t item);
void reopen_player_from_news(Cpu&, std::uint32_t manager, std::uint32_t item);
void open_bid_rejection(Cpu&, std::uint32_t player, std::uint32_t offer);
void open_offer_restriction(Cpu&, std::uint32_t, std::uint32_t, std::uint32_t);
void open_remove_exchange_player(Cpu&, std::uint32_t, std::uint32_t);
void open_transfer_completion_confirmation(Cpu&, std::uint32_t, std::uint32_t, std::uint32_t);
void open_shortlist_confirmation(Cpu&, std::uint32_t, std::uint32_t);
void open_player_comparison_confirmation(Cpu&, std::uint32_t, std::uint32_t);
void open_withdraw_bid_confirmation(Cpu&, std::uint32_t);
void open_transfer_listing_confirmation(Cpu&, std::uint32_t, std::uint32_t);
void open_release_confirmation(Cpu&, std::uint32_t);
void open_trial_period_selection(Cpu&, std::uint32_t);
void open_temporary_offer_confirmation(Cpu&, std::uint32_t, std::uint32_t);
void open_temporary_offer_terms_confirmation(Cpu&, std::uint32_t, std::uint32_t,
    std::uint32_t, std::uint32_t, std::uint32_t, std::uint32_t, std::uint32_t);
void clear_exchange_candidates(Cpu&);
std::uint32_t restriction_command(Cpu&, std::uint32_t);
std::uint32_t offer_command(Cpu&, std::uint32_t);
std::uint32_t previous_offer_command(Cpu&, std::uint32_t);
std::uint32_t withdraw_bid_command(Cpu&, std::uint32_t);
std::uint32_t release_player_command(Cpu&, std::uint32_t);
void open_exchange_candidates(Cpu&);
void open_completion_from_news(Cpu&, std::uint32_t, std::uint32_t);
}
