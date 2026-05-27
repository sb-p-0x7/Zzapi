#!/bin/bash
# =============================================================================
# PizzaFactory 빌드 스크립트 (macOS / Linux)
# 사용법: ./scripts/build.sh [Release|Debug]
# =============================================================================
set -e

BUILD_TYPE="${1:-Debug}"
PROJECT_DIR="$(cd "$(dirname "$0")/.." && pwd)"
BUILD_DIR="${PROJECT_DIR}/build"

echo "🍕 PizzaFactory 빌드 시작 (${BUILD_TYPE})"
echo "==========================================="

cmake -S "${PROJECT_DIR}" -B "${BUILD_DIR}" \
    -DCMAKE_BUILD_TYPE="${BUILD_TYPE}" \
    -DCMAKE_EXPORT_COMPILE_COMMANDS=ON

cmake --build "${BUILD_DIR}" --parallel "$(sysctl -n hw.ncpu 2>/dev/null || nproc 2>/dev/null || echo 4)"

echo ""
echo "🎉 빌드 성공! 실행하려면:"
echo "   ${BUILD_DIR}/PizzaFactory"
