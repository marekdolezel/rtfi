/**
 * @file error_codes.h
 * @author Marek Dolezel (marekdolezel@gmail.com)
 * @brief 
 * @version 0.1
 * @date 2018-11-26
 * 
 * @copyright Copyright (c) 2018
 * 
 */

#ifndef ERROR_CODES_H
#define ERROR_CODES_H

#include "debug.h"

/* NOTE: */
enum {
    ERR_NONE,
    ERR_INVALID_ARGS,
    ERR_NULL_POINTER,
    ERR_SOCK_CREATE,
    ERR_GDBSERVER_CONNECT,
    ERR_SETITIMER,
    ERR_SIGNAL,
    ERR_SELECT,
    ERR_SELECT_TIMEOUT,
    ERR_WRITE,
    ERR_MALLOC,
    ERR_FOPEN,
    ERR_FREAD,
    ERR_STAT,
    ERR_JSON_PARSE_VALUE_NULL,
    ERR_INVALID_REG_ID,
    ERR_INVALID_MEM_ADDR,
    ERR_INVALID_DEV_TYPE,
    ERR_INVALID_PLATFORM_TYPE,
    ERR_INVALID_RSP_RESPONSE,
    ERR_INVALID_SIZE,
    ERR_UNKNOWN_FAULT_TYPE
};

#define ERR_CODES_COUNT ERR_INVALID_SIZE + 1

#define ON_ERROR_DEBUG_AND_RETURN_ECODE(ret_code, func_name) {\
if (ret_code != ERR_NONE) {\
        debugprintf_ff(1, "call to %s() failed with error %d\n", func_name, ret_code);\
        return ret_code;\
    }\
} while(0);

#define ON_ERROR_DEBUG_AND_GOTO_LABEL(ret_code, func_name, label) {\
if (ret_code != ERR_NONE) {\
        debugprintf_ff(1, "call to %s() failed with error %d\n", func_name, ret_code);\
        goto label;\
    }\
} while(0);

#define ON_ERROR_DEBUGMSG_AND_RETURN_ECODE(ret_code, debugmsg) {\
if (ret_code != ERR_NONE) {\
        debugprintf_ff(1, debugmsg);\
        return ret_code;\
    }\
} while(0);
#endif