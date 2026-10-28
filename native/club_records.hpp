#pragma once
#include "guest_abi.hpp"

namespace cm::aot::club_records {
using guest::Cpu;

// Assignment operators preserve record padding and the original field order,
// including when source and destination overlap.
enum class RecordKind {
    player_total, money, transfer, player_count, match, two_scores,
    count, average, score, dated_total, dated_match, dated_score, position,
    club,
};
std::uint32_t assign(Cpu&, std::uint32_t destination, std::uint32_t source, RecordKind);
std::uint32_t initialize_dates(Cpu&, std::uint32_t record, std::uint32_t entry);
std::uint32_t destroy_record(Cpu&, std::uint32_t object, std::uint32_t deleting, std::uint32_t entry);
std::uint32_t has_pending_record(Cpu&, std::uint32_t object, std::uint32_t club, std::uint32_t kind);
std::uint32_t clear_record(Cpu&, std::uint32_t record);
std::uint32_t modern_record(Cpu&, std::uint32_t manager, std::uint32_t club, std::uint32_t create);
std::uint32_t season_record(Cpu&, std::uint32_t manager, std::uint32_t club, std::uint32_t create);
std::uint32_t append_pending(Cpu&, std::uint32_t manager, std::uint32_t club, std::uint32_t kind);
std::uint32_t destroy(Cpu&, std::uint32_t manager, std::uint32_t deleting);
std::uint32_t period_label(Cpu&, std::uint32_t period, std::uint32_t output, std::uint32_t capacity);
std::uint32_t record_label(Cpu&, std::uint32_t kind, std::uint32_t output, std::uint32_t capacity);
std::uint32_t record_available(Cpu&, std::uint32_t kind, std::uint32_t club, std::uint32_t period);
std::uint32_t record_transfer(Cpu&, std::uint32_t manager, std::uint32_t offer);
std::uint32_t record_transfer_totals(Cpu&, std::uint32_t manager, std::uint32_t amount, std::uint32_t seller, std::uint32_t buyer);
std::uint32_t record_player_totals(Cpu&, std::uint32_t manager, std::uint32_t player, std::uint32_t club, std::uint32_t appearances, std::uint32_t goals);
std::uint32_t save(Cpu&, std::uint32_t manager, std::uint32_t file);
std::uint32_t load(Cpu&, std::uint32_t manager, std::uint32_t file);
std::uint32_t qualifies_transfer(Cpu&, std::uint32_t manager, std::uint32_t offer, std::uint32_t strict);
std::uint32_t calculate_average(Cpu&, std::uint32_t manager, std::uint32_t club);
std::uint32_t select_best_player(Cpu&, std::uint32_t manager, std::uint32_t club, std::uint32_t record);
std::uint32_t encode_notice(Cpu&, std::uint32_t club, std::uint32_t record, std::uint32_t kind, std::uint32_t message);
std::uint32_t decode_notice(Cpu&, std::uint32_t message, std::uint32_t club_out, std::uint32_t record, std::uint32_t kind_out);
std::uint32_t flush_pending(Cpu&, std::uint32_t manager);
std::uint32_t read_history(Cpu&, std::uint32_t manager, std::uint32_t club, std::uint32_t count_out);
std::uint32_t format(Cpu&,std::uint32_t manager,std::uint32_t club,std::uint32_t record,std::uint32_t kind,std::uint32_t output);
std::uint32_t format_period(Cpu&,std::uint32_t manager,std::uint32_t club,std::uint32_t period,std::uint32_t kind,std::uint32_t output);
std::uint32_t construct(Cpu&,std::uint32_t manager,std::uint32_t file);
std::uint32_t configure_screen(Cpu&,std::uint32_t club,std::uint32_t mode,std::uint32_t preserve,
                               std::uint32_t alternate,std::uint32_t include_unavailable);
std::uint32_t record_league_positions(Cpu&,std::uint32_t manager,std::uint32_t competition,std::uint32_t modern);
std::uint32_t archive_season(Cpu&,std::uint32_t manager,std::uint32_t season);
std::uint32_t format_notice(Cpu&,std::uint32_t message,std::uint32_t details,std::uint32_t output);
std::uint32_t update_match(Cpu&,std::uint32_t manager,std::uint32_t match,std::uint32_t statistics);
}
