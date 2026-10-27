#pragma once
#include "screen_manager.hpp"
namespace cm::aot::screen_manager {
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
