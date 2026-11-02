#ifndef JSON_PARSER_H
#define JSON_PARSER_H
#pragma once

#include <stddef.h>

typedef enum {
    JSON_OBJECT,
    JSON_ARRAY,
    JSON_STRING,
    JSON_NUMBER,
    JSON_BOOL,
    JSON_NULL
} JSONType;

typedef struct JSONValue {
    JSONType type;
    union {
        char* str;
        double num;
        int boolean;
        struct {
            char** keys;
            struct JSONValue** values;
            size_t count;
        } object;
        struct {
            struct JSONValue** items;
            size_t count;
        } array;
    } data;
} JSONValue;

typedef JSONValue JSONObject;

#ifdef __cplusplus
extern "C" {
#endif

JSONObject* json_parse(const char* str);
void json_free(JSONObject* obj);
const char* json_get_string(JSONObject* obj, const char* key);
double json_get_number(JSONObject* obj, const char* key);
int json_get_bool(JSONObject* obj, const char* key);
JSONObject* json_get_object(JSONObject* obj, const char* key);
JSONObject* json_get_array(JSONObject* obj, const char* key);
int json_array_length(JSONObject* arr);
JSONObject* json_array_get(JSONObject* arr, int index);

#ifdef __cplusplus
}
#endif

#endif /* JSON_PARSER_H */
