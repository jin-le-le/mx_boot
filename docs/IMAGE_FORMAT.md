# MCU-Boot Image 镜像格式规范

**版本**: v1.1
**日期**: 2026-07-31（补 F103 布局说明）

> 本文档定义 bootloader 识别的"签名固件文件"格式（`*_signed.bin`）。
> 文件格式本身芯片无关；下文以 F407 为例，**F103 仅地址不同**（见第 2.2 节）。

---

## 目录

1. [概述](#1-概述)
2. [Flash布局](#2-flash布局)
3. [Image Header格式](#3-image-header格式)
4. [Header CRC计算](#4-header-crc计算)
5. [Image验证流程](#5-image验证流程)
6. [签名机制](#6-签名机制)

---

## 1. 概述

MCU-Boot 使用独立的 Header 区域存储固件元数据，Header 包含固件地址、大小、CRC32、版本号和签名等信息。

### 1.1 设计目标

| 目标 | 说明 |
|------|------|
| **完整性校验** | CRC32确保固件未被损坏 |
| **防篡改** | HMAC-SHA256签名防止恶意固件 |
| **版本控制** | 防回滚机制阻止降级攻击 |
| **最小开销** | 256字节固定大小 |

---

## 2. Flash布局

### 2.1 STM32F407ZG (1024KB)

```
0x08000000 ┌─────────────────────┐
           │     Bootloader      │     48KB
           │                     │     (Sector 0-2)
           │                     │
0x0800C000 ├─────────────────────┤  ← Header地址
           │    Image Header     │     256B
           │                     │
0x08010000 ├─────────────────────┤  ← App向量表地址
           │                     │
           │    Application      │     ~940KB
           │                     │     (Sector 3-11)
           │                     │
0x080FFFFF └─────────────────────┘  ← Flash结束地址
```

### 2.2 区域说明

| 区域 | 起始地址 | 大小 | 说明 |
|------|----------|------|------|
| Bootloader | 0x08000000 | 48KB | 启动引导程序 |
| Header | 0x0800C000 | 256B | 固件元数据 |
| Application | 0x08010000 | ~940KB | 应用程序 |

### 2.3 STM32F4 Flash扇区

| 扇区 | 起始地址 | 大小 | 说明 |
|------|----------|------|------|
| 0 | 0x08000000 | 16KB | Bootloader |
| 1 | 0x08004000 | 16KB | Bootloader |
| 2 | 0x08008000 | 16KB | Bootloader |
| 3 | 0x0800C000 | 16KB | Header |
| 4 | 0x08010000 | 64KB | App |
| 5-11 | - | 128KB×7 | App |

---

## 3. Image Header格式

Header 总大小 **256字节**：
- **F407**: 位于 Flash `0x0800C000`
- **F103**: 位于 Flash `0x08006000`

地址由设备的 `board_config.h::BL_MAGIC_HEADER_ADDR` 决定，上位机通过 `device_info_t.header_addr` 动态读取，不写死。

### 3.1 结构定义

```c
#pragma pack(push, 1)
typedef struct {
    uint32_t magic;              // Offset 0:   0x4D414749 ("MAGI")
    uint32_t header_version;    // Offset 4:   Header版本 (当前=2)
    uint32_t flags;             // Offset 8:   标志位
    uint32_t image_type;        // Offset 12:  镜像类型 (0=main)
    uint32_t image_addr;        // Offset 16:  固件地址 (0x08010000)
    uint32_t image_size;        // Offset 20:  固件大小 (bytes)
    uint32_t image_crc32;       // Offset 24:  固件CRC32
    uint32_t version_major;     // Offset 28:  主版本号
    uint32_t version_minor;     // Offset 32:  次版本号
    uint32_t version_build;     // Offset 36:  编译版本号
    uint8_t  signature[64];     // Offset 40:  HMAC-SHA256签名
    uint32_t min_hw_version;    // Offset 104: 最低硬件版本
    uint8_t  reserved[140];     // Offset 108: 保留 (用于扩展)
    uint32_t header_crc32;      // Offset 248: Header自身CRC32
} bl_image_header_t;
#pragma pack(pop)
```

### 3.2 字段详解

| 字段 | 偏移 | 大小 | 类型 | 说明 |
|------|------|------|------|------|
| magic | 0 | 4 | uint32_t | 固定值 `0x4D414749` ("MAGI") |
| header_version | 4 | 4 | uint32_t | Header结构版本，当前为2 |
| flags | 8 | 4 | uint32_t | 标志位，见下方 |
| image_type | 12 | 4 | uint32_t | 镜像类型，0=主应用 |
| image_addr | 16 | 4 | uint32_t | 固件烧录地址 |
| image_size | 20 | 4 | uint32_t | 固件大小(字节) |
| image_crc32 | 24 | 4 | uint32_t | 固件数据的CRC32 |
| version_major | 28 | 4 | uint32_t | 主版本号 |
| version_minor | 32 | 4 | uint32_t | 次版本号 |
| version_build | 36 | 4 | uint32_t | 编译版本号 |
| signature | 40 | 64 | uint8_t[] | HMAC-SHA256签名 |
| min_hw_version | 104 | 4 | uint32_t | 最低硬件版本要求 |
| reserved | 108 | 140 | uint8_t[] | 保留扩展区域 |
| header_crc32 | 248 | 4 | uint32_t | Header自身的CRC32 |

### 3.3 Flags定义

```c
#define BL_IMAGE_FLAG_SIGNED     (1 << 0)   // 固件已签名
#define BL_IMAGE_FLAG_ENCRYPTED  (1 << 1)   // 固件已加密 (预留)
```

### 3.4 字节序

所有多字节字段使用 **小端序 (Little-Endian)** 存储。

---

## 4. Header CRC计算

### 4.1 计算范围

Header CRC32 覆盖 **bytes 0-247**（共248字节），**不包含** CRC字段本身（bytes 248-251）。

### 4.2 计算公式

```
header_crc32 = CRC32(bytes[0:248])
```

### 4.3 CRC32算法

使用 **CRC32-IEEE** 多项式：

- 多项式: `0x04C11DB7`
- 初值: `0xFFFFFFFF`
- 结果异或: `0xFFFFFFFF`

### 4.4 验证流程

```
1. 读取Header bytes[0:248]
2. 计算CRC32得 computed_crc
3. 读取Header bytes[248:252] 得 stored_crc
4. 对比 computed_crc == stored_crc
```

---

## 5. Image验证流程

### 5.1 验证步骤

Bootloader启动时按以下顺序验证Image：

```
┌─────────────────────────────────────────┐
│  1. Magic检查                          │
│     hdr->magic == 0x4D414749?          │
└─────────────────┬───────────────────────┘
                  │ No
                  ▼
          ┌───────────────┐
          │   返回错误    │
          └───────────────┘
                  │ Yes
                  ▼
┌─────────────────────────────────────────┐
│  2. Header CRC检查                       │
│     CRC32(bytes[0:248]) == hdr->crc?   │
└─────────────────┬───────────────────────┘
                  │ No
                  ▼
          ┌───────────────┐
          │   返回错误    │
          └───────────────┘
                  │ Yes
                  ▼
┌─────────────────────────────────────────┐
│  3. 地址安全检查                         │
│     image_addr 在合法范围内?            │
│     image_size 合理?                    │
└─────────────────┬───────────────────────┘
                  │ No
                  ▼
          ┌───────────────┐
          │   返回错误    │
          └───────────────┘
                  │ Yes
                  ▼
┌─────────────────────────────────────────┐
│  4. Image CRC检查                        │
│     CRC32(image_addr, image_size) ==     │
│         hdr->image_crc32?               │
└─────────────────┬───────────────────────┘
                  │ No
                  ▼
          ┌───────────────┐
          │   返回错误    │
          └───────────────┘
                  │ Yes
                  ▼
┌─────────────────────────────────────────┐
│  5. 版本检查 (如果启用)                  │
│     version >= BL_MIN_APP_VERSION?      │
└─────────────────┬───────────────────────┘
                  │ No
                  ▼
          ┌───────────────┐
          │   返回错误    │
          └───────────────┘
                  │ Yes
                  ▼
┌─────────────────────────────────────────┐
│  6. 签名验证 (如果flags & SIGNED)        │
│     HMAC-SHA256(image) == signature?   │
└─────────────────┬───────────────────────┘
                  │ No
                  ▼
          ┌───────────────┐
          │   返回错误    │
          └───────────────┘
                  │ Yes
                  ▼
         ┌───────────────┐
         │   验证通过    │
         │  跳转App     │
         └───────────────┘
```

### 5.2 错误码

| 错误码 | 名称 | 说明 |
|--------|------|------|
| BL_IMAGE_OK | 成功 | 验证通过 |
| BL_IMAGE_ERR_MAGIC | Magic错误 | Magic值不匹配 |
| BL_IMAGE_ERR_HEADER_CRC | Header CRC错误 | Header损坏 |
| BL_IMAGE_ERR_ADDR | 地址错误 | 地址超出范围 |
| BL_IMAGE_ERR_IMAGE_CRC | Image CRC错误 | 固件损坏 |
| BL_IMAGE_ERR_VERSION | 版本错误 | 版本低于最低要求 |
| BL_IMAGE_ERR_SIGNATURE | 签名错误 | 签名验证失败 |

---

## 6. 签名机制

### 6.1 HMAC-SHA256签名

当 `flags & BL_IMAGE_FLAG_SIGNED` 时，固件必须包含有效签名。

### 6.2 签名计算

```
signature = HMAC-SHA256(key, image_data)
```

- **Key**: 预共享密钥 (在board_config.h中定义)
- **image_data**: 从 image_addr 开始的 image_size 字节

### 6.3 签名存储

计算得到的32字节HMAC-SHA256结果存储在 Header 的 `signature[64]` 字段的前32字节，后32字节填0。

### 6.4 签名验证流程

```
1. 检查 flags & BL_IMAGE_FLAG_SIGNED
2. 使用相同key计算 HMAC-SHA256(image_data)
3. 对比计算结果与 header.signature[0:32]
4. 一致则验证通过
```

---

## 附录

### A. Python示例代码

```python
import struct

def build_header(firmware_data, version, flags=0, signature=None):
    """构建Image Header"""
    header = bytearray(256)

    # Magic
    struct.pack_into('<I', header, 0, 0x4D414749)

    # Header version
    struct.pack_into('<I', header, 4, 2)

    # Flags
    struct.pack_into('<I', header, 8, flags)

    # Image type
    struct.pack_into('<I', header, 12, 0)

    # Image address
    struct.pack_into('<I', header, 16, 0x08010000)

    # Image size
    struct.pack_into('<I', header, 20, len(firmware_data))

    # Image CRC32
    struct.pack_into('<I', header, 24, crc32(firmware_data))

    # Version
    struct.pack_into('<III', header, 28, *version)

    # Signature (if provided)
    if signature:
        header[40:40+len(signature)] = signature

    # Header CRC (覆盖 bytes 0-247)
    header_crc = crc32(bytes(header[:248]))
    struct.pack_into('<I', header, 248, header_crc)

    return bytes(header)
```

---

*文档版本: v1.0*
*最后更新: 2026-07-11*
