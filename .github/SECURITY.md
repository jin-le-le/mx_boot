# Security Policy

## Reporting a Vulnerability

MCU_BOOT 是一个涉及 **ECDSA 数字签名验签** 的安全敏感项目。我们非常重视签名验证、防回滚、Flash 安全等领域的漏洞。

**请勿通过公开 Issue 报告安全漏洞。**

### 报告渠道

请通过以下任一方式私下披露：

- **GitHub Security Advisory**（推荐）：在仓库页面 → Security → Report a vulnerability
- **邮件**：发送至项目维护者邮箱，主题加上 `[SECURITY]` 前缀

报告时请尽量包含：

1. 受影响的版本 / commit hash
2. 漏洞类型（如签名绕过、降级攻击、Buffer overflow）
3. 复现步骤或 PoC
4. 影响范围（哪些芯片 / 配置组合受影响）
5. 你建议的修复方向（可选）

### 响应时间

- 收到报告后 **72 小时内**确认收到
- **7 天内**给出初步评估（是否接受 / 严重等级）
- **30 天内**发布修复版本或缓解措施（复杂问题可能延长，会同步进展）

## Threat Model

MCU_BOOT 设计上防御以下威胁：

| 威胁 | 防御机制 |
|------|----------|
| 攻击者构造未签名固件烧录 | `BL_REJECT_UNSIGNED=1` + header `flags & SIGNED` 校验 |
| 攻击者用伪造签名替换固件 | ECDSA P-256 验签（micro-ecc），公钥编译进 bootloader |
| 攻击者降级到有漏洞的旧版本 | `BL_ANTIROLLBACK_ENABLED=1` + `BL_MIN_APP_VERSION` |
| 攻击者通过 bootloader 擦除 bootloader 自身 | `bl_image_is_addr_safe` 地址范围校验 + LinkerScript 编译期 `ASSERT` |
| 攻击者读出 Flash 后伪造固件 | 私钥仅离线保管，MCU 端无任何密钥；可选 RDP Level 1 |

**不在威胁模型范围内**：

- 物理攻击（电压故障注入、激光、电磁脉冲等需要硬件改造的攻击）
- 侧信道攻击（micro-ecc 不抗 DPA/SPA，量产场景建议使用带 SCA 防护的实现）
- 调试器实时攻击（如 SWD 注入；启用 RDP Level 2 可缓解但会影响正常调试）

## 版本支持策略

只有最新发布的版本接收安全修复。每个 release 的 CHANGELOG 中会标注已修复的安全问题。

## 已知安全注意事项

### 1. 公钥替换

`boot/core/ecdsa_pubkey.h` 默认是全 0 占位符。**正式部署前必须运行 `tool/sign_tool/generate_keys.py` 生成自己的密钥对并替换占位公钥**，否则任何人都可以伪造你的固件（因为你可以用空公钥对应的"私钥"——但实际上空公钥会让验签直接失败，所以这只是一个安全提示，不是漏洞）。

### 2. RDP Level 配置

STM32F4 默认启用 RDP Level 1（防调试器读 Flash），STM32F1 当前未启用（资源紧张 + 调试期）。量产前请根据芯片支持情况确认 RDP 配置。

### 3. ECDSA 实现选择

micro-ecc 是经过审计的轻量库，但**不抗侧信道攻击**。如果产品部署在物理可达环境（消费电子、车载等），请评估是否需要替换为带 SCA 防护的实现（如 mbedtls + STM32 PKA）。
