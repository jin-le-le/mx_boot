# MCU_BOOT — 跨平台 ARM Cortex-M IAP Bootloader 框架

> 📖 **English**: [README.en.md](README.en.md) (1-minute overview)

[![Version](https://img.shields.io/badge/version-v2.0.2-blue.svg)]()
[![License](https://img.shields.io/badge/license-Apache--2.0-green.svg)](LICENSE)
[![Platform](https://img.shields.io/badge/platform-STM32F4%20%7C%20STM32F1%20%7C%20HC32F4-yellow.svg)]()
[![Signature](https://img.shields.io/badge/security-ECDSA%20P--256-red.svg)](.github/SECURITY.md)
[![CI](https://github.com/jinle/mcu_boot/actions/workflows/ci.yml/badge.svg)](https://github.com/jinle/mcu_boot/actions/workflows/ci.yml)

> 一套 **boot 核心 + port 抽象** 分层架构的 IAP Bootloader，目前已在 **STM32F407ZGT6**、**STM32F103C8T6**、**HC32F460PETB** 三款芯片上验证完整升级流程（擦除 / 编程 / CRC 校验 / ECDSA 验签 / 防回滚 / 跳转）。
>
> 移植到新芯片只需实现 7 个 port 接口文件，**boot 核心 0 改动**。

---

## 文档索引

### 入口文档

| 文档 | 用途 |
|------|------|
| **README.md**（本文） | 项目总览 + 快速上手 + 多芯片支持矩阵 |
| [docs/PORTING_GUIDE.md](docs/PORTING_GUIDE.md) | 移植到新芯片的完整步骤（10 步 + 测试清单 + 陷阱 + 量产场景） |
| [docs/PROTOCOL_SPEC.md](docs/PROTOCOL_SPEC.md) | 通信协议规范（帧格式 / opcode / device_info_t / 状态机） |
| [docs/IMAGE_FORMAT.md](docs/IMAGE_FORMAT.md) | 固件 Image Header 256 字节格式详解 |
| [docs/ECDSA_DESIGN.md](docs/ECDSA_DESIGN.md) | HMAC → ECDSA 升级的设计决策记录 |
| [docs/architecture_diagrams.md](docs/architecture_diagrams.md) | 4 张架构图 Mermaid 源码（系统架构 / 签名流程 / 升级时序 / 体积对比） |

### 治理文档

| 文档 | 用途 |
|------|------|
| [LICENSE](LICENSE) | Apache License 2.0（含 micro-ecc / EasyLogger 等第三方组件许可声明） |
| [.github/SECURITY.md](.github/SECURITY.md) | 安全漏洞披露流程 + 威胁模型 + 已知安全注意事项 |
| [.github/CONTRIBUTING.md](.github/CONTRIBUTING.md) | 贡献指南（分层原则 / commit 规范 / PR 自检清单） |
| [.github/CODE_OF_CONDUCT.md](.github/CODE_OF_CONDUCT.md) | 行为准则 |

---

## 核心特性

| 维度 | 说明 |
|------|------|
| **架构** | 4 层：App 配置 → Boot 核心 → Port 抽象 → HAL 驱动 |
| **共享代码** | `boot/core/` + `boot/lib/` 100% 芯片无关 |
| **签名** | ECDSA P-256（micro-ecc），64B 公钥编译进 bootloader，64B 签名嵌入固件 header |
| **完整性** | CRC32-IEEE，三档可选实现（按位 / 半字节表 / 全表） |
| **协议** | UART 115200 8-N-1，自定义帧（0xAA 帧头 + opcode + length + payload + CRC16） |
| **动态地址** | 上位机从 `device_info_t` 协议字段读取 `app_base` / `header_addr` / `mtu`，Flash 布局变了上位机零改动 |
| **日志** | EasyLogger 集成，`BL_LOG_ENABLED=0` 时所有 log_xxx 宏变成空操作，0 体积 |
| **集成** | `boot_init(); boot_run();` 两行代码集成到任何 CubeMX 工程 |

---

## 支持芯片矩阵

| 芯片 | Flash | RAM | Bootloader 配额 | App 起始 | Flash 起始 | 驱动库 | 状态 |
|------|-------|-----|------------------|----------|------------|--------|------|
| STM32F407ZGT6 | 1MB | 192KB | 48KB | 0x08010000 | 0x08000000 | ST HAL | ✅ 完整流程通过 |
| STM32F103C8T6 | 64KB | 20KB | 24KB | 0x08006400 | 0x08000000 | ST HAL | ✅ 完整流程通过 |
| HC32F460PETB | 512KB | 188KB | 48KB | 0x0000E000 | 0x00000000 | HC32 DDL | ✅ 完整流程通过 |

> HC32F460 的 Flash 起始地址是 `0x00000000`（与 STM32 的 `0x08000000` 不同），boot 核心零硬编码设计直接适配。

### 移植成本参考（F103 从 0 到跑通）

| 项目 | 工作量 |
|------|--------|
| 新建 `projects/stm32f103/`（CubeMX 配置 + 7 个 port 文件） | ~600 行代码 |
| 改 `board_config.h`（地址/UART/Timer/GPIO 实例） | ~30 行 |
| 改 `LinkerScript`（Flash 大小 + bootloader 配额 + ASSERT） | ~10 行 |
| 调试 silent write 失败（F103 CR mode bits 不自清） | ~8 小时 |
| 修改 `boot/core/` 和 `boot/lib/` | **0 行** |

---

## Flash 布局

### STM32F407ZG（1MB Flash）

```
0x08000000 ┌─────────────────────┐
           │     Bootloader      │  48KB（Sector 0-2）
0x0800C000 ├─────────────────────┤  ← Header 地址
           │    Image Header     │  16KB（Sector 3，实际 256B 用）
0x08010000 ├─────────────────────┤  ← App 向量表地址
           │    Application      │  ~940KB（Sector 4-11）
0x080FFFFF └─────────────────────┘
```

### STM32F103C8（64KB Flash）

```
0x08000000 ┌─────────────────────┐
           │     Bootloader      │  24KB（Page 0-23）
0x08006000 ├─────────────────────┤  ← Header 地址
           │    Image Header     │  1KB（Page 24，实际 256B 用）
0x08006400 ├─────────────────────┤  ← App 向量表地址
           │    Application      │  ~40KB（Page 25-63）
0x0800FFFF └─────────────────────┘
```

> ⚠️ **bootloader 配额必须严格限制**。链接脚本末尾的 `ASSERT(_boot_flash_used <= BOOTLOADER_SIZE, ...)` 会在编译期阻止 bootloader 自溢出到 app 区——否则升级擦除时会把 bootloader 自己的代码擦掉。

---

## 项目结构

```
MCU_BOOT_2.0.2/
├── boot/                          ★ 芯片无关核心（移植 0 改动）
│   ├── core/                      业务核心
│   │   ├── bl_main.c/.h           启动决策 / trap / 跳转 / boot_init / boot_run
│   │   ├── image.c/.h             镜像校验（magic / CRC32 / ECDSA / 防回滚）
│   │   ├── protocol.c/.h          通信协议状态机
│   │   └── ecdsa_pubkey.h         ECDSA P-256 公钥（generate_keys.py 生成）
│   ├── lib/                       内置库
│   │   ├── crc/                   CRC16-Modbus + CRC32-IEEE（三模式可切换）
│   │   ├── sha256/                SHA-256（ECDSA 验证前算固件哈希）
│   │   ├── ecdsa/                 micro-ecc P-256 验证（只编译 verify）
│   │   ├── ringbuffer/            环形缓冲区
│   │   └── elog/                  EasyLogger 日志库
│   └── hal_if_defines.h           HAL 接口定义（platform_desc_t 函数指针表）
│
├── projects/                      ★ 多芯片工程（每个芯片一份独立配置 + port 实现）
│   ├── stm32f407/
│   │   ├── App/                   board_config.h + bl_features.h
│   │   ├── Core/                  CubeMX 生成
│   │   ├── Drivers/               ST HAL + CMSIS
│   │   ├── LinkerScript/          STM32F407ZG_Bootloader.ld
│   │   ├── port/                  stm32f4_port.c + 6 个 port_xxx.c
│   │   └── CMakeLists.txt
│   ├── stm32f103/
│   │   ├── App/                   board_config.h + bl_features.h（仅 RAM 大小不同）
│   │   ├── Core/                  CubeMX 生成
│   │   ├── Drivers/               ST HAL + CMSIS
│   │   ├── LinkerScript/          STM32F103C8_Bootloader.ld
│   │   ├── port/                  stm32f1_port.c + 6 个 port_xxx.c
│   │   └── CMakeLists.txt
│   └── hc32f460/
│       ├── boot/                  HC32 bootloader（含 Drivers/DDL + port/）
│       └── app_printf/            HC32 demo app
│
├── tool/                          工具链
│   ├── sign_tool/                 Python 离线签名工具（sign.sh + sign_firmware.py + generate_keys.py）
│   └── mcu_boot_tool/             C# WinForms 上位机
│
├── examples/                      示例签名固件（可直接烧录测试）
├── docs/                          技术文档（PORTING_GUIDE / PROTOCOL_SPEC / IMAGE_FORMAT / ...）
├── .github/                       CI workflow + Issue/PR 模板 + SECURITY / CONTRIBUTING / CODE_OF_CONDUCT
├── Makefile                       顶层一键构建入口（make all / f407 / f103 / hc32）
├── README.md                      本文档
├── README.en.md                   英文版（一分钟概览）
└── LICENSE                        Apache License 2.0
```

---

## 快速开始

### 0. 一键编译三款芯片（推荐）

仓库根目录的 `Makefile` 包装了三个工程的构建：

```bash
make all        # 编译 F407 + F103 + HC32F460 bootloader
make f407       # 单独编译 F407
make f103       # 单独编译 F103
make hc32       # 单独编译 HC32F460
make sign-smoke # Python 签名工具冒烟测试
make clean      # 清理所有 build/ 目录
```

### 1. 手动构建 bootloader（以 F103 为例）

```bash
cd projects/stm32f103
cmake -B build -G Ninja -DCMAKE_TOOLCHAIN_FILE=cmake/gcc-arm-none-eabi.cmake
cmake --build build
```

产物在 `build/MCU_BOOT.elf` / `.bin` / `.hex` / `.dis` / `.map`。

F407 工程在 `projects/stm32f407/`，HC32 在 `projects/hc32f460/boot/`（注意 HC32 的 toolchain 文件名是 `gcc-hc32f460.cmake`）。

### 2. 烧录 bootloader

用 STM32CubeProgrammer / J-Flash / OpenOCD 烧录 `MCU_BOOT.hex`，或：

```bash
stm32cubeProgrammer -c port=SWD -w build/MCU_BOOT.bin 0x08000000
```

### 3. 生成 ECDSA 密钥（首次，做一次）

```bash
cd tool/sign_tool
python generate_keys.py
# → 生成 private_key.pem（保密！已 gitignore）
# → 生成 ecdsa_pubkey.h（覆盖 boot/core/ecdsa_pubkey.h 的占位符）
```

⚠️ **正式部署前必须替换 `boot/core/ecdsa_pubkey.h` 的全 0 占位符，否则你的固件任何人都能伪造。详见 [.github/SECURITY.md](.github/SECURITY.md)。**

### 4. 编译并签名 app 固件

```bash
./sign.sh app.bin 2.0.0 0x08006400    # F103
./sign.sh app.bin 2.0.0 0x08010000    # F407
./sign.sh app.bin 2.0.0 0x0000E000    # HC32F460
# → 输出 app_signed.bin（256B header + app.bin，header 含 ECDSA 签名 + CRC32 + 版本号）
```

### 5. 上位机升级

```bash
cd tool/mcu_boot_tool
dotnet build -c Release
./bin/Release/mcu_boot_tool.exe
```

GUI 操作：选串口 → 选 `app_signed.bin` → Upload。上位机自动从设备读 `app_base`，无需手动配置。

---

## 配置说明

### bl_features.h（业务相关，所有芯片共用，移植不改）

```c
#define BL_SIGNATURE_TYPE       1    /* 0=无签名, 1=ECDSA P-256 */
#define BL_REJECT_UNSIGNED      1    /* 1=量产(拒绝未签名), 0=开发(允许) */
#define BL_ANTIROLLBACK_ENABLED 1
#define BL_MIN_APP_VERSION      1

#define CRC32_MODE              0    /* 0=按位(0表), 1=全表(1KB), 2=半字节表(64B) */
#define CRC16_MODE              0

#define BL_LOG_ENABLED          0    /* 0=量产(0体积), 1=开发(带日志) */
#define BL_LOG_COLOR            0
#define BL_LOG_LINE             1

#define BL_BOOT_DELAY_MS        500  /* trap 窗口 */
#define BL_ENABLE_RX_TRAP       1
#define BL_RX_TRAP_PATTERN      { 0xAA, 0x01, 0x01, 0x00, 0x00 }
```

### board_config.h（芯片相关，移植必改）

```c
/* F407 */
#define BL_FLASH_BASE           0x08000000U
#define BL_FLASH_TOTAL          (1024 * 1024)
#define BL_BOOTLOADER_SIZE      (48 * 1024)
#define BL_APP_BASE             0x08010000U
#define BL_MAGIC_HEADER_ADDR    0x0800C000U
#define BL_UART_INSTANCE        3              /* USART3 */
#define BL_TIMER_INSTANCE       6              /* TIM6 */

/* F103 */
#define BL_FLASH_BASE           0x08000000U
#define BL_FLASH_TOTAL          (64 * 1024)
#define BL_BOOTLOADER_SIZE      (24 * 1024)
#define BL_APP_BASE             0x08006400U
#define BL_MAGIC_HEADER_ADDR    0x08006000U
#define BL_UART_INSTANCE        3              /* USART3 */
#define BL_TIMER_INSTANCE       2              /* TIM2 */
```

---

## 通信协议

### 帧格式

```
请求: [0xAA] [opcode] [length_le16] [payload...] [crc16_le16]
响应: [0x55] [opcode] [errcode] [length_le16] [payload...] [crc16_le16]
```

### 操作码

| Opcode | 名称 | 说明 |
|--------|------|------|
| 0x00 | DEVICE_INFO | 查询设备信息（BL版本、App版本、MTU、地址、能力位） |
| 0x01 | GET_CAPS | 获取 bootloader 能力位 |
| 0x21 | RESET | 软复位 |
| 0x22 | BOOT | 立即跳转到 App |
| 0x81 | ERASE | 擦除指定区域 |
| 0x82 | PROGRAM | 写入数据（半字对齐） |
| 0x83 | VERIFY | CRC32 校验指定区域 |
| 0x84 | WRITE_HEADER | 写入镜像头（256B） |

### `device_info_t` 响应结构（24B，packed）

```
偏移  字段             说明
0     bl_major         Bootloader 版本
1     bl_minor
2     bl_build
3     has_app          是否已有有效 App
4     app_major        App 版本（has_app=1 时有效）
5     app_minor
6     app_build
7-8   mtu              单包最大 payload（含 8B 头）
9-12  caps             能力位（BIT_SIGNED / BIT_ENCRYPTED / ...）
13-16 header_addr      设备 Header 地址（上位机动态读，不写死）
17-20 app_base         设备 App 起始地址
21-23 reserved
```

---

## Image Header 格式（256B，packed）

```
偏移   字段                说明
0      magic              0x4D414749 "MAGI"
4      header_version     = 2
8      flags              BIT0=SIGNED, BIT1=ENCRYPTED（预留）
12     image_type         = 0 (main)
16     image_addr         App 起始地址（签名时指定）
20     image_size         App 字节数
24     image_crc32        App 的 CRC32（标准 IEEE）
28-36  version            major.minor.build
40     signature          ECDSA P-256 签名（64B = r||s）
104    min_hw_version     = 0
108    reserved           140B
248    header_crc32       Header bytes [0..247] 的 CRC32
```

---

## 安全特性

### 1. ECDSA P-256 非对称签名

- **签**：厂家私钥（离线保管）+ Python 工具（`sign_firmware.py`）
- **验**：bootloader 编译进公钥（`ecdsa_pubkey.h`），运行时用 micro-ecc 验签
- **优势**：客户拿到的签好固件 + 上位机，**完全不需要任何密钥**。MCU 被读出 flash 也无法伪造新固件。

### 2. CRC32 完整性校验

标准 IEEE CRC32（poly 0xEDB88320, init/xorout 0xFFFFFFFF），与 Python `binascii.crc32` 完全一致。三档实现（按位 / 半字节表 / 全表）输出相同，按 Flash 预算选择。

### 3. 版本号防回滚

`BL_ANTIROLLBACK_ENABLED=1` 时，header 的 `version` 必须大于等于已安装版本，防止攻击者降级到旧的有漏洞版本。

### 4. 拒绝未签名固件

`BL_REJECT_UNSIGNED=1` 时，header `flags` 必须包含 `SIGNED` 位，否则拒绝烧录。**量产前必开**，否则攻击者可以构造 `flags=0` 的固件绕过签名验证。

### 5. Flash 读写保护（可选）

`BL_FLASH_PROTECTION_ENABLED=1` 时，配置 RDP Level 1 防止调试器读出 flash。F407 默认开 Level 1，F103 暂未启用（资源紧张 + 调试期）。

---

## 集成方式（移植到新芯片）

main.c 加两行：

```c
/* USER CODE BEGIN 2 */
boot_init();    /* 初始化（elog + image + config + platform） */
boot_run();     /* 启动 bootloader（永不返回） */
/* USER CODE END 2 */
```

完整移植流程：

1. 用 CubeMX 配置芯片（UART / TIM / GPIO / Clock / 中断）
2. 新建 `projects/<chip>/`，复制 F407 或 F103 工程做模板
3. 实现 `port/` 下 7 个文件（flash / uart / gpio / timer / system / console_uart / platform 聚合）
4. 改 `board_config.h`（Flash 地址、UART/Timer 实例）
5. 改 `LinkerScript`（Flash 大小、bootloader 配额、加 ASSERT）
6. 用 `generate_keys.py` 生成密钥，把 `public_key.h` 拷到 `boot/core/ecdsa_pubkey.h`
7. 编译 + 烧录

---

## 构建工具链

| 工具 | 版本 |
|------|------|
| ARM GCC (`arm-none-eabi-gcc`) | 14.x 或更新 |
| CMake | 3.22+ |
| Ninja | 任意版本 |
| Python | 3.8+（签名工具） |
| .NET SDK | 6.0+（C# 上位机） |
| `ecdsa` Python 包 | `pip install ecdsa` |

GCC 工具链默认。Clang (`starm-clang`) 作为备选，配置在 `cmake/starm-clang.cmake`。

---

## 版本历史

| 版本 | 日期 | 说明 |
|------|------|------|
| v2.1.0 | 2026-08 | 三芯片支持（+HC32F460 + DDL 驱动库），GPIO_FUNC 引脚独立映射适配 |
| v2.0.2 | 2026-07 | 双芯片支持（F407 + F103）、ECDSA P-256、动态地址协议、链接器 ASSERT 防御 |
| v1.0.0 | 2026-07 | 单芯片（F407）、HMAC 签名（已废弃） |
| v0.1.0 | 2026-06 | 初始版本，基础 IAP |

---

## 许可证

[Apache License 2.0](LICENSE)。bundled 的第三方组件保留各自原始许可：

- micro-ecc（BSD-2-Clause）
- EasyLogger（MIT）
- STM32 HAL / CMSIS（BSD-3-Clause，ST 提供）
- HC32F460 DDL（HDSC 提供）

---

## 联系方式

- **Bug / 功能建议**：[GitHub Issues](https://github.com/jinle/mcu_boot/issues)
- **设计讨论 / 使用问答**：[GitHub Discussions](https://github.com/jinle/mcu_boot/discussions)
- **安全漏洞披露**：参见 [.github/SECURITY.md](.github/SECURITY.md)（请勿通过公开 Issue 报告安全问题）
- **贡献 PR**：先读 [.github/CONTRIBUTING.md](.github/CONTRIBUTING.md)
