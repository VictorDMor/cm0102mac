#include "file_reference_bridge.hpp"
#include "aot_runtime.hpp"
#include <CoreServices/CoreServices.h>
#include <cstring>
#include <algorithm>
#include <climits>
#include <type_traits>

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
namespace cm::aot {
namespace {
struct CatalogField { std::uint32_t bit; unsigned offset, size; };
constexpr std::array catalog_fields{
    CatalogField{0x1, 140, 4}, CatalogField{0x2, 0, 2}, CatalogField{0x4, 2, 2},
    CatalogField{0x8, 4, 4}, CatalogField{0x10, 8, 4}, CatalogField{0x20, 16, 8},
    CatalogField{0x40, 24, 8}, CatalogField{0x80, 32, 8}, CatalogField{0x100, 40, 8},
    CatalogField{0x200, 48, 8}, CatalogField{0x400, 56, 16}, CatalogField{0x800, 72, 16},
    CatalogField{0x1000, 88, 16}, CatalogField{0x2000, 136, 4}, CatalogField{0x4000, 104, 16},
    CatalogField{0x8000, 120, 16}, CatalogField{0x10000, 12, 1}, CatalogField{0x20000, 13, 1},
    CatalogField{0x80000, 65, 1}, CatalogField{0x100000, 56, 16}
};
static_assert(sizeof(FSRef) == 80);
FSRef reference(const NativeFileReference& bytes) {
    FSRef ref{}; std::memcpy(&ref, bytes.data(), sizeof(ref)); return ref;
}
void reference(const FSRef& ref, NativeFileReference& bytes) { std::memcpy(bytes.data(), &ref, sizeof(ref)); }
void date(GuestFileCatalog& bytes, unsigned at, const UTCDateTime& value) {
    write_be<2>(bytes.data() + at, value.highSeconds);
    write_be<4>(bytes.data() + at + 2, value.lowSeconds);
    write_be<2>(bytes.data() + at + 6, value.fraction);
}
UTCDateTime date(const GuestFileCatalog& bytes, unsigned at) {
    return {std::uint16_t(read_be<2>(bytes.data() + at)), std::uint32_t(read_be<4>(bytes.data() + at + 2)),
        std::uint16_t(read_be<2>(bytes.data() + at + 6))};
}
template<unsigned N> void native_field(std::uint8_t* host, std::uint8_t* guest, bool to_guest) {
    using Word = std::conditional_t<N == 2, std::uint16_t, std::uint32_t>;
    Word value;
    if (to_guest) { std::memcpy(&value, host, N); write_be<N>(guest, value); }
    else { value = read_be<N>(guest); std::memcpy(host, &value, N); }
}
void finder(std::uint8_t* host, std::uint8_t* guest, bool folder, bool extended, bool to_guest) {
    if (!extended && folder) for (unsigned at = 0; at < 8; at += 2) native_field<2>(host + at, guest + at, to_guest);
    else if (!extended) for (unsigned at = 0; at < 8; at += 4) native_field<4>(host + at, guest + at, to_guest);
    else if (folder) {
        native_field<2>(host, guest, to_guest); native_field<2>(host + 2, guest + 2, to_guest);
        native_field<4>(host + 4, guest + 4, to_guest);
    } else for (unsigned at = 0; at < 8; at += 2) native_field<2>(host + at, guest + at, to_guest);
    native_field<2>(host + 8, guest + 8, to_guest); native_field<2>(host + 10, guest + 10, to_guest);
    if (extended) native_field<4>(host + 12, guest + 12, to_guest);
    else { native_field<2>(host + 12, guest + 12, to_guest); native_field<2>(host + 14, guest + 14, to_guest); }
}
FSCatalogInfo decode(GuestFileCatalog bytes, std::uint32_t bitmap) {
    FSCatalogInfo info{};
    if (bitmap & kFSCatInfoNodeFlags) info.nodeFlags = read_be<2>(bytes.data());
    if (bitmap & kFSCatInfoCreateDate) info.createDate = date(bytes, 16);
    if (bitmap & kFSCatInfoContentMod) info.contentModDate = date(bytes, 24);
    if (bitmap & kFSCatInfoAttrMod) info.attributeModDate = date(bytes, 32);
    if (bitmap & kFSCatInfoAccessDate) info.accessDate = date(bytes, 40);
    if (bitmap & kFSCatInfoBackupDate) info.backupDate = date(bytes, 48);
    if (bitmap & (kFSCatInfoPermissions | kFSCatInfoSetOwnership)) {
        info.permissions.userID = read_be<4>(bytes.data() + 56);
        info.permissions.groupID = read_be<4>(bytes.data() + 60);
        info.permissions.reserved1 = bytes[64]; info.permissions.userAccess = bytes[65];
        info.permissions.mode = read_be<2>(bytes.data() + 66);
    }
    const bool folder = info.nodeFlags & kFSNodeIsDirectoryMask;
    if (bitmap & kFSCatInfoFinderInfo) finder(info.finderInfo, bytes.data() + 72, folder, false, false);
    if (bitmap & kFSCatInfoFinderXInfo) finder(info.extFinderInfo, bytes.data() + 88, folder, true, false);
    if (bitmap & kFSCatInfoTextEncoding) info.textEncodingHint = read_be<4>(bytes.data() + 140);
    return info;
}
}
std::int16_t native_file_reference(const std::filesystem::path& path, NativeFileReference& bytes) {
    FSRef ref{};
    const auto error = ::FSPathMakeRefWithOptions(reinterpret_cast<const UInt8*>(path.c_str()),
        kFSPathMakeRefDoNotFollowLeafSymlink, &ref, nullptr);
    if (!error) reference(ref, bytes);
    return error;
}
std::int16_t native_reference_path(const NativeFileReference& bytes, std::filesystem::path& path) {
    const auto ref = reference(bytes);
    std::array<UInt8, PATH_MAX> buffer{};
    const auto error = ::FSRefMakePath(&ref, buffer.data(), buffer.size());
    if (!error) path = reinterpret_cast<const char*>(buffer.data());
    return error;
}
std::int16_t native_compare_references(const NativeFileReference& a, const NativeFileReference& b) {
    const auto first = reference(a), second = reference(b);
    return ::FSCompareFSRefs(&first, &second);
}
std::int16_t native_file_catalog(const NativeFileReference& bytes, std::uint32_t bitmap, NativeFileCatalog& result) {
    // A returned FSFileSecurityRef needs a separate owned-object bridge. Never
    // truncate or publish that host pointer in the Classic permissions slot.
    if (bitmap & kFSCatInfoFSFileSecurityRef) return paramErr;
    const auto ref = reference(bytes);
    FSCatalogInfo info{}; HFSUniStr255 name{}; FSRef parent{};
    const auto error = ::FSGetCatalogInfo(&ref, bitmap | kFSCatInfoNodeFlags, &info, &name, nullptr, &parent);
    if (error) return error;
    auto& data = result.bytes;
    write_be<2>(data.data(), info.nodeFlags); write_be<2>(data.data() + 2, std::uint16_t(info.volume));
    write_be<4>(data.data() + 4, info.parentDirID); write_be<4>(data.data() + 8, info.nodeID);
    data[12] = info.sharingFlags; data[13] = info.userPrivileges;
    date(data, 16, info.createDate); date(data, 24, info.contentModDate); date(data, 32, info.attributeModDate);
    date(data, 40, info.accessDate); date(data, 48, info.backupDate);
    write_be<4>(data.data() + 56, info.permissions.userID); write_be<4>(data.data() + 60, info.permissions.groupID);
    data[64] = info.permissions.reserved1; data[65] = info.permissions.userAccess;
    write_be<2>(data.data() + 66, info.permissions.mode);
    const bool folder = info.nodeFlags & kFSNodeIsDirectoryMask;
    finder(info.finderInfo, data.data() + 72, folder, false, true);
    finder(info.extFinderInfo, data.data() + 88, folder, true, true);
    write_be<8>(data.data() + 104, info.dataLogicalSize); write_be<8>(data.data() + 112, info.dataPhysicalSize);
    write_be<8>(data.data() + 120, info.rsrcLogicalSize); write_be<8>(data.data() + 128, info.rsrcPhysicalSize);
    write_be<4>(data.data() + 136, info.valence); write_be<4>(data.data() + 140, info.textEncodingHint);
    result.name.assign(name.unicode, name.unicode + name.length); reference(parent, result.parent);
    if (info.permissions.fileSec) CFRelease(info.permissions.fileSec);
    return error;
}
std::int16_t native_create_unicode_file(const NativeFileReference& bytes, std::span<const std::uint16_t> name,
    std::uint32_t bitmap, const GuestFileCatalog& catalog, NativeFileReference& created) {
    if (bitmap & kFSCatInfoFSFileSecurityRef) return paramErr;
    const auto parent = reference(bytes);
    const auto info = decode(catalog, bitmap);
    FSRef ref{};
    const auto error = ::FSCreateFileUnicode(&parent, name.size(), name.data(), bitmap, &info, &ref, nullptr);
    if (!error) reference(ref, created);
    return error;
}
std::int16_t native_set_file_catalog(const NativeFileReference& bytes, std::uint32_t bitmap, const GuestFileCatalog& catalog) {
    if (bitmap & kFSCatInfoFSFileSecurityRef) return paramErr;
    const auto ref = reference(bytes);
    auto info = decode(catalog, bitmap);
    // Finder records have different first words for folders and files. Ask the
    // native object for its kind even when the setter does not change flags.
    if (bitmap & (kFSCatInfoFinderInfo | kFSCatInfoFinderXInfo)) {
        FSCatalogInfo kind{};
        const auto error = ::FSGetCatalogInfo(&ref, kFSCatInfoNodeFlags, &kind, nullptr, nullptr, nullptr);
        if (error) return error;
        auto data = catalog;
        if (bitmap & kFSCatInfoFinderInfo) finder(info.finderInfo, data.data() + 72, kind.nodeFlags & kFSNodeIsDirectoryMask, false, false);
        if (bitmap & kFSCatInfoFinderXInfo) finder(info.extFinderInfo, data.data() + 88, kind.nodeFlags & kFSNodeIsDirectoryMask, true, false);
    }
    return ::FSSetCatalogInfo(&ref, bitmap, &info);
}
std::int16_t native_unicode_name(std::span<const std::uint16_t> characters, std::string& result) {
    if (characters.empty() || characters.size() > 255) return bdNamErr;
    const auto value = CFStringCreateWithCharacters(nullptr, characters.data(), characters.size());
    if (!value) return bdNamErr;
    std::array<char, 1024> text{};
    const bool converted = CFStringGetCString(value, text.data(), text.size(), kCFStringEncodingUTF8);
    CFRelease(value);
    if (!converted || std::find(characters.begin(), characters.end(), 0) != characters.end()) return bdNamErr;
    result = text.data();
    if (result == "." || result == ".." || result.find_first_of(":/") != std::string::npos) return bdNamErr;
    return noErr;
}
GuestFileCatalog read_guest_file_catalog(State& s, std::uint32_t address, std::uint32_t bitmap) {
    GuestFileCatalog result{};
    for (const auto& field : catalog_fields) if (bitmap & field.bit) {
        const auto at = std::uint64_t(address) + field.offset;
        if (at + field.size > (UINT64_C(1) << 32)) throw std::runtime_error("Catalog field exceeds word32 guest extent");
        const auto bytes = s.memory.copy_out(std::uint32_t(at), field.size);
        std::copy(bytes.begin(), bytes.end(), result.begin() + field.offset);
    }
    return result;
}
void write_guest_file_catalog(State& s, std::uint32_t address, std::uint32_t bitmap, const GuestFileCatalog& bytes) {
    for (const auto& field : catalog_fields) if (bitmap & field.bit) {
        const auto at = std::uint64_t(address) + field.offset;
        if (at + field.size > (UINT64_C(1) << 32)) throw std::runtime_error("Catalog field exceeds word32 guest extent");
        s.memory.copy_in(std::uint32_t(at), {bytes.begin() + field.offset, bytes.begin() + field.offset + field.size});
    }
}
}
#pragma clang diagnostic pop
