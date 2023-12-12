#ifndef UART_407_H
#define UART_407_H

#include "stm32f4xx_ll_bus.h"
#include "stm32f4xx_ll_usart.h"
#include "stm32f4xx_ll_dma.h"
#include "data.h"

void UART_init(USART_TypeDef *USARTx);

void DMA_init(DMA_TypeDef *DMAx);

void DMATtansferDataUART();

void getBase64Mas(uint8_t* RxMas, uint16_t size, uint8_t* ResultBase64Mas);

void transferLogDataUART(DataForLogging* data);
#endif //UART_407_H
