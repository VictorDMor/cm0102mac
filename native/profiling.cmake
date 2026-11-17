option(CM_ENABLE_TRACY "Instrument Continue and native presentation with Tracy" OFF)

# Keep compiler flags unchanged across OFF/ON. Unix Makefiles share flags.make
# across a target: conditional per-source defines would invalidate all ~1,900
# generated pages. Only these eleven files depend on the configured header.
set(cm_profile_sources main_thread.mm ui_bridge.mm ui_event_queue.cpp
    platform.cpp presentation_frame.cpp screen_manager_monitor.cpp
    print_panels.mm thread_host_checks.mm
    "${CM_GENERATED_DIR}/page_102a6.cpp"
    "${CM_GENERATED_DIR}/page_10119.cpp"
    "${CM_GENERATED_DIR}/page_1076e.cpp")
set(cm_profile_config_dir "${CMAKE_CURRENT_BINARY_DIR}/cm-profile-config")
set_property(SOURCE ${cm_profile_sources} APPEND PROPERTY INCLUDE_DIRECTORIES "${cm_profile_config_dir}")

# Explicit CPU zones only; the game's SIGPROF and hardware-fault handlers retain
# ownership. A collector must connect locally before events are retained.
set(cm_tracy_definitions TRACY_ENABLE TRACY_ON_DEMAND TRACY_ONLY_LOCALHOST
    TRACY_NO_BROADCAST TRACY_NO_CODE_TRANSFER TRACY_NO_CRASH_HANDLER
    TRACY_NO_SYSTEM_TRACING TRACY_NO_SAMPLING TRACY_NO_CONTEXT_SWITCH
    TRACY_NO_CALLSTACK TRACY_NO_FRAME_IMAGE)
set(cm_tracy_config_definitions "")
foreach(definition IN LISTS cm_tracy_definitions)
    string(APPEND cm_tracy_config_definitions "#define ${definition}\n")
endforeach()
configure_file(profiling_config.hpp.in "${cm_profile_config_dir}/cm_profiling_config.hpp" @ONLY)

if(CM_ENABLE_TRACY)
    # Upstream's documented single-file integration; the unmodified client and
    # all its compile settings remain private to this optional static library.
    add_library(cm_tracy STATIC vendor/tracy/public/TracyClient.cpp)
    target_compile_definitions(cm_tracy PRIVATE ${cm_tracy_definitions})
    find_package(Threads REQUIRED)
    target_link_libraries(cm_tracy PRIVATE Threads::Threads ${CMAKE_DL_LIBS})
    target_link_libraries(cm_ui PRIVATE cm_tracy)
    target_link_libraries(cm_aot PRIVATE cm_tracy)
endif()
