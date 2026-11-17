#pragma once
#include "cm_profiling_config.hpp"

// Only the explicitly instrumented translation units include this configuration.
// In ordinary builds even the arguments disappear; no client, clocks, strings,
// allocations or additional guest-memory reads are introduced.
#if CM_ENABLE_TRACY
#include "vendor/tracy/public/tracy/Tracy.hpp"
#define CM_PROFILE_ZONE(name) ZoneScopedN(name)
#define CM_PROFILE_VALUE(value) ZoneValue(value)
#define CM_PROFILE_EVENT(name) TracyMessageL(name)
#define CM_PROFILE_EVENT_IF(condition, name) do { if (condition) CM_PROFILE_EVENT(name); } while (false)
#define CM_PROFILE_FRAME(name) FrameMarkNamed(name)
#define CM_PROFILE_THREAD(name) tracy::SetThreadName(name)
#else
#define CM_PROFILE_ZONE(name) ((void)0)
#define CM_PROFILE_VALUE(value) ((void)0)
#define CM_PROFILE_EVENT(name) ((void)0)
#define CM_PROFILE_EVENT_IF(condition, name) ((void)0)
#define CM_PROFILE_FRAME(name) ((void)0)
#define CM_PROFILE_THREAD(name) ((void)0)
#endif
