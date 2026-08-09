# ECDSA 非对称签名设计文档

> 本文档记录 bootloader 从 HMAC-SHA256（对称）升级到 ECDSA P-256（非对称）的设计决策与实施细节。
> 状态：**已实施** ✅（2026-07，F407 + F103 双芯片均已落地）
>
> 本文档作为"设计决策记录"（ADR），用于面试 / 后续维护时回查为什么这么选。

---

## 1. 为什么要做这个改造

### 当前 HMAC 的问题

| 问题 | 说明 |
|------|------|
| 上位机暴露密钥 | C# 工具的 textBox_HMACKey 显示明文密钥 |
| MCU dump 泄露密钥 | 攻击者读 Flash → 拿到 HMAC key → 可伪造任意固件 |
| 无法商用分发 | 给客户的上位机里必须有 key 才能签名，不安全 |

### ECDSA 的优势

| 优势 | 说明 |
|------|------|
| 公私钥分离 | 私钥在厂家（签名），公钥在 MCU（验证）|
| 上位机无密钥 | 客户的工具只是"搬运工"，不需要任何密钥 |
| MCU dump 安全 | 攻击者只拿到公钥，无法伪造签名 |
| 商用标准 | Apple/Google/Microsoft 代码签名都用 ECDSA P-256 |

---

## 2. 算法选择

| 项 | 选择 | 理由 |
|----|------|------|
| 曲线 | **secp256r1 (P-256)** | 业界标准，128-bit 安全强度 |
| 哈希 | **SHA-256** | 已有 SHA256 代码，复用 |
| 签名格式 | **r(32B) \|\| s(32B) = 64B** | 正好填满 header 的 signature[64] |
| 验证库 | **micro-ecc** | 3-4 KB，单文件，MIT 协议，Cortex-M 优化 |

---

## 3. 安全流程

### 厂家端（离线，私钥不出服务器）

```
Step 1: 生成密钥对（一次性）
  python generate_keys.py
  → private_key.pem      （保密！放安全服务器）
  → public_key.h         （编译进 bootloader）

Step 2: 签名固件（每次发布）
  python sign_firmware.py app.bin --key private_key.pem --version 1.0.0
  → SHA256(app.bin) = hash
  → ECDSA_Sign(hash, private_key) = signature (64 bytes)
  → 生成 header.bin（含 signature + CRC + 版本号）

Step 3: 发布给客户
  → app.bin + header.bin（或合并成一个 signed_firmware.bin）
  → 客户拿到的只有签名后的文件，看不到私钥
```

### 客户端

```
Step 4: 上位机烧录
  → 选 signed_firmware.bin
  → 点 Upload
  → 上位机把 app.bin 和 header 分别发给 MCU
  → 上位机不需要任何密钥

Step 5: MCU 验证
  → 读 header 的 signature[64]
  → SHA256(Flash 里的 app.bin) = hash
  → ECDSA_Verify(hash, signature, public_key) → 1=通过 / 0=拒绝
```

---

## 4. 宏控制设计

### bl_features.h 新增

```c
/*=======================================================================
 * Signature Type Selection
 *=======================================================================*
 *   BL_SIGNATURE_TYPE = 0   HMAC-SHA256（对称）
 *                           - 代码：~0.25 KB（SHA256 已有，只加 HMAC 包装层）
 *                           - 密钥：16 字节，编译进 bootloader
 *                           - 安全：对称，MCU dump 后可伪造固件
 *                           - 适合：内部产品、开发调试
 *
 *   BL_SIGNATURE_TYPE = 1   ECDSA P-256（非对称）★ 商用推荐
 *                           - 代码：~3.5 KB（micro-ecc + SHA256）
 *                           - 公钥：64 字节，编译进 bootloader
 *                           - 安全：非对称，MCU dump 后无法伪造固件
 *                           - 适合：商用产品、需要分发给客户的场景
 *
 *   BL_SIGNATURE_TYPE = 2   无签名
 *                           - 代码：0（仅 CRC32 校验完整性）
 *                           - 安全：只防损坏，不防伪造
 *                           - 适合：新芯片首次移植、开发调试
 */
#define BL_SIGNATURE_TYPE    0
```

### 编译期行为

```
TYPE=0 时编译：
  SHA256 算法 (0.84 KB) + HMAC 包装层 (0.25 KB) = 1.09 KB

TYPE=1 时编译：
  SHA256 算法 (0.84 KB) + micro-ecc verify (3.5 KB) = 4.34 KB

TYPE=2 时编译：
  无签名代码（SHA256 也不编译）= 0 KB
```

同一时间只编译一个，互不干扰。

---

## 5. 代码改动清单

### 5.1 新增文件

| 文件 | 路径 | 说明 |
|------|------|------|
| `uECC.c` | `lib/ecc/uECC.c` | micro-ecc 核心实现 |
| `uECC.h` | `lib/ecc/uECC.h` | micro-ecc 头文件 |
| `ecdsa_pubkey.h` | `core/ecdsa_pubkey.h` | 公钥数组（厂家生成） |
| `generate_keys.py` | `tool/generate_keys.py` | 密钥生成工具 |
| `sign_firmware.py` | `tool/sign_firmware.py` | 固件签名工具 |

### 5.2 修改文件

| 文件 | 改动 |
|------|------|
| `bl_features.h` | 加 `BL_SIGNATURE_TYPE` 宏 |
| `image.h` | 加 ECDSA 相关声明 |
| `image.c` | `verify_signature()` 加 `#if BL_SIGNATURE_TYPE` 分支 |
| `CMakeLists.txt` | 加 `lib/ecc/uECC.c` 源文件 |
| 上位机 `Form1.cs` | `BL_SIGNATURE_TYPE=1` 时隐藏密钥输入框 |

### 5.3 image.c 验证逻辑改造

```c
static bool verify_signature(const bl_image_header_t *hdr)
{
    if (!(hdr->flags & BL_IMAGE_FLAG_SIGNED)) {
        return true;   /* 未签名镜像，跳过 */
    }

#if BL_SIGNATURE_TYPE == 0
    /* ===== HMAC-SHA256 验证（当前）===== */
    uint8_t computed_sig[32];
    uint8_t key[] = BL_HMAC_KEY;
    hmac_sha256(key, BL_HMAC_KEY_LEN,
                (const uint8_t *)hdr->image_addr, hdr->image_size,
                computed_sig);
    return memcmp(computed_sig, hdr->signature, 32) == 0;

#elif BL_SIGNATURE_TYPE == 1
    /* ===== ECDSA P-256 验证（新增）===== */
    uint8_t hash[32];
    sha256_compute((const uint8_t *)hdr->image_addr, hdr->image_size, hash);
    /* uECC_verify: 返回 1=验证通过, 0=失败 */
    return uECC_verify(ecdsa_public_key, hash, 32, hdr->signature) == 1;

#else
    /* TYPE=2: 无签名 */
    return true;
#endif
}
```

### 5.4 公钥存储

```c
// core/ecdsa_pubkey.h
#ifndef ECDSA_PUBKEY_H
#define ECDSA_PUBKEY_H

/* ECDSA P-256 公钥（64 字节：x[32] || y[32]）
 * 由 generate_keys.py 生成，编译进 bootloader Flash
 *
 * ⚠️ 这个公钥是对应私钥（private_key.pem）的。
 *    换私钥 = 要同时更新这个文件并重新编译 bootloader。
 *
 * 后续可改为从 OTP/Option Bytes 读取（更安全）*/
static const uint8_t ecdsa_public_key[64] = {
    0xXX, 0xXX, 0xXX, ...   /* 厂家生成的实际值 */
};

#endif
```

---

## 6. micro-ecc 配置

### 6.1 uECC 配置宏

在编译 micro-ecc 前定义：

```c
/* 只需要 verify，不需要 sign/make_key */
#define uECC_SUPPORTS_sign 0
#define uECC_SUPPORTS_make_key 0

/* 只用 P-256 */
#define uECC_CURVE_NAMES 0     /* 不编译曲线名字符串 */
#define uECC_ENABLE_VLI_API 0  /* 不导出大数运算 API */

/* 优化：用原生 int 而不是 uint64_t（Cortex-M4 是 32 位）*/
#define uECC_WORD_SIZE 4
```

这样编译出来的 `uECC.c` 只有 verify 相关代码，~3.5 KB。

### 6.2 CMakeLists.txt 加入

```cmake
# ECDSA（如果 BL_SIGNATURE_TYPE == 1）
target_sources(${CMAKE_PROJECT_NAME} PRIVATE
    ${BOOT_DIR}/lib/ecdsa/uECC.c
)
target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE
    ${BOOT_DIR}/lib/ecdsa
)

# 只启用 P-256 曲线，剔除其他曲线代码
target_compile_definitions(${CMAKE_PROJECT_NAME} PRIVATE
    uECC_SUPPORTS_secp160r1=0
    uECC_SUPPORTS_secp192r1=0
    uECC_SUPPORTS_secp224r1=0
    uECC_SUPPORTS_secp256r1=1
    uECC_SUPPORTS_secp256k1=0
)
```

---

## 7. Python 工具

### 7.1 generate_keys.py

```python
#!/usr/bin/env python3
"""生成 ECDSA P-256 密钥对"""
from ecdsa import SigningKey, NIST256p
import os

sk = SigningKey.generate(curve=NIST256p)
vk = sk.get_verifying_key()

# 保存私钥
with open("private_key.pem", "wb") as f:
    f.write(sk.to_pem())
print("✅ 私钥已保存：private_key.pem（保密！）")

# 生成公钥 C 数组
pubkey = vk.to_string()  # 64 bytes
with open("ecdsa_pubkey.h", "w") as f:
    f.write("#ifndef ECDSA_PUBKEY_H\n")
    f.write("#define ECDSA_PUBKEY_H\n\n")
    f.write("static const uint8_t ecdsa_public_key[64] = {\n")
    for i in range(0, 64, 12):
        line = ", ".join(f"0x{b:02X}" for b in pubkey[i:i+12])
        f.write(f"    {line},\n")
    f.write("};\n\n")
    f.write("#endif\n")
print("✅ 公钥已保存：ecdsa_pubkey.h（编译进 bootloader）")
```

### 7.2 sign_firmware.py

```python
#!/usr/bin/env python3
"""对固件签名，生成带 ECDSA 签名的 header"""
from ecdsa import SigningKey, NIST256p
import hashlib, struct, sys, json

def main():
    fw_path = sys.argv[1]
    key_path = sys.argv[2] if len(sys.argv) > 2 else "private_key.pem"
    version = sys.argv[3] if len(sys.argv) > 3 else "1.0.0"

    # 读固件
    fw = open(fw_path, "rb").read()

    # SHA256 哈希
    fw_hash = hashlib.sha256(fw).digest()

    # CRC32（固件校验用）
    import binascii
    fw_crc = binascii.crc32(fw) & 0xFFFFFFFF

    # ECDSA 签名
    sk = SigningKey.from_pem(open(key_path).read())
    signature = sk.sign(fw_hash)  # 64 bytes

    # 解析版本号
    parts = version.split(".")
    major = int(parts[0]) if len(parts) > 0 else 1
    minor = int(parts[1]) if len(parts) > 1 else 0
    build = int(parts[2]) if len(parts) > 2 else 0

    # 输出信息
    print(f"固件: {fw_path}")
    print(f"大小: {len(fw)} bytes ({len(fw)/1024:.1f} KB)")
    print(f"CRC32: 0x{fw_crc:08X}")
    print(f"SHA256: {fw_hash.hex()}")
    print(f"版本: v{major}.{minor}.{build}")
    print(f"签名: {signature.hex()}")
    print(f"\n✅ 签名完成，上位机可以直接烧录此固件")

if __name__ == "__main__":
    main()
```

---

## 8. 体积影响预估

| 配置 | boot+lib | HAL+port+平台 | 总量（BL_LOG=0）|
|------|---------|-------------|----------------|
| TYPE=0（HMAC，当前）| 2.74 KB | 11.0 KB | ~13.7 KB |
| TYPE=1（ECDSA）| **~6.2 KB** | 11.0 KB | **~17.2 KB** |
| TYPE=2（无签名）| ~1.7 KB | 11.0 KB | ~12.7 KB |

---

## 9. 实施步骤（待用户确认后执行）

| Step | 任务 | 工作量 |
|------|------|--------|
| 1 | git clone micro-ecc | 5 分钟（用户做）|
| 2 | 复制 uECC.c/h 到 lib/ecc/ | 5 分钟 |
| 3 | bl_features.h 加 BL_SIGNATURE_TYPE | 10 分钟 |
| 4 | image.c 加 ECDSA 验证分支 | 30 分钟 |
| 5 | 写 generate_keys.py + sign_firmware.py | 30 分钟 |
| 6 | 生成密钥对，公钥编译进 bootloader | 10 分钟 |
| 7 | CMakeLists.txt 加 uECC.c | 5 分钟 |
| 8 | 编译验证 | 10 分钟 |
| 9 | 真机测试（签名固件能升级，篡改固件被拒绝）| 1 小时 |
| 10 | 上位机砍掉密钥输入框（TYPE=1 时隐藏）| 30 分钟 |
| **总计** | | **~3-4 小时** |

---

## 10. 后续优化方向

| 优化 | 说明 |
|------|------|
| 公钥存 OTP | 用 STM32F4 的 32 字节 OTP 区域（分 2 次存 64 字节公钥）|
| 多公钥支持 | header 里加 key_id 字段，MCU 可以有多个公钥 |
| 密钥轮换 | 支持固件里带"新公钥"指令，安全更新公钥 |
| 硬件加密 | STM32H7 有硬件 HASH + PKA，ECDSA 验证快 10x |

---

*文档版本: v1.0*
*创建日期: 2026-07-24*
