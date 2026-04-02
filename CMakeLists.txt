cmake_minimum_required(VERSION 3.20)

project(Monkey VERSION 0.1.0 LANGUAGES CXX)

include(CTest)
include(FetchContent)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_library(project_warnings INTERFACE)

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

add_executable(monkey_smoke_tests
    src/lexer.cpp
    tests/lexer_test.cpp
)

target_include_directories(monkey_smoke_tests
    PRIVATE
        include
)

target_link_libraries(monkey_smoke_tests
    PRIVATE
        project_warnings
        Catch2::Catch2WithMain
)

include(Catch)
catch_discover_tests(monkey_smoke_tests)
