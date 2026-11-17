#pragma once
#include "main_thread.hpp"
#include "profiling.hpp"

// Two distinct scopes: the request lives on its caller's thread, and execution
// lives on main. Construct the original callable on the caller and keep it alive
// until synchronous dispatch returns, exactly as on_main does. Static names let
// Tracy aggregate operations without allocating dynamic text for every call.
#if CM_ENABLE_TRACY
#define CM_PROFILE_ON_MAIN(operation, ...) \
    [&]<class F>(F&& function) -> std::invoke_result_t<F> { \
        CM_PROFILE_ZONE("UI request: " operation); \
        const bool caller_is_main = ::cm::aot::on_main_thread(); \
        return ::cm::aot::on_main([&]() -> std::invoke_result_t<F> { \
            CM_PROFILE_ZONE("UI execution: " operation); \
            if (caller_is_main) return std::invoke(std::forward<F>(function)); \
            return std::invoke(function); \
        }); \
    }(__VA_ARGS__)
#else
#define CM_PROFILE_ON_MAIN(operation, ...) ::cm::aot::on_main(__VA_ARGS__)
#endif
