#include "Inc/RCC_config.h"
#include "Inc/GPIO_config.h"
#include "Inc/SD.h"
#include "Inc/data.h"
#include "Inc/UART_init.h"
#include "Inc/TIM_init.h"
#include <stdint.h>

extern volatile dataAboutCardData InfoFromCard;

extern volatile uint8_t flagGetLogData;

extern volatile uint8_t flagWriteLogDataInSD;

DataForLogging data[NUM_UNITS];
DataForLogging Rxdata[NUM_UNITS];
DataForBackup dataBackup;
DataForBackup RxdataBackup;

extern uint8_t TxMasUART[];         // массив для DMA

int main()
{
    RCC_config();
    GPIO_config();
    UART_init(USART1);
    DMA_init(DMA2);
    SDInitPeriph();
    uint32_t res = SDInitCard();
    if (res != 0)
    {
        // различные варианты: переинициализация, system_reset и т.п.
    }
    for (int i =0;i<128;++i)
    {
        data[i].id = i*3;
        for (int j =0;j<16;++j)
        {
            data[i].data[j] = j*i;
        }
    }
    //res = SDWriteSystemData(&InfoFromCard);
    res = SDReadSystemData(&InfoFromCard);
    dataBackup.another_critical_field   = 256;
    dataBackup.critical_field           = -734;
    //res = SDWriteLogData(data,&(InfoFromCard.currentPageAddrLogData));
    res = SDReadLogData(Rxdata,InfoFromCard.currentPageAddrLogData -9);
    for (int i=0;i<128;++i)
    {
        data[i].id = 1+i;
    }
    //res = SDWriteLogData(data,&(InfoFromCard.currentPageAddrLogData));
    res = SDReadLogData(Rxdata,InfoFromCard.currentPageAddrLogData-18);
    //res = SDWriteCritData(&dataBackup,&(InfoFromCard.currentPageAddrCritData));
    //res = SDReceiveData((uint8_t*)&RxdataBackup,wrAddrTest);
    res = SDReadCritData(&RxdataBackup,InfoFromCard.currentPageAddrCritData-2);
    //res = SDReadLogData(Rxdata,100);
    //res = SDWriteSystemData(&InfoFromCard);
    while(1)
    {
        if (flagGetLogData)
        {
            uint32_t addr = START_ADDRESS_LOG_DATA;
            while(addr != InfoFromCard.currentPageAddrLogData)
            {
                res = SDReadLogData(Rxdata,addr);
                if (res !=0 ) addr--;                           // здесь есть несколько вариантов в виде повторной отправки/переинициализации SD и др.
                addr++;
                transferLogDataUART(Rxdata);
            }
            flagGetLogData =0;
        }
        if (flagWriteLogDataInSD)
        {
            //запись данные в массив с данными для логирования
            //SDWriteLogData(data,InfoFromCard.currentPageAddrLogData);
        }
    }
}
