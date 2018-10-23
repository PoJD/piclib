/* 
 * Wrapper for data storing. Always uses 2 bytes for storing or loading any data
 * 
 * File:   dao.h
 * Author: pojd
 *
 * Created on November 7, 2015, 11:48 AM
 */

#ifndef DAO_H
#define	DAO_H

#ifdef	__cplusplus
extern "C" {
#endif

#include "utils.h"

// not initialized EEPROM has all 1s by default, so invalid is the int set to 0xFFFF (2^16) since we always use 2 bytes of data for value
#define INVALID_VALUE MAX_16_BITS

/**
 * Core Data Item structure used as an API for the DAO (to store or retrieve data from underlying EEPROM)
 */
typedef struct {
    // bucket is used to pick address in EEPROM. Real address is calculated as "bucket << 1" as we are using 2 bytes for each bucket
    // max bucket should be 2^9 as EEPROM has 2^10 entries
    unsigned int bucket;
    unsigned int value; // 2 bytes to hold the data itself
} DataItem;

/**
 * Detects whether the in-passed data item is valid or not
 * 
 * @param dataItem data item to check
 * @return true if valid, false otherwise
 */
boolean dao_isValid (DataItem *dataItem);

/**
 * Save data item into the storage
 * 
 * @param dataItem data item to store
 */
void dao_saveDataItem (DataItem *dataItem);

/**
 * Loads data item from storage for the given bucket
 * 
 * @param bucket bucket number to retrieve data item from
 * @return new data item
 */
DataItem dao_loadDataItem(unsigned int bucket);

#ifdef	__cplusplus
}
#endif

#endif	/* DAO_H */

