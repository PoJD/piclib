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
 * Core Data Item structure - wraps some data item (i.e. type and value)
 */
typedef struct {
    unsigned int address; // address in EEPROM
    unsigned int value; // up to 16 bits to hold the data itself
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
 * Loads data item from storage for the given address
 * 
 * @param address address to retrieve data item from
 * @return new data item
 */
DataItem dao_loadDataItem(unsigned int address);

#ifdef	__cplusplus
}
#endif

#endif	/* DAO_H */

