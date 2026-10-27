#pragma once
#include "guest_abi.hpp"

namespace cm::aot::screen_manager {
using guest::Cpu;
std::uint32_t run_client(Cpu&,std::uint32_t);
std::uint32_t create_control(Cpu&,unsigned);
std::uint32_t broadcast_history(Cpu&, std::uint32_t, std::uint32_t,
    std::uint32_t, std::uint32_t, std::uint32_t,
    std::uint32_t, std::uint32_t);
std::uint32_t append_history(Cpu&, std::uint32_t, std::uint32_t,
    std::uint32_t, std::uint32_t, std::uint32_t, std::uint32_t);
std::uint32_t history_title(Cpu&, std::uint32_t);
std::uint32_t set_history_title(Cpu&, std::uint32_t, std::uint32_t);
std::uint32_t clear_history(Cpu&, std::uint32_t);
std::uint32_t history_value(Cpu&, std::uint32_t, std::uint32_t);
std::uint32_t set_history_value(Cpu&, std::uint32_t, std::uint32_t,
    std::uint32_t, std::uint32_t);
std::uint32_t broadcast_history_value(Cpu&, std::uint32_t, std::uint32_t,
    std::uint32_t, std::uint32_t, std::uint32_t);
std::uint32_t construct(Cpu&,std::uint32_t);
std::uint32_t destroy(Cpu&,std::uint32_t,std::uint32_t);
std::uint32_t reset(Cpu&,std::uint32_t);
std::uint32_t disconnect(Cpu&,std::uint32_t);
std::uint32_t connect(Cpu&,std::uint32_t,std::uint32_t,std::uint32_t);
std::uint32_t network_poll(Cpu&,std::uint32_t);
std::uint32_t stop_network(Cpu&,std::uint32_t);
std::uint32_t initialize(Cpu&);
std::uint32_t send_shared_value(Cpu&,std::uint32_t,std::uint32_t,std::uint32_t);
std::uint32_t send_title(Cpu&,std::uint32_t,std::uint32_t);
std::uint32_t play_sound(Cpu&,std::uint32_t,std::uint32_t,std::uint32_t,
                       std::uint32_t,std::uint32_t);
std::uint32_t sound_status(Cpu&,std::uint32_t,std::uint32_t);
std::uint32_t stop_sound(Cpu&,std::uint32_t,std::uint32_t);
std::uint32_t remember_sound(Cpu&,std::uint32_t,std::uint32_t,std::uint32_t);
std::uint32_t forget_sound(Cpu&,std::uint32_t,std::uint32_t);
std::uint32_t refresh_screens(Cpu&,std::uint32_t,std::uint32_t,std::uint32_t);
std::uint32_t send_message(Cpu&,std::uint32_t,std::uint32_t,std::uint32_t,std::uint32_t,std::uint32_t);
std::uint32_t run(Cpu&,std::uint32_t);
std::uint32_t set_default_history(Cpu&, std::uint32_t, std::uint32_t,
    std::uint32_t, std::uint32_t, std::uint32_t, std::uint32_t,
    std::uint32_t, std::uint32_t);
std::uint32_t pending_screens(Cpu&, std::uint32_t);
std::uint32_t history_at_end(Cpu&, std::uint32_t);
std::uint32_t history_at_start(Cpu&, std::uint32_t, std::uint32_t);
std::uint32_t history_is_active(Cpu&, std::uint32_t);
std::uint32_t history_has_callback(Cpu&, std::uint32_t);
std::uint32_t screen_flag(Cpu&,std::uint32_t,std::uint32_t,unsigned);
std::uint32_t set_screen_disabled(Cpu&,std::uint32_t,std::uint32_t,std::uint32_t);
std::uint32_t group_disabled(Cpu&,std::uint32_t,std::uint32_t);
std::uint32_t inactive_groups(Cpu&,std::uint32_t);
std::uint32_t screen_group(Cpu&,std::uint32_t,std::uint32_t);
std::uint32_t copy_group_keys(Cpu&,std::uint32_t,std::uint32_t,std::uint32_t,std::uint32_t);
std::uint32_t swap_screen_keys(Cpu&,std::uint32_t,std::uint32_t,std::uint32_t);
std::uint32_t screen_count(Cpu&,std::uint32_t);
std::uint32_t active_history_count(Cpu&,std::uint32_t);
std::uint32_t control_value(Cpu&,std::uint32_t,std::uint32_t);
std::uint32_t current_screen(Cpu&,std::uint32_t);
std::uint32_t open_screen(Cpu&,std::uint32_t);
std::uint32_t close_group(Cpu&,std::uint32_t);
std::uint32_t configure_screen(Cpu&,std::uint32_t,std::uint32_t,std::uint32_t,std::uint32_t);
std::uint32_t close_screen(Cpu&,std::uint32_t,std::uint32_t,std::uint32_t);
std::uint32_t activate_screen(Cpu&,std::uint32_t,std::uint32_t);
std::uint32_t close_current_history(Cpu&,std::uint32_t);
std::uint32_t sound_construct(Cpu&,std::uint32_t);
std::uint32_t sound_destroy(Cpu&,std::uint32_t,std::uint32_t);
std::uint32_t sound_clear(Cpu&,std::uint32_t);
std::uint32_t sound_stop(Cpu&,std::uint32_t,std::uint32_t);
std::uint32_t sound_load(Cpu&,std::uint32_t,std::uint32_t,std::uint32_t,std::uint32_t);
std::uint32_t sound_read(Cpu&,std::uint32_t,std::uint32_t);
std::uint32_t sound_finished(Cpu&,std::uint32_t,std::uint32_t);
std::uint32_t sound_set_volume(Cpu&,std::uint32_t,std::uint32_t,std::uint32_t);
std::uint32_t sound_pending(Cpu&,std::uint32_t);
std::uint32_t sound_volume(Cpu&,std::uint32_t,std::uint32_t,std::uint32_t);
std::uint32_t sound_callback(Cpu&,std::uint32_t,std::uint32_t);
}
