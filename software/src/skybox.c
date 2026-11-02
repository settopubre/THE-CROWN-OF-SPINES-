/* Header: include/skybox.h
#ifndef SKYBOX_H
#define SKYBOX_H
#include "vector3d.h"
typedef struct Skybox Skybox;
Skybox* skybox_create(const char* cubemap_dir);
void skybox_render(Skybox* sb, Mat4 view, Mat4 proj);
void skybox_destroy(Skybox* sb);
#endif
*/

#include "skybox.h"
#include "gl_functions.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#if __has_include("stb_image.h")
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#else
#define HAS_STB 0
#endif

/* Minimal shader source for skybox */
static const char* skybox_vs =
    "#version 330 core\n"
    "layout(location = 0) in vec3 aPos;\n"
    "uniform mat4 uView;\n"
    "uniform mat4 uProjection;\n"
    "out vec3 vTexCoords;\n"
    "void main() {\n"
    "    vTexCoords = aPos;\n"
    "    vec4 pos = uProjection * uView * vec4(aPos * 500.0, 1.0);\n" /* Large cube */
    "    gl_Position = pos.xyww;\n" /* xyww ensures depth is always 1.0 */
    "}\n";

static const char* skybox_fs =
    "#version 330 core\n"
    "in vec3 vTexCoords;\n"
    "out vec4 FragColor;\n"
    "uniform samplerCube uSkybox;\n"
    "void main() {\n"
    "    FragColor = texture(uSkybox, vTexCoords);\n"
    "}\n";

struct Skybox {
    GLuint vao, vbo;
    GLuint texture;
    GLuint shader;
};

static GLuint _compile_shader(GLenum type, const char* src) {
    GLuint sh = glCreateShader(type); glShaderSource(sh, 1, &src, NULL); glCompileShader(sh);
    GLint ok; glGetShaderiv(sh, GL_COMPILE_STATUS, &ok);
    if (!ok) { char log[512]; glGetShaderInfoLog(sh, sizeof(log), NULL, log); fprintf(stderr, "Skybox Shader Error: %s\n", log); return 0; }
    return sh;
}

Skybox* skybox_create(const char* cubemap_dir) {
    Skybox* sb = (Skybox*)calloc(1, sizeof(Skybox));
    if (!sb) return NULL;

    float skyboxVertices[] = {
        -1.0f,  1.0f, -1.0f,
        -1.0f, -1.0f, -1.0f,
         1.0f, -1.0f, -1.0f,
         1.0f, -1.0f, -1.0f,
         1.0f,  1.0f, -1.0f,
        -1.0f,  1.0f, -1.0f,
        -1.0f, -1.0f,  1.0f,
        -1.0f, -1.0f, -1.0f,
        -1.0f,  1.0f, -1.0f,
        -1.0f,  1.0f, -1.0f,
        -1.0f,  1.0f,  1.0f,
        -1.0f, -1.0f,  1.0f,
         1.0f, -1.0f, -1.0f,
         1.0f, -1.0f,  1.0f,
         1.0f,  1.0f,  1.0f,
         1.0f,  1.0f,  1.0f,
         1.0f,  1.0f, -1.0f,
         1.0f, -1.0f, -1.0f,
        -1.0f, -1.0f,  1.0f,
        -1.0f,  1.0f,  1.0f,
         1.0f,  1.0f,  1.0f,
         1.0f,  1.0f,  1.0f,
         1.0f, -1.0f,  1.0f,
        -1.0f, -1.0f,  1.0f,
        -1.0f,  1.0f, -1.0f,
         1.0f,  1.0f, -1.0f,
         1.0f,  1.0f,  1.0f,
         1.0f,  1.0f,  1.0f,
        -1.0f,  1.0f,  1.0f,
        -1.0f,  1.0f, -1.0f,
        -1.0f, -1.0f, -1.0f,
        -1.0f, -1.0f,  1.0f,
         1.0f, -1.0f, -1.0f,
         1.0f, -1.0f, -1.0f,
        -1.0f, -1.0f,  1.0f,
         1.0f, -1.0f,  1.0f
    };

    glGenVertexArrays(1, &sb->vao);
    glGenBuffers(1, &sb->vbo);
    glBindVertexArray(sb->vao);
    glBindBuffer(GL_ARRAY_BUFFER, sb->vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(skyboxVertices), skyboxVertices, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);
    glEnableVertexAttribArray(0);
    glBindVertexArray(0);

    /* Create Shader */
    GLuint vs = _compile_shader(GL_VERTEX_SHADER, skybox_vs);
    GLuint fs = _compile_shader(GL_FRAGMENT_SHADER, skybox_fs);
    sb->shader = glCreateProgram();
    glAttachShader(sb->shader, vs); glAttachShader(sb->shader, fs);
    glLinkProgram(sb->shader);
    glDeleteShader(vs); glDeleteShader(fs);

    /* Load Textures */
#if HAS_STB == 1
    glGenTextures(1, &sb->texture);
    glBindTexture(GL_TEXTURE_CUBE_MAP, sb->texture);
    
    const char* faces[] = { "/px.png", "/nx.png", "/py.png", "/ny.png", "/pz.png", "/nz.png" };
    char path[512];
    
    for (unsigned int i = 0; i < 6; i++) {
        snprintf(path, sizeof(path), "%s%s", cubemap_dir, faces[i]);
        int width, height, nrChannels;
        unsigned char* data = stbi_load(path, &width, &height, &nrChannels, 0);
        if (data) {
            glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
            stbi_image_free(data);
        } else {
            fprintf(stderr, "Skybox: Failed to load %s\n", path);
        }
    }
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
#endif

    return sb;
}

void skybox_render(Skybox* sb, Mat4 view, Mat4 proj) {
    if (!sb) return;

    /* Remove translation from view matrix for skybox */
    Mat4 view_only_rot = view;
    view_only_rot.m[12] = 0.0;
    view_only_rot.m[13] = 0.0;
    view_only_rot.m[14] = 0.0;

    float v_mat[16], p_mat[16];
    for(int i=0;i<16;i++) { v_mat[i] = (float)view_only_rot.m[i]; p_mat[i] = (float)proj.m[i]; }

    glDepthMask(GL_FALSE); /* Disable depth writing so it doesn't occlude geometry */
    glDisable(GL_CULL_FACE); /* Render back faces of the cube (inside) */
    
    glUseProgram(sb->shader);
    GLint loc_view = glGetUniformLocation(sb->shader, "uView");
    if(loc_view >= 0) glUniformMatrix4fv(loc_view, 1, GL_FALSE, v_mat);
    
    GLint loc_proj = glGetUniformLocation(sb->shader, "uProjection");
    if(loc_proj >= 0) glUniformMatrix4fv(loc_proj, 1, GL_FALSE, p_mat);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_CUBE_MAP, sb->texture);
    
    glBindVertexArray(sb->vao);
    glDrawArrays(GL_TRIANGLES, 0, 36);
    glBindVertexArray(0);

    glDepthMask(GL_TRUE);
    glEnable(GL_CULL_FACE);
}

void skybox_destroy(Skybox* sb) {
    if (sb) {
        if (sb->vao) glDeleteVertexArrays(1, &sb->vao);
        if (sb->vbo) glDeleteBuffers(1, &sb->vbo);
        if (sb->texture) glDeleteTextures(1, &sb->texture);
        if (sb->shader) glDeleteProgram(sb->shader);
        free(sb);
    }
}
