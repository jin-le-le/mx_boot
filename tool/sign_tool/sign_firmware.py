#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
sign_firmware.py - ECDSA firmware signing tool

Usage:
  python sign_firmware.py app.bin
  python sign_firmware.py app.bin --key private_key.pem --version 2.1.0
  python sign_firmware.py app.bin --out signed.bin
"""

import os
import sys
import struct
import hashlib
import binascii
import argparse

# Windows console UTF-8 support
if sys.platform == 'win32':
    try:
        sys.stdout.reconfigure(encoding='utf-8')
        sys.stderr.reconfigure(encoding='utf-8')
    except Exception:
        pass

try:
    from ecdsa import SigningKey, NIST256p
    from ecdsa.util import sigencode_string
except ImportError:
    print("❌ Missing the ecdsa library. Install it with: pip install ecdsa")
    sys.exit(1)


# ============================================================
# Constants (must match the MCU-side image.h exactly)
# ============================================================
IMAGE_MAGIC        = 0x4D414749   # "MAGI"
IMAGE_FLAG_SIGNED  = 0x01         # BIT0 = signed
HEADER_SIZE        = 256          # Header is fixed at 256 bytes
HEADER_CRC_OFFSET  = 248          # header_crc32 sits at offset 248


def compute_crc32(data):
    """Compute CRC32 (IEEE 802.3, identical to the MCU side)"""
    return binascii.crc32(data) & 0xFFFFFFFF


def build_header(fw_data, signature, version_major, version_minor, version_build, image_addr):
    """Build the 256-byte header (matches the MCU-side bl_image_header_t exactly)

    Layout (packed, little-endian):
      offset 0:    magic          (uint32)
      offset 4:    header_version (uint32) = 2
      offset 8:    flags          (uint32) = SIGNED
      offset 12:   image_type     (uint32) = 0
      offset 16:   image_addr     (uint32) = 0x08010000
      offset 20:   image_size     (uint32)
      offset 24:   image_crc32    (uint32)
      offset 28:   version_major  (uint32)
      offset 32:   version_minor  (uint32)
      offset 36:   version_build  (uint32)
      offset 40:   signature      (64 bytes)
      offset 104:  min_hw_version (uint32) = 0
      offset 108:  reserved       (140 bytes = zeros)
      offset 248:  header_crc32   (uint32) = CRC32(bytes[0:248])
    """
    # Compute firmware CRC32
    fw_crc = compute_crc32(fw_data)

    # Pack the header (without the trailing 4-byte header_crc32)
    header = bytearray(HEADER_SIZE)

    struct.pack_into('<I', header, 0,   IMAGE_MAGIC)
    struct.pack_into('<I', header, 4,   2)                       # header_version
    struct.pack_into('<I', header, 8,   IMAGE_FLAG_SIGNED)       # flags
    struct.pack_into('<I', header, 12,  0)                       # image_type
    struct.pack_into('<I', header, 16,  image_addr)               # image_addr
    struct.pack_into('<I', header, 20,  len(fw_data))            # image_size
    struct.pack_into('<I', header, 24,  fw_crc)                  # image_crc32
    struct.pack_into('<I', header, 28,  version_major)
    struct.pack_into('<I', header, 32,  version_minor)
    struct.pack_into('<I', header, 36,  version_build)

    # signature (64 bytes, offset 40)
    assert len(signature) == 64, f"Signature must be 64 bytes, got {len(signature)}"
    header[40:104] = signature

    # min_hw_version = 0 (offset 104)
    # reserved = 0 (offset 108~247, already zeroed)

    # Compute the header's own CRC32 (covers bytes 0~247)
    hdr_crc = compute_crc32(bytes(header[0:HEADER_CRC_OFFSET]))
    struct.pack_into('<I', header, HEADER_CRC_OFFSET, hdr_crc)

    return bytes(header)


def sign_firmware(fw_path, key_path, version_str, out_path, image_addr):
    """Sign the firmware and emit the combined file"""

    # ===== 1. Read firmware =====
    with open(fw_path, "rb") as f:
        fw_data = f.read()
    print(f"Firmware: {fw_path}")
    print(f"Size: {len(fw_data)} bytes ({len(fw_data)/1024:.1f} KB)")

    # ===== 2. Compute hashes =====
    fw_hash = hashlib.sha256(fw_data).digest()
    fw_crc = compute_crc32(fw_data)
    print(f"CRC32: 0x{fw_crc:08X}")
    print(f"SHA256: {fw_hash.hex()}")

    # ===== 3. ECDSA signature =====
    with open(key_path, encoding='utf-8') as f:
        sk = SigningKey.from_pem(f.read())

    # Sign the firmware hash (not the firmware itself).
    # sigencode_string outputs the raw r||s format (64 bytes), compatible with micro-ecc.
    signature = sk.sign_digest_deterministic(fw_hash, sigencode=sigencode_string)
    assert len(signature) == 64, f"Unexpected signature length: {len(signature)}"
    print(f"ECDSA signature: {signature.hex()}")

    # ===== 4. Parse version string =====
    parts = version_str.split(".")
    major = int(parts[0]) if len(parts) > 0 else 1
    minor = int(parts[1]) if len(parts) > 1 else 0
    build = int(parts[2]) if len(parts) > 2 else 0
    print(f"Version: v{major}.{minor}.{build}")

    # ===== 5. Build the header =====
    header = build_header(fw_data, signature, major, minor, build, image_addr)

    # Verify header CRC
    hdr_crc_verify = compute_crc32(header[0:HEADER_CRC_OFFSET])
    hdr_crc_stored = struct.unpack_from('<I', header, HEADER_CRC_OFFSET)[0]
    assert hdr_crc_verify == hdr_crc_stored, "Header CRC self-check failed"
    print(f"Header CRC: 0x{hdr_crc_stored:08X} ✓")

    # ===== 6. Emit the combined file =====
    combined = header + fw_data
    with open(out_path, "wb") as f:
        f.write(combined)

    print(f"\n✅ Signing complete!")
    print(f"   Output: {out_path}")
    print(f"   Total size: {len(combined)} bytes ({len(combined)/1024:.1f} KB)")
    print(f"     ├─ Header: {HEADER_SIZE} bytes")
    print(f"     └─ App:    {len(fw_data)} bytes")
    print(f"\n   The host tool can flash this file directly (it auto-detects the MAGI header)")


if __name__ == "__main__":
    parser = argparse.ArgumentParser(
        description="ECDSA P-256 firmware signing tool",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog="""
Examples:
  python sign_firmware.py app.bin
  python sign_firmware.py app.bin --key private_key.pem --version 2.1.0
  python sign_firmware.py app.bin --out release/signed.bin
        """)
    parser.add_argument("firmware", help="Firmware file to sign (.bin)")
    parser.add_argument("--key", default="private_key.pem",
                        help="Private key file path (default: private_key.pem)")
    parser.add_argument("--version", default="1.0.0",
                        help="Firmware version (format: major.minor.build, default: 1.0.0)")
    parser.add_argument("--out", default=None,
                        help="Output file path (default: <firmware>_signed.bin)")
    parser.add_argument("--addr", default="0x08010000",
                        help="App flash address (default: 0x08010000)\n"
                             "F407: 0x08010000, F103: 0x08006400")

    args = parser.parse_args()

    if not os.path.exists(args.firmware):
        print(f"❌ Firmware file not found: {args.firmware}")
        sys.exit(1)
    if not os.path.exists(args.key):
        print(f"❌ Private key file not found: {args.key}")
        print(f"   Please run first: python generate_keys.py")
        sys.exit(1)

    out_path = args.out or args.firmware.replace(".bin", "_signed.bin")
    image_addr = int(args.addr, 16) if args.addr.startswith("0x") else int(args.addr)
    sign_firmware(args.firmware, args.key, args.version, out_path, image_addr)
