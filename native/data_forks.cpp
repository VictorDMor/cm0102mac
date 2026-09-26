#include "data_forks.hpp"
#include <CoreServices/CoreServices.h>
#include <set>
#include <vector>
#include <cstring>

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
namespace cm::aot {
struct NativeDataForks::Impl {
    std::set<FSIORefNum> files;
    ~Impl() { for (auto file : files) ::FSCloseFork(file); }
};
NativeDataForks::NativeDataForks() : impl_(std::make_unique<Impl>()) {}
NativeDataForks::~NativeDataForks() = default;
std::int16_t NativeDataForks::open_read(const std::filesystem::path& file, std::int16_t& ref) {
    return open(file, fsRdPerm, ref);
}
std::int16_t NativeDataForks::open(const std::filesystem::path& file, std::int8_t permission, std::int16_t& ref) {
    ref = 0;
    FSRef path;
    auto error = ::FSPathMakeRef(reinterpret_cast<const UInt8*>(file.c_str()), &path, nullptr);
    if (error) return error;
    FSIORefNum opened = 0;
    error = ::FSOpenFork(&path, 0, nullptr, permission, &opened);
    if (!error) { impl_->files.insert(opened); ref = opened; }
    return error;
}
std::int16_t NativeDataForks::create(const std::filesystem::path& file, std::uint32_t creator, std::uint32_t type) {
    FSRef parent;
    const auto error = ::FSPathMakeRef(reinterpret_cast<const UInt8*>(file.parent_path().c_str()), &parent, nullptr);
    if (error) return error;
    const auto string = CFStringCreateWithCString(nullptr, file.filename().c_str(), kCFStringEncodingUTF8);
    if (!string) return bdNamErr;
    const auto length = CFStringGetLength(string);
    if (length < 1 || length > 255) { CFRelease(string); return bdNamErr; }
    std::vector<UniChar> characters(length);
    CFStringGetCharacters(string, CFRangeMake(0, length), characters.data());
    CFRelease(string);
    FSCatalogInfo catalog{};
    FileInfo finder{};
    finder.fileType = type; finder.fileCreator = creator;
    std::memcpy(catalog.finderInfo, &finder, sizeof(finder));
    return ::FSCreateFileUnicode(&parent, characters.size(), characters.data(), kFSCatInfoFinderInfo, &catalog, nullptr, nullptr);
}
std::int16_t NativeDataForks::close(std::int16_t ref) {
    if (!impl_->files.contains(ref)) return rfNumErr;
    const auto error = ::FSCloseFork(ref);
    if (!error) impl_->files.erase(ref);
    return error;
}
std::int16_t NativeDataForks::size(std::int16_t ref, std::int64_t& bytes) const {
    if (!impl_->files.contains(ref)) return rfNumErr;
    SInt64 result = 0;
    const auto error = ::FSGetForkSize(ref, &result);
    if (!error) bytes = result;
    return error;
}
std::int16_t NativeDataForks::position(std::int16_t ref, std::int64_t& offset) const {
    if (!impl_->files.contains(ref)) return rfNumErr;
    SInt64 result = 0;
    const auto error = ::FSGetForkPosition(ref, &result);
    if (!error) offset = result;
    return error;
}
std::int16_t NativeDataForks::seek(std::int16_t ref, std::uint16_t mode, std::int32_t offset) {
    if (!impl_->files.contains(ref)) return rfNumErr;
    return ::FSSetForkPosition(ref, mode, offset);
}
std::int16_t NativeDataForks::read(std::int16_t ref, std::uint16_t mode, std::int32_t offset,
                                   std::span<std::uint8_t> buffer, std::uint32_t& actual) {
    actual = 0;
    if (!impl_->files.contains(ref)) return rfNumErr;
    ByteCount count = 0;
    std::uint8_t empty = 0;
    const auto error = ::FSReadFork(ref, mode, offset, buffer.size(), buffer.empty() ? &empty : buffer.data(), &count);
    actual = count;
    return error;
}
std::int16_t NativeDataForks::write(std::int16_t ref, std::uint16_t mode, std::int32_t offset,
                                  std::span<const std::uint8_t> buffer, std::uint32_t& actual) {
    actual = 0;
    if (!impl_->files.contains(ref)) return rfNumErr;
    ByteCount count = 0;
    const std::uint8_t empty = 0;
    const auto error = ::FSWriteFork(ref, mode, offset, buffer.size(), buffer.empty() ? &empty : buffer.data(), &count);
    actual = count;
    return error;
}
std::int16_t NativeDataForks::resize(std::int16_t ref, std::int32_t bytes) {
    if (!impl_->files.contains(ref)) return rfNumErr;
    if (bytes < 0) return paramErr;
    return ::FSSetForkSize(ref, fsFromStart, bytes);
}
}
#pragma clang diagnostic pop
