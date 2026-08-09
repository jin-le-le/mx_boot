#!/usr/bin/env bash
# ============================================================
#  HC32F460PETB App Printf Project - One-shot Build Script
#
#  Usage: run from the app_printf/ directory
#    bash run.sh         # Release build
#    bash run.sh Debug   # Debug build
#
#  Prerequisites:
#    - arm-none-eabi-gcc in PATH
#    - cmake + ninja installed
# ============================================================
set -euo pipefail

PROJECT_DIR="$(cd "$(dirname "$0")" && pwd)"
BUILD_DIR="${PROJECT_DIR}/build"
TOOLCHAIN_FILE="${PROJECT_DIR}/cmake/gcc-hc32f460.cmake"
COMPILE_COMMANDS="${BUILD_DIR}/compile_commands.json"
PROJECT_NAME="hc32_app_printf"

BUILD_TYPE="${1:-Release}"

echo "========================================"
echo "1. Prepare build directory (clean)..."
echo "========================================"
rm -rf "${BUILD_DIR}"
mkdir -p "${BUILD_DIR}"

echo "========================================"
echo "2. Run CMake configuration (Ninja)..."
echo "    Build type: ${BUILD_TYPE}"
echo "    Toolchain:  ${TOOLCHAIN_FILE}"
echo "========================================"
cd "${BUILD_DIR}"
cmake -G Ninja .. \
    -DCMAKE_TOOLCHAIN_FILE="${TOOLCHAIN_FILE}" \
    -DCMAKE_BUILD_TYPE="${BUILD_TYPE}" \
    -DCMAKE_EXPORT_COMPILE_COMMANDS=ON

echo ""
echo "========================================"
echo "3. Build the project (ninja)..."
echo "========================================"
ninja -j $(nproc 2>/dev/null || echo 4)

echo ""
echo "========================================"
echo "4. Fix compile_commands.json path format..."
echo "   (for VSCode IntelliSense)"
echo "========================================"
if [ -f "${COMPILE_COMMANDS}" ]; then
    if command -v perl &> /dev/null; then
        perl -i -pe 's|/([a-zA-Z])/|uc($1).":/"|ge; s|\\|/|g' "${COMPILE_COMMANDS}"
        echo "Paths converted to Windows format."
    else
        sed -i -e 's|/f/|F:/|g' -e 's|/d/|D:/|g' \
               -e 's|/c/|C:/|g' -e 's|/e/|E:/|g' \
               -e 's|\\|/|g' "${COMPILE_COMMANDS}"
    fi
fi

echo ""
echo "========================================"
echo "5. Build artifacts (CMake POST_BUILD already generated them)"
echo "========================================"
ls -la "${BUILD_DIR}/${PROJECT_NAME}".{elf,bin,hex,dis,map} 2>/dev/null || true

echo ""
echo "========================================"
echo "All done!"
echo "========================================"
echo ""
echo "Artifacts location: ${BUILD_DIR}/"
echo "  ${PROJECT_NAME}.elf   - ELF executable (for debugging)"
echo "  ${PROJECT_NAME}.bin   - Raw binary (for J-Flash flashing)"
echo "  ${PROJECT_NAME}.hex   - HEX file (for J-Link / OpenOCD flashing)"
echo "  ${PROJECT_NAME}.dis   - Disassembly (for inspecting addresses)"
echo "  ${PROJECT_NAME}.map   - Symbol map (for inspecting sizes)"
echo ""
echo "Flashing command example (J-Flash / J-Link Commander):"
echo "  JLink.exe -device HC32F460PE -if SWD -speed 4000 -autoconnect 1"
echo "  > loadfile ${PROJECT_NAME}.hex"
echo "  > r"
echo "  > g"
echo ""
