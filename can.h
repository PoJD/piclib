/* 
 * Main CAN interface. Provides methods for initializing the CAN protocol stack as well as sending CAN traffic over.
 * Implemented from PIC18F2XKXX datasheet, so could work for other chips too (assuming xc.h is included in your main file prior to this one)
 * 
 * File:   can.h
 * Author: pojd
 *
 * Created on October 31, 2015, 10:19 PM
 */

#ifndef CAN_H
#define	CAN_H

#ifdef	__cplusplus
extern "C" {
#endif
    
#include "utils.h"

/**
 * Type of CanMessage
 */
typedef enum {
    NORMAL        = 0, // normal message is a message sent by the node to reveal some action performed
    HEARTBEAT     = 1, // hearbeat message is also sent by this node, but only triggered by a timer
    CONFIG        = 2, // config would typically be sent by some master node in the network to setup this node (so this node would receive this message instead)
    COMPLEX       = 3, // complex message is allowing more complex commands to be transmitted (sort of extension of NORMAL)
    COMPLEX_REPLY = 4  // reply to a complex message
} MessageType;
    
/**
 * CanHeader structure - holds the header information used to wrap information needed to generate can IDs.
 */
typedef struct {
     /** the message ID - 8 bits only in this application, other 3 as part of the final can ID are used to encode other information  */
     byte nodeID;
          
     /** type of the message to be sent/received  */
     MessageType messageType;
} CanHeader;

/**
 * CanMessage structure - main structure for sending CAN traffic over
 */
typedef struct {
     CanHeader *header;

     /*
      * The below will become part of the data payload sent as part of this CAN message (custom protocol)
      */
     
     int dataLength;
     byte data[8];
 } CanMessage;
 
/**
 * Operations sent over CAN bus for a given node ID. Max 2 bits! Used to combine 1st byte in normal and heartbeat messages
 */
typedef enum {
    TOGGLE = 0b00,
    ON     = 0b01,
    OFF    = 0b10,
    GET    = 0b11  
} Operation;

/** Modes of the controller - see datasheet */ 
 typedef enum {
    CONFIG_MODE   = 0b100,
    LOOPBACK_MODE = 0b010,
    SLEEP_MODE    = 0b001,
    NORMAL_MODE   = 0b000
} Mode;

/**
 * Code of result of sending a CAN message
 */
typedef enum {
    OK           = 0, 
    ERROR        = 1,
    SENDING      = 2,
    NOTHING_SENT = 3
} MessageStatusCode;

/**
 * MessageStatus holds time and status of the last CAN send
 */
typedef struct {
     MessageStatusCode statusCode;
     int timestamp; // timestamp in whatever unit the target application wishes to use
 } MessageStatus;
 
/** was a CAN message send? The can util will update only the SENDING status when a can send is requested, 
 *  but the application code is required to keep it updated after a message is sent (both timestamp and status) */
MessageStatus messageStatus;


/**
 * Use the can header to setup the register for can ID (only low 11 bits should be set)
 * This would set the high and low registers as per the arguments
 * 
 * @param header can header to translate
 * @param high reference to the high 8 bits of the CAN ID
 * @param low reference to the low 8 bits of the CAN ID (just highest 3 bits would be used there)
 */
void can_headerToId(CanHeader *header, volatile byte *high, volatile byte *low);

/**
 * Read the respective can ID registers and translate that into can header
 * 
 * @param high reference to the high 8 bits of the CAN ID to read
 * @param low reference to the low 8 bits of the CAN ID to read (just highest 3 bits would be used there)
 * @return header translated can header
 */
CanHeader can_idToHeader(volatile byte *high, volatile byte *low);

/**
 * Extract operation from the in passed data byte received over CAN line
 * 
 * @param data data byte received over CAN to parse operation from
 * @return operation requested by this CAN message
 */
Operation can_extractOperationFromDataByte(volatile byte data);

/**
 * Combine a simple data byte from the in passed arguments
 * 
 * @param operation operation to trigger
 * @param canError flag whether can error was detected on the bus by the sender so far
 * @param firmwareVersion firmware version of the sender
 * @param switchCounter switch counter of the sender
 * @return combined data byte from the arguments to send over CAN
 */
byte can_combineCanDataByte(Operation operation, volatile boolean canError, byte firmwareVersion, volatile byte switchCounter);

/**
 * Sets up basic can settings (ports to starts with)
 */
void can_init();

/**
 * Sets up basic can settings (ports to starts with). RC ports are to be used for CANRX and CANTX
 */
void can_initRcPortsForCan();

/**
 * Sets operational mode of CAN chip
 * 
 * @param mode mode to set
 */
void can_setMode(volatile Mode mode);

/**
 * Sets up baud rate of CAN chip
 * 
 * @param baudRate the baud rate to use (in kbits per seond). Max is 500 (the implementation uses 16TQ for the bit sequencing)
 * @param cpuSpeed speed of the clock in MHz (mind that PLL settings in registers may affect this)
 */
void can_setupBaudRate(volatile int baudRate, volatile int cpuSpeed);

/** 
 * counter used to indicate if and how many filters has already been setup. If for example one already was, the second filter would be used
 * Max supported is 3 at the moment.
 */
byte filterCount = 0;

/**
 * Setup receive filter based on the in passed CanHeader - to receive only can messages for that header. This method will setup
 * a single mask only checking all bits to be equal.
 * 
 * @param header wrapper for the CanHeader - the filter will be setup to match the relevant can messages only
 */
void can_setupStrictReceiveFilter(CanHeader *header);
 
/**
 * Setup receive filter based on the in passed CanHeader - to receive only can messages for that header. This method will setup
 * a single mask only checking 1st bit of the nodeID and all bits of the message type to match
 * 
 * @param header wrapper for the CanHeader - the filter will be setup to match the relevant can messages only (using just the first bit from the nodeID)
 */
void can_setupFirstBitIdReceiveFilter(CanHeader *header);

/**
 * Attempts to send the message using TXB0 register. Does not wait for the send to finish, so leaves with the CAN module and transceiver to do their job
 * Max data payload to be sent is 8 bytes
 * 
 * @param canMessage: can message to be sent
 */
void can_send(CanMessage *canMessage);

/**
 * Attempts to send the message using TXB0 register. Does wait for the send to finish
 * Max data payload to be sent is 8 bytes
 * 
 * @param canMessage: can message to be sent
 */
void can_sendSynchronous(CanMessage *canMessage);

#ifdef	__cplusplus
}
#endif

#endif	/* CAN_H */

