/*
 * module.h
 *
 *  Created on: 2016-4-21
 *      Author: lzh
 */

#ifndef MODULE_H_
#define MODULE_H_

/**
 * Definition of module IDs are defined here
 */


typedef enum _MODULE_ID {
    SNORT_MODULE_ID     = 0x1,
    BACKUP_MODULE_ID,
    BARNYARD_MODULE_ID,
    MGMT_ID,
    NO_MODULE
} ModuleID;
//----End of ID definitions----

/**
 * Definition of message operations go here
 */
//--- module operation ---
typedef enum _ModuleOperation {

    //SET primitives
    SET_SNORT_MONITOR_IPV4_ADDRESS_LIST = 0x0,
    SET_SNORT_MONITOR_PORT_LIST = 0X1,
    SET_SNORT_MONITOR_PROTOCOL_LIST = 0x3,

    //GET primitives
    GET_SNORT_MONITOR_IPV4_ADDRESS_LIST_STATISTICS = 0x4,
    GET_SNORT_MONITOR_PORT_LIST_STATISTICS  = 0x5,
    GET_SNORT_MONITOR_PROTOCOL_LIST_STATISTICS = 0X6,

    //don't take any operation
    NO_FURTHER_OPERATION
}ModuleOperation;

#endif /* MODULE_H_ */
