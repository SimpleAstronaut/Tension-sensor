/**
 * @file    bsq_sensor.c
 * @brief   拉力传感器通信驱动
 * @version 0.1
 * @date    2026-01-25
 * @author  SimpleAstronaut
 * @copyright (C) Copyright 2025, China, SimpleAstronaut. All rights reserved.
 */

// 使用传感器为东莞金诺拉力传感器 www.jnsensor.com
// 搭配同厂商生产的数字量变送器 BSQ-DG-V2，使用 RS485 Modbus-Rtu 协议通信。

#include "bsq_sensor.h"
#include <math.h>
#include "modbus_crc.h"

/**
 * @brief  初始化 BSQ 设备结构体
 * @param  dev   指向 BSQ 设备结构体的指针
 * @param  huart 指向 STM32 HAL UART 句柄的指针
 * @param  addr  传感器的 Modbus 从机地址
 */
void BSQ_Init(BSQ_Device_t *dev, UART_HandleTypeDef *huart, uint8_t addr) {
    dev->rs485.huart = huart;
    dev->slave_addr = addr;
}

/**
 * @brief  读取保持寄存器内部辅助函数 (功能码 03)
 * @param  dev        设备句柄
 * @param  start_reg  起始寄存器地址
 * @param  reg_count  读取的寄存器数量
 * @param  out_buf    输出缓冲区，存放读取到的数据部分 (不含头尾)
 * @return true: 成功, false: 失败
 */
static bool Modbus_ReadRegs(BSQ_Device_t *dev, uint16_t start_reg,
                            uint16_t reg_count, uint8_t *out_buf) {
    uint8_t tx_buf[8];
    // 预期最大回包长度通常较小，32 字节足够覆盖头部 + 少量寄存器数据 + CRC.
    uint8_t rx_buf[32];
    uint16_t crc;

    // 1. 组包 (Request Frame).
    tx_buf[0] = dev->slave_addr;
    tx_buf[1] = 0x03;                    // Function Code
    tx_buf[2] = (start_reg >> 8) & 0xFF; // Register High
    tx_buf[3] = start_reg & 0xFF;        // Register Low
    tx_buf[4] = (reg_count >> 8) & 0xFF; // Count High
    tx_buf[5] = reg_count & 0xFF;        // Count Low

    crc = Modbus_CRC16(tx_buf, 6);
    tx_buf[6] = crc & 0xFF;        // CRC Low
    tx_buf[7] = (crc >> 8) & 0xFF; // CRC High

    // 2. 发送请求.
    RS485_Send(&dev->rs485, tx_buf, 8);

    // 3. 接收响应.
    // 预期回包长度: Addr(1) + Func(1) + Bytes(1) + Data(reg_count*2) + CRC(2).
    uint16_t expected_len = 5 + (reg_count * 2);
    if (RS485_Receive(&dev->rs485, rx_buf, expected_len, 200) != HAL_OK) {
        return false;
    }

    // 4. 校验响应.
    // 检查地址和功能码.
    if (rx_buf[0] != dev->slave_addr || rx_buf[1] != 0x03) {
        return false;
    }

    // 检查 CRC.
    crc = Modbus_CRC16(rx_buf, expected_len - 2);
    uint16_t recv_crc =
        rx_buf[expected_len - 2] | (rx_buf[expected_len - 1] << 8);

    if (crc != recv_crc) {
        return false;
    }

    // 5. 提取数据.
    // rx_buf[2] 是字节数，数据从 rx_buf[3] 开始.
    for (int i = 0; i < rx_buf[2]; i++) {
        out_buf[i] = rx_buf[3 + i];
    }

    return true;
}

/**
 * @brief  写单个寄存器内部辅助函数 (功能码 06)
 * @param  dev       设备句柄
 * @param  reg_addr  寄存器地址
 * @param  val       写入的 16 位数值
 * @return true: 成功, false: 失败
 */
static bool Modbus_WriteSingle(BSQ_Device_t *dev, uint16_t reg_addr,
                               uint16_t val) {
    uint8_t tx_buf[8];
    uint8_t rx_buf[8];
    uint16_t crc;

    // 1. 组包 (Request Frame).
    tx_buf[0] = dev->slave_addr;
    tx_buf[1] = 0x06;                   // Function Code
    tx_buf[2] = (reg_addr >> 8) & 0xFF; // Register High
    tx_buf[3] = reg_addr & 0xFF;        // Register Low
    tx_buf[4] = (val >> 8) & 0xFF;      // Value High
    tx_buf[5] = val & 0xFF;             // Value Low

    crc = Modbus_CRC16(tx_buf, 6);
    tx_buf[6] = crc & 0xFF;
    tx_buf[7] = (crc >> 8) & 0xFF;

    // 2. 发送请求.
    RS485_Send(&dev->rs485, tx_buf, 8);

    // 3. 接收响应 (固定 8 字节).
    if (RS485_Receive(&dev->rs485, rx_buf, 8, 200) != HAL_OK) {
        return false;
    }

    // 4. 简单的回显校验 (功能码).
    if (rx_buf[1] != 0x06) {
        return false;
    }
    return true;
}

/**
 * @brief  读取传感器当前重量
 * @note   策略：一次性读取2个寄存器 (测量值 + 小数点)，保证同步性
 * @param  dev   设备句柄
 * @param  data  输出参数，用于存储解析后的重量数据
 * @return true: 成功, false: 失败
 */
bool BSQ_ReadWeight(BSQ_Device_t *dev, BSQ_Data_t *data) {
    uint8_t raw_bytes[4]; // 2 个寄存器 * 2 字节

    // 读取 0x0000 (测量值) 和 0x0001 (小数点).
    if (!Modbus_ReadRegs(dev, REG_MEASURE_VAL, 2, raw_bytes)) {
        data->comm_error = true;
        return false;
    }

    // 解析测量值 (Int16, Big Endian).
    // 参考 PDF 第 18 页，确认为有符号整型.
    data->raw_value = (int16_t)((raw_bytes[0] << 8) | raw_bytes[1]);

    // 解析小数点 (UInt16, Big Endian).
    // 参考 PDF 第 10 页: 0=个位, 1=十分位, 2=百分位, 3=千分位.
    uint16_t dp_reg = (raw_bytes[2] << 8) | raw_bytes[3];
    data->decimal_point = (uint8_t)dp_reg;

    // 计算浮点值.
    float divisor = 1.0f;
    switch (data->decimal_point) {
        case 1:
            divisor = 10.0f;
            break;
        case 2:
            divisor = 100.0f;
            break;
        case 3:
            divisor = 1000.0f;
            break;
        default:
            divisor = 1.0f;
    }

    data->weight = (float)data->raw_value / divisor;
    data->comm_error = false;
    return true;
}

/**
 * @brief  发送去皮命令 (相对清零)
 * @note   向寄存器 0x0011 (40018) 写入 0x0001
 * @param  dev   设备句柄
 * @return true: 成功, false: 失败
 */
bool BSQ_Tare(BSQ_Device_t *dev) {
    return Modbus_WriteSingle(dev, REG_TARE, 0x0001);
}

/**
 * @brief  发送清零命令 (绝对零点校准)
 * @note   向寄存器 0x0016 (40023) 写入 0x0011
 * @param  dev   设备句柄
 * @return true: 成功, false: 失败
 */
bool BSQ_Zero(BSQ_Device_t *dev) {
    return Modbus_WriteSingle(dev, REG_ZERO, 0x0011);
}