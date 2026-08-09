# 架构图（Mermaid 格式 + 面试白板速画）

本文件包含 4 张图的源码，可以直接粘贴到 GitHub README（Mermaid 自动渲染），或导入 draw.io。

> 图示地址以 F407 (`0x08010000`) 为例。F103 是 `0x08006400`，其他芯片由 `device_info_t.app_base` 动态决定。

---



## 图 1：系统架构图（4 层）

### Mermaid（GitHub README 自动渲染）

```mermaid
graph TB
    subgraph APP["应用层"]
        MAIN["main.c<br/>boot_init() + boot_run()"]
    end

    subgraph CONFIG["配置层"]
        BOARD["board_config.h<br/>Flash地址 / 引脚 / UART实例<br/><i>移植时改</i>"]
        FEAT["bl_features.h<br/>签名策略 / CRC模式 / 日志<br/><i>所有芯片共用</i>"]
    end

    subgraph CORE["Boot 核心组件 5.4KB · 100%芯片无关"]
        direction LR
        BL["core/<br/>bl_main · image · protocol<br/>启动决策 · 镜像校验 · 协议解析"]
        LIB["lib/<br/>CRC · SHA256 · ECDSA · RB · elog"]
        IF["hal_if_defines.h<br/>platform_desc_t<br/>6个函数指针表"]
    end

    subgraph PORT["Port 适配层 ~500行 · 每芯片一份"]
        direction LR
        P1["port_flash.c"]
        P2["port_gpio.c"]
        P3["port_uart.c"]
        P4["port_timer.c"]
        P5["port_system.c"]
        P6["port_console.c"]
    end

    subgraph HAL["HAL 驱动层 · 厂商提供"]
        STM32["STM32F4xx HAL Driver<br/>+ CMSIS"]
    end

    MAIN --> BOARD
    MAIN --> FEAT
    BOARD --> BL
    FEAT --> BL
    BL --> LIB
    BL --> IF
    IF -->|"函数指针调用"| PORT
    PORT -->|"HAL API"| STM32

    style CORE fill:#2e7d32,color:#fff
    style PORT fill:#1565c0,color:#fff
    style HAL fill:#757575,color:#fff
    style IF fill:#e65100,color:#fff
```

---

## 图 2：安全签名流程

### Mermaid

```mermaid
sequenceDiagram
    participant MFR as 厂家（离线）
    participant TOOL as Python 签名工具
    participant UC as 客户上位机
    participant MCU as MCU Bootloader

    Note over MFR,TOOL: 首次：生成密钥（只做一次）
    MFR->>TOOL: generate_keys.py
    TOOL-->>MFR: private_key.pem（保密！）
    TOOL-->>MCU: ecdsa_pubkey.h（编译进BL）

    Note over MFR,TOOL: 每次发布新固件
    MFR->>TOOL: sign.sh app.bin 2.1.0
    TOOL->>TOOL: SHA256(app.bin) → 哈希
    TOOL->>TOOL: ECDSA_Sign(哈希, 私钥) → 签名64B
    TOOL-->>MFR: app_signed.bin<br/>(header256B + app)

    MFR->>UC: 分发 app_signed.bin

    Note over UC,MCU: 客户烧录（零密钥）
    UC->>MCU: Erase
    UC->>MCU: Program（分包传输）
    UC->>MCU: Verify CRC32
    UC->>MCU: WriteHeader（含签名）
    UC->>MCU: Boot

    Note over MCU: 验证流程
    MCU->>MCU: SHA256(Flash固件) → 哈希
    MCU->>MCU: uECC_verify(公钥, 哈希, 签名)
    alt 验证通过
        MCU-->>UC: ✅ 跳转App
    else 验证失败
        MCU-->>UC: ❌ 拒绝启动
    end
```

---

## 图 3：升级时序图

### Mermaid

```mermaid
sequenceDiagram
    participant UC as 上位机
    participant MCU as MCU Bootloader

    Note over UC: 点击「查询设备」<br/>100ms轮询 × 30s超时

    loop 每100ms
        UC->>MCU: GetDeviceInfo (0xAA 01 ...)
    end

    Note over MCU: 设备上电/复位<br/>500ms boot窗口

    MCU-->>UC: DeviceInfo 16B<br/>(BL版本 + App版本 + MTU + Caps)

    Note over UC: 状态灯变绿，Upload启用

    UC->>MCU: Erase (0x08010000, size)
    MCU-->>UC: OK

    loop 分包传输
        UC->>MCU: Program (addr, data[CRC16])
        MCU-->>UC: OK
    end

    UC->>MCU: Verify (addr, size, CRC32)
    MCU-->>UC: OK

    UC->>MCU: WriteHeader (256B, 含ECDSA签名)
    MCU-->>UC: OK

    UC->>MCU: Boot
    MCU-->>UC: OK
    Note over MCU: ECDSA验证 → 跳转App
```

---

## 图 4：体积对比图

### Mermaid 柱状图（用 markdown 表格代替）

```
体积对比 (KB)
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
MCUBoot (ARM官方)     ████████████████████████████ 32.0
你的项目 (BL_LOG=1)   █████████████████████ 23.7
你的项目 (BL_LOG=0)   ████████████████ 16.6
core + lib (纯组件)   █████ 5.4
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
```

---

## draw.io 导入方法

### 方法 1：用 draw.io 打开 Mermaid

1. 打开 https://app.diagrams.net
2. 菜单 → Arrange → Insert → Advanced → Mermaid
3. 粘贴上面的 Mermaid 代码
4. 自动生成图形，可拖拽调整位置
5. File → Export as → PNG（高清）或 SVG（矢量）

### 方法 2：从零画（更灵活）

1. 打开 https://app.diagrams.net
2. 用左侧形状库画矩形 + 箭头
3. 推荐布局：

```
┌─────────────────────────────────────────┐
│  main.c → boot_init() + boot_run()     │  ← 顶部：入口
├─────────────────────────────────────────┤
│  board_config.h    │  bl_features.h     │  ← 配置层
│  (芯片相关)         │  (业务相关)        │
├─────────────────────────────────────────┤
│                                         │
│  ┌──────────┐  ┌──────────┐           │
│  │  core/   │  │  lib/    │  5.4KB    │  ← 核心（绿色框）
│  │ 业务逻辑  │  │ 算法库   │  100%无关 │
│  └──────────┘  └──────────┘           │
│                                         │
│  ┌─────────────────────────────────┐   │
│  │  hal_if_defines.h               │   │
│  │  platform_desc_t (函数指针表)    │   │  ← 接口（橙色框）
│  └─────────────────────────────────┘   │
│                                         │
├─────────────────────────────────────────┤
│                                         │
│  ┌────────┐┌────────┐┌────────┐       │
│  │flash   ││gpio    ││uart    │       │
│  └────────┘└────────┘└────────┘       │  ← Port（蓝色框）
│  ┌────────┐┌────────┐┌────────┐       │
│  │timer   ││system  ││console │       │
│  └────────┘└────────┘└────────┘       │
│             ~500 行                    │
│                                         │
├─────────────────────────────────────────┤
│  STM32 HAL Driver + CMSIS               │  ← 底部：灰色
└─────────────────────────────────────────┘
```

配色建议：
- 核心层：绿色 `#4CAF50`
- 接口层：橙色 `#FF9800`
- Port 层：蓝色 `#2196F3`
- HAL 层：灰色 `#757575`

### 方法 3：Excalidraw（手绘风格，适合 README）

1. 打开 https://excalidraw.com
2. 手动画框 + 箭头
3. 导出 PNG/SVG

---

## 面试白板速画版（30 秒画完）

```
    ┌─────────────┐
    │  main.c     │  boot_init + boot_run
    ├──────┬──────┤
    │board │feat  │  配置
    ├──────┴──────┤
    │   core+lib  │  5.4KB 不用改 ← 画个圈强调
    │   (函数指针)  │
    ├──────┬──────┤
    │ port │6文件 │  ~500行 移植时写
    ├──────┴──────┤
    │  HAL Driver │  厂商的
    └─────────────┘
```

**30 秒话术**：
> "4 层架构，中间这块是核心，5.4KB，完全不碰硬件，通过函数指针调下面。移植时只换最下面两层。"
