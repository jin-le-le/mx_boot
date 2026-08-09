/**
 * @file port.h
 * @brief HC32F460 port common definitions
 */

#ifndef PORT_HC32F4_H
#define PORT_HC32F4_H

#include <stddef.h>
#include <stdint.h>
#include "hc32_ll.h"

/* HC32F460 Flash sector size: 8KB */
#define HC32F4_SECTOR_SIZE   (8 * 1024)

/* Console UART: USART1 @ PA10 (TX only) */
#define CONSOLE_USART_UNIT       (CM_USART1)
#define CONSOLE_USART_FCG        (FCG1_PERIPH_USART1)
#define CONSOLE_TX_PORT          (GPIO_PORT_A)
#define CONSOLE_TX_PIN           (GPIO_PIN_10)
#define CONSOLE_TX_FUNC          (GPIO_FUNC_32)

/* Protocol UART: USART4 @ PE6 (TX) / PB9 (RX) */
#define PROTO_USART_UNIT         (CM_USART4)
#define PROTO_USART_FCG          (FCG1_PERIPH_USART4)
#define PROTO_TX_PORT            (GPIO_PORT_E)
#define PROTO_TX_PIN             (GPIO_PIN_06)
#define PROTO_TX_FUNC            (GPIO_FUNC_36)
#define PROTO_RX_PORT            (GPIO_PORT_B)
#define PROTO_RX_PIN             (GPIO_PIN_09)
#define PROTO_RX_FUNC            (GPIO_FUNC_37)

/* LED: PA7 (PE6 is taken by USART4_TX) */
#define BL_LED_PORT              (GPIO_PORT_A)
#define BL_LED_PIN               (GPIO_PIN_07)

#endif /* PORT_HC32F4_H */
