#ifndef CYB3053_PROJECT2_ALLOC_H
#define CYB3053_PROJECT2_ALLOC_H

#include <stddef.h>

/**
 * Header for allocated blocks
 */
typedef struct header {
    size_t size; /**< Size of the block */
    int magic; /**< Magic number for error checking */
} header;

/**
 * Free block structure
 */
typedef struct free_block {
    size_t size; /**< Size of the block */
    struct free_block *next; /**< Pointer to the next free block */
} free_block;

void *tumalloc(size_t size);
void *tucalloc(size_t num, size_t size);
void *turealloc(void *ptr, size_t new_size);
void tufree(void *ptr);

#endif //CYB3053_PROJECT2_ALLOC_H
