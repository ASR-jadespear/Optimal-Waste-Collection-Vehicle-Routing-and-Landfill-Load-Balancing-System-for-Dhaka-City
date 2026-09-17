@echo off
REM ==============================================================================
REM Optimal Waste Collection Vehicle Routing and Landfill Load-Balancing System
REM Windows One-Click Build and Launch Script
REM ==============================================================================

echo ======================================================================
echo  Starting Dhaka Waste Routing ^& Landfill Load-Balancing System...
echo ======================================================================

where cmake >nul 2>nul
if %ERRORLEVEL% neq 0 (
    echo [ERROR] CMake was not found in PATH. Please install CMake or Visual Studio with C++ tools.
    pause
    exit /b 1
)

if not exist "build" (
    echo [INFO] Generating build project with CMake...
    cmake -B build
    if %ERRORLEVEL% neq 0 (
        echo [ERROR] CMake configuration failed.
        pause
        exit /b 1
    )
)

echo [INFO] Building project...
cmake --build build --config Release
if %ERRORLEVEL% neq 0 (
    echo [ERROR] Compilation failed.
    pause
    exit /b 1
)

echo [INFO] Launching Simulation GUI...
if exist "build\Release\dhaka_waste_sim.exe" (
    start "" "build\Release\dhaka_waste_sim.exe"
) else if exist "build\dhaka_waste_sim.exe" (
    start "" "build\dhaka_waste_sim.exe"
) else (
    echo [ERROR] dhaka_waste_sim.exe not found in build directory.
    pause
    exit /b 1
)

