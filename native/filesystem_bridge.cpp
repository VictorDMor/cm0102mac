#include "filesystem_bridge.hpp"
#include <CoreServices/CoreServices.h>
#include <algorithm>
#include <stdexcept>
#include <vector>

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
namespace cm::aot {
std::int16_t native_rename_object(const std::filesystem::path& path, const std::string& name) {
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
    return ::FSRenameUnicode(&ref, characters.size(), characters.data(), kTextEncodingUnknown, nullptr);
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
