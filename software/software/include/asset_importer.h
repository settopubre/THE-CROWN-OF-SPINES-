#ifndef ASSET_IMPORTER_H
#define ASSET_IMPORTER_H

#include "mesh.h"
#include "vector3d.h"
#include "gpu_handle.h"

typedef struct {
    Mesh* mesh;
    Vec3 position;
    Quat rotation;
    Vec3 scale;
    float color[3];
    uint32_t asset_id;

    /* Material Properties */
    float ambient[3];
    float specular[3];
    float shininess;
    char diffuse_texture[256];
    GPUHandle texture_id;
} AssetInstance;

#ifdef __cplusplus
extern "C" {
#endif

AssetInstance* asset_importer_load(const char* obj_path);
AssetInstance* asset_importer_load_with_mtl(const char* obj_path, const char* mtl_path);
void asset_instance_destroy(AssetInstance* inst);
void asset_instance_set_transform(AssetInstance* inst, Vec3 pos, Quat rot, Vec3 scale);
void asset_instance_render(AssetInstance* inst, Mat4 view, Mat4 proj);

#ifdef __cplusplus
}
#endif

#endif /* ASSET_IMPORTER_H */
