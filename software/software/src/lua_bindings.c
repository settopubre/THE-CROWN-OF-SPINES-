#include "lua_bindings.h"
#include "engine_abi.h"
#include "asset_importer.h"
#include "terrain.h"
#include "thermal.h"
#include "editor.h"
#include "asset_db.h"
#include <stdio.h>

extern Editor* g_editor;
extern AssetDB* g_db;
extern Terrain* g_terrain;

#ifdef HAS_LUA
#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>

static lua_State* L = NULL;
static char last_error[512] = {0};

static int l_engine_log(lua_State* L) {
    const char* msg = luaL_checkstring(L, 1);
    printf("[LUA] %s\n", msg);
    return 0;
}

static int l_engine_load_asset(lua_State* L) {
    const char* name = luaL_checkstring(L, 1);
    printf("[LUA] Loading asset: %s\n", name);
    lua_pushboolean(L, 1);
    return 1;
}

static int l_engine_place_asset(lua_State* L) {
    const char* name = luaL_checkstring(L, 1);
    double x = luaL_checknumber(L, 2);
    double y = luaL_checknumber(L, 3);
    double z = luaL_checknumber(L, 4);
    
    if (!g_editor || !g_db) {
        lua_pushstring(L, "Editor or AssetDB not initialized");
        return 1;
    }
    
    const AssetEntry* entry = asset_db_find_by_name(g_db, name);
    if (!entry) {
        char err[256];
        snprintf(err, sizeof(err), "Asset '%s' not found", name);
        lua_pushstring(L, err);
        return 1;
    }
    
    Vec3 pos = vec3_create(x, y, z);
    editor_place_asset(g_editor, entry->id, pos);
    lua_pushboolean(L, 1);
    return 1;
}

static int l_engine_get_terrain_height(lua_State* L) {
    double x = luaL_checknumber(L, 1);
    double z = luaL_checknumber(L, 2);
    double h = 0.0;
    if (g_terrain) {
        h = terrain_get_height(g_terrain, (float)x, (float)z);
    }
    lua_pushnumber(L, h);
    return 1;
}

static int l_engine_set_camera(lua_State* L) {
    double x = luaL_checknumber(L, 1);
    double y = luaL_checknumber(L, 2);
    double z = luaL_checknumber(L, 3);
    double tx = luaL_checknumber(L, 4);
    double ty = luaL_checknumber(L, 5);
    double tz = luaL_checknumber(L, 6);
    printf("[LUA] Camera set to (%.2f,%.2f,%.2f) looking at (%.2f,%.2f,%.2f)\n", x,y,z,tx,ty,tz);
    return 0;
}

static int l_engine_get_thermal(lua_State* L) {
    float temp = thermal_get_temperature();
    lua_pushnumber(L, temp);
    return 1;
}

static const struct luaL_Reg engine_functions[] = {
    {"engine_log", l_engine_log},
    {"engine_load_asset", l_engine_load_asset},
    {"engine_place_asset", l_engine_place_asset},
    {"engine_get_terrain_height", l_engine_get_terrain_height},
    {"engine_set_camera", l_engine_set_camera},
    {"engine_get_thermal", l_engine_get_thermal},
    {NULL, NULL}
};

int lua_bindings_init(void) {
    last_error[0] = '\0';
    L = luaL_newstate();
    if (!L) { snprintf(last_error, sizeof(last_error), "Failed to create Lua state"); return -1; }
    luaL_openlibs(L);
    luaL_newlibtable(L, engine_functions);
    luaL_setfuncs(L, engine_functions, 0);
    lua_setglobal(L, "engine");
    return 0;
}

void lua_bindings_close(void) {
    if (L) { lua_close(L); L = NULL; }
}

const char* lua_bindings_last_error(void) {
    return last_error;
}

int lua_bindings_load_file(const char* filename) {
    if (!L || !filename) return -1;
    int status = luaL_loadfile(L, filename);
    if (status != LUA_OK) {
        snprintf(last_error, sizeof(last_error), "%s", lua_tostring(L, -1));
        lua_pop(L, 1);
        return -1;
    }
    status = lua_pcall(L, 0, 0, 0);
    if (status != LUA_OK) {
        snprintf(last_error, sizeof(last_error), "%s", lua_tostring(L, -1));
        lua_pop(L, 1);
        return -1;
    }
    return 0;
}

int lua_bindings_call(const char* func_name) {
    if (!L || !func_name) return -1;
    lua_getglobal(L, func_name);
    if (!lua_isfunction(L, -1)) {
        snprintf(last_error, sizeof(last_error), "Function '%s' not found", func_name);
        lua_pop(L, 1);
        return -1;
    }
    int status = lua_pcall(L, 0, 0, 0);
    if (status != LUA_OK) {
        snprintf(last_error, sizeof(last_error), "%s", lua_tostring(L, -1));
        lua_pop(L, 1);
        return -1;
    }
    return 0;
}

void lua_bindings_set_number(const char* name, double value) {
    if (L && name) { lua_pushnumber(L, value); lua_setglobal(L, name); }
}

double lua_bindings_get_number(const char* name) {
    if (!L || !name) return 0.0;
    lua_getglobal(L, name);
    double val = lua_tonumber(L, -1);
    lua_pop(L, 1);
    return val;
}

#else /* !HAS_LUA */

static char last_error[512] = {0};

int lua_bindings_init(void) { last_error[0] = '\0'; return 0; }
void lua_bindings_close(void) {}
int lua_bindings_load_file(const char* filename) { (void)filename; return 0; }
int lua_bindings_call(const char* func_name) { (void)func_name; return 0; }
void lua_bindings_set_number(const char* name, double value) { (void)name; (void)value; }
double lua_bindings_get_number(const char* name) { (void)name; return 0.0; }
const char* lua_bindings_last_error(void) { return last_error; }

#endif /* HAS_LUA */
