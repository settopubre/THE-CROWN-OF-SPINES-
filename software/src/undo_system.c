/* Header: include/undo_system.h
#ifndef UNDO_SYSTEM_H
#define UNDO_SYSTEM_H
#include <stddef.h>
typedef struct UndoSystem UndoSystem;
UndoSystem* undo_create(int capacity);
void undo_push(UndoSystem* us, void* state, size_t state_size, void (*free_func)(void*));
void* undo_undo(UndoSystem* us);
void* undo_redo(UndoSystem* us);
void undo_clear(UndoSystem* us);
void undo_destroy(UndoSystem* us);
#endif
*/

#include "undo_system.h"
#include <stdlib.h>
#include <string.h>

typedef struct {
    void* data;
    size_t size;
    void (*free_func)(void*);
} UndoEntry;

struct UndoSystem {
    UndoEntry* entries;
    int capacity;
    int count;
    int current;
};

UndoSystem* undo_create(int capacity) {
    if (capacity <= 0) capacity = 32;
    UndoSystem* us = (UndoSystem*)calloc(1, sizeof(UndoSystem));
    if (!us) return NULL;
    
    us->capacity = capacity;
    us->entries = (UndoEntry*)calloc(capacity, sizeof(UndoEntry));
    us->count = 0;
    us->current = -1;
    
    return us;
}

void undo_push(UndoSystem* us, void* state, size_t state_size, void (*free_func)(void*)) {
    if (!us || !state) return;
    
    /* If we are in the middle of history, discard future */
    if (us->current < us->count - 1) {
        for (int i = us->current + 1; i < us->count; i++) {
            if (us->entries[i].data) us->entries[i].free_func(us->entries[i].data);
        }
        us->count = us->current + 1;
    }
    
    /* Resize buffer if full */
    if (us->count >= us->capacity) {
        /* Drop oldest */
        if (us->entries[0].data) us->entries[0].free_func(us->entries[0].data);
        
        for (int i = 0; i < us->count - 1; i++) {
            us->entries[i] = us->entries[i+1];
        }
        us->count--;
        us->current--;
    }
    
    /* Deep copy state */
    void* new_data = malloc(state_size);
    if (new_data) {
        memcpy(new_data, state, state_size);
        us->entries[us->count].data = new_data;
        us->entries[us->count].size = state_size;
        us->entries[us->count].free_func = free_func;
        us->count++;
        us->current++;
    }
}

void* undo_undo(UndoSystem* us) {
    if (!us || us->current < 1) return NULL;
    us->current--;
    return us->entries[us->current].data;
}

void* undo_redo(UndoSystem* us) {
    if (!us || us->current >= us->count - 1) return NULL;
    us->current++;
    return us->entries[us->current].data;
}

void undo_clear(UndoSystem* us) {
    if (!us) return;
    for (int i = 0; i < us->count; i++) {
        if (us->entries[i].data) us->entries[i].free_func(us->entries[i].data);
    }
    us->count = 0;
    us->current = -1;
}

void undo_destroy(UndoSystem* us) {
    if (us) {
        undo_clear(us);
        free(us->entries);
        free(us);
    }
}
