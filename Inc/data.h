#ifndef DATA_H
#define DATA_H

#include <stdint.h>

#define NUM_CHUNKS 16
#define NUM_UNITS 128
#define DEFAULT_VALUE 0

/**
 * @brief Unit of data that must be logged.
 * 
 * These data units may alter continuousely during the work of MCU. Snapshots of this data must be taken
 * periodically and stored in a log file.
 * 
 */
typedef struct{
	uint32_t id;
	uint16_t data[NUM_CHUNKS];
}DataForLogging;

/**
 * @brief Critical data for being saved and restored from backup.
 * 
 */
typedef struct {
	int critical_field;
	int another_critical_field;
}DataForBackup;

/**
 * @brief Initialize all the loggable data units with default values.
 * 
 * @param data Array of data
 * @param num Length of the data array
 */
//void init_data(struct DataForLogging *data, int num);

/**
 * @brief Initialize the critical data.
 * 
 * This routine must be invoked iff there is no backup stored in the non-volatile memory.
 * 
 * @param data Pointer to the critical data
 */
//void init_backup(struct DataForBackup *data);

#endif // DATA_H
