#include "SD.h"
#include "DMA_config.h"

SDIO_InitTypeDef strConf;
dataAboutCardData volatile InfoFromCard ={0,0,0};
uint32_t volatile RCA                   = 0;
uint8_t SDInitPeriph()
{
    LL_APB2_GRP1_EnableClock(LL_APB2_GRP1_PERIPH_SDIO);
    strConf.ClockEdge           = SDIO_CLOCK_EDGE_RISING;
    strConf.ClockBypass         = SDIO_CLOCK_BYPASS_DISABLE;
    strConf.ClockPowerSave      = SDIO_CLOCK_POWER_SAVE_DISABLE;
    strConf.BusWide             = SDIO_BUS_WIDE_1B;
    strConf.HardwareFlowControl = SDIO_HARDWARE_FLOW_CONTROL_DISABLE;
    strConf.ClockDiv            = SDIO_INIT_CLK_DIV;                    // для инициализации нужна низкая частота < 400 кГц
    SDIO_Init(SDIO,strConf);
    __SDIO_ENABLE();
    SDIO_PowerState_ON(SDIO);                                           // подача тактирования на карту
    return 0;
}

uint32_t SDInitCard()
{
    uint32_t volatile result =0;
    uint32_t volatile respData[4]={0,0,0,0};
    result = SDMMC_CmdGoIdleState(SDIO);                                // отправка cmd 0 - сброс всех карт на линии
    if (result !=0) return result;

    result = SDMMC_CmdOperCond(SDIO);                                   // отправка cmd 8 - проверка напряжения
    respData[0] = SDIO_GetResponse(SDIO,SDIO_RESP1);
    if (result != 0 || respData[0] !=0x1AA) return result;              // проверить в работе реальный ответ

    while(!(respData[0]>>31))                                           // проверяем, что флаг busy ушел и карта готова
    {
        result = SDMMC_CmdAppCommand(SDIO,0);                           // шлем 55 команду, говорим, что будет спец. команда
        if (result !=0) return result;
        result = SDMMC_CmdAppOperCommand(SDIO,1<<30);                   // в самой функции имеется маска для диапазона 3.2-3.3в и флаг busy (20 и 31 бит)
        respData[0] = SDIO_GetResponse(SDIO,SDIO_RESP1);
        for(int i=0; i<1000;++i)
        {__NOP();}
    }

    result = SDMMC_CmdSendCID(SDIO);                                    //  cmd 2 - запрос регистра идентификатора карты 
    if (result != 0) return result;
    respData[0] = SDIO_GetResponse(SDIO,SDIO_RESP4);
    respData[1] = SDIO_GetResponse(SDIO,SDIO_RESP3);
    respData[2] = SDIO_GetResponse(SDIO,SDIO_RESP2);
    respData[3] = SDIO_GetResponse(SDIO,SDIO_RESP1);


    result = SDMMC_CmdSetRelAdd(SDIO,&RCA);                             // отправка cmd 3 -  запрос RCA
    if (result != 0) return result;
    RCA <<=16; // здесь косяк с адресом, т.к. предыдущая команда отдает 2 байта, адрес же необходимо смещать до размера в 4 байта

    result = SDMMC_CmdSelDesel(SDIO,RCA);                               // отправка cmd  - 7, выбор карты с полученным RCA 
    // cmd 7 - выбор карты, переключение состояние standby/transfer или между "программирование"/"откл"
    if (result != 0) return result;
    respData[0] = SDIO_GetResponse(SDIO,SDIO_RESP1);                    // card status посмотреть, интересует curren_state 9:12

    //result = SDMMC_CmdAppCommand(SDIO,RCA);                             // шлем 55 команду, говорим, что будет спец. команда
    //    if (result !=0) return result;

    //result = SDMMC_CmdBusWidth(SDIO,SDIO_BUS_WIDE_4B);                  // отправка cm6 - повышаем разрядность шины данных до 4 бит
    //if (result !=0) return result;
    //respData[0] = SDIO_GetResponse(SDIO,SDIO_RESP1);

    //strConf.BusWide = SDIO_BUS_WIDE_4B;
    strConf.ClockDiv = 6;                                               // 48/(6+2) = 6 МГц, на 24 МГц в функциях с FIFO не успевал МК по скорости
    MODIFY_REG(SDIO->CLKCR,SDIO_CLKCR_CLKDIV,strConf.ClockDiv);         // повысили частоту clock до рабочей
    //MODIFY_REG(SDIO->CLKCR,SDIO_CLKCR_WIDBUS,strConf.BusWide);

    return 0;                                                           // успешная инициализация карты

}


uint32_t SDTransferData(uint8_t* TxData, uint32_t writeAddr)
{
    SDIO_DataInitTypeDef dataConf;
    dataConf.DataBlockSize      = SDIO_DATABLOCK_SIZE_512B;
    dataConf.DataLength         = 512;
    dataConf.DataTimeOut        = SDIO_STOPTRANSFERTIMEOUT;
    dataConf.TransferDir        = SDIO_TRANSFER_DIR_TO_CARD;
    dataConf.TransferMode       = SDIO_TRANSFER_MODE_BLOCK;
    dataConf.DPSM               = SDIO_DPSM_ENABLE;
    SDIO_ConfigData(SDIO,&dataConf);

    uint32_t pData;
    uint8_t* tempData = TxData;

    uint32_t volatile result =0;
    uint32_t volatile respData[4]={0,0,0,0};
    result          = SDMMC_CmdSendStatus(SDIO,RCA);
    if (result !=0) return result;
    respData[0]     = SDIO_GetResponse(SDIO,SDIO_RESP1);

    result          = SDMMC_CmdWriteSingleBlock(SDIO,writeAddr);
    if (result != 0) return result;
    respData[0]     = SDIO_GetResponse(SDIO,SDIO_RESP1);

    while(((respData[0]>>9)&0xf) != SD_RCV)
    {
        result      = SDMMC_CmdSendStatus(SDIO,RCA);
        if (result !=0) return result;
        respData[0] = SDIO_GetResponse(SDIO,SDIO_RESP1);
    }
    __SDIO_CLEAR_FLAG(SDIO,SDIO_STATIC_FLAGS);
    uint16_t dataRemaining = dataConf.DataLength;               // необходимо, для цикла ниже, т.к. flag SDIO_FLAG_DATAEND не всегда успевает 
    // подняться и может быть "заход" в необъявленную область памяти.
    while(!__SDIO_GET_FLAG(SDIO, SDIO_FLAG_TXUNDERR | SDIO_FLAG_DCRCFAIL | SDIO_FLAG_DTIMEOUT | SDIO_FLAG_DATAEND | SDIO_FLAG_STBITERR))
    {
        if (__SDIO_GET_FLAG(SDIO,SDIO_FLAG_TXFIFOHE) && dataRemaining>0)
        {
            for (int i=0;i<8;++i)
            {
                pData = (uint32_t)*tempData;
                tempData++;
                pData |= (uint32_t)(*tempData)<<8;
                tempData++;
                pData |= (uint32_t)(*tempData)<<16;
                tempData++;
                pData |= (uint32_t)(*tempData)<<24;
                tempData++;
                dataRemaining -=4;
                SDIO_WriteFIFO(SDIO,&pData);
            }
        }
    }
    while(((respData[0]>>9)&0xf) != SD_TRAN)
        {
            result      = SDMMC_CmdSendStatus(SDIO,RCA);
            if (result !=0) return result;
            respData[0] = SDIO_GetResponse(SDIO,SDIO_RESP1);
        }
    __SDIO_CLEAR_FLAG(SDIO,SDIO_STATIC_FLAGS);
    return 0;                                                           // succses
}

uint32_t SDReceiveData(uint8_t* RxData , uint32_t readAddr)
{
    SDIO_DataInitTypeDef dataConf;
    dataConf.DataBlockSize      = SDIO_DATABLOCK_SIZE_512B;
    dataConf.DataLength         = 512;                                   
    dataConf.DataTimeOut        = SDMMC_DATATIMEOUT;
    dataConf.TransferDir        = SDIO_TRANSFER_DIR_TO_SDIO;
    dataConf.TransferMode       = SDIO_TRANSFER_MODE_BLOCK;
    dataConf.DPSM               = SDIO_DPSM_ENABLE;
    uint32_t data =0;
    uint8_t* tempRx = RxData;
    SDIO_ConfigData(SDIO,&dataConf);

    uint32_t volatile result =0;
    uint32_t volatile respData[4]={0,0,0,0};
    result = SDMMC_CmdSendStatus(SDIO,RCA);
    if (result !=0) return result;
    respData[0] = SDIO_GetResponse(SDIO,SDIO_RESP1);
    while(((respData[0]>>9)&0xf) != SD_TRAN)
    {
        result = SDMMC_CmdSendStatus(SDIO,RCA);
        if (result !=0) return result;
        respData[0] = SDIO_GetResponse(SDIO,SDIO_RESP1);
    }
    result = SDMMC_CmdReadSingleBlock(SDIO,readAddr);
    if (result != 0) return result;
    respData[0] = SDIO_GetResponse(SDIO,SDIO_RESP1);

    __SDIO_CLEAR_FLAG(SDIO,SDIO_STATIC_FLAGS);

    while(((respData[0]>>9)&0xf) != SD_TRAN)
    {
        result = SDMMC_CmdSendStatus(SDIO,RCA);
        if (result !=0) return result;
        respData[0] = SDIO_GetResponse(SDIO,SDIO_RESP1);
    }
    uint16_t Cntdata = dataConf.DataLength;
    while(!__SDIO_GET_FLAG(SDIO, SDIO_FLAG_RXOVERR | SDIO_FLAG_DCRCFAIL | SDIO_FLAG_DTIMEOUT | SDIO_FLAG_DATAEND | SDIO_FLAG_STBITERR))
    {
        if (__SDIO_GET_FLAG(SDIO, SDIO_FLAG_RXFIFOHF) && (Cntdata >0))
        {
            for( int i =0;i<8;++i)
            {
                data = SDIO_ReadFIFO(SDIO);
                *tempRx = (uint8_t)(data & 0xff);
                tempRx++;
                Cntdata--;
                *tempRx = (uint8_t)((data>>8)& 0xff);
                tempRx++;
                Cntdata--;
                *tempRx = (uint8_t)((data>>16)& 0xff);
                tempRx++;
                Cntdata--;
                *tempRx = (uint8_t)((data>>24)& 0xff);
                tempRx++;
                Cntdata--;
            }
        }
    }
    if(__SDIO_GET_FLAG(SDIO,SDIO_FLAG_DTIMEOUT))
    {
        __SDIO_CLEAR_FLAG(SDIO,SDIO_FLAG_DTIMEOUT);
        return SDMMC_ERROR_DATA_TIMEOUT;
    }
    if(__SDIO_GET_FLAG(SDIO,SDIO_FLAG_DCRCFAIL))
    {
        __SDIO_CLEAR_FLAG(SDIO,SDIO_FLAG_DCRCFAIL);
        return SDMMC_ERROR_DATA_CRC_FAIL;
    }
    if(__SDIO_GET_FLAG(SDIO,SDIO_FLAG_RXOVERR))
    {
        __SDIO_CLEAR_FLAG(SDIO,SDIO_FLAG_RXOVERR);
        return SDMMC_ERROR_RX_OVERRUN;
    }
    while(Cntdata >0 && __SDIO_GET_FLAG(SDIO, SDIO_STA_RXDAVL))
    {
        data = SDIO_ReadFIFO(SDIO);
        *tempRx = (uint8_t)(data & 0xff);
        tempRx++;
        Cntdata--;
        *tempRx = (uint8_t)((data>>8)& 0xff);
        tempRx++;
        Cntdata--;
        *tempRx = (uint8_t)((data>>16)& 0xff);
        tempRx++;
        Cntdata--;
        *tempRx = (uint8_t)((data>>24)& 0xff);
        tempRx++;
        Cntdata--;
    }
    


    __SDIO_CLEAR_FLAG(SDIO,SDIO_STATIC_FLAGS);
    return 0;                                                           // succsess
}

uint32_t SDWriteLogData(DataForLogging* logData, uint32_t* writeAddr)
{
    uint8_t* data = (uint8_t*)logData;
    if (*writeAddr == 0 || *writeAddr >=(END_ADDRESS_LOG_DATA-9))        // чтобы не было переполнения при записи 9 страниц
        *writeAddr = START_ADDRESS_LOG_DATA;                                                 // запись будет осуществляться в 1 ячейку
    
    uint32_t result = 0;
    for (int i=0;i<9;++i)                                               // 9 - количество страниц, 4608/512
    {
    	uint32_t wrAddr = *writeAddr;
        result = SDTransferData(&(*(data+512*i)),wrAddr);
        if (result != 0)    return result;
        (*writeAddr)++;
    }
    return 0;
}

uint32_t SDReadLogData(DataForLogging* RxLogData,uint32_t readAddr)
{
    uint32_t result =0;
    uint8_t RxMas[9*512];                                               // размер структуры DataForLogging
    const uint8_t dataLogByteSize = 36;
    if (readAddr >= END_ADDRESS_LOG_DATA -9)
        readAddr = START_ADDRESS_LOG_DATA;
    for (int i=0;i<9;++i)
    {
    	uint8_t* rxAddr = (RxMas+512*i);
        result = SDReceiveData(rxAddr,readAddr+i);
        if (result != 0)    return result;
    }
    for (int i=0;i<NUM_UNITS;++i)
    {
        uint8_t* rxIDAddr = RxMas+i*dataLogByteSize;
        memcpy(&(RxLogData[i].id),rxIDAddr,sizeof(uint32_t));
        for (int j=0;j<NUM_CHUNKS;++j)
        {
            uint8_t* rxDataIAddr= RxMas+4+j*2+dataLogByteSize*i;
            memcpy(&(RxLogData[i].data[j]),rxDataIAddr,sizeof(uint16_t));
        }
    }
    return 0;
}

uint32_t SDWriteCritData(DataForBackup* critData, uint32_t* writeAddr)
{
    uint32_t result             = 0;
    uint8_t data[512];
    uint8_t* dataCr             = (uint8_t*) critData;
    for (int i=0;i<8;++i)
    {
        data[i] = dataCr[i];
    }
    if (*writeAddr ==0 || (*writeAddr< START_ADDRESS_CRIT_DATA || *writeAddr > END_ADDRESS_CRIT_DATA))
        *writeAddr = START_ADDRESS_CRIT_DATA;
    uint32_t wrAddr = *writeAddr;
    result = SDTransferData(data,wrAddr);
    if (result != 0)    return result;
    (*writeAddr)++;
    return 0;
}

uint32_t SDReadCritData(DataForBackup* RxCritData, uint32_t readAddr)
{
    uint32_t result =0;
    uint8_t RxMas[512];
    memset(RxMas,0x0,512);
    result = SDReceiveData(RxMas,readAddr);
    if (result != 0)    return result;
    uint8_t intSize = sizeof(int);

    memcpy(&(RxCritData->critical_field),RxMas,intSize);
    memcpy(&(RxCritData->another_critical_field),RxMas+intSize,intSize);
    return 0;
}

uint32_t SDReadSystemData(dataAboutCardData* data)
{
    uint32_t result =0;
    uint8_t RxMas[512];
    result = SDReceiveData(RxMas,0);
    if (result !=0) return result;
    uint8_t uintSize = sizeof(uint32_t);
    memcpy(&(data->currentPageAddrLogData),RxMas,uintSize);
    memcpy(&(data->currentPageAddrCritData),RxMas+uintSize,uintSize);
    memcpy(&(data->oldPageAddrLogData),RxMas+2*uintSize,uintSize);
    memcpy(&(data->oldAddrPageCritData),RxMas+3*uintSize,uintSize);
    memcpy(&(data->backUpIsAvaible),RxMas+4*uintSize,1);

    if (data->currentPageAddrLogData == 0 || data->currentPageAddrLogData> END_ADDRESS_LOG_DATA )
        data->currentPageAddrLogData = START_ADDRESS_LOG_DATA;
    if (data->currentPageAddrCritData == 0 || data->currentPageAddrCritData < START_ADDRESS_CRIT_DATA 
                                            || data->currentPageAddrCritData > END_ADDRESS_CRIT_DATA)
        data->currentPageAddrCritData = START_ADDRESS_CRIT_DATA;

    return 0;
}

uint32_t SDWriteSystemData(dataAboutCardData* data)
{
    uint32_t result =0;
    uint8_t mas[512];
    memset(mas,0x0,512);
    if (data->currentPageAddrCritData == data->oldAddrPageCritData && data->currentPageAddrLogData == data->oldPageAddrLogData)
        return 0;
    data->oldAddrPageCritData   = data->currentPageAddrCritData;        // чтобы лишний раз не перезаписывать, если данные не изменились
    data->oldPageAddrLogData    = data->currentPageAddrLogData;
    uint8_t* pData = (uint8_t*)data;
    for (int i=0;i<4*4+1;++i)
    {
        mas[i] = pData[i];
    }
    result = SDTransferData(mas,0);
    if (result !=0) return result;
    return 0;
}
