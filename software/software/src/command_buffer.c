#include "engine_abi.h"
#include "vector3d.h"
#include "node.h"
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#define CMD_BUFFER_CAPACITY 64
#define GROUP_BUFFER_SIZE 16

/* Local definitions removed from typedef conflicts; defined cleanly here */
typedef enum { CMD_MOVE, CMD_ROTATE, CMD_SCALE, CMD_GROUP } CommandType;

typedef struct Command {
    CommandType type;
    Vec3 delta;
    uint32_t target_id;
    Node* target_node;
    /* Group support */
    struct Command* group_cmds;
    int group_count;
} Command;

struct CommandBuffer {
    Command* commands;
    int capacity;
    int count;
    int current;
    int is_grouping;
    Command* group_buffer;
    int group_count;
    int group_capacity;
};

static void transform_apply(Node* target, Vec3 delta) {
    if (!target) return;
    Vec3 current = target->position;
    node_set_position(target, vec3_add(current, delta));
}

CommandBuffer* cmd_buffer_create(int capacity) {
    if (capacity <= 0 || capacity > CMD_BUFFER_CAPACITY) capacity = CMD_BUFFER_CAPACITY;
    CommandBuffer* cb = (CommandBuffer*)calloc(1, sizeof(CommandBuffer));
    if (!cb) return NULL;
    cb->commands = (Command*)calloc(capacity, sizeof(Command));
    if (!cb->commands) { free(cb); return NULL; }
    cb->capacity = capacity;
    cb->count = 0;
    cb->current = -1;
    
    cb->group_capacity = GROUP_BUFFER_SIZE;
    cb->group_buffer = (Command*)calloc(cb->group_capacity, sizeof(Command));
    cb->is_grouping = 0;
    cb->group_count = 0;
    return cb;
}

void cmd_buffer_destroy(CommandBuffer* cb) {
    if (cb) {
        for(int i=0; i<cb->count; i++) {
            if(cb->commands[i].type == CMD_GROUP && cb->commands[i].group_cmds) {
                free(cb->commands[i].group_cmds);
            }
        }
        free(cb->commands);
        free(cb->group_buffer);
        free(cb);
    }
}

void cmd_buffer_set_target(CommandBuffer* cb, Node* node) {
    if (cb && cb->current >= 0 && cb->current < cb->count) {
        cb->commands[cb->current].target_node = node;
    }
}

void cmd_buffer_begin_group(CommandBuffer* cb) {
    if (!cb) return;
    cb->is_grouping = 1;
    cb->group_count = 0;
}

void cmd_buffer_end_group(CommandBuffer* cb) {
    if (!cb || !cb->is_grouping) return;
    
    Command group_cmd;
    group_cmd.type = CMD_GROUP;
    group_cmd.group_cmds = (Command*)malloc(cb->group_count * sizeof(Command));
    if (group_cmd.group_cmds) {
        memcpy(group_cmd.group_cmds, cb->group_buffer, cb->group_count * sizeof(Command));
        group_cmd.group_count = cb->group_count;
    } else {
        group_cmd.group_cmds = NULL;
        group_cmd.group_count = 0;
    }
    
    cb->current++;
    if (cb->current >= cb->capacity) cb->current = 0;
    cb->commands[cb->current] = group_cmd;
    if (cb->count < cb->capacity) cb->count++;
    
    cb->is_grouping = 0;
}

static void _apply_command(Command* cmd, int reverse) {
    if (!cmd) return;
    
    if (cmd->type == CMD_GROUP) {
        for (int i = 0; i < cmd->group_count; i++) {
            if (cmd->group_cmds) _apply_command(&cmd->group_cmds[i], reverse);
        }
        return;
    }

    float sign = reverse ? -1.0f : 1.0f;
    Vec3 delta = vec3_mul(cmd->delta, sign);
    
    switch (cmd->type) {
        case CMD_MOVE:
            transform_apply(cmd->target_node, delta);
            break;
        case CMD_ROTATE:
        case CMD_SCALE:
        case CMD_GROUP:
            break;
    }
}

void cmd_buffer_execute(CommandBuffer* cb, Command cmd) {
    if (!cb) return;
    
    if (cb->is_grouping) {
        if (cb->group_count < cb->group_capacity) {
            cb->group_buffer[cb->group_count++] = cmd;
        }
    } else {
        _apply_command(&cmd, 0);
        cb->current++;
        if (cb->current >= cb->capacity) cb->current = 0;
        cb->commands[cb->current] = cmd;
        if (cb->count < cb->capacity) cb->count++;
    }
}

void cmd_buffer_undo(CommandBuffer* cb) {
    if (!cb || cb->count == 0 || cb->current < 0) return;
    _apply_command(&cb->commands[cb->current], 1);
    cb->current--;
    if (cb->current < 0) cb->current = cb->capacity - 1;
}

void cmd_buffer_redo(CommandBuffer* cb) {
    if (!cb || cb->count == 0) return;
    cb->current++;
    if (cb->current >= cb->count) cb->current = 0;
    _apply_command(&cb->commands[cb->current], 0);
}

int cmd_buffer_is_dirty(CommandBuffer* cb) {
    return cb && cb->count > 0;
}

void cmd_buffer_clear(CommandBuffer* cb) {
    if (!cb) return;
    for(int i=0; i<cb->count; i++) {
        if(cb->commands[i].type == CMD_GROUP && cb->commands[i].group_cmds) {
            free(cb->commands[i].group_cmds);
            cb->commands[i].group_cmds = NULL;
        }
    }
    cb->count = 0;
    cb->current = -1;
}
