#pragma once
#include "file_controller_internal.hpp"
#include "code_fragment_adapter.hpp"

namespace cm::aot::code_fragment {
using Abi = file_controller::Abi;
inline constexpr auto entries = code_fragment_entries;
void register_fragment(Abi&);
void find_fragment(Abi&);
void resolve_metadata(Abi&);
void saved_frame_word(Abi&);
void action_kind(Abi&);
inline void arithmetic_half(Abi& a, unsigned dest, unsigned source) {
    const auto value = a.r(source);
    a.s.wr<1>(1027, bool(value & 0x80000000u) && bool(value & 1));
    a.put(dest, std::uint32_t(std::bit_cast<std::int32_t>(value) >> 1));
}
}
