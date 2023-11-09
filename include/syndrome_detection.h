/**
 * @file syndrome_detection.h
 * @author Marek Dolezel (marekdolezel@me.com)
 * @brief 
 * @version 0.1
 * @date 2020-03-18
 * 
 * @copyright Copyright (c) 2020
 * 
 */

#ifndef SYNDROME_DETECTION_H
#define SYNDROME_DETECTION_H

#include <stdint.h>
#include <stdbool.h>

struct is_array_sorted_arg {
    uint32_t array_addr;
    uint32_t array_size_bytes;

    uint32_t demo_state_addr;
    uint32_t demo_state_size_bytes;
};

struct syndrome_detection_arg {
    struct is_array_sorted_arg ias_arg;
    bool sorting_demo_flg;
};

int syndrome_detection(struct syndrome_detection_arg sd_arg);
int get_arr_inversion_number(struct is_array_sorted_arg ias_arg, int *arr_inversion_number);
#endif