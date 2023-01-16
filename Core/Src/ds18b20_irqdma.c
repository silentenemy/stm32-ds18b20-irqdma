#include "ds18b20_irqdma.h"
#include <string.h>

typedef struct {
    uint8_t Reset;              //Communication Phase 1: Reset
    uint8_t ROM_Command;        //Communication Phase 2: Rom command
    uint8_t Function_Command;   //Communication Phase 3: DS18B20 function command
    uint8_t *ROM_TxBuffer;
    uint8_t *ROM_RxBuffer;
    uint8_t ROM_TxCount;
    uint8_t ROM_RxCount;
    uint8_t *Function_TxBuffer;
    uint8_t *Function_RxBuffer;
    uint8_t Function_TxCount;
    uint8_t Function_RxCount;
    uint8_t ROM;
    uint8_t Function;
} State;
State state;
uint8_t internal_Buffer[73];
UART_HandleTypeDef* huart_ow;

typedef struct {
		void(*OnComplete)(void);
		void(*OnErr)(void);
}OneWire_Callback;
OneWire_Callback onewire_callback;

void OneWire_SetCallback(void(*OnComplete)(void), void(*OnErr)(void))
{
	onewire_callback.OnErr = OnErr;
	onewire_callback.OnComplete = OnComplete;
}

void OneWire_Init(UART_HandleTypeDef *huart){
	OneWire_UARTInit(9600, huart);
}

// Declare a USART_HandleTypeDef handle structure.
void OneWire_UARTInit(uint32_t baudRate, UART_HandleTypeDef *huart){
//	huart->Instance = USART1;
	huart->Init.BaudRate = baudRate;
	huart->Instance->BRR = UART_BRR_SAMPLING16(HAL_RCC_GetPCLK2Freq(), huart->Init.BaudRate);
	huart->Init.WordLength = UART_WORDLENGTH_8B;
	huart->Init.StopBits = UART_STOPBITS_1;
	huart->Init.Parity = UART_PARITY_NONE;
	huart->Init.Mode = UART_MODE_TX_RX;
	huart->Init.HwFlowCtl = UART_HWCONTROL_NONE;
	huart->Init.OverSampling = UART_OVERSAMPLING_16;
    HAL_HalfDuplex_Init(huart);
    huart_ow = huart;
    return ;
}

void OneWire_TxCpltCallback(){
	//StateMachine();
}

void OneWire_RxCpltCallback(){
    StateMachine();
}

void StateMachine(){
    switch (state.Reset){
        case 0: // start the reset produce;
            OneWire_UARTInit(9600, huart_ow);
            internal_Buffer[0]=0xf0;
            HAL_UART_Transmit_DMA(huart_ow,&(internal_Buffer[0]),1);
            HAL_UART_Receive_DMA(huart_ow,&(internal_Buffer[0]),1);
            HAL_Delay(100);
            state.Reset++;
  	    break;
        case 1: // to check if the device exist or not.
        	if (internal_Buffer[0]==0xf0)
            {
	    		onewire_callback.OnErr();
	    		state.Reset = 0;
                break;
            }
            state.Reset++;
        case 2:
            if (ROMStateMachine()==0)
            	state.Reset++;
            else break;
        case 3:
            if (FunctionStateMachine()==0)
            	state.Reset++;
            else break;
        case 4:
        	onewire_callback.OnComplete();
        	state.Reset = 0;
	    break;
    }
    return ;
}

uint8_t ROMStateMachine(void){
    switch(state.ROM){
        case 0: // start the ROM command by sending the ROM_Command
            OneWire_UARTInit(115200, huart_ow);
            for (uint8_t i=0;i<8;i++)
                internal_Buffer[i]=((state.ROM_Command>>i)&0x01)?0xff:0x00;
            HAL_UART_Transmit_DMA(huart_ow,internal_Buffer,8);
            HAL_UART_Receive_DMA(huart_ow,&(internal_Buffer[0]),8);
            state.ROM++;
//            if (state.ROM_Command != 0xcc)
            break;
        case 1: // continue by sending necessary Tx buffer
            if (state.ROM_TxCount!=0){
                for (uint8_t i=0;i<state.ROM_TxCount;i++)
                    for (uint8_t j=0;j<8;j++)
                        internal_Buffer[i*8+j]=((state.ROM_TxBuffer[i]>>j)&0x01)?0xff:0x00;
                HAL_UART_Transmit_DMA(huart_ow,&(internal_Buffer[0]),state.ROM_TxCount*8);
                //HAL_UART_Receive_DMA(huart_ow,&(internal_Buffer[0]),state.ROM_TxCount*8);
                state.ROM++;
                break;
            }
            if (state.ROM_RxCount!=0){
                for (uint8_t i=0;i<=state.ROM_RxCount*8;i++)
                    internal_Buffer[i]=0xff;
                //HAL_UART_Transmit_DMA(huart_ow,&(internal_Buffer[0]),state.ROM_RxCount*8);
                HAL_UART_Receive_DMA(huart_ow,&(internal_Buffer[0]),state.ROM_RxCount*8);
                state.ROM++;
                break;
            }
	    state.ROM++;
        case 2:
            if (state.ROM_RxCount!=0){
        		for (uint8_t i=0;i<state.ROM_RxCount;i++)
	                	for (uint8_t j=0;j<8;j++)
	      	  	        state.ROM_RxBuffer[i]=state.ROM_RxBuffer[i]+
					(((internal_Buffer[i*8+j]==0xff)?0x01:0x00)<<j);
            }
            state.ROM=0;
            break;
        }
    return state.ROM;
}

uint8_t FunctionStateMachine(void){
    switch(state.Function){
        case 0:
            OneWire_UARTInit(115200, huart_ow);
            for (uint8_t i=0;i<8;i++)
                internal_Buffer[i]=((state.Function_Command>>i)&0x01)?0xff:0x00;
            HAL_UART_Transmit_DMA(huart_ow,&(internal_Buffer[0]),8);
            HAL_UART_Receive_DMA(huart_ow, &(internal_Buffer[0]),8);
            state.Function++;
//            if (state.Function_TxCount != 0)
            break;
        case 1: // continue by sending necessary Tx buffer
            if (state.Function_TxCount!=0){
                for (uint8_t i=0;i<state.Function_TxCount;i++)
                    for (uint8_t j=0;j<8;j++)
                        internal_Buffer[i*8+j]=((state.Function_TxBuffer[i]>>j)&0x01)?0xff:0x00;
                HAL_UART_Transmit_DMA(huart_ow,&(internal_Buffer[0]),state.Function_TxCount*8);
                //HAL_UART_Receive_DMA(huart_ow,&(internal_Buffer[0]),state.Function_TxCount*8);
                state.Function++;
                break;
            }
            if (state.Function_RxCount!=0){
                for (uint8_t i=0;i<=state.Function_RxCount*8;i++)
                    internal_Buffer[i]=0xff;
                //HAL_UART_Transmit_DMA(huart_ow,&(internal_Buffer[0]),8);//state.Function_RxCount*8);
                HAL_UART_Receive_DMA(huart_ow,&(internal_Buffer[0]),state.Function_RxCount*8);
                state.Function++;
                break;
            }
            state.Function++;
        case 2:
        	if (state.Function_RxCount!=0){
        		for (uint8_t i=0;i<state.Function_RxCount;i++)
        			for (uint8_t j=0;j<8;j++)
        				state.Function_RxBuffer[i]=state.Function_RxBuffer[i]+
							(((internal_Buffer[i*8+j]==0xff)?0x01:0x00)<<j);
        	}
            state.Function=0;
            break;
    	}
    return state.Function;
}

void OneWire_Execute(uint8_t ROM_Command,uint8_t* ROM_Buffer,
				uint8_t Function_Command,uint8_t* Function_buffer){
    memset(&(state),0,sizeof(State));
    state.ROM_Command=ROM_Command;
    state.Function_Command=Function_Command;
    switch (ROM_Command){
        case 0x33:  // Read ROM
            state.ROM_RxBuffer=ROM_Buffer;
            state.ROM_RxCount=8; //8 byte
            break;
        case 0x55:  // Match ROM
            state.ROM_TxBuffer=ROM_Buffer;
            state.ROM_TxCount=8;
            break;
        case 0xcc: break;  // Skip ROM
    }
    switch (Function_Command){
        case 0x44: break;
		// Convert T need to transmit nothing or we can read a 0
		// while the temperature is in progress read a 1 while the temperature is done.
        case 0x4e:  // Write Scratchpad
            state.Function_TxBuffer=Function_buffer;
            state.Function_TxCount=3;
            break;
        case 0x48: break; // Copy Scratchpad need to transmit nothing
        case 0xbe:  // Read Scratchpad
            state.Function_RxBuffer=Function_buffer;
            state.Function_RxCount=9;
            break;
        case 0xb8: break;
		// Recall EEPROM return transmit status to master 0 for in progress and 1 is for done.
        case 0xb4: break;
		// read power supply only work for undetermined power supply status. so don't need to implement it
    }
    StateMachine();
}

uint8_t OneWire_GetState() {
	return state.Reset;
}

float DS18B20_TempFloat(uint8_t scratchpad[]) {
	uint16_t tRegValue = (scratchpad[DS18B20_SCRATCHPAD_T_MSB_BYTE_IDX] << 8) | scratchpad[DS18B20_SCRATCHPAD_T_LSB_BYTE_IDX];
	uint16_t sign = tRegValue & DS18B20_SIGN_MASK;

	if (sign != 0)
	{
	  tRegValue = (0xFFFF - tRegValue + 1);
	}

   	tRegValue &= DS18B20_12_BITS_DATA_MASK;

	float temp = (float)tRegValue * DS18B20_T_STEP;

	if (sign != 0)
	{
		temp *= (-1);
	}

	return temp;
}
