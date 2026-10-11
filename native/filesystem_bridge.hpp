#pragma once
#include <cstdint>
#include <filesystem>

namespace cm::aot {
struct LegacyVolumeGeometry {
    std::uint32_t block_size;
    std::uint16_t blocks, free_blocks;
};
LegacyVolumeGeometry legacy_volume_geometry(const std::filesystem::space_info& space);
struct NativeVolumeParameters {
    std::int16_t error = 0, version = 0, privilege_model = 0;
    std::uint32_t attributes = 0, server_address = 0, speed = 0;
};
NativeVolumeParameters native_volume_parameters(const std::filesystem::path& path);
std::int16_t native_delete_object(const std::filesystem::path& path);
std::int16_t native_rename_object(const std::filesystem::path& path, const std::string& name,
    std::uint32_t encoding = UINT32_MAX);
}
