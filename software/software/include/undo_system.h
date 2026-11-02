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

#endif /* UNDO_SYSTEM_H */
