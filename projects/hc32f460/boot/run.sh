#!/usr/bin/env bash
# HC32F460 Bootloader one-shot build
set -euo pipefail

PROJECT_DIR="$(cd "$(dirname "$0")" && pwd)"
BUILD_DIR="${PROJECT_DIR}/build"
TOOLCHAIN_FILE="${PROJECT_DIR}/cmake/gcc-hc32f460.cmake"
COMPILE_COMMANDS="${BUILD_DIR}/compile_commands.json"
PROJECT_NAME="hc32_boot"

BUILD_TYPE="${1:-Release}"

echo "========================================"
echo "1. Clean build directory..."
echo "========================================"
rm -rf "${BUILD_DIR}"
mkdir -p "${BUILD_DIR}"

echo "========================================"
echo "2. CMake configure (Ninja)..."
echo "    Type: ${BUILD_TYPE}"
echo "========================================"
cd "${BUILD_DIR}"
cmake -G Ninja .. \
    -DCMAKE_TOOLCHAIN_FILE="${TOOLCHAIN_FILE}" \
    -DCMAKE_BUILD_TYPE="${BUILD_TYPE}" \
    -DCMAKE_EXPORT_COMPILE_COMMANDS=ON

echo ""
echo "========================================"
echo "3. Build (ninja)..."
echo "========================================"
ninja -j $(nproc 2>/dev/null || echo 4)

echo ""
echo "========================================"
echo "4. Fix compile_commands.json paths..."
echo "========================================"
if [ -f "${COMPILE_COMMANDS}" ]; then
    if command -v perl &> /dev/null; then
        perl -i -pe 's|/([a-zA-Z])/|uc($1).":/"|ge; s|\\|/|g' "${COMPILE_COMMANDS}"
    fi
fi

echo ""
echo "========================================"
echo "Done!"
echo "========================================"
echo ""
echo "Artifacts:"
ls -la "${BUILD_DIR}/${PROJECT_NAME}".{elf,bin,hex} 2>/dev/null || true
echo ""
echo "Flash via J-Flash or OpenOCD:"
echo "  loadfile build/${PROJECT_NAME}.hex"
