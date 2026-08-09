# 移植指南（Porting Guide）

> 把 MCU_BOOT bootloader 移植到新芯片的完整步骤。
> 适用对象：想为新芯片贡献 port 的开发者。
>
> 前置条件：Phase 1 已完成（core 层 100% 芯片无关），架构稳定。

---

## 0. 架构总览

### 4 层架构

```
┌─────────────────────────────────────────────────────┐
│  应用/配置层                                          │
│  └─ main.c / board_config.h / bl_features.h         │
├─────────────────────────────────────────────────────┤
│  Boot 核心层（✅ 芯片无关，移植时 0 改动）            │
│  └─ core/bl_main.c, image.c, protocol.c             │
│     hal_if_defines.h（接口契约）                     │
├─────────────────────────────────────────────────────┤
│  Port 抽象层（❌ 每芯片一份，移植时新建）             │
│  └─ port/<chip>/port_flash.c, port_gpio.c, ...      │
│     实现 hal_if_defines.h 定义的 6 个 interface      │
├─────────────────────────────────────────────────────┤
│  HAL 驱动层（❌ 芯片厂商提供，移植时换库）            │
│  └─ Drivers/<chip>_HAL/                             │
│     Core/Src/main.c 等 CubeMX 生成                  │
│     startup_xxx.s / LinkerScript                    │
└─────────────────────────────────────────────────────┘
```

### 移植时的"动"与"不动"

| 代码 | 移植时 | 备注 |
|------|--------|------|
| `boot/core/*` | ✅ **不改** | 业务逻辑，所有芯片共用 |
| `boot/hal_if_defines.h` | ✅ **不改** | 接口契约 |
| `boot/lib/*` | ✅ **不改** | CRC/SHA256/ringbuffer/EasyLogger |
| `App/bl_features.h` | ✅ **不改** | 业务配置（日志、签名、CRC 等）|
| `projects/<chip>/port/` | ❌ **新建** | 实现 6 个 port_*.c |
| `App/board_config.h` | ⚠️ **改值** | 改 Flash 地址、引脚、UART 实例 |
| `Drivers/` | ❌ **替换** | 换成新芯片的 HAL 库 |
| `Core/Src/main.c, usart.c, ...` | ❌ **替换** | CubeMX 重新生成 |
| `LinkerScript/` | ❌ **替换** | 新芯片的内存布局 |
| `startup_xxx.s` | ❌ **替换** | 新芯片的启动文件 |
| `CMakeLists.txt` | ⚠️ **改路径** | 加新芯片选项 |

---

## 1. 移植前准备

### 1.1 评估目标芯片

| 评估项 | 要求 |
|--------|------|
| CPU 架构 | **ARM Cortex-M**（M0/M0+/M3/M4/M7/M23/M33）|
| Flash 容量 | ≥ 32 KB（bootloader 最小 13 KB，建议预留 16 KB 扇区）|
| RAM 容量 | ≥ 8 KB（协议 buffer + 栈）|
| UART | 至少 1 个（升级用）+ 可选 1 个（日志用）|
| Timer | 至少 1 个 16-bit 定时器（用于 ms 计时）|
| Flash 编程接口 | 厂商 HAL 提供 erase/program API |

**不支持**：8051、AVR、PIC（8 位机） / Xtensa（ESP32 经典） / Linux SoC

### 1.2 准备工具

| 工具 | 用途 |
|------|------|
| 厂商的 HAL 配置工具（如 STM32CubeMX）| 生成 init 代码、链接脚本、startup |
| GCC ARM 工具链（推荐 14.x+）| 编译 |
| CMake + Ninja | 构建系统 |
| 调试器（ST-Link/J-Link/DAPLink）| 烧录 + GDB 调试 |
| 串口调试助手 | 看日志、模拟上位机 |

### 1.3 阅读现有 stm32f4 / stm32f1 port

**必读**（理解 port 层在做什么）：
```
projects/stm32f407/port/        ← 或 projects/stm32f103/port/
├── port.h                      ← 芯片特定的宏和辅助函数
├── port_flash.c                ← Flash erase/program 包装
├── port_gpio.c                 ← GPIO 包装
├── port_uart.c                 ← UART 包装
├── port_timer.c                ← Timer（ms/us/delay）包装
├── port_console_uart.c         ← 日志 UART（只 TX）
├── port_system.c               ← Reset/VTOR/Jump/IRQ 包装
└── stm32fX_port.c              ← 聚合：把所有 port_*.c 装进 platform_desc_t
```

> **推荐**：F103 的 port 层是更精简的参考（RAM 紧张场景下的优化经验），F407 是功能完整的参考。

---

## 2. 移植步骤（10 步）

### Step 1：复制工程模板

```bash
cd projects/
cp -r stm32f103 <new_chip>      # 例如 hc32f4, gd32f1
cd <new_chip>

# 重命名文件（不改内容）
mv port/stm32f1_port.c port/<new_chip>_port.c
```

完成后目录结构：
```
projects/<new_chip>/
├── App/                         ← board_config.h + bl_features.h
├── Core/                        ← CubeMX 生成
├── Drivers/                     ← 新芯片 HAL
├── LinkerScript/
├── cmake/
├── port/
│   ├── port.h
│   ├── port_flash.c
│   ├── port_gpio.c
├── port_uart.c
├── port_timer.c
├── port_system.c
└── <new_chip>_port.c
```

### Step 2：用 CubeMX 生成新芯片的工程

1. 打开 CubeMX，选目标芯片（如 STM32F103C8）
2. 配置外设（参考现有 STM32F4 配置）：
   - **USART3**（或你选的升级 UART）：115200 8N1，使能 RX 中断
   - **USART1**（或你选的日志 UART）：115200 8N1（可选）
   - **TIM6**（或你选的定时器）：1ms 中断
   - **GPIO**：LED 引脚 + KEY 引脚（如果板子有）
3. 配置时钟（HSE/PLL）
4. **生成代码**：选 CMake + GCC ARM 工具链

生成的文件覆盖到工程：
```
Core/Src/main.c, usart.c, tim.c, gpio.c, stm32f4xx_it.c, ...
Core/Inc/main.h, usart.h, ...
Drivers/STM32F1xx_HAL_Driver/        ← 替换掉 STM32F4 的
startup_stm32f103xb.s                ← 替换 startup_stm32f407xx.s
```

### Step 3：写 `board_config.h`（芯片相关配置）

复制现有的 `App/board_config.h`，**只改芯片相关部分**（业务部分在 `bl_features.h`，不动）：

```c
// App/board_config.h
#ifndef BOARD_CONFIG_H
#define BOARD_CONFIG_H

/*=======================================================================
 * Chip Selection  ← 改成你的芯片
 *=======================================================================*/
#define BL_CHIP_STM32F103    1     // 改这里

/*=======================================================================
 * Flash Layout  ← 改成你的芯片的 Flash 参数
 *=======================================================================*/
#define BL_FLASH_BASE        0x08000000U
#define BL_FLASH_TOTAL      (64 * 1024)       // STM32F103C8 = 64KB
#define BL_BOOTLOADER_SIZE   (16 * 1024)      // 留 16KB 给 bootloader
#define BL_APP_BASE          0x08004000U       // BL_FLASH_BASE + BL_BOOTLOADER_SIZE
#define BL_APP_MAX_SIZE      (BL_FLASH_TOTAL - BL_BOOTLOADER_SIZE - 256)
#define BL_MAGIC_HEADER_ADDR 0x08003F00U       // 留 256B 给 header

#define BL_BOOTLOADER_END    (BL_FLASH_BASE + BL_BOOTLOADER_SIZE)
#define BL_HEADER_SIZE       256

/*=======================================================================
 * UART Configuration  ← 改成你的 UART
 *=======================================================================*/
#define BL_UART_INSTANCE     3               // USART3
#define BL_UART_BAUDRATE     115200

#define BL_CONSOLE_UART      1               // USART1（日志）
#define BL_CONSOLE_BAUDRATE  115200

/*=======================================================================
 * Timer  ← 改成你的 Timer
 *=======================================================================*/
#define BL_TIMER_INSTANCE    6

/*=======================================================================
 * GPIO Pin Definitions  ← 改成你板子上的引脚
 *=======================================================================*/
#define BL_LED_PORT          GPIO_PORT_C
#define BL_LED_PIN           13

#define BL_KEY_PORT          GPIO_PORT_A
#define BL_KEY_PIN           0

/*=======================================================================
 * HAL 抽象的 GPIO 端口编号（按芯片 GPIO 数量调整）
 *=======================================================================*/
#define GPIO_PORT_A   0
#define GPIO_PORT_B   1
#define GPIO_PORT_C   2
// 只有 3 个 GPIO 端口就只定义 3 个

#include "bl_features.h"   /* 自动 include 业务配置 */
#endif
```

### Step 4：写 `LinkerScript/<new_chip>.ld`

参考现有 `LinkerScript/STM32F407ZG_Bootloader.ld`，改：

```ld
FLASH_BASE = 0x08000000;
FLASH_SIZE = 64K;            /* STM32F103C8 = 64KB */
RAM_BASE   = 0x20000000;
RAM_SIZE   = 20K;            /* STM32F103C8 = 20KB */

BOOTLOADER_SIZE = 16K;       /* 跟 board_config.h 一致 */
```

### Step 5：实现 6 个 port_*.c 文件

这是移植的**主要工作**。每个文件实现 `hal_if_defines.h` 里定义的接口。

#### 5.1 `port_flash.c` — 最复杂，需要懂芯片 Flash 模型

**必须实现的函数**：
```c
static int   port_flash_init(void);
static void  port_flash_deinit(void);
static int   port_flash_unlock(void);
static int   port_flash_lock(void);
static int   port_flash_erase(uint32_t addr, uint32_t size);
static int   port_flash_program(uint32_t addr, const uint8_t *data, uint32_t size);
static int   port_flash_read(uint32_t addr, uint8_t *data, uint32_t size);

const flash_if_t flash_<chip> = {
    .init = port_flash_init,
    /* ... */
    .base_addr = 0x08000000,
    .total_size = 64 * 1024,
    .sector_size = 1024,        /* 每芯片不同！*/
    .program_granularity = 2,  /* STM32F1 必须 2 字节对齐，STM32F4 1/2/4/8 都行 */
};
```

**关键点**：
- `erase` 函数要把 addr/size 对齐到芯片的 sector 边界
- `program` 函数要遵守 program_granularity（STM32F1 必须半字，STM32F4 可变）
- 失败要返回非 0 错误码

参考 `port/stm32f4/port_flash.c` 的实现，改成新芯片的 HAL API。

#### 5.2 `port_gpio.c`

```c
static int   port_gpio_init(uint8_t port, uint16_t pin, uint8_t mode, uint8_t pupd);
static void  port_gpio_set(uint8_t port, uint16_t pin, bool level);
static bool  port_gpio_read(uint8_t port, uint16_t pin);
static void  port_gpio_deinit(uint8_t port, uint16_t pin);
```

**关键点**：mode/pupd 用项目自定义的 `BL_GPIO_MODE_*` 和 `GPIO_PUPD_*` 宏（不是 HAL 的）。

#### 5.3 `port_uart.c`

```c
static int   port_uart_init(uint32_t baudrate);
static void  port_uart_deinit(void);
static int   port_uart_write(const uint8_t *data, uint32_t size);
static int   port_uart_read(uint8_t *data, uint32_t size, uint32_t timeout_ms);
static void  port_uart_register_rx_cb(void (*callback)(const uint8_t *data, uint32_t size));
```

**关键点**：
- RX 中断里要调 callback（保存到 static 变量）
- write 用阻塞模式（bootloader 不需要 TX 中断）

#### 5.4 `port_timer.c`

```c
static int       port_timer_init(uint32_t period_us);
static void      port_timer_deinit(void);
static uint32_t  port_timer_get_us(void);
static uint32_t  port_timer_get_ms(void);
static void      port_timer_delay_us(uint32_t us);
static void      port_timer_delay_ms(uint32_t ms);
static void      port_timer_register_periodic_cb(tim_periodic_callback_t callback);
```

**关键点**：
- Timer 配置成 1ms 中断
- 中断里 `s_ms_count++`（用 32-bit，49 天溢出无影响）
- `get_us()` = `ms_count * 1000 + timer counter`

#### 5.5 `port_system.c`

```c
static void  port_system_reset(void);
static void  port_system_set_vtor(uint32_t addr);
static void  port_system_disable_irq(uint8_t irq_num);
static void  port_system_enable_irq(uint8_t irq_num);
static void  port_system_get_unique_id(uint8_t *id);
static void  port_system_jump(uint32_t sp, uint32_t pc);
static void  port_system_disable_all_irq(void);   /* ★ 关键：跨 Cortex-M 可移植 */
```

**关键点**：
- `jump` 用 CMSIS `__set_MSP + 函数指针`（跨 Cortex-M 通用，参考 stm32f4 port）
- `disable_all_irq` 根据 Cortex-M 核心版本写不同数量的 ICER
- `get_unique_id` 每个芯片 UID 地址不同（查手册）

#### 5.6 `<chip>_port.c`（聚合）

```c
extern const flash_if_t  flash_<chip>;
extern const uart_if_t   uart_<chip>;
extern const gpio_if_t   gpio_<chip>;
extern const timer_if_t  timer_<chip>;
extern const system_if_t system_<chip>;

static const platform_desc_t s_platform = {
    .flash     = &flash_<chip>,
    .uart      = &uart_<chip>,
    .gpio      = &gpio_<chip>,
    .timer     = &timer_<chip>,
    .system    = &system_<chip>,
    .chip_name = "STM32F103C8",
    .chip_id   = 0x412,
    .bl_version = "1.0.0",
};

const platform_desc_t *platform_get(void) {
    return &s_platform;
}
```

### Step 6：修改 `Core/Src/main.c`

在 CubeMX 生成的 main.c 的 USER CODE BEGIN 2 区域，加入 bootloader 启动代码：

```c
/* USER CODE BEGIN 2 */
#include "bl_config.h"

const platform_desc_t *plat = platform_get();

#if BL_LOG_ENABLED
elog_init();
elog_set_fmt(ELOG_LVL_ASSERT,  ELOG_FMT_LVL | ELOG_FMT_TAG | ELOG_FMT_LINE);
elog_set_fmt(ELOG_LVL_ERROR,   ELOG_FMT_LVL | ELOG_FMT_TAG | ELOG_FMT_LINE);
elog_set_fmt(ELOG_LVL_WARN,    ELOG_FMT_LVL | ELOG_FMT_TAG | ELOG_FMT_LINE);
elog_set_fmt(ELOG_LVL_INFO,    ELOG_FMT_LVL | ELOG_FMT_TAG | ELOG_FMT_LINE);
elog_set_fmt(ELOG_LVL_DEBUG,   ELOG_FMT_LVL | ELOG_FMT_TAG | ELOG_FMT_LINE);
elog_set_fmt(ELOG_LVL_VERBOSE, ELOG_FMT_LVL | ELOG_FMT_TAG | ELOG_FMT_LINE);
elog_start();
#endif

bl_image_init(BL_MAGIC_HEADER_ADDR, plat->flash);

bl_config_t config = {
    .boot_delay_ms     = BL_BOOT_DELAY_MS,
    .rx_timeout_ms     = BL_RX_TIMEOUT_MS,
    .app_vtor          = BL_APP_BASE,
    .magic_header_addr = BL_MAGIC_HEADER_ADDR,
#if BL_ENABLE_KEY_TRAP
    .enable_key_trap   = true,
    .key_port          = BL_KEY_TRAP_PORT,
    .key_pin           = BL_KEY_TRAP_PIN,
#else
    .enable_key_trap   = false,
    .key_port          = 0,
    .key_pin           = 0,
#endif
    .enable_rx_trap    = true,
};

bl_main_init(&config, plat);

bl_result_t result = bl_main();
if (result == BL_RESULT_TRAPPED) {
    bl_main_loop();
}

while (1) {
}
/* USER CODE END 2 */
```

### Step 7：修改 `Core/Src/usart.c`（保留 RX 回调）

CubeMX 生成的 usart.c 可能会覆盖 RX 中断处理。要确保保留 bootloader 的 RX callback 机制。

在 `USER CODE BEGIN 1` 区域：
```c
/* 调用 bootloader 的 RX 回调 */
extern void bl_uart_rx_handler(uint8_t byte);
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart) {
    if (huart->Instance == USART3) {
        /* bootloader RX 处理 */
    }
}
```

### Step 8：修改 `CMakeLists.txt`

每个芯片一个独立 `projects/<chip>/CMakeLists.txt`，从 `projects/stm32f103/CMakeLists.txt` 复制后改三处：

```cmake
# 1. 链接脚本路径
set(LINKER_SCRIPT ${CMAKE_SOURCE_DIR}/LinkerScript/<NewChip>_Bootloader.ld)

# 2. port 源文件列表（重命名 <chip>_port.c）
target_sources(${CMAKE_PROJECT_NAME} PRIVATE
    App/bitops.h App/utils.h App/board_config.h App/bl_features.h
    ${BOOT_DIR}/core/bl_main.c
    ${BOOT_DIR}/core/image.c
    ${BOOT_DIR}/core/protocol.c
    ${BOOT_DIR}/lib/crc/crc16.c
    ${BOOT_DIR}/lib/crc/crc32.c
    ${BOOT_DIR}/lib/sha256/sha256.c
    ${BOOT_DIR}/lib/ringbuffer/ringbuffer.c
    ${BOOT_DIR}/lib/elog/elog.c
    ${BOOT_DIR}/lib/elog/elog_utils.c
    ${BOOT_DIR}/lib/elog/elog_port.c
    ${BOOT_DIR}/lib/ecdsa/uECC.c
    port/port_flash.c
    port/port_gpio.c
    port/port_uart.c
    port/port_console_uart.c
    port/port_timer.c
    port/port_system.c
    port/<new_chip>_port.c       # ← 重命名
)

# 3. include 路径（Drivers 改成新芯片的 HAL 路径）
target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE
    Core/Inc
    Drivers/<NewChip>_HAL_Driver/Inc    # ← 新芯片 HAL
    App
    ${BOOT_DIR}
    ...
)
```

用户编译：
```bash
cd projects/<new_chip>
cmake -B build -G Ninja -DCMAKE_TOOLCHAIN_FILE=cmake/gcc-arm-none-eabi.cmake
cmake --build build
```

### Step 9：编译

```bash
mkdir build && cd build
cmake -G Ninja -DBL_CHIP=<new_chip> ..
ninja
```

预期：0 warning 0 error。**如果有错误**：
- `undefined reference to platform_get`：检查 `<chip>_port.c` 是否在 CMakeLists.txt
- `xxx undeclared`：检查 HAL API 名字（不同芯片 HAL 可能略有差异）
- 链接错地址：检查 LinkerScript 跟 board_config.h 是否一致

### Step 10：真机测试

按"测试清单"（第 4 节）逐项验证。

---

## 3. 不同芯片的常见差异

### 3.1 STM32F1 vs STM32F4

| 差异点 | STM32F4 | STM32F1 |
|--------|---------|---------|
| Flash 编程粒度 | 1/2/4/8 字节 | **必须 2 字节对齐** |
| Flash sector 模型 | 16K/64K/128K 不等 | **1K/16K 页（统一）** |
| GPIO 配置 | `GPIOA->MODER`（一个寄存器管 16 脚）| `GPIOA->CRL/CRH`（两个寄存器，分别管 P0-P7 / P8-P15）|
| 时钟 | 168MHz HSE=8M | 72MHz HSE=8M |
| Cortex-M | M4 + FPU | M3（无 FPU）|

**port_flash.c 是改动最大的**：F1 的 Flash 模型跟 F4 完全不同。

### 3.2 STM32 vs 国产 MCU（HC32/GD32/AT32）

| 差异点 | STM32 | 国产 MCU |
|--------|-------|---------|
| HAL 风格 | `HAL_xxx` | `DDL_xxx` 或 `Drv_xxx`（看厂商）|
| HAL 完整度 | 完整 | 多数兼容 ST，少数有差异 |
| 时钟树 | 标准 ST | 兼容但有细节差异 |
| Flash 模型 | 标准 ST | 兼容但有细节差异 |
| Unique ID 地址 | `0x1FFF7A10`（F4）| 查厂商手册 |

**策略**：国产 MCU 通常是 ST 的 pin-to-pin 替代，HAL API 名字可能略不同，但**结构相似**。复制 stm32f4 port 后逐个改 API 名字。

---

## 4. 测试清单

移植完成后逐项验证：

### 4.1 基础编译

- [ ] `cmake -DBL_CHIP=<new_chip>` 成功
- [ ] `ninja` 0 warning 0 error
- [ ] 生成 .bin 文件
- [ ] Flash 占用 < 16 KB（一个扇区）

### 4.2 烧录 + 启动

- [ ] 烧录 BL.bin 到 0x08000000
- [ ] 复位后 USART1（日志口）能看到 `I/bl (...) [BOOT] started`
- [ ] 看到 `boot in 0s... (key+rx to stay)` 倒计时

### 4.3 升级流程

- [ ] 上位机点"查询设备"
- [ ] 设备复位后能在 500ms 内 trap
- [ ] Device Info 字段正确填充（BL/APP/MTU/Caps）
- [ ] 选固件 + Upload
- [ ] Erase/Program/Verify/WriteHeader 全过
- [ ] 设备能跳转到新 App

### 4.4 安全特性

- [ ] HMAC 签名校验通过（`signature verified`）
- [ ] CRC32 校验通过
- [ ] Header CRC32 校验通过

### 4.5 边界场景

- [ ] 30s 不复位 → 上位机超时弹窗
- [ ] 中途断开 → 上位机状态回灰
- [ ] 设备无 App（空 Flash）→ APP 显示 N/A，能升级任意版本

---

## 5. 常见陷阱

### 5.1 Flash 编程失败

**症状**：`program failed at 0x0800xxxx`

**原因**：
1. addr/size 没对齐到 sector 边界
2. program_granularity 不对（F1 必须 2 字节）
3. Flash 没解锁
4. 写入前没擦除

**排查**：在 `port_flash_program` 加 `log_i` 打印 addr/size/对齐情况。

### 5.2 跳转后 HardFault

**症状**：升级完成 → Boot 命令 → 设备卡死 / HardFault

**原因**：
1. `disable_all_irq` 没正确关中断
2. VTOR 没设置
3. App 的链接脚本跟 bootloader 不一致（vector table 位置错）
4. SP/PC 读取错了字节顺序

**排查**：
- 在 `bl_jump_to_app` 打印 `sp` 和 `pc`，确认值合理
- 检查 App 的链接脚本 vector table 在 0x08010000（或你的 `BL_APP_BASE`）
- 单步调试 `__set_MSP` 和 `app_entry()` 调用

### 5.3 日志不输出

**症状**：USART1 没有任何输出

**原因**：
1. USART1 时钟没 enable
2. USART1 GPIO 没配对（PA9 TX / PA10 RX）
3. EasyLogger 没初始化（`elog_init` 没调用）
4. `elog_port_output` 写错了 UART 实例

**排查**：
- 在 main.c 的 USER CODE BEGIN 2 直接 `HAL_UART_Transmit(&huart1, "test\r\n", 6, 100);`，看是否有输出
- 有输出 → EasyLogger 配置问题；无输出 → UART 初始化问题

### 5.4 UART RX 收不到数据

**症状**：上位机发数据，设备 trap 不触发，Device Info 不显示

**原因**：
1. RX GPIO 没配成 AF 模式
2. RX 中断没使能
3. UART RX callback 没注册到 bootloader

**排查**：
- 在 `HAL_UART_RxCpltCallback` 加 `HAL_GPIO_TogglePin(LED_GPIO_Port, LED_Pin);`，看 LED 是否闪
- 不闪 → 中断没进来 → 检查 NVIC 配置
- 闪 → callback 没注册 → 检查 `bl_uart_rx_cb`

### 5.5 编译警告

**常见警告 + 处理**：

| 警告 | 原因 | 处理 |
|------|------|------|
| `GPIO_MODE_ANALOG redefined` | 项目宏跟 HAL 宏冲突 | 用 `BL_GPIO_MODE_*` 前缀（项目已处理）|
| `signed conversion changes value` | 类型不匹配 | 显式 cast |
| `unused variable` | 死代码 | 删除或加 `(void)x;` |

---

## 6. 移植工作量预估

| 芯片类型 | 工作量 | 备注 |
|---------|--------|------|
| STM32F4 → STM32F1（同厂商不同系列）| 半天 | HAL 兼容，主要改 Flash 模型 |
| STM32F4 → STM32F7 / H7（同厂商升级）| 半天 | HAL 兼容度更高 |
| STM32F4 → STM32F0 / G0（同厂商降级）| 1 天 | Cortex-M0，NVIC 写法要改 |
| STM32F4 → HC32F4 / GD32F4（国产替代）| 1 天 | HAL 名字略不同 |
| STM32F4 → 国产 RISC-V（如 ESP32-C3）| 3-5 天 | 架构完全不同，要重写 port_system |
| STM32F4 → nRF52（Nordic）| 1-2 天 | HAL 风格不同 |

---

## 7. 完成移植后的贡献流程

### 7.1 提交 PR

1. Fork 仓库
2. 创建分支：`git checkout -b feature/port-<chip>`
3. 在 `projects/<chip>/` 提交完整工程（含 CubeMX 配置 + port 实现）
4. 更新根 README 的支持芯片矩阵
5. PR 描述里写：芯片型号、测试通过的功能、已知问题

### 7.2 CI 验证

仓库有 GitHub Actions CI（详见 `docs/CICD_GUIDE.md`），自动编译你的芯片。

### 7.3 文档更新

在 `README.md` 的"支持芯片"列表加你的芯片型号。

---

## 8. 求助

如果移植遇到问题：
1. 先看本指南第 5 节"常见陷阱"
2. 在 GitHub Issues 搜类似问题
3. 提 Issue 时附上：芯片型号、完整编译日志、调试输出

---

## 9. 量产烧录场景

移植完成后，量产阶段的烧录/升级分三种场景：

### 9.1 厂家烧录 Bootloader（生产线下线）

工具：ST-Link / J-Link / CMSIS-DAP

```
1. 烧录 MCU_BOOT.bin/.hex 到 Flash 起始地址（如 0x08000000）
2. 烧录完成后设备上电
3. BL 启动 → 500ms trap 窗口 → 无 App 则停在 bootloader
```

### 9.2 客户升级固件（使用上位机）

```
1. 打开 MCU_BOOT_Tool.exe
2. 选择串口 → 点【查询设备】
3. 设备上电/复位 → 状态灯变绿（设备就绪）
4. 点【Browse】→ 选择 *_signed.bin（签名固件）
   版本号自动显示（从文件 header 读取）
5. 点【Upload】→ 自动完成 Erase/Program/Verify/WriteHeader/Boot
6. 设备重启跑新固件

注意：上位机不需要密钥！密钥只在厂家的 Python 签名脚本里。
```

### 9.3 客户没有上位机（命令行烧录）

```bash
# 用 STM32CubeProgrammer 或 OpenOCD
STM32_Programmer.exe -c port=COM3 -w app_signed.bin 0x08010000  # F407
STM32_Programmer.exe -c port=COM3 -w app_signed.bin 0x08006400  # F103
```

---

## 10. 参考

### 项目内相关文档
- 平台化解耦历史：`docs/archive/PLATFORM_DECOUPLING_ROADMAP.md`（已归档，作为历史记录）
- 开源准备：`docs/archive/OPEN_SOURCE_EXECUTION_PLAN.md`（已归档）
- 协议规范：`docs/PROTOCOL_SPEC.md`
- 镜像格式：`docs/IMAGE_FORMAT.md`
- 架构图：`docs/architecture_diagrams.md`

### F103 实战经验
- 静默写入失败调试：`DEBUG_LOG_20260730_F103_FLASH_SILENT_WRITE.md`（必读，包含 F103 特有的 flash 模式位残留问题）

### 主流芯片的 HAL 文档
- **STM32**: ST 官网 STM32CubeMX/HAL
- **HC32**: 华大半导体官网 HDSC DDL
- **GD32**: 兆易创新官网 GD32FwLib
- **AT32**: 雅特力官网 AT32F4xx_Firmware_Library
- **NXP Kinetis**: NXP MCUXpresso
- **Nordic nRF52**: Nordic Semiconductor nRF5 SDK

### Cortex-M 通用参考
- **CMSIS**: ARM CMSIS 标准接口
- **ARMv7-M Architecture Reference Manual**: 寄存器/指令集权威手册
- **ARM Cortex-M Programming Guide**: 入门到精通

---

*文档版本: v1.0*
*创建日期: 2026-07-20*
*下次更新：Phase 2 完成 STM32F1 / HC32F4 移植后补充经验*
