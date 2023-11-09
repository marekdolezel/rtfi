/**
 * @file storage.h
 * @author Marek Dolezel (marekdolezel@gmail.com)
 * @brief 
 * @version 0.1
 * @date 2018-11-26 renamed platform.h
 * 
 * @copyright Copyright (c) 2018
 * 
 */

#ifndef STORAGE_H
#define STORAGE_H

#include <stdint.h>
#include <stdbool.h>

#define ST_BITS_IN_BYTE 8

typedef enum {
    ST_BIG_ENDIAN,    // lsb on highest address
    ST_LITTLE_ENDIAN, // lsb on lowest address
} endianness_t;

struct storage {
    uint8_t *content;
    uint32_t size;
    uint32_t capacity;
    endianness_t endianness;
};

uint32_t storage_create(struct storage *storage, uint32_t size, endianness_t endianness);
void storage_destroy(struct storage *storage);
void storage_set_value(struct storage *storage, uint64_t value);
uint64_t storage_get_value(struct storage *storage);

void storage_set_byte(struct storage *storage, uint8_t byte_idx, uint8_t byte);
void storage_toggle_bit(struct storage *storage, uint32_t bit_idx);
void storage_get_byte(struct storage *storage, uint8_t byte_idx, uint8_t *byte);

void storage_set_bit(struct storage *storage, uint32_t bit_idx, bool bit);
void storage_get_bit(struct storage *storage, uint32_t bit_idx, bool *bit);


#endif /* STORAGE_H */