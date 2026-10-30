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
    std::int16_t open(const std::filesystem::path& file, std::span<const std::uint16_t> fork_name,
                      std::int8_t permission, std::int16_t& ref);
    std::int16_t create(const std::filesystem::path& file, std::uint32_t creator, std::uint32_t type);
    std::int16_t close(std::int16_t ref);
    // The game saves by writing a temporary file, deleting the old save and
    // renaming the temporary one. Call before a delete or rename in
    // `directory`: files written there reach stable storage first, so a crash
    // cannot lose both copies.
    void make_durable(const std::filesystem::path& directory);
    std::int16_t size(std::int16_t ref, std::int64_t& bytes) const;
    std::int16_t position(std::int16_t ref, std::int64_t& offset) const;
    std::int16_t seek(std::int16_t ref, std::uint16_t mode, std::int64_t offset);
    std::int16_t read(std::int16_t ref, std::uint16_t mode, std::int64_t offset,
                      std::span<std::uint8_t> buffer, std::uint32_t& actual);
    std::int16_t write(std::int16_t ref, std::uint16_t mode, std::int64_t offset,
                       std::span<const std::uint8_t> buffer, std::uint32_t& actual);
    std::int16_t resize(std::int16_t ref, std::int64_t bytes);
    std::int16_t resize(std::int16_t ref, std::uint16_t mode, std::int64_t offset);
    // A quick-save staging directory is private to one operation. Track
    // physical write, seek, resize, close and durability failures there.
    // Unattributed mutating failures are conservatively latched separately.
    bool begin_write_audit(const std::filesystem::path& directory);
    bool finish_write_audit();
    void abort_write_audit() noexcept;
    void note_write_audit_failure(std::int16_t ref);
    void note_write_audit_failure(const std::filesystem::path& path);
    void note_write_audit_directory_failure(const std::filesystem::path& directory);
    // For a failed mutating request whose path cannot be resolved. Such an
    // unrelated concurrent failure can conservatively reject a quick save.
    void note_unattributed_write_audit_failure();
private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};
}
