#include "data_forks.hpp"
#include "hfs_names.hpp"
#include <CoreServices/CoreServices.h>
#include <fcntl.h>
#include <map>
#include <optional>
#include <set>
#include <unistd.h>
#include <vector>
#include <cstring>

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
namespace cm::aot {
struct NativeDataForks::Impl {
    struct File { std::filesystem::path path; bool modified = false; bool audited = false; };
    std::map<FSIORefNum, File> files;
    std::set<std::filesystem::path> written; // Closed after a write, not yet made durable.
    std::optional<std::filesystem::path> audit_directory;
    bool audit_ok = true;
    bool audited(const std::filesystem::path& path) const {
        if (!audit_directory) return false;
        std::error_code error;
        return std::filesystem::equivalent(path.parent_path(), *audit_directory, error) && !error;
    }
    void failure(const std::filesystem::path& path) {
        if (audited(path)) audit_ok = false;
    }
    ~Impl() { for (const auto& [file, state] : files) ::FSCloseFork(file); }
};
NativeDataForks::NativeDataForks() : impl_(std::make_unique<Impl>()) {}
NativeDataForks::~NativeDataForks() = default;
bool NativeDataForks::begin_write_audit(const std::filesystem::path& directory) {
    if (impl_->audit_directory) return false;
    std::error_code error;
    const auto canonical = std::filesystem::canonical(directory, error);
    if (error || !std::filesystem::is_directory(canonical, error) || error) return false;
    for (const auto& [ref, file] : impl_->files) {
        (void)ref;
        if (std::filesystem::equivalent(file.path.parent_path(), canonical, error) || error) return false;
    }
    impl_->audit_directory = canonical;
    impl_->audit_ok = true;
    return true;
}
bool NativeDataForks::finish_write_audit() {
    if (!impl_->audit_directory) return false;
    for (auto& [ref, file] : impl_->files) {
        (void)ref;
        if (file.audited) impl_->audit_ok = false;
        file.audited = false;
    }
    for (const auto& path : impl_->written)
        if (impl_->audited(path)) impl_->audit_ok = false;
    const bool result = impl_->audit_ok;
    impl_->audit_directory.reset();
    return result;
}
void NativeDataForks::abort_write_audit() noexcept {
    for (auto& [ref, file] : impl_->files) { (void)ref; file.audited = false; }
    impl_->audit_directory.reset();
    impl_->audit_ok = false;
}
void NativeDataForks::note_write_audit_failure(std::int16_t ref) {
    const auto found = impl_->files.find(ref);
    if (found != impl_->files.end() && found->second.audited) impl_->audit_ok = false;
}
void NativeDataForks::note_write_audit_failure(const std::filesystem::path& path) {
    impl_->failure(path);
}
void NativeDataForks::note_write_audit_directory_failure(const std::filesystem::path& directory) {
    if (!impl_->audit_directory) return;
    std::error_code error;
    if (std::filesystem::equivalent(directory, *impl_->audit_directory, error) && !error) impl_->audit_ok = false;
}
void NativeDataForks::note_unattributed_write_audit_failure() {
    if (impl_->audit_directory) impl_->audit_ok = false;
}
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
    if (error) {
        if (permission != fsRdPerm) impl_->failure(file);
        return error;
    }
    FSIORefNum opened = 0;
    error = ::FSOpenFork(&path, fork_name.size(), fork_name.empty() ? nullptr : fork_name.data(), permission, &opened);
    if (!error) { impl_->files.emplace(opened, Impl::File{file, false, permission != fsRdPerm && impl_->audited(file)}); ref = opened; }
    else if (permission != fsRdPerm) impl_->failure(file);
    return error;
}
std::int16_t NativeDataForks::create(const std::filesystem::path& file, std::uint32_t creator, std::uint32_t type) {
    FSRef parent;
    const auto error = ::FSPathMakeRef(reinterpret_cast<const UInt8*>(file.parent_path().c_str()), &parent, nullptr);
    if (error) { impl_->failure(file); return error; }
    const auto string = CFStringCreateWithCString(nullptr, hfs_name(file.filename().string()).c_str(), kCFStringEncodingUTF8);
    if (!string) { impl_->failure(file); return bdNamErr; }
    const auto length = CFStringGetLength(string);
    if (length < 1 || length > 255) { CFRelease(string); impl_->failure(file); return bdNamErr; }
    std::vector<UniChar> characters(length);
    CFStringGetCharacters(string, CFRangeMake(0, length), characters.data());
    CFRelease(string);
    FSCatalogInfo catalog{};
    FileInfo finder{};
    finder.fileType = type; finder.fileCreator = creator;
    std::memcpy(catalog.finderInfo, &finder, sizeof(finder));
    const auto result = ::FSCreateFileUnicode(&parent, characters.size(), characters.data(), kFSCatInfoFinderInfo, &catalog, nullptr, nullptr);
    if (result) impl_->failure(file);
    return result;
}
std::int16_t NativeDataForks::close(std::int16_t ref) {
    const auto found = impl_->files.find(ref);
    if (found == impl_->files.end()) return rfNumErr;
    const auto error = ::FSCloseFork(ref);
    if (error && found->second.audited) impl_->audit_ok = false;
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
        const bool watched = impl_->audited(*file);
        if (const int descriptor = ::open(file->c_str(), O_RDONLY); descriptor >= 0) {
            const int flushed = ::fcntl(descriptor, F_FULLFSYNC);
            const int closed = ::close(descriptor);
            if (watched && (flushed || closed)) impl_->audit_ok = false;
        } else if (watched) impl_->audit_ok = false;
        file = impl_->written.erase(file);
    }
}
std::int16_t NativeDataForks::size(std::int16_t ref, std::int64_t& bytes) const {
    const auto found = impl_->files.find(ref);
    if (found == impl_->files.end()) return rfNumErr;
    SInt64 result = 0;
    const auto error = ::FSGetForkSize(ref, &result);
    if (error && found->second.audited) impl_->audit_ok = false;
    if (!error) bytes = result;
    return error;
}
std::int16_t NativeDataForks::position(std::int16_t ref, std::int64_t& offset) const {
    const auto found = impl_->files.find(ref);
    if (found == impl_->files.end()) return rfNumErr;
    SInt64 result = 0;
    const auto error = ::FSGetForkPosition(ref, &result);
    if (error && found->second.audited) impl_->audit_ok = false;
    if (!error) offset = result;
    return error;
}
std::int16_t NativeDataForks::seek(std::int16_t ref, std::uint16_t mode, std::int64_t offset) {
    const auto found = impl_->files.find(ref);
    if (found == impl_->files.end()) return rfNumErr;
    const auto error = ::FSSetForkPosition(ref, mode, offset);
    if (error && found->second.audited) impl_->audit_ok = false;
    return error;
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
    if (error && found->second.audited) impl_->audit_ok = false;
    actual = count;
    if (count) found->second.modified = true;
    return error;
}
std::int16_t NativeDataForks::resize(std::int16_t ref, std::int64_t bytes) {
    if (bytes < 0 && impl_->files.contains(ref)) { note_write_audit_failure(ref); return paramErr; }
    return resize(ref, fsFromStart, bytes);
}
std::int16_t NativeDataForks::resize(std::int16_t ref, std::uint16_t mode, std::int64_t offset) {
    const auto found = impl_->files.find(ref);
    if (found == impl_->files.end()) return rfNumErr;
    const auto error = ::FSSetForkSize(ref, mode, offset);
    if (error && found->second.audited) impl_->audit_ok = false;
    if (!error) found->second.modified = true;
    return error;
}
}
#pragma clang diagnostic pop
