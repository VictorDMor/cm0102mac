#include "directory_catalog.hpp"
#include <CoreFoundation/CoreFoundation.h>
#include <algorithm>
#include <array>
#include <cerrno>
#include <map>
#include <sys/stat.h>
#include <tuple>

namespace cm::aot {
namespace fs = std::filesystem;
namespace {
using Name = std::unique_ptr<std::remove_pointer_t<CFStringRef>, decltype(&CFRelease)>;
Name decoded(const fs::path& name) {
    return Name(CFStringCreateWithCString(nullptr, name.c_str(), kCFStringEncodingUTF8), &CFRelease);
}
struct NameLess {
    bool operator()(const Name& a, const Name& b) const {
        return CFStringCompare(a.get(), b.get(), kCFCompareCaseInsensitive | kCFCompareNonliteral) == kCFCompareLessThan;
    }
};
struct Stamp {
    std::uint64_t device = 0, inode = 0, mode = 0;
    std::int64_t modified = 0, modified_ns = 0, changed = 0, changed_ns = 0;
    bool operator==(const Stamp&) const = default;
};
Stamp stamp(const fs::path& path) {
    if (path.empty()) return {};
    struct stat value{};
    if (stat(path.c_str(), &value)) {
        if (errno == ENOENT || errno == ENOTDIR) return {};
        throw fs::filesystem_error("Cannot inspect directory", path, std::error_code(errno, std::generic_category()));
    }
    return {std::uint64_t(value.st_dev), value.st_ino, value.st_mode,
            value.st_mtimespec.tv_sec, value.st_mtimespec.tv_nsec,
            value.st_ctimespec.tv_sec, value.st_ctimespec.tv_nsec};
}
}
struct NativeDirectoryCatalog::Impl {
    using Key = std::array<fs::path, 3>;
    struct Listing {
        std::array<Stamp, 2> stamps;
        bool valid = false;
        std::vector<Entry> entries;
        std::map<Name, std::size_t, NameLess> names;
    };
    std::map<Key, Listing> listings;

    Listing& get(const Key& key) {
        const std::array current{stamp(key[0]), stamp(key[1])};
        auto& cached = listings[key];
        if (cached.valid && cached.stamps == current) return cached;
        Listing fresh;
        fresh.stamps = current;
        auto append = [&](unsigned source) {
            if (!S_ISDIR(current[source].mode)) return;
            for (const auto& item : fs::directory_iterator(key[source])) {
                if (item.path() == key[2]) continue;
                Entry entry{item.path(), item.symlink_status().type()};
                auto name = decoded(item.path().filename());
                // Undecodable names never compared equal in the original view.
                if (!name) { fresh.entries.push_back(std::move(entry)); continue; }
                const auto [found, inserted] = fresh.names.try_emplace(std::move(name), fresh.entries.size());
                if (inserted) fresh.entries.push_back(std::move(entry));
                else if (source == 1) fresh.entries[found->second] = std::move(entry);
            }
        };
        append(0);
        append(1);
        std::sort(fresh.entries.begin(), fresh.entries.end(), [](const auto& a, const auto& b) {
            return a.path.filename().string() < b.path.filename().string();
        });
        fresh.names.clear();
        for (std::size_t i = 0; i < fresh.entries.size(); ++i) {
            auto name = decoded(fresh.entries[i].path.filename());
            if (name) fresh.names.emplace(std::move(name), i);
        }
        // Concurrent directory changes were never an atomic snapshot. Do not
        // retain a listing across queries if a mutation overlapped its scan.
        fresh.valid = current == std::array{stamp(key[0]), stamp(key[1])};
        cached = std::move(fresh);
        return cached;
    }
};
NativeDirectoryCatalog::NativeDirectoryCatalog() : impl_(std::make_unique<Impl>()) {}
NativeDirectoryCatalog::~NativeDirectoryCatalog() = default;
const std::vector<NativeDirectoryCatalog::Entry>& NativeDirectoryCatalog::list(
    const fs::path& original, const fs::path& replacement, const fs::path& excluded) {
    return impl_->get({original, replacement, excluded}).entries;
}
fs::path NativeDirectoryCatalog::child(const fs::path& original, const fs::path& replacement,
                                      const fs::path& excluded, const std::string& name) {
    const auto& listing = impl_->get({original, replacement, excluded});
    auto key = decoded(name);
    if (key) {
        const auto found = listing.names.find(key);
        if (found != listing.names.end()) return listing.entries[found->second].path;
    }
    return {};
}
}
