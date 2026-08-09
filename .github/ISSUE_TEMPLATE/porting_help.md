---
name: Porting Help
about: 移植到新芯片时遇到的问题（不确定是 bug 还是芯片特性）
title: "[PORTING] "
labels: porting, question
assignees: ''
---

## 目标芯片

| 项 | 值 |
|---|---|
| 芯片型号 |  |
| Flash 容量 |  |
| RAM 容量 |  |
| Flash 起始地址 |  |
| Flash 擦除粒度 |  |
| Flash 编程粒度 |  |
| 驱动库 | <!-- ST HAL / HC32 DDL / NXP SDK / 其他 --> |

## 卡在哪一步

<!-- 参考 docs/PORTING_GUIDE.md 的 10 步流程，说明你进行到第几步。 -->

## 你的 port 实现关键片段

```c
/* 把你卡住的 port_xxx.c 实现贴这里 */
```

## 现象

<!-- 编译过 / 链接过 / 烧录后无反应 / trap 不触发 / ERASE 命令失败 / PROGRAM 数据不对 / VERIFY 失败 / 跳转后 HardFault -->

## 已做的检查

- [ ] `board_config.h` 地址对齐到 Flash 粒度边界
- [ ] `LinkerScript` 加了 `ASSERT(_boot_flash_used <= BOOTLOADER_SIZE, ...)`
- [ ] `boot/core/` 和 `boot/lib/` 一行没改
- [ ] 中断处理：Flash 擦写时关了全局中断（F103 / HC32 必做）
- [ ] 跳转前清了 NVIC ICER/ICPR
