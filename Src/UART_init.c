
#include "UART_init.h"



uint8_t volatile flagGetLogData =0;
uint8_t RxMasUART[20];
uint8_t cnt =0;

/**
 * размер выбран на основании размера 1 страницы считываемой памяти = 512 байт, избыточность кода base64 510/3*4 (избыточность)+
 * + 9 (<CR>)+ 4 (последние не кратные 3 байты, дополненные до 4 байт) = 680+9+4;
*/
uint8_t TxMasUART[693];


void UART_init(USART_TypeDef *USARTx)
{
    uint32_t clk = SystemCoreClock/2;
    if (USART3 == USARTx)
        LL_APB1_GRP1_EnableClock(LL_APB1_GRP1_PERIPH_USART3);
    else if(USART2 == USARTx)
        LL_APB1_GRP1_EnableClock(LL_APB1_GRP1_PERIPH_USART2);
    else if (USART1 == USARTx)
    {
        LL_APB2_GRP1_EnableClock(LL_APB2_GRP1_PERIPH_USART1);
        clk *=2;
    }
    

    LL_USART_SetBaudRate(USARTx,clk,LL_USART_OVERSAMPLING_16,115200);
    LL_USART_SetDataWidth(USARTx,LL_USART_DATAWIDTH_8B);
    LL_USART_SetStopBitsLength(USARTx,LL_USART_STOPBITS_1);
    LL_USART_SetParity(USARTx,LL_USART_PARITY_NONE);
    LL_USART_SetTransferDirection(USARTx,LL_USART_DIRECTION_TX_RX);
    LL_USART_SetHWFlowCtrl(USARTx,LL_USART_HWCONTROL_NONE);
    LL_USART_EnableDMAReq_TX(USARTx);
    LL_USART_EnableIT_RXNE(USARTx);

    LL_USART_Enable(USARTx);
}

void USART1_IRQHandler()
{
    if (LL_USART_IsActiveFlag_ORE(USART1))
        LL_USART_ClearFlag_ORE(USART1);
    if (LL_USART_IsActiveFlag_RXNE(USART1))
    {
        RxMasUART[cnt] = LL_USART_ReceiveData8(USART1);
        if (cnt >= 19)
        {
            cnt =0;
            return;
        }
        if (RxMasUART[cnt] == '\n' && RxMasUART[0] == 'g' && RxMasUART[1] == 'e' && RxMasUART[2] == 't')     // неизвестен запрос когда отдавать
        {
            cnt =0;
            flagGetLogData = 1;
            return;
        }
        cnt++;
    }
}

void DMA_init(DMA_TypeDef *DMAx)
{
    if (DMA1 == DMAx)
        LL_AHB1_GRP1_EnableClock(LL_AHB1_GRP1_PERIPH_DMA1);
    else
        LL_AHB1_GRP1_EnableClock(LL_AHB1_GRP1_PERIPH_DMA2);

    LL_DMA_SetPeriphAddress(DMAx,LL_DMA_STREAM_7,(uint32_t)&(USART1->DR));
    LL_DMA_SetMemoryAddress(DMAx,LL_DMA_STREAM_7,(uint32_t)&TxMasUART);
    LL_DMA_ConfigTransfer(DMAx,LL_DMA_STREAM_7,LL_DMA_DIRECTION_MEMORY_TO_PERIPH |
                                                LL_DMA_MODE_NORMAL |
                                                LL_DMA_PERIPH_NOINCREMENT | LL_DMA_MEMORY_INCREMENT |
                                                LL_DMA_PDATAALIGN_BYTE | LL_DMA_MDATAALIGN_BYTE);
    LL_DMA_ConfigFifo(DMAx,LL_DMA_STREAM_7,LL_DMA_FIFOMODE_DISABLE,LL_DMA_FIFOTHRESHOLD_1_2);
    LL_DMA_SetChannelSelection(DMAx,LL_DMA_STREAM_7,LL_DMA_CHANNEL_4);
    LL_DMA_EnableIT_TC(DMAx,LL_DMA_STREAM_7);
    NVIC_EnableIRQ(DMA2_Stream7_IRQn);

}

void DMATtansferDataUART()
{
    LL_DMA_SetDataLength(DMA2,LL_DMA_STREAM_7,sizeof(TxMasUART));
    LL_DMA_EnableStream(DMA2,LL_DMA_STREAM_7);
}

void DMA2_Stream7_IRQHandler()
{
    if (LL_DMA_IsActiveFlag_TC7(DMA2))
        LL_DMA_ClearFlag_TC7(DMA2);
    
}


void getBase64Mas(uint8_t* RxMas, uint16_t size, uint8_t* ResultBase64Mas)
{
    uint8_t alfabet[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    uint16_t tempSize = size -size%3;
    uint16_t i=0;
    if (tempSize >0)
    {
        while(tempSize >0)
        {
            ResultBase64Mas[i++] = alfabet[*RxMas>>2];
            if (i%76 == 0)
                ResultBase64Mas[i++] = '\n';
            uint8_t tmpVal = *RxMas++;
            ResultBase64Mas[i++] = alfabet[(tmpVal<<4)& 0x30 | (*RxMas>>4)& 0xf];
            if (i%76 == 0)
                ResultBase64Mas[i++] = '\n';
            tmpVal = *RxMas++;
            ResultBase64Mas[i++] = alfabet[(tmpVal<<2)&0x3c | (*RxMas>>6)& 0x3];
            if (i%76 == 0)
                ResultBase64Mas[i++] = '\n';
            ResultBase64Mas[i++] = alfabet[(*RxMas)& 0x3f];
            if (i%76 == 0)
                ResultBase64Mas[i++] = '\n';
            tempSize -=3;
            *RxMas++;
        }
    }
    if (size%3 == 2)
    {
        ResultBase64Mas[i++] = alfabet[(*RxMas>>2)& 0x3f];
        uint8_t tmpVal = *RxMas++;
        ResultBase64Mas[i++] = alfabet[(tmpVal<<4)& 0x30 | (*RxMas>>4)& 0xf];
        ResultBase64Mas[i++] = alfabet[(*RxMas<<2) & 0x3c];
        ResultBase64Mas[i++] = '=';
    }
    else if (size%3 == 1)
    {
        ResultBase64Mas[i++] = alfabet[(*RxMas>>2)& 0x3f];
        ResultBase64Mas[i++] = alfabet[(*RxMas<<4)& 0x30];
        ResultBase64Mas[i++] = '=';
        ResultBase64Mas[i++] = '=';
    }
}


void transferLogDataUART(DataForLogging* data)
{
    uint8_t* pData = (uint8_t*) data;
    for (int i=0;i<9;++i)
    {
        getBase64Mas((pData+512*i),512,TxMasUART);
        DMATtansferDataUART();
    }
}
