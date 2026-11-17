#pragma once
#include <exception>
#include <functional>
#include <optional>
#include <type_traits>
#include <utility>

namespace cm::aot {
bool on_main_thread() noexcept;
// Only the caller waits. The main thread must never wait for game work.
// Exceptions are transported back to the caller, never through libdispatch.
void main_thread_invoke(void* context, void (*invoke)(void*) noexcept);
template<class F> auto on_main(F&& function) -> std::invoke_result_t<F> {
    using Result = std::invoke_result_t<F>;
    if (on_main_thread()) return std::invoke(std::forward<F>(function));
    static_assert(!std::is_reference_v<Result>);
    std::exception_ptr error;
    if constexpr (std::is_void_v<Result>) {
        auto call = [&]() noexcept { try { std::invoke(function); } catch (...) { error = std::current_exception(); } };
        main_thread_invoke(&call, [](void* value) noexcept { (*static_cast<decltype(call)*>(value))(); });
        if (error) std::rethrow_exception(error);
    } else {
        std::optional<Result> result;
        auto call = [&]() noexcept { try { result.emplace(std::invoke(function)); } catch (...) { error = std::current_exception(); } };
        main_thread_invoke(&call, [](void* value) noexcept { (*static_cast<decltype(call)*>(value))(); });
        if (error) std::rethrow_exception(error);
        return std::move(*result);
    }
}
// Run one sequential game owner with at least the main thread's stack budget.
// `service` runs on main until the worker (including its destructors) finishes.
int run_game_thread(std::function<int()> game, const std::function<void()>& service);
bool game_thread_active() noexcept;
// Headless fixtures use the same executor without creating an application.
void service_main_queue();
}
