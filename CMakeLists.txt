cmake_minimum_required(VERSION 3.21)
project(LedgerLite LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_AUTOMOC ON)

find_package(Threads REQUIRED)
find_package(Qt6 REQUIRED COMPONENTS Widgets)

# core: plain C++, no Qt
add_library(ledger_core STATIC core/src/Ledger.cpp core/src/Money.cpp)
target_include_directories(ledger_core PUBLIC core/include)
target_compile_options(ledger_core PRIVATE -Wall -Wextra -Wpedantic)

# tests: assert-based, no framework
enable_testing()
add_executable(ledger_tests tests/test_ledger.cpp)
target_link_libraries(ledger_tests PRIVATE ledger_core Threads::Threads)
add_test(NAME ledger_tests COMMAND ledger_tests)

# app: Qt Widgets front end, calls core only
add_executable(ledger_app app/main.cpp)
target_link_libraries(ledger_app PRIVATE ledger_core Qt6::Widgets)
