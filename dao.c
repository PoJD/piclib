#include <xc.h>
#include "dao.h"

/**
 * Internal methods
 */


void dao_setupEepromAddress(unsigned int address) {
    // we have 10bits of data (1024 bytes)
    EEADRH = (address >> 8) & 0b11;
    EEADR = address & MAX_8_BITS;        
}

/**
 * Sets up eeprom read/write. It also sets the address to write/read the data to/from in EEPROM based on the bucket
 * 
 * @param address
 */
unsigned int dao_setupEeprom(unsigned int bucket) {
    // write to EEPROM, write only, rest are flags cleared by us this way
    EECON1 = 0;
    
    // translate from bucket to address, i.e. multiply by 2 as each bucket occupies 2 bytes of data
    unsigned int address = bucket << 1;
    dao_setupEepromAddress(address);
    return address;
}

/**
 * Writes the pre-prepared byte of data out to the storage and waits for the hardware to finish writing it
 */
void dao_writeByte() {
    // initiate write now
    EECON2 = 0x55;
    EECON2 = 0xAA;
    EECON1bits.WR = 1;

    // wait for the write to finish (this bit is cleared by HW)
    while(EECON1bits.WR);
    // clear the interrupt flag (not enabled handling it anyway, must be cleared according to datasheet)
    PIR4bits.EEIF = 0;
}

/**
 * Reads next byte from the storage assuming all the registers were setup properly in order to do so (address, etc)
 */
byte dao_readByte() {
    EECON1bits.RD = 1; // request read bit
    NOP(); // wait 1 cycle
    return EEDATA; // and now read the data
}

/*
 * Public API
 */

boolean dao_isValid (DataItem *dataItem) {
    return (dataItem->value != INVALID_VALUE);
}

void dao_saveDataItem (DataItem *dataItem) {
    // always disable interrupts during EEPROM write
    di();
    
    unsigned int address = dao_setupEeprom(dataItem->bucket);

    // now enable write to EEPROM
    EECON1bits.WREN = 1;
    
    // first write 8 higher bits of the value
    EEDATA = MAX_8_BITS & (dataItem->value >> 8);
    dao_writeByte();
    
    // now lower 8 bits of the value
    dao_setupEepromAddress(address+1);
    EEDATA = MAX_8_BITS & dataItem->value;
    dao_writeByte();

    // now disable write to EEPROM
    EECON1bits.WREN = 0;
    
    // now enable interrupts again
    ei();
}

DataItem dao_loadDataItem(unsigned int bucket) {
    DataItem result;
    result.bucket = bucket;
    
    // always disable interrupts during EEPROM read
    di();
    
    unsigned int address = dao_setupEeprom(bucket);
    
    // now read high 8 bits of data
    result.value = dao_readByte() << 8;
    
    // now move the address and initiate another read to get the 8 lower bits
    dao_setupEepromAddress(address+1);
    result.value += dao_readByte();
    
    // now enable interrupts again
    ei();

    return result; // return a copy of the instance, not a reference to this local instance
}
