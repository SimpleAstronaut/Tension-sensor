/**
 * @file    modbus_crc.c
 * @brief   Modbus CRC16 校验计算算法实现
 * @version 0.1
 * @date    2026-01-26
 * @author  SimpleAstronaut
 * @copyright (C) Copyright 2025, China, SimpleAstronaut. All rights reserved.
 */

#include "modbus_crc.h"

/**
 * @brief  计算 Modbus RTU 协议的 CRC16 校验码
 * @note   使用查表法或直接计算法 (当前为直接计算法)
 *         多项式: 0xA001, 初始值: 0xFFFF
 * @param  buffer  指向要计算的数据缓冲区的指针
 * @param  len     数据的字节长度
 * @return uint16_t 计算得到的 16 位 CRC 校验码
 */
uint16_t Modbus_CRC16(uint8_t *buffer, uint16_t len) {
    uint16_t crc = 0xFFFF; // Modbus CRC 初始值

    for (uint16_t pos = 0; pos < len; pos++) {
        crc ^= (uint16_t)buffer[pos]; // 将当前数据字节与 CRC 寄存器的低字节进行异或

        for (int i = 8; i != 0; i--) { // 对每一位进行移位处理
            if ((crc & 0x0001) != 0) {
                // 如果最低位 (LSB) 为 1，则右移并异或多项式 0xA001
                crc >>= 1;
                crc ^= 0xA001;
            } else {
                // 如果最低位为 0，则仅右移
                crc >>= 1;
            }
        }
    }

    // 返回最终的 CRC 值
    // 注意：Modbus 协议在传输时通常要求低字节在前 (Little Endian)，
    // 此处返回的是 MCU 的原生 16 位值，发送时需根据需要拆分高低字节。
    return crc;
}
