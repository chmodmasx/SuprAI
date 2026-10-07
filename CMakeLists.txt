cmake_minimum_required(VERSION 3.24)

project(SuprAI
    VERSION 0.1.0
    DESCRIPTION "Linux-native modular AI desktop"
    LANGUAGES CXX
)

option(SUPRAI_BUILD_TESTS "Build SuprAI tests" ON)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_AUTOMOC ON)

find_package(Qt6 6.8 REQUIRED COMPONENTS
    Core
    Gui
    Widgets
    Qml
    Quick
    QuickControls2
    Network
    DBus
)

qt_standard_project_setup(REQUIRES 6.8)

add_subdirectory(src)

if(SUPRAI_BUILD_TESTS)
    enable_testing()
    find_package(Qt6 6.8 REQUIRED COMPONENTS Test)
    add_subdirectory(tests)
endif()
