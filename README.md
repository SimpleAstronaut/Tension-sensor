# Tension-sensor

拉力传感器搭配数字量变送器基于 RS485 转 TTL 模块的传感器驱动依赖库

## 项目简介

本项目为东莞金诺拉力传感器（www.jnsensor.com）配套驱动库，适用于 BSQ-DG-V2 数字量变送器，基于 **RS485 Modbus-RTU 协议**通信。采用 C 语言实现，适配 STM32 HAL 库，适用于嵌入式开发场景。

### 主要特性

- ✅ 支持 Modbus-RTU 协议（功能码 03/06）
- ✅ 实时读取拉力/重量数据
- ✅ 支持去皮（Tare）和清零（Zero）操作
- ✅ 自动 CRC16 校验
- ✅ 浮点数自动计算（根据小数点位数）
- ✅ 完整的错误处理机制

---

## 硬件连接

### 接线示意

```
拉力传感器 → 数字量变送器(BSQ-DG-V2) → RS485转TTL模块 → MCU(UART)
```

### 引脚连接

| RS485 模块 | MCU (STM32) |
|-----------|-------------|
| TX        | UART RX     |
| RX        | UART TX     |
| DE/RE     | GPIO (控制收发) |
| VCC       | 5V/3.3V     |
| GND       | GND         |

---

## 快速开始

### 1. 文件集成

将 `Bsq` 目录下的所有文件添加到您的工程：

```
Bsq/
├── bsq_sensor.c        # 传感器主驱动
├── bsq_sensor.h        # 传感器头文件
├── bsp_rs485.c         # RS485 底层驱动
├── bsp_rs485.h         # RS485 头文件
├── modbus_crc.c        # Modbus CRC16 校验
└── modbus_crc.h        # CRC 头文件
```

### 2. 初始化配置

```c
#include "bsq_sensor.h"

// 定义设备句柄和数据结构
BSQ_Device_t sensor;
BSQ_Data_t weight_data;

int main(void)
{
    // HAL 初始化
    HAL_Init();
    SystemClock_Config();
    
    // 初始化 UART (例如 USART1, 波特率 9600, 8N1)
    MX_USART1_UART_Init();
    
    // 初始化传感器 (从机地址默认为 0x01)
    BSQ_Init(&sensor, &huart1, 0x01);
    
    while(1)
    {
        // 读取重量数据
        if (BSQ_ReadWeight(&sensor, &weight_data)) {
            printf("Weight: %.3f kg\n", weight_data.weight);
        } else {
            printf("Read Error!\n");
        }
        
        HAL_Delay(1000);
    }
}
```

### 3. UART 配置要求

- **波特率**: 9600 bps（默认，可根据变送器配置调整）
- **数据位**: 8
- **校验位**: None
- **停止位**: 1

---

## API 文档

### 数据结构

#### `BSQ_Device_t` - 设备句柄

```c
typedef struct {
    RS485_Handle_t rs485;      // RS485 通信句柄
    uint8_t        slave_addr; // Modbus 从机地址
} BSQ_Device_t;
```

#### `BSQ_Data_t` - 传感器数据

```c
typedef struct {
    float    weight;        // 最终重量（浮点数，单位：kg）
    int16_t  raw_value;     // 原始整数值
    uint8_t  decimal_point; // 小数点位数 (0/1/2/3)
    bool     comm_error;    // 通信错误标志
} BSQ_Data_t;
```

#### `RS485_Handle_t` - RS485 句柄

```c
typedef struct {
    UART_HandleTypeDef *huart;    // UART 句柄
    GPIO_TypeDef       *de_port;  // DE 引脚端口
    uint16_t           de_pin;    // DE 引脚编号
} RS485_Handle_t;
```

---

### 核心函数

#### 1. `BSQ_Init` - 初始化传感器

```c
void BSQ_Init(BSQ_Device_t *dev, UART_HandleTypeDef *huart, uint8_t addr);
```

**参数说明**:
- `dev`: 指向 BSQ 设备结构体的指针
- `huart`: STM32 HAL UART 句柄指针（如 `&huart1`）
- `addr`: Modbus 从机地址（通常为 `0x01`）

**示例**:
```c
BSQ_Device_t sensor;
BSQ_Init(&sensor, &huart1, 0x01);
```

---

#### 2. `BSQ_ReadWeight` - 读取重量数据

```c
bool BSQ_ReadWeight(BSQ_Device_t *dev, BSQ_Data_t *data);
```

**参数说明**:
- `dev`: 设备句柄
- `data`: 输出参数，存储解析后的重量数据

**返回值**:
- `true`: 读取成功
- `false`: 通信失败

**功能说明**:
- 一次性读取**测量值**和**小数点位数**两个寄存器（保证数据同步）
- 自动计算浮点数重量值
- 支持 0-3 位小数点精度

**示例**:
```c
BSQ_Data_t data;
if (BSQ_ReadWeight(&sensor, &data)) {
    printf("Weight: %.3f kg (Raw: %d, Decimal: %d)\n", 
           data.weight, data.raw_value, data.decimal_point);
}
```

---

#### 3. `BSQ_Tare` - 去皮（相对清零）

```c
bool BSQ_Tare(BSQ_Device_t *dev);
```

**参数说明**:
- `dev`: 设备句柄

**返回值**:
- `true`: 去皮成功
- `false`: 操作失败

**功能说明**:
- 将当前重量设为零点（相对零点）
- 适用于去除容器重量等场景

**示例**:
```c
if (BSQ_Tare(&sensor)) {
    printf("Tare successful!\n");
}
```

---

#### 4. `BSQ_Zero` - 清零（绝对零点校准）

```c
bool BSQ_Zero(BSQ_Device_t *dev);
```

**参数说明**:
- `dev`: 设备句柄

**返回值**:
- `true`: 清零成功
- `false`: 操作失败

**功能说明**:
- 进行绝对零点校准
- **注意**: 执行此操作时请确保传感器无负载

**示例**:
```c
if (BSQ_Zero(&sensor)) {
    printf("Zero calibration successful!\n");
}
```

---

### RS485 底层函数

#### `RS485_Send` - 发送数据

```c
void RS485_Send(RS485_Handle_t *h485, uint8_t *data, uint16_t len);
```

#### `RS485_Receive` - 接收数据

```c
HAL_StatusTypeDef RS485_Receive(RS485_Handle_t *h485, uint8_t *data, 
                                uint16_t len, uint32_t timeout);
```

---

### Modbus CRC 工具函数

#### `Modbus_CRC16` - 计算 CRC16 校验

```c
uint16_t Modbus_CRC16(uint8_t *buffer, uint16_t len);
```

---

## 寄存器地址定义

| 寄存器名称 | 地址 (Hex) | Modbus 地址 | 说明 |
|-----------|-----------|------------|------|
| `REG_MEASURE_VAL` | 0x0000 | 40001 | 测量值（有符号整数） |
| `REG_DECIMAL_PT` | 0x0001 | 40002 | 小数点位数（0~3） |
| `REG_TARE` | 0x0011 | 40018 | 去皮寄存器 |
| `REG_ZERO` | 0x0016 | 40023 | 清零寄存器 |

---

## 常见问题

### 1. 通信失败/无数据��回

**可能原因**:
- 从机地址不匹配 → 检查变送器地址配置
- 波特率不一致 → 确认 UART 波特率与变送器一致（默认 9600）
- RS485 接线错误 → 检查 A/B 线是否接反
- DE/RE 控制引脚未配置 → 确保 `RS485_Handle_t` 中正确设置 GPIO

### 2. 数据读取不稳定

**解决方案**:
- 增加通信超时时间（默认 200ms）
- 检查电源供电是否稳定
- 添加延迟避免频繁读取

### 3. 小数点位数显示异常

**检查项**:
- 确认变送器小数点配置（通过设备说明书）
- 验证 `BSQ_ReadWeight` 返回的 `decimal_point` 值

---

## 版本信息

- **版本**: 0.1
- **日期**: 2026-01-25
- **作者**: SimpleAstronaut

---

## 开源协议

本项目采用 **GNU General Public License v3.0** 开源协议。

详见 [LICENSE](LICENSE) 文件。

---

## 技术支持

如有问题或建议，请提交 Issue 或 Pull Request。

传感器技术支持：[www.jnsensor.com](http://www.jnsensor.com)
