#ifndef ASSET_DB_H
#define ASSET_DB_H

#include <stdint.h>
#include <time.h>

typedef struct {
    uint32_t id;
    char name[256];
    char path[1024];
    char category[256];
    uint32_t triangle_count;
    float bounding_radius;
    time_t mtime;
} AssetEntry;

typedef struct {
    AssetEntry* entries;
    uint32_t count;
    uint32_t capacity;
} AssetDB;

#ifdef __cplusplus
extern "C" {
#endif

AssetDB* asset_db_create(uint32_t capacity);
void asset_db_destroy(AssetDB* db);
void asset_db_scan_directory(AssetDB* db, const char* directory);
int asset_db_reload_if_changed(AssetDB* db, const char* directory);
const AssetEntry* asset_db_get(const AssetDB* db, uint32_t id);
const AssetEntry* asset_db_find_by_name(const AssetDB* db, const char* name);
uint32_t asset_db_get_count(const AssetDB* db);
uint32_t* asset_db_get_assets_by_category(AssetDB* db, const char* category, uint32_t* out_count);

#ifdef __cplusplus
}
#endif

#endif /* ASSET_DB_H */
