cmake_minimum_required(VERSION 3.20)

project(Monkey VERSION 0.1.0 LANGUAGES CXX)

include(CTest)
include(FetchContent)
include(CheckCXXCompilerFlag)
include(CheckIPOSupported)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_library(project_warnings INTERFACE)
add_library(project_optimizations INTERFACE)

option(MONKEY_ENABLE_LTO "Enable link-time optimization for supported builds" OFF)
option(MONKEY_ENABLE_NATIVE_ARCH "Enable native CPU tuning for local builds" OFF)

if(MONKEY_ENABLE_LTO)
    check_ipo_supported(RESULT MONKEY_IPO_SUPPORTED OUTPUT MONKEY_IPO_ERROR)

    if(NOT MONKEY_IPO_SUPPORTED)
        message(FATAL_ERROR "MONKEY_ENABLE_LTO is ON but IPO/LTO is not supported: ${MONKEY_IPO_ERROR}")
    endif()
endif()

if(MONKEY_ENABLE_NATIVE_ARCH)
    if(MSVC)
        message(FATAL_ERROR "MONKEY_ENABLE_NATIVE_ARCH is not configured for MSVC in this project.")
    endif()

    check_cxx_compiler_flag("-march=native" MONKEY_HAS_MARCH_NATIVE)

    if(NOT MONKEY_HAS_MARCH_NATIVE)
        message(FATAL_ERROR "MONKEY_ENABLE_NATIVE_ARCH is ON but -march=native is not supported by ${CMAKE_CXX_COMPILER_ID}.")
    endif()

    target_compile_options(project_optimizations INTERFACE -march=native)
endif()

target_compile_options(project_warnings INTERFACE
    $<$<CXX_COMPILER_ID:MSVC>:/W4 /permissive->
    $<$<NOT:$<CXX_COMPILER_ID:MSVC>>:-Wall -Wextra -Wpedantic>
)

FetchContent_Declare(
    Catch2
    GIT_REPOSITORY https://github.com/catchorg/Catch2.git
    GIT_TAG v3.6.0
)

FetchContent_MakeAvailable(Catch2)

enable_testing()

add_library(monkey_lib
    src/evaluator.cpp
    src/lexer.cpp
    src/parser.cpp
)

target_include_directories(monkey_lib
    PRIVATE
        include
)

target_link_libraries(monkey_lib
    PRIVATE
        project_optimizations
        project_warnings
)

if(MONKEY_ENABLE_LTO)
    set_property(TARGET monkey_lib PROPERTY INTERPROCEDURAL_OPTIMIZATION TRUE)
endif()

add_executable(monkey_repl
    src/main.cpp
    src/repl.cpp
)

target_include_directories(monkey_repl
    PRIVATE
        include
)

target_link_libraries(monkey_repl
    PRIVATE
        monkey_lib
        project_optimizations
        project_warnings
)

if(MONKEY_ENABLE_LTO)
    set_property(TARGET monkey_repl PROPERTY INTERPROCEDURAL_OPTIMIZATION TRUE)
endif()

add_executable(monkey_smoke_tests
    tests/ast_test.cpp
    tests/evaluator_test.cpp
    tests/lexer_test.cpp
    tests/object_test.cpp
    tests/parser_test.cpp
)

add_executable(monkey_performance_tests
    tests/performance_test.cpp
)

target_include_directories(monkey_smoke_tests
    PRIVATE
        include
)

target_link_libraries(monkey_smoke_tests
    PRIVATE
        monkey_lib
        project_optimizations
        project_warnings
        Catch2::Catch2WithMain
)

target_include_directories(monkey_performance_tests
    PRIVATE
        include
)

target_link_libraries(monkey_performance_tests
    PRIVATE
        monkey_lib
        project_optimizations
        project_warnings
        Catch2::Catch2WithMain
)

if(MONKEY_ENABLE_LTO)
    set_property(TARGET monkey_smoke_tests PROPERTY INTERPROCEDURAL_OPTIMIZATION TRUE)
    set_property(TARGET monkey_performance_tests PROPERTY INTERPROCEDURAL_OPTIMIZATION TRUE)
endif()

include(Catch)
catch_discover_tests(monkey_smoke_tests)
