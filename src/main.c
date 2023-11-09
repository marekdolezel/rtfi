#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <getopt.h>
#include <sys/stat.h>
#include <signal.h>

#include "error_codes.h"
#include "debug.h"
#include "fault_injection_mechanism.h"
#include "protocol.h"
#include "syndrome_detection.h"
#include <SFMT.h>

/* declarations */
int fault_manager(uint32_t addr, uint32_t length, bool reg, long experiment_id);

bool port_flg = false;
bool host_flg = false;
bool pt_cfg_flg = false;
bool addr_flg = false;
bool length_flg = false;
bool inj_reg_flg = false;
bool sorting_demo_flg = false;
bool sorting_array_base_addr_flg = false;
bool sorting_array_arr_size_bytes_flg = false;
bool sorting_array_demo_state_addr_flg = false;
bool sorting_array_demo_state_size_bytes_flg = false;


void
signal_handler (int sig_num)
{
    debugprintf_ff(1, "Handling signal %d\n", sig_num);
    exit(-1);
} 

void signal_handler_empty(int sig_num) { }

int
main(int argc, char **argv)
{
    sfmt_t sfmt;
        sfmt_init_gen_rand(&sfmt, time(NULL));
    for (int i=0; i < 100; i++)
        printf("%u\n", sfmt_genrand_uint32(&sfmt));
    debugprintf(1, "Debug is enabled\n");

    signal(SIGINT, signal_handler_empty); /* We are not ready to terminate */
    
    char *error_code[ERR_CODES_COUNT] = {
        "None",
        "invalid arguments",
        "Pointer to object is NULL",
        "Unable to create socket",
        "Unable to connect to GDB server",
        "setitimer() has failed",
        "signal() has failed to register handler"
        "select() returned -1, error",
        "select() timeout",
        "write() Less bytes than expected were sent",
        "malloc() failed",
        "fopen() failed",
        "freat() failed",
        "stat() failed",
        "invalid json file?",
        "invalid register ID",
        "invalid memory address",
        "invalid device type",
        "invalid platform type",
        "invalid rsp response",
        "invalid size"
    };
    int ret_code = ERR_NONE;
    uint32_t addr = -1, length = -1;
    long experiment_id = -1;
    struct fim_injection fault;
    struct fim_configuration cfg;
    char *endptr;

    struct syndrome_detection_arg sd_arg;

    int c;
    while ((c = getopt(argc, argv, "p:h:a:l:re:sw:x:t:z:")) != -1) {
        switch (c)
        {
            case 'p':
                /* Port number */
                cfg.port = atoi(optarg);
                port_flg = true;
            break;

            case 'h':
                /* IP address (usually 127.0.0.1) */
                // TODO: convert from string to 32 int
                cfg.host = atoi(optarg);
                host_flg = true;
            break;

            case 'a':
                /* Address of object to be injected */
                addr = strtol(optarg, &endptr, 16);
                debugprintf_ff(2, "ADDR: %x(%d)\n", addr, addr);
                addr_flg = true;
            break;

            case 'l':
                /* Length of object */
                length = strtol(optarg, &endptr, 16);
                debugprintf_ff(2, "LENGTH: %x(%d)\n", length, length);
                length_flg = true;
            break;

            case 'r':
                /* Injection to register */
                inj_reg_flg = true;
                debugprintf_ff(2, "Injection to register\n");
            break;

            case 'e':
                /* Experiment identifier */
                experiment_id = strtoll(optarg, &endptr, 10);
                debugprintf_ff(2, "experiment_id: %ld\n", experiment_id);
            break;

            case 's':
                /* Sorting task s w x t z */
                sorting_demo_flg = true;
                sd_arg.sorting_demo_flg = true;
            break;

            case 'w': 
                /* array_base_addr */
                sd_arg.ias_arg.array_addr = strtol(optarg, &endptr, 16);
                debugprintf_ff(2, "ADDR: %x(%d)\n", addr, addr);
                sorting_array_base_addr_flg = true;
            break;

            case 'x':
                /* array size in bytes */
                sd_arg.ias_arg.array_size_bytes = strtol(optarg, &endptr, 16);
                debugprintf_ff(2, "LENGTH: %x(%d)\n", length, length);
                sorting_array_arr_size_bytes_flg = true;
            break;

            case 't': 
                /* demo_state_addr */
                sd_arg.ias_arg.demo_state_addr = strtol(optarg, &endptr, 16);
                debugprintf_ff(2, "ADDR: %x(%d)\n", addr, addr);
                sorting_array_demo_state_addr_flg = true;
            break;

            case 'z':
                /* demo_state variable size in bytes */
                sd_arg.ias_arg.demo_state_size_bytes = strtol(optarg, &endptr, 16);
                debugprintf_ff(2, "LENGTH: %x(%d)\n", length, length);
                sorting_array_demo_state_size_bytes_flg = true;
            break;



        default:
            break;
        }
    }

    if (inj_reg_flg == true && (addr_flg == true || length_flg == true)) {
        printf("ERROR: flag -r cannot be combined with -a or -l\n\n");
    }
    if (port_flg == false || host_flg == false) {
        printf("Please enter required flags\n"
            "\t-p - device port \n"
            "\t-h - device ip address\n"
            "\tMemory injection:\n"
            "\t\t-a - address in memory, where fault will be injected\n"
            "\t\t-l - length of injection area\n"
            "\tRegister injection:\n"
            "\t\t-r - inject to random register\n\n"
            "Please note, that flags for memory injection cannot be combined with flags for register injection\n");
        return ERR_INVALID_ARGS;
    }

    if (sorting_demo_flg == true && 
    (sorting_array_demo_state_size_bytes_flg == false) || sorting_array_demo_state_addr_flg == false ||
     sorting_array_arr_size_bytes_flg == false || sorting_array_base_addr_flg == false ) {
        printf("\t -s flag needs -w [array_addr] -x [array_size_bytes] -t [demo_state_addr] -z [demo_state_size_bytes] flags\n");
        return ERR_INVALID_ARGS;
     }

    debugprintf(1, "Calling fi_init()\n"); 
    ret_code = fim_init(cfg);

    if (ret_code != ERR_NONE) {
        goto error;
    }

    debugprintf(1, "Running fault_manager() after fi_init() \n");    
    ret_code = fault_manager(addr, length, inj_reg_flg, experiment_id);

    if (ret_code != ERR_NONE) {
        goto error;
    }
    
    signal(SIGINT, signal_handler);
    
   

    debugprintf_ff(1, "Running syndromeDetection\n");
    ret_code = syndrome_detection(sd_arg);

    if (ret_code != ERR_NONE) {
        error:
        #ifdef DEBUG
        if (ret_code >= ERR_CODES_COUNT || ret_code < 0 ) {
            debugprintf_ff(1, "Aborting. (error %d). WARNING: unknown error %d\n", ret_code, ERR_CODES_COUNT);
        }
        else
            debugprintf_ff(1, "Aborting. (error %d - %s)\n", ret_code, error_code[ret_code]);
        #endif
    }
    /* Close connection to target */
    fim_close();

    return ret_code;
    
}