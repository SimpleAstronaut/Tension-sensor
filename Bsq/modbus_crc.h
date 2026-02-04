/* modbus_crc.h */
#ifndef __MODBUS_CRC_H
#define __MODBUS_CRC_H
#include <stdint.h>
uint16_t Modbus_CRC16(uint8_t *buffer, uint16_t len);
#endif
