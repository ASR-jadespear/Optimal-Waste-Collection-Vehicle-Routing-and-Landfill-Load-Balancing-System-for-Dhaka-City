cmake_minimum_required(VERSION 3.16)
project(DhakaWasteRoutingSystem VERSION 1.0.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Compiler warnings and optimization
if(MSVC)
    add_compile_options(/W4 /O2 /permissive-)
else()
    add_compile_options(-Wall -Wextra -Wno-unused-parameter -O2)
endif()

# Core include directories
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

# -----------------------------------------------------------------------------
# Dependency Management: Find system Raylib or automatically FetchContent
# -----------------------------------------------------------------------------
find_package(raylib 5.0 QUIET)

if(NOT raylib_FOUND)
    message(STATUS "System Raylib not found. Using CMake FetchContent to download Raylib 5.5.0 automatically...")
    include(FetchContent)
    FetchContent_Declare(
        raylib
        GIT_REPOSITORY https://github.com/raysan5/raylib.git
        GIT_TAG 5.5
        GIT_SHALLOW TRUE
    )
    # Set Raylib build options for fast build
    set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
    set(BUILD_GAMES OFF CACHE BOOL "" FORCE)
    FetchContent_MakeAvailable(raylib)
    message(STATUS "Raylib downloaded and configured via FetchContent.")
else()
    message(STATUS "Using system-installed Raylib (${raylib_VERSION})")
endif()

# -----------------------------------------------------------------------------
# Source Files
# -----------------------------------------------------------------------------
set(CORE_SOURCES
    src/core/Graph.cpp
    src/core/Vehicle.cpp
)

set(ALGO_SOURCES
    src/algorithms/MergeSort.cpp
    src/algorithms/KnapsackDP.cpp
    src/algorithms/GreedyTSP.cpp
    src/algorithms/Pathfinding.cpp
    src/algorithms/MaxFlow.cpp
)

set(SIM_SOURCES
    src/simulation/DhakaMapData.cpp
    src/simulation/SimulationEngine.cpp
)

set(UI_SOURCES
    src/ui/CameraController.cpp
    src/ui/UIComponents.cpp
    src/ui/Renderer.cpp
)

# -----------------------------------------------------------------------------
# Target 1: Console Unit Tests Executable (validates algorithms independently)
# -----------------------------------------------------------------------------
add_executable(algo_tests
    tests/test_algorithms.cpp
    ${CORE_SOURCES}
    ${ALGO_SOURCES}
    ${SIM_SOURCES}
)

# -----------------------------------------------------------------------------
# Target 2: Main Simulation Application (Raylib GUI)
# -----------------------------------------------------------------------------
add_executable(dhaka_waste_sim
    src/main.cpp
    ${CORE_SOURCES}
    ${ALGO_SOURCES}
    ${SIM_SOURCES}
    ${UI_SOURCES}
)

target_link_libraries(dhaka_waste_sim PRIVATE raylib)

if(WIN32)
    target_link_libraries(dhaka_waste_sim PRIVATE winmm gdi32 user32 shell32)
endif()

# Output build information
message(STATUS "Targets configured: algo_tests, dhaka_waste_sim")

