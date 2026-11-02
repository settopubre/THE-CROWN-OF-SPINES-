#include "ai_action_executor.h"
#include "ai_controller.h"
#include "json_parser.h"
#include "editor.h"
#include "asset_db.h"
#include "spatial_memory.h"
#include "engine_abi.h"
#include "ui_overlay.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

extern Editor* g_editor;
extern AssetDB* g_db;
extern AIController* g_ai_controller;

/* FIX: Statistical brain wiring — stat worker handle from engine.c */
extern struct RWorker* g_stat_worker;

static char last_error[512] = {0};

const char* ai_action_get_last_error(void) {
    return last_error;
}

int ai_action_execute(const char* json_action) {
    if (!json_action) return -1;
    last_error[0] = '\0';

    JSONObject* root = json_parse(json_action);
    if (!root) {
        snprintf(last_error, sizeof(last_error), "Invalid JSON");
        return -1;
    }

    const char* action = json_get_string(root, "action");
    if (!action) {
        snprintf(last_error, sizeof(last_error), "Missing 'action' field");
        json_free(root);
        return -1;
    }

    int result = 0;
    if (strcmp(action, "walk") == 0) {
        JSONObject* target_obj = json_get_object(root, "target");
        if (target_obj && g_ai_controller) {
            Vec3 target = vec3_create(
                json_get_number(target_obj, "x"),
                json_get_number(target_obj, "y"),
                json_get_number(target_obj, "z")
            );
            ai_controller_set_target(g_ai_controller, target);
        } else {
            snprintf(last_error, sizeof(last_error), "Missing target or controller");
            result = -1;
        }
    } 
    else if (strcmp(action, "build") == 0) {
        const char* asset_name = json_get_string(root, "asset");
        JSONObject* pos_obj = json_get_object(root, "position");
        if (asset_name && pos_obj && g_editor) {
            Vec3 pos = vec3_create(json_get_number(pos_obj, "x"), 0.0, json_get_number(pos_obj, "z"));
            editor_place_asset(g_editor, 0, pos);
            spatial_store((int32_t)pos.x, (int32_t)pos.z, 0, asset_name);
        } else {
            snprintf(last_error, sizeof(last_error), "Missing asset/position for build");
            result = -1;
        }
    } 
    else if (strcmp(action, "delete") == 0) {
        JSONObject* pos_obj = json_get_object(root, "position");
        if (pos_obj && g_editor) {
            Vec3 pos = vec3_create(json_get_number(pos_obj, "x"), 0.0, json_get_number(pos_obj, "z"));
            editor_select_at(g_editor, pos, vec3_create(0, -1, 0));
            if (g_editor->selected_asset_id != UINT32_MAX) {
                editor_delete_selected(g_editor);
            }
        } else {
            snprintf(last_error, sizeof(last_error), "Missing position for delete");
            result = -1;
        }
    }
    else if (strcmp(action, "run_test") == 0) {
        const char* test_type = json_get_string(root, "test");
        JSONObject* data_obj = json_get_object(root, "data");
        
        if (!test_type || !data_obj) {
            snprintf(last_error, sizeof(last_error), "Missing 'test' or 'data' field for run_test");
            result = -1;
        } else if (!g_stat_worker) {
            snprintf(last_error, sizeof(last_error), "Statistical worker not available");
            result = -1;
        } else {
            char data_json[4096] = "{";
            int offset = 1;
            int first = 1;
            
            const char* keys[] = {"group1","group2","before","after","x","y","value","group","var1","var2","outcome",NULL};
            for (int k = 0; keys[k]; k++) {
                JSONObject* arr = json_get_array(data_obj, keys[k]);
                if (arr) {
                    if (!first) { data_json[offset++] = ','; }
                    offset += snprintf(data_json + offset, sizeof(data_json) - offset, "\"%s\":[", keys[k]);
                    int len = json_array_length(arr);
                    for (int i = 0; i < len; i++) {
                        JSONObject* item = json_array_get(arr, i);
                        double val = (item && item->type == JSON_NUMBER) ? item->data.num : 0.0;
                        offset += snprintf(data_json + offset, sizeof(data_json) - offset, "%s%g", i > 0 ? "," : "", val);
                    }
                    data_json[offset++] = ']';
                    first = 0;
                }
            }
            data_json[offset++] = '}';
            data_json[offset] = '\0';
            
            if (r_stat_send_test(g_stat_worker, test_type, data_json) == 0) {
                char* rsp = r_stat_recv_result(g_stat_worker);
                if (rsp) {
                    JSONObject* result_json = json_parse(rsp);
                    if (result_json) {
                        double p_value = json_get_number(result_json, "p_value");
                        const char* interp = json_get_string(result_json, "interpretation");
                        
                        char msg[512];
                        float r, g, b;
                        
                        if (p_value < 0.05) {
                            r = 0.0f; g = 1.0f; b = 0.0f;
                            snprintf(msg, sizeof(msg), "[STAT] p=%.4f - Significant", p_value);
                        } else if (p_value < 0.10) {
                            r = 1.0f; g = 1.0f; b = 0.0f;
                            snprintf(msg, sizeof(msg), "[STAT] p=%.4f - Marginally significant", p_value);
                        } else {
                            r = 1.0f; g = 1.0f; b = 1.0f;
                            snprintf(msg, sizeof(msg), "[STAT] p=%.4f - Not significant", p_value);
                        }
                        
                        if (interp && interp[0]) {
                            strncat(msg, ": ", sizeof(msg) - strlen(msg) - 1);
                            strncat(msg, interp, sizeof(msg) - strlen(msg) - 1);
                        }
                        
                        ui_terminal_add_line(msg, r, g, b);
                        json_free(result_json);
                    } else {
                        ui_terminal_add_line("[STAT] Failed to parse R response", 1.0f, 0.5f, 0.5f);
                    }
                } else {
                    ui_terminal_add_line("[STAT] No response from R worker (timeout?)", 1.0f, 0.5f, 0.5f);
                    result = -1;
                }
            } else {
                snprintf(last_error, sizeof(last_error), "Failed to send to R worker");
                result = -1;
            }
        }
    }
    else {
        snprintf(last_error, sizeof(last_error), "Unknown action: %s", action);
        result = -1;
    }

    json_free(root);
    return result;
}

void ai_action_execute_sequence(const char* json_actions) {
    if (!json_actions) return;
    ai_action_execute(json_actions);
}
