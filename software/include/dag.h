#ifndef DAG_H
#define DAG_H
#pragma once

#include <stdint.h>

typedef struct DAGNode {
    uint32_t id;
    char name[64];
    void* data;
    uint32_t* inputs;
    uint32_t input_count;
    uint32_t* outputs;
    uint32_t output_count;
    void (*func)(void*);
    int evaluated;
} DAGNode;

typedef struct {
    DAGNode* nodes;
    uint32_t capacity;
    uint32_t count;
} DAG;

#ifdef __cplusplus
extern "C" {
#endif

DAG* dag_create(uint32_t capacity);
void dag_destroy(DAG* dag);
uint32_t dag_add_node(DAG* dag, const char* name, void* data);
void dag_connect(DAG* dag, uint32_t from_node, uint32_t to_node);
int dag_evaluate(DAG* dag);
void dag_set_node_func(DAG* dag, uint32_t node_id, void (*func)(void*));

#ifdef __cplusplus
}
#endif

#endif /* DAG_H */
