#!/usr/bin/env bash
set -euo pipefail

PROJECT_DIR="$(cd "$(dirname "$0")" && pwd)"
BUILD_DIR="${PROJECT_DIR}/build"
TOOLCHAIN_FILE="${PROJECT_DIR}/cmake/gcc-arm-none-eabi.cmake"
FLASH_CFG="${PROJECT_DIR}/flash.cfg"
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
echo "4. Fix compile_commands.json path format (convert to Windows drive letters)..."
echo "========================================"
if [ -f "${COMPILE_COMMANDS}" ]; then
    if command -v perl &> /dev/null; then
        echo "Using perl to convert all paths..."
        perl -i -pe 's|/([a-zA-Z])/|uc($1).":/"|ge; s|\\|/|g' "${COMPILE_COMMANDS}"
        echo "Paths converted to Windows format."
    else
        echo "WARNING: perl not found, falling back to sed for common drive letters."
        # Manually list drive letters this project may use
        sed -i -e 's|/f/|F:/|g' -e 's|/d/|D:/|g' \
               -e 's|/c/|C:/|g' -e 's|/e/|E:/|g' \
               -e 's|\\|/|g' "${COMPILE_COMMANDS}"
        echo "Common drive letters converted via sed. Add more if needed."
    fi
else
    echo "${COMPILE_COMMANDS} not found, skipping path fix."
fi

echo "========================================"
echo "5. Generate .bin / .hex / .dis..."
echo "========================================"
cd "${BUILD_DIR}"
arm-none-eabi-objcopy -O binary "${PROJECT_NAME}.elf" "${PROJECT_NAME}.bin"
arm-none-eabi-objcopy -O binary "${PROJECT_NAME}.elf" "${PROJECT_NAME}.hex"
arm-none-eabi-objdump -d "${PROJECT_NAME}.elf" > "${PROJECT_NAME}.dis"
echo "${PROJECT_NAME}.bin generated."
echo "${PROJECT_NAME}.hex generated."
echo "${PROJECT_NAME}.dis generated."


# echo "========================================"
# echo "6. Flash and verify (OpenOCD)..."
# echo "========================================"
# if ! command -v openocd &> /dev/null; then
#     echo "ERROR: openocd not found in PATH. Please install it first."
#     exit 1
# fi
# cd "${PROJECT_DIR}"          # back to project root so flash.cfg relative paths work
# openocd -f "${FLASH_CFG}"

echo "========================================"
echo "All done!"
echo "========================================"
