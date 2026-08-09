## 改动说明

<!-- 这个 PR 做了什么？为什么？关联到哪个 Issue？ -->

Closes #

## 改动类型

- [ ] 🐛 Bug 修复（不改外部行为）
- [ ] ✨ 新功能（如新芯片支持、新协议命令）
- [ ] ♻️ 重构（不改外部行为）
- [ ] 📚 文档更新
- [ ] ⚡ 性能优化（如减小 Flash 占用）
- [ ] 🔒 安全相关（涉及签名 / 防回滚 / Flash 保护）
- [ ] 🔨 构建 / CI / 配置

## 影响范围

- [ ] `boot/core/`（所有芯片共用）
- [ ] `boot/lib/`（哪些子目录？）
- [ ] `projects/stm32f407/`
- [ ] `projects/stm32f103/`
- [ ] `projects/hc32f460/`
- [ ] `tool/sign_tool/`
- [ ] `tool/mcu_boot_tool/`
- [ ] 文档（哪些？）

## 自检清单

- [ ] 本地至少一个芯片编译通过
- [ ] 改 `boot/core/` 时，确认对其他芯片不破坏（如何验证？）
- [ ] 没有引入新的硬编码地址（`boot/core/` 不允许出现具体 Flash 地址）
- [ ] 没有 commit 构建产物 / 私钥 / 临时文件
- [ ] 改了协议或 Header 格式时，同步更新 `docs/PROTOCOL_SPEC.md` / `docs/IMAGE_FORMAT.md`
- [ ] 改了芯片相关行为时，同步更新 `README.md` 支持矩阵或 `docs/PORTING_GUIDE.md`
- [ ] commit message 遵循 Conventional Commits（见 CONTRIBUTING.md）

## 测试方式

<!-- 描述你如何验证这个改动。例：
- F407 上烧录新 bootloader，用上位机升级 app_signed.bin，确认完整流程通过
- F103 上跑 bl_log 100 次连续 ERASE+PROGRAM，无 HardFault
-->

## 截图 / 日志（可选）

<!-- 如果是 GUI / 协议改动，贴前后对比 -->
