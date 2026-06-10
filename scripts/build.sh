#!/bin/bash
# =============================================================================
# PizzaFactory build script (macOS / Linux)
# Usage: ./scripts/build.sh [Release|Debug]
# =============================================================================
set -e

BUILD_TYPE="${1:-Debug}"
PROJECT_DIR="$(cd "$(dirname "$0")/.." && pwd)"
BUILD_DIR="${PROJECT_DIR}/build"

echo "PizzaFactory build start (${BUILD_TYPE})"
echo "==========================================="

cmake -S "${PROJECT_DIR}" -B "${BUILD_DIR}" \
    -DCMAKE_BUILD_TYPE="${BUILD_TYPE}" \
    -DCMAKE_EXPORT_COMPILE_COMMANDS=ON

cmake --build "${BUILD_DIR}" --parallel "$(sysctl -n hw.ncpu 2>/dev/null || nproc 2>/dev/null || echo 4)"

echo ""
echo "Build succeeded! Run it with:"
echo "   ${BUILD_DIR}/PizzaFactory"
