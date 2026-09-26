#pragma once
#include <cstdint>
#include <filesystem>
#include <memory>
#include <span>

namespace cm::aot {
class NativeDataForks {
public:
    NativeDataForks();
    ~NativeDataForks();
    std::int16_t open_read(const std::filesystem::path& file, std::int16_t& ref);
    std::int16_t open(const std::filesystem::path& file, std::int8_t permission, std::int16_t& ref);
    std::int16_t create(const std::filesystem::path& file, std::uint32_t creator, std::uint32_t type);
    std::int16_t close(std::int16_t ref);
    std::int16_t size(std::int16_t ref, std::int64_t& bytes) const;
    std::int16_t position(std::int16_t ref, std::int64_t& offset) const;
    std::int16_t seek(std::int16_t ref, std::uint16_t mode, std::int32_t offset);
    std::int16_t read(std::int16_t ref, std::uint16_t mode, std::int32_t offset,
                      std::span<std::uint8_t> buffer, std::uint32_t& actual);
    std::int16_t write(std::int16_t ref, std::uint16_t mode, std::int32_t offset,
                       std::span<const std::uint8_t> buffer, std::uint32_t& actual);
    std::int16_t resize(std::int16_t ref, std::int32_t bytes);
private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};
}
