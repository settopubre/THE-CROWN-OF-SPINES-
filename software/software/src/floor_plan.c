#include "floor_plan.h"
#include "render_server.h"
#include "gl_functions.h"
#include "input_server.h"
#include "grid.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <stdio.h>

#define MAX_POINTS 256
#define WALL_HEIGHT 8.0f   /* Default 8 foot walls */
#define WALL_THICKNESS 0.5f /* 6 inch walls */

struct FloorPlan {
    Vec3 points[MAX_POINTS];
    uint32_t point_count;
    int is_drawing;
    int is_closed;
    Vec3 current_mouse_point;
    float wall_height;
    float wall_thickness;
    Mesh* wall_mesh;
    int show_measurements;
};

static FloorPlan* g_fp = NULL;

FloorPlan* floor_plan_create(void) {
    FloorPlan* fp = (FloorPlan*)calloc(1, sizeof(FloorPlan));
    if (!fp) return NULL;
    fp->point_count = 0;
    fp->is_drawing = 1;
    fp->is_closed = 0;
    fp->wall_height = WALL_HEIGHT;
    fp->wall_thickness = WALL_THICKNESS;
    fp->wall_mesh = NULL;
    fp->show_measurements = 1;
    
    printf("[FLOOR PLAN] Click to place wall points. Press ENTER to extrude walls. Press C to close loop.\n");
    printf("[FLOOR PLAN] Grid snap: ON | Wall height: %.1fft | Thickness: %.2fft\n", 
           fp->wall_height, fp->wall_thickness);
    
    return fp;
}

void floor_plan_destroy(FloorPlan* fp) {
    if (!fp) return;
    if (fp->wall_mesh) mesh_destroy(fp->wall_mesh);
    free(fp);
}

void floor_plan_add_point(FloorPlan* fp, Vec3 point) {
    if (!fp || fp->point_count >= MAX_POINTS) return;
    if (!fp->is_drawing) return;
    
    /* Snap Y to 0 (floor plane) */
    point.y = 0.0f;
    
    /* Don't add duplicate of last point */
    if (fp->point_count > 0) {
        Vec3 last = fp->points[fp->point_count - 1];
        if (vec3_distance(&last, &point) < 0.1f) return;
    }
    
    fp->points[fp->point_count++] = point;
    printf("[FLOOR PLAN] Point %u placed at (%.2f, %.2f)\n", 
           fp->point_count, point.x, point.z);
    
    /* Show segment measurement */
    if (fp->point_count >= 2 && fp->show_measurements) {
        Vec3 a = fp->points[fp->point_count - 2];
        Vec3 b = fp->points[fp->point_count - 1];
        float dist_ft = vec3_distance(&a, &b);
        float dist_in = dist_ft * 12.0f;
        if (dist_ft >= 1.0f) {
            printf("  Segment: %.2f ft\n", dist_ft);
        } else {
            printf("  Segment: %.1f in\n", dist_in);
        }
    }
}

void floor_plan_close_loop(FloorPlan* fp) {
    if (!fp || fp->point_count < 3) return;
    fp->is_closed = 1;
    fp->is_drawing = 0;
    
    /* Calculate total perimeter */
    float total = 0.0f;
    for (uint32_t i = 0; i < fp->point_count; i++) {
        Vec3 a = fp->points[i];
        Vec3 b = fp->points[(i + 1) % fp->point_count];
        total += vec3_distance(&a, &b);
    }
    
    /* Calculate area (shoelace formula) */
    float area = 0.0f;
    for (uint32_t i = 0; i < fp->point_count; i++) {
        Vec3 a = fp->points[i];
        Vec3 b = fp->points[(i + 1) % fp->point_count];
        area += (a.x * b.z) - (b.x * a.z);
    }
    area = fabsf(area) * 0.5f;
    
    printf("[FLOOR PLAN] Loop closed. %u points. Perimeter: %.2f ft. Area: %.2f sqft\n",
           fp->point_count, total, area);
}

void floor_plan_extrude_walls(FloorPlan* fp) {
    if (!fp || fp->point_count < 2) return;
    
    /* Auto-close if not closed */
    if (!fp->is_closed && fp->point_count >= 3) {
        floor_plan_close_loop(fp);
    }
    
    uint32_t wall_count = fp->is_closed ? fp->point_count : fp->point_count - 1;
    
    /* Each wall segment becomes a box (8 vertices, 12 triangles) */
    uint32_t verts_per_wall = 8;
    uint32_t tris_per_wall = 12;
    uint32_t total_verts = wall_count * verts_per_wall;
    uint32_t total_tris = wall_count * tris_per_wall;
    
    Mesh* m = (Mesh*)calloc(1, sizeof(Mesh));
    if (!m) return;
    
    m->vertex_count = total_verts;
    m->index_count = total_tris * 3;
    m->positions = (float*)malloc(total_verts * 3 * sizeof(float));
    m->normals = (float*)malloc(total_verts * 3 * sizeof(float));
    m->indices = (uint32_t*)malloc(m->index_count * sizeof(uint32_t));
    
    if (!m->positions || !m->normals || !m->indices) {
        free(m->positions); free(m->normals); free(m->indices); free(m);
        return;
    }
    
    for (uint32_t w = 0; w < wall_count; w++) {
        Vec3 start = fp->points[w];
        Vec3 end = fp->points[(w + 1) % fp->point_count];
        
        /* Wall direction and perpendicular */
        Vec3 dir = vec3_sub(end, start);
        float length = vec3_length(dir);
        if (length < 0.01f) continue;
        
        dir = vec3_normalize(dir);
        Vec3 perp = vec3_create(-dir.z, 0.0f, dir.x);
        Vec3 half_thick = vec3_mul(perp, fp->wall_thickness * 0.5f);
        Vec3 height_vec = vec3_create(0.0f, fp->wall_height, 0.0f);
        
        /* 8 corners of the wall box */
        Vec3 corners[8] = {
            vec3_sub(start, half_thick),                          /* bottom inner */
            vec3_add(start, half_thick),                          /* bottom outer */
            vec3_add(end, half_thick),                            /* bottom outer end */
            vec3_sub(end, half_thick),                            /* bottom inner end */
            vec3_add(vec3_sub(start, half_thick), height_vec),   /* top inner */
            vec3_add(vec3_add(start, half_thick), height_vec),   /* top outer */
            vec3_add(vec3_add(end, half_thick), height_vec),     /* top outer end */
            vec3_add(vec3_sub(end, half_thick), height_vec)      /* top inner end */
        };
        
        uint32_t base_idx = w * verts_per_wall;
        
        /* Store vertices */
        for (int i = 0; i < 8; i++) {
            m->positions[(base_idx + i) * 3]     = corners[i].x;
            m->positions[(base_idx + i) * 3 + 1] = corners[i].y;
            m->positions[(base_idx + i) * 3 + 2] = corners[i].z;
        }
        
        /* Compute normals per face later */
        for (int i = 0; i < 8; i++) {
            m->normals[(base_idx + i) * 3]     = 0.0f;
            m->normals[(base_idx + i) * 3 + 1] = 1.0f;
            m->normals[(base_idx + i) * 3 + 2] = 0.0f;
        }
        
        /* Indices for 6 faces, 2 triangles each */
        uint32_t faces[12][3] = {
            {0, 1, 2}, {0, 2, 3},   /* bottom */
            {4, 6, 5}, {4, 7, 6},   /* top */
            {0, 4, 5}, {0, 5, 1},   /* outer */
            {2, 6, 7}, {2, 7, 3},   /* inner */
            {1, 5, 6}, {1, 6, 2},   /* end */
            {3, 7, 4}, {3, 4, 0}    /* start */
        };
        
        uint32_t idx_base = w * tris_per_wall * 3;
        for (int f = 0; f < 12; f++) {
            for (int v = 0; v < 3; v++) {
                m->indices[idx_base + f * 3 + v] = base_idx + faces[f][v];
            }
        }
    }
    
    /* Clean up old mesh */
    if (fp->wall_mesh) mesh_destroy(fp->wall_mesh);
    fp->wall_mesh = m;
    
    /* Calculate wall area for materials estimate */
    float total_wall_area = 0.0f;
    for (uint32_t w = 0; w < wall_count; w++) {
        Vec3 start = fp->points[w];
        Vec3 end = fp->points[(w + 1) % fp->point_count];
        total_wall_area += vec3_distance(&start, &end) * fp->wall_height;
    }
    
    printf("[FLOOR PLAN] Walls extruded: %u segments, %.1f sqft wall area, %.1f ft total length\n",
           wall_count, total_wall_area, 
           total_wall_area / fp->wall_height);
    
    fp->is_drawing = 0;
}

void floor_plan_clear(FloorPlan* fp) {
    if (!fp) return;
    fp->point_count = 0;
    fp->is_drawing = 1;
    fp->is_closed = 0;
    if (fp->wall_mesh) {
        mesh_destroy(fp->wall_mesh);
        fp->wall_mesh = NULL;
    }
    printf("[FLOOR PLAN] Cleared. Ready for new floor plan.\n");
}

void floor_plan_set_wall_height(FloorPlan* fp, float height) {
    if (fp && height > 0.0f) {
        fp->wall_height = height;
        printf("[FLOOR PLAN] Wall height: %.1f ft\n", height);
    }
}

void floor_plan_set_wall_thickness(FloorPlan* fp, float thickness) {
    if (fp && thickness > 0.0f) {
        fp->wall_thickness = thickness;
        printf("[FLOOR PLAN] Wall thickness: %.2f ft (%.1f in)\n", 
               thickness, thickness * 12.0f);
    }
}

void floor_plan_render(FloorPlan* fp, Mat4 view, Mat4 proj) {
    if (!fp) return;
    
    /* Render the floor plan lines */
    if (fp->point_count >= 2) {
        uint32_t line_count = fp->is_closed ? fp->point_count : fp->point_count - 1;
        float* line_verts = (float*)malloc(line_count * 2 * 6 * sizeof(float));
        
        for (uint32_t i = 0; i < line_count; i++) {
            Vec3 a = fp->points[i];
            Vec3 b = fp->points[(i + 1) % fp->point_count];
            
            /* Thick yellow line for walls */
            uint32_t base = i * 12;
            line_verts[base + 0] = a.x; line_verts[base + 1] = a.y + 0.02f; line_verts[base + 2] = a.z;
            line_verts[base + 3] = 1.0f; line_verts[base + 4] = 0.8f; line_verts[base + 5] = 0.2f;
            line_verts[base + 6] = b.x; line_verts[base + 7] = b.y + 0.02f; line_verts[base + 8] = b.z;
            line_verts[base + 9] = 1.0f; line_verts[base + 10] = 0.8f; line_verts[base + 11] = 0.2f;
        }
        
        render_server_draw_lines(line_verts, line_count * 2, vec3_create(1.0, 0.8, 0.2));
        free(line_verts);
    }
    
    /* Render vertex points */
    if (fp->point_count > 0) {
        float* point_verts = (float*)malloc(fp->point_count * 2 * 6 * sizeof(float));
        for (uint32_t i = 0; i < fp->point_count; i++) {
            uint32_t base = i * 12;
            /* Small cross at each point */
            Vec3 p = fp->points[i];
            float s = 0.3f;
            point_verts[base + 0] = p.x - s; point_verts[base + 1] = p.y + 0.03f; point_verts[base + 2] = p.z;
            point_verts[base + 3] = 0.0f; point_verts[base + 4] = 1.0f; point_verts[base + 5] = 0.0f;
            point_verts[base + 6] = p.x + s; point_verts[base + 7] = p.y + 0.03f; point_verts[base + 8] = p.z;
            point_verts[base + 9] = 0.0f; point_verts[base + 10] = 1.0f; point_verts[base + 11] = 0.0f;
        }
        render_server_draw_lines(point_verts, fp->point_count * 2, vec3_create(0.0, 1.0, 0.0));
        free(point_verts);
    }
    
    /* Render the extruded wall mesh */
    if (fp->wall_mesh) {
        Mat4 model = mat4_identity();
        render_server_draw_mesh(fp->wall_mesh, model);
    }
}

/* Getter for engine to check if floor plan is active */
int floor_plan_is_active(FloorPlan* fp) {
    return fp && fp->is_drawing;
}

FloorPlan* floor_plan_get(void) {
    if (!g_fp) g_fp = floor_plan_create();
    return g_fp;
}
