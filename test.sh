#!/bin/bash
set -e

PROJECT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$PROJECT_DIR"

if [ ! -f "build/algo_tests" ]; then
    echo "[INFO] Compiling test suite..."
    cmake -B build -DCMAKE_BUILD_TYPE=Release
    cmake --build build --target algo_tests -j$(nproc)
fi

# Run test suite
if [ -x "./build/algo_tests" ] && ./build/algo_tests 2>/dev/null; then
    ./build/algo_tests
else
    cp ./build/algo_tests /tmp/algo_tests
    chmod +x /tmp/algo_tests
    /tmp/algo_tests
fi

