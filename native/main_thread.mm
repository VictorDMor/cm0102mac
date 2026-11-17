#include "main_thread.hpp"
#include "profiling.hpp"
#import <Foundation/Foundation.h>
#include <dispatch/dispatch.h>
#include <pthread.h>
#include <algorithm>
#include <atomic>
#include <future>
#include <stdexcept>
#include <system_error>

namespace cm::aot {
namespace { std::atomic<bool> active{false}; }
bool on_main_thread() noexcept { return pthread_main_np() != 0; }
bool game_thread_active() noexcept { return active.load(std::memory_order_acquire); }
void service_main_queue() {
    if (!on_main_thread()) throw std::logic_error("Main queue service must run on main");
    @autoreleasepool { CFRunLoopRunInMode(kCFRunLoopDefaultMode, 0.008, true); }
}
void main_thread_invoke(void* context, void (*invoke)(void*) noexcept) {
    if (on_main_thread()) return invoke(context);
    if (!game_thread_active()) throw std::logic_error("No main-thread host is servicing game requests");
    CM_PROFILE_ZONE("Main dispatch round trip");
    struct Call { void* context; void (*invoke)(void*) noexcept; } call{context, invoke};
    dispatch_sync_f(dispatch_get_main_queue(), &call, [](void* value) {
        CM_PROFILE_ZONE("Main dispatch execution");
        @autoreleasepool {
            auto& call = *static_cast<Call*>(value);
            call.invoke(call.context);
        }
    });
}
int run_game_thread(std::function<int()> game, const std::function<void()>& service) {
    if (!on_main_thread()) throw std::logic_error("The game host must run on the main thread");
    if (active.exchange(true)) throw std::logic_error("A game worker is already running");
    CM_PROFILE_THREAD("CM AppKit host");
    struct ActiveScope { ~ActiveScope() { active.store(false, std::memory_order_release); } } scope;
    struct Work {
        std::function<int()> game;
        std::promise<int> result;
        std::atomic<bool> finished{false};
    } work{std::move(game), {}};
    // std::function's moved-from state is unspecified. libc++ may retain a
    // small callable there; make the worker its sole owner before starting.
    game = {};
    auto result = work.result.get_future();
    pthread_attr_t attributes;
    if (const auto error = pthread_attr_init(&attributes)) throw std::system_error(error, std::generic_category(), "Worker attributes");
    // A saved original callback context alone occupies ~300 KiB. Darwin's
    // secondary-thread default is too small for the nested native call chain.
    const auto stack = std::max<std::size_t>(8 * 1024 * 1024, pthread_get_stacksize_np(pthread_self()));
    const auto stack_error = pthread_attr_setstacksize(&attributes, stack);
    pthread_t worker{};
    const auto error = stack_error ? stack_error : pthread_create(&worker, &attributes, [](void* value) -> void* {
        auto& work = *static_cast<Work*>(value);
        pthread_setname_np("CM game and simulation");
        CM_PROFILE_THREAD("CM game and simulation");
        @autoreleasepool {
            try { work.result.set_value(work.game()); }
            catch (...) { work.result.set_exception(std::current_exception()); }
            // Captured owners can have UI-aware destructors too. Release the
            // callable on the game thread before main is allowed to join.
            // packaged_task is unsuitable here: its future may retain the
            // callable's shared state until main consumes the result.
            work.game = {};
        }
        work.finished.store(true, std::memory_order_release);
        return nullptr;
    }, &work);
    pthread_attr_destroy(&attributes);
    if (error) throw std::system_error(error, std::generic_category(), "Game worker");
    // Do not join until diagnostics, modal requests and all game-owned
    // destructors have completed. They can still need the main dispatch queue.
    std::exception_ptr host_error;
    while (!work.finished.load(std::memory_order_acquire)) {
        try { service(); }
        catch (...) {
            // Report the first failure when it happens, not at quit, and keep
            // the main queue draining: a service that throws before running
            // its loop would otherwise spin here while the worker waits on a
            // dispatch_sync that never runs.
            if (!host_error) {
                host_error = std::current_exception();
                try { std::rethrow_exception(host_error); }
                catch (const std::exception& error) { NSLog(@"Host service failed: %s", error.what()); }
                catch (...) { NSLog(@"Host service failed"); }
            }
            // Nothing may leave this loop while the worker still runs.
            try { service_main_queue(); } catch (...) {}
        }
    }
    pthread_join(worker, nullptr);
    if (host_error) std::rethrow_exception(host_error);
    return result.get();
}
}
