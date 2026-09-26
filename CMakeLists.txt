cmake_minimum_required(VERSION 3.28)
project(CM0102NativeChecks LANGUAGES CXX)

if(NOT APPLE OR NOT CMAKE_SYSTEM_PROCESSOR MATCHES "arm64|aarch64")
    message(FATAL_ERROR "These checks target a native Apple Silicon host")
endif()
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_OSX_ARCHITECTURES arm64)

set(RECM_SOURCE_DIR "" CACHE PATH "Path to the existing ReCM repository (read-only input)")
if(NOT EXISTS "${RECM_SOURCE_DIR}/src/cpp/tests/test_db_structs.cpp")
    message(FATAL_ERROR "Set RECM_SOURCE_DIR to the existing ReCM repository")
endif()

# This intentionally builds the existing, independent record-layout checks.
# Linking the entire Windows engine currently fails its 32-bit pointer-layout
# assertions. Disabling those assertions would hide a real porting defect.
add_executable(recm_native_db_checks "${RECM_SOURCE_DIR}/src/cpp/tests/test_db_structs.cpp")
target_include_directories(recm_native_db_checks PRIVATE "${RECM_SOURCE_DIR}/src/cpp")
target_compile_options(recm_native_db_checks PRIVATE -Wall -Wextra -Werror -Wpedantic -UNDEBUG)
enable_testing()
add_test(NAME recm_native_db_checks COMMAND recm_native_db_checks)
