#define _POSIX_C_SOURCE 200809L
#include "heightmap_loader.h"
#include "terrain.h"
#include "water.h"
#include "instance_renderer.h"
#include "render_server.h"
#include "ui_overlay.h"
#include "r_ipc.h"
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/keysym.h>
#include <GL/gl.h>
#include <GL/glx.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <math.h>
#include <signal.h>
#include "shaders.h"
#include "engine_abi.h"
#include "gl_functions.h"
#include "vector3d.h"
#include "arsenal.h"
#include "bvh.h"
#include "ai_thread.h"
#include "encoder.h"
#include "retarget.h"
#include "grid.h"
#include "lua_bindings.h"
#include "input_server.h"
#include "editor.h"
#include "splat.h"
#include "web_exporter.h"
#include "ortho_export.h"
#include "flythrough.h"
#include "sequencer.h"
#include "formwork.h"
#include "rebar_architecture.h"
#include "ai_controller.h"
#include "ai_vision.h"
#include "ai_action_executor.h"
#include "bridge_server.h"
#include "floor_plan.h"
#include "physics.h"

typedef GLXContext (*glXCreateContextAttribsARBProc)(Display*, GLXFBConfig, GLXContext, Bool, const int*);

PFNGLCREATESHADERPROC glCreateShader;
PFNGLSHADERSOURCEPROC glShaderSource;
PFNGLCOMPILESHADERPROC glCompileShader;
PFNGLGETSHADERIVPROC glGetShaderiv;
PFNGLGETSHADERINFOLOGPROC glGetShaderInfoLog;
PFNGLDELETESHADERPROC glDeleteShader;
PFNGLCREATEPROGRAMPROC glCreateProgram;
PFNGLATTACHSHADERPROC glAttachShader;
PFNGLBINDATTRIBLOCATIONPROC glBindAttribLocation;
PFNGLLINKPROGRAMPROC glLinkProgram;
PFNGLGETPROGRAMIVPROC glGetProgramiv;
PFNGLGETPROGRAMINFOLOGPROC glGetProgramInfoLog;
PFNGLDELETEPROGRAMPROC glDeleteProgram;
PFNGLUSEPROGRAMPROC glUseProgram;
PFNGLGETUNIFORMLOCATIONPROC glGetUniformLocation;
PFNGLUNIFORMMATRIX4FVPROC glUniformMatrix4fv;
PFNGLUNIFORM1IPROC glUniform1i;
PFNGLUNIFORM1FPROC glUniform1f;
PFNGLUNIFORM3FPROC glUniform3f;
PFNGLUNIFORM4FPROC glUniform4f;
PFNGLGENVERTEXARRAYSPROC glGenVertexArrays;
PFNGLBINDVERTEXARRAYPROC glBindVertexArray;
PFNGLDELETEVERTEXARRAYSPROC glDeleteVertexArrays;
PFNGLGENBUFFERSPROC glGenBuffers;
PFNGLBINDBUFFERPROC glBindBuffer;
PFNGLBUFFERDATAPROC glBufferData;
PFNGLBUFFERSUBDATAPROC glBufferSubData;
PFNGLDELETEBUFFERSPROC glDeleteBuffers;
PFNGLVERTEXATTRIBPOINTERPROC glVertexAttribPointer;
PFNGLENABLEVERTEXATTRIBARRAYPROC glEnableVertexAttribArray;
PFNGLVERTEXATTRIBDIVISORPROC glVertexAttribDivisor;
PFNGLDRAWELEMENTSINSTANCEDPROC glDrawElementsInstanced;

GLuint shader_program = 0;

static Display* dpy = NULL;
static Window win = 0;
static GLXContext ctx = NULL;
static volatile int running = 1;

static Mesh* g_mesh = NULL;
static Camera* g_camera = NULL;
static Gizmo* g_gizmo = NULL;
static Timer* g_timer = NULL;
static Arsenal* g_arsenal = NULL;
BVH* g_bvh = NULL;
AIThread* g_ai_thread = NULL;
static Skeleton* g_skeleton = NULL;
static Encoder* g_encoder = NULL;
static Grid* g_grid = NULL;
static struct RWorker* g_r_worker = NULL;
struct RWorker* g_stat_worker = NULL;

Editor* g_editor = NULL;
AssetDB* g_db = NULL;
AIController* g_ai_controller = NULL;

static Splat* g_splat = NULL;
Terrain* g_terrain = NULL;
static Water* g_water = NULL;
static Formwork* g_formwork = NULL;
static RebarLayout* g_rebar = NULL;
static int g_floor_plan_mode = 0;

static int g_lmb_dragging = 0;
static int g_last_mouse_x = 0, g_last_mouse_y = 0;
static double g_last_time = 0.0;

static UIOverlay* g_ui_overlay = NULL;
static FloorPlan* g_floor_plan = NULL;
static PhysicsWorld* g_physics = NULL;

/* --- GEOMETRY SPAWNER STATE --- */
typedef struct {
    Mesh* mesh;
    int body_id;
} SpawnedObject;
static SpawnedObject g_spawned[64];
static int g_spawned_count = 0;
/* ------------------------------ */

static double get_time(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec + ts.tv_nsec * 1e-9;
}

static int load_gl_functions(void) {
#define LOAD_GL_FUNC(name) name = (void*)glXGetProcAddressARB((const GLubyte*)#name); if(!name) { fprintf(stderr, "ERR: failed to load " #name "\n"); return 0; }
    LOAD_GL_FUNC(glCreateShader); LOAD_GL_FUNC(glShaderSource); LOAD_GL_FUNC(glCompileShader);
    LOAD_GL_FUNC(glGetShaderiv); LOAD_GL_FUNC(glGetShaderInfoLog); LOAD_GL_FUNC(glDeleteShader);
    LOAD_GL_FUNC(glCreateProgram); LOAD_GL_FUNC(glAttachShader); LOAD_GL_FUNC(glBindAttribLocation);
    LOAD_GL_FUNC(glLinkProgram); LOAD_GL_FUNC(glGetProgramiv); LOAD_GL_FUNC(glGetProgramInfoLog);
    LOAD_GL_FUNC(glDeleteProgram); LOAD_GL_FUNC(glUseProgram); LOAD_GL_FUNC(glGetUniformLocation);
    LOAD_GL_FUNC(glUniformMatrix4fv); LOAD_GL_FUNC(glUniform1i); LOAD_GL_FUNC(glUniform1f);
    LOAD_GL_FUNC(glUniform3f); LOAD_GL_FUNC(glUniform4f);
    LOAD_GL_FUNC(glGenVertexArrays); LOAD_GL_FUNC(glBindVertexArray);
    LOAD_GL_FUNC(glGenBuffers); LOAD_GL_FUNC(glBindBuffer); LOAD_GL_FUNC(glBufferData);
    LOAD_GL_FUNC(glBufferSubData);
    LOAD_GL_FUNC(glVertexAttribPointer); LOAD_GL_FUNC(glEnableVertexAttribArray);
    LOAD_GL_FUNC(glVertexAttribDivisor); LOAD_GL_FUNC(glDrawElementsInstanced);
    LOAD_GL_FUNC(glDeleteVertexArrays); LOAD_GL_FUNC(glDeleteBuffers);
#undef LOAD_GL_FUNC
    return 1;
}

static int find_fbconfig(Display* d, int screen, GLXFBConfig* out) {
    int attrs[] = { GLX_X_RENDERABLE, True, GLX_DRAWABLE_TYPE, GLX_WINDOW_BIT,
        GLX_RENDER_TYPE, GLX_RGBA_BIT, GLX_X_VISUAL_TYPE, GLX_TRUE_COLOR,
        GLX_RED_SIZE, 8, GLX_GREEN_SIZE, 8, GLX_BLUE_SIZE, 8, GLX_ALPHA_SIZE, 8,
        GLX_DEPTH_SIZE, 24, GLX_DOUBLEBUFFER, True, None };
    int cnt; GLXFBConfig* fbc = glXChooseFBConfig(d, screen, attrs, &cnt);
    if (!fbc || cnt == 0) return 0;
    for (int i = 0; i < cnt; i++) {
        XVisualInfo* vi = glXGetVisualFromFBConfig(d, fbc[i]);
        if (vi) { int sb, samp; glXGetFBConfigAttrib(d, fbc[i], GLX_SAMPLE_BUFFERS, &sb);
            glXGetFBConfigAttrib(d, fbc[i], GLX_SAMPLES, &samp); *out = fbc[i]; XFree(vi);
            if (sb == 0 && samp == 0) { XFree(fbc); return 1; } } }
    *out = fbc[0]; XFree(fbc); return 1;
}

static GLuint compile_shader(GLenum type, const char* src) {
    GLuint sh = glCreateShader(type); glShaderSource(sh, 1, &src, NULL); glCompileShader(sh);
    GLint ok; glGetShaderiv(sh, GL_COMPILE_STATUS, &ok);
    if (!ok) { char log[512]; glGetShaderInfoLog(sh, sizeof(log), NULL, log);
        fprintf(stderr, "Shader compile error: %s\n", log); glDeleteShader(sh); return 0; }
    return sh;
}

static int init_shaders(void) {
    GLuint vs = compile_shader(GL_VERTEX_SHADER, vertex_shader_source);
    GLuint fs = compile_shader(GL_FRAGMENT_SHADER, fragment_shader_source);
    if (!vs || !fs) return 0;
    shader_program = glCreateProgram();
    glAttachShader(shader_program, vs); glAttachShader(shader_program, fs);
    glBindAttribLocation(shader_program, 0, "aPos"); glBindAttribLocation(shader_program, 1, "aNormal");
    glLinkProgram(shader_program); GLint ok; glGetProgramiv(shader_program, GL_LINK_STATUS, &ok);
    if (!ok) { char log[512]; glGetShaderInfoLog(shader_program, sizeof(log), NULL, log);
        fprintf(stderr, "Shader link error: %s\n", log); glDeleteProgram(shader_program); shader_program = 0; return 0; }
    glDeleteShader(vs); glDeleteShader(fs); return 1;
}

static int init_window(Display** out_dpy, Window* out_win, GLXContext* out_ctx, int w, int h) {
    *out_dpy = XOpenDisplay(NULL); if (!*out_dpy) { fprintf(stderr, "ERR: XOpenDisplay\n"); return 0; }
    int scr = DefaultScreen(*out_dpy); GLXFBConfig fbc;
    if (!find_fbconfig(*out_dpy, scr, &fbc)) { fprintf(stderr, "ERR: no FBConfig\n"); XCloseDisplay(*out_dpy); return 0; }
    XVisualInfo* vi = glXGetVisualFromFBConfig(*out_dpy, fbc);
    if (!vi) { fprintf(stderr, "ERR: no visual\n"); XCloseDisplay(*out_dpy); return 0; }
    XSetWindowAttributes swa; swa.colormap = XCreateColormap(*out_dpy, RootWindow(*out_dpy, vi->screen), vi->visual, AllocNone);
    swa.event_mask = ExposureMask | KeyPressMask | StructureNotifyMask | ButtonPressMask | ButtonReleaseMask | PointerMotionMask;
    *out_win = XCreateWindow(*out_dpy, RootWindow(*out_dpy, vi->screen), 0, 0, w, h, 0,
                             vi->depth, InputOutput, vi->visual, CWColormap | CWEventMask, &swa);
    if (!*out_win) { XFree(vi); XCloseDisplay(*out_dpy); return 0; }
    XStoreName(*out_dpy, *out_win, "3DAIENGINE"); XMapWindow(*out_dpy, *out_win);
    Atom wm_del = XInternAtom(*out_dpy, "WM_DELETE_WINDOW", False); XSetWMProtocols(*out_dpy, *out_win, &wm_del, 1);
    const char* exts = glXQueryExtensionsString(*out_dpy, scr);
    if (!strstr(exts, "GLX_ARB_create_context")) { fprintf(stderr, "ERR: no GLX_ARB_create_context\n"); XFree(vi); return 0; }
    glXCreateContextAttribsARBProc create_ctx = (glXCreateContextAttribsARBProc)glXGetProcAddressARB((const GLubyte*)"glXCreateContextAttribsARB");
    if (!create_ctx) { fprintf(stderr, "ERR: no glXCreateContextAttribsARB\n"); XFree(vi); return 0; }
    int ctx_attrs[] = { GLX_CONTEXT_MAJOR_VERSION_ARB, 3, GLX_CONTEXT_MINOR_VERSION_ARB, 3,
        GLX_CONTEXT_PROFILE_MASK_ARB, GLX_CONTEXT_CORE_PROFILE_BIT_ARB, None };
    *out_ctx = create_ctx(*out_dpy, fbc, NULL, True, ctx_attrs);
    if (!*out_ctx) { fprintf(stderr, "ERR: context creation failed\n"); XFree(vi); return 0; }
    glXMakeCurrent(*out_dpy, *out_win, *out_ctx);
    if (!load_gl_functions()) { XFree(vi); return 0; }
    XFree(vi); return 1;
}

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

static Mesh* mesh_create_hollow_cylinder(float r_out, float r_in, float height, int segments) {
    Mesh* m = (Mesh*)calloc(1, sizeof(Mesh));
    int verts_per_ring = segments + 1;
    m->vertex_count = verts_per_ring * 4;
    m->index_count = segments * 24;
    m->positions = (float*)malloc(m->vertex_count * 3 * sizeof(float));
    m->normals   = (float*)malloc(m->vertex_count * 3 * sizeof(float));
    m->indices   = (uint32_t*)malloc(m->index_count * sizeof(uint32_t));
    float half_h = height * 0.5f;
    for (int i = 0; i <= segments; i++) {
        float angle = 2.0f * M_PI * i / segments;
        float cx = cosf(angle), cz = sinf(angle);
        int ot = i; m->positions[ot*3] = cx*r_out; m->positions[ot*3+1] = half_h; m->positions[ot*3+2] = cz*r_out;
        m->normals[ot*3] = cx; m->normals[ot*3+1] = 0.0f; m->normals[ot*3+2] = cz;
        int ob = i + verts_per_ring; m->positions[ob*3] = cx*r_out; m->positions[ob*3+1] = -half_h; m->positions[ob*3+2] = cz*r_out;
        m->normals[ob*3] = cx; m->normals[ob*3+1] = 0.0f; m->normals[ob*3+2] = cz;
        int it = i + verts_per_ring * 2; m->positions[it*3] = cx*r_in; m->positions[it*3+1] = half_h; m->positions[it*3+2] = cz*r_in;
        m->normals[it*3] = -cx; m->normals[it*3+1] = 0.0f; m->normals[it*3+2] = -cz;
        int ib = i + verts_per_ring * 3; m->positions[ib*3] = cx*r_in; m->positions[ib*3+1] = -half_h; m->positions[ib*3+2] = cz*r_in;
        m->normals[ib*3] = -cx; m->normals[ib*3+1] = 0.0f; m->normals[ib*3+2] = -cz;
    }
    uint32_t idx = 0;
    for (int i = 0; i < segments; i++) {
        int next = (i + 1) % verts_per_ring;
        m->indices[idx++] = i; m->indices[idx++] = i + verts_per_ring; m->indices[idx++] = next;
        m->indices[idx++] = next; m->indices[idx++] = i + verts_per_ring; m->indices[idx++] = next + verts_per_ring;
        int it = i + verts_per_ring*2, ib = i + verts_per_ring*3;
        int nt = next + verts_per_ring*2, nb = next + verts_per_ring*3;
        m->indices[idx++] = it; m->indices[idx++] = nt; m->indices[idx++] = ib;
        m->indices[idx++] = nt; m->indices[idx++] = nb; m->indices[idx++] = ib;
        m->indices[idx++] = i; m->indices[idx++] = next; m->indices[idx++] = it;
        m->indices[idx++] = next; m->indices[idx++] = nt; m->indices[idx++] = it;
        m->indices[idx++] = i + verts_per_ring; m->indices[idx++] = ib; m->indices[idx++] = next + verts_per_ring;
        m->indices[idx++] = next + verts_per_ring; m->indices[idx++] = ib; m->indices[idx++] = nb;
    }
    return m;
}

static void draw_tank_boundary(float w, float h, float d) {
    float hw = w*0.5f, hh = h*0.5f, hd = d*0.5f;
    float verts[] = {
        -hw,-hh,-hd, 0.3,0.5,0.8,  hw,-hh,-hd, 0.3,0.5,0.8,
         hw,-hh,-hd, 0.3,0.5,0.8,  hw, hh,-hd, 0.3,0.5,0.8,
         hw, hh,-hd, 0.3,0.5,0.8, -hw, hh,-hd, 0.3,0.5,0.8,
        -hw, hh,-hd, 0.3,0.5,0.8, -hw,-hh,-hd, 0.3,0.5,0.8,
        -hw,-hh, hd, 0.3,0.5,0.8,  hw,-hh, hd, 0.3,0.5,0.8,
         hw,-hh, hd, 0.3,0.5,0.8,  hw, hh, hd, 0.3,0.5,0.8,
         hw, hh, hd, 0.3,0.5,0.8, -hw, hh, hd, 0.3,0.5,0.8,
        -hw, hh, hd, 0.3,0.5,0.8, -hw,-hh, hd, 0.3,0.5,0.8,
        -hw,-hh,-hd, 0.3,0.5,0.8, -hw,-hh, hd, 0.3,0.5,0.8,
         hw,-hh,-hd, 0.3,0.5,0.8,  hw,-hh, hd, 0.3,0.5,0.8,
         hw, hh,-hd, 0.3,0.5,0.8,  hw, hh, hd, 0.3,0.5,0.8,
        -hw, hh,-hd, 0.3,0.5,0.8, -hw, hh, hd, 0.3,0.5,0.8
    };
    render_server_draw_lines(verts, 24, vec3_create(0.3, 0.5, 0.8));
}

static void draw_sidebar_menu(void) {
    if (!g_ui_overlay) return;
    ui_overlay_begin(g_ui_overlay);
    
    /* Sidebar Background */
    float sx = 10.0f, sy = 10.0f, sw = 220.0f, sh = 350.0f;
    float bg[] = {
        sx, sy, 0, 0.1, 0.1, 0.15,  sx+sw, sy, 0, 0.1, 0.1, 0.15,
        sx+sw, sy, 0, 0.1, 0.1, 0.15, sx+sw, sy+sh, 0, 0.1, 0.1, 0.15,
        sx+sw, sy+sh, 0, 0.1, 0.1, 0.15, sx, sy+sh, 0, 0.1, 0.1, 0.15,
        sx, sy+sh, 0, 0.1, 0.1, 0.15, sx, sy, 0, 0.1, 0.1, 0.15
    };
    glBindBuffer(GL_ARRAY_BUFFER, g_ui_overlay->vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(bg), bg, GL_DYNAMIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6*sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6*sizeof(float), (void*)(3*sizeof(float)));
    glEnableVertexAttribArray(1);
    glDrawArrays(GL_TRIANGLES, 0, 6);

    /* Sidebar Border */
    float border[] = {
        sx, sy, 0, 0.4, 0.6, 0.9,  sx+sw, sy, 0, 0.4, 0.6, 0.9,
        sx+sw, sy, 0, 0.4, 0.6, 0.9, sx+sw, sy+sh, 0, 0.4, 0.6, 0.9,
        sx+sw, sy+sh, 0, 0.4, 0.6, 0.9, sx, sy+sh, 0, 0.4, 0.6, 0.9,
        sx, sy+sh, 0, 0.4, 0.6, 0.9, sx, sy, 0, 0.4, 0.6, 0.9
    };
    glBufferData(GL_ARRAY_BUFFER, sizeof(border), border, GL_DYNAMIC_DRAW);
    glDrawArrays(GL_LINES, 0, 8);

    /* Menu Text */
    ui_overlay_text(g_ui_overlay, sx + 15, sy + sh - 30, "=== 3DAI TOOLBOX ===", 0.4, 0.8, 1.0);
    ui_overlay_text(g_ui_overlay, sx + 15, sy + sh - 60, "[1] Spawn Physics Box", 1.0, 1.0, 1.0);
    ui_overlay_text(g_ui_overlay, sx + 15, sy + sh - 90, "[2] Spawn Physics Sphere", 1.0, 1.0, 1.0);
    ui_overlay_text(g_ui_overlay, sx + 15, sy + sh - 120, "[3] Clear All Objects", 1.0, 0.5, 0.5);
    ui_overlay_text(g_ui_overlay, sx + 15, sy + sh - 160, "--------------------", 0.5, 0.5, 0.5);
    ui_overlay_text(g_ui_overlay, sx + 15, sy + sh - 190, "[P] Floor Plan Mode", 1.0, 1.0, 1.0);
    ui_overlay_text(g_ui_overlay, sx + 15, sy + sh - 220, "[T] Toggle Terminal", 1.0, 1.0, 1.0);
    ui_overlay_text(g_ui_overlay, sx + 15, sy + sh - 250, "[Mouse] Orbit Camera", 1.0, 1.0, 1.0);
    ui_overlay_text(g_ui_overlay, sx + 15, sy + sh - 280, "[ESC] Quit Engine", 1.0, 0.5, 0.5);
    
    char count_str[64];
    snprintf(count_str, sizeof(count_str), "Objects: %d / 64", g_spawned_count);
    ui_overlay_text(g_ui_overlay, sx + 15, sy + 30, count_str, 0.5, 1.0, 0.5);

    ui_overlay_end(g_ui_overlay);
}

static void cleanup(Display* d, Window w, GLXContext c) {
    if (g_ui_overlay) { ui_overlay_destroy(g_ui_overlay); g_ui_overlay = NULL; }
    if (shader_program) { glDeleteProgram(shader_program); shader_program = 0; }
    if (g_mesh) mesh_destroy(g_mesh);
    if (g_camera) camera_destroy(g_camera);
    if (g_gizmo) gizmo_destroy(g_gizmo);
    if (g_timer) engine_timer_destroy(g_timer);
    if (g_arsenal) arsenal_destroy(g_arsenal);
    if (g_bvh) bvh_destroy(g_bvh);
    if (g_ai_thread) ai_thread_stop(g_ai_thread);
    if (g_skeleton) skeleton_destroy(g_skeleton);
    if (g_encoder) encoder_destroy(g_encoder);
    if (g_editor) editor_destroy(g_editor);
    if (g_splat) splat_destroy(g_splat);
    if (g_terrain) terrain_destroy(g_terrain);
    if (g_water) water_destroy(g_water);
    if (g_formwork) formwork_destroy(g_formwork);
    if (g_rebar) rebar_layout_destroy(g_rebar);
    if (g_ai_controller) ai_controller_destroy(g_ai_controller);
    if (g_floor_plan) floor_plan_destroy(g_floor_plan);
    if (g_physics) physics_destroy(g_physics);
    input_server_shutdown();
    if (g_grid) grid_destroy(g_grid);
    if (g_r_worker) r_worker_stop(g_r_worker);
    if (g_stat_worker) r_stat_worker_stop(g_stat_worker);
    lua_bindings_close();
    if (c) { glXMakeCurrent(d, None, NULL); glXDestroyContext(d, c); }
    if (w) XDestroyWindow(d, w);
    if (d) XCloseDisplay(d);
}

int main(int argc, char* argv[]) {
    (void)argc; (void)argv; /* Explicitly silence unused parameter warnings */
    signal(SIGPIPE, SIG_IGN);
    
    if (!init_window(&dpy, &win, &ctx, 800, 600)) return 1;
    if (!init_shaders()) { cleanup(dpy, win, ctx); return 1; }
    
    g_camera = camera_create_orbit(15.0f);
    g_gizmo = gizmo_create();
    g_timer = engine_timer_create();
    g_arsenal = arsenal_create(3.0f, 32);
    g_grid = grid_create(1.0f, 20.0f);
    g_floor_plan = floor_plan_get();
    g_physics = physics_create();
    physics_add_ground(g_physics);
    input_server_init();
    
    g_db = asset_db_create(256);
    if (g_db) asset_db_scan_directory(g_db, "./assets");
    g_editor = editor_create(g_db);
    
    g_skeleton = skeleton_create();
    g_ai_controller = ai_controller_create(g_skeleton);
    
    Heightmap* hm = heightmap_load_raw("assets/terrain.raw", 512, 512);
    if (hm) {
        g_terrain = terrain_create_from_heightmap(hm->data, hm->width, hm->height, 2.0f, 5.0f);
        heightmap_destroy(hm);
    }
    g_splat = splat_load("assets/scene.ply");
    
    // --- CROWN OF SPINES: TANK & 10 PISTONS INIT ---
    float tank_w = 20.0f, tank_h = 10.0f, tank_d = 20.0f;
    g_water = water_create(tank_w, tank_d, 0.5f);
    water_set_wave_type(g_water, 1); // Gerstner waves

    Mesh* piston_mesh = mesh_create_hollow_cylinder(0.6f, 0.4f, 3.0f, 16);
    InstanceRenderer* piston_renderer = instance_renderer_create(piston_mesh, 10);
    
    float crown_freq[5] = {1.0f, 1.5f, 2.0f, 3.0f, 4.5f};
    float crown_phase[5] = {0.0f, 0.0f, 0.0f, 0.0f, 0.0f};
    // -----------------------------------------------
    
    // --- GATED R WORKER STARTUP ---
    if (!r_worker_probe_availability()) {
        printf("[WARN] R+rayshader not available — statistical worker disabled\n");
    } else {
        g_r_worker = r_worker_start();
        g_stat_worker = r_stat_worker_start();
        if (g_stat_worker) printf("  Statistical AI brain: ONLINE\n");
    }
    // ------------------------------
    
    bridge_server_start();
    lua_bindings_init();
    
    g_ai_thread = ai_thread_start("http://localhost:11434", NULL, "deepseek-coder:6.7b", BACKEND_OLLAMA);
    
    printf("3DAIENGINE: GL context active, 800x600\n");
    printf("  Use Sidebar Menu [1, 2, 3] to spawn geometry.\n");
    
    g_ui_overlay = ui_overlay_create(800, 600);
    g_last_time = get_time();
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    
    while (running) {
        double now = get_time();
        double dt = now - g_last_time;
        g_last_time = now;
        engine_timer_tick(g_timer);
        
        if (g_physics) physics_step(g_physics, (float)dt);
        
        // Update Pistons
        for (int f = 0; f < 5; f++) crown_phase[f] += crown_freq[f] * dt;
        float piston_spacing = tank_w / 10.0f;
        float tank_edge_z = -(tank_d * 0.5f) + 2.0f; 
        for (int p = 0; p < 10; p++) {
            float px = -(tank_w * 0.5f) + (p + 0.5f) * piston_spacing;
            float pz = tank_edge_z;
            float displacement = 0.0f;
            for (int f = 0; f < 5; f++) displacement += sinf(crown_phase[f] + (p * (f + 1) * 0.5f)) * 1.2f; 
            float py = fmaxf(-tank_h * 0.4f, fminf(tank_h * 0.4f, displacement));
            Mat4 model = mat4_translate(vec3_create(px, py, pz));
            instance_renderer_add_instance(piston_renderer, model, vec3_create(0.8f, 0.5f, 0.2f));
        }
        
        input_server_poll_events(dpy);
        InputEvent ev;
        while (input_server_get_event(&ev)) {
            if (ev.type == 0) {
                if (ui_terminal_is_active()) {
                    if (ev.key == ENGINE_KEY_ESCAPE) ui_terminal_toggle();
                    else if (ev.key == XK_Return || ev.key == XK_KP_Enter) ui_terminal_input_enter();
                    else if (ev.key == XK_BackSpace) ui_terminal_input_backspace();
                    else if (ev.key >= 32 && ev.key <= 126) ui_terminal_input_char((char)ev.key);
                    continue;
                }
                
                if (ev.key == ENGINE_KEY_ESCAPE) running = 0;
                else if (ev.key == ENGINE_KEY_G) grid_toggle_visible(g_grid);
                else if (ev.key == 'P' || ev.key == 'p') {
                    g_floor_plan_mode = !g_floor_plan_mode;
                    if (g_floor_plan_mode && g_floor_plan) floor_plan_clear(g_floor_plan);
                }
                else if (ev.key == 'C' || ev.key == 'c') {
                    if (g_floor_plan_mode && g_floor_plan) floor_plan_close_loop(g_floor_plan);
                }
                else if (ev.key == XK_Return || ev.key == XK_KP_Enter) {
                    if (g_floor_plan_mode && g_floor_plan) { floor_plan_extrude_walls(g_floor_plan); g_floor_plan_mode = 0; }
                }
                else if (ev.key == 'T' || ev.key == 't') ui_terminal_toggle();
                
                /* --- GEOMETRY SPAWNER TOOLS --- */
                else if (ev.key == '1') {
                    if (g_spawned_count < 64 && g_physics) {
                        Vec3 pos = vec3_create((rand()%10-5)*1.0f, 8.0f, (rand()%10-5)*1.0f);
                        int id = physics_add_box(g_physics, pos, vec3_create(0.5,0.5,0.5), 5.0f);
                        g_spawned[g_spawned_count].mesh = NULL; // Placeholder for visual mesh
                        g_spawned[g_spawned_count].body_id = id;
                        g_spawned_count++;
                        printf("[TOOL] Spawned Physics Box (ID: %d)\n", id);
                    }
                }
                else if (ev.key == '2') {
                    if (g_spawned_count < 64 && g_physics) {
                        Vec3 pos = vec3_create((rand()%10-5)*1.0f, 8.0f, (rand()%10-5)*1.0f);
                        int id = physics_add_sphere(g_physics, pos, 0.5f, 5.0f);
                        g_spawned[g_spawned_count].mesh = NULL;
                        g_spawned[g_spawned_count].body_id = id;
                        g_spawned_count++;
                        printf("[TOOL] Spawned Physics Sphere (ID: %d)\n", id);
                    }
                }
                else if (ev.key == '3') {
                    for (int i = 0; i < g_spawned_count; i++) {
                        if (g_physics) physics_remove_body(g_physics, g_spawned[i].body_id);
                    }
                    g_spawned_count = 0;
                    printf("[TOOL] Cleared all spawned objects.\n");
                }
                /* ------------------------------ */
            } else if (ev.type == 1) {
                if (ev.button == MOUSE_LEFT && g_floor_plan_mode && g_floor_plan) {
                    float wx = (ev.x - 400.0f) * 0.1f;
                    float wz = (ev.y - 300.0f) * 0.1f;
                    Vec3 world_pos = vec3_create(wx, 0.0f, wz);
                    if (g_grid) world_pos = grid_snap(g_grid, world_pos);
                    floor_plan_add_point(g_floor_plan, world_pos);
                } else if (ev.button == MOUSE_LEFT) {
                    g_lmb_dragging = 1;
                    g_last_mouse_x = ev.x;
                    g_last_mouse_y = ev.y;
                } else if (ev.button == 4) camera_mouse_scroll(g_camera, -1.0f);
                else if (ev.button == 5) camera_mouse_scroll(g_camera, 1.0f);
            } else if (ev.type == 2) {
                if (ev.button == MOUSE_LEFT) g_lmb_dragging = 0;
            } else if (ev.type == 3) {
                if (g_lmb_dragging) {
                    int dx = ev.x - g_last_mouse_x, dy = ev.y - g_last_mouse_y;
                    camera_mouse_drag(g_camera, (float)dx, (float)dy);
                    g_last_mouse_x = ev.x; g_last_mouse_y = ev.y;
                }
            }
        }

        XEvent xev;
        if (XCheckTypedEvent(dpy, ClientMessage, &xev)) {
            if ((Atom)xev.xclient.data.l[0] == XInternAtom(dpy, "WM_DELETE_WINDOW", False)) running = 0;
        }
        
        camera_update(g_camera, (float)dt);
        Mat4 view = camera_get_view_matrix(g_camera);
        Mat4 proj = camera_get_projection_matrix(g_camera, 800.0f / 600.0f);
        
        glClearColor(0.1f, 0.1f, 0.15f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        
        // Render Tank, Water, Pistons
        draw_tank_boundary(tank_w, tank_h, tank_d);
        if (g_water) {
            water_update(g_water, (float)dt, 0.0f, 1.0f);
            water_render(g_water, view, proj, vec3_create(0,0,0));
        }
        instance_renderer_render(piston_renderer, view, proj);
        instance_renderer_clear(piston_renderer);

        if (g_arsenal) arsenal_render(g_arsenal, view, proj);
        if (g_mesh) { if (g_skeleton) skinning_apply(g_mesh, g_skeleton); mesh_render(g_mesh); }
        gizmo_render(g_gizmo, view, proj);
        if (g_grid) grid_render(g_grid, view, proj);
        if (g_splat) splat_render(g_splat, view, proj, 2.0f);
        if (g_terrain) terrain_render(g_terrain, view, proj);
        if (g_floor_plan) floor_plan_render(g_floor_plan, view, proj);
        
        // Render Sidebar Menu
        draw_sidebar_menu();
        if (g_ui_overlay) {
            ui_overlay_begin(g_ui_overlay);
            ui_terminal_render(g_ui_overlay);
            ui_overlay_end(g_ui_overlay);
        }
        
        glXSwapBuffers(dpy, win);
        double target = 1.0 / 60.0;
        if (dt < target) { long ns = (long)((target - dt) * 1e9); struct timespec ts = {0, ns}; nanosleep(&ts, NULL); }
        
        static double fps_print_acc = 0.0; fps_print_acc += dt;
        if (fps_print_acc >= 1.0) {
            printf("\r[3DAI] FPS: %.1f | Physics: ON | Objects: %d", engine_timer_get_fps(g_timer), g_spawned_count);
            fflush(stdout); fps_print_acc = 0.0;
        }
    }
    
    cleanup(dpy, win, ctx);
    printf("\n3DAIENGINE: shutdown complete\n");
    return 0;
}
