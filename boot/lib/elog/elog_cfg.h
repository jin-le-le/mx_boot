/*
 * This file is part of the EasyLogger Library.
 * https://github.com/armink/EasyLogger
 *
 * Copyright (c) 2015-2016, Armink, <armink.ztl@gmail.com>
 * MIT License
 *
 * ====== MCU_BOOT 项目定制配置（STM32F4 裸机）======
 *
 * 跟官方默认配置的差异：
 *   - 关闭 async/buf/pthread（裸机无 POSIX 线程）
 *   - ELOG_LINE_BUF_SIZE 从 1024 减到 256（bootloader 日志不会太长）
 *   - 关闭 ELOG_ASSERT_ENABLE（项目有自己的 assert_failed）
 *   - 关闭 ELOG_FMT_USING_FUNC/DIR（节省文件名字符串，只保留 line）
 *   - 保留 ELOG_COLOR_ENABLE（串口终端可显示颜色，可读性好）
 *
 * 体积估算（BL_LOG=1 时）：
 *   - 当前配置：约 +2.0 KB vs 关日志
 *   - 若关 ELOG_COLOR_ENABLE：再省 ~500 B
 *   - 若关 ELOG_FMT_USING_LINE：再省 ~200 B
 */

#ifndef _ELOG_CFG_H_
#define _ELOG_CFG_H_

/*---------------------------------------------------------------------------*/
/* enable log output.
 * 这里**不**直接 #define ELOG_OUTPUT_ENABLE，而是由 board_config.h 里的
 * BL_LOG_ENABLED 控制。BL_LOG_ENABLED=0 时不定义 ELOG_OUTPUT_ENABLE，
 * elog.h 里的 log_i/log_e 等宏展开为 ((void)0)，所有 elog_xxx 代码会被
 * --gc-sections 剔除，日志零开销。
 *
 * 保留 #ifndef 是为了允许外部编译选项覆盖（极少用）。*/
#ifndef ELOG_OUTPUT_ENABLE
/* 默认行为：由 board_config.h 决定，这里不定义 */
#endif

/* ELOG_OUTPUT_LVL 必须有默认值（即使 BL_LOG=0，elog.h 也会引用）
 * 默认 INFO(3)。board_config.h 在 BL_LOG=1 时会覆盖为 BL_LOG_LEVEL */
#ifndef ELOG_OUTPUT_LVL
#define ELOG_OUTPUT_LVL                          ELOG_LVL_INFO
#endif

/* 关闭 assert 检查（项目有自己的 assert_failed CubeMX 默认实现）*/
/* #define ELOG_ASSERT_ENABLE */

/* 单条日志格式化缓冲区大小。bootloader 日志最长不超过 200 字节，256 足够 */
#define ELOG_LINE_BUF_SIZE                       256

/* 行号最大长度（数字位数），5 = 最多 99999 行 */
#define ELOG_LINE_NUM_MAX_LEN                    5

/* tag 过滤器最大长度 */
#define ELOG_FILTER_TAG_MAX_LEN                  16

/* 关键字过滤器最大长度 */
#define ELOG_FILTER_KW_MAX_LEN                   16

/* tag 级别过滤器最大数量 */
#define ELOG_FILTER_TAG_LVL_MAX_NUM              4

/* 换行符
 * 用 "\r\n" 而不是 "\n"：UART 输出不经过 printf 翻译，单独的 \n 只下移
 * 一行但不回到第 0 列，会导致 MobaXterm/PuTTY 等终端显示错位（每行
 * 累积前一行的列偏移）。"\r\n" 是 Windows/Linux 终端通用的换行。*/
#define ELOG_NEWLINE_SIGN                        "\r\n"

/*---------------------------------------------------------------------------*/
/* 颜色 / 格式化选项：由 bl_features.h 的 BL_LOG_COLOR / BL_LOG_LINE 控制
 * 这里用 #ifndef 守卫：bl_features.h 优先，没定义时用这里的默认值 */

#ifndef ELOG_COLOR_ENABLE
/* 默认不启用颜色。用户在 bl_features.h 设 BL_LOG_COLOR=1 启用 */
/* #define ELOG_COLOR_ENABLE */
#endif

/* 各级别默认颜色（ELOG_COLOR_ENABLE 启用时才编译）*/
#define ELOG_COLOR_ASSERT                        (F_MAGENTA B_NULL S_NORMAL)
#define ELOG_COLOR_ERROR                         (F_RED B_NULL S_NORMAL)
#define ELOG_COLOR_WARN                          (F_YELLOW B_NULL S_NORMAL)
#define ELOG_COLOR_INFO                          (F_CYAN B_NULL S_NORMAL)
#define ELOG_COLOR_DEBUG                         (F_GREEN B_NULL S_NORMAL)
#define ELOG_COLOR_VERBOSE                       (F_BLUE B_NULL S_NORMAL)

/*---------------------------------------------------------------------------*/
/* 日志格式化输出选项 */

/* 输出函数名：关闭（每个 log 调用会塞 __FUNCTION__ 字符串到 ROM）*/
/* #define ELOG_FMT_USING_FUNC */

/* 输出文件路径：关闭（__FILE__ 字符串太长，省 ROM）*/
/* #define ELOG_FMT_USING_DIR */

/* 输出行号：由 bl_features.h 的 BL_LOG_LINE 控制 */
#ifndef ELOG_FMT_USING_LINE
/* #define ELOG_FMT_USING_LINE */   /* 默认不定义，由 bl_features.h 控制 */
#endif

/*---------------------------------------------------------------------------*/
/* 异步输出模式 - 裸机不需要，全部关闭 */

/* #define ELOG_ASYNC_OUTPUT_ENABLE */
/* #define ELOG_ASYNC_OUTPUT_LVL */
/* #define ELOG_ASYNC_OUTPUT_BUF_SIZE */
/* #define ELOG_ASYNC_LINE_OUTPUT */
/* #define ELOG_ASYNC_OUTPUT_USING_PTHREAD */  /* 裸机无 POSIX */

/*---------------------------------------------------------------------------*/
/* 缓冲输出模式 - 裸机不需要，关闭 */

/* #define ELOG_BUF_OUTPUT_ENABLE */
/* #define ELOG_BUF_OUTPUT_BUF_SIZE */

#endif /* _ELOG_CFG_H_ */
