#include "automation.hpp"
#include "aot_runtime.hpp"
#include <fstream>
#include <iostream>
#include <sstream>
#include <thread>
#include <unistd.h>

using namespace cm::aot;
namespace {
void require(bool ok, const std::string& message) { if (!ok) throw std::runtime_error(message); }
std::string read(const std::filesystem::path& path) {
    std::ifstream input(path);
    std::stringstream text;
    text << input.rdbuf();
    return text.str();
}
void request(const std::filesystem::path& directory, unsigned sequence, const std::string& line) {
    const auto temporary = directory / "inbox" / (std::to_string(sequence) + ".tmp");
    std::ofstream(temporary) << line << '\n';
    std::filesystem::rename(temporary, directory / "inbox" / (std::to_string(sequence) + ".req"));
}
// Waits for the inbox scan interval, then takes the next request.
std::optional<AutomationRequest> take(Automation& automation, const CommandContext& context) {
    std::this_thread::sleep_for(std::chrono::milliseconds(25));
    return automation.take(context);
}
// The original main loop polls without sleeping. Between event calls it
// runs a few thousand instructions when idle, millions while it works.
void loop(Automation& automation, std::uint64_t& executed, std::uint64_t work = 6000) {
    const auto end = std::chrono::steady_clock::now() + std::chrono::milliseconds(600);
    while (std::chrono::steady_clock::now() < end) {
        automation.enter(executed);
        automation.leave(executed);
        executed += work;
        std::this_thread::sleep_for(std::chrono::microseconds(200));
    }
}
}
int main() {
    try {
        const auto directory = std::filesystem::temp_directory_path() / ("cm-automation-checks-" + std::to_string(getpid()));
        std::filesystem::remove_all(directory);
        std::filesystem::create_directories(directory);
        Automation automation(directory);
        auto state = std::make_unique<State>();
        CommandContext busy, ready;
        ready.ready = true; ready.identity = 42; ready.handler = 0x10334940; ready.screen = 0x30000000;
        std::uint64_t executed = 1;

        // A snapshot is taken at any event boundary, even while busy.
        request(directory, 1, "snapshot first-look");
        auto taken = take(automation, busy);
        require(taken && taken->kind == AutomationRequest::Kind::snapshot && taken->name == "first-look", "Snapshot while busy");
        automation.finish(*taken, true, "saved");

        // Inputs wait for an idle interactive screen.
        request(directory, 2, "command continue_game");
        require(!take(automation, busy), "Command taken while the game is busy");
        require(!take(automation, ready), "Command taken before the screen idled");
        loop(automation, executed, 5'000'000);
        require(!take(automation, ready), "Command taken while each loop pass ran millions of instructions");
        loop(automation, executed);
        taken = take(automation, ready);
        require(taken && taken->kind == AutomationRequest::Kind::command && taken->command == DirectCommand::continue_game,
            "Command at an idle screen");
        automation.finish(*taken, true, "dispatched");

        // Keys: classic message (key code << 8 | character) and modifiers.
        request(directory, 3, "key cmd+s return shift+tab");
        require(!take(automation, ready), "An accepted input must wait for a fresh idle window");
        loop(automation, executed);
        taken = take(automation, ready);
        require(taken && taken->keys == std::vector<std::pair<std::uint32_t, std::uint16_t>>{
            {1u << 8 | 's', 0x0100}, {36u << 8 | 13, 0}, {48u << 8 | 9, 0x0200}}, "Named keys and modifiers");
        automation.finish(*taken, true, "");
        request(directory, 4, "text Ab1! x");
        loop(automation, executed);
        taken = take(automation, ready);
        require(taken && taken->keys == std::vector<std::pair<std::uint32_t, std::uint16_t>>{
            {0u << 8 | 'A', 0x0200}, {11u << 8 | 'b', 0}, {18u << 8 | '1', 0}, {18u << 8 | '!', 0x0200},
            {49u << 8 | ' ', 0}, {7u << 8 | 'x', 0}}, "Text keys");
        automation.finish(*taken, true, "");

        // Refusals: unknown requests and keys, stale sequence numbers.
        for (const auto& [sequence, line] : std::vector<std::pair<unsigned, std::string>>{
                 {5, "launch missiles"}, {6, "key hyper+x"}, {7, "control 5000"}, {8, "memory dump 0x10 0"}, {3, "snapshot late"}}) {
            request(directory, sequence, line);
            require(!take(automation, ready), "Refused request was returned: " + line);
        }
        const auto log = read(directory / "log.txt");
        for (const auto* expected : {"5 refused: unknown request launch", "6 refused: unknown key hyper+x",
                                     "7 refused: control needs a table index", "8 refused: memory size", "3 refused: sequence 3 is not after 8"})
            require(log.find(expected) != std::string::npos, std::string("Log lacks: ") + expected);
        require(std::filesystem::is_empty(directory / "inbox"), "Refused requests stay in the inbox");

        // Status: written on change, ready only when idle at an interactive screen.
        loop(automation, executed);
        automation.boundary(*state, ready, "Ready");
        auto status = read(directory / "status.json");
        require(status.find("\"done\": 8") != std::string::npos && status.find("\"ready\": true") != std::string::npos &&
                status.find("\"context\": \"0x000000000000002a\"") != std::string::npos, "Ready status:\n" + status);
        automation.boundary(*state, busy, "Unavailable while processing");
        status = read(directory / "status.json");
        require(status.find("\"ready\": false") != std::string::npos && status.find("\"controls\"") == std::string::npos,
            "Busy status:\n" + status);
        std::filesystem::remove_all(directory);
        std::cout << "Automation requests, key mapping, idle gating, refusals and status checks passed\n";
        return 0;
    } catch (const std::exception& error) { std::cerr << error.what() << '\n'; return 1; }
}
