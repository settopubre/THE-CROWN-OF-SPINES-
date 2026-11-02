#ifndef SHADERS_H
#define SHADERS_H

const char* vertex_shader_source = 
    "#version 330 core\n"
    "layout(location = 0) in vec3 aPos;\n"
    "layout(location = 1) in vec3 aNormal;\n"
    "layout(location = 2) in vec3 aColor;\n"
    "uniform mat4 uModel;\n"
    "uniform mat4 uView;\n"
    "uniform mat4 uProjection;\n"
    "out vec3 vColor;\n"
    "out vec3 vNormal;\n"
    "out vec3 vFragPos;\n"
    "void main() {\n"
    "    gl_Position = uProjection * uView * uModel * vec4(aPos, 1.0);\n"
    "    vFragPos = vec3(uModel * vec4(aPos, 1.0));\n"
    "    vNormal = mat3(transpose(inverse(uModel))) * aNormal;\n"
    "    vColor = aColor;\n"
    "}\n";

const char* fragment_shader_source = 
    "#version 330 core\n"
    "in vec3 vColor;\n"
    "in vec3 vNormal;\n"
    "in vec3 vFragPos;\n"
    "out vec4 FragColor;\n"
    "void main() {\n"
    "    vec3 lightDir = normalize(vec3(1.0, 1.0, 1.0));\n"
    "    float diff = max(dot(normalize(vNormal), lightDir), 0.0);\n"
    "    vec3 ambient = vec3(0.2, 0.2, 0.3);\n"
    "    vec3 baseColor = (vColor != vec3(0.0)) ? vColor : vec3(0.8, 0.6, 0.4);\n"
    "    vec3 result = ambient + diff * baseColor;\n"
    "    FragColor = vec4(result, 1.0);\n"
    "}\n";

#endif
