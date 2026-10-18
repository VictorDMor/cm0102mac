#pragma once
#include "semantic_dependency_dispatch.hpp"
#include <array>

namespace cm::aot::awol {
inline constexpr std::uint32_t constructor_entry=0x100031b0,destructor_entry=0x10003300;
inline constexpr std::uint32_t load_entry=0x100033c0,save_entry=0x100036e0;
inline constexpr std::uint32_t primary_status_entry=0x10003dc0,secondary_status_entry=0x10003ec0,pending_status_entry=0x10003fa0;
inline constexpr std::uint32_t remove_primary_staff_entry=0x10006600,remove_primary_item_entry=0x100066d0;
inline constexpr std::uint32_t remove_secondary_item_entry=0x100067d0,remove_secondary_staff_entry=0x10006830;
inline constexpr std::uint32_t notice_entry=0x10007100,sampling_entry=0x10006080,daily_entry=0x10004070;
inline constexpr std::uint32_t registration_entry=0x10006900,training_notice_entry=0x10008770;
inline constexpr std::uint32_t primary_text_entry=0x100072e0,training_text_entry=0x10008b60,fitness_text_entry=0x10009b60;
inline constexpr std::uint32_t training_event_entry=0x10007a50,fitness_event_entry=0x100090a0;
// Prepared components only. No production adapter owns these entries yet.
inline constexpr std::array record_entries{constructor_entry,destructor_entry,load_entry,save_entry,
    primary_status_entry,secondary_status_entry,pending_status_entry,remove_primary_staff_entry,
    remove_primary_item_entry,remove_secondary_item_entry,remove_secondary_staff_entry,notice_entry,sampling_entry,daily_entry,registration_entry,training_notice_entry,primary_text_entry,training_text_entry,fitness_text_entry,training_event_entry,fitness_event_entry};
void process_training_absence(State&,SemanticDependencyDispatch* =nullptr,SemanticGuestObserver* =nullptr);
void process_fitness_concern(State&,SemanticDependencyDispatch* =nullptr,SemanticGuestObserver* =nullptr);
void describe_primary_absence(State&,SemanticDependencyDispatch* =nullptr,SemanticGuestObserver* =nullptr);
void describe_training_absence(State&,SemanticDependencyDispatch* =nullptr,SemanticGuestObserver* =nullptr);
void describe_fitness_concern(State&,SemanticDependencyDispatch* =nullptr,SemanticGuestObserver* =nullptr);
void register_primary_absence(State&,SemanticDependencyDispatch* =nullptr,SemanticGuestObserver* =nullptr);
void notify_training_absence(State&,SemanticDependencyDispatch* =nullptr,SemanticGuestObserver* =nullptr);
void process_daily_absences(State&,SemanticDependencyDispatch* =nullptr,SemanticGuestObserver* =nullptr);
void sample_absence_range(State&,SemanticDependencyDispatch* =nullptr,SemanticGuestObserver* =nullptr);
void construct_records(State&,SemanticDependencyDispatch* =nullptr,SemanticGuestObserver* =nullptr);
void destroy_records(State&,SemanticDependencyDispatch* =nullptr,SemanticGuestObserver* =nullptr);
void load_records(State&,SemanticDependencyDispatch* =nullptr,SemanticGuestObserver* =nullptr);
void save_records(State&,SemanticDependencyDispatch* =nullptr,SemanticGuestObserver* =nullptr);
void primary_absence_status(State&,SemanticDependencyDispatch* =nullptr,SemanticGuestObserver* =nullptr);
void secondary_absence_status(State&,SemanticDependencyDispatch* =nullptr,SemanticGuestObserver* =nullptr);
void has_pending_record(State&,SemanticDependencyDispatch* =nullptr,SemanticGuestObserver* =nullptr);
void remove_primary_by_staff(State&,SemanticDependencyDispatch* =nullptr,SemanticGuestObserver* =nullptr);
void remove_primary_item(State&,SemanticDependencyDispatch* =nullptr,SemanticGuestObserver* =nullptr);
void remove_secondary_item(State&,SemanticDependencyDispatch* =nullptr,SemanticGuestObserver* =nullptr);
void remove_secondary_by_staff(State&,SemanticDependencyDispatch* =nullptr,SemanticGuestObserver* =nullptr);
void notify_absence_change(State&,SemanticDependencyDispatch* =nullptr,SemanticGuestObserver* =nullptr);
}
