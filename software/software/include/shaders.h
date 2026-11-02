#ifndef SHADERS_H
#define SHADERS_H

static const char* vertex_shader_source = 
    "#version 330 core\n"
    "layout (location = 0) in vec3 aPos;\n"
    "layout (location = 1) in vec3 aNormal;\n"
    "layout (location = 2) in vec3 aColor;\n"
    "uniform mat4 uModel;\n"
    "uniform mat4 uView;\n"
    "uniform mat4 uProjection;\n"
    "out vec3 FragPos;\n"
    "out vec3 Normal;\n"
    "out vec3 VertexColor;\n"
    "void main() {\n"
    "    FragPos = vec3(uModel * vec4(aPos, 1.0));\n"
    "    Normal = mat3(transpose(inverse(uModel))) * aNormal;\n"
    "    VertexColor = aColor;\n"
    "    gl_Position = uProjection * uView * vec4(FragPos, 1.0);\n"
    "}\n";

static const char* fragment_shader_source = 
    "#version 330 core\n"
    "out vec4 FragColor;\n"
    "in vec3 FragPos;\n"
    "in vec3 Normal;\n"
    "in vec3 VertexColor;\n"
    "uniform vec3 uLightPos;\n"
    "uniform vec3 uViewPos;\n"
    "uniform vec3 uLightColor;\n"
    "uniform vec3 uObjectColor;\n"
    "void main() {\n"
    "    float ambientStrength = 0.2;\n"
    "    vec3 ambient = ambientStrength * uLightColor;\n"
    "    \n"
    "    vec3 norm = normalize(Normal);\n"
    "    vec3 lightDir = normalize(uLightPos - FragPos);\n"
    "    float diff = max(dot(norm, lightDir), 0.0);\n"
    "    vec3 diffuse = diff * uLightColor;\n"
    "    \n"
    "    float specularStrength = 0.5;\n"
    "    vec3 viewDir = normalize(uViewPos - FragPos);\n"
    "    vec3 halfwayDir = normalize(lightDir + viewDir);\n"
    "    float spec = pow(max(dot(norm, halfwayDir), 0.0), 32.0);\n"
    "    vec3 specular = specularStrength * spec * uLightColor;\n"
    "    \n"
    "    vec3 result = (ambient + diffuse + specular) * uObjectColor;\n"
    "    if (uObjectColor == vec3(0.0)) result = (ambient + diffuse + specular) * VertexColor;\n"
    "    FragColor = vec4(result, 1.0);\n"
    "}\n";

#endif /* SHADERS_H */
