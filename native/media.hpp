#pragma once
#include "guest_abi.hpp"

namespace cm::aot::media {
// `bl` into another routine of this module: the C++ when it runs here, otherwise the
// generated body through the dispatcher. Set the argument registers first.
void call(guest::Cpu&, std::uint32_t entry, std::uint32_t back);
}
