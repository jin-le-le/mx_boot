---
name: Bug Report
about: 报告 MCU_BOOT 的 bug（升级失败 / HardFault / 协议异常等）
title: "[BUG] "
labels: bug
assignees: ''
---

## 现象描述

<!-- 一句话说明发生了什么。例：F103 升级到 70% 时 HardFault。 -->

## 复现步骤

1.
2.
3.

## 期望行为

<!-- 正常情况下应该发生什么。 -->

## 实际行为

<!-- 实际发生了什么。如果有日志 / crash dump / 寄存器值请贴出来。 -->

## 环境

| 项 | 值 |
|---|---|
| 芯片型号 | <!-- 如 STM32F103C8T6 / STM32F407ZGT6 / HC32F460PETB --> |
| Bootloader 版本 | <!-- commit hash 或 v2.0.2 --> |
| `bl_features.h` 关键配置 | <!-- BL_SIGNATURE_TYPE / BL_LOG_ENABLED / CRC32_MODE --> |
| 上位机版本 | <!-- tool/mcu_boot_tool 的 commit --> |
| 工具链 | <!-- arm-none-eabi-gcc 14.x + CMake + Ninja --> |
| 串口 / 调试器 | <!-- 如 ST-Link V2 / J-Link / USB-TTL @ 115200 --> |

## 升级前的 Flash 布局

```
<!-- 把 chip 的 board_config.h 关键宏贴出来，方便排查地址冲突 -->
BL_FLASH_BASE           =
BL_FLASH_TOTAL          =
BL_BOOTLOADER_SIZE      =
BL_APP_BASE             =
BL_MAGIC_HEADER_ADDR    =
```

## 日志 / 截图

<!-- 如果 BL_LOG_ENABLED=1，把 EasyLogger 输出贴这里。如果有上位机截图也贴这里。 -->

```
[日志贴这里]
```

## 已做的排查

<!-- 你已经尝试过什么，排除了哪些可能性。这能节省维护者的时间。 -->
