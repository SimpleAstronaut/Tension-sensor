/* bsq_sensor.h */
#ifndef __BSQ_SENSOR_H
#define __BSQ_SENSOR_H

#ifdef __cplusplus
extern "C" {
#endif

#include "bsp_rs485.h"
#include <stdbool.h>

// 寄存器地址定义 (基于PDF Page 10 & 14)
#define REG_MEASURE_VAL   0x0000 // 测量值 (40001)
#define REG_DECIMAL_PT    0x0001 // 小数点 (40002)
#define REG_TARE          0x0011 // 去皮 (40018)
#define REG_ZERO          0x0016 // 清零 (40023) // 注意PDF Page 12

typedef struct {
    float    weight;        // 最终重量 (浮点)
    int16_t  raw_value;     // 原始整数值
    uint8_t  decimal_point; // 小数点位数
    bool     comm_error;    // 通信错误标志
} BSQ_Data_t;

typedef struct {
    RS485_Handle_t rs485;
    uint8_t        slave_addr;
} BSQ_Device_t;

void BSQ_Init(BSQ_Device_t *dev, UART_HandleTypeDef *huart, uint8_t addr);
bool BSQ_ReadWeight(BSQ_Device_t *dev, BSQ_Data_t *data);
bool BSQ_Tare(BSQ_Device_t *dev);
bool BSQ_Zero(BSQ_Device_t *dev);

#ifdef __cplusplus
}
#endif

#endif
