#include "stm32f1xx_hal.h"

#ifndef __DS18B20_H__
#define __DS18B20_H__
void OneWire_Init(UART_HandleTypeDef *huart);
void OneWire_UARTInit(uint32_t baudRate, UART_HandleTypeDef *huart);
void OneWire_Execute(uint8_t ROM_Command,uint8_t* ROM_Buffer,
                     uint8_t Function_Command,uint8_t* Function_buffer);
void StateMachine(void);
void OneWire_SetCallback(void(*OnComplete)(void), void(*OnErr)(void));
uint8_t ROMStateMachine(void);
uint8_t FunctionStateMachine(void);
void OneWire_TxCpltCallback(void);
void OneWire_RxCpltCallback(void);
uint8_t OneWire_GetState(void);
float DS18B20_TempFloat(uint8_t scratchpad[]);

#define DS18B20_SCRATCHPAD_T_LSB_BYTE_IDX                                   0
#define DS18B20_SCRATCHPAD_T_MSB_BYTE_IDX                                   1

#define DS18B20_12_BITS_DATA_MASK                                           0x7FF

#define DS18B20_SIGN_MASK                                                   0xF800

#define DS18B20_T_STEP                                                      0.0625

#endif
