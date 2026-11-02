#include "engine_abi.h"
#include "gl_functions.h"
#include "mesh.h"
#include "vector3d.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <GL/gl.h>
#include <math.h>

#define MAX_LINE 2048
#define INIT_BUF 4096

typedef struct { float x, y, z; } Vec3f;

static void _compute_triangle_normal(const Vec3f* v0, const Vec3f* v1, const Vec3f* v2, float* out_n) {
    float e1x = v1->x - v0->x, e1y = v1->y - v0->y, e1z = v1->z - v0->z;
    float e2x = v2->x - v0->x, e2y = v2->y - v0->y, e2z = v2->z - v0->z;
    float nx = e1y*e2z - e1z*e2y;
    float ny = e1z*e2x - e1x*e2z;
    float nz = e1x*e2y - e1y*e2x;
    float len = sqrtf(nx*nx + ny*ny + nz*nz);
    if (len > 0.0001f) { nx/=len; ny/=len; nz/=len; }
    out_n[0] = nx; out_n[1] = ny; out_n[2] = nz;
}

Mesh* mesh_load_obj(const char* filename) {
    DEFER_FILE(fp) = fopen(filename, "r");
    if (!fp) return NULL;

    Mesh* m = calloc(1, sizeof(Mesh));
    if (!m) return NULL;

    uint32_t vcap = INIT_BUF, ncap = INIT_BUF;
    Vec3f* raw_v = malloc(vcap * sizeof(Vec3f));
    Vec3f* raw_n = malloc(ncap * sizeof(Vec3f));
    uint32_t vc = 0, nc = 0;
    int has_normals = 0;

    uint32_t pbuf_cap = 16384, ibuf_cap = 65536;
    m->positions = malloc(pbuf_cap * sizeof(float) * 3);
    m->normals   = malloc(pbuf_cap * sizeof(float) * 3);
    m->indices   = malloc(ibuf_cap * sizeof(uint32_t));

    char line[MAX_LINE];
    while (fgets(line, MAX_LINE, fp)) {
        if (line[0] == '#') continue;
        
        if (line[0] == 'v') {
            float x, y, z;
            if (line[1] == 'n') {
                if (sscanf(line, "vn %f %f %f", &x, &y, &z) == 3) {
                    if (nc >= ncap) { ncap *= 2; raw_n = realloc(raw_n, ncap * sizeof(Vec3f)); }
                    raw_n[nc++] = (Vec3f){x, y, z};
                    has_normals = 1;
                }
            } else if (line[1] == ' ' || line[1] == '\t') {
                if (sscanf(line, "v %f %f %f", &x, &y, &z) == 3) {
                    if (vc >= vcap) { vcap *= 2; raw_v = realloc(raw_v, vcap * sizeof(Vec3f)); }
                    raw_v[vc++] = (Vec3f){x, y, z};
                }
            }
        } else if (line[0] == 'f') {
            int fidx[64];
            int fcount = 0;
            const char* p = line + 1;
            
            while (*p) {
                while (*p == ' ' || *p == '\t') p++;
                if (*p == '\0' || *p == '\n') break;
                
                char buf[32] = {0};
                int i = 0;
                while (*p && *p != ' ' && *p != '\t' && *p != '\n') {
                    if (*p == '/') {
                        while (*p && *p != ' ' && *p != '\t') p++;
                        break;
                    }
                    buf[i++] = *p++;
                }
                int idx = atoi(buf);
                fidx[fcount++] = (idx > 0) ? idx - 1 : (int)vc + idx;
            }

            /* Centroid fallback triangulation for concave/complex polygons */
            Vec3f centroid = {0.0f, 0.0f, 0.0f};
            for (int i = 0; i < fcount; i++) {
                centroid.x += raw_v[fidx[i]].x;
                centroid.y += raw_v[fidx[i]].y;
                centroid.z += raw_v[fidx[i]].z;
            }
            centroid.x /= fcount; centroid.y /= fcount; centroid.z /= fcount;

            /* Emit triangles from centroid to each edge */
            for (int i = 0; i < fcount; i++) {
                int next = (i + 1) % fcount;
                Vec3f v0 = centroid;
                Vec3f v1 = raw_v[fidx[i]];
                Vec3f v2 = raw_v[fidx[next]];

                float tri_normal[3];
                if (has_normals) {
                    /* Interpolate or just compute geometric normal */
                    _compute_triangle_normal(&v0, &v1, &v2, tri_normal);
                } else {
                    _compute_triangle_normal(&v0, &v1, &v2, tri_normal);
                }

                /* Add 3 vertices per triangle */
                for (int j = 0; j < 3; j++) {
                    if (m->vertex_count * 3 + 3 >= pbuf_cap) {
                        pbuf_cap *= 2;
                        m->positions = realloc(m->positions, pbuf_cap * sizeof(float) * 3);
                        m->normals   = realloc(m->normals, pbuf_cap * sizeof(float) * 3);
                    }
                    if (m->index_count + 1 >= ibuf_cap) {
                        ibuf_cap *= 2;
                        m->indices = realloc(m->indices, ibuf_cap * sizeof(uint32_t));
                    }
                    uint32_t base = m->vertex_count * 3;
                    if (j == 0) { m->positions[base] = v0.x; m->positions[base+1] = v0.y; m->positions[base+2] = v0.z; }
                    else if (j == 1) { m->positions[base] = v1.x; m->positions[base+1] = v1.y; m->positions[base+2] = v1.z; }
                    else { m->positions[base] = v2.x; m->positions[base+1] = v2.y; m->positions[base+2] = v2.z; }

                    m->normals[base] = tri_normal[0];
                    m->normals[base+1] = tri_normal[1];
                    m->normals[base+2] = tri_normal[2];
                    m->indices[m->index_count++] = m->vertex_count++;
                }
            }
        }
    }

    m->positions = realloc(m->positions, m->vertex_count * 3 * sizeof(float));
    m->normals   = realloc(m->normals, m->vertex_count * 3 * sizeof(float));
    m->indices   = realloc(m->indices, m->index_count * sizeof(uint32_t));
    
    free(raw_v); free(raw_n);
    return m;
}

void mesh_destroy(Mesh* mesh) {
    if (!mesh) return;
    if (mesh->uploaded) {
        glDeleteVertexArrays(1, &mesh->vao);
        glDeleteBuffers(1, &mesh->vbo_pos);
        glDeleteBuffers(1, &mesh->vbo_nrm);
        glDeleteBuffers(1, &mesh->ebo);
    }
    free(mesh->positions);
    free(mesh->normals);
    free(mesh->indices);
    free(mesh);
}

void mesh_render(Mesh* mesh) {
    if (!mesh || mesh->index_count == 0) return;
    if (!mesh->uploaded) {
        glGenVertexArrays(1, &mesh->vao);
        glBindVertexArray(mesh->vao);

        glGenBuffers(1, &mesh->vbo_pos);
        glBindBuffer(GL_ARRAY_BUFFER, mesh->vbo_pos);
        glBufferData(GL_ARRAY_BUFFER, mesh->vertex_count * 3 * sizeof(float), mesh->positions, GL_STATIC_DRAW);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);
        glEnableVertexAttribArray(0);

        glGenBuffers(1, &mesh->vbo_nrm);
        glBindBuffer(GL_ARRAY_BUFFER, mesh->vbo_nrm);
        glBufferData(GL_ARRAY_BUFFER, mesh->vertex_count * 3 * sizeof(float), mesh->normals, GL_STATIC_DRAW);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);
        glEnableVertexAttribArray(1);

        glGenBuffers(1, &mesh->ebo);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh->ebo);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, mesh->index_count * sizeof(uint32_t), mesh->indices, GL_STATIC_DRAW);

        glBindVertexArray(0);
        mesh->uploaded = 1;
    }
    glBindVertexArray(mesh->vao);
    glDrawElements(GL_TRIANGLES, mesh->index_count, GL_UNSIGNED_INT, (void*)0);
    glBindVertexArray(0);
}

uint32_t mesh_get_triangle_count(const Mesh* mesh) {
    return mesh ? (mesh->index_count / 3) : 0;
}
