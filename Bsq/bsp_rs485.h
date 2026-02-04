/* bsp_rs485.h */
#ifndef __BSP_RS485_H
#define __BSP_RS485_H
#include "main.h"

// ¾ä±ú¶¨Òå
typedef struct {
    UART_HandleTypeDef *huart;
    GPIO_TypeDef       *de_port;
    uint16_t           de_pin;
} RS485_Handle_t;

void RS485_Send(RS485_Handle_t *h485, uint8_t *data, uint16_t len);
HAL_StatusTypeDef RS485_Receive(RS485_Handle_t *h485, uint8_t *data, uint16_t len, uint32_t timeout);
#endif
