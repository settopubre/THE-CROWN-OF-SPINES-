#include "rebar_clash.h"
#include "gl_functions.h"
#include <stdlib.h>
#include <math.h>
#include <string.h>

#define GRID_HASH_SIZE 1024

typedef struct {
    int* bars;
    int count;
    int capacity;
} CellBars;

static uint32_t _hash_cell(int x, int z) {
    return ((x * 73856093) ^ (z * 19349663)) % GRID_HASH_SIZE;
}

ClashReport* rebar_detect_clashes(const RebarLayout* rl, float min_spacing, float min_cover) {
    if (!rl) return NULL;
    ClashReport* cr = (ClashReport*)calloc(1, sizeof(ClashReport));
    if (!cr) return NULL;
    cr->capacity = 256;
    cr->clashes = (Clash*)calloc(cr->capacity, sizeof(Clash));

    CellBars grid[GRID_HASH_SIZE];
    memset(grid, 0, sizeof(grid));

    float cell_size = min_spacing > 0.0f ? min_spacing : 1.0f;

    for (uint32_t i = 0; i < rl->count; i++) {
        Vec3 mid = vec3_lerp(rl->bars[i].start, rl->bars[i].end, 0.5f);
        int cx = (int)(mid.x / cell_size);
        int cz = (int)(mid.z / cell_size);
        uint32_t h = _hash_cell(cx, cz);

        CellBars* cell = &grid[h];
        if (cell->count >= cell->capacity) {
            int new_cap = (cell->capacity == 0) ? 8 : cell->capacity * 2;
            cell->bars = (int*)realloc(cell->bars, new_cap * sizeof(int));
            cell->capacity = new_cap;
        }
        cell->bars[cell->count++] = i;
    }

    for (int h = 0; h < GRID_HASH_SIZE; h++) {
        CellBars* cell = &grid[h];
        if (cell->count <= 0) continue;

        for (int a = 0; a < cell->count; a++) {
            uint32_t i = cell->bars[a];
            Vec3 mid_i = vec3_lerp(rl->bars[i].start, rl->bars[i].end, 0.5f);

            for (int b = a + 1; b < cell->count; b++) {
                uint32_t j = cell->bars[b];
                Vec3 mid_j = vec3_lerp(rl->bars[j].start, rl->bars[j].end, 0.5f);
                double d = vec3_distance(&mid_i, &mid_j);

                if (d < min_spacing) {
                    if (cr->count >= cr->capacity) {
                        cr->capacity *= 2;
                        cr->clashes = (Clash*)realloc(cr->clashes, cr->capacity * sizeof(Clash));
                    }
                    Clash* c = &cr->clashes[cr->count++];
                    c->bar_a = i; c->bar_b = j;
                    c->location = vec3_lerp(mid_i, mid_j, 0.5f);
                    c->distance = (float)d;
                    strcpy(c->severity, "SPACING");
                }
            }
        }
    }

    for (int h = 0; h < GRID_HASH_SIZE; h++) {
        free(grid[h].bars);
    }

    for (uint32_t i = 0; i < rl->count; i++) {
        Vec3 mid_i = vec3_lerp(rl->bars[i].start, rl->bars[i].end, 0.5f);
        if (mid_i.x < min_cover || mid_i.z < min_cover || mid_i.x > 10.0f - min_cover || mid_i.z > 10.0f - min_cover) {
            if (cr->count >= cr->capacity) {
                cr->capacity *= 2;
                cr->clashes = (Clash*)realloc(cr->clashes, cr->capacity * sizeof(Clash));
            }
            Clash* c = &cr->clashes[cr->count++];
            c->bar_a = i; c->bar_b = UINT32_MAX;
            c->location = mid_i;
            float dist_x = fminf(mid_i.x, 10.0f - mid_i.x);
            float dist_z = fminf(mid_i.z, 10.0f - mid_i.z);
            c->distance = fminf(dist_x, dist_z);
            strcpy(c->severity, "COVER");
        }
    }
    return cr;
}

void clash_report_destroy(ClashReport* cr) { if(cr){ free(cr->clashes); free(cr); } }
void clash_report_print(const ClashReport* cr) { if(!cr) return; printf("[CLASHES] Found: %u\n", cr->count); }

void clash_report_render(ClashReport* cr, const RebarLayout* rl, Mat4 view, Mat4 proj) {
    (void)rl;
    if(!cr || cr->count==0) return;
    
    float* data = (float*)malloc(cr->count * 6 * sizeof(float));
    if (!data) return;
    
    for(uint32_t i=0; i<cr->count; i++){
        data[i*6]   = cr->clashes[i].location.x; 
        data[i*6+1] = cr->clashes[i].location.y; 
        data[i*6+2] = cr->clashes[i].location.z;
        if(strcmp(cr->clashes[i].severity, "COVER")==0){ 
            data[i*6+3]=1.0f; data[i*6+4]=1.0f; data[i*6+5]=0.0f; 
        } else { 
            data[i*6+3]=1.0f; data[i*6+4]=0.0f; data[i*6+5]=0.0f; 
        }
    }
    
    if (!cr->uploaded) {
        glGenVertexArrays(1, &cr->vao);
        glGenBuffers(1, &cr->vbo);
        glBindVertexArray(cr->vao);
        glBindBuffer(GL_ARRAY_BUFFER, cr->vbo);
        glBufferData(GL_ARRAY_BUFFER, cr->count * 6 * sizeof(float), data, GL_DYNAMIC_DRAW);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0); 
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float))); 
        glEnableVertexAttribArray(1);
        glBindVertexArray(0);
        cr->uploaded = 1;
    } else {
        glBindBuffer(GL_ARRAY_BUFFER, cr->vbo);
        glBufferSubData(GL_ARRAY_BUFFER, 0, cr->count * 6 * sizeof(float), data);
    }
    
    free(data);

    glUseProgram(shader_program); 
    glPointSize(8.0f); 
    glEnable(GL_PROGRAM_POINT_SIZE);
    
    GLint loc_view = glGetUniformLocation(shader_program, "uView");
    if(loc_view>=0){ float f[16]; for(int i=0;i<16;i++) f[i]=(float)view.m[i]; glUniformMatrix4fv(loc_view,1,GL_FALSE,f); }
    GLint loc_proj = glGetUniformLocation(shader_program, "uProjection");
    if(loc_proj>=0){ float f[16]; for(int i=0;i<16;i++) f[i]=(float)proj.m[i]; glUniformMatrix4fv(loc_proj,1,GL_FALSE,f); }

    glBindVertexArray(cr->vao);
    glDrawArrays(GL_POINTS, 0, cr->count);
    glBindVertexArray(0);
    
    glDisable(GL_PROGRAM_POINT_SIZE);
}
