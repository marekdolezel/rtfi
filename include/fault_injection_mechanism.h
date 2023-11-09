/**
 * @file fault_injection_mechanism.h
 * @author Marek Dolezel (marekdolezel@gmail.com)
 * @brief 
 * @version 0.1
 * @date 2018-11-26
 * 
 * @copyright Copyright (c) 2018
 * 
 */

#ifndef FAULT_INJECTOR_H
#define FAULT_INJECTOR_H

#include <stdint.h>
#include <stdio.h>
#include "storage.h"

/* Fault types */
typedef enum {
    INJ_TYPE_BITFLIP_TRANSIENT,
    // INJ_TYPE_SET_STUCKAT_ZERO_PERMANENT,
    // INJ_TYPE_SET_STUCKAT_ONE_PERMANENT,
    // INJ_TYPE_SET_STUCKAT_ZERO_TRANSIENT,
    // INJ_TYPE_SET_STUCKAT_ONE_TRANSIENT
} injection_type_t;

#define INJ_TYPE_END INJ_TYPE_BITFLIP_TRANSIENT
#define INJ_TYPE_COUNT INJ_TYPE_END + 1

typedef enum {
    INJ_DEV_MEM,
    INJ_DEV_REG

} injection_device;

struct fim_injection {
    injection_type_t injection_type;
    injection_device device;

    uint32_t mem_addr;
    uint32_t mem_length;
    
    uint8_t reg_id;

    uint8_t bit_idx;

    bool prev_bit_value;
    bool prev_bit_value_isset;
    
    uint8_t thread;

    int fault_injection_t_us; /*< Time of injection */
    int fault_duration_t_us; /*< Duration time, only for transient fault */
};

uint32_t fim_inject(struct fim_injection *injection, bool is_reinjection);


/* Platform and protocol related structures */

#define PROTO_NUMOF_PROTO 1

typedef enum {
    PROTO_RSP,
} protocol_t;

typedef enum {
    CPU_NIOSII,
} cpu_t;

typedef enum {
    BOARD_DE0NANO,
} board_t;

struct reg {
    uint32_t id;
    char *name;
    uint32_t size;
    char *altname;

    void *content;
};

struct cpu {
    cpu_t type;

    struct reg reg[32];
    uint8_t reg_count;
};

struct mem {
    uint32_t size;
    uint32_t lowest_addr;
    uint32_t highest_addr;
};

struct platform_info {
    endianness_t endianness;
    protocol_t protocol_type;
    board_t board;
    struct cpu cpu;
    struct mem mem;
};

struct fim_configuration {
    FILE *pt_cfg;
    uint32_t pt_cfg_size;

    uint32_t host;
    uint16_t port;
};

uint32_t fim_platform_watchpoint_trigger();
uint32_t fim_platform_watchpoint(bool set, uint32_t mem_addr, uint32_t mem_length);
uint32_t fim_platform_execute();
uint32_t fim_platform_stop_execution();
int fim_platform_get_object(uint32_t mem_addr, uint32_t mem_length, struct storage *st);
int fim_platform_get_register(uint8_t reg_id, struct storage *st);
uint32_t fim_init(struct fim_configuration fi_cfg);
uint32_t fim_close(void);

uint32_t fim_platforminfo_sizeof_mem(void);
uint32_t fim_platforminfo_numof_regs(void);
uint32_t fim_plaforminfo_sizeof_reg(uint32_t id);
uint32_t fim_platforminfo_endianness(void);


#endif