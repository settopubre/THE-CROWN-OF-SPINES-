CC = gcc
CXX = g++

CFLAGS_BASE = -std=c11 -Wall -Wextra -I./include -I/usr/include/bullet -pthread -DHAS_LUA -I/usr/include/lua5.3
CXXFLAGS_BASE = -std=c++11 -Wall -Wextra -I./include -I./third_party/imgui -I./third_party/imgui/backends -I/usr/include/bullet

CFLAGS_ENGINE = $(CFLAGS_BASE) -O2 -march=native
CXXFLAGS_ENGINE = $(CXXFLAGS_BASE) -O2 -march=native

CFLAGS_DEBUG = $(CFLAGS_BASE) -g -O0 -DDEBUG -fsanitize=address -fno-omit-frame-pointer
CXXFLAGS_DEBUG = $(CXXFLAGS_BASE) -g -O0 -DDEBUG -fsanitize=address -fno-omit-frame-pointer

CFLAGS_GRAPH = -std=c11 -Wall -Wextra -O2 -march=native -I./include -fopenmp
LDFLAGS_ENGINE = -lX11 -lGL -lm -pthread -lstdc++ -llua5.3 -lBulletDynamics -lBulletCollision -lLinearMath
LDFLAGS_DEBUG = -lX11 -lGL -lm -pthread -lstdc++ -llua5.3 -lBulletDynamics -lBulletCollision -lLinearMath -fsanitize=address
LDFLAGS_GRAPH = -lm -fopenmp -lgomp

ENGINE_SRC = src/engine.c \
             src/ai_controller.c \
             src/ai_vision.c \
             src/ai_action_executor.c \
             src/obj_loader.c \
             src/camera.c \
             src/gizmo.c \
             src/timer.c \
             src/vector3d.c \
             src/scene_graph.c \
             src/skeleton.c \
             src/kdtree.c \
             src/autorig.c \
             src/ik_solver.c \
             src/skinning.c \
             src/command_buffer.c \
             src/r_ipc.c \
             src/http_client.c \
             src/json_parser.c \
             src/ai_thread.c \
             src/kalman.c \
             src/retarget.c \
             src/interp.c \
             src/arsenal.c \
             src/bvh.c \
             src/tracker.c \
             src/spline.c \
             src/sequencer.c \
             src/encoder.c \
             src/thermal.c \
             src/decimate.c \
             src/scaler.c \
             src/splat.c \
             src/splat_diff.c \
             src/formwork.c \
             src/gravel.c \
             src/rebar_architecture.c \
             src/rebar_clash.c \
             src/terrain.c \
             src/heightmap_loader.c \
             src/noise_gen.c \
             src/terrain_paint.c \
             src/asset_db.c \
             src/asset_importer.c \
             src/scene_serializer.c \
             src/editor.c \
             src/palette.c \
             src/transform_tool.c \
             src/flythrough.c \
             src/ortho_export.c \
             src/web_exporter.c \
             src/lua_bindings.c \
             src/grid.c \
             src/shape_palette.c \
             src/extrude.c \
             src/surface_sketch.c \
             src/render_server.c \
             src/input_server.c \
             src/editor_ui.c \
             src/scene_tree.c \
             src/node.c \
             src/editor_viewport.c \
             src/editor_sidebar.c \
             src/editor_toolbar.c \
             src/editor_gizmo.c \
             src/spatial_memory.c \
             src/rwkv_wkv.c \
             src/rwkv_simd_avx2.c \
             src/rwkv_model.c \
             src/rwkv_ffn.c \
             src/prefab.c \
             src/instance_renderer.c \
             src/lod_manager.c \
             src/erosion.c \
             src/water.c \
             src/skybox.c \
             src/sun.c \
             src/ui_overlay.c \
             src/undo_system.c \
             src/pour_sequence.c \
             src/bridge_server.c \
             src/floor_plan.c \
             src/physics.c \
             src/logger.c \
             src/engine_core.c \
             src/crown_log.c \
             src/crown_display.c \
             src/crown_render_system.c \
             src/crown_game.c \
             src/imgui_bridge.c

ENGINE_CPP_SRC = src/frugally_bridge.cpp

IMGUI_SRC = third_party/imgui/imgui.cpp \
            third_party/imgui/imgui_draw.cpp \
            third_party/imgui/imgui_tables.cpp \
            third_party/imgui/imgui_widgets.cpp \
            third_party/imgui/backends/imgui_impl_opengl3.cpp \
            third_party/imgui/imgui_glue.cpp

ENGINE_C_OBJS   = $(patsubst src/%.c, build/%.o, $(ENGINE_SRC))
ENGINE_CPP_OBJS = $(patsubst src/%.cpp, build/%.o, $(ENGINE_CPP_SRC))
IMGUI_OBJS      = $(patsubst third_party/%.cpp, build/third_party/%.o, $(IMGUI_SRC))
ENGINE_OBJS     = $(ENGINE_C_OBJS) $(ENGINE_CPP_OBJS) $(IMGUI_OBJS)

ENGINE_TARGET = 3dai_engine

GRAPH_SRC = src/graph_tool.c src/vector3d.c
GRAPH_TARGET = 3dai_graph
BUBBLETEA_DIR = bubbletea
BUBBLETEA_TARGET = 3dai_tui

.PHONY: all engine debug graph bubbletea clean

all: engine graph bubbletea

engine: $(ENGINE_TARGET)

debug:
	@echo "=== BUILDING DEBUG WITH ASAN ==="
	$(MAKE) engine CFLAGS_ENGINE="$(CFLAGS_DEBUG)" CXXFLAGS_ENGINE="$(CXXFLAGS_DEBUG)" LDFLAGS_ENGINE="$(LDFLAGS_DEBUG)"

build/%.o: src/%.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS_ENGINE) -c $< -o $@

build/rwkv_simd_avx2.o: src/rwkv_simd_avx2.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS_ENGINE) -mavx2 -c $< -o $@

build/physics.o: src/physics.c
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS_ENGINE) -c $< -o $@

build/%.o: src/%.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS_ENGINE) -c $< -o $@

build/third_party/%.o: third_party/%.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS_ENGINE) -c $< -o $@

$(ENGINE_TARGET): $(ENGINE_OBJS)
	$(CXX) $(ENGINE_OBJS) -o $(ENGINE_TARGET) $(LDFLAGS_ENGINE)

graph: $(GRAPH_TARGET)

$(GRAPH_TARGET): $(GRAPH_SRC) include/graph_tool.h include/vector3d.h
	$(CC) $(CFLAGS_GRAPH) $(GRAPH_SRC) -DSTANDALONE_GRAPH_TOOL=1 -o $(GRAPH_TARGET) $(LDFLAGS_GRAPH)

bubbletea:
	cd $(BUBBLETEA_DIR) && go mod tidy && go build -o ../$(BUBBLETEA_TARGET)

clean:
	rm -rf build $(ENGINE_TARGET) $(GRAPH_TARGET) $(BUBBLETEA_TARGET)
	rm -f output/*.csv output/*.png output/*.obj output/*.json output/*.weightmap.png output/*.mp4 output/*.sparkline.png output/*.ppm output/*.pdf
	rm -f sim_data/*.dat
