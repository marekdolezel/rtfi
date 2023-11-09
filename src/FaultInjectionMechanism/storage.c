/**
 * @file storage.c
 * @author Marek Dolezel (marekdolezel@me.com)
 * @brief 
 * @version 0.1
 * @date 2020-03-26
 * 
 * @copyright Copyright (c) 2020
 * 
 */

#include <malloc.h>
#include <stdio.h>
#include <stdbool.h>
#include <assert.h>

#include "storage.h"

#include "debug.h"
#include "error_codes.h"
// #include "rsp.h"


uint32_t
storage_create(struct storage *storage, uint32_t size, endianness_t endianness)
{
    uint32_t ret_code = ERR_NONE;

    storage->content = malloc(sizeof(uint8_t) * size);
    storage->size = size;
    storage->capacity = size;
    storage->endianness = endianness;

    debugprintf_ff(4, "size: %d, capacity: %d\n", storage->size, storage->capacity);

    if (storage->content == NULL)
        ret_code = ERR_MALLOC;

    //memset(storage->content, 0, sizeof(storage->content) * size);
    return ret_code;
}

void
storage_destroy(struct storage *storage)
{
    if (storage == NULL) {
        debugprintf_ff(1, "Warning: storage_destroy(storage==NULL)\n");
        return;
    }

    free(storage->content);
}

void
storage_set_byte(struct storage *storage, uint8_t byte_idx, uint8_t byte)
{
    if (storage->capacity == 0) {
        debugprintf_ff(1,"Warning: storage is full\n");
        return;
    }

    uint32_t max_idx = storage->size - 1;

    if (storage->endianness == ST_LITTLE_ENDIAN) {
        storage->content[byte_idx] = byte;
        storage->capacity--;
    }
    if (storage->endianness == ST_BIG_ENDIAN) {
        storage->content[max_idx - byte_idx] = byte;
            storage->capacity--;
    }
         
}

void
storage_get_byte(struct storage *storage, uint8_t byte_idx, uint8_t *byte)
{
    *byte = storage->content[byte_idx];
}


void
_get_byte_and_bit_idx(uint32_t _bits, uint32_t bit_idx, uint32_t *byte_idx, uint32_t *bit_idx_inbyte)
{
    /* which byte contains our bit? */
    uint32_t _byte_idx = 0;
    uint32_t _lbit_idx = 0;
    uint32_t _rbit_idx = 7;

    while(_lbit_idx < bit_idx && _rbit_idx < bit_idx ) {
        _lbit_idx += 8;
        _rbit_idx += 8;
        _byte_idx++;
    }

    *byte_idx = _byte_idx;
    *bit_idx_inbyte = bit_idx - _lbit_idx;
    // debugprintf_ff(1, "_bits %d,  bit_idx %d, bit_idx_inbyte %d\n", _bits, bit_idx, *bit_idx_inbyte);
}

void
storage_set_bit(struct storage *storage, uint32_t bit_idx, bool bit)
{
    uint32_t byte_idx;
    uint32_t bit_idx_inbyte;

    _get_byte_and_bit_idx(storage->size*ST_BITS_IN_BYTE, bit_idx, &byte_idx, &bit_idx_inbyte);

    if (bit == true) 
        storage->content[byte_idx] |=  1 << bit_idx_inbyte; // set bit to 1 
    else
        storage->content[byte_idx] &=  ~(1 << bit_idx_inbyte); // set bit to 0
}

void
storage_toggle_bit(struct storage *storage, uint32_t bit_idx)
{
    uint32_t byte_idx;
    uint32_t bit_idx_inbyte;

    _get_byte_and_bit_idx(storage->size*ST_BITS_IN_BYTE, bit_idx, &byte_idx, &bit_idx_inbyte);
    storage->content[byte_idx] ^=  1 << bit_idx_inbyte; // toggle bit
}


void
storage_get_bit(struct storage *storage, uint32_t bit_idx, bool *bit)
{
    uint32_t byte_idx;
    uint32_t bit_idx_inbyte;

    _get_byte_and_bit_idx(storage->size*ST_BITS_IN_BYTE, bit_idx, &byte_idx, &bit_idx_inbyte);

    uint8_t byte = storage->content[byte_idx];
    *bit = (byte >> bit_idx_inbyte) & 1U;
}

void
storage_set_value(struct storage *storage, uint64_t value)
{
    int content_idx;
    int value_idx;

    if (storage->endianness == ST_LITTLE_ENDIAN) {
        value_idx = 0;
        for (content_idx = 0; content_idx < storage->size; content_idx++, value_idx++) 
            storage->content[content_idx] = (value >> value_idx*8) & 0xFF;
    }

    if (storage->endianness == ST_BIG_ENDIAN) {
        value_idx = storage->size - 1; 
        for (content_idx = 0; content_idx >= 0; content_idx--, value_idx--) 
            storage->content[content_idx] = (value >> value_idx*8) & 0xFF;
    }    
}

uint64_t
storage_get_value(struct storage *storage)
{
    uint64_t reg_value = 0;
    uint64_t mask = 0xFF;

    for (int idx = 0; idx < storage->size; idx++) {
        uint64_t mask = (reg_value | (0xFF << idx*8));
        uint64_t content = (reg_value | (storage->content[idx] << idx*8));
        
        reg_value = content & mask;
    }

    return reg_value;
}