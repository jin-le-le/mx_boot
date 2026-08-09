# Contributing to MCU_BOOT

感谢你对 MCU_BOOT 的兴趣！本文档说明如何向本项目提交贡献。

---

## 项目分层（提交前必读）

```
boot/core/    ← 芯片无关核心（100% 共享，改动需特别谨慎）
boot/lib/     ← 第三方 / 通用库（micro-ecc、EasyLogger、CRC、SHA256、ringbuffer）
projects/<chip>/App/   ← 板级配置（board_config.h / bl_features.h）
projects/<chip>/port/  ← 芯片相关的 port 实现（每芯片一份）
projects/<chip>/Core/  ← CubeMX 生成的 HAL 初始化代码
projects/<chip>/Drivers/  ← 厂商 SDK
```

### 改动原则

| 改动位置 | 是否需要 PR |
|----------|-------------|
| `boot/core/` | ✅ 但必须证明对 F407 / F103 / HC32F460 三芯片都不破坏 |
| `boot/lib/crc`、`boot/lib/ringbuffer`、`boot/lib/sha256` | ✅ 自有代码，按业务需要改 |
| `boot/lib/{ecdsa,elog}` | ⚠️ 第三方代码，只升级版本或修高危 bug |
| `projects/<chip>/App/`、`port/` | ✅ 芯片相关，欢迎补充新芯片或修移植问题 |
| `projects/<chip>/Drivers/`、`Core/` | ❌ 厂商生成，不要直接改 |

---

## 开发环境

| 工具 | 版本 |
|------|------|
| ARM GCC (`arm-none-eabi-gcc`) | 14.x+ |
| CMake | 3.22+ |
| Ninja | any |
| Python | 3.8+（签名工具） |
| .NET SDK | 6.0+（C# 上位机） |
| `ecdsa` Python 包 | `pip install ecdsa` |

---

## 提交前自检清单

PR 提交前请确认：

- [ ] 至少一个芯片工程能本地编译通过（`cmake --build build`）
- [ ] 如果改了 `boot/core/`，跑通至少一个芯片的完整升级流程（擦除 → 编程 → CRC → ECDSA → 跳转）
- [ ] 没有引入新的硬编码地址（`boot/core/` 不允许出现 `0x08000000` 之类的值，从 `board_config.h` 取）
- [ ] 没有提交构建产物（`build/`、`*.o`、`*.elf`、`*.bin` 等，已在 `.gitignore` 中排除）
- [ ] 没有提交私钥（`tool/sign_tool/private_key.pem`，已 gitignore）
- [ ] 代码风格与周边代码一致（4 空格缩进、K&R 大括号、注释用中文或英文都行但要保持文件内一致）
- [ ] 如果改了协议或 Image Header 格式，**同步更新** `docs/PROTOCOL_SPEC.md` / `docs/IMAGE_FORMAT.md`

---

## Commit 规范

参考 [Conventional Commits](https://www.conventionalcommits.org/)：

```
<type>(<scope>): <subject>

<body>
```

### 常用 type

| type | 用途 |
|------|------|
| `feat` | 新功能（如新增一款芯片的 port） |
| `fix` | bug 修复 |
| `refactor` | 重构（不改外部行为） |
| `docs` | 文档更新 |
| `chore` | 构建 / 配置 / CI 等杂项 |
| `test` | 测试相关 |
| `perf` | 性能优化（如减小代码体积） |

### 示例

```
feat(hc32f460): add port_flash for HC32F460 EFM controller

实现 port_flash.c 适配 HC32 DDL EFM 接口，处理 4 字节编程粒度
和 BUS_HOLD 期间的 CPU stall。

Closes #42
```

```
fix(bl_main): clear all NVIC ICER/ICPR before jump to app

之前只清 ICER[0]，F407 上有超过 32 个 IRQ 时高位 IRQ 残留会
导致跳转后偶发 HardFault。
```

---

## 添加新芯片支持的 PR 流程

参考 `docs/PORTING_GUIDE.md` 的完整步骤，PR 时建议：

1. 新建 `projects/<chip>/` 目录，结构对齐现有 F407 / F103 / HC32F460
2. 在 `board_config.h` 顶部加注释说明芯片型号 / Flash / RAM / 验证状态
3. 在根 `README.md` 的"支持芯片矩阵"表格补一行
4. 如果芯片厂商 SDK 体积 >50 MB，**不要直接提交到仓库**，写 `projects/<chip>/Drivers/README.md` 给出下载来源和解压指引
5. PR 描述里说明：实际编译大小 / bootloader 配额 / 验证了哪些场景

---

## 报告 Bug vs 提交 PR

- **不确定是不是 bug**：先开 Issue 描述现象、复现步骤、芯片型号、bootloader 版本
- **明确的小 bug / 文档错字**：直接开 PR
- **新功能 / 大改动**：先开 Issue 讨论设计，达成共识后再写代码

---

## 行为准则

参与本项目即代表你同意遵守 [Code of Conduct](.github/CODE_OF_CONDUCT.md)。请在交流中保持尊重和建设性。
