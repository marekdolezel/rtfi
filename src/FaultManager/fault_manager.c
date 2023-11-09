#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdbool.h>
#include <time.h> // for seed
#include <SFMT.h> // marseine twister
#include <sys/time.h>
#include <signal.h>

#include "fault_injection_mechanism.h"
#include "error_codes.h"
#include "debug.h"

#define NUMOF_USECS_IN_SEC 1000000

#ifdef SFMT
    #define RAND() sfmt_genrand_uint32(&sfmt)
#else
    #define RAND() rand()
#endif

char *injtype[] = {
    // "INJ_TYPE_FLIP",
    // "INJ_TYPE_SET_ZERO",
    // "INJ_TYPE_SET_ONE",
    // "INJ_TYPE_SET_ZERO_PERMANENT",
    // "INJ_TYPE_SET_ONE_PERMANENT",
    // "INJ_TYPE_SET_STUCKAT_ZERO_TRANSIENT",
    // "INJ_TYPE_SET_STUCKAT_ONE_TRANSIENT"
    "INJ_TYPE_BITFLIP_TRANSIENT"
};

volatile sig_atomic_t reinjecting = 1;
sfmt_t sfmt;


void fim_endof_transient_fault(int sig)
{
    /* Timer has elapsed. We are not longer injecting permanent or transient fault */
    debugprintf_ff(1, "Alarm handler is running - end of transient fault\n");
    reinjecting = 0;
}

int fim_set_fault_parameters(struct fim_injection *fault, uint32_t addr, uint32_t length) 
{
    if (fault == NULL)
        debugprintf_ff_null(1, "fault");

    fault->fault_duration_t_us = (RAND() % (NUMOF_USECS_IN_SEC));
    fault->fault_injection_t_us = (RAND() % (NUMOF_USECS_IN_SEC));
    fault->injection_type = (RAND() % (INJ_TYPE_COUNT));

    switch(fault->device) {
        case INJ_DEV_REG:
            fault->reg_id = (RAND() % (fim_platforminfo_numof_regs()));
            fault->bit_idx = (RAND() % (fim_plaforminfo_sizeof_reg(fault->reg_id)*8)); /* NIOS2: all regs are 4bytes */        
        break;

        case INJ_DEV_MEM:
            fault->mem_addr = addr;
            fault->mem_length = length;
            fault->bit_idx = (RAND() % (length*8));
        break;
    }
}

int
fault_manager(uint32_t addr, uint32_t length, bool reg, long experiment_id)
{
    int     ret_code    = ERR_NONE;
    bool    mem         = !reg;
    

    /* Fault related information */
    // TODO: change this to fault_duration: transient, permanent, intermittent
    bool    is_transient_fault  = false;
    bool    is_permanent_fault  = false;
    bool    set_watchpoint      = false;
    struct fim_injection     fault; fault.prev_bit_value_isset = false;
    struct itimerval        itimer;


    int     execution_t_after_last_fault_ended_sec = 5;

    #ifdef SFMT
        printf("PRNG: Using SMFT generator; see README.md\n");
        sfmt_init_gen_rand(&sfmt, time(NULL));
    #else
        printf("PRNG: Using c rand() generator\n");
        srand(time(NULL));
    #endif

    if (reg == true)
        fault.device = INJ_DEV_REG;
    else 
        fault.device = INJ_DEV_MEM;

    fim_set_fault_parameters(&fault, addr, length);

    //TODO: separate experiment id from fault information, this change requires some changes in helper scripts
    if (reg == true)
        printf("EXPERIMENT_REG(id:%ld): reg_id:%d, bit_idx:%d, injection_type:%s, inj_t_us:%d", 
            experiment_id, fault.reg_id, fault.bit_idx, injtype[fault.injection_type], fault.fault_injection_t_us);
    else 
        printf("EXPERIMENT_MEM(id:%ld): mem_addr:0x%x, mem_length:0x%x; bit_idx:%d, injection_type:%s, t1(usec):%d, t2(usec):%d\n", 
            experiment_id, fault.mem_addr, fault.mem_length, 
            fault.bit_idx, injtype[fault.injection_type],
            fault.fault_injection_t_us, fault.fault_duration_t_us );

    injection_type_t fault_type = fault.injection_type;
    if (fault_type == INJ_TYPE_BITFLIP_TRANSIENT)
            is_transient_fault = true;


    if (is_permanent_fault == true || is_transient_fault == true)
        set_watchpoint = true;


    /* Alarm from timer will be sent only once */
    if (is_permanent_fault == true || is_transient_fault == true) {
        itimer.it_interval.tv_sec = 0;
        itimer.it_interval.tv_usec = 0;
    }

    /* set timer - not counting yet */
    if (is_permanent_fault == true) {
        /* 
         * fault_duration for permanent fault is infinity
         * there is no phase 3 for usleep(execution_t_after_fault_ended_sec)
         * so we just add execution_t_after_fault_ended_sec to itimer and
         * when the itimer expires, we stop reinjecting the fault and return from
         * fault manager back to main
         */

        itimer.it_value.tv_sec = execution_t_after_last_fault_ended_sec;
        itimer.it_value.tv_usec = fault.fault_duration_t_us; 
    }
    if (is_transient_fault == true) {
        itimer.it_value.tv_sec = 0;
        itimer.it_value.tv_usec = fault.fault_duration_t_us;
    }

    /* Register alarm handler */
    if(signal(SIGALRM, fim_endof_transient_fault) == SIG_ERR) {
        debugprintf_ff(4, "Unable to register signal handler\n");
        return ERR_SIGNAL;
    }


    /* 
     * At this point the binary has been loaded to the FPGA. The script that calls us leaves platform in paused state. 
     * We need to invoke fi_platform_execute() and sleep for t1(injection_time): Uniform(0,1000000) usecs
     * 
     * TRANSIENT FAULT: the following injection_types represent this type of fault
     *      INJ_TYPE_BITFLIP_TRANSIENT
     *                               + injection_time   
     *                  usleep       |      itimer              usleep
     *  time:   (x)-----------------(x)--------------------(x)---------------------(x)
     *           <------------------><----------------------><---------------------->
     *                  NO FAULTS         fault_duration           NO FAULTS (see 1)
     * 
     * 
     * 
     * PERMANENT FAUTLT: the following injection_types represent this type of fault
     *      INJ_TYPE_SET_STUCKAT_ZERO_PERMANENT
     *      INJ_TYPE_SET_STUCKAT_ONE_PERMANENT
     *                               + injection_time   
     *                  usleep       |      
     *  time:   (x)-----------------(x)--------------------------------------------(x)
     *           <------------------><----------------------------------------------->
     *                  NO FAULTS         FAULT IS PRESENT 
     * 
     * 
     * 1: 
     *  Fault is not present if rewritten by write instruction. 
     *  If fault was present when the system accessed memory location/register with fault, errorneous results may be produced.
     *  
    */

    /* Start execution */
    debugprintf(1, "fi_platform_execute()\n");
    fim_platform_execute();

    debugprintf(1, "usleep(%d usec) (%d sec)\n", fault.fault_injection_t_us, (fault.fault_injection_t_us / NUMOF_USECS_IN_SEC));
    usleep(fault.fault_injection_t_us);


    /* Stop execution */
    debugprintf(1, "fim_platform_stop_execution()\n");
    ret_code = fim_platform_stop_execution();
    ON_ERROR_DEBUG_AND_RETURN_ECODE(ret_code, "fi_platform_stop_execution");

    /* Inject fault */
    debugprintf(1, "fim_inject()\n");
    ret_code = fim_inject(&fault, false);
    ON_ERROR_DEBUG_AND_RETURN_ECODE(ret_code, "fi_inject");
    
    if (is_transient_fault == true || is_permanent_fault == true) { /* Reinjecting permanent or transient fault */
        debugprintf_ff(1, "setting watchpoint .. ");
        ret_code = fim_platform_watchpoint(true, fault.mem_addr, fault.mem_length);
        ON_ERROR_DEBUG_AND_RETURN_ECODE(ret_code, "fi_platform_watchpoint");
        debugprintf_ff(1, "set\n");

        /* Start executing of target */
        ret_code = fim_platform_execute();
        ON_ERROR_DEBUG_AND_RETURN_ECODE(ret_code, "fi_platform_execute");

        if(setitimer(ITIMER_REAL, &itimer, NULL) == -1) {
            debugprintf_ff(4, "setitimer has failed with -1\n");
            return ERR_SETITIMER;
        }    
        
        /* Platform is running before we start reinjecting fault */
        bool running = true;
        while(reinjecting) {
            debugprintf(1, "<--Watchpoint loop\n");
            debugprintf(1, "Waiting watchpoint hit \n");

            /* Wait for watchpoint hit, target will be paused for us */
            ret_code = fim_platform_watchpoint_trigger();
            debugprintf(1, "ret_code watchpoint_trigger %d\n", ret_code);
            if (ret_code != ERR_NONE) {
                debugprintf(1, "No write to structure/variable\n");
            }

            if (ret_code == ERR_NONE) {
                /* Watchpoint was triggered, we are not running */
                running = false;

                /* Inject fault */
                debugprintf(1, "fim_inject()\n");
                fim_inject(&fault, true);

                /* Start execution */
                ret_code = fim_platform_execute();
                if (ret_code == 0) running = true;
                ON_ERROR_DEBUG_AND_RETURN_ECODE(ret_code, "fi_platform_execute");
            }

            debugprintf(1, "<--Watchpoint loop\n\n");
        }
        ret_code = ERR_NONE;
        
        /* When we stop reinjecting is target running or not? */
        if (running == true) {
            debugprintf(1, "fim_platform_stop_execution()\n");
            ret_code = fim_platform_stop_execution();
            ON_ERROR_DEBUG_AND_RETURN_ECODE(ret_code, "fim_platform_stop_execution");
        }
        
        debugprintf(1, "removing watchpoint .. ");
        ret_code = fim_platform_watchpoint(false, fault.mem_addr, fault.mem_length);
        ON_ERROR_DEBUG_AND_RETURN_ECODE(ret_code, "fi_platform_watchpoint");
        debugprintf_ff(1, "removed\n");


        if (is_transient_fault == true) {
            debugprintf_ff(1,"Transient fault is gone, running fim_platform_execute()\n");
            /* Start execution */
            ret_code = fim_platform_execute();
            ON_ERROR_DEBUG_AND_RETURN_ECODE(ret_code, "fi_platform_execute");

            /* Sleep for execution_t_after_fault_ended_sec */
            usleep(execution_t_after_last_fault_ended_sec*NUMOF_USECS_IN_SEC);

            /* Stop executing before further inspection of fault syndrome */
            ret_code = fim_platform_stop_execution();
            ON_ERROR_DEBUG_AND_RETURN_ECODE(ret_code, "fi_platform_stop_execution");

        }
    }
    
    debugprintf(1, "Return on last line\n");
    return ret_code;
}