#include "filesystem_bridge.hpp"
#include <CoreServices/CoreServices.h>
#include <algorithm>
#include <stdexcept>
#include <vector>

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
namespace cm::aot {
LegacyVolumeGeometry legacy_volume_geometry(const std::filesystem::space_info& space) {
    // The original game multiplies these fields in 32 bits before converting
    // free bytes to MiB (0x1073afc0). Expose a conservative Classic volume view
    // whose products fit; never report more free space than the backing disk.
    const auto capacity = std::min<std::uint64_t>(space.capacity, UINT32_MAX);
    const auto block_size = std::max<std::uint64_t>(4096,
        (((capacity + 65534) / 65535) + 4095) & ~UINT64_C(4095));
    const auto blocks = capacity / block_size;
    const auto free_blocks = std::min<std::uint64_t>(space.available / block_size, blocks);
    return {std::uint32_t(block_size), std::uint16_t(blocks), std::uint16_t(free_blocks)};
}
std::int16_t native_rename_object(const std::filesystem::path& path, const std::string& name, std::uint32_t encoding) {
    FSRef ref;
    const auto error = ::FSPathMakeRefWithOptions(reinterpret_cast<const UInt8*>(path.c_str()),
        kFSPathMakeRefDoNotFollowLeafSymlink, &ref, nullptr);
    if (error) return error;
    const auto text = CFStringCreateWithCString(nullptr, name.c_str(), kCFStringEncodingUTF8);
    if (!text) return bdNamErr;
    const auto length = CFStringGetLength(text);
    if (length < 1 || length > 255) { CFRelease(text); return bdNamErr; }
    std::vector<UniChar> characters(length);
    CFStringGetCharacters(text, CFRangeMake(0, length), characters.data());
    CFRelease(text);
    return ::FSRenameUnicode(&ref, characters.size(), characters.data(), encoding, nullptr);
}
std::int16_t native_delete_object(const std::filesystem::path& path) {
    FSRef ref;
    const auto error = ::FSPathMakeRefWithOptions(reinterpret_cast<const UInt8*>(path.c_str()),
        kFSPathMakeRefDoNotFollowLeafSymlink, &ref, nullptr);
    return error ? error : ::FSDeleteObject(&ref);
}
NativeVolumeParameters native_volume_parameters(const std::filesystem::path& path) {
    NativeVolumeParameters result;
    FSRef ref;
    result.error = ::FSPathMakeRef(reinterpret_cast<const UInt8*>(path.c_str()), &ref, nullptr);
    if (result.error) return result;
    FSCatalogInfo catalog{};
    result.error = ::FSGetCatalogInfo(&ref, kFSCatInfoVolume, &catalog, nullptr, nullptr, nullptr);
    if (result.error) return result;
    GetVolParmsInfoBuffer parameters{};
    result.error = ::FSGetVolumeParms(catalog.volume, &parameters, sizeof(parameters));
    if (result.error) return result;
    if (parameters.vMLocalHand) throw std::runtime_error("Native volume has private handle data requiring a bridge");
    // Version 2 contains all legacy fields needed by the original path lookup.
    // Host pointers in later versions are deliberately not copied or truncated.
    result.version = std::min<std::int16_t>(parameters.vMVersion, 2);
    result.attributes = parameters.vMAttrib;
    result.server_address = parameters.vMServerAdr;
    result.speed = parameters.vMVolumeGrade;
    result.privilege_model = parameters.vMForeignPrivID;
    return result;
}
}
#pragma clang diagnostic pop
