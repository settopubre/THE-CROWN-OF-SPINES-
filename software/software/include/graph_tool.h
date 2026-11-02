#ifndef GRAPH_TOOL_H
#define GRAPH_TOOL_H
#pragma once

#include "vector3d.h"

typedef struct {
    int node_count;
    Vec3* nodes;
    double** distance_matrix;
    double** adjacency_matrix;
    double sigma;
    double threshold;
    int k_nearest;
} GraphTool;

#ifdef __cplusplus
extern "C" {
#endif

GraphTool* graph_tool_create(void);
void graph_tool_destroy(GraphTool* graph);
int graph_tool_load_nodes_from_flat_dir(GraphTool* graph, const char* dir_path);
int graph_tool_compute_distance_matrix(GraphTool* graph);
int graph_tool_compute_gaussian_adjacency(GraphTool* graph);

int graph_tool_export_distance_matrix_csv(const GraphTool* graph, const char* filename);
int graph_tool_export_adjacency_matrix_csv(const GraphTool* graph, const char* filename);
int graph_tool_export_node_positions_csv(const GraphTool* graph, const char* filename);
int graph_tool_export_obj(const GraphTool* graph, const char* filename);
int graph_tool_export_edges_json(const GraphTool* graph, const char* filename);
int graph_tool_export_metadata_csv(const GraphTool* graph, const char* filename);

void graph_tool_print_summary(const GraphTool* graph);

#ifdef __cplusplus
}
#endif

#endif
