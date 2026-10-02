#include "match_replay.hpp"
#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <unistd.h>

using namespace cm::aot;
namespace {
void require(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}
struct TemporaryFile {
    std::filesystem::path path;
    TemporaryFile() {
        auto pattern = (std::filesystem::temp_directory_path() / "cm-match-replay-XXXXXX").string();
        const int fd = mkstemp(pattern.data());
        if (fd < 0) throw std::runtime_error("Cannot create match replay test file");
        close(fd);
        path = pattern;
    }
    ~TemporaryFile() { std::error_code ignored; std::filesystem::remove(path, ignored); }
};
}
int main() {
    try {
        TemporaryFile file;
        auto original = std::make_unique<State>();
        original->pc = 0x1071a828;
        original->executed = 1234567;
        original->native_calls = 89;
        original->heap_end = 0x34567000;
        original->memory_error = -12;
        original->registers[0] = 0xa5;
        original->registers.back() = 0x5a;
        original->unique[1024] = 0x7e;
        original->allocated.emplace(0x20000100, 64);
        original->available.emplace(0x20000800, 256);
        original->memory.map(0x30000000, 16);
        original->memory.copy_in(0x30000000, {1, 2, 3, 4, 5, 6, 7, 8});
        original->memory.map(0x30010001, 13);
        original->memory.copy_in(0x30010001, {0xf0, 0xe1, 0xd2, 0xc3});
        require(setenv("CM_AOT_MATCH_SNAPSHOT", file.path.c_str(), 1) == 0,
                "Cannot arm private match snapshot test");
        require(match_snapshot_requested(), "Private match snapshot was not armed");
        capture_match_snapshot(*original);
        require(!match_snapshot_requested(), "Private match snapshot must capture only once");
        auto restored = std::make_unique<State>();
        load_match_snapshot(*restored, file.path);
        require(restored->pc == original->pc && restored->executed == original->executed &&
                restored->native_calls == original->native_calls &&
                restored->heap_end == original->heap_end &&
                restored->memory_error == original->memory_error &&
                restored->registers == original->registers &&
                restored->unique == original->unique &&
                restored->allocated == original->allocated &&
                restored->available == original->available &&
                restored->memory.mapped_ranges() == original->memory.mapped_ranges() &&
                match_snapshot_memory_hash(*restored) == match_snapshot_memory_hash(*original),
                "Private match snapshot changed replay state");
        std::filesystem::resize_file(file.path, std::filesystem::file_size(file.path) - 1);
        bool rejected = false;
        try {
            auto truncated = std::make_unique<State>();
            load_match_snapshot(*truncated, file.path);
        } catch (const std::runtime_error&) { rejected = true; }
        require(rejected, "Truncated private match snapshot was accepted");
        std::cout << "Private match snapshot round trip and truncation checks passed\n";
        return 0;
    } catch (const std::exception& error) { std::cerr << error.what() << '\n'; return 1; }
}
