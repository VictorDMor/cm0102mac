#include "resource_bridge.hpp"
#include "hfs_names.hpp"
#include <CoreServices/CoreServices.h>
#include <cstring>
#include <map>
#include <stdexcept>
#include <algorithm>
#include <set>

// Resource Manager APIs listed here are provided for ARM64 by the macOS SDK.
// Original resource payloads remain byte sequences; native handles never enter
// the game's 32-bit address space.
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
namespace cm::aot {
namespace {
void convert_resource_order(std::uint32_t type, std::int16_t id, std::vector<std::uint8_t>& bytes, bool currently_native) {
    if (bytes.empty()) return;
    const auto error = ::CoreEndianFlipData(kCoreEndianResourceManagerDomain,
        type, id, bytes.data(), bytes.size(), currently_native);
    if (error != noErr && error != handlerNotFoundErr)
        throw std::runtime_error("Cannot convert original resource byte order: " + std::to_string(error));
}
}
struct NativeResources::Impl {
    ResFileRefNum application = -1, previous = -1;
    std::uint64_t next_ticket = 1;
    struct ResourceHandle { Handle value; ResFileRefNum file; };
    std::map<std::uint64_t, ResourceHandle> handles;
    std::set<ResFileRefNum> opened;
    std::set<ResFileRefNum> writable;
    mutable std::int16_t last_error = 0;
    ~Impl() {
        for (auto ref : opened) ::CloseResFile(ref);
        if (application != -1) ::CloseResFile(application);
        if (previous != -1) ::UseResFile(previous);
    }
};
NativeResources::NativeResources(const std::filesystem::path& application_file) : impl_(std::make_unique<Impl>()) {
    FSRef ref;
    const auto path_error = ::FSPathMakeRef(reinterpret_cast<const UInt8*>(application_file.c_str()), &ref, nullptr);
    if (path_error) throw std::runtime_error("Cannot identify original resource file: " + std::to_string(path_error));
    impl_->previous = ::CurResFile();
    impl_->application = ::FSOpenResFile(&ref, fsRdPerm);
    if (impl_->application == -1) throw std::runtime_error("Cannot open original resource fork: " + std::to_string(::ResError()));
    ::UseResFile(impl_->application);
}
NativeResources::~NativeResources() = default;
NativeResource NativeResources::get(std::uint32_t type, std::int16_t id, bool current_only, bool indexed) {
    Handle handle = indexed ? ::Get1IndResource(type, id) : current_only ? ::Get1Resource(type, id) : ::GetResource(type, id);
    impl_->last_error = ::ResError();
    if (!handle) return {};
    NativeResource result;
    for (const auto& [ticket, existing] : impl_->handles)
        if (existing.value == handle) result.ticket = ticket;
    if (!result.ticket) {
        result.ticket = impl_->next_ticket++;
        impl_->handles.emplace(result.ticket, Impl::ResourceHandle{handle, ::HomeResFile(handle)});
    }
    result.loaded = *handle != nullptr;
    if (result.loaded) {
        const auto size = ::GetHandleSize(handle);
        if (size < 0) throw std::runtime_error("Invalid native resource size");
        result.bytes.resize(size);
        if (size) {
            std::memcpy(result.bytes.data(), *handle, size);
            ResID actual_id;
            ResType actual_type;
            Str255 resource_name;
            ::GetResInfo(handle, &actual_id, &actual_type, resource_name);
            // CarbonCore automatically swaps some built-in resource types on
            // little-endian hosts. Undo that swap on our copy for original PPC
            // code, leaving the native Resource Manager's handle untouched.
            convert_resource_order(actual_type, actual_id, result.bytes, true);
        }
    }
    return result;
}
std::vector<std::uint8_t> NativeResources::string(std::int16_t id, std::int16_t index) {
    // GetIndString itself is a 32-bit-only API. STR# payloads are big-endian
    // counts followed by Pascal strings, even when fetched by the native API.
    const auto resource = get(0x53545223, id, false);
    if (!resource.loaded || resource.bytes.size() < 2 || index < 1) return {0};
    const auto count = (unsigned(resource.bytes[0]) << 8) | resource.bytes[1];
    if (unsigned(index) > count) return {0};
    std::size_t position = 2;
    for (int i = 1; i <= index; ++i) {
        if (position >= resource.bytes.size()) throw std::runtime_error("Truncated original STR# table");
        const auto end = position + resource.bytes[position] + 1;
        if (end > resource.bytes.size()) throw std::runtime_error("Truncated original Pascal string");
        if (i == index) return {resource.bytes.begin() + position, resource.bytes.begin() + end};
        position = end;
    }
    return {0};
}
std::int16_t NativeResources::current() const { return ::CurResFile(); }
std::int16_t NativeResources::error() const { return impl_->last_error; }
std::int16_t NativeResources::count(std::uint32_t type) const {
    const auto count = ::Count1Resources(type);
    impl_->last_error = ::ResError();
    return count;
}
void NativeResources::use(std::int16_t ref) { ::UseResFile(ref); impl_->last_error = ::ResError(); }
void NativeResources::set_load(bool load) { ::SetResLoad(load); }
std::vector<std::string> NativeResources::names(std::uint32_t type) const {
    std::vector<std::string> result;
    const auto count = ::CountResources(type);
    impl_->last_error = ::ResError();
    for (short index = 1; index <= count; ++index) {
        Handle handle = ::GetIndResource(type, index);
        if (!handle) throw std::runtime_error("Cannot enumerate native menu resources");
        ResID id;
        ResType actual_type;
        Str255 name;
        ::GetResInfo(handle, &id, &actual_type, name);
        if (name[0]) result.emplace_back(reinterpret_cast<const char*>(name + 1), name[0]);
    }
    std::sort(result.begin(), result.end());
    return result;
}
void NativeResources::release(std::uint64_t ticket, bool detach) {
    const auto handle = impl_->handles.at(ticket).value;
    if (detach) ::DetachResource(handle);
    else ::ReleaseResource(handle);
    impl_->last_error = ::ResError();
    if (impl_->last_error == 0) {
        if (detach) ::DisposeHandle(handle); // The game retains its independent native-data copy.
        impl_->handles.erase(ticket);
    }
}
void NativeResources::create(const std::filesystem::path& file, std::uint32_t creator, std::uint32_t type) {
    FSRef parent;
    impl_->last_error = ::FSPathMakeRef(reinterpret_cast<const UInt8*>(file.parent_path().c_str()), &parent, nullptr);
    if (impl_->last_error) return;
    const auto name = hfs_name(file.filename().string());
    CFStringRef string = ::CFStringCreateWithCString(nullptr, name.c_str(), kCFStringEncodingUTF8);
    if (!string) { impl_->last_error = bdNamErr; return; }
    std::vector<UniChar> characters(::CFStringGetLength(string));
    ::CFStringGetCharacters(string, CFRangeMake(0, characters.size()), characters.data());
    ::CFRelease(string);
    HFSUniStr255 fork;
    impl_->last_error = ::FSGetResourceForkName(&fork);
    if (impl_->last_error) return;
    FSCatalogInfo catalog{};
    FileInfo finder{};
    finder.fileType = type;
    finder.fileCreator = creator;
    std::memcpy(catalog.finderInfo, &finder, sizeof(finder));
    impl_->last_error = ::FSCreateResourceFile(&parent, characters.size(), characters.data(),
        kFSCatInfoFinderInfo, &catalog, fork.length, fork.unicode, nullptr, nullptr);
}
std::int16_t NativeResources::open(const std::filesystem::path& file, std::int8_t permission) {
    FSRef ref;
    impl_->last_error = ::FSPathMakeRef(reinterpret_cast<const UInt8*>(file.c_str()), &ref, nullptr);
    if (impl_->last_error) return -1;
    const auto result = ::FSOpenResFile(&ref, permission);
    impl_->last_error = ::ResError();
    if (result != -1 && result != impl_->application) {
        impl_->opened.insert(result);
        if (permission != fsRdPerm) impl_->writable.insert(result);
    }
    return result;
}
std::vector<std::uint64_t> NativeResources::close(std::int16_t ref) {
    std::vector<std::uint64_t> retired;
    if (!impl_->opened.contains(ref)) { impl_->last_error = resFNotFound; return retired; }
    ::CloseResFile(ref);
    impl_->last_error = ::ResError();
    if (!impl_->last_error) {
        impl_->opened.erase(ref);
        impl_->writable.erase(ref);
        // Resource Manager disposes handles belonging to this file.
        for (auto it = impl_->handles.begin(); it != impl_->handles.end();) {
            if (it->second.file == ref) {
                retired.push_back(it->first);
                it = impl_->handles.erase(it);
            } else ++it;
        }
    }
    return retired;
}
std::uint64_t NativeResources::add(std::uint32_t type, std::int16_t id, const std::vector<std::uint8_t>& name,
                                 const std::vector<std::uint8_t>& bytes) {
    if (!impl_->writable.contains(::CurResFile())) { impl_->last_error = permErr; return 0; }
    if (name.size() > 255) { impl_->last_error = paramErr; return 0; }
    auto native_bytes = bytes;
    convert_resource_order(type, id, native_bytes, false);
    Handle handle = ::NewHandle(native_bytes.size());
    if (!handle) { impl_->last_error = ::MemError(); return 0; }
    if (!native_bytes.empty()) std::memcpy(*handle, native_bytes.data(), native_bytes.size());
    Str255 native_name{};
    native_name[0] = name.size();
    std::copy(name.begin(), name.end(), native_name + 1);
    ::AddResource(handle, type, id, native_name);
    impl_->last_error = ::ResError();
    if (impl_->last_error) { ::DisposeHandle(handle); return 0; }
    const auto ticket = impl_->next_ticket++;
    impl_->handles.emplace(ticket, Impl::ResourceHandle{handle, ::HomeResFile(handle)});
    return ticket;
}
void NativeResources::write(std::uint64_t ticket, const std::vector<std::uint8_t>& bytes) {
    const auto& resource = impl_->handles.at(ticket);
    if (!impl_->writable.contains(resource.file)) { impl_->last_error = permErr; return; }
    ResID id;
    ResType type;
    Str255 name;
    ::GetResInfo(resource.value, &id, &type, name);
    impl_->last_error = ::ResError();
    if (impl_->last_error) return;
    auto native_bytes = bytes;
    convert_resource_order(type, id, native_bytes, false);
    const auto flags = ::HGetState(resource.value);
    ::HUnlock(resource.value);
    ::SetHandleSize(resource.value, native_bytes.size());
    impl_->last_error = ::MemError();
    if (!impl_->last_error && !native_bytes.empty()) std::memcpy(*resource.value, native_bytes.data(), native_bytes.size());
    ::HSetState(resource.value, flags);
    if (impl_->last_error) return;
    ::ChangedResource(resource.value);
    impl_->last_error = ::ResError();
    if (impl_->last_error) return;
    ::WriteResource(resource.value);
    impl_->last_error = ::ResError();
}
void NativeResources::remove(std::uint64_t ticket) {
    const auto& resource = impl_->handles.at(ticket);
    if (!impl_->writable.contains(resource.file)) { impl_->last_error = permErr; return; }
    ::RemoveResource(resource.value);
    impl_->last_error = ::ResError();
    if (!impl_->last_error) {
        ::DisposeHandle(resource.value); // The original relocatable handle retains its own bytes.
        impl_->handles.erase(ticket);
    }
}
}
#pragma clang diagnostic pop
