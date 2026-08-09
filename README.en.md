# MCU_BOOT — Portable ARM Cortex-M IAP Bootloader

[![Version](https://img.shields.io/badge/version-v2.1.0-blue.svg)]()
[![License](https://img.shields.io/badge/license-Apache--2.0-green.svg)](LICENSE)
[![Platform](https://img.shields.io/badge/platform-STM32F4%20%7C%20STM32F1%20%7C%20HC32F4-yellow.svg)]()
[![Signature](https://img.shields.io/badge/security-ECDSA%20P--256-red.svg)](.github/SECURITY.md)
[![CI](https://github.com/jinle/mcu_boot/actions/workflows/ci.yml/badge.svg)](https://github.com/jinle/mcu_boot/actions/workflows/ci.yml)

> A layered IAP bootloader framework with **chip-independent core + chip-specific port layer**.
> Adding a new chip = implement 7 port files, **boot core stays untouched**.

📖 **中文版请看 [README.md](README.md)** | 中文文档更详细。

---

## What is this

A small, security-focused IAP (In-Application Programming) bootloader for ARM Cortex-M microcontrollers. It sits between power-on and your application, provides UART-based firmware upgrade, and verifies every image with **ECDSA P-256 signatures** before jumping to it.

Design goals: easy to port across chips, safe to ship to customers, friendly to commercial use (Apache-2.0, no copyleft).

## Highlights

| Dimension | Detail |
|---|---|
| **Architecture** | 4 layers: App config → Boot core → Port abstraction → HAL driver |
| **Chip-independent code** | `boot/core/` + `boot/lib/` — 100% portable, ~3500 LOC |
| **Signature** | ECDSA P-256 (micro-ecc, ~3.5 KB); private key stays offline |
| **Integrity** | CRC32-IEEE, three switchable implementations (bitwise / nibble-table / full-table) |
| **Anti-rollback** | Header `version` field must be `>= BL_MIN_APP_VERSION` |
| **Protocol** | UART 115200 8-N-1, custom framing: `0xAA | opcode | len | payload | CRC16` |
| **Dynamic addresses** | Host tool reads `app_base / header_addr / mtu / caps` from `device_info_t` — no per-chip configuration needed |
| **Logging** | EasyLogger integration; flips to **0-byte overhead** in production |
| **Integration** | Two lines in `main.c`: `boot_init(); boot_run();` |

## Supported chips

| Chip | Flash | RAM | Bootloader quota | App start | Driver lib | Status |
|---|---|---|---|---|---|---|
| STM32F407ZGT6 | 1 MB | 192 KB | 48 KB | 0x08010000 | ST HAL | ✅ Full flow verified |
| STM32F103C8T6 | 64 KB | 20 KB | 24 KB | 0x08006400 | ST HAL | ✅ Full flow verified |
| HC32F460PETB | 512 KB | 188 KB | 48 KB | 0x0000E000 | HC32 DDL | ✅ Full flow verified |

> HC32 maps Flash at `0x00000000` (unlike STM32's `0x08000000`). The zero-hardcoded-address core adapts automatically.

## Flash layout (F407 example)

```
0x08000000 ┌─────────────────────┐
           │     Bootloader      │  48 KB (Sector 0-2)
0x0800C000 ├─────────────────────┤  ← Header address
           │    Image Header     │  16 KB sector (256 B used)
0x08010000 ├─────────────────────┤  ← App vector table
           │    Application      │  ~940 KB
0x080FFFFF └─────────────────────┘
```

The linker script enforces `ASSERT(_boot_flash_used <= BOOTLOADER_SIZE)` at link time — a bootloader that would spill into the app region (and self-erase during upgrade) cannot even build.

## Quick start

```bash
# 1. Build all three bootloaders
make all

# 2. Generate your ECDSA keypair (one-time)
cd tool/sign_tool
python generate_keys.py
# → private_key.pem (keep secret, gitignored)
# → ecdsa_pubkey.h  (overwrite boot/core/ecdsa_pubkey.h)

# 3. Sign your app firmware
./sign.sh app.bin 2.1.0 0x08010000    # F407
./sign.sh app.bin 2.1.0 0x08006400    # F103
./sign.sh app.bin 2.1.0 0x0000E000    # HC32F460

# 4. Flash bootloader via SWD (CubeProgrammer / J-Flash / OpenOCD)
# 5. Use the C# host tool to upload app_signed.bin via UART
```

## Toolchain

| Tool | Version |
|---|---|
| `arm-none-eabi-gcc` | 14.x or newer |
| CMake | 3.22+ |
| Ninja | any |
| Python | 3.8+ (signing tool, `pip install ecdsa`) |
| .NET SDK | 6.0+ (C# host tool, optional) |

## Threat model

| Threat | Defense |
|---|---|
| Attacker crafts unsigned firmware | `BL_REJECT_UNSIGNED=1` + header `flags & SIGNED` check |
| Attacker forges signature | ECDSA P-256 verify (micro-ecc); pubkey compiled in |
| Attacker downgrades to vulnerable version | `BL_ANTIROLLBACK_ENABLED=1` + `BL_MIN_APP_VERSION` |
| Bootloader erases itself | `bl_image_is_addr_safe` + linker-time `ASSERT` |
| Flash dump → firmware forgery | Private key never leaves the offline signing machine |

See [.github/SECURITY.md](.github/SECURITY.md) for the full threat model and vulnerability disclosure process.

## Documentation

The detailed docs are written in Chinese. The structure (file paths and code are English, so you can still navigate):

| Doc | Content |
|---|---|
| [README.md](README.md) | Full project overview (中文) |
| [docs/PORTING_GUIDE.md](docs/PORTING_GUIDE.md) | 10-step porting guide for new chips |
| [docs/PROTOCOL_SPEC.md](docs/PROTOCOL_SPEC.md) | UART protocol spec (frame format, opcodes, state machine) |
| [docs/IMAGE_FORMAT.md](docs/IMAGE_FORMAT.md) | 256-byte image header layout |
| [docs/ECDSA_DESIGN.md](docs/ECDSA_DESIGN.md) | Why ECDSA P-256 over HMAC / RSA |
| [docs/architecture_diagrams.md](docs/architecture_diagrams.md) | Mermaid source for 4 architecture diagrams |
| [.github/SECURITY.md](.github/SECURITY.md) | Threat model + vulnerability disclosure |
| [.github/CONTRIBUTING.md](.github/CONTRIBUTING.md) | Contribution guide |

## License

[Apache License 2.0](LICENSE). Bundled third-party components retain their original licenses:

- micro-ecc — BSD-2-Clause
- EasyLogger — MIT
- STM32 HAL / CMSIS — BSD-3-Clause (ST)
- HC32F460 DDL — HDSC

## Contact

- **Bugs / features**: [GitHub Issues](https://github.com/jinle/mcu_boot/issues)
- **Discussions**: [GitHub Discussions](https://github.com/jinle/mcu_boot/discussions)
- **Security reports**: see [.github/SECURITY.md](.github/SECURITY.md) (do **not** open public issues for security reports)
- **Contributing**: see [.github/CONTRIBUTING.md](.github/CONTRIBUTING.md)
