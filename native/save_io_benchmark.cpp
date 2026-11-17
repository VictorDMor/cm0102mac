// Time native save IO with already serialized, opaque bytes. No game is run.
#include "data_forks.hpp"
#include "quick_save_transaction.hpp"
#include <algorithm>
#include <chrono>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <vector>
#include <unistd.h>

using namespace cm::aot;
namespace fs = std::filesystem;
namespace {
void require(bool value, const char* message) { if (!value) throw std::runtime_error(message); }
std::vector<std::uint8_t> read(const fs::path& path) {
    std::ifstream input(path, std::ios::binary | std::ios::ate);
    require(bool(input), "Cannot open benchmark input");
    const auto size = input.tellg();
    require(size > 0, "Benchmark input is empty or unreadable");
    std::vector<std::uint8_t> bytes(static_cast<std::size_t>(size));
    input.seekg(0);
    require(bool(input.read(reinterpret_cast<char*>(bytes.data()), bytes.size())), "Cannot read benchmark input");
    return bytes;
}
struct TemporaryDirectory {
    fs::path path;
    TemporaryDirectory() {
        auto pattern = (fs::temp_directory_path() / "cm-save-io-XXXXXX").string();
        require(mkdtemp(pattern.data()) != nullptr, "Cannot create disposable benchmark directory");
        path = pattern;
    }
    ~TemporaryDirectory() { std::error_code error; fs::remove_all(path, error); }
};
class PhaseClock {
    std::chrono::steady_clock::time_point start_ = std::chrono::steady_clock::now();
public:
    double elapsed_ms() {
        const auto now = std::chrono::steady_clock::now();
        const auto milliseconds = std::chrono::duration<double, std::milli>(now - start_).count();
        start_ = now;
        return milliseconds;
    }
};
}
int main(int argc, char** argv) {
    try {
        require(argc == 2, "Usage: cm-save-io-benchmark <existing-save>");
        const auto bytes = read(argv[1]);
        TemporaryDirectory temporary;
        const auto target = temporary.path / "Career.sav";
        fs::copy_file(argv[1], target); // Input is read-only; every mutation is disposable.
        for (unsigned run = 0; run < 5; ++run) {
            PhaseClock clock;
            auto created = QuickSaveTransaction::create(target, temporary.path);
            require(bool(created), "Cannot create save transaction");
            auto transaction = std::move(*created);
            const auto create_ms = clock.elapsed_ms();
            NativeDataForks forks;
            require(forks.begin_write_audit(transaction->staging_directory()), "Cannot begin write audit");
            require(!forks.create(transaction->staged_path(), 0, 0), "Cannot create staged file");
            std::int16_t ref;
            require(!forks.open(transaction->staged_path(), 3, ref), "Cannot open staged file");
            for (std::size_t offset = 0; offset < bytes.size();) {
                const auto length = std::min<std::size_t>(65536, bytes.size() - offset);
                std::uint32_t written;
                require(!forks.write(ref, 3, 0, std::span(bytes).subspan(offset, length), written) && written == length,
                        "Staged write failed or was incomplete");
                offset += length;
            }
            require(!forks.close(ref), "Cannot close staged file");
            const auto write_ms = clock.elapsed_ms();
            forks.make_durable(transaction->staging_directory());
            require(forks.finish_write_audit(), "Staged write or durability audit failed");
            const auto durable_ms = clock.elapsed_ms();
            const auto promotion = transaction->promote();
            require(promotion.state == QuickSavePromotionState::promoted, "Save promotion was not confirmed");
            const auto promote_ms = clock.elapsed_ms();
            require(bool(transaction->discard_previous_save()), "Cannot discard confirmed previous save");
            require(bool(transaction->remove_empty_staging_directory()), "Cannot remove empty staging directory");
            remove_stale_quick_save_staging(temporary.path, false, temporary.path);
            const auto cleanup_ms = clock.elapsed_ms();
            require(read(target) == bytes, "Native save IO changed serialized bytes");
            std::cout << "{\"bytes\":" << bytes.size() << ",\"run\":" << run << ",\"create_ms\":" << create_ms
                      << ",\"write_ms\":" << write_ms << ",\"durable_ms\":" << durable_ms
                      << ",\"promote_ms\":" << promote_ms << ",\"cleanup_ms\":" << cleanup_ms << "}\n";
        }
        return 0;
    } catch (const std::exception& error) { std::cerr << error.what() << '\n'; return 1; }
}
