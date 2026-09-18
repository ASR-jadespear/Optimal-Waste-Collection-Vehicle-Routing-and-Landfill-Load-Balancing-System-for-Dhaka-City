#!/bin/bash
set -e

# ==============================================================================
# Optimal Waste Collection Vehicle Routing and Landfill Load-Balancing System
# Linux One-Click Build and Launch Script
# ==============================================================================

PROJECT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$PROJECT_DIR"

echo "======================================================================"
echo " Starting Dhaka Waste Routing & Landfill Load-Balancing System...    "
echo "======================================================================"

# 1. Compile project using CMake
echo "[INFO] Building project with CMake..."
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j$(nproc)

# 2. Launch the application
# If partition has noexec mount flag (e.g. Windows NTFS mount), run from /tmp
if [ -x "./build/dhaka_waste_sim" ] && ./build/dhaka_waste_sim --test 2>/dev/null; then
    echo "[INFO] Launching simulation GUI..."
    ./build/dhaka_waste_sim "$@"
else
    echo "[INFO] Launching simulation GUI (via /tmp buffer)..."
    cp ./build/dhaka_waste_sim /tmp/dhaka_waste_sim
    chmod +x /tmp/dhaka_waste_sim
    /tmp/dhaka_waste_sim "$@"
fi

