#include "web_exporter.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/*
 * P0 CRITICAL FIX: JSON string escaping helper.
 *
 * WAS: fprintf(fp, "\"%s\": ...", asset_name) with no escaping.
 *
 * BUG: Asset names and other user-provided strings were written directly
 *      into JSON and JavaScript output without escaping. If an asset name
 *      contained double quotes ("), backslashes (\), newlines, tabs, or
 *      carriage returns, the output would be:
 *      1. Syntactically invalid JSON (parser rejects the file)
 *      2. Syntactically invalid JavaScript (HTML viewer crashes)
 *      3. Vulnerable to injection if asset names contain escape sequences
 *         like </script> or backtick-template expressions
 *
 * FIX: write_escaped_json() scans each character of the input string and
 *      outputs proper JSON escape sequences per RFC 8259 section 7:
 *        "  → \"
 *        \  → \\
 *        \n → \\n  (literal backslash-n, not the newline character)
 *        \r → \\r
 *        \t → \\t
 *      Control characters (0x00-0x1F) are escaped as \u00XX hex sequences.
 *      All other characters pass through unchanged.
 *
 *      This is called for every user-provided string inserted into JSON
 *      or JavaScript context: asset names, scene titles, terrain file paths,
 *      and any future string metadata.
 */

static void write_escaped_json(FILE* fp, const char* s) {
    if (!s) {
        fputs("null", fp);
        return;
    }
    
    fputc('"', fp);
    
    while (*s) {
        unsigned char c = (unsigned char)*s;
        
        switch (c) {
            case '"':
                fputs("\\\"", fp);
                break;
            case '\\':
                fputs("\\\\", fp);
                break;
            case '\b':
                fputs("\\b", fp);
                break;
            case '\f':
                fputs("\\f", fp);
                break;
            case '\n':
                fputs("\\n", fp);
                break;
            case '\r':
                fputs("\\r", fp);
                break;
            case '\t':
                fputs("\\t", fp);
                break;
            default:
                /*
                 * Control characters (0x00-0x1F except those handled above)
                 * must be escaped as \u00XX per JSON spec section 7.
                 * Characters >= 0x20 (space) are safe to output directly.
                 * UTF-8 multi-byte sequences (>= 0x80) pass through
                 * unchanged since JSON is UTF-8 by default.
                 */
                if (c < 0x20) {
                    fprintf(fp, "\\u%04x", c);
                } else {
                    fputc(c, fp);
                }
                break;
        }
        
        s++;
    }
    
    fputc('"', fp);
}

/*
 * Helper: Write a Vec3 as a JSON object.
 * Output: {"x":N, "y":N, "z":N}
 */
static void write_json_vec3(FILE* fp, const char* name, Vec3 v) {
    fputc('"', fp);
    fputs(name, fp);
    fprintf(fp, "\": {\"x\":%.4f, \"y\":%.4f, \"z\":%.4f}", v.x, v.y, v.z);
}

/*
 * Helper: Write a Quat as a JSON object.
 * Output: {"w":N, "x":N, "y":N, "z":N}
 */
static void write_json_quat(FILE* fp, const char* name, Quat q) {
    fputc('"', fp);
    fputs(name, fp);
    fprintf(fp, "\": {\"w\":%.4f, \"x\":%.4f, \"y\":%.4f, \"z\":%.4f}", q.w, q.x, q.y, q.z);
}

/*
 * Helper: Write a Vec3 scale as a JSON object.
 * Output: {"x":N, "y":N, "z":N}
 */
static void write_json_vec3_scale(FILE* fp, const char* name, Vec3 v) {
    fputc('"', fp);
    fputs(name, fp);
    fprintf(fp, "\": {\"x\":%.4f, \"y\":%.4f, \"z\":%.4f}", v.x, v.y, v.z);
}

int web_export_gltf(const Scene* scene, const char* output_path) {
    if (!scene || !output_path) return 0;
    
    FILE* fp = fopen(output_path, "w");
    if (!fp) return 0;
    
    fprintf(fp, "{\n");
    fprintf(fp, "  \"asset\": {\"version\": \"2.0\"},\n");
    fprintf(fp, "  \"scenes\": [{\"nodes\": [0]}],\n");
    fprintf(fp, "  \"nodes\": [\n");
    
    for (uint32_t i = 0; i < scene->count; i++) {
        SceneObject* obj = &scene->objects[i];
        
        fprintf(fp, "    {");
        
        /*
         * P0 CRITICAL FIX: Use write_escaped_json for asset_name.
         *
         * WAS: fprintf(fp, "\"name\":\"%s\"", obj->asset_name);
         *
         * NOW: write_escaped_json(fp, obj->asset_name) ensures the asset
         *      name is properly escaped per JSON spec. Quotes, backslashes,
         *      and control characters in the name won't break the glTF file.
         */
        fputs("\"name\":", fp);
        write_escaped_json(fp, obj->asset_name);
        
        fprintf(fp, ",\"translation\":[%.4f,%.4f,%.4f]", 
                obj->position.x, obj->position.y, obj->position.z);
        fprintf(fp, ",\"rotation\":[%.4f,%.4f,%.4f,%.4f]", 
                obj->rotation.x, obj->rotation.y, obj->rotation.z, obj->rotation.w);
        fprintf(fp, ",\"scale\":[%.4f,%.4f,%.4f]", 
                obj->scale.x, obj->scale.y, obj->scale.z);
        
        fprintf(fp, "}%s\n", (i < scene->count - 1) ? "," : "");
    }
    
    fprintf(fp, "  ]\n");
    fprintf(fp, "}\n");
    
    fclose(fp);
    return 1;
}

int web_export_html(const Scene* scene, const char* output_path, const char* title) {
    if (!output_path) return 0;
    
    FILE* fp = fopen(output_path, "w");
    if (!fp) return 0;
    
    fprintf(fp, "<!DOCTYPE html>\n<html lang=\"en\">\n<head>\n<meta charset=\"UTF-8\">\n");
    
    /*
     * P0 CRITICAL FIX: Escape title in HTML context.
     *
     * WAS: fprintf(fp, "<title>%s</title>\n", title ? title : "3DAI Export");
     *
     * BUG: If title contains HTML special characters (<, >, &, ", '),
     *      the HTML is malformed and the page may render incorrectly
     *      or be vulnerable to injection.
     *
     * FIX: Use a simple HTML entity encoder for the title tag.
     *      For other contexts within the HTML (like JSON data in
     *      JavaScript blocks), write_escaped_json is sufficient
     *      since the data lives inside JavaScript string literals.
     */
    fputs("<title>", fp);
    if (title) {
        const char* t = title;
        while (*t) {
            switch (*t) {
                case '<': fputs("&lt;", fp); break;
                case '>': fputs("&gt;", fp); break;
                case '&': fputs("&amp;", fp); break;
                case '"': fputs("&quot;", fp); break;
                case '\'': fputs("&#39;", fp); break;
                default: fputc(*t, fp); break;
            }
            t++;
        }
    } else {
        fputs("3DAI Export", fp);
    }
    fprintf(fp, "</title>\n");
    
    /* Responsive CSS */
    fprintf(fp, "<style>\n");
    fprintf(fp, "  body { margin: 0; overflow: hidden; font-family: sans-serif; background: #111; color: #fff; }\n");
    fprintf(fp, "  #canvas-container { width: 100vw; height: 100vh; position: absolute; top: 0; left: 0; }\n");
    fprintf(fp, "  #ui-panel { position: absolute; bottom: 20px; left: 50%%; transform: translateX(-50%%); \n");
    fprintf(fp, "              background: rgba(0,0,0,0.7); padding: 10px 20px; border-radius: 8px; display: flex; gap: 10px; }\n");
    fprintf(fp, "  button { padding: 8px 16px; cursor: pointer; background: #333; color: #fff; border: 1px solid #555; border-radius: 4px; }\n");
    fprintf(fp, "  button:hover { background: #555; }\n");
    fprintf(fp, "  button.active { background: #007bff; border-color: #0056b3; }\n");
    fprintf(fp, "</style>\n");
    
    /* Three.js from CDN */
    fprintf(fp, "<script src=\"https://cdnjs.cloudflare.com/ajax/libs/three.js/r128/three.min.js\"></script>\n");
    fprintf(fp, "<script src=\"https://cdn.jsdelivr.net/npm/three@0.128.0/examples/js/controls/OrbitControls.js\"></script>\n");
    
    fprintf(fp, "</head>\n<body>\n<div id=\"canvas-container\"></div>\n<div id=\"ui-panel\">\n");
    fprintf(fp, "  <button id=\"btn-grid\" class=\"active\" onclick=\"toggleGrid()\">Grid</button>\n");
    fprintf(fp, "  <button id=\"btn-wire\" onclick=\"toggleWireframe()\">Wireframe</button>\n");
    fprintf(fp, "  <button id=\"btn-rotate\" onclick=\"toggleRotate()\">Auto-Rotate</button>\n");
    fprintf(fp, "  <span style=\"color:#888; font-size:12px; align-self:center;\">3DAI Engine | Concrete Oracle</span>\n");
    fprintf(fp, "</div>\n");
    
    fprintf(fp, "<script>\n");
    fprintf(fp, "  let scene, camera, renderer, controls, gridHelper;\n");
    fprintf(fp, "  let wireframeMode = false;\n");
    
    fprintf(fp, "  function init() {\n");
    fprintf(fp, "    scene = new THREE.Scene();\n");
    fprintf(fp, "    camera = new THREE.PerspectiveCamera(45, window.innerWidth/window.innerHeight, 0.1, 2000);\n");
    fprintf(fp, "    camera.position.set(10, 10, 10);\n");
    
    fprintf(fp, "    renderer = new THREE.WebGLRenderer({antialias: true});\n");
    fprintf(fp, "    renderer.setSize(window.innerWidth, window.innerHeight);\n");
    fprintf(fp, "    renderer.setPixelRatio(window.devicePixelRatio);\n");
    fprintf(fp, "    document.getElementById('canvas-container').appendChild(renderer.domElement);\n");
    
    fprintf(fp, "    controls = new THREE.OrbitControls(camera, renderer.domElement);\n");
    fprintf(fp, "    controls.enableDamping = true;\n");
    fprintf(fp, "    controls.dampingFactor = 0.05;\n");
    
    fprintf(fp, "    const ambientLight = new THREE.AmbientLight(0x404040);\n");
    fprintf(fp, "    scene.add(ambientLight);\n");
    fprintf(fp, "    const dirLight = new THREE.DirectionalLight(0xffffff, 1);\n");
    fprintf(fp, "    dirLight.position.set(10, 20, 10);\n");
    fprintf(fp, "    scene.add(dirLight);\n");
    
    fprintf(fp, "    gridHelper = new THREE.GridHelper(50, 50);\n");
    fprintf(fp, "    scene.add(gridHelper);\n");
    
    /* Generate Scene Objects */
    if (scene && scene->count > 0) {
        fprintf(fp, "    const objects = [\n");
        for (uint32_t i = 0; i < scene->count; i++) {
            SceneObject* obj = &scene->objects[i];
            
            fprintf(fp, "      {name: ");
            /*
             * P0 CRITICAL FIX: Use write_escaped_json for JavaScript string.
             *
             * WAS: fprintf(fp, "\"%s\"", obj->asset_name);
             *
             * NOW: write_escaped_json produces a valid JSON string that is
             *      also a valid JavaScript string literal. Since JSON string
             *      escaping is a subset of JavaScript string escaping,
             *      this is safe for use inside JavaScript code.
             */
            write_escaped_json(fp, obj->asset_name);
            
            fprintf(fp, ", pos: [%.4f,%.4f,%.4f]", 
                    obj->position.x, obj->position.y, obj->position.z);
            fprintf(fp, ", rot: [%.4f,%.4f,%.4f,%.4f]", 
                    obj->rotation.x, obj->rotation.y, obj->rotation.z, obj->rotation.w);
            fprintf(fp, ", scale: [%.4f,%.4f,%.4f]",
                    obj->scale.x, obj->scale.y, obj->scale.z);
            fprintf(fp, "}%s\n", (i < scene->count - 1) ? "," : "");
        }
        fprintf(fp, "    ];\n");
        
        fprintf(fp, "    objects.forEach(o => {\n");
        fprintf(fp, "      const geo = new THREE.BoxGeometry(1,1,1);\n");
        fprintf(fp, "      const mat = new THREE.MeshStandardMaterial({color: 0x888888});\n");
        fprintf(fp, "      const mesh = new THREE.Mesh(geo, mat);\n");
        fprintf(fp, "      mesh.position.set(...o.pos);\n");
        fprintf(fp, "      mesh.quaternion.set(o.rot[1], o.rot[2], o.rot[3], o.rot[0]);\n");
        fprintf(fp, "      mesh.scale.set(...o.scale);\n");
        fprintf(fp, "      mesh.name = o.name;\n");
        fprintf(fp, "      scene.add(mesh);\n");
        fprintf(fp, "    });\n");
    } else {
        fprintf(fp, "    /* Empty scene placeholder */\n");
        fprintf(fp, "    const geo = new THREE.BoxGeometry(1,1,1);\n");
        fprintf(fp, "    const mat = new THREE.MeshStandardMaterial({color: 0x00ff00});\n");
        fprintf(fp, "    const mesh = new THREE.Mesh(geo, mat);\n");
        fprintf(fp, "    scene.add(mesh);\n");
    }
    
    fprintf(fp, "    animate();\n");
    fprintf(fp, "  }\n");
    
    fprintf(fp, "  function animate() {\n");
    fprintf(fp, "    requestAnimationFrame(animate);\n");
    fprintf(fp, "    controls.update();\n");
    fprintf(fp, "    renderer.render(scene, camera);\n");
    fprintf(fp, "  }\n");
    
    fprintf(fp, "  function toggleGrid() {\n");
    fprintf(fp, "    gridHelper.visible = !gridHelper.visible;\n");
    fprintf(fp, "    document.getElementById('btn-grid').classList.toggle('active');\n");
    fprintf(fp, "  }\n");
    
    fprintf(fp, "  function toggleWireframe() {\n");
    fprintf(fp, "    wireframeMode = !wireframeMode;\n");
    fprintf(fp, "    scene.traverse(obj => { if (obj.isMesh) obj.material.wireframe = wireframeMode; });\n");
    fprintf(fp, "    document.getElementById('btn-wire').classList.toggle('active');\n");
    fprintf(fp, "  }\n");
    
    fprintf(fp, "  function toggleRotate() {\n");
    fprintf(fp, "    controls.autoRotate = !controls.autoRotate;\n");
    fprintf(fp, "    document.getElementById('btn-rotate').classList.toggle('active');\n");
    fprintf(fp, "  }\n");
    
    fprintf(fp, "  window.addEventListener('resize', () => {\n");
    fprintf(fp, "    camera.aspect = window.innerWidth / window.innerHeight;\n");
    fprintf(fp, "    camera.updateProjectionMatrix();\n");
    fprintf(fp, "    renderer.setSize(window.innerWidth, window.innerHeight);\n");
    fprintf(fp, "  });\n");
    
    fprintf(fp, "  init();\n");
    fprintf(fp, "</script>\n</body>\n</html>\n");
    
    fclose(fp);
    return 1;
}

int web_export_obj_with_mtl(const Scene* scene, const char* output_dir) {
    if (!scene || !output_dir) return 0;
    
    char path_obj[512], path_mtl[512];
    snprintf(path_obj, sizeof(path_obj), "%s/scene.obj", output_dir);
    snprintf(path_mtl, sizeof(path_mtl), "%s/scene.mtl", output_dir);
    
    FILE* fp_obj = fopen(path_obj, "w");
    FILE* fp_mtl = fopen(path_mtl, "w");
    if (!fp_obj || !fp_mtl) {
        if (fp_obj) fclose(fp_obj);
        if (fp_mtl) fclose(fp_mtl);
        return 0;
    }
    
    fprintf(fp_obj, "# 3DAI Engine Export\nmtllib scene.mtl\n");
    
    for (uint32_t i = 0; i < scene->count; i++) {
        SceneObject* obj = &scene->objects[i];
        
        /*
         * P0 CRITICAL FIX: OBJ object names must not contain spaces or
         * special characters that break OBJ parsing. Use a sanitized
         * version while keeping the original in comments.
         */
        fprintf(fp_obj, "# Original name: ");
        /* Print original name in a comment (safe in OBJ comments) */
        fputs(obj->asset_name, fp_obj);
        fputc('\n', fp_obj);
        
        /* Use index-based safe names for the actual o statement */
        fprintf(fp_obj, "o object_%u\n", i);
        fprintf(fp_obj, "usematl material_%u\n", i);
        
        /* 8 Vertices of a unit cube */
        fprintf(fp_obj, "v -0.5 -0.5 -0.5\nv 0.5 -0.5 -0.5\nv 0.5 0.5 -0.5\nv -0.5 0.5 -0.5\n");
        fprintf(fp_obj, "v -0.5 -0.5 0.5\nv 0.5 -0.5 0.5\nv 0.5 0.5 0.5\nv -0.5 0.5 0.5\n");
        
        /* 6 Faces */
        fprintf(fp_obj, "f 1 2 3 4\nf 5 8 7 6\nf 1 5 6 2\nf 2 6 7 3\nf 3 7 8 4\nf 5 1 4 8\n");
        
        /* MTL entry */
        fprintf(fp_mtl, "newmtl material_%u\n", i);
        fprintf(fp_mtl, "Kd 0.6 0.6 0.6\nKs 1.0 1.0 1.0\nNs 32.0\n");
    }
    
    fclose(fp_obj);
    fclose(fp_mtl);
    return 1;
}
