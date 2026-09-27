#pragma once
#include <filesystem>
#include <memory>
#include <vector>

namespace cm::aot {
// A live directory view: validate directory identity and change timestamps on
// every query, retaining decoded names and entry types only while unchanged.
class NativeDirectoryCatalog {
public:
    struct Entry {
        std::filesystem::path path;
        std::filesystem::file_type type;
    };
    NativeDirectoryCatalog();
    ~NativeDirectoryCatalog();
    const std::vector<Entry>& list(const std::filesystem::path& original,
                                  const std::filesystem::path& replacement = {},
                                  const std::filesystem::path& excluded = {});
    std::filesystem::path child(const std::filesystem::path& original,
                                const std::filesystem::path& replacement,
                                const std::filesystem::path& excluded,
                                const std::string& name);
private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};
}
