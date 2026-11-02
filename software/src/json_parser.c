#include "json_parser.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <math.h>
#include <ctype.h>

static const char* skip_ws(const char* p) {
    while (*p && isspace((unsigned char)*p)) p++;
    return p;
}

static JSONValue* parse_value(const char** pp);

static char* parse_string_raw(const char** pp) {
    const char* p = *pp;
    if (*p != '"') return NULL;
    p++;
    const char* start = p;
    while (*p && *p != '"') {
        if (*p == '\\' && *(p+1)) p += 2;
        else p++;
    }
    if (*p != '"') return NULL;
    size_t len = p - start;
    char* out = (char*)malloc(len + 1);
    if (!out) return NULL;
    
    /* Simple unescape */
    size_t j = 0;
    for (size_t i = 0; i < len; i++) {
        if (start[i] == '\\' && i+1 < len) {
            char c = start[++i];
            if (c == 'n') out[j++] = '\n';
            else if (c == 'r') out[j++] = '\r';
            else if (c == 't') out[j++] = '\t';
            else if (c == '"') out[j++] = '"';
            else if (c == '\\') out[j++] = '\\';
            else out[j++] = c;
        } else {
            out[j++] = start[i];
        }
    }
    out[j] = '\0';
    *pp = p + 1;
    return out;
}

static double parse_number(const char** pp) {
    const char* p = *pp;
    char* end;
    double val = strtod(p, &end);
    *pp = end;
    return val;
}

static JSONValue* parse_object(const char** pp) {
    const char* p = skip_ws(*pp);
    if (*p != '{') return NULL;
    p++;
    
    JSONValue* obj = (JSONValue*)calloc(1, sizeof(JSONValue));
    if (!obj) return NULL;
    obj->type = JSON_OBJECT;
    
    p = skip_ws(p);
    if (*p == '}') { *pp = p + 1; return obj; }
    
    while (1) {
        p = skip_ws(p);
        if (*p != '"') { json_free(obj); return NULL; }
        
        char* key = parse_string_raw(&p);
        if (!key) { json_free(obj); return NULL; }
        
        p = skip_ws(p);
        if (*p != ':') { free(key); json_free(obj); return NULL; }
        p++;
        
        p = skip_ws(p);
        JSONValue* val = parse_value(&p);
        if (!val) { free(key); json_free(obj); return NULL; }
        
        /* Resize arrays */
        size_t new_count = obj->data.object.count + 1;
        char** new_keys = (char**)realloc(obj->data.object.keys, new_count * sizeof(char*));
        JSONValue** new_vals = (JSONValue**)realloc(obj->data.object.values, new_count * sizeof(JSONValue*));
        if (!new_keys || !new_vals) {
            free(key); json_free(val); json_free(obj); return NULL;
        }
        obj->data.object.keys = new_keys;
        obj->data.object.values = new_vals;
        obj->data.object.keys[obj->data.object.count] = key;
        obj->data.object.values[obj->data.object.count] = val;
        obj->data.object.count = new_count;
        
        p = skip_ws(p);
        if (*p == '}') { *pp = p + 1; return obj; }
        if (*p != ',') { json_free(obj); return NULL; }
        p++;
    }
}

static JSONValue* parse_array(const char** pp) {
    const char* p = skip_ws(*pp);
    if (*p != '[') return NULL;
    p++;
    
    JSONValue* arr = (JSONValue*)calloc(1, sizeof(JSONValue));
    if (!arr) return NULL;
    arr->type = JSON_ARRAY;
    
    p = skip_ws(p);
    if (*p == ']') { *pp = p + 1; return arr; }
    
    while (1) {
        p = skip_ws(p);
        JSONValue* val = parse_value(&p);
        if (!val) { json_free(arr); return NULL; }
        
        size_t new_count = arr->data.array.count + 1;
        JSONValue** new_items = (JSONValue**)realloc(arr->data.array.items, new_count * sizeof(JSONValue*));
        if (!new_items) { json_free(val); json_free(arr); return NULL; }
        arr->data.array.items = new_items;
        arr->data.array.items[arr->data.array.count] = val;
        arr->data.array.count = new_count;
        
        p = skip_ws(p);
        if (*p == ']') { *pp = p + 1; return arr; }
        if (*p != ',') { json_free(arr); return NULL; }
        p++;
    }
}

static JSONValue* parse_value(const char** pp) {
    const char* p = skip_ws(*pp);
    if (!*p) return NULL;
    
    if (*p == '"') {
        JSONValue* v = (JSONValue*)calloc(1, sizeof(JSONValue));
        if (!v) return NULL;
        v->type = JSON_STRING;
        v->data.str = parse_string_raw(&p);
        *pp = p;
        return v;
    }
    if (*p == '{') { *pp = p; return parse_object(pp); }
    if (*p == '[') { *pp = p; return parse_array(pp); }
    if (strncmp(p, "true", 4) == 0) {
        JSONValue* v = (JSONValue*)calloc(1, sizeof(JSONValue));
        if (!v) return NULL;
        v->type = JSON_BOOL; v->data.boolean = 1;
        *pp = p + 4;
        return v;
    }
    if (strncmp(p, "false", 5) == 0) {
        JSONValue* v = (JSONValue*)calloc(1, sizeof(JSONValue));
        if (!v) return NULL;
        v->type = JSON_BOOL; v->data.boolean = 0;
        *pp = p + 5;
        return v;
    }
    if (strncmp(p, "null", 4) == 0) {
        JSONValue* v = (JSONValue*)calloc(1, sizeof(JSONValue));
        if (!v) return NULL;
        v->type = JSON_NULL;
        *pp = p + 4;
        return v;
    }
    if (*p == '-' || isdigit((unsigned char)*p)) {
        JSONValue* v = (JSONValue*)calloc(1, sizeof(JSONValue));
        if (!v) return NULL;
        v->type = JSON_NUMBER;
        v->data.num = parse_number(&p);
        *pp = p;
        return v;
    }
    return NULL;
}

JSONObject* json_parse(const char* str) {
    if (!str) return NULL;
    const char* p = str;
    return parse_value(&p);
}

static void json_value_free(JSONValue* v) {
    if (!v) return;
    if (v->type == JSON_STRING) free(v->data.str);
    else if (v->type == JSON_OBJECT) {
        for (size_t i = 0; i < v->data.object.count; i++) {
            free(v->data.object.keys[i]);
            json_value_free(v->data.object.values[i]);
        }
        free(v->data.object.keys);
        free(v->data.object.values);
    } else if (v->type == JSON_ARRAY) {
        for (size_t i = 0; i < v->data.array.count; i++)
            json_value_free(v->data.array.items[i]);
        free(v->data.array.items);
    }
    free(v);
}

void json_free(JSONObject* obj) {
    json_value_free((JSONValue*)obj);
}

static JSONValue* json_find_key(JSONObject* obj, const char* key) {
    if (!obj || obj->type != JSON_OBJECT) return NULL;
    for (size_t i = 0; i < obj->data.object.count; i++) {
        if (strcmp(obj->data.object.keys[i], key) == 0)
            return obj->data.object.values[i];
    }
    return NULL;
}

const char* json_get_string(JSONObject* obj, const char* key) {
    JSONValue* v = json_find_key(obj, key);
    return (v && v->type == JSON_STRING) ? v->data.str : NULL;
}

double json_get_number(JSONObject* obj, const char* key) {
    JSONValue* v = json_find_key(obj, key);
    return (v && v->type == JSON_NUMBER) ? v->data.num : 0.0;
}

int json_get_bool(JSONObject* obj, const char* key) {
    JSONValue* v = json_find_key(obj, key);
    return (v && v->type == JSON_BOOL) ? v->data.boolean : 0;
}

JSONObject* json_get_object(JSONObject* obj, const char* key) {
    JSONValue* v = json_find_key(obj, key);
    return (v && v->type == JSON_OBJECT) ? (JSONObject*)v : NULL;
}

JSONObject* json_get_array(JSONObject* obj, const char* key) {
    JSONValue* v = json_find_key(obj, key);
    return (v && v->type == JSON_ARRAY) ? (JSONObject*)v : NULL;
}

int json_array_length(JSONObject* arr) {
    return (arr && arr->type == JSON_ARRAY) ? (int)arr->data.array.count : 0;
}

JSONObject* json_array_get(JSONObject* arr, int index) {
    if (!arr || arr->type != JSON_ARRAY || index < 0 || (size_t)index >= arr->data.array.count)
        return NULL;
    return (JSONObject*)arr->data.array.items[index];
}
