#pragma once

#include "mbcontroller.h" 
// MB DEFINES
#define MB_SLAVE_ID (1)
#define MB_TCP_PORT_NUMBER      (CONFIG_FMB_TCP_PORT_DEFAULT)
#define MB_MDNS_PORT            (502)   

#define TAG "slave"

// Defines below are used to define register start address for each type of Modbus registers
#define MB_REG_INPUT_START_AREA0            0 // register offset input area 0
#define MB_REG_HOLDING_START_AREA0          0

#define MB_PAR_INFO_GET_TOUT                (10) // Timeout for get parameter info
#define MB_CHAN_DATA_MAX_VAL                (10)
#define MB_CHAN_DATA_OFFSET                 (1.1f)

#define MB_READ_MASK                        (MB_EVENT_INPUT_REG_RD \
                                                | MB_EVENT_HOLDING_REG_RD \
                                                | MB_EVENT_DISCRETE_RD \
                                                | MB_EVENT_COILS_RD)
#define MB_WRITE_MASK                       (MB_EVENT_HOLDING_REG_WR \
                                                | MB_EVENT_COILS_WR)
#define MB_READ_WRITE_MASK                  (MB_READ_MASK | MB_WRITE_MASK)

#define MB_SLAVE_ADDR (CONFIG_MB_SLAVE_ADDR)

//variables 
//Data Structures
extern mb_communication_info_t comm_info;

//Declarations
void slave_operation_func_logging();

//Slave Operation need to change
void slave_operation_func();


esp_err_t kill_slave();


esp_err_t slave_init(mb_communication_info_t* comm_info);



void start_slave();