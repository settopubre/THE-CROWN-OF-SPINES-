#include "mesh.h"
#include "vector3d.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <float.h>

typedef struct {
    double Q[10];
} Quadric;

typedef struct {
    uint32_t v0, v1;
    double cost;
    Vec3 optimal;
} Edge;

static void _quadric_init(Quadric* q) {
    memset(q, 0, sizeof(Quadric));
    q->Q[9] = 1.0;
}

static void _quadric_from_plane(Quadric* q, float a, float b, float c, float d) {
    _quadric_init(q);
    q->Q[0] = a*a; q->Q[1] = a*b; q->Q[2] = a*c; q->Q[3] = a*d;
    q->Q[4] = b*b; q->Q[5] = b*c; q->Q[6] = b*d;
    q->Q[7] = c*c; q->Q[8] = c*d;
}

static double _quadric_error(Quadric* q, float x, float y, float z) {
    return q->Q[0]*x*x + 2*q->Q[1]*x*y + 2*q->Q[2]*x*z + 2*q->Q[3]*x +
           q->Q[4]*y*y + 2*q->Q[5]*y*z + 2*q->Q[6]*y +
           q->Q[7]*z*z + 2*q->Q[8]*z + q->Q[9];
}

static int _compare_edges(const void* a, const void* b) {
    const Edge* ea = (const Edge*)a;
    const Edge* eb = (const Edge*)b;
    if (ea->cost < eb->cost) return -1;
    if (ea->cost > eb->cost) return 1;
    return 0;
}

Mesh* decimate_mesh(Mesh* mesh, uint32_t target_tris) {
    if (!mesh || mesh->index_count / 3 <= target_tris) return mesh;
    
    uint32_t tri_count = mesh->index_count / 3;
    uint32_t vert_count = mesh->vertex_count;
    
    Quadric* quadrics = (Quadric*)calloc(vert_count, sizeof(Quadric));
    if (!quadrics) return mesh;
    
    for (uint32_t t = 0; t < tri_count; t++) {
        uint32_t i0 = mesh->indices[t*3], i1 = mesh->indices[t*3+1], i2 = mesh->indices[t*3+2];
        float* v0 = &mesh->positions[i0*3];
        float* v1 = &mesh->positions[i1*3];
        float* v2 = &mesh->positions[i2*3];
        
        float e1[3] = {v1[0]-v0[0], v1[1]-v0[1], v1[2]-v0[2]};
        float e2[3] = {v2[0]-v0[0], v2[1]-v0[1], v2[2]-v0[2]};
        float n[3] = {e1[1]*e2[2]-e1[2]*e2[1], e1[2]*e2[0]-e1[0]*e2[2], e1[0]*e2[1]-e1[1]*e2[0]};
        float len = sqrtf(n[0]*n[0] + n[1]*n[1] + n[2]*n[2]);
        if (len < 1e-6) continue;
        n[0]/=len; n[1]/=len; n[2]/=len;
        float d = -(n[0]*v0[0] + n[1]*v0[1] + n[2]*v0[2]);
        
        Quadric tri_q;
        _quadric_from_plane(&tri_q, n[0], n[1], n[2], d);
        
        for (int j = 0; j < 10; j++) quadrics[i0].Q[j] += tri_q.Q[j];
        for (int j = 0; j < 10; j++) quadrics[i1].Q[j] += tri_q.Q[j];
        for (int j = 0; j < 10; j++) quadrics[i2].Q[j] += tri_q.Q[j];
    }
    
    Edge* edges = (Edge*)malloc(tri_count * 3 * sizeof(Edge));
    if (!edges) { free(quadrics); return mesh; }
    
    uint32_t edge_count = 0;
    for (uint32_t t = 0; t < tri_count; t++) {
        for (int e = 0; e < 3; e++) {
            uint32_t v0 = mesh->indices[t*3 + e];
            uint32_t v1 = mesh->indices[t*3 + (e+1)%3];
            if (v0 > v1) { uint32_t tmp = v0; v0 = v1; v1 = tmp; }
            
            int found = 0;
            for (uint32_t i = 0; i < edge_count; i++) {
                if (edges[i].v0 == v0 && edges[i].v1 == v1) { found = 1; break; }
            }
            if (found) continue;
            
            Edge* edge = &edges[edge_count++];
            edge->v0 = v0; edge->v1 = v1;
            
            Vec3 mid = vec3_create(
                (mesh->positions[v0*3] + mesh->positions[v1*3]) * 0.5f,
                (mesh->positions[v0*3+1] + mesh->positions[v1*3+1]) * 0.5f,
                (mesh->positions[v0*3+2] + mesh->positions[v1*3+2]) * 0.5f
            );
            edge->optimal = mid;
            
            Quadric combined;
            for (int j = 0; j < 10; j++) combined.Q[j] = quadrics[v0].Q[j] + quadrics[v1].Q[j];
            edge->cost = _quadric_error(&combined, mid.x, mid.y, mid.z);
        }
    }
    
    /* Replace O(n²) bubble sort with qsort */
    qsort(edges, edge_count, sizeof(Edge), _compare_edges);
    
    uint32_t* vertex_map = (uint32_t*)malloc(vert_count * sizeof(uint32_t));
    uint32_t* index_map = (uint32_t*)malloc(tri_count * 3 * sizeof(uint32_t));
    if (!vertex_map || !index_map) { free(edges); free(quadrics); free(vertex_map); free(index_map); return mesh; }
    
    for (uint32_t i = 0; i < vert_count; i++) vertex_map[i] = i;
    memcpy(index_map, mesh->indices, tri_count * 3 * sizeof(uint32_t));
    
    uint32_t current_tris = tri_count;
    for (uint32_t e = 0; e < edge_count && current_tris > target_tris; e++) {
        Edge* edge = &edges[e];
        uint32_t v0 = vertex_map[edge->v0], v1 = vertex_map[edge->v1];
        if (v0 == v1) continue;
        
        mesh->positions[v0*3] = edge->optimal.x;
        mesh->positions[v0*3+1] = edge->optimal.y;
        mesh->positions[v0*3+2] = edge->optimal.z;
        vertex_map[v1] = v0;
        
        uint32_t write_idx = 0;
        for (uint32_t t = 0; t < current_tris; t++) {
            uint32_t i0 = vertex_map[index_map[t*3]];
            uint32_t i1 = vertex_map[index_map[t*3+1]];
            uint32_t i2 = vertex_map[index_map[t*3+2]];
            if (i0 != i1 && i1 != i2 && i0 != i2) {
                index_map[write_idx*3] = i0;
                index_map[write_idx*3+1] = i1;
                index_map[write_idx*3+2] = i2;
                write_idx++;
            }
        }
        current_tris = write_idx;
    }
    
    uint32_t* new_indices = (uint32_t*)malloc(current_tris * 3 * sizeof(uint32_t));
    if (new_indices) {
        memcpy(new_indices, index_map, current_tris * 3 * sizeof(uint32_t));
        free(mesh->indices);
        mesh->indices = new_indices;
        mesh->index_count = current_tris * 3;
    }
    
    free(edges); free(quadrics); free(vertex_map); free(index_map);
    mesh->uploaded = 0;
    return mesh;
}
