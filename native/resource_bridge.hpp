#pragma once
#include <cstdint>
#include <filesystem>
#include <memory>
#include <vector>

namespace cm::aot {
struct NativeResource {
    std::uint64_t ticket = 0;
    bool loaded = false;
    std::vector<std::uint8_t> bytes;
};
class NativeResources {
public:
    explicit NativeResources(const std::filesystem::path& application_file);
    ~NativeResources();
    NativeResource get(std::uint32_t type, std::int16_t id, bool current_only, bool indexed = false);
    std::vector<std::uint8_t> string(std::int16_t id, std::int16_t index);
    std::int16_t current() const;
    std::int16_t error() const;
    std::int16_t count(std::uint32_t type) const;
    void use(std::int16_t ref);
    void set_load(bool load);
    void release(std::uint64_t ticket, bool detach);
    std::vector<std::string> names(std::uint32_t type) const;
    void create(const std::filesystem::path& file, std::uint32_t creator, std::uint32_t type);
    std::int16_t open(const std::filesystem::path& file, std::int8_t permission);
    std::vector<std::uint64_t> close(std::int16_t ref);
    std::uint64_t add(std::uint32_t type, std::int16_t id, const std::vector<std::uint8_t>& name,
                      const std::vector<std::uint8_t>& bytes);
    void write(std::uint64_t ticket, const std::vector<std::uint8_t>& bytes);
    void remove(std::uint64_t ticket);
private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};
}
