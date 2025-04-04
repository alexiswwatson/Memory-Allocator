#include "alloc.h"

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stdint.h>

#define ALIGNMENT 16 /**< The alignment of the memory blocks */

static free_block *HEAD = NULL; /**< Pointer to the first element of the free list */
static free_block *NEXT = NULL; /**< Pointer to the next element of the free list -- next-fit search */

/**
 * Split a free block into two blocks
 *
 * @param block The block to split
 * @param size The size of the first new split block
 * @return A pointer to the first block or NULL if the block cannot be split
 */
void *split(free_block *block, int size) {
    if((block->size < size + sizeof(free_block))) {
        return NULL;
    }

    void *split_pnt = (char *)block + size + sizeof(free_block);
    free_block *new_block = (free_block *) split_pnt;

    new_block->size = block->size - size - sizeof(free_block);
    new_block->next = block->next;

    if (block == HEAD) {
        HEAD = new_block;
    }

    block->size = size;
    block->next = new_block;

    NEXT = new_block;

    return block;
}

/**
 * Find the previous neighbor of a block
 *
 * @param block The block to find the previous neighbor of
 * @return A pointer to the previous neighbor or NULL if there is none
 */
free_block *find_prev(free_block *block) {
    free_block *curr = HEAD;
    while(curr != NULL) {
        char *next = (char *)curr + curr->size + sizeof(free_block);
        if(next == (char *)block)
            return curr;
        curr = curr->next;
    }
    return NULL;
}

/**
 * Find the next neighbor of a block
 *
 * @param block The block to find the next neighbor of
 * @return A pointer to the next neighbor or NULL if there is none
 */
free_block *find_next(free_block *block) {
    char *block_end = (char*)block + block->size + sizeof(free_block);
    free_block *curr = HEAD;

    while(curr != NULL) {
        if((char *)curr == block_end)
            return curr;
        curr = curr->next;
    }
    return NULL;
}

/**
 * Remove a block from the free list
 *
 * @param block The block to remove
 */
void remove_free_block(free_block *block) {
    free_block *curr = HEAD;
    if(curr == block) {
        HEAD = block->next;
        return;
    }
    while(curr != NULL) {
        if(curr->next == block) {
            curr->next = block->next;
            return;
        }
        curr = curr->next;
    }
}

/**
 * Coalesce neighboring free blocks
 *
 * @param block The block to coalesce
 * @return A pointer to the first block of the coalesced blocks
 */
void *coalesce(free_block *block) {
    if (block == NULL) {
        return NULL;
    }

    free_block *prev = find_prev(block);
    free_block *next = find_next(block);

    // Coalesce with previous block if it is contiguous.
    if (prev != NULL) {
        char *end_of_prev = (char *)prev + prev->size + sizeof(free_block);
        if (end_of_prev == (char *)block) {
            prev->size += block->size + sizeof(free_block);

            // Ensure prev->next is updated to skip over 'block', only if 'block' is directly next to 'prev'.
            if (prev->next == block) {
                prev->next = block->next;
            }
            block = prev; // Update block to point to the new coalesced block.
        }
    }

    // Coalesce with next block if it is contiguous.
    if (next != NULL) {
        char *end_of_block = (char *)block + block->size + sizeof(free_block);
        if (end_of_block == (char *)next) {
            block->size += next->size + sizeof(free_block);

            // Ensure block->next is updated to skip over 'next'.
            block->next = next->next;
        }
    }

    return block;
}

/**
 * Call sbrk to get memory from the OS
 *
 * @param size The amount of memory to allocate
 * @return A pointer to the allocated memory
 */
void *do_alloc(size_t size) {
    void *ptr = sbrk(0); // top of heap

    intptr_t addr_end = (intptr_t) ptr & (ALIGNMENT - 1); // find last digit
    intptr_t align;

    if (addr_end == 0) {
        align = 0;
    } else {
        align = ALIGNMENT - addr_end;
    }

    void *block_ptr = sbrk(size + sizeof(header) + (size_t) align);

    // if it fails
    if (block_ptr == (void *)-1) {
        printf("Error allocating memory with sbrk()\n");
        return NULL; // Allocation failed
    }

    void *head = (void *) ((intptr_t) block_ptr + align);

    header *head_data = (header *) head;
    head_data->size = size;
    head_data->magic = 12345;

    return block_ptr + sizeof(header) + align;
}

/**
 * Allocates memory for the end user
 *
 * @param size The amount of memory to allocate
 * @return A pointer to the requested block of memory
 */
void *tumalloc(size_t size) {
    if (HEAD == NULL) { // case if no free list
        void *block_ptr = do_alloc(size);

        return block_ptr;
    } 

    free_block *curr = NEXT;
    free_block *start = NEXT;
    while (curr != NULL || start) {
        if ((size + sizeof(header)) <= curr->size) {
            void *ptr = split(curr, size + sizeof(header));
            header *head = (header *) ptr;

            remove_free_block(ptr);

            head->size = size;
            head->magic = 12345;

            return (void *) head + sizeof(header);
        }
        if (curr->next == NULL) {
            curr = HEAD;
        } else {
            curr = curr->next;
        }
    }

    void *block_ptr = do_alloc(size);

    return block_ptr;
}

/**
 * Allocates and initializes a list of elements for the end user
 *
 * @param num How many elements to allocate
 * @param size The size of each element
 * @return A pointer to the requested block of initialized memory
 */
void *tucalloc(size_t num, size_t size) {
    void *block_ptr = tumalloc(num * size);

    memset(block_ptr, 0, num * size);

    return block_ptr;
}

/**
 * Reallocates a chunk of memory with a bigger size
 *
 * @param ptr A pointer to an already allocated piece of memory
 * @param new_size The new requested size to allocate
 * @return A new pointer containing the contents of ptr, but with the new_size
 */
void *turealloc(void *ptr, size_t new_size) {
    header *head = (header *) (ptr - sizeof(header));
    size_t old_size = head->size;

    tufree(ptr); // put the block back in the free list while keeping data stored there

    void *block_ptr = tumalloc(new_size);

    if (new_size <= old_size) { // check to make sure the right amount of data is copied
        memcpy(block_ptr, ptr, new_size);
    } else {
        memcpy(block_ptr, ptr, old_size);
    }

    return block_ptr;
}

/**
 * Removes used chunk of memory and returns it to the free list
 *
 * @param ptr Pointer to the allocated piece of memory
 */
void tufree(void *ptr) {
    free_block *new = ptr - sizeof(header);

    new->size = sizeof(ptr + sizeof(header));
    new->next = NULL;

    if (HEAD == NULL) { // start the free list if needed
        HEAD = new;
    } else if (HEAD->next == NULL) { // add second element to free list if needed
        HEAD->next = new;
    } else { // free list is long enough
        free_block *curr = HEAD;

        uintptr_t new_addr = (uintptr_t) new;
        uintptr_t curr_next_addr = (uintptr_t) curr->next;

        while (curr->next != NULL && curr_next_addr < new_addr) { // loop through address-ordered free list
            curr = curr->next;
            curr_next_addr = (uintptr_t) curr;
        }

        new->next = curr->next;
        curr->next = new;
    } 

    if (new == new->next) { // in case a block wants to point to itself
        new->next = NULL;
    }
    
    coalesce(new);
}
