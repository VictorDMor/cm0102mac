#include "data_forks.hpp"
#include "hfs_names.hpp"
#include <CoreServices/CoreServices.h>
#include <fcntl.h>
#include <map>
#include <set>
#include <unistd.h>
#include <vector>
#include <cstring>

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
namespace cm::aot {
struct NativeDataForks::Impl {
    struct File { std::filesystem::path path; bool modified = false; };
    std::map<FSIORefNum, File> files;
    std::set<std::filesystem::path> written; // Closed after a write, not yet made durable.
    ~Impl() { for (const auto& [file, state] : files) ::FSCloseFork(file); }
};
NativeDataForks::NativeDataForks() : impl_(std::make_unique<Impl>()) {}
NativeDataForks::~NativeDataForks() = default;
std::int16_t NativeDataForks::open_read(const std::filesystem::path& file, std::int16_t& ref) {
    return open(file, fsRdPerm, ref);
}
std::int16_t NativeDataForks::open(const std::filesystem::path& file, std::int8_t permission, std::int16_t& ref) {
    return open(file, {}, permission, ref);
}
std::int16_t NativeDataForks::open(const std::filesystem::path& file, std::span<const std::uint16_t> fork_name,
                                  std::int8_t permission, std::int16_t& ref) {
    ref = 0;
    FSRef path;
    auto error = ::FSPathMakeRef(reinterpret_cast<const UInt8*>(file.c_str()), &path, nullptr);
    if (error) return error;
    FSIORefNum opened = 0;
    error = ::FSOpenFork(&path, fork_name.size(), fork_name.empty() ? nullptr : fork_name.data(), permission, &opened);
    if (!error) { impl_->files.emplace(opened, Impl::File{file}); ref = opened; }
    return error;
}
std::int16_t NativeDataForks::create(const std::filesystem::path& file, std::uint32_t creator, std::uint32_t type) {
    FSRef parent;
    const auto error = ::FSPathMakeRef(reinterpret_cast<const UInt8*>(file.parent_path().c_str()), &parent, nullptr);
    if (error) return error;
    const auto string = CFStringCreateWithCString(nullptr, hfs_name(file.filename().string()).c_str(), kCFStringEncodingUTF8);
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
    const auto found = impl_->files.find(ref);
    if (found == impl_->files.end()) return rfNumErr;
    const auto error = ::FSCloseFork(ref);
    if (!error) {
        if (found->second.modified) impl_->written.insert(found->second.path);
        impl_->files.erase(found);
    }
    return error;
}
void NativeDataForks::make_durable(const std::filesystem::path& directory) {
    for (auto file = impl_->written.begin(); file != impl_->written.end();) {
        std::error_code error;
        if (!std::filesystem::equivalent(file->parent_path(), directory, error)) { ++file; continue; }
        if (const int descriptor = ::open(file->c_str(), O_RDONLY); descriptor >= 0) {
            ::fcntl(descriptor, F_FULLFSYNC);
            ::close(descriptor);
        }
        file = impl_->written.erase(file);
    }
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
std::int16_t NativeDataForks::seek(std::int16_t ref, std::uint16_t mode, std::int64_t offset) {
    if (!impl_->files.contains(ref)) return rfNumErr;
    return ::FSSetForkPosition(ref, mode, offset);
}
std::int16_t NativeDataForks::read(std::int16_t ref, std::uint16_t mode, std::int64_t offset,
                                   std::span<std::uint8_t> buffer, std::uint32_t& actual) {
    actual = 0;
    if (!impl_->files.contains(ref)) return rfNumErr;
    ByteCount count = 0;
    std::uint8_t empty = 0;
    const auto error = ::FSReadFork(ref, mode, offset, buffer.size(), buffer.empty() ? &empty : buffer.data(), &count);
    actual = count;
    return error;
}
std::int16_t NativeDataForks::write(std::int16_t ref, std::uint16_t mode, std::int64_t offset,
                                  std::span<const std::uint8_t> buffer, std::uint32_t& actual) {
    actual = 0;
    const auto found = impl_->files.find(ref);
    if (found == impl_->files.end()) return rfNumErr;
    ByteCount count = 0;
    const std::uint8_t empty = 0;
    const auto error = ::FSWriteFork(ref, mode, offset, buffer.size(), buffer.empty() ? &empty : buffer.data(), &count);
    actual = count;
    if (count) found->second.modified = true;
    return error;
}
std::int16_t NativeDataForks::resize(std::int16_t ref, std::int64_t bytes) {
    if (bytes < 0 && impl_->files.contains(ref)) return paramErr;
    return resize(ref, fsFromStart, bytes);
}
std::int16_t NativeDataForks::resize(std::int16_t ref, std::uint16_t mode, std::int64_t offset) {
    const auto found = impl_->files.find(ref);
    if (found == impl_->files.end()) return rfNumErr;
    const auto error = ::FSSetForkSize(ref, mode, offset);
    if (!error) found->second.modified = true;
    return error;
}
}
#pragma clang diagnostic pop
