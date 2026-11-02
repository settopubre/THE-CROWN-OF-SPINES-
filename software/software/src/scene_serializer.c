#include "scene_serializer.h"
#include "json_parser.h"
#include "vector3d.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

Scene* scene_create(uint32_t capacity) {
    if (capacity == 0) capacity = 64;
    Scene* s = (Scene*)calloc(1, sizeof(Scene));
    if (!s) return NULL;
    s->objects = (SceneObject*)calloc(capacity, sizeof(SceneObject));
    if (!s->objects) { free(s); return NULL; }
    s->capacity = capacity;
    s->count = 0;
    return s;
}

void scene_destroy(Scene* scene) {
    if (scene) {
        free(scene->objects);
        free(scene);
    }
}

void scene_add_object(Scene* scene, const SceneObject* obj) {
    if (!scene || !obj) return;
    if (scene->count >= scene->capacity) {
        uint32_t new_cap = scene->capacity * 2;
        SceneObject* new_objs = realloc(scene->objects, new_cap * sizeof(SceneObject));
        if (!new_objs) return;
        scene->objects = new_objs;
        scene->capacity = new_cap;
    }
    scene->objects[scene->count++] = *obj;
}

int scene_save(const Scene* scene, const char* filename) {
    if (!scene || !filename) return 0;
    DEFER_FILE(fp) = fopen(filename, "w");
    if (!fp) return 0;
    
    fprintf(fp, "{\n  \"terrain\": \"%s\",\n  \"objects\": [\n", scene->terrain_file);
    for (uint32_t i = 0; i < scene->count; i++) {
        SceneObject* o = &scene->objects[i];
        fprintf(fp, "    {\n");
        fprintf(fp, "      \"name\": \"%s\",\n", o->asset_name);
        fprintf(fp, "      \"pos\": { \"x\": %.4f, \"y\": %.4f, \"z\": %.4f },\n", o->position.x, o->position.y, o->position.z);
        fprintf(fp, "      \"rot\": { \"w\": %.4f, \"x\": %.4f, \"y\": %.4f, \"z\": %.4f },\n", o->rotation.w, o->rotation.x, o->rotation.y, o->rotation.z);
        fprintf(fp, "      \"scale\": { \"x\": %.4f, \"y\": %.4f, \"z\": %.4f }\n", o->scale.x, o->scale.y, o->scale.z);
        fprintf(fp, "    }%s\n", (i < scene->count - 1) ? "," : "");
    }
    fprintf(fp, "  ]\n}\n");
    return 1;
}

Scene* scene_load(const char* filename) {
    if (!filename) return NULL;
    
    DEFER_FILE(fp) = fopen(filename, "r");
    if (!fp) return NULL;
    
    fseek(fp, 0, SEEK_END);
    long fsize = ftell(fp);
    fseek(fp, 0, SEEK_SET);
    char* buffer = (char*)malloc(fsize + 1);
    if (!buffer) return NULL;
    fread(buffer, 1, fsize, fp);
    buffer[fsize] = '\0';
    
    JSONObject* root = json_parse(buffer);
    free(buffer);
    if (!root) return NULL;
    
    Scene* scene = scene_create(32);
    const char* terr = json_get_string(root, "terrain");
    if (terr) strncpy(scene->terrain_file, terr, sizeof(scene->terrain_file)-1);
    
    JSONObject* arr = json_get_array(root, "objects");
    if (arr) {
        int len = json_array_length(arr);
        for (int i = 0; i < len; i++) {
            JSONObject* obj = json_array_get(arr, i);
            if (!obj) continue;
            SceneObject so;
            const char* name = json_get_string(obj, "name");
            if (name) strncpy(so.asset_name, name, sizeof(so.asset_name)-1);
            else so.asset_name[0] = '\0';
            
            JSONObject* pos = json_get_object(obj, "pos");
            so.position = vec3_create(
                json_get_number(pos, "x"), json_get_number(pos, "y"), json_get_number(pos, "z"));
            
            JSONObject* rot = json_get_object(obj, "rot");
            so.rotation = (Quat){
                json_get_number(rot, "w"), json_get_number(rot, "x"),
                json_get_number(rot, "y"), json_get_number(rot, "z")};
            
            JSONObject* sc = json_get_object(obj, "scale");
            so.scale = vec3_create(
                json_get_number(sc, "x"), json_get_number(sc, "y"), json_get_number(sc, "z"));
                
            scene_add_object(scene, &so);
        }
    }
    
    json_free(root);
    return scene;
}
