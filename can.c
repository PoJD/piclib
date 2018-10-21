
#include <xc.h>
#include "utils.h"
#include "can.h"

/*
 * CAN ID and CAN DATA specific methods
 */

void can_headerToId(CanHeader *header, volatile byte *high, volatile byte *low) {
    // high byte is set to equal to message type - should be 3 bits only, so highest 5 bits will be 0 anyway
    // low byte = the node ID directly
    // high bits are set to message type on purpose to ease filtering - make sure the highest bits are filtered easily
    int canID = (header->messageType << 8) + header->nodeID;

    *high = (canID >> 3) & MAX_8_BITS; // take highest 8 bits as a byte
    // take 3 bits only and set as high bits inside low byte register 
    // this also sets EXIDEN bit to 0 to only accept standard IDs (no extended ones)
    *low = (canID & 0b111) << 5;
}

CanHeader can_idToHeader(volatile byte *high, volatile byte *low) {
    // shift hight byte to get the upper 8 bits, add the low byte and then shift it all to right
    // since we only have 11 bits of CAN ID in there (just highest 3 bits of low ID are the lowest 3 bits)
    int canID = ( (*high << 8) + *low ) >> 5;
    
    CanHeader header;
    header.messageType = canID >> 8; //high 8 bits is message type (should contain just 3 bits really)
    header.nodeID = canID & MAX_8_BITS; // low 8 bits is the node ID
    
    return header; // return by copy, not reference to a local variable
}

/* 
 * CAN DATA byte:
 * 
 * bits 8,7 = Operation
 * bit 6 = CAN error flag
 * bit 5,4 = firmware
 * bit 3,2,1 = switch counter (how many times was the switch already pressed)
 */

Operation can_extractOperationFromDataByte(volatile byte data) {
    return (data >> 6);
}

byte can_combineCanDataByte(Operation operation, volatile boolean canError, byte firmwareVersion, volatile byte switchCounter) {
    return ((operation&0b11) << 6) + ((canError&0b1) << 5) + ((firmwareVersion&0b11) << 3 ) + (switchCounter&0b111);
}


/**
 * Generic public CAN API
 */

/**
 * Default can init used all the time regardless the actual ports used for CANRX and CANTX
 */
void can_initDefault() {
    // also set the Enable Drive High bit, that should help with the stability
    CIOCONbits.ENDRHI = 1;
    
    messageStatus.statusCode = NOTHING_SENT;
    
    // when initiating again, reset this flag
    filterSetup = FALSE;
}

void can_init() {
    // TRISB2 = CAN BUS TX = has to be set as OUTPUT for CAN
    TRISBbits.TRISB2 = 0;
    // TRISB3 = CAN BUS RX = has to be set as INPUT for CAN
    TRISBbits.TRISB3 = 1;

    can_initDefault();
}

void can_initRcPortsForCan() {
    // CONFIG3H.CANMX = 0; should be set in config.h!
    // TRISC6 = CAN BUS TX = has to be set as OUTPUT for CAN
    TRISCbits.TRISC6 = 0;
    // TRISC7 = CAN BUS RX = has to be set as INPUT for CAN
    TRISCbits.TRISC7 = 1;

    can_initDefault();  
}

void can_setMode(volatile Mode mode) {
    if (CANSTATbits.OPMODE == mode) {
        // nothing to do here and also in some cases the below failed trying to "change" to the same mode
        // or not failed but was not done for some reason... Probably could be the abort trigger below...
        return;
    }
    
    // wait until we are in required mode (it may take a few cycles according to datasheet)
    // at times it also happen the ECAN module did not reply to that instruction, so rather trying in a loop
    // otherwise simple while waiting for the mode to be properly set sometime did run infinitely
    do {
        CANCONbits.REQOP = mode;
        for (int i=0; i<5; i++) {
            NOP();
        }
    } while (CANSTATbits.OPMODE != mode);
}

void can_setupBaudRate(volatile int baudRate, volatile int cpuSpeed) {
    // SJW to be 1
    // will use 1 TQ for SYNC, 4 for PROP SEG, 8 for phase 1 and 3 for phase 2 = 16TQ (recommendation in datasheeet to place sample point at 80% of bit time)
    // TBIT = 1000 / BAUD_RATE = 16TQ => TQ = 1000 / 16*(BAUD_RATE)
    // TQ (micros) = (2 * (BRP + 1))/CPU_SPEED (MHz) => BRP = (1000*CPU_SPEED)/(32*BAUD_RATE) -1, BRP = 9 = 0b1001 (for our speed and setting)

    // BRGCON1 = first 2 bits as 0 = SJW 1, last 6 bits are BRP, so mask first two bits of BRP (to make SJW 00)
    int BRP = (int)(1000*cpuSpeed)/(int)(32*baudRate) -1;
    BRGCON1 = BRP & 0b111111;
    
    // phase 2 segment programmable (1), sampled once (0), 3 bits for phase seg 1 (8=111), 3 bits for propagation time (4=011)
    BRGCON2 = 0b10111011;
    // enable can bus wake up (0), line filter not applied to this (0), 3 unused bits (0), phase segment 2 (3=010)
    BRGCON3 = 0b00000010;
}

void can_setupStrictReceiveFilter(CanHeader *header) {
    // setup just 1 acceptance filter to only accept CAN message for the in passed header information
    // so either set first or second acceptance filter (based on whether this method was already called or not)
    // in addition to setting it up, also enable it through the RXFCON0bits bits
    if (!filterSetup) {
        can_headerToId(header, &RXF0SIDH, &RXF0SIDL);
        RXFCON0bits.RXF0EN = 1;
    } else {
        can_headerToId(header, &RXF1SIDH, &RXF1SIDL);
        RXFCON0bits.RXF1EN = 1;
    }
    
    // remember to use the second filter next time
    filterSetup = TRUE;
    
    // now setup the strict mask -- all high 11 bits are the canID, rest is EXIDEN bits and others unused in legacy mode
    RXM0SIDH = 0b11111111;
    // first 3 bits finish the canID, the 5th bit set EXIDEN to 1 - use the same value of EXIDEN as in the corresponding filter
    RXM0SIDL = 0b11101000;
    
    // we really only want to receive this 1 strict message, which is unlikely to happen often
    // now enable the interrupts to receive CAN messages in buffer 0
    PIE5bits.RXB0IE = 1;
}

void can_setupFirstBitIdReceiveFilter(CanHeader *header) {
    can_setupStrictReceiveFilter(header);
    
    // the only difference is the mask - only mask first 4 bits of the canID (3 for message type and 1 for the first bit from nodeID)
    RXM0SIDH = 0b11110000;
    RXM0SIDL = 0b00001000;
}

void can_waitForPreviousSend() {   
    // confirm nothing is in the transmit register yet
    while (TXB0CONbits.TXREQ) {
        // check the register is not in error (previous send failed)
        if (TXB0CONbits.TXERR) {
            // abort any pending transmissions now, it should clear the below flag as a result too
            CANCONbits.ABAT = 1;
        }
    }
}

void can_send(CanMessage *canMessage) {
    messageStatus.statusCode = SENDING;
    
    //there could be previous message still in the queue
    can_waitForPreviousSend();
    
    // set up can ID appropriately
    can_headerToId(canMessage->header, &TXB0SIDH, &TXB0SIDL);

    // now take data length and all data too
    TXB0DLC = canMessage->dataLength;
    
    volatile byte* data = &TXB0D0;
    int i;
    for (i=0; i<canMessage->dataLength; i++, data++) {
        *data = canMessage->data[i];
    }
    
    // request transmitting the message (setting the special bit)
    TXB0CONbits.TXREQ = 1; 
}

void can_sendSynchronous(CanMessage *canMessage) {
    can_send(canMessage);
    can_waitForPreviousSend();
}
