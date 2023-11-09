#ifndef __PROTOCOL_H
#define __PROTOCOL_H
#include <stdint.h>

/** 
 * FI Commands private to fault injector. 
 * These commands are triggered by FI actions (flip bit, set bit to zero(one), set to always zero(one)). Actions are taken by faultManager.
 * FI Commands are converted to backend commands. Each backend has its own set of backend commands. 
 */
typedef enum {
    PROTO_OK,
    PROTO_ERR,
    PROTO_GET_REG,
    PROTO_SET_REG,
    PROTO_GET_MEM,
    PROTO_SET_MEM,
    PROTO_SET_BREAKPOINT,
    PROTO_REMOVE_BREAKPOINT,
    PROTO_SET_W_WATCHPOINT,
    PROTO_REMOVE_W_WATCHPOINT,
    PROTO_EXECUTE,
    PROTO_STOP_EXEC

} proto_cmd_type;

typedef struct proto_command {
    proto_cmd_type type;
    uint32_t mem_addr;
    uint32_t length;
    uint8_t thread;
    uint8_t reg_id;
    struct storage *st;
} proto_cmd;

#endif /* __PROTOCOL_H */