-- 3DAI Engine - Island Generation Configuration
-- Values are loaded into Lua global scope and read by engine at startup

-- Terrain dimensions
terrain_width = 1024
terrain_depth = 1024
terrain_scale = 1.0

-- Elevation & Water
mountain_height = 50.0
valley_depth = -2.0
water_level = 5.0

-- Biome Densities (0.0 to 1.0)
tree_density = 0.3
rock_density = 0.1
grass_density = 0.8
sand_density = 0.4

-- Generated Structures
building_count = 5
road_network_enabled = true
bridge_count = 2

-- Visual Settings
sky_type = "default"  -- Options: "default", "sunset", "storm", "night"
ambient_light = 0.4
fog_density = 0.02
water_shader_quality = "high"

-- Performance
max_visible_trees = 2000
lod_transition_distance = 80.0
shadow_resolution = 2048
