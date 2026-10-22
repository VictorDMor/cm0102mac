#pragma once
#include "direct_commands.hpp"
#include <chrono>
#include <cstdint>
#include <filesystem>
#include <memory>
#include <optional>
#include <string>
#include <string_view>

namespace cm::aot {
struct State;
// Opt-in driver channel for isolated validation apps (CM_AOT_AUTOMATION names
// its directory). A driver drops numbered request files into inbox/; the game
// takes one at an original interactive event boundary and reports progress in
// status.json. Requests name original commands, controls and keys; none
// carries a screen position, and none changes what the game simulates.
struct AutomationRequest {
    enum class Kind { command, control, key, text, snapshot, memory } kind{};
    std::uint64_t sequence = 0;
    DirectCommand command{};
    std::int16_t control = -1;
    // Key and text requests: classic event messages (key code << 8 | character)
    // with their modifiers. Snapshot/memory requests: output name and range.
    std::vector<std::pair<std::uint32_t, std::uint16_t>> keys;
    std::string name;
    std::uint32_t address = 0, size = 0;
};
class Automation {
public:
    // Null unless CM_AOT_AUTOMATION names an existing directory.
    static std::unique_ptr<Automation> from_environment();
    explicit Automation(std::filesystem::path directory);
    // The game entered / left its event wait, with its instruction count. A
    // screen counts as ready only while the game idles there: a load keeps
    // the start screen's handler while it runs.
    void enter(std::uint64_t executed);
    void leave(std::uint64_t executed);
    bool dirty() const { return changes_ != written_; }
    // Rewrite status.json when readiness, screen, date or progress changed.
    void boundary(const State& s, const CommandContext& context, std::string_view status);
    // The oldest pending request; only while the game waits for input.
    std::optional<AutomationRequest> take(const CommandContext& context);
    void finish(const AutomationRequest& request, bool accepted, std::string_view result);
    std::filesystem::path output(std::string_view kind, std::string_view name) const;
private:
    std::filesystem::path directory_;
    std::uint64_t done_ = 0, written_ = 0, changes_ = 1;
    std::string last_result_;
    bool ready_ = false;
    std::uint64_t identity_ = 0;
    std::uint64_t date_ = 0;
    std::string status_;
    std::chrono::steady_clock::time_point since_ = std::chrono::steady_clock::now();
    double last_busy_ = 0;
    bool idle_ = false, waiting_ = false;
    std::chrono::steady_clock::time_point window_ = since_, idle_since_ = since_, entered_ = since_;
    std::chrono::steady_clock::duration waited_{};
    // Instructions between leaving one event call and entering the next.
    std::uint64_t left_ = 0, most_work_ = 0, calls_ = 0;
    std::uint64_t idle_work_ = 2000000;
    bool trace_ = false;
    std::chrono::steady_clock::time_point next_scan_{};
    // A request read from the inbox that waits for an idle screen.
    std::optional<std::pair<AutomationRequest, std::string>> pending_;
    void evaluate(std::chrono::steady_clock::time_point now);
};
}
