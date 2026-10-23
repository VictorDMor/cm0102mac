#include "automation.hpp"
#include "aot_runtime.hpp"
#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <unistd.h>

namespace cm::aot {
namespace {
constexpr std::uint32_t date_global = 0x108b3f10, phase_global = 0x108b40a4;
constexpr std::uint16_t shift = 0x0200, command_key = 0x0100, option = 0x0800, control_key = 0x1000;

// Host reads use copies so they never reach the game's read observers.
std::uint32_t peek(const State& s, std::uint32_t address, unsigned size) {
    const auto bytes = s.memory.copy_out(address, size);
    std::uint32_t value = 0;
    for (const auto byte : bytes) value = value << 8 | byte;
    return value;
}
std::optional<std::array<std::uint32_t, 3>> game_date(const State& s) {
    try {
        const auto date = peek(s, date_global, 4), phase = peek(s, phase_global, 4);
        if (!date || !phase) return std::nullopt;
        return std::array<std::uint32_t, 3>{peek(s, date, 2), peek(s, date + 2, 2), peek(s, phase, 1)};
    } catch (const std::exception&) { return std::nullopt; }
}
std::string hex(std::uint64_t value, int digits = 8) {
    char text[24];
    std::snprintf(text, sizeof text, "0x%0*llx", digits, static_cast<unsigned long long>(value));
    return text;
}
std::string quoted(std::string_view text) {
    std::string result = "\"";
    for (const char c : text) {
        if (c == '"' || c == '\\') { result += '\\'; result += c; }
        else if (std::uint8_t(c) < 0x20) { char escape[8]; std::snprintf(escape, sizeof escape, "\\u%04x", c); result += escape; }
        else result += c;
    }
    return result + '"';
}
// US ANSI virtual key codes for printable ASCII; shifted characters share the
// unshifted key's code.
std::optional<std::pair<std::uint8_t, bool>> key_for(char c) {
    using namespace std::literals;
    static constexpr auto plain = "asdfhgzxcv\0bqweryt123465=97-80]ou[ip\0lj'k;\\,/nm.\0 `"sv;
    static constexpr auto shifted = "ASDFHGZXCV\0BQWERYT!@#$^%+(&_*)}OU{IP\0LJ\"K:|<?NM>\0\0~"sv;
    static_assert(plain.size() == 51 && shifted.size() == 51);
    if (c == 0) return std::nullopt;
    if (const auto at = plain.find(c); at != std::string_view::npos) return std::pair{std::uint8_t(at), false};
    if (const auto at = shifted.find(c); at != std::string_view::npos) return std::pair{std::uint8_t(at), true};
    return std::nullopt;
}
std::optional<std::pair<std::uint32_t, std::uint16_t>> parse_key(std::string spec) {
    std::uint16_t modifiers = 0;
    for (std::size_t plus; spec.size() > 1 && (plus = spec.find('+')) != std::string::npos && plus + 1 < spec.size();) {
        const auto name = spec.substr(0, plus);
        if (name == "shift") modifiers |= shift;
        else if (name == "cmd") modifiers |= command_key;
        else if (name == "option") modifiers |= option;
        else if (name == "control") modifiers |= control_key;
        else return std::nullopt;
        spec.erase(0, plus + 1);
    }
    static constexpr std::array<std::tuple<std::string_view, std::uint8_t, std::uint8_t>, 15> named{{
        {"return", 36, 13}, {"enter", 76, 3}, {"tab", 48, 9}, {"space", 49, 32}, {"delete", 51, 8},
        {"escape", 53, 27}, {"forward-delete", 117, 127}, {"left", 123, 28}, {"right", 124, 29},
        {"down", 125, 31}, {"up", 126, 30}, {"home", 115, 1}, {"end", 119, 4}, {"page-up", 116, 11}, {"page-down", 121, 12}}};
    for (const auto& [name, code, character] : named)
        if (spec == name) return std::pair{std::uint32_t(code) << 8 | character, modifiers};
    if (spec.size() != 1) return std::nullopt;
    const auto key = key_for(spec[0]);
    if (!key) return std::nullopt;
    return std::pair{std::uint32_t(key->first) << 8 | std::uint8_t(spec[0]), std::uint16_t(modifiers | (key->second ? shift : 0))};
}
bool valid_name(std::string_view name) {
    return !name.empty() && name.size() <= 64 && std::all_of(name.begin(), name.end(), [](char c) {
        return (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') || c == '-' || c == '_';
    });
}
AutomationRequest parse(std::uint64_t sequence, const std::string& line) {
    AutomationRequest request;
    request.sequence = sequence;
    std::istringstream input(line);
    std::string verb;
    input >> verb;
    if (verb == "command") {
        std::string name; input >> name;
        const auto found = std::find(direct_command_names.begin(), direct_command_names.end(), name);
        if (found == direct_command_names.end()) throw std::runtime_error("unknown command " + name);
        request.kind = AutomationRequest::Kind::command;
        request.command = DirectCommand(found - direct_command_names.begin());
    } else if (verb == "control") {
        int index = -1; input >> index;
        if (!input || index < 0 || index >= 1200) throw std::runtime_error("control needs a table index");
        request.kind = AutomationRequest::Kind::control;
        request.control = std::int16_t(index);
    } else if (verb == "drag") {
        int from = -1, to = -1; input >> from >> to;
        if (!input || from < 0 || from >= 1200 || to < 0 || to >= 1200) throw std::runtime_error("drag needs two control table indexes");
        request.kind = AutomationRequest::Kind::drag;
        request.control = std::int16_t(from);
        request.target = std::int16_t(to);
    } else if (verb == "key") {
        request.kind = AutomationRequest::Kind::key;
        for (std::string spec; input >> spec;) {
            const auto key = parse_key(spec);
            if (!key) throw std::runtime_error("unknown key " + spec);
            request.keys.push_back(*key);
        }
        if (request.keys.empty()) throw std::runtime_error("key needs at least one key");
    } else if (verb == "text") {
        request.kind = AutomationRequest::Kind::text;
        std::string text; std::getline(input, text);
        if (!text.empty() && text[0] == ' ') text.erase(0, 1);
        for (const char c : text) {
            const auto key = key_for(c);
            if (!key) throw std::runtime_error("text has a character without a key");
            request.keys.push_back({std::uint32_t(key->first) << 8 | std::uint8_t(c), std::uint16_t(key->second ? shift : 0)});
        }
        if (request.keys.empty()) throw std::runtime_error("text is empty");
    } else if (verb == "snapshot") {
        request.kind = AutomationRequest::Kind::snapshot;
        input >> request.name;
        if (!valid_name(request.name)) throw std::runtime_error("snapshot needs a short lowercase name");
    } else if (verb == "profile") {
        request.kind = AutomationRequest::Kind::profile;
        std::string action;
        input >> action >> request.name;
        if (action == "start" && request.name.empty()) {}
        else if (action != "stop" || !valid_name(request.name)) throw std::runtime_error("profile needs start, or stop and a short lowercase name");
    } else if (verb == "capture") {
        request.kind = AutomationRequest::Kind::capture;
        std::string action;
        input >> action >> request.name;
        if (action == "stop" && request.name.empty()) {}
        else if (action != "start" || !valid_name(request.name)) throw std::runtime_error("capture needs start and a short lowercase name, or stop");
    } else if (verb == "memory") {
        request.kind = AutomationRequest::Kind::memory;
        std::string address, size;
        input >> request.name >> address >> size;
        if (!valid_name(request.name)) throw std::runtime_error("memory needs a short lowercase name");
        try {
            request.address = std::uint32_t(std::stoul(address, nullptr, 0));
            request.size = std::uint32_t(std::stoul(size, nullptr, 0));
        } catch (const std::exception&) { throw std::runtime_error("memory needs an address and a size"); }
        if (!request.size || request.size > 0x1000000) throw std::runtime_error("memory size must be 1 byte to 16 MiB");
    } else throw std::runtime_error("unknown request " + verb);
    return request;
}
void replace(const std::filesystem::path& path, const std::string& text) {
    const auto temporary = path.parent_path() / ("." + path.filename().string() + ".tmp");
    {
        std::ofstream output(temporary, std::ios::binary | std::ios::trunc);
        output << text;
        if (!output) throw std::runtime_error("Cannot write automation file " + temporary.string());
    }
    std::filesystem::rename(temporary, path);
}
}
std::unique_ptr<Automation> Automation::from_environment() {
    const char* directory = std::getenv("CM_AOT_AUTOMATION");
    if (!directory || !*directory) return nullptr;
    if (!std::filesystem::is_directory(directory)) throw std::runtime_error("CM_AOT_AUTOMATION is not a directory");
    return std::make_unique<Automation>(directory);
}
Automation::Automation(std::filesystem::path directory) : directory_(std::move(directory)) {
    trace_ = std::getenv("CM_AOT_AUTOMATION_TRACE") != nullptr;
    if (const char* work = std::getenv("CM_AOT_AUTOMATION_IDLE_WORK")) idle_work_ = std::strtoull(work, nullptr, 10);
    std::filesystem::create_directories(directory_ / "inbox");
    std::fprintf(stderr, "Automation channel: %s\n", directory_.c_str());
}
std::filesystem::path Automation::output(std::string_view kind, std::string_view name) const {
    return directory_ / kind / name;
}
void Automation::enter(std::uint64_t executed) {
    const auto now = std::chrono::steady_clock::now();
    entered_ = now; waiting_ = true;
    if (left_ && executed >= left_) most_work_ = std::max(most_work_, executed - left_);
    ++calls_;
    evaluate(now);
}
void Automation::leave(std::uint64_t executed) {
    waited_ += std::chrono::steady_clock::now() - std::max(entered_, window_);
    waiting_ = false; left_ = executed;
}
void Automation::evaluate(std::chrono::steady_clock::time_point now) {
    // Judge idleness over 250 ms windows. The original main loop polls with
    // no sleep, so time spent waiting proves little; idle means at least
    // half of the window passed inside event waits, or every gap between
    // event calls stayed short. Busy ends where the first idle window began.
    if (now - window_ < std::chrono::milliseconds(250)) return;
    const auto waited = waited_ + (waiting_ ? now - std::max(entered_, window_) : std::chrono::steady_clock::duration{});
    const bool idle = waited * 2 >= now - window_ || (calls_ >= 2 && most_work_ < idle_work_);
    if (trace_)
        std::fprintf(stderr, "Automation window: calls=%llu most_work=%llu waited=%.0f%% idle=%d\n",
            static_cast<unsigned long long>(calls_), static_cast<unsigned long long>(most_work_),
            100 * std::chrono::duration<double>(waited).count() / std::chrono::duration<double>(now - window_).count(), idle);
    if (idle && !idle_) idle_since_ = window_;
    idle_ = idle; waited_ = {}; window_ = now; most_work_ = 0; calls_ = 0;
}
void Automation::boundary(const State& s, const CommandContext& context, std::string_view status) {
    // The game polls thousands of times a second while it works; the date
    // changes at most once per game day. Read it every 50 ms, or on a change.
    const auto now = std::chrono::steady_clock::now();
    if (now >= next_date_ || (context.ready && idle_) != ready_ || context.identity != identity_ || status != status_ ||
        changes_ != written_) {
        next_date_ = now + std::chrono::milliseconds(50);
        date_value_ = game_date(s);
    }
    const auto& date = date_value_;
    const auto packed = date ? (std::uint64_t((*date)[1]) << 32 | std::uint64_t((*date)[0]) << 16 | (*date)[2]) : 0;
    if (const bool ready = context.ready && idle_; ready != ready_) {
        if (ready) last_busy_ = std::max(0.0, std::chrono::duration<double>(idle_since_ - since_).count());
        since_ = ready ? idle_since_ : now; ready_ = ready; ++changes_;
    }
    if (context.identity != identity_ || packed != date_ || status != status_) {
        identity_ = context.identity; date_ = packed; status_ = status; ++changes_;
    }
    if (changes_ == written_) return;
    written_ = changes_;
    std::ostringstream json;
    json << "{\n  \"pid\": " << getpid()
         << ",\n  \"time\": " << std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::system_clock::now().time_since_epoch()).count()
         << ",\n  \"ready\": " << (ready_ ? "true" : "false")
         << ",\n  \"last_busy_seconds\": " << last_busy_
         << ",\n  \"status\": " << quoted(status)
         << ",\n  \"done\": " << done_
         << ",\n  \"result\": " << quoted(last_result_);
    if (date) json << ",\n  \"date\": {\"day\": " << (*date)[0] << ", \"year\": " << (*date)[1] << ", \"phase\": " << (*date)[2] << "}";
    if (context.ready) {
        json << ",\n  \"handler\": \"" << hex(context.handler) << "\", \"screen\": \"" << hex(context.screen)
             << "\", \"context\": \"" << hex(context.identity, 16) << "\",\n  \"commands\": {";
        bool first = true;
        for (std::size_t i = 0; i < direct_command_count; ++i) {
            if (!context.actions[i]) continue;
            json << (first ? "" : ", ") << quoted(direct_command_names[i]) << ": " << context.controls[i];
            first = false;
        }
        json << "},\n  \"controls\": [";
        first = true;
        try {
            for (const auto& control : original_controls(s, context)) {
                json << (first ? "\n    " : ",\n    ") << "[" << control.index << ", " << control.action << ", " << control.flags << "]";
                first = false;
            }
        } catch (const std::exception&) {}
        json << "\n  ]";
    }
    json << "\n}\n";
    try { replace(directory_ / "status.json", json.str()); }
    catch (const std::exception& failure) { std::fprintf(stderr, "Automation status: %s\n", failure.what()); }
}
std::optional<AutomationRequest> Automation::take(const CommandContext& context) {
    const auto now = std::chrono::steady_clock::now();
    evaluate(now);
    if (!pending_) {
        if (now < next_scan_) return std::nullopt;
        next_scan_ = now + std::chrono::milliseconds(20);
        std::optional<std::pair<std::uint64_t, std::filesystem::path>> oldest;
        std::error_code error;
        for (const auto& entry : std::filesystem::directory_iterator(directory_ / "inbox", error)) {
            const auto path = entry.path();
            if (path.extension() != ".req") continue;
            const auto stem = path.stem().string();
            if (stem.empty() || stem.size() > 18 || !std::all_of(stem.begin(), stem.end(), [](char c) { return c >= '0' && c <= '9'; })) continue;
            const auto sequence = std::stoull(stem);
            if (!oldest || sequence < oldest->first) oldest = {sequence, path};
        }
        if (!oldest) return std::nullopt;
        std::string line;
        std::getline(std::ifstream(oldest->second) >> std::ws, line);
        std::filesystem::remove(oldest->second, error);
        AutomationRequest request;
        request.sequence = oldest->first;
        try {
            if (request.sequence <= done_) throw std::runtime_error("sequence " + std::to_string(request.sequence) + " is not after " + std::to_string(done_));
            request = parse(oldest->first, line);
        } catch (const std::exception& failure) {
            finish(request, false, failure.what());
            return std::nullopt;
        }
        pending_.emplace(std::move(request), std::move(line));
    }
    // Snapshots and memory reads work at any event boundary; everything
    // else waits until the game idles at an interactive screen.
    if (pending_->first.input() && !(context.ready && idle_)) return std::nullopt;
    auto [request, line] = std::move(*pending_);
    pending_.reset();
    std::fprintf(stderr, "Automation request %llu: %s\n", static_cast<unsigned long long>(request.sequence), line.c_str());
    return request;
}
void Automation::finish(const AutomationRequest& request, bool accepted, std::string_view result) {
    done_ = std::max(done_, request.sequence);
    // An accepted input starts work: the screen is not ready again until
    // the game has idled through a fresh window.
    if (accepted && request.input()) {
        idle_ = false; waited_ = {}; window_ = std::chrono::steady_clock::now(); most_work_ = 0; calls_ = 0;
    }
    last_result_ = std::string(accepted ? "ok" : "refused") + (result.empty() ? "" : ": ") + std::string(result);
    ++changes_;
    std::ofstream log(directory_ / "log.txt", std::ios::app);
    log << request.sequence << ' ' << last_result_ << '\n';
}
}
