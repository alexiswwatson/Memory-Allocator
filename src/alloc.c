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
    uintptr_t ptr_addr = (uintptr_t) ptr;

    int addr_end = ptr_addr & 0xF; // find last digit
    int align = ALIGNMENT - addr_end;

    void *block_ptr = sbrk(size + (size_t) align);

    // if it fails
    if (block_ptr == (void *)-1) {
        printf("Error allocating memory with sbrk()\n");
        return NULL; // Allocation failed
    }
    return block_ptr;
}

/**
 * Allocates memory for the end user
 *
 * @param size The amount of memory to allocate
 * @return A pointer to the requested block of memory
 */
void *tumalloc(size_t size) {
    if (HEAD == NULL) { // first case if no free list
        header *head = do_alloc(size + sizeof(header));
        NEXT = HEAD;
        void *block_ptr = head + sizeof(header);
        head->size = size;
        return block_ptr;
    } 
    else if (HEAD->next == NULL) { // second case if free list has only one element
        if (size > HEAD->size) { // if requested size is too large, allocate more
            header *head = do_alloc(size + sizeof(header));
            NEXT = HEAD;
            void *block_ptr = head + sizeof(header);
            head->size = size;
            return block_ptr;
        } else if (size == HEAD->size) { // if requested size is a perfect fit, replace the block
            void *block_ptr = HEAD;
            HEAD = NULL;
            NEXT = HEAD;
            return block_ptr;
        }

        free_block *head = split(HEAD, size + sizeof(header));
        void *new_block = head + sizeof(header);
        
        remove_free_block(head);

        NEXT = HEAD;
        head->size = size;

        return new_block;
    }

    if (NEXT == NULL) { // next-fit pointer calibration
        NEXT = HEAD;
    }

    free_block *start = NEXT;
    free_block *curr = NEXT;
    int stop = 0;

    while (curr->next && curr->next != start) { // make one valid loop around the free list
        free_block *prev = find_prev(curr);

        if (size == curr->size) { // easy case if perfect fit
            prev->next = curr->next;
            NEXT = curr->next;

            return curr;
        } else if (size < curr->size) { // if there is room to fit
            free_block *head = split(curr, size + sizeof(header));

            remove_free_block(head);

            void *block_ptr = head + sizeof(header);

            head->size = size;

            return block_ptr;
        }

        if (curr->next == NULL) { // loop back around if end reached
            curr->next = HEAD;
        }

        curr = curr->next;
    }

    header *head = do_alloc(size + sizeof(header));
    void *block_ptr = head + sizeof(header);

    head->size = size;

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
    tufree(ptr); // put the block back in the free list while keeping data stored there
    
    header *head = tumalloc(new_size + sizeof(header));
    size_t old_size = head->size;
    void *block_ptr = ptr;

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
