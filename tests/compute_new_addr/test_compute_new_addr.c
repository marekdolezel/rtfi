#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>

#include "fault_injection_mechanism.h"
#include "error_codes.h"
#include "debug.h"

uint32_t _fim_compute_new_addr(uint32_t mem_addr, uint32_t mem_length, uint32_t *bit_idx_new, uint32_t bit_idx);

int assert(int id, int value, int expected, char *name) {
    if (value == expected) {
        printf("test %s %d OK\n", name, id);
        return 0;
    }
    else {
        printf("test %s %d failed with %d (%d expected) \n", name, id, value, expected);
        return 1;
    }
}
int main()
{
    int new_bit = 0;
    int new_addr = 0;
    new_addr = _fim_compute_new_addr(0,4, &new_bit, 8); assert(1, new_bit, 0, "bit"); assert(1, new_addr, 1, "new_addr");
    new_addr = _fim_compute_new_addr(0,4, &new_bit, 16); assert(1, new_bit, 0, "bit"); assert(1, new_addr, 2, "new_addr");
    new_addr = _fim_compute_new_addr(0,4, &new_bit, 24); assert(1, new_bit, 0, "bit"); assert(1, new_addr, 3, "new_addr");
    
    for (int i=0; i<8; i++) {
        new_addr = _fim_compute_new_addr(0,4, &new_bit, i); assert(1, new_bit, i, "bit"); assert(1, new_addr, 0, "new_addr");
    }

     new_addr = _fim_compute_new_addr(0,4, &new_bit, 9); assert(1, new_bit, 1, "bit"); assert(1, new_addr, 1, "new_addr");
     new_addr = _fim_compute_new_addr(0,4, &new_bit, 10); assert(1, new_bit, 2, "bit"); assert(1, new_addr, 1, "new_addr");
     new_addr = _fim_compute_new_addr(0,4, &new_bit, 11); assert(1, new_bit, 3, "bit"); assert(1, new_addr, 1, "new_addr");
     new_addr = _fim_compute_new_addr(0,4, &new_bit, 12); assert(1, new_bit, 4, "bit"); assert(1, new_addr, 1, "new_addr");
     new_addr = _fim_compute_new_addr(0,4, &new_bit, 13); assert(1, new_bit, 5, "bit"); assert(1, new_addr, 1, "new_addr");
     new_addr = _fim_compute_new_addr(0,4, &new_bit, 14); assert(1, new_bit, 6, "bit"); assert(1, new_addr, 1, "new_addr");
     new_addr = _fim_compute_new_addr(0,4, &new_bit, 15); assert(1, new_bit, 7, "bit"); assert(1, new_addr, 1, "new_addr");

}