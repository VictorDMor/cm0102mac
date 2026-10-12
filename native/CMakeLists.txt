cmake_minimum_required(VERSION 3.28)
project(CM0102AheadOfTime LANGUAGES CXX OBJCXX)
if(NOT APPLE OR NOT CMAKE_SYSTEM_PROCESSOR MATCHES "arm64|aarch64")
    message(FATAL_ERROR "The native port experiment requires Apple Silicon")
endif()
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_OBJCXX_STANDARD 23)
set(CMAKE_OBJCXX_STANDARD_REQUIRED ON)
set(CMAKE_OSX_ARCHITECTURES arm64)
set(CM_GENERATED_DIR "" CACHE PATH "Output of tools/compile_recomp.py")
if(NOT EXISTS "${CM_GENERATED_DIR}/compilation.json")
    message(FATAL_ERROR "Set CM_GENERATED_DIR to a completed AOT compilation")
endif()
file(GLOB CM_GENERATED_SOURCES CONFIGURE_DEPENDS "${CM_GENERATED_DIR}/*.cpp")
add_library(cm_audio STATIC audio_bridge.mm audio_samples.cpp)
target_include_directories(cm_audio PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")
target_compile_options(cm_audio PRIVATE -Wall -Wextra -Werror -Wpedantic "$<$<COMPILE_LANGUAGE:OBJCXX>:-fobjc-arc>")
target_link_libraries(cm_audio PUBLIC "-framework AVFAudio" "-framework Foundation")
add_library(cm_ui STATIC ui_bridge.mm native_drawing_view.mm presentation.cpp menu_resource.cpp regions.cpp cursor_resource.cpp)
target_compile_options(cm_ui PRIVATE -Wall -Wextra -Werror -Wpedantic -fobjc-arc)
target_link_libraries(cm_ui PUBLIC "-framework AppKit" "-framework Carbon" "-framework CoreText")
add_library(cm_aot STATIC cpp_object_storage_adapter.cpp cpp_object_storage_arrays.cpp cpp_object_storage_allocation.cpp code_fragment_adapter.cpp code_fragment_registration.cpp code_fragment_lookup.cpp code_fragment_metadata.cpp runtime_lifecycle_adapter.cpp runtime_lifecycle_objects.cpp runtime_lifecycle_exit.cpp runtime_lifecycle_signal.cpp file_path_adapter.cpp file_path_names.cpp file_path_specs.cpp file_path_resolution.cpp file_controller_adapter.cpp file_controller_state.cpp file_controller_streams.cpp file_controller_paths.cpp file_controller_forks.cpp file_reference_bridge.cpp unicode_bridge.cpp fork_io_bridge.cpp stream_io_adapter.cpp stream_io_characters.cpp stream_io_read.cpp stream_io_write.cpp stream_buffer_adapter.cpp stream_buffer_storage.cpp stream_buffer_transfer.cpp stream_buffer_lifecycle.cpp allocator_adapter.cpp allocator_arenas.cpp allocator_pools.cpp allocator_facades.cpp formatted_output_adapter.cpp formatted_output_format.cpp formatted_output_numbers.cpp formatted_output_float.cpp formatted_output_descriptor.cpp formatted_output_facades.cpp floating_input_adapter.cpp decimal_input_bridge.cpp character_conversion_adapter.cpp formatted_input_adapter.cpp integer_parsing_adapter.cpp memory_primitives_adapter.cpp string_core_adapter.cpp collection_lookup_adapter.cpp background_selection_adapter.cpp rng_core_adapter.cpp date_subsystem_adapter.cpp date_subsystem_audit.cpp calendar_adapter.cpp aot_runtime.cpp aot_integrity.cpp performance_trace.cpp directory_catalog.cpp floating_point.cpp decimal_bridge.cpp carbon_bridge.cpp platform.cpp native_drawing.cpp direct_commands.cpp sound_bridge.cpp text_bridge.cpp process_bridge.cpp resource_bridge.cpp filesystem_bridge.cpp printing_bridge.cpp callbacks.cpp apple_events.cpp data_forks.cpp match_replay.cpp season_reference.cpp ${CM_GENERATED_SOURCES})
target_sources(cm_aot PRIVATE vector_context.cpp)
target_sources(cm_aot PRIVATE guest_context_transfer.cpp)
target_sources(cm_aot PRIVATE exception_runtime_adapter.cpp exception_runtime_context.cpp exception_runtime_frames.cpp exception_runtime_cleanup.cpp exception_runtime_matching.cpp exception_runtime_search.cpp exception_runtime_capture.cpp exception_runtime_handlers.cpp exception_runtime_unexpected.cpp)
target_sources(cm_aot PRIVATE compiler_arithmetic_adapter.cpp compiler_arithmetic_integer.cpp compiler_arithmetic_conversion.cpp)
target_sources(cm_aot PRIVATE register_context.cpp register_context_adapter.cpp)
target_sources(cm_aot PRIVATE stdlib_core.cpp stdlib_core_adapter.cpp)
target_sources(cm_aot PRIVATE time_runtime.cpp time_runtime_adapter.cpp)
target_sources(cm_aot PRIVATE import_descriptor.cpp import_descriptor_adapter.cpp)
target_sources(cm_aot PRIVATE native_service.cpp native_service_adapter.cpp)
target_sources(cm_aot PRIVATE engine_initialization.cpp engine_initialization_adapter.cpp runtime_diagnostics.cpp)
target_sources(cm_aot PRIVATE utility_object.cpp utility_object_adapter.cpp)
target_sources(cm_aot PRIVATE tree_mutation.cpp tree_mutation_adapter.cpp)
target_sources(cm_aot PRIVATE aggregate_object.cpp aggregate_object_adapter.cpp)
target_sources(cm_aot PRIVATE record_group.cpp record_group_adapter.cpp)
target_sources(cm_aot PRIVATE record_query.cpp record_query_adapter.cpp)
target_sources(cm_aot PRIVATE record_history.cpp record_history_adapter.cpp)
target_sources(cm_aot PRIVATE match_engine.cpp match_engine_adapter.cpp match_debugger_session.cpp)
add_library(cm_match_debugger_database STATIC match_debugger_database.cpp)
target_include_directories(cm_match_debugger_database PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")
target_link_libraries(cm_match_debugger_database PUBLIC "-framework CoreFoundation")
target_compile_options(cm_match_debugger_database PRIVATE -Wall -Wextra -Werror -Wpedantic)
add_library(cm_match_debugger_bootstrap STATIC match_debugger_bootstrap.cpp)
target_link_libraries(cm_match_debugger_bootstrap PUBLIC cm_aot cm_match_debugger_database)
target_compile_options(cm_match_debugger_bootstrap PRIVATE -Wall -Wextra -Werror -Wpedantic -ffp-contract=off)
add_library(cm_match_debugger_factory STATIC match_debugger_factory.cpp)
target_link_libraries(cm_match_debugger_factory PUBLIC cm_match_debugger_bootstrap)
target_compile_options(cm_match_debugger_factory PRIVATE -Wall -Wextra -Werror -Wpedantic -ffp-contract=off)
target_link_libraries(cm_aot PUBLIC cm_ui cm_audio z "-framework CoreFoundation" "-framework CoreServices" "-framework ApplicationServices")
target_include_directories(cm_aot PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")
target_compile_options(cm_aot PRIVATE -Wall -Wextra -Werror -Wpedantic -ffp-contract=off)
add_library(cm_probe_host STATIC aot_probe.cpp)
target_link_libraries(cm_probe_host PUBLIC cm_aot)
target_compile_options(cm_probe_host PRIVATE -Wall -Wextra -Werror -Wpedantic)
add_executable(cm-aot-probe probe_main.cpp)
target_link_libraries(cm-aot-probe PRIVATE cm_probe_host)
add_executable(cm-season-reference-checks season_reference_checks.cpp)
add_executable(cm-rng-core-checks rng_core_checks.cpp)
target_link_libraries(cm-rng-core-checks PRIVATE cm_aot)
target_compile_options(cm-rng-core-checks PRIVATE -Wall -Wextra -Werror -Wpedantic)
target_link_libraries(cm-season-reference-checks PRIVATE cm_aot)
target_compile_options(cm-season-reference-checks PRIVATE -Wall -Wextra -Werror -Wpedantic)
add_executable(cm-season-reference season_reference_main.cpp)
target_link_libraries(cm-season-reference PRIVATE cm_aot)
target_compile_options(cm-season-reference PRIVATE -Wall -Wextra -Werror -Wpedantic)
foreach(check_name IN ITEMS construction presentation subsystem_audit)
    add_executable(cm-date-${check_name}-checks date_${check_name}_checks.cpp)
    target_link_libraries(cm-date-${check_name}-checks PRIVATE cm_aot)
    target_compile_options(cm-date-${check_name}-checks PRIVATE -Wall -Wextra -Werror -Wpedantic)
endforeach()
add_executable(cm-calendar-checks calendar_checks.cpp)
add_executable(cm-date-weekday-checks date_weekday_checks.cpp)
target_link_libraries(cm-date-weekday-checks PRIVATE cm_aot)
target_compile_options(cm-date-weekday-checks PRIVATE -Wall -Wextra -Werror -Wpedantic)
add_executable(cm-date-day-of-month-checks date_day_of_month_checks.cpp)
target_link_libraries(cm-date-day-of-month-checks PRIVATE cm_aot)
target_compile_options(cm-date-day-of-month-checks PRIVATE -Wall -Wextra -Werror -Wpedantic)
add_executable(cm-date-month-checks date_month_checks.cpp)
target_link_libraries(cm-date-month-checks PRIVATE cm_aot)
target_compile_options(cm-date-month-checks PRIVATE -Wall -Wextra -Werror -Wpedantic)
add_executable(cm-date-difference-checks date_difference_checks.cpp)
add_executable(cm-date-before-checks date_ordering_checks.cpp)
target_compile_definitions(cm-date-before-checks PRIVATE CM_CHECK_BEFORE=1)
target_link_libraries(cm-date-before-checks PRIVATE cm_aot)
target_compile_options(cm-date-before-checks PRIVATE -Wall -Wextra -Werror -Wpedantic)
add_executable(cm-date-ordering-checks date_ordering_checks.cpp)
target_link_libraries(cm-date-ordering-checks PRIVATE cm_aot)
target_compile_options(cm-date-ordering-checks PRIVATE -Wall -Wextra -Werror -Wpedantic)
target_link_libraries(cm-date-difference-checks PRIVATE cm_aot)
target_compile_options(cm-date-difference-checks PRIVATE -Wall -Wextra -Werror -Wpedantic)
target_link_libraries(cm-calendar-checks PRIVATE cm_aot)
target_compile_options(cm-calendar-checks PRIVATE -Wall -Wextra -Werror -Wpedantic)
add_executable(cm-calendar-reference-checks calendar_reference_checks.cpp)
target_link_libraries(cm-calendar-reference-checks PRIVATE cm_aot)
target_compile_options(cm-calendar-reference-checks PRIVATE -Wall -Wextra -Werror -Wpedantic)
add_executable(cm-competition-dispatch-checks competition_dispatch_checks.cpp)
target_link_libraries(cm-competition-dispatch-checks PRIVATE cm_aot)
target_compile_options(cm-competition-dispatch-checks PRIVATE -Wall -Wextra -Werror -Wpedantic)
add_executable(cm-match-replay match_replay_main.cpp)
target_link_libraries(cm-match-replay PRIVATE cm_aot)
target_compile_options(cm-match-replay PRIVATE -Wall -Wextra -Werror -Wpedantic)
add_executable(cm-native-prototype MACOSX_BUNDLE prototype_main.mm)
target_link_libraries(cm-native-prototype PRIVATE cm_probe_host)
target_compile_options(cm-native-prototype PRIVATE -Wall -Wextra -Werror -Wpedantic -fobjc-arc)
set_target_properties(cm-native-prototype PROPERTIES
    OUTPUT_NAME "CM 01-02 Native Prototype"
    MACOSX_BUNDLE_INFO_PLIST "${CMAKE_CURRENT_SOURCE_DIR}/Prototype-Info.plist.in")
add_executable(cm-aot-checks aot_checks.cpp)
target_link_libraries(cm-aot-checks PRIVATE cm_aot)
target_compile_options(cm-aot-checks PRIVATE -Wall -Wextra -Werror -Wpedantic)
add_executable(cm-bridge-checks bridge_checks.cpp)
target_link_libraries(cm-bridge-checks PRIVATE cm_aot)
target_compile_options(cm-bridge-checks PRIVATE -Wall -Wextra -Werror -Wpedantic)
enable_testing()
set(CM_ORACLE_DIR "" CACHE PATH "Original relocated segments and oracle.tsv")
if(EXISTS "${CM_ORACLE_DIR}/instructions.sqlite")
find_package(Python3 REQUIRED COMPONENTS Interpreter)
set(CM_UTILITY_OBJECT_REFERENCE "${CMAKE_BINARY_DIR}/utility_object_reference.cpp")
add_custom_command(OUTPUT "${CM_UTILITY_OBJECT_REFERENCE}"
    COMMAND "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/../tools/make_utility_object_reference.py"
            "${CM_ORACLE_DIR}" "${CM_UTILITY_OBJECT_REFERENCE}"
    DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/../tools/make_utility_object_reference.py"
            "${CMAKE_CURRENT_SOURCE_DIR}/../tools/utility_object_audit.py"
            "${CMAKE_CURRENT_SOURCE_DIR}/../tools/compile_recomp.py"
            "${CMAKE_CURRENT_SOURCE_DIR}/../tools/local_temporaries.py"
            "${CM_ORACLE_DIR}/instructions.sqlite" "${CM_ORACLE_DIR}/program.json"
    VERBATIM)
add_library(cm-utility-object-reference OBJECT "${CM_UTILITY_OBJECT_REFERENCE}")
target_include_directories(cm-utility-object-reference PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}")
add_executable(cm-utility-object-checks utility_object_checks.cpp)
target_sources(cm-utility-object-checks PRIVATE $<TARGET_OBJECTS:cm-utility-object-reference>)
target_link_libraries(cm-utility-object-checks PRIVATE cm_aot)
target_compile_options(cm-utility-object-checks PRIVATE -Wall -Wextra -Werror -Wpedantic)
add_test(NAME utility_object_component_equivalence COMMAND cm-utility-object-checks)
set(CM_TREE_MUTATION_REFERENCE "${CMAKE_BINARY_DIR}/tree_mutation_reference.cpp")
add_custom_command(OUTPUT "${CM_TREE_MUTATION_REFERENCE}"
    COMMAND "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/../tools/make_tree_mutation_reference.py"
            "${CM_ORACLE_DIR}" "${CM_TREE_MUTATION_REFERENCE}"
    DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/../tools/make_tree_mutation_reference.py"
            "${CMAKE_CURRENT_SOURCE_DIR}/../tools/tree_mutation_audit.py"
            "${CMAKE_CURRENT_SOURCE_DIR}/../tools/compile_recomp.py"
            "${CMAKE_CURRENT_SOURCE_DIR}/../tools/local_temporaries.py"
            "${CM_ORACLE_DIR}/instructions.sqlite" "${CM_ORACLE_DIR}/program.json"
    VERBATIM)
add_library(cm-tree-mutation-reference OBJECT "${CM_TREE_MUTATION_REFERENCE}")
target_include_directories(cm-tree-mutation-reference PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}")
add_executable(cm-tree-mutation-checks tree_mutation_checks.cpp)
target_sources(cm-tree-mutation-checks PRIVATE $<TARGET_OBJECTS:cm-tree-mutation-reference>)
target_link_libraries(cm-tree-mutation-checks PRIVATE cm_aot)
target_compile_options(cm-tree-mutation-checks PRIVATE -Wall -Wextra -Werror -Wpedantic)
add_test(NAME tree_mutation_component_equivalence COMMAND cm-tree-mutation-checks)
set(CM_AGGREGATE_OBJECT_REFERENCE "${CMAKE_BINARY_DIR}/aggregate_object_reference.cpp")
add_custom_command(OUTPUT "${CM_AGGREGATE_OBJECT_REFERENCE}"
    COMMAND "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/../tools/make_aggregate_object_reference.py"
            "${CM_ORACLE_DIR}" "${CM_AGGREGATE_OBJECT_REFERENCE}"
    DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/../tools/make_aggregate_object_reference.py"
            "${CMAKE_CURRENT_SOURCE_DIR}/../tools/aggregate_object_audit.py"
            "${CMAKE_CURRENT_SOURCE_DIR}/../tools/compile_recomp.py"
            "${CMAKE_CURRENT_SOURCE_DIR}/../tools/local_temporaries.py"
            "${CM_ORACLE_DIR}/instructions.sqlite" "${CM_ORACLE_DIR}/program.json"
    VERBATIM)
add_library(cm-aggregate-object-reference OBJECT "${CM_AGGREGATE_OBJECT_REFERENCE}")
target_include_directories(cm-aggregate-object-reference PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}")
add_executable(cm-aggregate-object-checks aggregate_object_checks.cpp)
target_sources(cm-aggregate-object-checks PRIVATE $<TARGET_OBJECTS:cm-aggregate-object-reference>)
target_link_libraries(cm-aggregate-object-checks PRIVATE cm_aot)
target_compile_options(cm-aggregate-object-checks PRIVATE -Wall -Wextra -Werror -Wpedantic)
add_test(NAME aggregate_object_component_equivalence COMMAND cm-aggregate-object-checks)
set(CM_RECORD_GROUP_REFERENCE "${CMAKE_BINARY_DIR}/record_group_reference.cpp")
add_custom_command(OUTPUT "${CM_RECORD_GROUP_REFERENCE}"
    COMMAND "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/../tools/make_record_group_reference.py"
            "${CM_ORACLE_DIR}" "${CM_RECORD_GROUP_REFERENCE}"
    DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/../tools/make_record_group_reference.py"
            "${CMAKE_CURRENT_SOURCE_DIR}/../tools/record_group_audit.py"
            "${CMAKE_CURRENT_SOURCE_DIR}/../tools/compile_recomp.py"
            "${CMAKE_CURRENT_SOURCE_DIR}/../tools/local_temporaries.py"
            "${CM_ORACLE_DIR}/instructions.sqlite" "${CM_ORACLE_DIR}/program.json"
    VERBATIM)
add_library(cm-record-group-reference OBJECT "${CM_RECORD_GROUP_REFERENCE}")
target_include_directories(cm-record-group-reference PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}")
add_executable(cm-record-group-checks record_group_checks.cpp)
target_sources(cm-record-group-checks PRIVATE $<TARGET_OBJECTS:cm-record-group-reference>)
target_link_libraries(cm-record-group-checks PRIVATE cm_aot)
target_compile_options(cm-record-group-checks PRIVATE -Wall -Wextra -Werror -Wpedantic)
add_test(NAME record_group_component_equivalence COMMAND cm-record-group-checks)
set(CM_RECORD_QUERY_REFERENCE "${CMAKE_BINARY_DIR}/record_query_reference.cpp")
add_custom_command(OUTPUT "${CM_RECORD_QUERY_REFERENCE}"
    COMMAND "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/../tools/make_record_query_reference.py"
            "${CM_ORACLE_DIR}" "${CM_RECORD_QUERY_REFERENCE}"
    DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/../tools/make_record_query_reference.py"
            "${CMAKE_CURRENT_SOURCE_DIR}/../tools/record_query_audit.py"
            "${CMAKE_CURRENT_SOURCE_DIR}/../tools/compile_recomp.py"
            "${CMAKE_CURRENT_SOURCE_DIR}/../tools/local_temporaries.py"
            "${CM_ORACLE_DIR}/instructions.sqlite" "${CM_ORACLE_DIR}/program.json"
    VERBATIM)
add_library(cm-record-query-reference OBJECT "${CM_RECORD_QUERY_REFERENCE}")
target_include_directories(cm-record-query-reference PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}")
add_executable(cm-record-query-checks record_query_checks.cpp)
target_sources(cm-record-query-checks PRIVATE $<TARGET_OBJECTS:cm-record-query-reference>)
target_link_libraries(cm-record-query-checks PRIVATE cm_aot)
target_compile_options(cm-record-query-checks PRIVATE -Wall -Wextra -Werror -Wpedantic)
add_test(NAME record_query_component_equivalence COMMAND cm-record-query-checks)
set(CM_RECORD_HISTORY_REFERENCE "${CMAKE_BINARY_DIR}/record_history_reference.cpp")
add_custom_command(OUTPUT "${CM_RECORD_HISTORY_REFERENCE}"
    COMMAND "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/../tools/make_record_history_reference.py"
            "${CM_ORACLE_DIR}" "${CM_RECORD_HISTORY_REFERENCE}"
    DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/../tools/make_record_history_reference.py"
            "${CMAKE_CURRENT_SOURCE_DIR}/../tools/record_history_audit.py"
            "${CMAKE_CURRENT_SOURCE_DIR}/../tools/compile_recomp.py"
            "${CMAKE_CURRENT_SOURCE_DIR}/../tools/local_temporaries.py"
            "${CM_ORACLE_DIR}/instructions.sqlite" "${CM_ORACLE_DIR}/program.json"
    VERBATIM)
add_library(cm-record-history-reference OBJECT "${CM_RECORD_HISTORY_REFERENCE}")
target_include_directories(cm-record-history-reference PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}")
add_executable(cm-record-history-checks record_history_checks.cpp)
target_sources(cm-record-history-checks PRIVATE $<TARGET_OBJECTS:cm-record-history-reference>)
target_link_libraries(cm-record-history-checks PRIVATE cm_aot)
target_compile_options(cm-record-history-checks PRIVATE -Wall -Wextra -Werror -Wpedantic)
add_test(NAME record_history_component_equivalence COMMAND cm-record-history-checks)
set(CM_MATCH_ENGINE_REFERENCE "${CMAKE_BINARY_DIR}/match_engine_reference.cpp")
add_custom_command(OUTPUT "${CM_MATCH_ENGINE_REFERENCE}"
    COMMAND "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/../tools/make_match_engine_reference.py"
            "${CM_ORACLE_DIR}" "${CM_MATCH_ENGINE_REFERENCE}"
    DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/../tools/make_match_engine_reference.py"
            "${CMAKE_CURRENT_SOURCE_DIR}/../tools/match_engine_audit.py"
            "${CMAKE_CURRENT_SOURCE_DIR}/../tools/compile_recomp.py"
            "${CMAKE_CURRENT_SOURCE_DIR}/../tools/local_temporaries.py"
            "${CM_ORACLE_DIR}/instructions.sqlite" "${CM_ORACLE_DIR}/program.json"
    VERBATIM)
add_library(cm-match-engine-reference OBJECT "${CM_MATCH_ENGINE_REFERENCE}")
target_include_directories(cm-match-engine-reference PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}")
target_compile_options(cm-match-engine-reference PRIVATE -Wall -Wextra -Werror -Wpedantic -ffp-contract=off)
foreach(CM_MATCH_CHECK IN ITEMS caller core)
    add_executable(cm-match-engine-${CM_MATCH_CHECK}-checks match_engine_${CM_MATCH_CHECK}_checks.cpp)
    target_sources(cm-match-engine-${CM_MATCH_CHECK}-checks PRIVATE $<TARGET_OBJECTS:cm-match-engine-reference>)
    target_link_libraries(cm-match-engine-${CM_MATCH_CHECK}-checks PRIVATE cm_aot)
    target_compile_options(cm-match-engine-${CM_MATCH_CHECK}-checks PRIVATE -Wall -Wextra -Werror -Wpedantic -ffp-contract=off)
    add_test(NAME match_engine_${CM_MATCH_CHECK}_equivalence COMMAND cm-match-engine-${CM_MATCH_CHECK}-checks)
endforeach()
add_executable(cm-match-engine-live-checks match_engine_live_checks.cpp)
target_link_libraries(cm-match-engine-live-checks PRIVATE cm_aot)
target_compile_options(cm-match-engine-live-checks PRIVATE -Wall -Wextra -Werror -Wpedantic -ffp-contract=off)
add_executable(cm-match-debugger-session-checks match_debugger_session_checks.cpp)
target_link_libraries(cm-match-debugger-session-checks PRIVATE cm_aot)
target_compile_options(cm-match-debugger-session-checks PRIVATE -Wall -Wextra -Werror -Wpedantic -ffp-contract=off)
add_executable(cm-match-debugger-database-checks match_debugger_database_checks.cpp)
target_link_libraries(cm-match-debugger-database-checks PRIVATE cm_match_debugger_database)
target_compile_options(cm-match-debugger-database-checks PRIVATE -Wall -Wextra -Werror -Wpedantic)
add_test(NAME match_debugger_database_structure COMMAND cm-match-debugger-database-checks)
set(CM_MATCH_DEBUGGER_GAME_DIR "" CACHE PATH "Read-only original game database for standalone debugger catalog checks")
if(EXISTS "${CM_MATCH_DEBUGGER_GAME_DIR}/Data/index.dam")
    add_test(NAME match_debugger_actual_database COMMAND cm-match-debugger-database-checks "${CM_MATCH_DEBUGGER_GAME_DIR}")
endif()
add_executable(cm-match-debugger-bootstrap-checks match_debugger_bootstrap_checks.cpp)
target_link_libraries(cm-match-debugger-bootstrap-checks PRIVATE cm_match_debugger_bootstrap)
target_compile_options(cm-match-debugger-bootstrap-checks PRIVATE -Wall -Wextra -Werror -Wpedantic -ffp-contract=off)
if(EXISTS "${CM_MATCH_DEBUGGER_GAME_DIR}/Data/index.dam" AND EXISTS "${CM_ORACLE_DIR}/program.json")
    add_test(NAME match_debugger_fresh_database_world COMMAND cm-match-debugger-bootstrap-checks
        "${CM_ORACLE_DIR}" "${CM_MATCH_DEBUGGER_GAME_DIR}" "${CMAKE_BINARY_DIR}/debugger-bootstrap-checks")
endif()
add_executable(cm-match-debugger-factory-checks match_debugger_factory_checks.cpp)
target_link_libraries(cm-match-debugger-factory-checks PRIVATE cm_match_debugger_factory)
target_compile_options(cm-match-debugger-factory-checks PRIVATE -Wall -Wextra -Werror -Wpedantic -ffp-contract=off)
if(EXISTS "${CM_MATCH_DEBUGGER_GAME_DIR}/Data/index.dam" AND EXISTS "${CM_ORACLE_DIR}/program.json")
    add_test(NAME match_debugger_fresh_match_factory COMMAND cm-match-debugger-factory-checks
        "${CM_ORACLE_DIR}" "${CM_MATCH_DEBUGGER_GAME_DIR}" "${CMAKE_BINARY_DIR}/debugger-factory-checks")
endif()
foreach(CM_MATCH_CHECK IN ITEMS production transfer)
    add_executable(cm-match-engine-${CM_MATCH_CHECK}-checks match_engine_${CM_MATCH_CHECK}_checks.cpp)
    target_link_libraries(cm-match-engine-${CM_MATCH_CHECK}-checks PRIVATE cm_aot)
    target_compile_options(cm-match-engine-${CM_MATCH_CHECK}-checks PRIVATE -Wall -Wextra -Werror -Wpedantic -ffp-contract=off)
endforeach()
set(CM_MATCH_ENGINE_SNAPSHOT "" CACHE FILEPATH "Private initialized match-entry snapshot for focused engine checks")
if(EXISTS "${CM_MATCH_ENGINE_SNAPSHOT}")
    add_test(NAME match_engine_live_equivalence COMMAND cm-match-engine-live-checks "${CM_MATCH_ENGINE_SNAPSHOT}")
    add_test(NAME match_debugger_session COMMAND cm-match-debugger-session-checks "${CM_MATCH_ENGINE_SNAPSHOT}")
    add_test(NAME match_engine_production COMMAND cm-match-engine-production-checks "${CM_MATCH_ENGINE_SNAPSHOT}")
    add_test(NAME match_engine_original_profile COMMAND "${CMAKE_COMMAND}" -E env
        CM_AOT_MATCH_PC_PROFILE=1 $<TARGET_FILE:cm-match-engine-production-checks> "${CM_MATCH_ENGINE_SNAPSHOT}" --profile)
    add_test(NAME match_engine_nonlocal_transfer COMMAND cm-match-engine-transfer-checks "${CM_MATCH_ENGINE_SNAPSHOT}")
endif()
set(CM_ENGINE_INITIALIZATION_REFERENCE "${CMAKE_BINARY_DIR}/engine_initialization_reference.cpp")
add_custom_command(OUTPUT "${CM_ENGINE_INITIALIZATION_REFERENCE}"
    COMMAND "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/../tools/make_engine_initialization_reference.py"
            "${CM_ORACLE_DIR}" "${CM_ENGINE_INITIALIZATION_REFERENCE}"
    DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/../tools/make_engine_initialization_reference.py"
            "${CMAKE_CURRENT_SOURCE_DIR}/../tools/engine_initialization_audit.py"
            "${CMAKE_CURRENT_SOURCE_DIR}/../tools/compile_recomp.py"
            "${CMAKE_CURRENT_SOURCE_DIR}/../tools/local_temporaries.py"
            "${CM_ORACLE_DIR}/instructions.sqlite" "${CM_ORACLE_DIR}/program.json"
    VERBATIM)
add_library(cm-engine-initialization-reference OBJECT "${CM_ENGINE_INITIALIZATION_REFERENCE}")
target_include_directories(cm-engine-initialization-reference PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}")
add_executable(cm-engine-initialization-checks engine_initialization_checks.cpp)
target_sources(cm-engine-initialization-checks PRIVATE $<TARGET_OBJECTS:cm-engine-initialization-reference>)
target_link_libraries(cm-engine-initialization-checks PRIVATE cm_aot)
target_compile_options(cm-engine-initialization-checks PRIVATE -Wall -Wextra -Werror -Wpedantic)
add_test(NAME engine_initialization_component_equivalence COMMAND cm-engine-initialization-checks)
add_executable(cm-engine-initialization-production-checks engine_initialization_production_checks.cpp engine_initialization_transfer_checks.cpp)
target_link_libraries(cm-engine-initialization-production-checks PRIVATE cm_aot)
target_compile_options(cm-engine-initialization-production-checks PRIVATE -Wall -Wextra -Werror -Wpedantic)
set(CM_ORIGINAL_APPLICATION_FILE "$ENV{HOME}/Library/Application Support/CM 01-02 Native/Game/CM 01-02"
    CACHE FILEPATH "Original application including its resource fork, read only by initialization fixtures")
add_executable(cm-utility-object-production-checks utility_object_production_checks.cpp utility_object_transfer_checks.cpp)
target_link_libraries(cm-utility-object-production-checks PRIVATE cm_aot)
target_compile_options(cm-utility-object-production-checks PRIVATE -Wall -Wextra -Werror -Wpedantic)
add_test(NAME modern_utility_object_production COMMAND "${CMAKE_COMMAND}" -E env
    "CM_AOT_ORIGINAL_APPLICATION=${CM_ORIGINAL_APPLICATION_FILE}"
    $<TARGET_FILE:cm-utility-object-production-checks> "${CM_ORACLE_DIR}")
add_test(NAME original_utility_object_profile COMMAND "${CMAKE_COMMAND}" -E env
    "CM_AOT_ORIGINAL_APPLICATION=${CM_ORIGINAL_APPLICATION_FILE}"
    CM_AOT_MATCH_PC_PROFILE=1 $<TARGET_FILE:cm-utility-object-production-checks> "${CM_ORACLE_DIR}" --profile)
add_test(NAME modern_utility_object_transfer COMMAND "${CMAKE_COMMAND}" -E env
    "CM_AOT_ORIGINAL_APPLICATION=${CM_ORIGINAL_APPLICATION_FILE}"
    $<TARGET_FILE:cm-utility-object-production-checks> "${CM_ORACLE_DIR}" --transfer)
add_executable(cm-tree-mutation-production-checks tree_mutation_production_checks.cpp tree_mutation_transfer_checks.cpp)
target_link_libraries(cm-tree-mutation-production-checks PRIVATE cm_aot)
target_compile_options(cm-tree-mutation-production-checks PRIVATE -Wall -Wextra -Werror -Wpedantic)
add_test(NAME modern_tree_mutation_production COMMAND "${CMAKE_COMMAND}" -E env
    "CM_AOT_ORIGINAL_APPLICATION=${CM_ORIGINAL_APPLICATION_FILE}"
    $<TARGET_FILE:cm-tree-mutation-production-checks> "${CM_ORACLE_DIR}")
add_test(NAME original_tree_mutation_profile COMMAND "${CMAKE_COMMAND}" -E env
    "CM_AOT_ORIGINAL_APPLICATION=${CM_ORIGINAL_APPLICATION_FILE}"
    CM_AOT_MATCH_PC_PROFILE=1 $<TARGET_FILE:cm-tree-mutation-production-checks> "${CM_ORACLE_DIR}" --profile)
add_test(NAME modern_tree_mutation_transfer COMMAND "${CMAKE_COMMAND}" -E env
    "CM_AOT_ORIGINAL_APPLICATION=${CM_ORIGINAL_APPLICATION_FILE}"
    $<TARGET_FILE:cm-tree-mutation-production-checks> "${CM_ORACLE_DIR}" --transfer)
add_executable(cm-aggregate-object-production-checks aggregate_object_production_checks.cpp aggregate_object_transfer_checks.cpp)
target_link_libraries(cm-aggregate-object-production-checks PRIVATE cm_aot)
target_compile_options(cm-aggregate-object-production-checks PRIVATE -Wall -Wextra -Werror -Wpedantic)
add_test(NAME modern_aggregate_object_production COMMAND "${CMAKE_COMMAND}" -E env
    "CM_AOT_ORIGINAL_APPLICATION=${CM_ORIGINAL_APPLICATION_FILE}"
    $<TARGET_FILE:cm-aggregate-object-production-checks> "${CM_ORACLE_DIR}")
add_test(NAME original_aggregate_object_profile COMMAND "${CMAKE_COMMAND}" -E env
    "CM_AOT_ORIGINAL_APPLICATION=${CM_ORIGINAL_APPLICATION_FILE}"
    CM_AOT_MATCH_PC_PROFILE=1 $<TARGET_FILE:cm-aggregate-object-production-checks> "${CM_ORACLE_DIR}" --profile)
add_test(NAME modern_aggregate_object_transfer COMMAND "${CMAKE_COMMAND}" -E env
    "CM_AOT_ORIGINAL_APPLICATION=${CM_ORIGINAL_APPLICATION_FILE}"
    $<TARGET_FILE:cm-aggregate-object-production-checks> "${CM_ORACLE_DIR}" --transfer)
add_executable(cm-record-group-production-checks record_group_production_checks.cpp record_group_transfer_checks.cpp)
target_link_libraries(cm-record-group-production-checks PRIVATE cm_aot)
target_compile_options(cm-record-group-production-checks PRIVATE -Wall -Wextra -Werror -Wpedantic)
add_test(NAME modern_record_group_production COMMAND "${CMAKE_COMMAND}" -E env
    "CM_AOT_ORIGINAL_APPLICATION=${CM_ORIGINAL_APPLICATION_FILE}"
    $<TARGET_FILE:cm-record-group-production-checks> "${CM_ORACLE_DIR}")
add_test(NAME original_record_group_profile COMMAND "${CMAKE_COMMAND}" -E env
    "CM_AOT_ORIGINAL_APPLICATION=${CM_ORIGINAL_APPLICATION_FILE}"
    CM_AOT_MATCH_PC_PROFILE=1 $<TARGET_FILE:cm-record-group-production-checks> "${CM_ORACLE_DIR}" --profile)
add_test(NAME modern_record_group_transfer COMMAND "${CMAKE_COMMAND}" -E env
    "CM_AOT_ORIGINAL_APPLICATION=${CM_ORIGINAL_APPLICATION_FILE}"
    $<TARGET_FILE:cm-record-group-production-checks> "${CM_ORACLE_DIR}" --transfer)
add_executable(cm-record-query-production-checks record_query_production_checks.cpp record_query_transfer_checks.cpp)
target_link_libraries(cm-record-query-production-checks PRIVATE cm_aot)
target_compile_options(cm-record-query-production-checks PRIVATE -Wall -Wextra -Werror -Wpedantic)
add_test(NAME modern_record_query_production COMMAND "${CMAKE_COMMAND}" -E env
    "CM_AOT_ORIGINAL_APPLICATION=${CM_ORIGINAL_APPLICATION_FILE}"
    $<TARGET_FILE:cm-record-query-production-checks> "${CM_ORACLE_DIR}")
add_test(NAME original_record_query_profile COMMAND "${CMAKE_COMMAND}" -E env
    "CM_AOT_ORIGINAL_APPLICATION=${CM_ORIGINAL_APPLICATION_FILE}"
    CM_AOT_MATCH_PC_PROFILE=1 $<TARGET_FILE:cm-record-query-production-checks> "${CM_ORACLE_DIR}" --profile)
add_test(NAME modern_record_query_transfer COMMAND "${CMAKE_COMMAND}" -E env
    "CM_AOT_ORIGINAL_APPLICATION=${CM_ORIGINAL_APPLICATION_FILE}"
    $<TARGET_FILE:cm-record-query-production-checks> "${CM_ORACLE_DIR}" --transfer)
add_executable(cm-record-history-production-checks record_history_production_checks.cpp record_history_transfer_checks.cpp)
target_link_libraries(cm-record-history-production-checks PRIVATE cm_aot)
target_compile_options(cm-record-history-production-checks PRIVATE -Wall -Wextra -Werror -Wpedantic)
add_test(NAME modern_record_history_production COMMAND "${CMAKE_COMMAND}" -E env
    "CM_AOT_ORIGINAL_APPLICATION=${CM_ORIGINAL_APPLICATION_FILE}"
    $<TARGET_FILE:cm-record-history-production-checks> "${CM_ORACLE_DIR}")
add_test(NAME original_record_history_profile COMMAND "${CMAKE_COMMAND}" -E env
    "CM_AOT_ORIGINAL_APPLICATION=${CM_ORIGINAL_APPLICATION_FILE}"
    CM_AOT_MATCH_PC_PROFILE=1 $<TARGET_FILE:cm-record-history-production-checks> "${CM_ORACLE_DIR}" --profile)
add_test(NAME modern_record_history_transfer COMMAND "${CMAKE_COMMAND}" -E env
    "CM_AOT_ORIGINAL_APPLICATION=${CM_ORIGINAL_APPLICATION_FILE}"
    $<TARGET_FILE:cm-record-history-production-checks> "${CM_ORACLE_DIR}" --transfer)
add_test(NAME modern_engine_initialization_production COMMAND "${CMAKE_COMMAND}" -E env
    "CM_AOT_ORIGINAL_APPLICATION=${CM_ORIGINAL_APPLICATION_FILE}"
    $<TARGET_FILE:cm-engine-initialization-production-checks> "${CM_ORACLE_DIR}")
add_test(NAME original_engine_initialization_profile COMMAND "${CMAKE_COMMAND}" -E env
    "CM_AOT_ORIGINAL_APPLICATION=${CM_ORIGINAL_APPLICATION_FILE}"
    CM_AOT_MATCH_PC_PROFILE=1 $<TARGET_FILE:cm-engine-initialization-production-checks> "${CM_ORACLE_DIR}" --profile)
add_test(NAME modern_engine_initialization_transfer COMMAND "${CMAKE_COMMAND}" -E env
    "CM_AOT_ORIGINAL_APPLICATION=${CM_ORIGINAL_APPLICATION_FILE}"
    $<TARGET_FILE:cm-engine-initialization-production-checks> "${CM_ORACLE_DIR}" --transfer)
set(CM_NATIVE_SERVICE_REFERENCE "${CMAKE_BINARY_DIR}/native_service_reference.cpp")
add_custom_command(OUTPUT "${CM_NATIVE_SERVICE_REFERENCE}"
    COMMAND "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/../tools/make_native_service_reference.py"
            "${CM_ORACLE_DIR}" "${CM_NATIVE_SERVICE_REFERENCE}"
    DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/../tools/make_native_service_reference.py"
            "${CMAKE_CURRENT_SOURCE_DIR}/../tools/native_service_audit.py"
            "${CMAKE_CURRENT_SOURCE_DIR}/../tools/compile_recomp.py"
            "${CMAKE_CURRENT_SOURCE_DIR}/../tools/local_temporaries.py"
            "${CM_ORACLE_DIR}/instructions.sqlite" "${CM_ORACLE_DIR}/program.json"
    VERBATIM)
add_library(cm-native-service-reference OBJECT "${CM_NATIVE_SERVICE_REFERENCE}")
target_include_directories(cm-native-service-reference PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}")
add_executable(cm-native-service-checks native_service_checks.cpp)
target_sources(cm-native-service-checks PRIVATE $<TARGET_OBJECTS:cm-native-service-reference>)
target_link_libraries(cm-native-service-checks PRIVATE cm_aot)
target_compile_options(cm-native-service-checks PRIVATE -Wall -Wextra -Werror -Wpedantic)
add_test(NAME native_service_component_equivalence COMMAND cm-native-service-checks)
add_test(NAME modern_native_service_production COMMAND cm-native-service-checks --production)
add_test(NAME original_native_service_profile COMMAND "${CMAKE_COMMAND}" -E env
    CM_AOT_MATCH_PC_PROFILE=1 $<TARGET_FILE:cm-native-service-checks> --profile)
add_executable(cm-native-service-transfer-checks native_service_transfer_checks.cpp)
target_link_libraries(cm-native-service-transfer-checks PRIVATE cm_aot)
target_compile_options(cm-native-service-transfer-checks PRIVATE -Wall -Wextra -Werror -Wpedantic)
add_test(NAME modern_native_service_transfer COMMAND cm-native-service-transfer-checks)
add_executable(cm-native-service-event-checks native_service_event_checks.cpp)
target_link_libraries(cm-native-service-event-checks PRIVATE cm_aot)
target_compile_options(cm-native-service-event-checks PRIVATE -Wall -Wextra -Werror -Wpedantic)
add_test(NAME modern_native_service_event COMMAND cm-native-service-event-checks "${CM_ORACLE_DIR}")
add_test(NAME original_native_service_event COMMAND cm-native-service-event-checks "${CM_ORACLE_DIR}" --original)
set(CM_IMPORT_DESCRIPTOR_REFERENCE "${CMAKE_BINARY_DIR}/import_descriptor_reference.cpp")
add_custom_command(OUTPUT "${CM_IMPORT_DESCRIPTOR_REFERENCE}"
    COMMAND "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/../tools/make_import_descriptor_reference.py"
            "${CM_ORACLE_DIR}" "${CM_IMPORT_DESCRIPTOR_REFERENCE}"
    DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/../tools/make_import_descriptor_reference.py"
            "${CMAKE_CURRENT_SOURCE_DIR}/../tools/import_descriptor_audit.py"
            "${CMAKE_CURRENT_SOURCE_DIR}/../tools/compile_recomp.py"
            "${CMAKE_CURRENT_SOURCE_DIR}/../tools/local_temporaries.py"
            "${CM_ORACLE_DIR}/instructions.sqlite" "${CM_ORACLE_DIR}/program.json"
    VERBATIM)
add_library(cm-import-descriptor-reference OBJECT "${CM_IMPORT_DESCRIPTOR_REFERENCE}")
target_include_directories(cm-import-descriptor-reference PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}")
add_executable(cm-import-descriptor-checks import_descriptor_checks.cpp)
target_link_libraries(cm-import-descriptor-checks PRIVATE cm_aot)
target_sources(cm-import-descriptor-checks PRIVATE $<TARGET_OBJECTS:cm-import-descriptor-reference>)
target_compile_options(cm-import-descriptor-checks PRIVATE -Wall -Wextra -Werror -Wpedantic)
add_test(NAME import_descriptor_component_equivalence COMMAND cm-import-descriptor-checks)
add_test(NAME modern_import_descriptor_production COMMAND cm-import-descriptor-checks --production)
add_test(NAME original_import_descriptor_profile COMMAND "${CMAKE_COMMAND}" -E env
    CM_AOT_MATCH_PC_PROFILE=1 $<TARGET_FILE:cm-import-descriptor-checks> --profile)
set(CM_TIME_RUNTIME_REFERENCE "${CMAKE_BINARY_DIR}/time_runtime_reference.cpp")
add_custom_command(OUTPUT "${CM_TIME_RUNTIME_REFERENCE}"
    COMMAND "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/../tools/make_time_runtime_reference.py"
            "${CM_ORACLE_DIR}" "${CM_TIME_RUNTIME_REFERENCE}"
    DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/../tools/make_time_runtime_reference.py"
            "${CMAKE_CURRENT_SOURCE_DIR}/../tools/time_runtime_audit.py"
            "${CMAKE_CURRENT_SOURCE_DIR}/../tools/compile_recomp.py"
            "${CMAKE_CURRENT_SOURCE_DIR}/../tools/local_temporaries.py"
            "${CM_ORACLE_DIR}/instructions.sqlite" "${CM_ORACLE_DIR}/program.json"
    VERBATIM)
add_library(cm-time-runtime-reference OBJECT "${CM_TIME_RUNTIME_REFERENCE}")
target_include_directories(cm-time-runtime-reference PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}")
add_executable(cm-time-runtime-checks time_runtime_checks.cpp)
target_link_libraries(cm-time-runtime-checks PRIVATE cm_aot)
target_sources(cm-time-runtime-checks PRIVATE $<TARGET_OBJECTS:cm-time-runtime-reference>)
add_test(NAME time_runtime_component_equivalence COMMAND cm-time-runtime-checks)
add_executable(cm-time-runtime-integration-checks time_runtime_integration_checks.cpp)
target_link_libraries(cm-time-runtime-integration-checks PRIVATE cm_aot)
target_sources(cm-time-runtime-integration-checks PRIVATE $<TARGET_OBJECTS:cm-time-runtime-reference>)
target_compile_options(cm-time-runtime-checks PRIVATE -Wall -Wextra -Werror -Wpedantic)
target_compile_options(cm-time-runtime-integration-checks PRIVATE -Wall -Wextra -Werror -Wpedantic)
add_test(NAME modern_time_runtime_production COMMAND cm-time-runtime-integration-checks)
add_test(NAME original_time_runtime_profile COMMAND "${CMAKE_COMMAND}" -E env
    CM_AOT_MATCH_PC_PROFILE=1 $<TARGET_FILE:cm-time-runtime-integration-checks> --profile)
add_executable(cm-time-runtime-transfer-checks time_runtime_transfer_checks.cpp)
target_link_libraries(cm-time-runtime-transfer-checks PRIVATE cm_aot)
target_compile_options(cm-time-runtime-transfer-checks PRIVATE -Wall -Wextra -Werror -Wpedantic)
add_test(NAME modern_time_runtime_transfer COMMAND cm-time-runtime-transfer-checks)
set(CM_STDLIB_CORE_REFERENCE "${CMAKE_BINARY_DIR}/stdlib_core_reference.cpp")
add_custom_command(OUTPUT "${CM_STDLIB_CORE_REFERENCE}"
    COMMAND "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/../tools/make_stdlib_core_reference.py"
            "${CM_ORACLE_DIR}" "${CM_STDLIB_CORE_REFERENCE}"
    DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/../tools/make_stdlib_core_reference.py"
            "${CMAKE_CURRENT_SOURCE_DIR}/../tools/stdlib_core_audit.py"
            "${CMAKE_CURRENT_SOURCE_DIR}/../tools/compile_recomp.py"
            "${CMAKE_CURRENT_SOURCE_DIR}/../tools/local_temporaries.py"
            "${CM_ORACLE_DIR}/instructions.sqlite" "${CM_ORACLE_DIR}/program.json"
    VERBATIM)
add_library(cm-stdlib-core-reference OBJECT "${CM_STDLIB_CORE_REFERENCE}")
target_include_directories(cm-stdlib-core-reference PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}")
add_executable(cm-stdlib-core-checks stdlib_core_checks.cpp)
target_link_libraries(cm-stdlib-core-checks PRIVATE cm_aot)
target_sources(cm-stdlib-core-checks PRIVATE $<TARGET_OBJECTS:cm-stdlib-core-reference>)
add_test(NAME stdlib_core_component_equivalence COMMAND cm-stdlib-core-checks)
add_executable(cm-stdlib-core-integration-checks stdlib_core_integration_checks.cpp)
target_link_libraries(cm-stdlib-core-integration-checks PRIVATE cm_aot)
target_sources(cm-stdlib-core-integration-checks PRIVATE $<TARGET_OBJECTS:cm-stdlib-core-reference>)
target_compile_options(cm-stdlib-core-checks PRIVATE -Wall -Wextra -Werror -Wpedantic)
target_compile_options(cm-stdlib-core-integration-checks PRIVATE -Wall -Wextra -Werror -Wpedantic)
add_test(NAME modern_stdlib_core_production COMMAND cm-stdlib-core-integration-checks)
add_test(NAME original_stdlib_core_profile COMMAND "${CMAKE_COMMAND}" -E env
    CM_AOT_MATCH_PC_PROFILE=1 $<TARGET_FILE:cm-stdlib-core-integration-checks> --profile)
add_executable(cm-stdlib-core-transfer-checks stdlib_core_transfer_checks.cpp)
target_link_libraries(cm-stdlib-core-transfer-checks PRIVATE cm_aot)
target_compile_options(cm-stdlib-core-transfer-checks PRIVATE -Wall -Wextra -Werror -Wpedantic)
add_test(NAME modern_stdlib_core_transfer COMMAND cm-stdlib-core-transfer-checks)
set(CM_REGISTER_REFERENCE "${CMAKE_BINARY_DIR}/register_context_reference.cpp")
add_custom_command(OUTPUT "${CM_REGISTER_REFERENCE}"
    COMMAND "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/../tools/make_register_context_reference.py"
            "${CM_ORACLE_DIR}" "${CM_REGISTER_REFERENCE}"
    DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/../tools/make_register_context_reference.py"
            "${CMAKE_CURRENT_SOURCE_DIR}/../tools/register_context_audit.py"
            "${CMAKE_CURRENT_SOURCE_DIR}/../tools/compile_recomp.py"
            "${CMAKE_CURRENT_SOURCE_DIR}/../tools/local_temporaries.py"
            "${CM_ORACLE_DIR}/instructions.sqlite" "${CM_ORACLE_DIR}/program.json"
    VERBATIM)
add_library(cm-register-context-reference OBJECT "${CM_REGISTER_REFERENCE}")
target_include_directories(cm-register-context-reference PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}")
target_compile_options(cm-register-context-reference PRIVATE -Wall -Wextra -Werror -Wpedantic -ffp-contract=off)
add_executable(cm-register-context-checks register_context_checks.cpp)
target_link_libraries(cm-register-context-checks PRIVATE cm_aot cm-register-context-reference)
target_compile_options(cm-register-context-checks PRIVATE -Wall -Wextra -Werror -Wpedantic -ffp-contract=off)
add_test(NAME register_context_component_equivalence COMMAND cm-register-context-checks)
add_executable(cm-register-context-integration-checks register_context_integration_checks.cpp)
target_link_libraries(cm-register-context-integration-checks PRIVATE cm_aot)
target_compile_options(cm-register-context-integration-checks PRIVATE -Wall -Wextra -Werror -Wpedantic -ffp-contract=off)
add_test(NAME modern_register_context_production COMMAND cm-register-context-integration-checks)
add_test(NAME original_register_context_pc_profile COMMAND "${CMAKE_COMMAND}" -E env
    CM_AOT_MATCH_PC_PROFILE=1 "$<TARGET_FILE:cm-register-context-integration-checks>" --profile)
add_executable(cm-register-context-transfer-checks register_context_transfer_checks.cpp)
target_link_libraries(cm-register-context-transfer-checks PRIVATE cm_aot)
target_compile_options(cm-register-context-transfer-checks PRIVATE -Wall -Wextra -Werror -Wpedantic -ffp-contract=off)
add_test(NAME modern_register_context_transfer COMMAND cm-register-context-transfer-checks)
set(CM_ARITHMETIC_REFERENCE "${CMAKE_BINARY_DIR}/compiler_arithmetic_reference.cpp")
add_custom_command(OUTPUT "${CM_ARITHMETIC_REFERENCE}"
    COMMAND "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/../tools/make_compiler_arithmetic_reference.py"
            "${CM_ORACLE_DIR}" "${CM_ARITHMETIC_REFERENCE}"
    DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/../tools/make_compiler_arithmetic_reference.py"
            "${CMAKE_CURRENT_SOURCE_DIR}/../tools/compiler_arithmetic_audit.py"
            "${CMAKE_CURRENT_SOURCE_DIR}/../tools/compile_recomp.py"
            "${CMAKE_CURRENT_SOURCE_DIR}/../tools/local_temporaries.py"
            "${CMAKE_CURRENT_SOURCE_DIR}/../tools/unsigned_division_audit.py"
            "${CM_ORACLE_DIR}/instructions.sqlite" "${CM_ORACLE_DIR}/program.json"
    VERBATIM)
add_library(cm-compiler-arithmetic-reference OBJECT "${CM_ARITHMETIC_REFERENCE}")
target_include_directories(cm-compiler-arithmetic-reference PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}")
target_compile_options(cm-compiler-arithmetic-reference PRIVATE -Wall -Wextra -Werror -Wpedantic -ffp-contract=off)
add_executable(cm-compiler-arithmetic-checks compiler_arithmetic_checks.cpp)
target_link_libraries(cm-compiler-arithmetic-checks PRIVATE cm_aot cm-compiler-arithmetic-reference)
target_compile_options(cm-compiler-arithmetic-checks PRIVATE -Wall -Wextra -Werror -Wpedantic -ffp-contract=off)
add_test(NAME compiler_arithmetic_component_equivalence COMMAND cm-compiler-arithmetic-checks)
add_executable(cm-compiler-arithmetic-integration-checks compiler_arithmetic_integration_checks.cpp)
target_link_libraries(cm-compiler-arithmetic-integration-checks PRIVATE cm_aot)
target_compile_options(cm-compiler-arithmetic-integration-checks PRIVATE -Wall -Wextra -Werror -Wpedantic -ffp-contract=off)
add_test(NAME modern_compiler_arithmetic_production COMMAND cm-compiler-arithmetic-integration-checks)
add_test(NAME original_compiler_arithmetic_pc_profile COMMAND "${CMAKE_COMMAND}" -E env
    CM_AOT_MATCH_PC_PROFILE=1 $<TARGET_FILE:cm-compiler-arithmetic-integration-checks> --profile)
set(CM_CONTEXT_REFERENCE "${CMAKE_BINARY_DIR}/exception_context_reference.cpp")
add_custom_command(OUTPUT "${CM_CONTEXT_REFERENCE}"
    COMMAND "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/../tools/make_exception_context_reference.py"
            "${CM_ORACLE_DIR}" "${CM_CONTEXT_REFERENCE}"
    DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/../tools/make_exception_context_reference.py"
            "${CMAKE_CURRENT_SOURCE_DIR}/../tools/compile_recomp.py"
            "${CMAKE_CURRENT_SOURCE_DIR}/../tools/vector_context_lowering.py"
            "${CMAKE_CURRENT_SOURCE_DIR}/../tools/local_temporaries.py"
            "${CMAKE_CURRENT_SOURCE_DIR}/../tools/exception_runtime_audit.py"
            "${CM_ORACLE_DIR}/instructions.sqlite" "${CM_ORACLE_DIR}/program.json"
    VERBATIM)
add_library(cm-exception-context-reference OBJECT "${CM_CONTEXT_REFERENCE}")
target_include_directories(cm-exception-context-reference PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}")
target_compile_options(cm-exception-context-reference PRIVATE -Wall -Wextra -Werror -Wpedantic -ffp-contract=off)
add_executable(cm-vector-context-checks vector_context_checks.cpp)
target_link_libraries(cm-vector-context-checks PRIVATE cm_aot cm-exception-context-reference)
target_compile_options(cm-vector-context-checks PRIVATE -Wall -Wextra -Werror -Wpedantic)
add_executable(cm-exception-frame-checks exception_frame_checks.cpp)
target_link_libraries(cm-exception-frame-checks PRIVATE cm_aot cm-exception-context-reference)
target_compile_options(cm-exception-frame-checks PRIVATE -Wall -Wextra -Werror -Wpedantic)
add_executable(cm-exception-cleanup-checks exception_cleanup_checks.cpp)
target_link_libraries(cm-exception-cleanup-checks PRIVATE cm_aot cm-exception-context-reference)
target_compile_options(cm-exception-cleanup-checks PRIVATE -Wall -Wextra -Werror -Wpedantic)
add_executable(cm-exception-matching-checks exception_matching_checks.cpp)
target_link_libraries(cm-exception-matching-checks PRIVATE cm_aot cm-exception-context-reference)
target_compile_options(cm-exception-matching-checks PRIVATE -Wall -Wextra -Werror -Wpedantic)
set(CM_SEARCH_REFERENCE "${CMAKE_BINARY_DIR}/exception_search_reference.cpp")
add_custom_command(OUTPUT "${CM_SEARCH_REFERENCE}"
    COMMAND "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/../tools/make_exception_context_reference.py"
            "${CM_ORACLE_DIR}" "${CM_SEARCH_REFERENCE}" --with-search
    DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/../tools/make_exception_context_reference.py"
            "${CMAKE_CURRENT_SOURCE_DIR}/../tools/compile_recomp.py"
            "${CMAKE_CURRENT_SOURCE_DIR}/../tools/vector_context_lowering.py"
            "${CMAKE_CURRENT_SOURCE_DIR}/../tools/local_temporaries.py"
            "${CMAKE_CURRENT_SOURCE_DIR}/../tools/exception_runtime_audit.py"
            "${CM_ORACLE_DIR}/instructions.sqlite" "${CM_ORACLE_DIR}/program.json"
    VERBATIM)
add_library(cm-exception-search-reference OBJECT "${CM_SEARCH_REFERENCE}")
target_include_directories(cm-exception-search-reference PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}")
target_compile_options(cm-exception-search-reference PRIVATE -Wall -Wextra -Werror -Wpedantic -ffp-contract=off)
add_executable(cm-exception-search-checks exception_search_checks.cpp)
target_link_libraries(cm-exception-search-checks PRIVATE cm_aot cm-exception-search-reference)
target_compile_options(cm-exception-search-checks PRIVATE -Wall -Wextra -Werror -Wpedantic)
set(CM_TRANSFER_REFERENCE "${CMAKE_BINARY_DIR}/exception_transfer_reference.cpp")
add_custom_command(OUTPUT "${CM_TRANSFER_REFERENCE}"
    COMMAND "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/../tools/make_exception_context_reference.py"
            "${CM_ORACLE_DIR}" "${CM_TRANSFER_REFERENCE}" --with-search --with-transfers
    DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/../tools/make_exception_context_reference.py"
            "${CMAKE_CURRENT_SOURCE_DIR}/../tools/compile_recomp.py"
            "${CMAKE_CURRENT_SOURCE_DIR}/../tools/vector_context_lowering.py"
            "${CMAKE_CURRENT_SOURCE_DIR}/../tools/local_temporaries.py"
            "${CMAKE_CURRENT_SOURCE_DIR}/../tools/exception_runtime_audit.py"
            "${CM_ORACLE_DIR}/instructions.sqlite" "${CM_ORACLE_DIR}/program.json"
    VERBATIM)
add_library(cm-exception-transfer-reference OBJECT "${CM_TRANSFER_REFERENCE}")
target_include_directories(cm-exception-transfer-reference PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}")
target_compile_options(cm-exception-transfer-reference PRIVATE -Wall -Wextra -Werror -Wpedantic -ffp-contract=off)
add_executable(cm-exception-transfer-checks exception_transfer_checks.cpp)
target_link_libraries(cm-exception-transfer-checks PRIVATE cm_aot cm-exception-transfer-reference)
target_compile_options(cm-exception-transfer-checks PRIVATE -Wall -Wextra -Werror -Wpedantic)
foreach(CM_CAPTURE_MODE IN ITEMS capture scalar full)
    set(CM_CAPTURE_REFERENCE "${CMAKE_BINARY_DIR}/exception_${CM_CAPTURE_MODE}_reference.cpp")
    if(CM_CAPTURE_MODE STREQUAL "capture")
        set(CM_CAPTURE_ARGUMENTS --with-search --with-capture --with-transfers)
    elseif(CM_CAPTURE_MODE STREQUAL "scalar")
        set(CM_CAPTURE_ARGUMENTS --scalar-prefix)
    else()
        set(CM_CAPTURE_ARGUMENTS --complete --with-transfers)
    endif()
    add_custom_command(OUTPUT "${CM_CAPTURE_REFERENCE}"
        COMMAND "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/../tools/make_exception_context_reference.py"
                "${CM_ORACLE_DIR}" "${CM_CAPTURE_REFERENCE}" ${CM_CAPTURE_ARGUMENTS}
        DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/../tools/make_exception_context_reference.py"
                "${CMAKE_CURRENT_SOURCE_DIR}/../tools/compile_recomp.py"
                "${CMAKE_CURRENT_SOURCE_DIR}/../tools/vector_context_lowering.py"
                "${CMAKE_CURRENT_SOURCE_DIR}/../tools/local_temporaries.py"
                "${CMAKE_CURRENT_SOURCE_DIR}/../tools/exception_runtime_audit.py"
                "${CM_ORACLE_DIR}/instructions.sqlite" "${CM_ORACLE_DIR}/program.json"
        VERBATIM)
    add_library(cm-exception-${CM_CAPTURE_MODE}-reference OBJECT "${CM_CAPTURE_REFERENCE}")
    target_include_directories(cm-exception-${CM_CAPTURE_MODE}-reference PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}")
    target_compile_options(cm-exception-${CM_CAPTURE_MODE}-reference PRIVATE -Wall -Wextra -Werror -Wpedantic -ffp-contract=off)
endforeach()
target_link_libraries(cm-exception-transfer-checks PRIVATE cm-exception-capture-reference)
add_executable(cm-exception-capture-checks exception_capture_checks.cpp)
target_link_libraries(cm-exception-capture-checks PRIVATE cm_aot cm-exception-capture-reference cm-exception-scalar-reference)
target_compile_options(cm-exception-capture-checks PRIVATE -Wall -Wextra -Werror -Wpedantic)
add_executable(cm-exception-handler-checks exception_handler_checks.cpp)
target_link_libraries(cm-exception-handler-checks PRIVATE cm_aot cm-exception-full-reference)
target_compile_options(cm-exception-handler-checks PRIVATE -Wall -Wextra -Werror -Wpedantic)
add_executable(cm-exception-integration-checks exception_integration_checks.cpp)
target_link_libraries(cm-exception-integration-checks PRIVATE cm_aot)
target_compile_options(cm-exception-integration-checks PRIVATE -Wall -Wextra -Werror -Wpedantic)
endif()
add_executable(cm-presentation-checks presentation_checks.cpp presentation.cpp)
target_compile_options(cm-presentation-checks PRIVATE -Wall -Wextra -Werror -Wpedantic)
add_test(NAME native_presentation_geometry COMMAND cm-presentation-checks)
add_executable(cm-audio-checks audio_checks.cpp)
target_link_libraries(cm-audio-checks PRIVATE cm_audio)
target_compile_options(cm-audio-checks PRIVATE -Wall -Wextra -Werror -Wpedantic)
add_test(NAME native_audio COMMAND cm-audio-checks)
add_executable(cm-sound-checks sound_checks.cpp)
target_link_libraries(cm-sound-checks PRIVATE cm_aot)
target_compile_options(cm-sound-checks PRIVATE -Wall -Wextra -Werror -Wpedantic)
add_test(NAME native_bridges COMMAND cm-bridge-checks)
add_executable(cm-memory-checks memory_checks.cpp)
target_link_libraries(cm-memory-checks PRIVATE cm_aot)
target_compile_options(cm-memory-checks PRIVATE -Wall -Wextra -Werror -Wpedantic)
add_test(NAME native_memory_bounds COMMAND cm-memory-checks)
add_executable(cm-stream-io-checks stream_io_checks.cpp)
target_link_libraries(cm-stream-io-checks PRIVATE cm_aot)
target_compile_options(cm-stream-io-checks PRIVATE -Wall -Wextra -Werror -Wpedantic)
add_executable(cm-stream-buffer-checks stream_buffer_checks.cpp)
target_link_libraries(cm-stream-buffer-checks PRIVATE cm_aot)
target_compile_options(cm-stream-buffer-checks PRIVATE -Wall -Wextra -Werror -Wpedantic)
add_executable(cm-allocator-equivalence-checks allocator_equivalence_checks.cpp)
target_link_libraries(cm-allocator-equivalence-checks PRIVATE cm_aot)
target_compile_options(cm-allocator-equivalence-checks PRIVATE -Wall -Wextra -Werror -Wpedantic)
add_executable(cm-allocator-checks allocator_checks.cpp)
target_link_libraries(cm-allocator-checks PRIVATE cm_aot)
target_compile_options(cm-allocator-checks PRIVATE -Wall -Wextra -Werror -Wpedantic)
add_executable(cm-floating-point-checks floating_point_checks.cpp)
target_link_libraries(cm-floating-point-checks PRIVATE cm_aot)
target_compile_options(cm-floating-point-checks PRIVATE -Wall -Wextra -Werror -Wpedantic)
add_test(NAME original_float_conversion COMMAND cm-floating-point-checks)
add_executable(cm-decimal-checks decimal_checks.cpp decimal_input_checks.cpp)
target_link_libraries(cm-decimal-checks PRIVATE cm_aot)
target_compile_options(cm-decimal-checks PRIVATE -Wall -Wextra -Werror -Wpedantic)
if(EXISTS "${CM_ORACLE_DIR}/oracle.tsv")
    add_test(NAME original_routines COMMAND cm-aot-checks "${CM_ORACLE_DIR}" "${CM_ORACLE_DIR}/oracle.tsv")
    add_test(NAME original_sound_callbacks COMMAND cm-sound-checks "${CM_ORACLE_DIR}")
endif()
add_library(cm_resources STATIC resource_fork.cpp)
target_include_directories(cm_resources PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")
target_compile_options(cm_resources PRIVATE -Wall -Wextra -Werror -Wpedantic)
add_executable(cm-resource-checks resource_checks.cpp)
target_link_libraries(cm-resource-checks PRIVATE cm_resources)
target_compile_options(cm-resource-checks PRIVATE -Wall -Wextra -Werror -Wpedantic)
add_test(NAME resource_forks COMMAND cm-resource-checks)
add_executable(cm-platform-checks platform_checks.cpp)
target_link_libraries(cm-platform-checks PRIVATE cm_aot)
target_compile_options(cm-platform-checks PRIVATE -Wall -Wextra -Werror -Wpedantic)
add_test(NAME native_filesystem COMMAND cm-platform-checks)
add_executable(cm-fork-io-checks fork_io_checks.cpp)
target_link_libraries(cm-fork-io-checks PRIVATE cm_aot)
target_compile_options(cm-fork-io-checks PRIVATE -Wall -Wextra -Werror -Wpedantic)
add_executable(cm-cpp-object-storage-checks cpp_object_storage_checks.cpp)
target_link_libraries(cm-cpp-object-storage-checks PRIVATE cm_aot)
target_compile_options(cm-cpp-object-storage-checks PRIVATE -Wall -Wextra -Werror -Wpedantic)
add_executable(cm-code-fragment-checks code_fragment_checks.cpp)
target_link_libraries(cm-code-fragment-checks PRIVATE cm_aot)
target_compile_options(cm-code-fragment-checks PRIVATE -Wall -Wextra -Werror -Wpedantic)
add_executable(cm-runtime-lifecycle-checks runtime_lifecycle_checks.cpp)
target_link_libraries(cm-runtime-lifecycle-checks PRIVATE cm_aot)
target_compile_options(cm-runtime-lifecycle-checks PRIVATE -Wall -Wextra -Werror -Wpedantic)
add_executable(cm-file-path-checks file_path_checks.cpp)
target_link_libraries(cm-file-path-checks PRIVATE cm_aot)
target_compile_options(cm-file-path-checks PRIVATE -Wall -Wextra -Werror -Wpedantic)
add_executable(cm-file-controller-checks file_controller_checks.cpp)
target_link_libraries(cm-file-controller-checks PRIVATE cm_aot)
target_compile_options(cm-file-controller-checks PRIVATE -Wall -Wextra -Werror -Wpedantic)
add_executable(cm-file-reference-checks file_reference_checks.cpp)
target_link_libraries(cm-file-reference-checks PRIVATE cm_aot)
target_compile_options(cm-file-reference-checks PRIVATE -Wall -Wextra -Werror -Wpedantic)
add_executable(cm-resource-bridge-checks resource_bridge_checks.cpp)
target_link_libraries(cm-resource-bridge-checks PRIVATE cm_aot cm_resources)
target_compile_options(cm-resource-bridge-checks PRIVATE -Wall -Wextra -Werror -Wpedantic)
add_executable(cm-menu-checks menu_checks.cpp)
target_link_libraries(cm-menu-checks PRIVATE cm_ui cm_resources)
target_compile_options(cm-menu-checks PRIVATE -Wall -Wextra -Werror -Wpedantic)
add_executable(cm-printing-checks printing_checks.cpp)
target_link_libraries(cm-printing-checks PRIVATE cm_aot)
target_compile_options(cm-printing-checks PRIVATE -Wall -Wextra -Werror -Wpedantic)
add_test(NAME native_printing COMMAND cm-printing-checks)
add_executable(cm-input-checks input_checks.mm)
target_link_libraries(cm-input-checks PRIVATE cm_ui)
target_compile_options(cm-input-checks PRIVATE -Wall -Wextra -Werror -Wpedantic -fobjc-arc)
add_test(NAME native_input_queue COMMAND cm-input-checks)
add_test(NAME validation_event_pointer COMMAND cm-input-checks --validation-pointer)
add_executable(cm-callback-checks callback_checks.cpp)
target_link_libraries(cm-callback-checks PRIVATE cm_aot)
target_compile_options(cm-callback-checks PRIVATE -Wall -Wextra -Werror -Wpedantic)
if(EXISTS "${CM_ORACLE_DIR}/oracle.tsv")
    add_test(NAME native_callbacks COMMAND cm-callback-checks "${CM_ORACLE_DIR}")
endif()
add_executable(cm-event-checks event_checks.mm)
target_link_libraries(cm-event-checks PRIVATE cm_aot)
target_compile_options(cm-event-checks PRIVATE -Wall -Wextra -Werror -Wpedantic -fobjc-arc)
if(EXISTS "${CM_ORACLE_DIR}/oracle.tsv")
    add_test(NAME original_input_callbacks COMMAND cm-event-checks "${CM_ORACLE_DIR}")
endif()
add_executable(cm-background-checks background_checks.cpp)
target_link_libraries(cm-background-checks PRIVATE cm_aot)
target_compile_options(cm-background-checks PRIVATE -Wall -Wextra -Werror -Wpedantic)
add_executable(cm-background-selection-checks background_selection_checks.cpp)
target_link_libraries(cm-background-selection-checks PRIVATE cm_aot)
target_compile_options(cm-background-selection-checks PRIVATE -Wall -Wextra -Werror -Wpedantic)
add_executable(cm-floating-input-checks floating_input_checks.cpp)
target_link_libraries(cm-floating-input-checks PRIVATE cm_aot)
target_compile_options(cm-floating-input-checks PRIVATE -Wall -Wextra -Werror -Wpedantic)
add_executable(cm-formatted-output-checks formatted_output_checks.cpp)
target_link_libraries(cm-formatted-output-checks PRIVATE cm_aot)
target_compile_options(cm-formatted-output-checks PRIVATE -Wall -Wextra -Werror -Wpedantic)
add_executable(cm-character-conversion-checks character_conversion_checks.cpp)
target_link_libraries(cm-character-conversion-checks PRIVATE cm_aot)
target_compile_options(cm-character-conversion-checks PRIVATE -Wall -Wextra -Werror -Wpedantic)
add_executable(cm-formatted-input-checks formatted_input_checks.cpp)
target_link_libraries(cm-formatted-input-checks PRIVATE cm_aot)
target_compile_options(cm-formatted-input-checks PRIVATE -Wall -Wextra -Werror -Wpedantic)
add_executable(cm-integer-parsing-checks integer_parsing_checks.cpp)
target_link_libraries(cm-integer-parsing-checks PRIVATE cm_aot)
target_compile_options(cm-integer-parsing-checks PRIVATE -Wall -Wextra -Werror -Wpedantic)
add_executable(cm-memory-primitives-checks memory_primitives_checks.cpp)
target_link_libraries(cm-memory-primitives-checks PRIVATE cm_aot)
target_compile_options(cm-memory-primitives-checks PRIVATE -Wall -Wextra -Werror -Wpedantic)
add_executable(cm-string-core-checks string_core_checks.cpp)
target_link_libraries(cm-string-core-checks PRIVATE cm_aot)
target_compile_options(cm-string-core-checks PRIVATE -Wall -Wextra -Werror -Wpedantic)
add_executable(cm-collection-lookup-checks collection_lookup_checks.cpp)
target_link_libraries(cm-collection-lookup-checks PRIVATE cm_aot)
target_compile_options(cm-collection-lookup-checks PRIVATE -Wall -Wextra -Werror -Wpedantic)
add_executable(cm-network-checks network_checks.cpp)
target_link_libraries(cm-network-checks PRIVATE cm_aot)
target_compile_options(cm-network-checks PRIVATE -Wall -Wextra -Werror -Wpedantic)
add_executable(cm-day-advance-checks day_advance_checks.cpp)
target_link_libraries(cm-day-advance-checks PRIVATE cm_aot)
target_compile_options(cm-day-advance-checks PRIVATE -Wall -Wextra -Werror -Wpedantic)
add_executable(cm-match-arithmetic-checks match_arithmetic_checks.cpp)
target_link_libraries(cm-match-arithmetic-checks PRIVATE cm_aot)
target_compile_options(cm-match-arithmetic-checks PRIVATE -Wall -Wextra -Werror -Wpedantic)
add_executable(cm-match-replay-checks match_replay_checks.cpp)
target_link_libraries(cm-match-replay-checks PRIVATE cm_aot)
target_compile_options(cm-match-replay-checks PRIVATE -Wall -Wextra -Werror -Wpedantic)
add_test(NAME private_match_replay COMMAND cm-match-replay-checks)
add_executable(cm-match-scheduler-checks match_scheduler_checks.cpp)
target_link_libraries(cm-match-scheduler-checks PRIVATE cm_aot)
target_compile_options(cm-match-scheduler-checks PRIVATE -Wall -Wextra -Werror -Wpedantic)
add_test(NAME original_match_scheduler COMMAND cm-match-scheduler-checks)
add_executable(cm-control-cursor-checks control_cursor_checks.cpp)
target_link_libraries(cm-control-cursor-checks PRIVATE cm_aot)
target_compile_options(cm-control-cursor-checks PRIVATE -Wall -Wextra -Werror -Wpedantic)
add_test(NAME original_control_cursor COMMAND cm-control-cursor-checks)
add_executable(cm-unsigned-division-checks unsigned_division_checks.cpp)
target_link_libraries(cm-unsigned-division-checks PRIVATE cm_aot)
target_compile_options(cm-unsigned-division-checks PRIVATE -Wall -Wextra -Werror -Wpedantic)
add_test(NAME original_unsigned_division COMMAND cm-unsigned-division-checks)
add_executable(cm-match-presentation-checks match_presentation_checks.cpp)
target_link_libraries(cm-match-presentation-checks PRIVATE cm_aot)
target_compile_options(cm-match-presentation-checks PRIVATE -Wall -Wextra -Werror -Wpedantic)
add_test(NAME original_match_presentation COMMAND cm-match-presentation-checks)
add_executable(cm-shutdown-checks shutdown_checks.cpp)
target_link_libraries(cm-shutdown-checks PRIVATE cm_probe_host)
target_compile_options(cm-shutdown-checks PRIVATE -Wall -Wextra -Werror -Wpedantic)
set(CM_GAME_DIR "" CACHE PATH "Verified original Mac game files for resource lifecycle checks")
if(TARGET cm-vector-context-checks)
    add_test(NAME original_vector_context_storage COMMAND cm-vector-context-checks "${CM_ORACLE_DIR}")
    add_test(NAME modern_exception_frame_equivalence COMMAND cm-exception-frame-checks "${CM_ORACLE_DIR}")
    add_test(NAME modern_exception_cleanup_equivalence COMMAND cm-exception-cleanup-checks "${CM_ORACLE_DIR}")
    add_test(NAME modern_exception_matching_equivalence COMMAND cm-exception-matching-checks "${CM_ORACLE_DIR}")
    add_test(NAME modern_exception_search_equivalence COMMAND cm-exception-search-checks "${CM_ORACLE_DIR}")
    add_test(NAME modern_exception_transfer_equivalence COMMAND cm-exception-transfer-checks "${CM_ORACLE_DIR}")
    add_test(NAME modern_exception_capture_equivalence COMMAND cm-exception-capture-checks "${CM_ORACLE_DIR}")
    add_test(NAME modern_exception_handler_equivalence COMMAND cm-exception-handler-checks "${CM_ORACLE_DIR}")
    add_test(NAME modern_exception_production_integration COMMAND cm-exception-integration-checks "${CM_ORACLE_DIR}")
endif()
if(EXISTS "${CM_ORACLE_DIR}/program.json")
    add_test(NAME modern_rng_core_equivalence COMMAND cm-rng-core-checks "${CM_ORACLE_DIR}" "${CMAKE_BINARY_DIR}/rng-core-private")
    add_test(NAME modern_date_construction_equivalence COMMAND cm-date-construction-checks "${CM_ORACLE_DIR}" "${CMAKE_BINARY_DIR}/date-construction-private.bin" --composed)
    add_test(NAME modern_stream_io_equivalence COMMAND cm-stream-io-checks "${CM_ORACLE_DIR}")
    add_test(NAME native_fork_io COMMAND cm-fork-io-checks "${CM_ORACLE_DIR}")
    add_test(NAME modern_file_controller_equivalence COMMAND cm-file-controller-checks "${CM_ORACLE_DIR}")
    add_test(NAME modern_cpp_object_storage_equivalence COMMAND cm-cpp-object-storage-checks "${CM_ORACLE_DIR}")
    add_test(NAME modern_code_fragment_equivalence COMMAND cm-code-fragment-checks "${CM_ORACLE_DIR}")
    add_test(NAME modern_runtime_lifecycle_equivalence COMMAND cm-runtime-lifecycle-checks "${CM_ORACLE_DIR}")
    add_test(NAME modern_file_path_equivalence COMMAND cm-file-path-checks "${CM_ORACLE_DIR}")
    add_test(NAME native_file_references COMMAND cm-file-reference-checks "${CM_ORACLE_DIR}")
    add_test(NAME modern_stream_buffer_equivalence COMMAND cm-stream-buffer-checks "${CM_ORACLE_DIR}")
    add_test(NAME modern_allocator_equivalence COMMAND cm-allocator-equivalence-checks "${CM_ORACLE_DIR}")
    add_test(NAME original_allocator_replenishment COMMAND cm-allocator-checks "${CM_ORACLE_DIR}")
    add_test(NAME original_background_selection COMMAND cm-background-checks "${CM_ORACLE_DIR}")
    add_test(NAME modern_background_selection_equivalence COMMAND cm-background-selection-checks "${CM_ORACLE_DIR}")
    add_test(NAME modern_floating_input_equivalence COMMAND cm-floating-input-checks "${CM_ORACLE_DIR}")
    add_test(NAME modern_formatted_output_equivalence COMMAND cm-formatted-output-checks "${CM_ORACLE_DIR}")
    add_test(NAME modern_character_conversion_equivalence COMMAND cm-character-conversion-checks "${CM_ORACLE_DIR}")
    add_test(NAME modern_formatted_input_equivalence COMMAND cm-formatted-input-checks "${CM_ORACLE_DIR}")
    add_test(NAME modern_integer_parsing_equivalence COMMAND cm-integer-parsing-checks "${CM_ORACLE_DIR}")
    add_test(NAME modern_memory_primitives_equivalence COMMAND cm-memory-primitives-checks "${CM_ORACLE_DIR}")
    add_test(NAME modern_string_core_equivalence COMMAND cm-string-core-checks "${CM_ORACLE_DIR}")
    add_test(NAME modern_collection_lookup_equivalence COMMAND cm-collection-lookup-checks "${CM_ORACLE_DIR}")
    add_test(NAME original_decimal_formatting COMMAND cm-decimal-checks "${CM_ORACLE_DIR}")
    add_test(NAME original_network_unavailable COMMAND cm-network-checks "${CM_ORACLE_DIR}")
    add_test(NAME private_season_transcript COMMAND cm-season-reference-checks "${CM_ORACLE_DIR}")
    add_test(NAME modern_calendar_oracle COMMAND cm-calendar-reference-checks "${CM_ORACLE_DIR}" --modern)
    add_test(NAME modern_calendar_equivalence COMMAND cm-calendar-checks "${CM_ORACLE_DIR}")
    add_test(NAME modern_date_difference_equivalence COMMAND cm-date-difference-checks "${CM_ORACLE_DIR}")
    add_test(NAME modern_date_weekday_equivalence COMMAND cm-date-weekday-checks "${CM_ORACLE_DIR}")
    add_test(NAME modern_date_day_of_month_equivalence COMMAND cm-date-day-of-month-checks "${CM_ORACLE_DIR}")
    add_test(NAME modern_date_month_equivalence COMMAND cm-date-month-checks "${CM_ORACLE_DIR}")
    add_test(NAME modern_date_before_equivalence COMMAND cm-date-before-checks "${CM_ORACLE_DIR}")
    add_test(NAME modern_date_ordering_equivalence COMMAND cm-date-ordering-checks "${CM_ORACLE_DIR}")
    add_test(NAME original_calendar_reference COMMAND cm-calendar-reference-checks "${CM_ORACLE_DIR}")
    add_test(NAME original_competition_dispatch COMMAND cm-competition-dispatch-checks "${CM_ORACLE_DIR}")
    add_test(NAME original_day_advance_arithmetic COMMAND cm-day-advance-checks "${CM_ORACLE_DIR}")
    add_test(NAME original_match_arithmetic COMMAND cm-match-arithmetic-checks "${CM_ORACLE_DIR}")
    if(EXISTS "${CM_GAME_DIR}/CM 01-02")
        add_test(NAME original_application_shutdown COMMAND cm-shutdown-checks "${CM_ORACLE_DIR}" "${CM_GAME_DIR}")
    endif()
endif()

set(CM_DATE_SNAPSHOT "" CACHE FILEPATH "Private real snapshot for Date presentation checks")
if(EXISTS "${CM_DATE_SNAPSHOT}")
    add_test(NAME modern_date_subsystem_streams COMMAND cm-date-subsystem_audit-checks "${CM_DATE_SNAPSHOT}" "${CMAKE_BINARY_DIR}/date-audit-private")
    add_test(NAME modern_date_presentation_equivalence COMMAND cm-date-presentation-checks "${CM_DATE_SNAPSHOT}" "${CMAKE_BINARY_DIR}/date-presentation-private.bin" --composed)
endif()

add_executable(cm-direct-command-checks direct_command_checks.cpp)
target_link_libraries(cm-direct-command-checks PRIVATE cm_aot)
target_compile_options(cm-direct-command-checks PRIVATE -Wall -Wextra -Werror -Wpedantic)
if(CM_ORACLE_DIR)
    add_test(NAME native_direct_commands COMMAND cm-direct-command-checks "${CM_ORACLE_DIR}")
endif()

add_executable(cm-presentation-ui-checks presentation_ui_checks.mm)
target_link_libraries(cm-presentation-ui-checks PRIVATE cm_ui)
target_compile_options(cm-presentation-ui-checks PRIVATE -Wall -Wextra -Werror -Wpedantic -fobjc-arc)
add_test(NAME native_presentation_ui COMMAND cm-presentation-ui-checks)

add_executable(cm-native-drawing-checks native_drawing_checks.mm)
target_link_libraries(cm-native-drawing-checks PRIVATE cm_aot)
target_compile_options(cm-native-drawing-checks PRIVATE -Wall -Wextra -Werror -Wpedantic -fobjc-arc)
if(EXISTS "${CM_ORACLE_DIR}/program.json")
    add_test(NAME native_vector_drawing COMMAND cm-native-drawing-checks "${CM_ORACLE_DIR}")
endif()
