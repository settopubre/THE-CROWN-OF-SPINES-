#include "asset_db.h"
#include "vector3d.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>
#include <math.h>
#include <time.h>

static void _scan_recursive(AssetDB* db, const char* dir_path, const char* base_category, int depth);

static uint32_t _count_obj_triangles(const char* filepath) {
    DEFER_FILE(fp) = fopen(filepath, "r");
    if (!fp) return 0;
    uint32_t tris = 0;
    char line[1024];
    while (fgets(line, sizeof(line), fp)) {
        if (line[0] == 'f' && (line[1] == ' ' || line[1] == '\t')) {
            int slashes = 0;
            for (int i = 0; line[i]; i++) if (line[i] == '/') slashes++;
            int verts = slashes / 3;
            tris += (verts >= 3) ? (verts - 2) : 0;
        }
    }
    return tris;
}

static float _compute_bounding_radius(const char* filepath) {
    DEFER_FILE(fp) = fopen(filepath, "r");
    if (!fp) return 0.0f;
    double cx = 0, cy = 0, cz = 0;
    uint32_t vcount = 0;
    char line[1024];
    while (fgets(line, sizeof(line), fp)) {
        if (line[0] == 'v' && line[1] == ' ') {
            float x, y, z;
            if (sscanf(line, "v %f %f %f", &x, &y, &z) == 3) {
                cx += x; cy += y; cz += z; vcount++;
            }
        }
    }
    if (vcount == 0) return 0.0f;
    cx /= vcount; cy /= vcount; cz /= vcount;
    
    fseek(fp, 0, SEEK_SET);
    double max_r = 0.0;
    while (fgets(line, sizeof(line), fp)) {
        if (line[0] == 'v' && line[1] == ' ') {
            float x, y, z;
            if (sscanf(line, "v %f %f %f", &x, &y, &z) == 3) {
                double d = (x-cx)*(x-cx) + (y-cy)*(y-cy) + (z-cz)*(z-cz);
                if (d > max_r) max_r = d;
            }
        }
    }
    return sqrt(max_r);
}

static void _add_entry(AssetDB* db, const char* name, const char* path, const char* category, time_t mtime) {
    if (db->count >= db->capacity) {
        uint32_t new_cap = db->capacity * 2;
        AssetEntry* new_ent = realloc(db->entries, new_cap * sizeof(AssetEntry));
        if (!new_ent) return;
        db->entries = new_ent;
        db->capacity = new_cap;
    }
    AssetEntry* e = &db->entries[db->count];
    e->id = db->count;
    strncpy(e->name, name, sizeof(e->name)-1); e->name[sizeof(e->name)-1] = '\0';
    strncpy(e->path, path, sizeof(e->path)-1); e->path[sizeof(e->path)-1] = '\0';
    strncpy(e->category, category ? category : "root", sizeof(e->category)-1); 
    e->category[sizeof(e->category)-1] = '\0';
    e->triangle_count = _count_obj_triangles(path);
    e->bounding_radius = _compute_bounding_radius(path);
    e->mtime = mtime;
    db->count++;
}

static void _scan_recursive(AssetDB* db, const char* dir_path, const char* base_category, int depth) {
    if (depth > 8) return;
    
    DEFER_DIR(dir) = opendir(dir_path);
    if (!dir) return;
    
    struct dirent* entry;
    char filepath[1024], name_buf[256], category_buf[256];
    
    while ((entry = readdir(dir)) != NULL) {
        if (entry->d_name[0] == '.') continue;
        snprintf(filepath, sizeof(filepath), "%s/%s", dir_path, entry->d_name);
        
        struct stat st;
        if (stat(filepath, &st) != 0) continue;
        
        if (S_ISDIR(st.st_mode)) {
            if (strcmp(base_category, "root") == 0) {
                snprintf(category_buf, sizeof(category_buf), "%s", entry->d_name);
            } else {
                snprintf(category_buf, sizeof(category_buf), "%s/%s", base_category, entry->d_name);
            }
            _scan_recursive(db, filepath, category_buf, depth + 1);
        } else if (S_ISREG(st.st_mode) && strstr(entry->d_name, ".obj")) {
            strncpy(name_buf, entry->d_name, sizeof(name_buf)-1);
            name_buf[strrchr(name_buf, '.') - name_buf] = '\0';
            _add_entry(db, name_buf, filepath, base_category, st.st_mtime);
        }
    }
}

AssetDB* asset_db_create(uint32_t capacity) {
    if (capacity == 0) capacity = 64;
    AssetDB* db = (AssetDB*)calloc(1, sizeof(AssetDB));
    if (!db) return NULL;
    db->capacity = capacity;
    db->entries = (AssetEntry*)calloc(capacity, sizeof(AssetEntry));
    if (!db->entries) { free(db); return NULL; }
    return db;
}

void asset_db_destroy(AssetDB* db) {
    if (db) {
        free(db->entries);
        free(db);
    }
}

void asset_db_scan_directory(AssetDB* db, const char* directory) {
    if (!db || !directory) return;
    _scan_recursive(db, directory, "root", 0);
}

int asset_db_reload_if_changed(AssetDB* db, const char* directory) {
    if (!db || !directory) return 0;
    
    int changed = 0;
    for (uint32_t i = 0; i < db->count; i++) {
        struct stat st;
        if (stat(db->entries[i].path, &st) == 0) {
            if (st.st_mtime != db->entries[i].mtime) {
                changed = 1;
                break;
            }
        }
    }
    
    if (changed) {
        free(db->entries);
        db->capacity = 64;
        db->entries = (AssetEntry*)calloc(db->capacity, sizeof(AssetEntry));
        db->count = 0;
        _scan_recursive(db, directory, "root", 0);
        return 1;
    }
    
    return 0;
}

uint32_t* asset_db_get_assets_by_category(AssetDB* db, const char* category, uint32_t* out_count) {
    if (!db || !category || !out_count) {
        if(out_count) *out_count = 0;
        return NULL;
    }
    
    *out_count = 0;
    
    for (uint32_t i = 0; i < db->count; i++) {
        if (strcmp(db->entries[i].category, category) == 0) {
            (*out_count)++;
        }
    }
    
    if (*out_count == 0) return NULL;
    
    uint32_t* ids = (uint32_t*)malloc((*out_count) * sizeof(uint32_t));
    if (!ids) { *out_count = 0; return NULL; }
    
    uint32_t idx = 0;
    for (uint32_t i = 0; i < db->count; i++) {
        if (strcmp(db->entries[i].category, category) == 0) {
            ids[idx++] = i;
        }
    }
    
    return ids;
}

const AssetEntry* asset_db_get(const AssetDB* db, uint32_t id) {
    if (!db || id >= db->count) return NULL;
    return &db->entries[id];
}

const AssetEntry* asset_db_find_by_name(const AssetDB* db, const char* name) {
    if (!db || !name) return NULL;
    for (uint32_t i = 0; i < db->count; i++) {
        if (strcmp(db->entries[i].name, name) == 0) return &db->entries[i];
    }
    return NULL;
}

uint32_t asset_db_get_count(const AssetDB* db) { return db ? db->count : 0; }
