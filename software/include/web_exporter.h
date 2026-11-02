#ifndef WEB_EXPORTER_H
#define WEB_EXPORTER_H
#pragma once

#include "scene_serializer.h"

#ifdef __cplusplus
extern "C" {
#endif

int web_export_gltf(const Scene* scene, const char* output_path);
int web_export_html(const Scene* scene, const char* output_path, const char* title);
int web_export_obj_with_mtl(const Scene* scene, const char* output_dir);

#ifdef __cplusplus
}
#endif

#endif /* WEB_EXPORTER_H */
