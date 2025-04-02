#include "alloc.h"

#include <stdio.h>

/**
 * A simple linked list implementation to test the allocator
 */
typedef struct node {
    int data; // The data stored in the list
    struct node *next; // The next element in the list
} node;

/**
 * Create a new list with a single element
 *
 * @param data The data to store in the list
 * @return A pointer to the new list
 */
node *list_new(int data) {
    // Allocate memory for the list
    node *list = (node *)tumalloc(sizeof(node));

    // Check if the allocation was successful
    if (list == NULL) {
        return NULL;
    }

    // Initialize the list
    list->next = NULL;
    list->data = data;

    return list;
}

/**
 * Add a new element to the end of the list
 *
 * @param list The list to add to
 * @param data The data to store in the new element
 */
void list_add(node *list, int data) {
    // Find the end of the list
    node *curr = list;
    while (curr->next != NULL) {
        curr = curr->next;
    }

    // Add a new element to the end of the list
    curr->next = (node *)tumalloc(sizeof(node));

    // Check if the allocation was successful
    if (curr->next == NULL) {
        return;
    }

    // Initialize the new element
    curr->next->data = data;
    curr->next->next = NULL;
}

/**
 * Remove an element from the list
 *
 * @param list The list to remove from
 * @param index The index of the element to remove
 * @return 0 if the element was removed, -1 if the element was not found
 */
int list_remove(node **list, int index) {
    // Check if the list is empty
    if(*list == NULL) {
        return -1;
    }

    // Remove the first element if the index is 0
    node *curr = *list;
    if (index == 0) {
        *list = curr->next;
        tufree(curr);
        return 0;
    }

    // Find the element to remove
    int i = 0;
    while (curr != NULL && i < index - 1) {
        curr = curr->next;
        i++;
    }

    // Check if the element was found
    if (curr == NULL || curr->next == NULL) {
        return -1;
    }

    // Remove the element
    node *next = curr->next->next;
    tufree(curr->next);
    curr->next = next;

    return 0;
}

/**
 * Remove all elements from the list
 *
 * @param list The list to remove elements from
 */
void list_remove_all(node *list) {
    node *curr = list;
    while (curr) {
        node *next = curr->next;
        tufree(curr);
        curr = next;
    }
}

/**
 * Print all elements in the list
 *
 * @param list The list to print
 */
void list_print_all(node *list) {
    node *curr = list;
    while (curr) {
        printf("%d\n", curr->data);
        curr = curr->next;
    }
}

// The head of the list
static node *HEAD = NULL;

/**
 * Main function to test the allocator
 */
int main(int argc, char** argv) {
    printf("Main\n");
    // Allocate some memory
    int *thing = tumalloc(5*sizeof(int));

    // Check if the allocation was successful
    if(thing == NULL) {
        printf("Failed to allocate memory\n");
        return 1;
    }

    // Set some values in the allocated memory
    thing[0] = 5;
    thing[1]= 10;
    thing[2] = 20;
    thing[3] = 30;
    thing[4] = 40;

    // Print the first value
    printf("%d\n", *thing);

    // Allocate memory for an int array
    int *other_thing = tumalloc(5*sizeof(int));

    // Check if the allocation was successful
    if(other_thing == NULL) {
        printf("Failed to allocate memory\n");
        return 1;
    }

    // Set some values in the allocated memory
    other_thing[0] = 5;
    other_thing[1]= 10;
    other_thing[2] = 20;
    other_thing[3] = 30;
    other_thing[4] = 40;

    // Print the first value
    printf("%d\n", *other_thing);

    // Free the allocated memory
    tufree(thing);
    tufree(other_thing);

    // Create a new list
    HEAD = list_new(5);

    // Check if the allocation was successful
    if(HEAD == NULL) {
        printf("Failed to allocate memory\n");
        return 1;
    }

    // Add some elements to the list
    list_add(HEAD, 10);
    list_add(HEAD, 20);
    list_add(HEAD, 30);
    list_add(HEAD, 40);

    // Print all elements in the list
    list_print_all(HEAD);

    // Remove an element from the list
    int ret = list_remove(&HEAD, 0);

    // Check if the removal was successful
    if(ret != 0) {
        printf("Failed to remove element\n");
        return 1;
    }

    // Print all elements in the list
    list_print_all(HEAD);

    // Remove all elements from the list
    list_remove_all(HEAD);

    // // Allocate memory and initialize to 0
    // int *more_things = tucalloc(10, sizeof(int));

    // // Check if the allocation was successful
    // if(more_things == NULL) {
    //     printf("Failed to allocate memory\n");
    //     return 1;
    // }

    // // Set some values in the allocated memory
    // more_things[0] = 5;
    // more_things[1] = 10;
    // more_things[2] = 20;
    // more_things[3] = 30;
    // more_things[4] = 40;
    // more_things[6] = 60;
    // more_things[7] = 70;
    // more_things[8] = 80;
    // more_things[9] = 90;

    // // Print all elements in the list
    // for(int i=0; i<10; i++) {
    //     printf("%d\n", more_things[i]);
    // }

    // // Reallocate memory
    // int *bigger_things = turealloc(more_things, 20*sizeof(int));

    // // Check if the reallocation was successful
    // if(bigger_things == NULL) {
    //     printf("Failed to allocate memory\n");
    //     return 1;
    // }

    // // Set some values in the allocated memory
    // for(int i=10; i<20; i++) {
    //     bigger_things[i] = i*10;
    // }

    // // Print all elements in the list
    // for(int i=0; i<20; i++) {
    //     printf("%d\n", bigger_things[i]);
    // }

    // // Free the allocated memory
    // tufree(more_things);

    return 0;
}
