#!/usr/bin/env bash
set -euo pipefail

PROJECT_DIR="$(cd "$(dirname "$0")" && pwd)"
BUILD_DIR="${PROJECT_DIR}/build"
TOOLCHAIN_FILE="${PROJECT_DIR}/cmake/gcc-arm-none-eabi.cmake"
COMPILE_COMMANDS="${BUILD_DIR}/compile_commands.json"
PROJECT_NAME="MCU_BOOT"

echo "========================================"
echo "1. Prepare build directory..."
echo "========================================"
rm -rf "${BUILD_DIR}"
mkdir -p "${BUILD_DIR}"

echo "========================================"
echo "2. Run CMake configure (Ninja)..."
echo "========================================"
cd "${BUILD_DIR}"
cmake -G Ninja .. \
    -DCMAKE_TOOLCHAIN_FILE="${TOOLCHAIN_FILE}" \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_EXPORT_COMPILE_COMMANDS=ON

echo "========================================"
echo "3. Build project (ninja)..."
echo "========================================"
ninja -j $(nproc 2>/dev/null || echo 4)

echo "========================================"
echo "4. Fix compile_commands.json path format..."
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

echo "========================================"
echo "5. Generate .bin / .hex / .dis..."
echo "========================================"
cd "${BUILD_DIR}"
arm-none-eabi-objcopy -O binary "${PROJECT_NAME}.elf" "${PROJECT_NAME}.bin"
arm-none-eabi-objcopy -O ihex "${PROJECT_NAME}.elf" "${PROJECT_NAME}.hex"
arm-none-eabi-objdump -d "${PROJECT_NAME}.elf" > "${PROJECT_NAME}.dis"
echo "${PROJECT_NAME}.bin generated."

echo "========================================"
echo "All done!"
echo "========================================"
