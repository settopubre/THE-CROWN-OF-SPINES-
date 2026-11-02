#include "asset_importer.h"
#include "mesh.h"
#include "gl_functions.h"
#include "render_server.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#if __has_include("stb_image.h")
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#else
#define HAS_STB 0
#endif

static GLuint _load_texture(const char* path) {
#if HAS_STB == 1
    int w, h, ch;
    unsigned char* data = stbi_load(path, &w, &h, &ch, 4);
    if (!data) return 0;
    
    GLuint tex;
    glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_2D, tex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
    glGenerateMipmap(GL_TEXTURE_2D);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    
    stbi_image_free(data);
    return tex;
#else
    (void)path;
    return 0;
#endif
}

static void _parse_mtl(const char* mtl_path, AssetInstance* inst) {
    if (!mtl_path || !inst) return;
    inst->color[0] = inst->color[1] = inst->color[2] = 0.8f;
    inst->ambient[0] = inst->ambient[1] = inst->ambient[2] = 0.2f;
    inst->specular[0] = inst->specular[1] = inst->specular[2] = 1.0f;
    inst->shininess = 32.0f;
    
    DEFER_FILE(fp) = fopen(mtl_path, "r");
    if (!fp) return;
    char line[256];
    while (fgets(line, sizeof(line), fp)) {
        if (line[0] == '#') continue;
        if (strncmp(line, "Kd", 2) == 0) sscanf(line+2, "%f %f %f", &inst->color[0], &inst->color[1], &inst->color[2]);
        else if (strncmp(line, "Ka", 2) == 0) sscanf(line+2, "%f %f %f", &inst->ambient[0], &inst->ambient[1], &inst->ambient[2]);
        else if (strncmp(line, "Ks", 2) == 0) sscanf(line+2, "%f %f %f", &inst->specular[0], &inst->specular[1], &inst->specular[2]);
        else if (strncmp(line, "Ns", 2) == 0) sscanf(line+2, "%f", &inst->shininess);
        else if (strncmp(line, "map_Kd", 6) == 0) {
            char tex_path[256] = {0};
            /* Resolve relative path */
            const char* slash = strrchr(mtl_path, '/');
            if (slash) {
                size_t dir_len = slash - mtl_path + 1;
                strncpy(tex_path, mtl_path, dir_len);
                sscanf(line+6, "%s", tex_path + dir_len);
            } else {
                sscanf(line+6, "%s", tex_path);
            }
            strncpy(inst->diffuse_texture, tex_path, sizeof(inst->diffuse_texture)-1);
        }
    }
    
    if (inst->diffuse_texture[0] != '\0') {
        inst->texture_id = _load_texture(inst->diffuse_texture);
    }
}

AssetInstance* asset_importer_load(const char* obj_path) {
    if (!obj_path) return NULL;
    AssetInstance* inst = (AssetInstance*)calloc(1, sizeof(AssetInstance));
    if (!inst) return NULL;
    inst->mesh = mesh_load_obj(obj_path);
    if (!inst->mesh) { free(inst); return NULL; }
    inst->color[0] = inst->color[1] = inst->color[2] = 0.8f;
    inst->ambient[0] = inst->ambient[1] = inst->ambient[2] = 0.2f;
    inst->specular[0] = inst->specular[1] = inst->specular[2] = 1.0f;
    inst->shininess = 32.0f;
    inst->position = vec3_create(0.0, 0.0, 0.0);
    inst->rotation = quat_identity();
    inst->scale = vec3_create(1.0, 1.0, 1.0);
    inst->texture_id = 0;
    return inst;
}

AssetInstance* asset_importer_load_with_mtl(const char* obj_path, const char* mtl_path) {
    AssetInstance* inst = asset_importer_load(obj_path);
    if (inst && mtl_path) _parse_mtl(mtl_path, inst);
    return inst;
}

void asset_instance_destroy(AssetInstance* inst) {
    if (!inst) return;
    if (inst->mesh) mesh_destroy(inst->mesh);
    if (inst->texture_id) glDeleteTextures(1, &inst->texture_id);
    free(inst);
}

void asset_instance_set_transform(AssetInstance* inst, Vec3 pos, Quat rot, Vec3 scale) {
    if (inst) {
        inst->position = pos;
        inst->rotation = rot;
        inst->scale = scale;
    }
}

void asset_instance_render(AssetInstance* inst, Mat4 view, Mat4 proj) {
    if (!inst || !inst->mesh) return;
    
    /* Compute model matrix */
    Mat4 T = mat4_translate(inst->position);
    Mat4 R = mat4_from_quat(inst->rotation);
    Mat4 S = mat4_scale(inst->scale);
    Mat4 model = mat4_mul(T, mat4_mul(R, S));
    
    /* Bind texture if available */
    if (inst->texture_id) {
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, inst->texture_id);
        GLint tex_loc = glGetUniformLocation(shader_program, "uTexture");
        if (tex_loc >= 0) glUniform1i(tex_loc, 0);
    }
    
    /* Set material uniforms */
    GLint loc_ambient = glGetUniformLocation(shader_program, "uAmbient");
    if (loc_ambient >= 0) glUniform3f(loc_ambient, inst->ambient[0], inst->ambient[1], inst->ambient[2]);
    
    GLint loc_specular = glGetUniformLocation(shader_program, "uSpecular");
    if (loc_specular >= 0) glUniform3f(loc_specular, inst->specular[0], inst->specular[1], inst->specular[2]);
    
    GLint loc_shininess = glGetUniformLocation(shader_program, "uShininess");
    if (loc_shininess >= 0) glUniform1f(loc_shininess, inst->shininess);

    /* Delegate draw call */
    render_server_draw_mesh(inst->mesh, model);
}
