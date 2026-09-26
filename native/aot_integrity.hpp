#pragma once
#include "aot_runtime.hpp"

namespace cm::aot {
void load_verified(Memory& memory, std::uint32_t address,
                   const std::filesystem::path& file, std::uint64_t size,
                   const char* expected_sha256);
}
