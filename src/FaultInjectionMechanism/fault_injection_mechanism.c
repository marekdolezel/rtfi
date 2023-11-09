/**
 * @fimle fault_injector.c
 * @author Marek Dolezel (marekdolezel@gmail.com)
 * @brief 
 * @version 0.1
 * @date 2018-11-22
 * 
 * @copyright Copyright (c) 2018
 * 
 */

#include <stdbool.h>

#include "fault_injection_mechanism.h"
#include "error_codes.h"
//#include "json.h"
//#include "json_process.h"
#include "debug.h"

/** 
 * Supported protocols: 
 * gdb rsp
 */
#include "protocol.h"
#include "rsp.h"

struct fim_struct {
    struct platform_info pt_info;
    struct rsp_connection conn;
    struct fim_injection *last_injection;
} fim_struct;

uint32_t
_fim_platforminfo(struct platform_info *pt_info_out, FILE *pt_cfg, uint32_t fimle_size)
{
    debugprintf_ff(4, "pt_info_out %p, pt_cfg %p, fimle_size %d \n", pt_info_out, pt_cfg, fimle_size);

    int ret_code = ERR_NONE;

    pt_info_out->board = BOARD_DE0NANO;
    pt_info_out->cpu.type = CPU_NIOSII;
    pt_info_out->protocol_type = PROTO_RSP;
    pt_info_out->mem.size = 32 * 1024 * 1024;
    pt_info_out->mem.lowest_addr = 0;
    pt_info_out->mem.highest_addr = 32 * 1024 * 1024 - 1;
    pt_info_out->cpu.reg_count = 32;
    pt_info_out->endianness = ST_LITTLE_ENDIAN;

    debugprintf_ff(4, "Highest memory address %x\n",pt_info_out->mem.highest_addr);
    debugprintf_ff(4, "Lowest memory address %x\n",pt_info_out->mem.lowest_addr);
    debugprintf_ff(4, "Mem size %d bytes, %d MB\n",pt_info_out->mem.size, pt_info_out->mem.size/1024/1024);
    debugprintf_ff(4, "Number of registers = %d\n", pt_info_out->cpu.reg_count);

    return ret_code;
}

uint32_t
fim_plaforminfo_sizeof_reg(uint32_t id)
{
   return 4;
}

uint32_t
fim_platforminfo_numof_regs()
{
    return fim_struct.pt_info.cpu.reg_count;
}

uint32_t
fim_platforminfo_sizeof_mem()
{
    return fim_struct.pt_info.mem.size;
}

uint32_t
fim_platforminfo_endianness()
{
    return fim_struct.pt_info.endianness;
}


uint32_t 
fim_init(struct fim_configuration fim_cfg)
{
    uint32_t    ret_code = ERR_NONE;
    protocol_t  protocol_type;

    ret_code = _fim_platforminfo(&fim_struct.pt_info, fim_cfg.pt_cfg, fim_cfg.pt_cfg_size);
    ON_ERROR_DEBUG_AND_RETURN_ECODE(ret_code, "_fim_platform");

    debugprintf_ff(4, "calling rsp_init(), port=%d \n",fim_cfg.port);
    ret_code = rsp_init(&fim_struct.conn, fim_cfg.host, fim_cfg.port);
    ON_ERROR_DEBUG_AND_RETURN_ECODE(ret_code, "rsp_init");

    return ret_code;
}

uint32_t 
fim_close()
{
    rsp_close(&fim_struct.conn);
}

int
_fim_platform_execute_command(struct proto_command *cmd, struct storage *s)
{
    int             ret_code = ERR_NONE;
    proto_cmd_type  cmd_response;

    if (cmd->type == PROTO_SET_MEM || cmd->type == PROTO_SET_REG)
        cmd->st = s;

    ret_code = rsp_send_command(&fim_struct.conn, cmd);
    ON_ERROR_DEBUG_AND_RETURN_ECODE(ret_code, "rsp_send_command");

    ret_code = rsp_recv_command(&fim_struct.conn, &cmd_response, s);  //TODO: invalid s->size !!!
    ON_ERROR_DEBUG_AND_RETURN_ECODE(ret_code, "rsp_recv_command");
}

uint32_t _fim_platform_execute_command_nopayload(proto_cmd_type cmd_type)
{
    struct proto_command    cmd;

    if (cmd_type != PROTO_EXECUTE && cmd_type != PROTO_STOP_EXEC)
        return -1;

    cmd.type = cmd_type;
    cmd.st = NULL; // hack

    return _fim_platform_execute_command(&cmd, NULL);
}

uint32_t
fim_platform_watchpoint_trigger() 
{
    struct proto_command    cmd;
    proto_cmd_type          cmd_response;

    cmd.st = NULL;

    return rsp_recv_command(&fim_struct.conn, &cmd_response, NULL);
}

uint32_t
fim_platform_watchpoint(bool set, uint32_t mem_addr, uint32_t mem_length)
{
    int                     ret_code = ERR_NONE;
    struct  proto_command   cmd;
    struct mem*             memory = &fim_struct.pt_info.mem;

    if (mem_addr > memory->highest_addr || mem_addr < memory->lowest_addr)
        return ERR_INVALID_MEM_ADDR;

    cmd.type = set ? PROTO_SET_W_WATCHPOINT : PROTO_REMOVE_W_WATCHPOINT;
    cmd.mem_addr = mem_addr;
    cmd.length = mem_length;
                
    debugprintf_ff(1, "Set/remove write watchpoint (set=%d)\n", set);

    ret_code = _fim_platform_execute_command(&cmd, NULL);
    ON_ERROR_DEBUG_AND_RETURN_ECODE(ret_code, "_fim_platform_execute_command");

}

uint32_t
fim_platform_execute()
{
    return _fim_platform_execute_command_nopayload(PROTO_EXECUTE);
}

uint32_t
fim_platform_stop_execution()
{
    return _fim_platform_execute_command_nopayload(PROTO_STOP_EXEC);
}

int
fim_platform_get_object(uint32_t mem_addr, uint32_t mem_length, struct storage *st)
{
    struct proto_command    cmd;

    cmd.type        = PROTO_GET_MEM;
    cmd.thread      = -1; /* Doesn't apply */
    cmd.mem_addr    = mem_addr;
    cmd.length      = mem_length;

    return _fim_platform_execute_command(&cmd, st);
}

int
fim_platform_get_register(uint8_t reg_id, struct storage *st)
{
    struct proto_command    cmd;

    cmd.type        = PROTO_GET_REG;
    cmd.thread      = -1; /* Doesn't apply */
    cmd.reg_id      = reg_id;

    return _fim_platform_execute_command(&cmd, st);
}
/*
    When we inject transient single bit flip fault we save the value of bit before the injection to 'prev_bit_value'.
    If the FaultManager decides it is time to reinject the fault,
    then we need to make sure that current value of the bit is equal to prev_bit_value, otherwise no injection should occur as the fault
    is still present.
*/

int 
_fim_inject_bit(uint8_t bit_idx, struct storage *storage, injection_type_t injection_type, bool *prev_bit_value, bool reinjection)
{
    bool nowbit_value;
    switch(injection_type) {
            case INJ_TYPE_BITFLIP_TRANSIENT:
                printf("Flipping bit at index %d\n", bit_idx);
                if (reinjection == false) {
                    storage_get_bit(storage, bit_idx, prev_bit_value);
                    storage_toggle_bit(storage, bit_idx);
                }
                else {
                    storage_get_bit(storage, bit_idx, &nowbit_value);
                    if (nowbit_value == *prev_bit_value)
                        storage_toggle_bit(storage, bit_idx);
                }
                
            break;
                
            // // case INJ_TYPE_SET_ZERO:
            // case INJ_TYPE_SET_STUCKAT_ZERO_TRANSIENT:
            // case INJ_TYPE_SET_STUCKAT_ZERO_PERMANENT:
            //     debugprintf_ff(1, "Setting bit %d to zero\n", bit_idx);
            //     storage_set_bit(storage, bit_idx, false); 
            // break;

            // // case INJ_TYPE_SET_ONE:
            // case INJ_TYPE_SET_STUCKAT_ONE_TRANSIENT:
            // case INJ_TYPE_SET_STUCKAT_ONE_PERMANENT:
            //     debugprintf_ff(1, "Setting bit %d to one\n", bit_idx);
            //     storage_set_bit(storage, bit_idx, true); 
            // break;
            
            default:
                printf("Unkown type of fault\n");
                return ERR_UNKNOWN_FAULT_TYPE;
            break;
    }   

    return ERR_NONE;
}

uint32_t _fim_compute_new_addr(uint32_t mem_addr, uint32_t mem_length, uint32_t *bit_idx_new, uint32_t bit_idx)
{
    /* which byte contains our bit? */
    uint32_t _byte_idx = 0;
    uint32_t _lbit_idx = 0;
    uint32_t _rbit_idx = 7;

    uint32_t new_mem_addr;

    while(_lbit_idx < bit_idx && _rbit_idx < bit_idx ) {
        debugprintf(1, "_lbitidx %d\n", _lbit_idx);
        _lbit_idx += 8;
        _rbit_idx += 8;
        _byte_idx++;
    }
    *bit_idx_new = bit_idx - _lbit_idx;

    // *bit_idx_new = bit_idx % 8;
    new_mem_addr = mem_addr + _byte_idx;
    // int new_mem_addr = mem_addr / 8;
    if (new_mem_addr >= mem_addr && new_mem_addr <= mem_addr + mem_length - 1 )
        debugprintf(1, "new_mem_addr is valid\n");
    else
        debugprintf(1, "error: new_mem_addr must be in range <mem_addr, mem_addr+mem_melngth-1>");

    return new_mem_addr;
}

uint32_t
fim_inject(struct fim_injection *injection, bool is_reinjection)
{
    int                     ret_code = ERR_NONE;

    struct proto_command    cmd;
    struct storage          storage;
    uint32_t                new_bit_idx = 0;

    if (injection == NULL) {
        debugprintf_ff_null(1, "struct fim_injection *injection\n");
        return ERR_NULL_POINTER;
    }

    fim_struct.last_injection = injection;

    if (injection->device != INJ_DEV_MEM && injection->device != INJ_DEV_REG) {
        debugprintf_ff(1, " faults can be injected only to registers or memory\n");
        return ERR_INVALID_DEV_TYPE;
    }

    /* Injecting memory */
    if (injection->device == INJ_DEV_MEM) {
        if (injection-> mem_addr > fim_struct.pt_info.mem.size) {
            debugprintf_ff(1, "Invalid memory address %x (highest mem_addr %x)\n", injection->mem_addr, fim_struct.pt_info.mem.size-1);
            return ERR_INVALID_MEM_ADDR;
        }
    

        /* Prepare storage for memory content */
        ret_code = storage_create(&storage, 1, fim_struct.pt_info.endianness);
        ON_ERROR_DEBUG_AND_RETURN_ECODE(ret_code, "storage_create");

        /* Get memory content (address, length) from target, we just get the whole structure or global variable */
        cmd.type = PROTO_GET_MEM;
        cmd.thread = -1; /* Doesn't apply */
        cmd.mem_addr = _fim_compute_new_addr(injection->mem_addr, injection->mem_length, &new_bit_idx, injection->bit_idx);
        cmd.length = 0x1;

        debugprintf_ff(1, "new_addr 0x%x, new_bit_idx %d\n", cmd.mem_addr, new_bit_idx);

        debugprintf_ff(1, "Fetching content of %d bytes from memory 0x%x", cmd.length, cmd.mem_addr);
        ret_code = _fim_platform_execute_command(&cmd, &storage);
        ON_ERROR_DEBUG_AND_GOTO_LABEL(ret_code, "_fim_platform_execute_command", error);
        debugprintf_ff(1, "fetched\n");
        debugprintf_ff(1, "Memory value is %ld (decimal)\n", storage_get_value(&storage));


        /* Inject fault */
        uint32_t bit_idx = new_bit_idx;
        _fim_inject_bit(bit_idx, &storage, injection->injection_type, &injection->prev_bit_value, is_reinjection); injection->prev_bit_value_isset = true;
        debugprintf_ff(1, "Memory value (with fault) is %ld(decimal)\n", storage_get_value(&storage));

        /* Store memory content back to target */
        cmd.type = PROTO_SET_MEM;

        debugprintf_ff(1, "Sending memory content to target .. ");
        ret_code = _fim_platform_execute_command(&cmd, &storage);
        ON_ERROR_DEBUG_AND_GOTO_LABEL(ret_code, "_fim_platform_execute_command", error);
        debugprintf_ff(1, "done\n");

        error:
        storage_destroy(&storage);
    }

    /* Inject register */
    if (injection->device == INJ_DEV_REG) {

        if (injection->reg_id >= fim_platforminfo_numof_regs() || injection->reg_id < 0 ) {
            debugprintf_ff(1, "Invalid register id(%d), valid registers are 0 to %d\n", injection->reg_id, fim_platforminfo_numof_regs()-1);
            return ERR_INVALID_REG_ID;
        }


        /* Prepare storage for register content */
        storage_create(&storage, 4, fim_struct.pt_info.endianness);


        /* Get that register */
        cmd.thread = injection->thread;
        cmd.reg_id = injection->reg_id;
        cmd.type = PROTO_GET_REG;

        debugprintf_ff(1, "Fetching register .. %d", cmd.reg_id);
        ret_code = _fim_platform_execute_command(&cmd, &storage);
        ON_ERROR_DEBUG_AND_RETURN_ECODE(ret_code, "_fim_platform_execute_command");
        debugprintf_ff(1,"done\n");


        /* Inject fault */
        uint8_t bit_idx = injection->bit_idx;
        _fim_inject_bit(bit_idx, &storage, injection->injection_type, &injection->prev_bit_value, is_reinjection); injection->prev_bit_value_isset = true;


        /* Store register back to target */
        cmd.type = PROTO_SET_REG;

        debugprintf_ff(1, "Storing register to target\n");
        ret_code = _fim_platform_execute_command(&cmd, &storage);
        ON_ERROR_DEBUG_AND_RETURN_ECODE(ret_code, "_fim_platform_execute_command");
        debugprintf_ff(1, "Register content has been sent to target\n");
    }

    return ret_code;
}