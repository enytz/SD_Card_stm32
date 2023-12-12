#include "DMA_config.h"


uint8_t TxMas[128];
//uint32_t RxMas[32];

void DMA_configRCC()
{
    NVIC_EnableIRQ(DMA2_Stream3_IRQn);
    LL_AHB1_GRP1_EnableClock(LL_AHB1_GRP1_PERIPH_DMA2);

    if (LL_DMA_IsEnabledStream(DMA2,LL_DMA_STREAM_3))
        LL_DMA_DisableStream(DMA2,LL_DMA_STREAM_3);
	while(LL_DMA_IsEnabledStream(DMA2, LL_DMA_STREAM_3))
		;
    LL_DMA_EnableIT_TC(DMA2,LL_DMA_STREAM_3);
    LL_DMA_SetChannelSelection(DMA2,LL_DMA_STREAM_3,LL_DMA_CHANNEL_4);
}

void DMA_TxConfig()
{
    LL_DMA_ConfigTransfer(DMA2,LL_DMA_STREAM_3, LL_DMA_DIRECTION_MEMORY_TO_PERIPH |
																 LL_DMA_MODE_PFCTRL |
																 LL_DMA_MEMORY_INCREMENT |
																 LL_DMA_PERIPH_NOINCREMENT |
																 LL_DMA_PDATAALIGN_WORD |
																 LL_DMA_MDATAALIGN_BYTE);
    LL_DMA_SetPeriphAddress(DMA2,LL_DMA_STREAM_3,(uint32_t)&(SDIO->FIFO));
	LL_DMA_SetMemoryAddress(DMA2,LL_DMA_STREAM_3,(uint32_t)&TxMas[0]);
    LL_DMA_EnableFifoMode(DMA2,LL_DMA_STREAM_3);
    LL_DMA_SetFIFOThreshold(DMA2,LL_DMA_STREAM_3,LL_DMA_FIFOTHRESHOLD_1_4);                 // fifo состоит из 4 слов
    LL_DMA_SetMemoryBurstxfer(DMA2,LL_DMA_STREAM_3,LL_DMA_MBURST_INC4);
    LL_DMA_SetPeriphBurstxfer(DMA2,LL_DMA_STREAM_3,LL_DMA_PBURST_SINGLE);
    LL_DMA_SetDataLength(DMA2,LL_DMA_STREAM_3,512/4);                                         // передача размером байт

    LL_DMA_EnableStream(DMA2,LL_DMA_STREAM_3);
}

void DMA2_Stream3_IRQHandler()
{
    if (LL_DMA_IsEnabledIT_TC(DMA2,LL_DMA_STREAM_3))
        LL_DMA_ClearFlag_TC3(DMA2);
}
