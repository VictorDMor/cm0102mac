#pragma once
#include <array>
#include <cstdint>
#include <filesystem>
#include <span>
#include <string>

namespace cm::aot {
struct State;
// FSRef is an opaque 80-byte identity supplied by the native File Manager.
// Catalog fields, in contrast, require explicit PowerPC endian/width conversion.
using NativeFileReference = std::array<std::uint8_t, 80>;
using GuestFileCatalog = std::array<std::uint8_t, 144>;
struct NativeFileCatalog {
    GuestFileCatalog bytes{};
    std::u16string name;
    NativeFileReference parent{};
};
std::int16_t native_file_reference(const std::filesystem::path&, NativeFileReference&);
std::int16_t native_reference_path(const NativeFileReference&, std::filesystem::path&);
std::int16_t native_compare_references(const NativeFileReference&, const NativeFileReference&);
std::int16_t native_file_catalog(const NativeFileReference&, std::uint32_t bitmap, NativeFileCatalog&);
std::int16_t native_create_unicode_file(const NativeFileReference&, std::span<const std::uint16_t> name,
    std::uint32_t bitmap, const GuestFileCatalog&, NativeFileReference&);
std::int16_t native_set_file_catalog(const NativeFileReference&, std::uint32_t bitmap, const GuestFileCatalog&);
std::int16_t native_unicode_name(std::span<const std::uint16_t>, std::string&);
GuestFileCatalog read_guest_file_catalog(State&, std::uint32_t address, std::uint32_t bitmap);
void write_guest_file_catalog(State&, std::uint32_t address, std::uint32_t bitmap, const GuestFileCatalog&);
}
