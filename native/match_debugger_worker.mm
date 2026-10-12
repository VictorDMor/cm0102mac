#include "match_debugger_worker.hpp"
#include "match_debugger_factory.hpp"
#include "match_debugger_database.hpp"
#include "match_engine_adapter.hpp"
#import <Foundation/Foundation.h>
#include <algorithm>
#include <chrono>
#include <cstdio>
#include <cstring>
#include <poll.h>
#include <signal.h>
#include <unistd.h>

namespace {
using namespace cm::aot::match_debugger;
namespace fs = std::filesystem;

NSString* text(std::string_view value) {
    return [[NSString alloc] initWithBytes:value.data() length:value.size() encoding:NSUTF8StringEncoding];
}
void send(NSDictionary* value) {
    NSError* error = nil;
    NSData* bytes = [NSJSONSerialization dataWithJSONObject:value options:0 error:&error];
    if (!bytes) throw std::runtime_error("Cannot encode engine worker response");
    if (std::fwrite(bytes.bytes, 1, bytes.length, stdout) != bytes.length || std::fputc('\n', stdout) == EOF ||
        std::fflush(stdout)) throw std::runtime_error("Engine worker response pipe closed");
}
std::uint32_t integer(NSDictionary* value, NSString* key) {
    id field = value[key];
    if (![field isKindOfClass:NSNumber.class] || CFGetTypeID((__bridge CFTypeRef)field) == CFBooleanGetTypeID() ||
        [field doubleValue] < 0 || [field doubleValue] > UINT32_MAX ||
        [field doubleValue] != [field unsignedLongLongValue])
        throw std::invalid_argument("Command requires unsigned integer club IDs and a nonzero seed");
    return [field unsignedIntValue];
}
NSString* execution(ExecutionState state) {
    switch (state) {
    case ExecutionState::paused: return @"paused";
    case ExecutionState::running: return @"running";
    case ExecutionState::completed: return @"completed";
    case ExecutionState::aborted: return @"aborted";
    case ExecutionState::faulted: return @"faulted";
    }
    throw std::logic_error("Unknown debugger execution state");
}

// This directory is created by this worker under its caller-owned empty root.
// Never follow links or remove an input directory during experiment cleanup.
void remove_backing(const fs::path& root, const fs::path& directory) {
    if (directory.empty()) return;
    if (directory.parent_path() != root || !directory.filename().string().starts_with("experiment-") ||
        fs::is_symlink(fs::symlink_status(directory)))
        throw std::runtime_error("Unsafe debugger backing cleanup target");
    if (!fs::exists(directory)) return;
    for (const auto& entry : fs::recursive_directory_iterator(directory))
        if (entry.is_symlink()) throw std::runtime_error("Debugger backing contains a link");
    fs::remove_all(directory);
}

NSString* original_string(const cm::aot::Memory& memory, std::uint32_t address) {
    if (!address) return @"";
    std::array<std::uint8_t, 105> bytes{};
    std::size_t count = 0;
    for (; count < bytes.size(); ++count) {
        bytes[count] = std::uint8_t(memory.read<1>(address + count));
        if (!bytes[count]) break;
    }
    if (count == bytes.size()) throw std::runtime_error("Unterminated original player name");
    NSString* result = [[NSString alloc] initWithBytes:bytes.data() length:count encoding:NSWindowsCP1252StringEncoding];
    if (!result) throw std::runtime_error("Invalid original player name encoding");
    return result;
}

NSArray* players(const Session& session) {
    NSMutableArray* result = [NSMutableArray new];
    const auto& memory = session.state().memory;
    for (unsigned team = 0; team < 2; ++team) {
        for (unsigned actor = 0; actor < 20; ++actor) {
            const auto record = session.match_address() + 0x311c + (team * 20 + actor) * 0x1c4;
            const auto staff = std::uint32_t(memory.read<4>(record + 0x6c));
            if (!staff) continue;
            // Original name formatter 100caeb0 reads these resolved string
            // pointers: first name +28, surname +2c, common name +30.
            NSString* common = original_string(memory, std::uint32_t(memory.read<4>(staff + 0x30)));
            NSString* name = common.length ? common : [NSString stringWithFormat:@"%@ %@",
                original_string(memory, std::uint32_t(memory.read<4>(staff + 0x28))),
                original_string(memory, std::uint32_t(memory.read<4>(staff + 0x2c)))];
            const auto position = int(cm::aot::signed_value<1>(memory.read<1>(record + 0x19)));
            [result addObject:@{@"team": @(team), @"actor": @(actor), @"position": @(position),
                @"active": @(position >= 0 && position < 11),
                @"staffID": @(memory.read<4>(staff)), @"name": [name stringByTrimmingCharactersInSet:NSCharacterSet.whitespaceCharacterSet]}];
        }
    }
    return result;
}

class Worker {
    fs::path program_, game_, root_, active_backing_;
    DatabaseCatalog catalog_;
    std::unique_ptr<Session> session_;
    MatchSetup setup_;
    std::uint64_t generation_ = 0, attempt_ = 0;
    NSMutableArray* events_ = [NSMutableArray new];
public:
    Worker(fs::path program, fs::path game, fs::path root)
        : program_(fs::canonical(program)), game_(fs::canonical(game)), root_(fs::canonical(root)),
          catalog_(DatabaseCatalog::load(game_)) {
        if (!fs::is_directory(root_) || !fs::is_empty(root_) || fs::is_symlink(fs::symlink_status(root)))
            throw std::invalid_argument("Engine worker requires an empty private directory");
        for (const auto& input : {program_, game_}) {
            const auto relative = root_.lexically_relative(input);
            const auto inverse = input.lexically_relative(root_);
            if (relative.empty() || *relative.begin() != ".." || inverse.empty() || *inverse.begin() != "..")
                throw std::invalid_argument("Engine worker backing must be independent of inputs");
        }
    }
    ~Worker() {
        session_.reset();
        try { remove_backing(root_, active_backing_); }
        catch (const std::exception& error) { std::fprintf(stderr, "%s\n", error.what()); }
    }
    bool running() const { return session_ && session_->execution_state() == ExecutionState::running; }
    void event(NSString* description) {
        [events_ addObject:@{@"step": @(session_ ? session_->steps().size() : 0), @"description": description}];
    }
    void create(const MatchSetup& requested) {
        // Construct first. Failure leaves the previous experiment paused and
        // inspectable; a successful replacement releases its files immediately.
        if (session_) session_->pause();
        for (auto id : {requested.home_club, requested.away_club})
            if (catalog_.club(id).kind != ClubKind::club) throw std::invalid_argument("Select actual clubs for this experiment");
        const auto next = root_ / ("experiment-" + std::to_string(++attempt_));
        std::unique_ptr<Session> replacement;
        try {
            replacement = std::make_unique<Session>([&, next, requested] {
                return create_match({program_, game_, next}, requested, [](std::string_view stage) {
                    send(@{@"type": @"progress", @"message": text(stage)});
                });
            }, fresh_match_step_limit);
        } catch (...) { remove_backing(root_, next); throw; }
        session_ = std::move(replacement);
        remove_backing(root_, active_backing_);
        active_backing_ = next;
        generation_ = attempt_;
        setup_ = requested;
        [events_ removeAllObjects];
        event(@"Fresh actual match created and paused before kickoff");
    }
    void snapshot(std::uint32_t request = 0) {
        if (!session_) { send(@{@"type": @"state", @"status": @"empty", @"requestID": @(request)}); return; }
        const auto value = session_->inspect();
        const auto& diagnostics = cm::aot::match_engine_diagnostics(session_->state());
        const auto& steps = session_->steps();
        send(@{@"type": @"state", @"status": execution(session_->execution_state()), @"requestID": @(request),
            @"generation": @(generation_), @"homeID": @(setup_.home_club), @"awayID": @(setup_.away_club),
            @"seed": @(setup_.random_seed), @"engineUnits": @(value.elapsed_engine_units),
            @"homeScore": @(value.home_score), @"awayScore": @(value.away_score),
            @"phase": @(value.phase), @"interruption": @(value.interruption), @"transition": @(value.transition),
            @"actorQueue": @(value.actor_queue_count), @"stepCount": @(steps.size()),
            @"lastInstructions": @(steps.empty() ? 0 : steps.back().instructions),
            @"sharedModernCalls": @(diagnostics.calls[2]), @"failure": text(session_->failure()),
            @"players": players(*session_), @"events": [events_ copy]});
        [events_ removeAllObjects];
    }
    bool command(NSDictionary* value, std::uint32_t request) {
        id operation = value[@"command"];
        if (![operation isKindOfClass:NSString.class]) throw std::invalid_argument("Missing worker command");
        NSMutableSet* keys = [NSMutableSet setWithArray:[operation isEqual:@"create"] ?
            @[@"command", @"homeID", @"awayID", @"seed"] : @[@"command"]];
        if (value[@"requestID"]) [keys addObject:@"requestID"];
        if (![[NSSet setWithArray:value.allKeys] isEqual:keys]) throw std::invalid_argument("Unexpected command fields");
        if ([operation isEqual:@"quit"]) return false;
        if ([operation isEqual:@"create"]) create({integer(value, @"homeID"), integer(value, @"awayID"), integer(value, @"seed"), 2001, 194});
        else {
            if (!session_) throw std::logic_error("Create a match before using its controls");
            if ([operation isEqual:@"step"]) { record(session_->step()); }
            else if ([operation isEqual:@"run"]) { session_->run(); event(@"Continuous engine execution started"); }
            else if ([operation isEqual:@"pause"]) {
                if (session_->execution_state() == ExecutionState::running) { session_->pause(); event(@"Engine execution paused"); }
            }
            else if ([operation isEqual:@"reset"]) create(setup_);
            else if (![operation isEqual:@"inspect"]) throw std::invalid_argument("Unknown worker command");
        }
        snapshot(request);
        return true;
    }
    void advance() { if (auto step = session_->advance_if_running()) record(*step); }
    void record(const StepRecord& step) {
        if (step.before.home_score != step.after.home_score || step.before.away_score != step.after.away_score)
            event([NSString stringWithFormat:@"Engine score changed: %d–%d → %d–%d (unit %d)",
                int(step.before.home_score), int(step.before.away_score), int(step.after.home_score), int(step.after.away_score), int(step.after.elapsed_engine_units)]);
        if (step.before.phase != step.after.phase || step.before.interruption != step.after.interruption || step.before.transition != step.after.transition)
            event([NSString stringWithFormat:@"Engine state changed: phase %d, interruption %d, transition %d (unit %d)",
                int(step.after.phase), int(step.after.interruption), int(step.after.transition), int(step.after.elapsed_engine_units)]);
        if (step.engine_return != 1) event([NSString stringWithFormat:@"Engine returned %u; session %@", step.engine_return, execution(session_->execution_state())]);
    }
};
}

int run_match_debugger_worker(int argc, char** argv) {
    @autoreleasepool {
        try {
            if (argc != 5) throw std::invalid_argument("Expected --engine-worker original-program original-game private-root");
            signal(SIGPIPE, SIG_IGN);
            Worker worker(argv[2], argv[3], argv[4]);
            send(@{@"type": @"ready"});
            std::string input;
            auto reported = std::chrono::steady_clock::now();
            auto next_step = reported;
            bool alive = true;
            while (alive) {
                @autoreleasepool {
                    pollfd pipe{STDIN_FILENO, POLLIN, 0};
                    const auto delay = std::chrono::duration_cast<std::chrono::milliseconds>(next_step - std::chrono::steady_clock::now()).count();
                    const auto available = poll(&pipe, 1, worker.running() ? int(std::clamp<std::int64_t>(delay + 1, 0, 34)) : -1);
                    if (available < 0) { if (errno == EINTR) continue; throw std::runtime_error("Cannot poll engine command pipe"); }
                    if (available && (pipe.revents & (POLLIN | POLLHUP))) {
                        std::array<char, 4096> bytes;
                        const auto size = read(STDIN_FILENO, bytes.data(), bytes.size());
                        if (!size) break;
                        if (size < 0) { if (errno == EINTR) continue; throw std::runtime_error("Cannot read engine command pipe"); }
                        input.append(bytes.data(), std::size_t(size));
                        if (input.size() > 65536) throw std::runtime_error("Engine command exceeded its input limit");
                        for (auto newline = input.find('\n'); newline != std::string::npos; newline = input.find('\n')) {
                            NSData* data = [NSData dataWithBytes:input.data() length:newline];
                            input.erase(0, newline + 1);
                            std::uint32_t request = 0;
                            try {
                                NSError* error = nil;
                                id value = [NSJSONSerialization JSONObjectWithData:data options:0 error:&error];
                                if (![value isKindOfClass:NSDictionary.class]) throw std::invalid_argument("Malformed engine command JSON");
                                if (value[@"requestID"]) request = integer(value, @"requestID");
                                alive = worker.command(value, request);
                            } catch (const std::exception& error) {
                                send(@{@"type": @"error", @"message": text(error.what()), @"requestID": @(request)});
                                worker.snapshot(request);
                            }
                            if (!alive) break;
                        }
                    }
                    if (available && (pipe.revents & (POLLERR | POLLNVAL))) throw std::runtime_error("Engine command pipe failed");
                    if (!alive) break;
                    if (worker.running()) {
                        const auto now = std::chrono::steady_clock::now();
                        if (now >= next_step) {
                            try { worker.advance(); }
                            catch (const std::exception& error) { send(@{@"type": @"error", @"message": text(error.what())}); }
                            next_step = std::chrono::steady_clock::now() + std::chrono::milliseconds(33);
                        }
                        if (!worker.running() || now - reported >= std::chrono::milliseconds(100)) { worker.snapshot(); reported = now; }
                    }
                }
            }
            send(@{@"type": @"closed"});
            return 0;
        } catch (const std::exception& error) {
            std::fprintf(stderr, "%s\n", error.what());
            return 1;
        }
    }
}
