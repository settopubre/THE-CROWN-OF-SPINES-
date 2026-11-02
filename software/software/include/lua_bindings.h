#ifndef LUA_BINDINGS_H
#define LUA_BINDINGS_H

#ifdef __cplusplus
extern "C" {
#endif

int lua_bindings_init(void);
void lua_bindings_close(void);
int lua_bindings_load_file(const char* filename);
int lua_bindings_call(const char* func_name);
void lua_bindings_set_number(const char* name, double value);
double lua_bindings_get_number(const char* name);

#ifdef __cplusplus
}
#endif

#endif /* LUA_BINDINGS_H */
