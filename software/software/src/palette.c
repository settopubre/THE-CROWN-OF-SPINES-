#include "palette.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

static void _scan_categories(Palette* pal) {
    if (!pal->db) return;
    pal->category_count = 0;
    
    /* Count unique categories first */
    /* Simple approach: assume max 64 categories for buffer allocation */
    char* unique_cats[64];
    uint32_t counts[64] = {0};
    
    for (uint32_t i = 0; i < pal->db->count; i++) {
        const char* cat = pal->db->entries[i].category;
        int found = 0;
        for (uint32_t k = 0; k < pal->category_count && k < 64; k++) {
            if (strcmp(unique_cats[k], cat) == 0) {
                counts[k]++;
                found = 1;
                break;
            }
        }
        if (!found && pal->category_count < 64) {
            unique_cats[pal->category_count] = (char*)cat; /* Just store pointer, no need to strdup */
            counts[pal->category_count] = 1;
            pal->category_count++;
        }
    }
    
    pal->categories = (char**)calloc(pal->category_count, sizeof(char*));
    pal->category_counts = (uint32_t*)calloc(pal->category_count, sizeof(uint32_t));
    memcpy(pal->categories, unique_cats, pal->category_count * sizeof(char*));
    memcpy(pal->category_counts, counts, pal->category_count * sizeof(uint32_t));
}

Palette* palette_create(AssetDB* db) {
    Palette* pal = (Palette*)calloc(1, sizeof(Palette));
    if (!pal) return NULL;
    pal->db = db;
    _scan_categories(pal);
    return pal;
}

void palette_destroy(Palette* pal) {
    if (pal) {
        free(pal->categories);
        free(pal->category_counts);
        free(pal);
    }
}

const char* palette_get_current_category(const Palette* pal) {
    if (!pal || pal->selected_category_index >= pal->category_count) return "";
    return pal->categories[pal->selected_category_index];
}

uint32_t palette_get_asset_count_in_category(const Palette* pal) {
    if (!pal || pal->selected_category_index >= pal->category_count) return 0;
    return pal->category_counts[pal->selected_category_index];
}

const AssetEntry* palette_get_selected(const Palette* pal) {
    if (!pal || pal->category_count == 0) return NULL;
    
    const char* target_cat = palette_get_current_category(pal);
    uint32_t current_idx = 0;
    
    for (uint32_t i = 0; i < pal->db->count; i++) {
        if (strcmp(pal->db->entries[i].category, target_cat) == 0) {
            if (current_idx == pal->selected_asset_index) return &pal->db->entries[i];
            current_idx++;
        }
    }
    return NULL; /* Index out of bounds for this category */
}

void palette_next_category(Palette* pal) {
    if (!pal || pal->category_count == 0) return;
    pal->selected_category_index = (pal->selected_category_index + 1) % pal->category_count;
    pal->selected_asset_index = 0;
}

void palette_prev_category(Palette* pal) {
    if (!pal || pal->category_count == 0) return;
    if (pal->selected_category_index == 0) pal->selected_category_index = pal->category_count - 1;
    else pal->selected_category_index--;
    pal->selected_asset_index = 0;
}

void palette_next_asset(Palette* pal) {
    uint32_t count = palette_get_asset_count_in_category(pal);
    if (count > 0) {
        pal->selected_asset_index = (pal->selected_asset_index + 1) % count;
    }
}

void palette_prev_asset(Palette* pal) {
    uint32_t count = palette_get_asset_count_in_category(pal);
    if (count > 0) {
        if (pal->selected_asset_index == 0) pal->selected_asset_index = count - 1;
        else pal->selected_asset_index--;
    }
}
