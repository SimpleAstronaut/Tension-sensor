/**
 * @file    bsp_rs485.c
 * @brief   RS485 底层通信驱动
 * @version 0.1
 * @date    2026-01-25
 * @author  SimpleAstronaut
 * @copyright (C) Copyright 2025, China, SimpleAstronaut. All rights reserved.
 */

#include "bsp_rs485.h"
#include "cmsis_os.h"

/**
 * @brief  通过 RS485 发送数据
 * @note   当前实现使用了临界区保护发送过程
 * @param  h485  RS485 句柄
 * @param  data  发送数据缓冲区指针
 * @param  len   发送数据长度
 */
void RS485_Send(RS485_Handle_t *h485, uint8_t *data, uint16_t len) {
    // 进入临界区，防止发送过程被任务切换打断
    vPortEnterCritical();

    // 2. 发送数据 (阻塞模式，超时 100ms)
    HAL_UART_Transmit(h485->huart, data, len, 100);

    // 3. 等待发送完成 (非常重要！)
    // 注意：当前使用自动流控模块，无需手动切换 DE 引脚，故注释掉 TC 等待和 GPIO 操作
    // while (__HAL_UART_GET_FLAG(h485->huart, UART_FLAG_TC) == RESET);

    // 退出临界区
    vPortExitCritical();

    // 4. 切换回接收模式
    // HAL_GPIO_WritePin(h485->de_port, h485->de_pin, GPIO_PIN_RESET);
}

/**
 * @brief  从 RS485 接收数据
 * @note   使用 HAL 库阻塞接收，建议在超时设置合理的情况下在任务中调用
 * @param  h485     RS485 句柄
 * @param  data     接收数据缓冲区指针
 * @param  len      期望接收的数据长度
 * @param  timeout  接收超时时间 (单位: ms)
 * @return HAL_StatusTypeDef (HAL_OK 表示成功，HAL_TIMEOUT 表示超时)
 */
HAL_StatusTypeDef RS485_Receive(RS485_Handle_t *h485, uint8_t *data, uint16_t len, uint32_t timeout) {
    // 阻塞接收，在FreeRTOS任务中调用不会影响其他任务(前提是超时合理)
    return HAL_UART_Receive(h485->huart, data, len, timeout);
}