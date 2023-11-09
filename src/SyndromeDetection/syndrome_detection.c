/**
 * @file syndrome_detection.c
 * @author Marek Dolezel (marekdolezel@me.com)
 * @brief 
 * @version 0.1
 * @date 2020-03-18
 * 
 * @copyright Copyright (c) 2020
 * 
 */

#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>

#include "syndrome_detection.h"

#include "debug.h"
#include "error_codes.h"
#include "protocol.h"
#include "rsp.h"
#include "storage.h"
#include "fault_injection_mechanism.h"

uint32_t 
get_var(uint32_t addr, uint32_t size_bytes)
{
    int             ret_code = ERR_NONE;
    struct storage  storage;
    uint32_t        variable;

    ret_code = storage_create(&storage, size_bytes, fim_platforminfo_endianness());
    ON_ERROR_DEBUG_AND_RETURN_ECODE(ret_code, "storage_create");

    /* Get variable */
    ret_code = fim_platform_get_object(addr, size_bytes, &storage);
    ON_ERROR_DEBUG_AND_RETURN_ECODE(ret_code, "fim_platform_get_object");
    variable = storage_get_value(&storage);
    storage_destroy(&storage);
    debugprintf(4, "variable = %d\n",variable);

    return variable;
}

int
get_arr_inversion_number(struct is_array_sorted_arg ias_arg, int *arr_inversion_number)
{
    int             ret_code = ERR_NONE;
    struct storage  storage;

    int             *arr;
    int             _arr_inversion_number = 0;

    uint32_t        array_base_addr = ias_arg.array_addr;
    uint32_t        array_size_bytes = ias_arg.array_size_bytes;

    int64_t         demo_state = 0;
    int             additional_time = 6; /* additional time [sec] when demo state is zero */
    uint32_t        demo_state_addr = ias_arg.demo_state_addr;
    uint32_t        demo_state_size_bytes = ias_arg.demo_state_size_bytes;


    debugprintf_ff(4, "array_base_addr=%x, array_size_bytes=%x\n", array_base_addr, array_size_bytes);
    debugprintf_ff(4, "demo_state_addr=%x, demo_state_size_bytes=%x\n", demo_state_addr, demo_state_size_bytes);


    demo_state = get_var(demo_state_addr, demo_state_size_bytes);
    if (demo_state == 0) {
        debugprintf(1, "Demostate is 0; running target for a while (6secs)\n");
        /* Start executing of target */
        ret_code = fim_platform_execute();
        ON_ERROR_DEBUG_AND_RETURN_ECODE(ret_code, "fi_platform_execute");

        printf("ADDITIONAL_TIME:%d\n", additional_time);
        sleep(additional_time);

        /* Stop executing before further inspection of fault syndrome */
        ret_code = fim_platform_stop_execution();
        ON_ERROR_DEBUG_AND_RETURN_ECODE(ret_code, "fi_platform_stop_execution");
        
        demo_state = get_var(demo_state_addr, demo_state_size_bytes);
        if (demo_state == 0)
            debugprintf(1, "Demostate is still zero, infinite loop?\n");
    } 
    else
        printf("ADDITIONAL_TIME:0\n");
    
    printf("DEMO_STATE:%ld\n", demo_state);


    arr = malloc(sizeof(int)*100);
    
    /* Get the entire array - it's array of ints */
    for (int arr_idx = 0; arr_idx < 100; arr_idx++) { 
        ret_code = storage_create(&storage, 4, fim_platforminfo_endianness());
        ON_ERROR_DEBUG_AND_RETURN_ECODE(ret_code, "storage_create");

        ret_code = fim_platform_get_object(array_base_addr + arr_idx*4, 4, &storage);
        ON_ERROR_DEBUG_AND_RETURN_ECODE(ret_code, "fim_platform_get_object");

       

        arr[arr_idx] = storage_get_value(&storage);
        debugprintf(1, "Got element value %ld\n", storage_get_value(&storage));

        storage_destroy(&storage);
    }

    for (int i = 0; i < 100; i++) {
        for (int j = 0; j < 100; j++) {
            if (i < j && arr[i] > arr[j]) {
                printf("inversion pair (%d, %d) ..A[%d]=%d A[%d]=%d\n", i, j, i, j, arr[i], arr[j]); //TODO: fix 
                _arr_inversion_number++;
            }
        }
    }

    *arr_inversion_number = _arr_inversion_number;
    return ERR_NONE;

}

int
syndrome_detection(struct syndrome_detection_arg sd_arg)
{
    int ret_code = ERR_NONE;
    int arr_inversion_number;
    
    if (sd_arg.sorting_demo_flg == true) {
        ret_code = get_arr_inversion_number(sd_arg.ias_arg, &arr_inversion_number);  
        if (ret_code == ERR_NONE) {
            printf("SORT:%d\n", arr_inversion_number);
        }
        else {
            return ret_code;
        }
    }

    return ret_code;
}