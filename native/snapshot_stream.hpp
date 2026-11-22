#pragma once
#include <filesystem>
#include <functional>
#include <iosfwd>

namespace cm::aot {
// New captures are one checksummed Zstandard frame containing the original
// snapshot bytes. Existing raw captures use the same payload reader.
void write_snapshot_file(const std::filesystem::path&, const std::function<void(std::ostream&)>&);
void read_snapshot_file(const std::filesystem::path&, const std::function<void(std::istream&)>&);
}
