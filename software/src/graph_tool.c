#define _POSIX_C_SOURCE 199309L
#include "vector3d.h"
#include "graph_tool.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <dirent.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>
#ifdef HAS_OPENMP
#include <omp.h>
#endif

#ifndef M_E
#define M_E 2.71828182845904523536
#endif

static int parse_node_file(const char* filepath, Vec3* out_vec, int id) {
    DEFER_FILE(fp) = fopen(filepath, "r");
    if (!fp) {
        fprintf(stderr, "ERROR: Cannot open %s\n", filepath);
        return 0;
    }
    double x, y, z;
    if (fscanf(fp, "%lf %lf %lf", &x, &y, &z) != 3) {
        fprintf(stderr, "ERROR: Invalid format in %s (expected: x y z)\n", filepath);
        return 0;
    }
    *out_vec = vec3_create(x, y, z);
    out_vec->id = id;
    return 1;
}

static int is_node_file(const char* name) {
    if (strncmp(name, "node_", 5) != 0) return 0;
    const char* ext = strrchr(name, '.');
    if (!ext) return 0;
    return (strcmp(ext, ".dat") == 0);
}

static int count_node_files(const char* dir_path) {
    DEFER_DIR(dir) = opendir(dir_path);
    if (!dir) {
        fprintf(stderr, "ERROR: Cannot open directory %s\n", dir_path);
        return -1;
    }
    int count = 0;
    struct dirent* entry;
    while ((entry = readdir(dir)) != NULL) {
        if (is_node_file(entry->d_name)) count++;
    }
    return count;
}

static int dir_exists(const char* path) {
    struct stat st;
    return (stat(path, &st) == 0 && S_ISDIR(st.st_mode));
}

static int file_is_empty_or_missing(const char* path) {
    struct stat st;
    if (stat(path, &st) != 0) return 1;
    return (st.st_size == 0);
}

static int needs_data_generation(const char* dir_path) {
    if (!dir_exists(dir_path)) {
        mkdir(dir_path, 0755);
        return 1;
    }
    char filepath[1024];
    snprintf(filepath, sizeof(filepath), "%s/node_001.dat", dir_path);
    return file_is_empty_or_missing(filepath);
}

static void clear_sim_data(const char* dir_path) {
    DEFER_DIR(dir) = opendir(dir_path);
    if (!dir) return;
    struct dirent* entry;
    char filepath[1024];
    while ((entry = readdir(dir)) != NULL) {
        if (is_node_file(entry->d_name)) {
            snprintf(filepath, sizeof(filepath), "%s/%s", dir_path, entry->d_name);
            unlink(filepath);
        }
    }
}

static void generate_sample_nodes(const char* dir_path, int count) {
    srand(42);
    char filepath[1024];
    for (int i = 0; i < count; i++) {
        snprintf(filepath, sizeof(filepath), "%s/node_%03d.dat", dir_path, i + 1);
        FILE* fp = fopen(filepath, "w");
        if (!fp) {
            fprintf(stderr, "FATAL: Cannot create %s\n", filepath);
            exit(1);
        }
        double x = (rand() % 1000) / 100.0 - 5.0;
        double y = (rand() % 1000) / 100.0 - 5.0;
        double z = (rand() % 1000) / 100.0 - 5.0;
        fprintf(fp, "%.6f %.6f %.6f\n", x, y, z);
        fclose(fp);
    }
}

static void print_help(const char* prog) {
    printf("3DAI Engine - Graph Tool (Offline)\n");
    printf("Usage: %s [-n NODES] [-s SIGMA] [-t THRESH] [-h]\n", prog);
    printf("  -n INT   Number of nodes to generate (default: 10)\n");
    printf("  -s FLOAT Sigma for Gaussian weighting (default: 2.0)\n");
    printf("  -t FLOAT Adjacency threshold (default: 0.01)\n");
    printf("  -h       Show this help\n");
}

GraphTool* graph_tool_create(void) {
    GraphTool* g = (GraphTool*)malloc(sizeof(GraphTool));
    if (!g) return NULL;
    memset(g, 0, sizeof(GraphTool));
    g->sigma = 1.0;
    g->threshold = 0.1;
    g->k_nearest = 5;
    return g;
}

void graph_tool_destroy(GraphTool* g) {
    if (!g) return;
    free(g->nodes);
    if (g->distance_matrix) {
        for (int i = 0; i < g->node_count; i++) free(g->distance_matrix[i]);
        free(g->distance_matrix);
    }
    if (g->adjacency_matrix) {
        for (int i = 0; i < g->node_count; i++) free(g->adjacency_matrix[i]);
        free(g->adjacency_matrix);
    }
    free(g);
}

int graph_tool_load_nodes_from_flat_dir(GraphTool* g, const char* dir_path) {
    if (!g || !dir_path) return 0;
    int file_count = count_node_files(dir_path);
    if (file_count <= 0) {
        fprintf(stderr, "ERROR: No node_*.dat files found in %s\n", dir_path);
        return 0;
    }
    printf("  Found %d node files\n", file_count);
    g->node_count = file_count;
    g->nodes = (Vec3*)malloc(file_count * sizeof(Vec3));
    if (!g->nodes) {
        fprintf(stderr, "ERROR: Memory allocation failed\n");
        return 0;
    }
    DEFER_DIR(dir) = opendir(dir_path);
    if (!dir) return 0;
    int loaded = 0;
    struct dirent* entry;
    char filepath[1024];
    while ((entry = readdir(dir)) != NULL && loaded < file_count) {
        if (!is_node_file(entry->d_name)) continue;
        snprintf(filepath, sizeof(filepath), "%s/%s", dir_path, entry->d_name);
        if (parse_node_file(filepath, &g->nodes[loaded], loaded)) loaded++;
    }
    g->node_count = loaded;
    printf("  Successfully loaded %d nodes\n", loaded);
    return 1;
}

int graph_tool_compute_distance_matrix(GraphTool* g) {
    if (!g || g->node_count == 0) return 0;
    int n = g->node_count;
    g->distance_matrix = (double**)malloc(n * sizeof(double*));
    if (!g->distance_matrix) return 0;
    for (int i = 0; i < n; i++) {
        g->distance_matrix[i] = (double*)malloc(n * sizeof(double));
        if (!g->distance_matrix[i]) return 0;
    }
    #ifdef HAS_OPENMP
    #pragma omp parallel for schedule(dynamic)
    #endif
    for (int i = 0; i < n; i++) {
        g->distance_matrix[i][i] = 0.0;
        for (int j = i + 1; j < n; j++) {
            double dist = vec3_distance(&g->nodes[i], &g->nodes[j]);
            g->distance_matrix[i][j] = dist;
            g->distance_matrix[j][i] = dist;
        }
    }
    #ifdef HAS_OPENMP
    printf("  [OPENMP] Threads active: %d\n", omp_get_max_threads());
    #endif
    printf("  Distance matrix computed: %dx%d\n", n, n);
    return 1;
}

int graph_tool_compute_gaussian_adjacency(GraphTool* g) {
    if (!g || !g->distance_matrix || g->node_count == 0) return 0;
    int n = g->node_count;
    g->adjacency_matrix = (double**)malloc(n * sizeof(double*));
    if (!g->adjacency_matrix) return 0;
    for (int i = 0; i < n; i++) {
        g->adjacency_matrix[i] = (double*)calloc(n, sizeof(double));
        if (!g->adjacency_matrix[i]) return 0;
    }
    double sigma_sq_2 = 2.0 * g->sigma * g->sigma;
    for (int i = 0; i < n; i++) {
        for (int j = i + 1; j < n; j++) {
            double d = g->distance_matrix[i][j];
            double w = exp(-(d * d) / sigma_sq_2);
            if (w >= g->threshold) {
                g->adjacency_matrix[i][j] = w;
                g->adjacency_matrix[j][i] = w;
            }
        }
    }
    int edge_count = 0;
    for (int i = 0; i < n; i++) {
        for (int j = i + 1; j < n; j++) {
            if (g->adjacency_matrix[i][j] > 0.0) edge_count++;
        }
    }
    printf("  Gaussian adjacency: %d edges (sigma=%.3f, threshold=%.3f)\n", 
           edge_count, g->sigma, g->threshold);
    return 1;
}

int graph_tool_export_distance_matrix_csv(const GraphTool* g, const char* filename) {
    if (!g || !g->distance_matrix || !filename) return 0;
    DEFER_FILE(fp) = fopen(filename, "w");
    if (!fp) return 0;
    int n = g->node_count;
    for (int i = 0; i < n; i++) {
        for (int j = 0; j < n; j++) {
            fprintf(fp, "%.6f%s", g->distance_matrix[i][j], (j < n - 1) ? "," : "\n");
        }
    }
    printf("  Exported: %s\n", filename);
    return 1;
}

int graph_tool_export_adjacency_matrix_csv(const GraphTool* g, const char* filename) {
    if (!g || !g->adjacency_matrix || !filename) return 0;
    DEFER_FILE(fp) = fopen(filename, "w");
    if (!fp) return 0;
    int n = g->node_count;
    for (int i = 0; i < n; i++) {
        for (int j = 0; j < n; j++) {
            fprintf(fp, "%.6f%s", g->adjacency_matrix[i][j], (j < n - 1) ? "," : "\n");
        }
    }
    printf("  Exported: %s\n", filename);
    return 1;
}

int graph_tool_export_node_positions_csv(const GraphTool* g, const char* filename) {
    if (!g || !g->nodes || !filename) return 0;
    DEFER_FILE(fp) = fopen(filename, "w");
    if (!fp) return 0;
    fprintf(fp, "id,x,y,z\n");
    for (int i = 0; i < g->node_count; i++) {
        fprintf(fp, "%d,%.6f,%.6f,%.6f\n", g->nodes[i].id, g->nodes[i].x, g->nodes[i].y, g->nodes[i].z);
    }
    printf("  Exported: %s\n", filename);
    return 1;
}

int graph_tool_export_obj(const GraphTool* g, const char* filename) {
    if (!g || !g->nodes || !filename) return 0;
    DEFER_FILE(fp) = fopen(filename, "w");
    if (!fp) return 0;
    int edge_count = 0;
    if (g->adjacency_matrix) {
        for (int i = 0; i < g->node_count; i++) {
            for (int j = i + 1; j < g->node_count; j++) {
                if (g->adjacency_matrix[i][j] > 0.0) edge_count++;
            }
        }
    }
    fprintf(fp, "# 3DAI Engine - Robot Spatial Graph | Nodes: %d | Edges: %d\n", g->node_count, edge_count);
    for (int i = 0; i < g->node_count; i++) {
        fprintf(fp, "v %.6f %.6f %.6f\n", g->nodes[i].x, g->nodes[i].y, g->nodes[i].z);
    }
    if (g->adjacency_matrix) {
        for (int i = 0; i < g->node_count; i++) {
            for (int j = i + 1; j < g->node_count; j++) {
                if (g->adjacency_matrix[i][j] > 0.0) {
                    fprintf(fp, "l %d %d\n", i + 1, j + 1);
                }
            }
        }
    }
    printf("  Exported: %s\n", filename);
    return 1;
}

int graph_tool_export_edges_json(const GraphTool* g, const char* filename) {
    if (!g || !g->adjacency_matrix || !g->distance_matrix || !filename) return 0;
    DEFER_FILE(fp) = fopen(filename, "w");
    if (!fp) return 0;
    fprintf(fp, "[\n");
    int first = 1;
    for (int i = 0; i < g->node_count; i++) {
        for (int j = i + 1; j < g->node_count; j++) {
            if (g->adjacency_matrix[i][j] > 0.0) {
                if (!first) fprintf(fp, ",\n");
                fprintf(fp, "  {\"src\":%d,\"dst\":%d,\"weight\":%.6f,\"dist\":%.6f}", i, j, g->adjacency_matrix[i][j], g->distance_matrix[i][j]);
                first = 0;
            }
        }
    }
    fprintf(fp, "\n]\n");
    printf("  Exported: %s\n", filename);
    return 1;
}

int graph_tool_export_metadata_csv(const GraphTool* g, const char* filename) {
    if (!g || !g->nodes || !filename) return 0;
    DEFER_FILE(fp) = fopen(filename, "w");
    if (!fp) return 0;
    fprintf(fp, "id,x,y,z,degree,avg_weight\n");
    for (int i = 0; i < g->node_count; i++) {
        int degree = 0;
        double weight_sum = 0.0;
        if (g->adjacency_matrix) {
            for (int j = 0; j < g->node_count; j++) {
                if (g->adjacency_matrix[i][j] > 0.0) { degree++; weight_sum += g->adjacency_matrix[i][j]; }
            }
        }
        double avg_weight = (degree > 0) ? weight_sum / degree : 0.0;
        fprintf(fp, "%d,%.6f,%.6f,%.6f,%d,%.6f\n", g->nodes[i].id, g->nodes[i].x, g->nodes[i].y, g->nodes[i].z, degree, avg_weight);
    }
    printf("  Exported: %s\n", filename);
    return 1;
}

void graph_tool_print_summary(const GraphTool* g) {
    if (!g) return;
    printf("\n=== GRAPH SUMMARY ===\n");
    printf("Nodes: %d\n", g->node_count);
    printf("Sigma: %.3f\n", g->sigma);
    printf("Threshold: %.3f\n", g->threshold);
    if (g->nodes && g->node_count > 0) {
        printf("\nFirst 3 nodes:\n");
        int show = (g->node_count < 3) ? g->node_count : 3;
        for (int i = 0; i < show; i++) {
            printf("  Node %d: (%.3f, %.3f, %.3f)\n", g->nodes[i].id, g->nodes[i].x, g->nodes[i].y, g->nodes[i].z);
        }
    }
    if (g->adjacency_matrix && g->node_count > 0) {
        printf("\nAdjacency matrix (first 5x5):\n");
        int show = (g->node_count < 5) ? g->node_count : 5;
        for (int i = 0; i < show; i++) {
            printf("  ");
            for (int j = 0; j < show; j++) printf("%.3f ", g->adjacency_matrix[i][j]);
            printf("\n");
        }
    }
    printf("======================\n\n");
}

#ifdef STANDALONE_GRAPH_TOOL
int main(int argc, char* argv[]) {
    int node_count = 10;
    double sigma = 2.0;
    double threshold = 0.01;
    
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-n") == 0 && i + 1 < argc) node_count = atoi(argv[++i]);
        else if (strcmp(argv[i], "-s") == 0 && i + 1 < argc) sigma = atof(argv[++i]);
        else if (strcmp(argv[i], "-t") == 0 && i + 1 < argc) threshold = atof(argv[++i]);
        else if (strcmp(argv[i], "-h") == 0) { print_help(argv[0]); return 0; }
        else { fprintf(stderr, "Unknown option: %s\n", argv[i]); print_help(argv[0]); return 1; }
    }
    
    printf("\n========================================\n");
    printf("        3DAI ENGINE - GRAPH TOOL        \n");
    printf("========================================\n");
    printf("[CONFIG] nodes=%d sigma=%.3f threshold=%.3f\n\n", node_count, sigma, threshold);
    
    struct timespec t_start, t_end;
    clock_gettime(CLOCK_MONOTONIC, &t_start);
    
    if (needs_data_generation("sim_data")) {
        printf("[PREP] Generating sample data...\n");
        generate_sample_nodes("sim_data", node_count);
    } else {
        int existing = count_node_files("sim_data");
        if (existing != node_count) {
            printf("[SCALE] Node count changed (%d -> %d). Clearing sim_data/...\n", existing, node_count);
            clear_sim_data("sim_data");
            generate_sample_nodes("sim_data", node_count);
        }
    }
    if (!dir_exists("output")) mkdir("output", 0755);
    
    GraphTool* graph = graph_tool_create();
    if (!graph) { fprintf(stderr, "FATAL: Failed to create graph\n"); return 1; }
    graph->sigma = sigma;
    graph->threshold = threshold;
    
    printf("[1/5] Loading nodes...\n");
    if (!graph_tool_load_nodes_from_flat_dir(graph, "sim_data")) { graph_tool_destroy(graph); return 1; }
    printf("[1/5] Loading nodes... DONE\n\n");
    
    printf("[2/5] Computing distance matrix...\n");
    if (!graph_tool_compute_distance_matrix(graph)) { graph_tool_destroy(graph); return 1; }
    printf("[2/5] Computing distance matrix... DONE\n\n");
    
    printf("[3/5] Building Gaussian adjacency...\n");
    if (!graph_tool_compute_gaussian_adjacency(graph)) { graph_tool_destroy(graph); return 1; }
    printf("[3/5] Building Gaussian adjacency... DONE\n\n");
    
    printf("[4/5] Exporting data files...\n");
    graph_tool_export_node_positions_csv(graph, "output/node_positions.csv");
    graph_tool_export_distance_matrix_csv(graph, "output/distance_matrix.csv");
    graph_tool_export_adjacency_matrix_csv(graph, "output/adjacency_matrix.csv");
    graph_tool_export_obj(graph, "output/robot_scene.obj");
    graph_tool_export_edges_json(graph, "output/edges.json");
    graph_tool_export_metadata_csv(graph, "output/metadata.csv");
    printf("[4/5] Exporting data files... DONE\n");
    
    printf("[5/5] Printing summary...\n");
    graph_tool_print_summary(graph);
    graph_tool_destroy(graph);
    
    clock_gettime(CLOCK_MONOTONIC, &t_end);
    double elapsed = (t_end.tv_sec - t_start.tv_sec) + (t_end.tv_nsec - t_start.tv_nsec) * 1e-9;
    
    printf("\n========================================\n");
    printf("    PIPELINE COMPLETE - READY FOR R     \n");
    printf("[PERF] Execution time: %.4f seconds\n", elapsed);
    printf("========================================\n\n");
    return 0;
}
#endif
