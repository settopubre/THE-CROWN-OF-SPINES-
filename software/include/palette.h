#ifndef PALETTE_H
#define PALETTE_H
#pragma once

#include "asset_db.h"
#include <stdint.h>

typedef struct {
    AssetDB* db;
    uint32_t selected_category_index;
    uint32_t selected_asset_index;
    char** categories;
    uint32_t category_count;
    uint32_t* category_counts; /* Cache for speed */
} Palette;

#ifdef __cplusplus
extern "C" {
#endif

Palette* palette_create(AssetDB* db);
void palette_destroy(Palette* pal);
void palette_next_category(Palette* pal);
void palette_prev_category(Palette* pal);
void palette_next_asset(Palette* pal);
void palette_prev_asset(Palette* pal);
const AssetEntry* palette_get_selected(const Palette* pal);
const char* palette_get_current_category(const Palette* pal);
uint32_t palette_get_asset_count_in_category(const Palette* pal);

#ifdef __cplusplus
}
#endif

#endif /* PALETTE_H */
