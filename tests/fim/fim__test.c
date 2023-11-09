#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>

#include "fault_injection_mechanism.h"
#include "error_codes.h"
#include "debug.h"

int
main(int argc, char **argv)
{
    int                         ret_code = ERR_NONE;
    struct fim_injection        fault;
    struct fim_configuration    cfg;
    char*                       endptr;


    cfg.port = atoi(argv[1]);
    cfg.host = 0;

    /* Wait for binary loading */
    printf("sleep 5; before binary loads\n");
    sleep(5);

    ret_code = fim_init(cfg);

    /* stuckatVar variable */
    fault.mem_length = 4;
    fault.mem_addr = strtol(argv[4], &endptr, 16);
    fault.injection_type = atoi(argv[2]);
    fault.bit_idx = atoi(argv[3]);
    fault.device = INJ_DEV_MEM;

    printf("type_offault=%d\n", fault.injection_type);
    /* Execute */
    ret_code = fim_platform_execute();
    if (ret_code != ERR_NONE) {
        printf("test: error %d\n", ret_code);
    }

    printf("sleep 5; let platform run for 5 secs\n");
    sleep(5);
    
    /* Stop */
    ret_code = fim_platform_stop_execution();
    if (ret_code != ERR_NONE) {
        printf("test: error %d\n", ret_code);
    }

    /* Inject */
    ret_code = fim_inject(&fault);
    if (ret_code != ERR_NONE) {
        printf("test: Injection failed with error %d\n", ret_code);
        return ret_code;
    }

    /* Execute */
    ret_code = fim_platform_execute();
    if (ret_code != ERR_NONE) {
        printf("test: error %d\n", ret_code);
    }

    sleep(5);

    /* Stop */
    ret_code = fim_platform_stop_execution();
    if (ret_code != ERR_NONE) {
        printf("test: error %d\n", ret_code);
    }

    return ret_code;
}