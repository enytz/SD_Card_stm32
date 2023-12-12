#ifndef SD_H_F405 
#define SD_H_F405

#include <stdint.h>

//#define SD_SIZE                     (uint32_t)(7740588032)
//#define SD_NUMBERS_PAGE             (uint32_t)(30236672)
#define SD_NUMBERS_PAGE             (uint32_t)(15138816)        // ?? непонятно, на этот сектор тоже уже ругается.
#define START_ADDRESS_CRIT_DATA     (uint32_t)(10000001)
#define START_ADDRESS_LOG_DATA      (uint32_t)(1)
#define END_ADDRESS_CRIT_DATA       (uint32_t)(15000000)
#define END_ADDRESS_LOG_DATA        (uint32_t)(9999999)
#define ADDRESS_INFO_DATA           (uint32_t)(0)
#define ADDRESS_INFO_DATA_RES       (uint32_t)(10000000)        // предполагается переписывать эту страницу раз в 10 записей 0 для резервирования

typedef struct
{
    uint32_t currentPageAddrLogData;
    uint32_t currentPageAddrCritData;
    uint32_t oldPageAddrLogData;
    uint32_t oldAddrPageCritData;
    uint8_t backUpIsAvaible;
}dataAboutCardData;

typedef enum
{
    SD_IDLE,
    SD_READY,
    SD_IDENT,
    SD_STBY,
    SD_TRAN,
    SD_DATA,
    SD_RCV,
    SD_PRG,
    SD_DIS
}CURRENT_STATE_SD;

#include "stm32f4xx_ll_sdmmc.h"
#include "stm32f4xx_ll_bus.h"
#include "data.h"
#include <string.h>

/**
  * @brief Init SDIO interface
  * @retval 0 - ok, other value - failure
  */
uint8_t SDInitPeriph();

/**
  * @brief Init SD Card, taking RCA
  * @retval 0 - ok, other value - failure
  */
uint32_t SDInitCard();

/**
  * @brief Write in SD 512 bytes
  * @param TxData pointer on massive with uint8_t data
  * @param writeAddr page address in SD card 1 - 9999999
  * @retval 0 - ok, other value - failure
  */
uint32_t SDTransferData(uint8_t* TxData, uint32_t writeAddr);

/**
  * @brief Read from SD 512 bytes
  * @param RxData pointer on massive with uint8_t data
  * @param readAddr page address in SD card 1 - 9999999
  * @retval 0 - ok, other value - failure
  */
uint32_t SDReceiveData(uint8_t* RxData, uint32_t readAddr);

/**
  * @brief Write in SD logging data (4608 bytes)
  * @param logData pointer on massive with logging data
  * @param writeAddr pointer on page address in SD card 1 - 9999999 (use currentPageAddrLogData in struct dataAboutCardData)
  * @retval 0 - ok, other value - failure
  */
uint32_t SDWriteLogData(DataForLogging* logData, uint32_t* writeAddr);

/**
  * @brief Read from SD logging data (4608 bytes)
  * @param RxLogData pointer on massive with logging data
  * @param readAddr pointer on page address in SD card 1 - 9999999 (use currentPageAddrLogData in struct dataAboutCardData or START_ADDRESS_LOG_DATA)
  * @retval 0 - ok, other value - failure
  */
uint32_t SDReadLogData(DataForLogging* RxLogData, uint32_t readAddr);

/**
  * @brief Write in SD critical data
  * @param critData pointer on massive with critical data
  * @param writeAddr pointer on page address in SD card 10000001 - 15000000 (use currentPageAddrCritData in struct dataAboutCardData)
  * @retval 0 - ok, other value - failure
  */
uint32_t SDWriteCritData(DataForBackup* critData, uint32_t* writeAddr);

/**
  * @brief Read from SD critical data
  * @param RxCritData pointer on massive with critical data
  * @param readAddr pointer on page address in SD card 10000001 - 15000000 (use currentPageAddrCritData in struct dataAboutCardData)
  * @retval 0 - ok, other value - failure
  */
uint32_t SDReadCritData(DataForBackup* RxCritData, uint32_t readAddr);

/**
  * @brief Read from SD system data, data saved in 0 page in SD
  * @note Data consist of the current page address logging data, critical data, old page address logging and critical data 
  * for check do need to rewrite page
  * @param data pointer on massive with critical data
  * @retval 0 - ok, other value - failure
  */
uint32_t SDReadSystemData(dataAboutCardData* data);

/**
  * @brief Write in SD system data, data saved in 0 page in SD
  * @note Data consist of the current page address logging data, critical data, old page address logging and critical data 
  * for check do need to rewrite page
  * @param data pointer on massive with critical data
  * @retval 0 - ok, other value - failure
  */
uint32_t SDWriteSystemData(dataAboutCardData* data);
#endif  //SD_H_F405