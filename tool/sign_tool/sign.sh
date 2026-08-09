#!/bin/bash
# ============================================================
#  MCU_BOOT firmware signing script
#
#  Usage (from any directory):
#    ./sign.sh app.bin                 # default version 1.0.0
#    ./sign.sh app.bin 2.1.0           # specify version
#
#  Generates a key pair automatically on first run, then signs in one command.
# ============================================================

set -e

FIRMWARE="$1"
VERSION="${2:-1.0.0}"
ADDR="${3:-0x08010000}"   # default F407 address; F103 uses 0x08006400

if [[ -z "$FIRMWARE" ]]; then
    echo "Usage: ./sign.sh <firmware.bin> [version] [app address]"
    echo "Examples:"
    echo "  ./sign.sh app.bin 2.1.0                    # F407 (default 0x08010000)"
    echo "  ./sign.sh app.bin 2.1.0 0x08006400         # F103"
    exit 1
fi

if [[ ! -f "$FIRMWARE" ]]; then
    echo "❌ File not found: $FIRMWARE"
    exit 1
fi

# ---- Find a Python interpreter that has ecdsa ----
PYTHON=""
for p in "python3" "python" "py"; do
    if command -v "$p" &>/dev/null && "$p" -c "import ecdsa" 2>/dev/null; then
        PYTHON="$p"; break
    fi
done

if [[ -z "$PYTHON" ]]; then
    echo "❌ No Python interpreter with the ecdsa package found. Please run: pip install ecdsa"
    exit 1
fi

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
cd "$SCRIPT_DIR"

# ---- Key check ----
if [[ ! -f "private_key.pem" ]]; then
    echo "🔑 First run: generating key pair..."
    "$PYTHON" generate_keys.py
    echo ""
    echo "⚠️  Remember to copy ecdsa_pubkey.h to boot/core/"
    echo "    and recompile the bootloader (only needed once; you can skip it afterwards)"
    echo ""
fi

# ---- Sign ----
echo "📝 Signing: $FIRMWARE (v$VERSION, addr=$ADDR)"
"$PYTHON" sign_firmware.py "$FIRMWARE" --key private_key.pem --version "$VERSION" --addr "$ADDR"

SIGNED="${FIRMWARE%.bin}_signed.bin"
echo ""
echo "✅ Done! Signed file: $SIGNED"
echo "   Pick this file in the host tool -> Upload (no key required)"
