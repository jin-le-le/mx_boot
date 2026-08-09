/*
 * EasyLogger 芯片无关 port 层
 *
 * 改造说明（2026-07-20）：
 *   原版直接调 STM32 HAL_UART_Transmit（依赖 main.h、usart.h、huart1），
 *   导致这个文件只能在 STM32 + CubeMX 工程下工作。
 *
 *   现在改用 platform_desc_t.console_uart 接口（hal_if_defines.h 定义），
 *   elog_port.c 完全芯片无关，所有芯片共用本文件，移植时 0 改动。
 *
 *   每个芯片的 port/<chip>/ 下需要：
 *     1. 实现一个 uart_if_t console_uart_<chip>
 *     2. 注册到 platform_desc_t.console_uart 字段
 *   如果某些芯片没有日志 UART，注册为 NULL，elog 会安静跳过输出。
 */

#include <elog.h>
#include "hal_if_defines.h"   /* platform_desc_t 定义（include path 已配置）*/

/* 通过单例拿到 console_uart，避免 main.c 再额外注入 */
extern const platform_desc_t *platform_get(void);

/**
 * EasyLogger port initialize
 * UART 等硬件初始化由各芯片 port 层自己负责（在 platform_desc_t.console_uart）
 */
ElogErrCode elog_port_init(void) {
    /* 如果 console_uart 实现了 init，可以在这里调用：
     *   const platform_desc_t *p = platform_get();
     *   if (p && p->console_uart && p->console_uart->init) {
     *       p->console_uart->init(115200);
     *   }
     * 但通常 CubeMX/启动代码已经初始化过 UART，这里留空即可。
     */
    return ELOG_NO_ERR;
}

/**
 * EasyLogger port deinitialize
 */
void elog_port_deinit(void) {
    const platform_desc_t *p = platform_get();
    if (p && p->console_uart && p->console_uart->deinit) {
        p->console_uart->deinit();
    }
}

/**
 * output log port interface
 *
 * 通过 platform_desc_t.console_uart 接口输出，跟具体芯片 HAL 解耦。
 * 没有 console_uart（NULL）时安静跳过，避免 null pointer fault。
 */
void elog_port_output(const char *log, size_t size) {
    const platform_desc_t *p = platform_get();
    if (p && p->console_uart && p->console_uart->write) {
        p->console_uart->write((const uint8_t *)log, size);
    }
}

/**
 * output lock - 单线程裸机，空实现
 */
void elog_port_output_lock(void) {
    /* 如果以后上 RTOS，可以放 __disable_irq() 或 mutex take */
}

/**
 * output unlock - 单线程裸机，空实现
 */
void elog_port_output_unlock(void) {
    /* 对应放 __enable_irq() 或 mutex give */
}

/**
 * get current time interface - 项目未配置 RTC，返回空
 */
const char *elog_port_get_time(void) {
    return "";
}

/**
 * get current process name interface - 裸机无进程概念
 */
const char *elog_port_get_p_info(void) {
    return "";
}

/**
 * get current thread name interface - 裸机无线程概念
 */
const char *elog_port_get_t_info(void) {
    return "";
}
